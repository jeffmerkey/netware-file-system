
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

    Cleanup.c

Abstract:

    This module implements the File Cleanup routine for Nwfs called by the
    dispatch driver.

  Authors:

    Jeff Merkey (jeffmerkey@gmail.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

****************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_CLEANUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCommonCleanup)
#endif


NTSTATUS
NwCommonCleanup (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for cleanup of a file/directory called by both
    the fsd and fsp threads.

    Cleanup is invoked whenever the last handle to a file object is closed.
    This is different than the Close operation which is invoked when the last
    reference to a file object is deleted.

    The function of cleanup is to essentially "cleanup" the file/directory
    after a user is done with it.  The Fcb/Dcb remains around (because MM
    still has the file object referenced) but is now available for another
    user to open (i.e., as far as the user is concerned it is now closed).

    See close for a more complete description of what close does.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN NoCacheMap = TRUE;

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
    //  Get the file object out of the Irp and decode the type of open.
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    TypeOfOpen = NwDecodeFileObject( IrpContext,
                                     FileObject,
                                     &Fcb,
                                     &Ccb );

    //
    //  No work here for an UnopenedFile.
    //

    if (TypeOfOpen == UnopenedFileObject) {

        NwCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

        return STATUS_SUCCESS;
    }

    //
    //  Keep a local pointer to the Vcb.
    //

    Vcb = Fcb->Vcb;

    //
    //  Acquire the current file.
    //

    NwAcquireFcbExclusive( IrpContext, Fcb, FALSE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Set the flag in the FileObject to indicate that cleanup is complete.
        //

        SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

        //
        //  Case on the type of open that we are trying to cleanup.
        //

        switch (TypeOfOpen) {

        case UserDirectoryOpen:

            //
            //  Check if we need to complete any dir notify Irps on this file object.
            //

            FsRtlNotifyCleanup( Vcb->NotifySync,
                                &Vcb->DirNotifyList,
                                Ccb );

            break;

        case UserFileOpen:

            //
            //  Coordinate the cleanup operation with the oplock state.
            //  Oplock cleanup operations can always cleanup immediately so no
            //  need to check for STATUS_PENDING.
            //

            FsRtlCheckOplock( &Fcb->Oplock,
                              Irp,
                              IrpContext,
                              NULL,
                              NULL );

            //
            //  Unlock all outstanding file locks.
            //

            if (Fcb->FileLock != NULL) {

                FsRtlFastUnlockAll( Fcb->FileLock,
                                    FileObject,
                                    IoGetRequestorProcess( Irp ),
                                    NULL );
            }

            //
            //  Cleanup the cache map.
            //

            NoCacheMap = (BOOLEAN)(Fcb->FcbNonpaged->SegmentObject.SharedCacheMap == NULL);
            CcUninitializeCacheMap( FileObject, NULL, NULL );

            //
            //  Check the fast io state.
            //

            Fcb->IsFastIoPossible = NwIsFastIoPossible( Fcb );

            break;

        case UserVolumeOpen :

            break;

        default :

            NwBugCheck( TypeOfOpen, 0, 0 );
        }

        //
        //  We must clean up the share access at this time, since we may not
        //  get a Close call for awhile if the file was mapped through this
        //  File Object.
        //

        IoRemoveShareAccess( FileObject, &Fcb->ShareAccess );

        //
        //  If this file object has locked the volume then perform the unlock operation.
        //

        if (FileObject == Vcb->VolumeLockFileObject) {

            ClearFlag( Vcb->VcbState, VCB_STATE_LOCKED );
            ClearFlag( Vcb->Vpb->Flags, VPB_LOCKED );
            Vcb->VolumeLockFileObject = NULL;
        }

        //
        //  Now acquire our global lock in order to modify the fields in the
        //  in-memory structures.
        //

        NwLock();

        //
        //  Decrement the cleanup counts in the Vcb and Fcb.
        //

        NwDecrementHandleCounts( IrpContext, Fcb );

        //
        //  Decide here if we are to delay the close.
        //

        if ((Fcb->FileHandleCount == 0) &&
            (NoCacheMap) &&
            (TypeOfOpen != UserVolumeOpen)) {

            SetFlag( Fcb->FcbState, FCB_STATE_DELAY_CLOSE );
        }

        NwUnlock();

    } finally {

        NwReleaseFcb( IrpContext, Fcb );
    }

    //
    //  Since this is a normal termination then complete the request.
    //

    NwCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}

