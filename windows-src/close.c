
/***************************************************************************
*
*   Copyright (c) 1999-2022 Jeffrey V. Merkey
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jeffmerkey@gmail.com
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the Lesser GNU Public License 2.1 as
*   published by the Free Software Foundation.  (Windows NT/2000
*   is a registered trademark of Microsoft Corporation).
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
****************************************************************************

Module Name:

    Close.c

Abstract:

    This module implements the File Close routine for Nwfs called by the
    Fsd/Fsp dispatch routines.

    The close operation interacts with both the async and delayed close queues
    in the NwData structure.  Since close may be called recursively we may
    violate the locking order in acquiring the Vcb or Fcb.  In this case
    we may move the request to the async close queue.  If this is the last
    reference on the Fcb and there is a chance the user may reopen this
    file again soon we would like to defer the close.  In this case we
    may move the request to the delayed close queue.

    Once we are past the decode file operation there is no need for the
    file object.  If we are moving the request to either of the work
    queues then we remember all of the information from the file object and
    complete the request with STATUS_SUCCESS.  The Io system can then
    reuse the file object and we can complete the request when convenient.

    The async close queue consists of requests which we would like to
    complete as soon as possible.  They are queued using the original
    IrpContext where some of the fields have been overwritten with
    information from the file object.  We will extract this information,
    cleanup the IrpContext and then call the close worker routine.

    The delayed close queue consists of requests which we would like to
    defer the close for.  We keep size of this list within a range
    determined by the size of the system.  We let it grow to some maximum
    value and then shrink to some minimum value.  We allocate a small
    structure which contains the key information from the file object
    and use this information along with an IrpContext on the stack
    to complete the request.

  Authors:

    Jeff Merkey (jeffmerkey@gmail.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

***************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_CLOSE)

//
//  Local support routines
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCommonClose)
#pragma alloc_text(PAGE, NwFspClose)
#endif


NTSTATUS
NwCommonClose (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the Fsd entry for the close operation.  We decode the file
    object to find the Fcb and Ccb structures and type of open.  We call our
    internal worker routine to perform the actual work.  If the work wasn't
    completed then we post to one of our worker queues.  The Ccb isn't needed
    after this point so we delete the Ccb and return STATUS_SUCCESS to our
    caller in all cases.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    STATUS_SUCCESS

--*/

{
    TYPE_OF_OPEN TypeOfOpen;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN DeleteFcb = FALSE;
    BOOLEAN StartWorker = FALSE;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS.
    //

    if (IrpContext->Vcb == NULL) {

        NwCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Decode the file object to get the type of open and Fcb/Ccb.
    //

    TypeOfOpen = NwDecodeFileObject( IrpContext,
                                     IoGetCurrentIrpStackLocation( Irp )->FileObject,
                                     &Fcb,
                                     &Ccb );

    //
    //  No work to do for unopened file objects.
    //

    if (TypeOfOpen == UnopenedFileObject) {

        NwCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

        return STATUS_SUCCESS;
    }

    Vcb = Fcb->Vcb;

    //
    //  Lock the Vcb and decrement the reference counts.
    //

    NwLock();

    RemoveEntryList( &Ccb->CcbLinks );

    NwDecrementObjectCounts( Fcb );

    //
    //  Check for the last reference
    //

    if (Fcb->FileObjectCount == 0) {

        //
        //  If this is a user file or directory then check if we should post
        //  this to the delayed close queue.
        //

        if ((Vcb->VcbCondition == VcbMounted) &&
            FlagOn(Fcb->FcbState, FCB_STATE_DELAY_CLOSE)) {

            ClearFlag(Fcb->FcbState, FCB_STATE_DELAY_CLOSE);
            SetFlag(Fcb->FcbState, FCB_STATE_CLOSE_DELAYED);

            InsertTailList( &NwData.DelayedCloseQueue,
                            &Fcb->DelayedCloseLinks );

            NwData.DelayedCloseCount += 1;

            //
            //  If we are above our threshold then start the delayed
            //  close operation.
            //

            if (NwData.DelayedCloseCount > NwData.MaxDelayedCloseCount) {

                NwData.ReduceDelayedClose = TRUE;

                if (!NwData.FspCloseActive) {

                    NwData.FspCloseActive = TRUE;
                    StartWorker = TRUE;
                }
            }

        } else {

            NwUnlinkFcb( Fcb );
            DeleteFcb = TRUE;
        }
    }

    NwUnlock();

    //
    //  We can always deallocate the Ccb if present.
    //

    NwDeleteCcb( IrpContext, Ccb );

    if (DeleteFcb) {
        NwDeleteFcb( Fcb );
    }

    //
    //  Always complete this request with STATUS_SUCCESS.
    //

    NwCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    //
    //  Start the FspClose thread if we need to.
    //

    if (StartWorker) {

        ExQueueWorkItem( &NwData.CloseItem, CriticalWorkQueue );
    }

    //
    //  Always return STATUS_SUCCESS for closes.
    //

    return STATUS_SUCCESS;
}


VOID
NwFspClose (                                //  implemented in Close.c
    IN PVCB Vcb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to process the close queues in the NwData.  We will
    do as many of the delayed closes as we need to do in order to reach our
    low water mark.

Arguments:

    Vcb - If specified, remove all the delayed closes on only this Vcb.

Return Value:

    None

--*/

{
    PLIST_ENTRY Entry;

    PFCB Fcb;

    LIST_ENTRY FcbsToDeleteList;

    PAGED_CODE();

    //
    //  We must disable APCs since we are in an Ex Worker thread.  Note
    //  that nothing in this routine (or anything called beneath us) can
    //  raise an exception.
    //

    FsRtlEnterFileSystem();

    //
    //  Continue processing until there are no more closes to process.
    //

    InitializeListHead( &FcbsToDeleteList );

    NwLock();

    Entry = NwData.DelayedCloseQueue.Flink;

    while ((NwData.DelayedCloseCount > NwData.MinDelayedCloseCount) &&
           (Entry != &NwData.DelayedCloseQueue)) {

        Fcb = CONTAINING_RECORD(Entry, FCB, DelayedCloseLinks);

        Entry = Entry->Flink;

        if (ARGUMENT_PRESENT(Vcb) && (Fcb->Vcb != Vcb)) {
            continue;
        }

        ASSERT( Fcb->FileObjectCount == 0 );
        ASSERT( FlagOn(Fcb->FcbState, FCB_STATE_CLOSE_DELAYED) );

        NwUnlinkFcb(Fcb);

        InsertTailList( &FcbsToDeleteList, &Fcb->FcbLinks );
    }

    NwData.FspCloseActive = FALSE;
    NwData.ReduceDelayedClose = FALSE;

    NwUnlock();

    Entry = FcbsToDeleteList.Flink;

    while (Entry != &FcbsToDeleteList) {

        Fcb = CONTAINING_RECORD(Entry, FCB, FcbLinks);

        ASSERT( Fcb->FileObjectCount == 0 );
        ASSERT( FlagOn(Fcb->FcbState, FCB_STATE_CLOSE_DELAYED) );

        Entry = Entry->Flink;

        NwDeleteFcb( Fcb );
    }

    //
    //  Enable APCs
    //

    FsRtlExitFileSystem();

    return;
}

