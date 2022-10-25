
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

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Nwfs called
    by the Fsd/Fsp dispatch drivers.

  Authors:

    Jeff Merkey (jeffmerkey@gmail.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

****************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_FSCTRL)

//
//  Local constants
//

BOOLEAN NwDisable = TRUE;

//
//  Local support routines
//

NTSTATUS
NwUserFsctl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCommonFsControl)
#pragma alloc_text(PAGE, NwIsPathnameValid)
#pragma alloc_text(PAGE, NwIsVolumeMounted)
#pragma alloc_text(PAGE, NwLockVolume)
#pragma alloc_text(PAGE, NwMountVolume)
#pragma alloc_text(PAGE, NwOplockRequest)
#pragma alloc_text(PAGE, NwUnlockVolume)
#pragma alloc_text(PAGE, NwUserFsctl)
#pragma alloc_text(PAGE, NwVerifyVolume)
#endif


NTSTATUS
NwCommonFsControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_USER_FS_REQUEST:

        Status = NwUserFsctl( IrpContext, Irp );
        break;

    case IRP_MN_MOUNT_VOLUME:

        Status = NwMountVolume( IrpContext, Irp );
        break;

    case IRP_MN_VERIFY_VOLUME:

        Status = NwVerifyVolume( IrpContext, Irp );
        break;

    default:

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NwUserFsctl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the common routine for implementing the user's requests made
    through NtFsControlFile.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    //  Special case for the main device object.
    //

    if (IrpSp->DeviceObject == NwData.FileSystemDeviceObject) {

        if (IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_QUERY_ALL_DISKS) {

            return NwProcessVolumeLayout( IrpContext, Irp, IrpSp );

        } else {

            NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    //
    //  Case on the control code.
    //

    switch ( IrpSp->Parameters.FileSystemControl.FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1 :
    case FSCTL_REQUEST_OPLOCK_LEVEL_2 :
    case FSCTL_REQUEST_BATCH_OPLOCK :
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE :
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
    case FSCTL_OPLOCK_BREAK_NOTIFY :
    case FSCTL_OPLOCK_BREAK_ACK_NO_2 :
    case FSCTL_REQUEST_FILTER_OPLOCK :

        Status = NwOplockRequest( IrpContext, Irp );
        break;

    case FSCTL_LOCK_VOLUME :

        Status = NwLockVolume( IrpContext, Irp );
        break;

    case FSCTL_UNLOCK_VOLUME :

        Status = NwUnlockVolume( IrpContext, Irp );
        break;

    case FSCTL_DISMOUNT_VOLUME :

        Status = NwDismountVolume( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_MOUNTED :

        Status = NwIsVolumeMounted( IrpContext, Irp );
        break;

    case FSCTL_IS_PATHNAME_VALID :

        Status = NwIsPathnameValid( IrpContext, Irp );
        break;

    //
    //  We don't support any of the other known or unknown requests.
    //

    default:

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NwMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the normal entry point for a mount operation of a file
    system.  However, because Netware volumes can span multiple partitions,
    we cannot use the built in mount feature.  Indeed, this function should
    never even be called since we have never registered with the I/O system
    as a file system.  The actual mount of Netware volumes is accomplished 
    with the NwProcessVolumeLayout() call.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    IrpSp->Parameters.MountVolume.Vpb->DeviceObject = NULL;

    NwCompleteRequest( IrpContext, Irp, STATUS_UNRECOGNIZED_VOLUME );
    return STATUS_UNRECOGNIZED_VOLUME;
}


//
//  Local support routine
//

NTSTATUS
NwVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine usually performs the verify volume operation for a file system.
    Again though, we should not see this since we don't use the build in 
    verification system.  However in any case, do something reasonable.
    
Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PVPB Vpb = IrpSp->Parameters.VerifyVolume.Vpb;

    //
    //  We check that we are talking to a NwDisk device.
    //

    ASSERT( Vpb->RealDevice->DeviceType == FILE_DEVICE_DISK );
    ASSERT( FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT ));

    //
    //  Verifies are just no-oped right now, i.e set the flag and say no.
    //

    ClearFlag( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

    NwCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
NwOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine to handle oplock requests made via the
    NtFsControlFile call.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PFCB Fcb;
    PCCB Ccb;

    ULONG OplockCount = 0;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    //  We only permit oplock requests on files.
    //

    if (NwDecodeFileObject( IrpContext,
                            IrpSp->FileObject,
                            &Fcb,
                            &Ccb ) != UserFileOpen ) {

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Make this a waitable Irpcontext so we don't fail to acquire
    //  the resources.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );

    //
    //  Switch on the function control code.  We grab the Fcb exclusively
    //  for oplock requests, shared for oplock break acknowledgement.
    //

    switch (IrpSp->Parameters.FileSystemControl.FsControlCode) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1 :
    case FSCTL_REQUEST_OPLOCK_LEVEL_2 :
    case FSCTL_REQUEST_BATCH_OPLOCK :
    case FSCTL_REQUEST_FILTER_OPLOCK :

        NwAcquireFcbExclusive( IrpContext, Fcb, FALSE );

        if (IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {

            if (Fcb->FileLock != NULL) {

                OplockCount = (ULONG) FsRtlAreThereCurrentFileLocks( Fcb->FileLock );
            }

        } else {

            OplockCount = Fcb->FileHandleCount;
        }

        break;

    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        NwAcquireFcbShared( IrpContext, Fcb, FALSE );
        break;

    default:

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Use a try finally to free the Fcb.
    //

    try {

        //
        //  Verify that the Vcb is in a usable condition.
        //

        if (Fcb->Vcb->VcbCondition != VcbMounted) {
            try_return( Status = STATUS_FILE_INVALID );
        }

        //
        //  Call the FsRtl routine to grant/acknowledge oplock.
        //

        Status = FsRtlOplockFsctrl( &Fcb->Oplock,
                                    Irp,
                                    OplockCount );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->IsFastIoPossible = NwIsFastIoPossible( Fcb );

        //
        //  The oplock package will complete the Irp.
        //

        Irp = NULL;

    try_exit: NOTHING;
    } finally {

        //
        //  Release all of our resources
        //

        NwReleaseFcb( IrpContext, Fcb );
    }

    //
    //  Complete the request if there was no exception.
    //

    NwCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
NwLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the lock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (NwDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb ) != UserVolumeOpen) {

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Vcb.
    //

    Vcb = Fcb->Vcb;
    NwAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    try {

        NwAcquireFcbExclusive( IrpContext, Fcb, FALSE );

        //
        //  Verify that the Vcb is in a usable condition.
        //

        if (Vcb->VcbCondition != VcbMounted) {
            try_return( Status = STATUS_FILE_INVALID );
        }

        //
        //  If the volume is already locked then complete with success if this file
        //  object has the volume locked, fail otherwise.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) {

            Status = STATUS_ACCESS_DENIED;

            if (Vcb->VolumeLockFileObject == IrpSp->FileObject) {

                Status = STATUS_SUCCESS;
            }

        //
        //  If the cleanup count for the volume is greater than 1 then this request
        //  will fail.
        //

        } else if (Vcb->FileHandleCount > 1) {

            Status = STATUS_ACCESS_DENIED;

        //
        //  We will try to get rid of all of the user references.  If there is only one
        //  remaining after the purge then we can allow the volume to be locked.
        //

        } else {

            Status = NwPurgeVolume( IrpContext, Vcb );

            if (NT_SUCCESS(Status)) {

                if (Vcb->FcbCount > NWFS_RESIDUAL_FCB_COUNT) {

                    Status = STATUS_ACCESS_DENIED;

                } else {

                    SetFlag( Vcb->Vpb->Flags, VPB_LOCKED );
                    SetFlag( Vcb->VcbState, VCB_STATE_LOCKED );
                    Vcb->VolumeLockFileObject = IrpSp->FileObject;
                    Status = STATUS_SUCCESS;
                }
            }
        }

    try_exit: NOTHING;
    } finally {

        //
        //  Release the Vcb.
        //

        if (ExIsResourceAcquiredExclusive(Fcb->Resource)) {
            NwReleaseFcb( IrpContext, Fcb);
        }
        NwReleaseVcb( IrpContext, Vcb );
    }

    //
    //  Complete the request if there haven't been any exceptions.
    //

    NwCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
NwUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the unlock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (NwDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb ) != UserVolumeOpen ) {

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Vcb.
    //

    Vcb = Fcb->Vcb;

    NwAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        NwAcquireFcbExclusive( IrpContext, Fcb, FALSE );

        //
        //  We won't check for a valid Vcb for this request.  An unlock will always
        //  succeed on a locked volume.
        //

        if (IrpSp->FileObject == Vcb->VolumeLockFileObject) {

            ClearFlag( Vcb->VcbState, VCB_STATE_LOCKED );
            ClearFlag( Vcb->Vpb->Flags, VPB_LOCKED );
            Vcb->VolumeLockFileObject = NULL;
            Status = STATUS_SUCCESS;
        }

    } finally {

        //
        //  Release all of our resources
        //

        if (ExIsResourceAcquiredExclusive(Fcb->Resource)) {
            NwReleaseFcb( IrpContext, Fcb);
        }
        NwReleaseVcb( IrpContext, Vcb );
    }

    //
    //  Complete the request if there haven't been any exceptions.
    //

    NwCompleteRequest( IrpContext, Irp, Status );
    return Status;
}



//
//  Local support routine
//

NTSTATUS
NwDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the dismount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.  We only dismount a volume
    which has been locked.  The intent here is that someone has locked the
    volume (they are the only remaining handle).
    
Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PDEVICE_OBJECT NewTargetDeviceObject;

    KIRQL SavedIrql;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;
    PVPB Vpb;

    PAGED_CODE();

    if (NwDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb ) != UserVolumeOpen ) {

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Vcb.
    //

    Vcb = Fcb->Vcb;
    Vpb = Vcb->Vpb;

    NwAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    try {

        NwAcquireFcbExclusive( IrpContext, Fcb, FALSE );

        if (Vcb->VolumeLockFileObject != IrpSp->FileObject) {

            try_return( Status = STATUS_NOT_LOCKED );
        }

        //
        //  Make sure nobody is enroute to the file system.
        //

        IoAcquireVpbSpinLock( &SavedIrql );

        ASSERT(FlagOn(Vpb->Flags, VPB_LOCKED));

        if (Vpb->ReferenceCount != 1) {
            IoReleaseVpbSpinLock( SavedIrql );
            try_return( Status = STATUS_SHARING_VIOLATION );
        }

        IoReleaseVpbSpinLock( SavedIrql );

        //
        //  Make all the delayed closes on this Vcb go away.
        //

        NwFspClose( Vcb );

        //
        //  Now we mark the volume as dismounting, attach to the pseudo
        //  device, then delete the pseudo device.
        //
        //  The close of the DASD handle should cause the pseudo to become
        //  deleted (uness there are direct device opens to the pseudo device,
        //  in which case the delete will be delayed until they are closed).
        //  Since we are attached, we will get a DeleteDevice fast-io callback
        //  which we can use to free the Netware resources and our own
        //  resources (include our own volume device object).
        //
        //  See NwDetachDevice() for more details.
        //

        Vcb->VcbCondition = VcbDismountInProgress;

        NewTargetDeviceObject =
            IoAttachDeviceToDeviceStack( IrpSp->DeviceObject,
                                         Vcb->TargetDeviceObject );

        if (NewTargetDeviceObject != Vcb->TargetDeviceObject) {
            try_return( Status = STATUS_INVALID_DEVICE_STATE );
        }

        IoDeleteDevice( Vcb->TargetDeviceObject );

        Status = STATUS_SUCCESS;

    try_exit:  NOTHING;
    } finally {

        //
        //  Release all of our resources
        //

        if (ExIsResourceAcquiredExclusive(Fcb->Resource)) {
            NwReleaseFcb( IrpContext, Fcb);
        }
        NwReleaseVcb( IrpContext, Vcb );
    }

    //
    //  Complete the request if there haven't been any exceptions.
    //

    NwCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
NwIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if a volume is currently mounted.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFCB Fcb;
    PCCB Ccb;
    NTSTATUS Status = STATUS_FILE_INVALID;

    PAGED_CODE();

    //
    //  Decode the file object.
    //

    (VOID)NwDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb );

    //
    //  See if the Vcb is in a usable condition.
    //

    if ((Fcb != NULL) &&
        (Fcb->Vcb->VcbCondition == VcbMounted)) {
        
        Status = STATUS_SUCCESS;
    }

    NwCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NwIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if pathname is a valid NWFS pathname.
    We always succeed this request.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    None

--*/

{
    PAGED_CODE();

    NwCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}

