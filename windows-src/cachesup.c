
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

    Cache.c

  Abstract:

    This module implements the cache management routines for the Nwfs
    FSD and FSP, by calling the Common Cache Manager.

  Authors:

    Jeff Merkey (jeffmerkey@gmail.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

***************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_CACHESUP)

//
//  Local debug trace level
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCompleteMdl)
#pragma alloc_text(PAGE, NwPurgeVolume)
#endif


NTSTATUS
NwCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the function of completing Mdl reads.
    It should be called only from NwFsdRead.

Arguments:

    Irp - Supplies the originating Irp.

Return Value:

    NTSTATUS - Will always be STATUS_SUCCESS.

--*/

{
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    //
    // Do completion processing.
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    CcMdlReadComplete( FileObject, Irp->MdlAddress );

    //
    // Mdl is now deallocated.
    //

    Irp->MdlAddress = NULL;

    //
    // Complete the request and exit right away.
    //

    NwCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}


NTSTATUS
NwPurgeVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to purge the volume.  The purpose is to make all the
    stale file objects in the system go away in order to lock the volume.

    The Vcb is already acquired exclusively.  We will lock out all file
    operations by acquiring the global file resource.  Then we will walk
    through all of the Fcb's and perform the purge.

Arguments:

    Vcb - Vcb for the volume to purge.

Return Value:

    NTSTATUS - The first failure of the purge operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PFCB PrevFcb = NULL;
    PFCB ThisFcb;

    PLIST_ENTRY Entry;

    LIST_ENTRY FcbsToDeleteList;

    PAGED_CODE();

    InitializeListHead( &FcbsToDeleteList );

    //
    //  Lock to lookup the next Fcb.
    //

    NwLock();

    //
    //  Run through all the Fcbs on the volume.
    //

    ThisFcb = CONTAINING_RECORD(Vcb->FcbList.Flink, FCB, FcbLinks);

    Entry = Vcb->FcbList.Flink;

    while (Entry != &Vcb->FcbList) {

        ThisFcb = CONTAINING_RECORD(Entry, FCB, FcbLinks);

        if (FlagOn(ThisFcb->FcbState, FCB_STATE_CLOSE_DELAYED)) {

            ASSERT( ThisFcb->FileObjectCount == 0 );

            //
            //  We need to delete this Fcb, first take it out of view before
            //  dropping the lock.
            //

            Entry = Entry->Flink;

            NwUnlinkFcb( ThisFcb );
            InsertTailList( &FcbsToDeleteList, &ThisFcb->FcbLinks );

            continue;
        }

        ASSERT( ThisFcb->FileObjectCount != 0 );

        //
        //  Bias this Fcb's FileObject count so that we know it won't be the
        //  victim of a recursive close.
        //

        NwIncrementObjectCounts(IrpContext, ThisFcb);

        //
        //  Now release the Vcb lock.  We know ThisFcb cannot go away.
        //

        NwUnlock();

        //
        //  If there is a image section then see if that can be closed.
        //

        if (ThisFcb->FcbNonpaged->SegmentObject.ImageSectionObject != NULL) {

            MmFlushImageSection( &ThisFcb->FcbNonpaged->SegmentObject, MmFlushForWrite );
        }

        //
        //  If there is a data section then purge this.  If there is an image
        //  or user mapped section then we won't be able to.  Remember this
        //  if it is our first error.
        //

        if ((ThisFcb->FcbNonpaged->SegmentObject.DataSectionObject != NULL) &&
            !CcPurgeCacheSection( &ThisFcb->FcbNonpaged->SegmentObject,
                                   NULL,
                                   0,
                                   FALSE ) &&
            (Status == STATUS_SUCCESS)) {

            Status = STATUS_UNABLE_TO_DELETE_SECTION;
        }

        //
        //  Now grab the Vcb lock again to get the next Fcb and to check
        //  if we got a close.
        //

        PrevFcb = ThisFcb;

        NwLock();

        Entry = Entry->Flink;

        //
        //  Now unbias the FileObject count.
        //

        NwDecrementObjectCounts( PrevFcb );

        if (PrevFcb->FileObjectCount == 0) {

            //
            //  We need to delete this Fcb, first take it out of view before
            //  dropping the lock.
            //

            NwUnlinkFcb( PrevFcb );
            InsertTailList( &FcbsToDeleteList, &PrevFcb->FcbLinks );
        }
    }

    NwUnlock();


    //
    //  Now walk through all the FCBs that we found to delete and do the deed.
    //

    Entry = FcbsToDeleteList.Flink;

    while (Entry != &FcbsToDeleteList) {

        ThisFcb = CONTAINING_RECORD(Entry, FCB, FcbLinks);

        ASSERT( ThisFcb->FileObjectCount == 0 );

        Entry = Entry->Flink;

        NwDeleteFcb( ThisFcb );
    }

    return Status;
}

