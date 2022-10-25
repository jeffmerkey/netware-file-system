
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

    StrucSup.c

Abstract:

    This module implements the Nwfs in-memory data structure manipulation
    routines

  Authors:

    Jeff Merkey (jeffmerkey@gmail.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

****************************************************************************/

#include "NwProcs.h"
#include "Fenris.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_STRUCSUP)

//
//  Local macros
//

//
//  PFCB
//  NwAllocateFcb (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  NwDeallocateFcb (
//      IN PFCB Fcb
//      );
//
//  PFCB_NONPAGED
//  NwAllocateFcbNonpaged (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  NwDeallocateFcbNonpaged (
//      IN PFCB_NONPAGED FcbNonpaged
//      );
//
//  PCCB
//  NwAllocateCcb (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  NwDeallocateCcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PCCB Ccb
//      );
//
//  PFILE_LOCK
//  NwAllocateFileLock (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  NwDeallocateFileLock (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFILE_LOCK FileLock
//      );
//

#define NwAllocateFcb(IC) \
    FsRtlAllocatePoolWithTag( NwPagedPool, sizeof( FCB ), TAG_FCB )

#define NwDeallocateFcb(F) \
    ExFreePool( F )

#define NwAllocateFcbNonpaged(IC) \
    FsRtlAllocatePoolWithTag( NwNonPagedPool, sizeof( FCB_NONPAGED ), TAG_FCB_NONPAGED )

#define NwDeallocateFcbNonpaged(FNP) \
    ExFreePool( FNP )

#define NwAllocateCcb(IC) \
    FsRtlAllocatePoolWithTag( NwPagedPool, sizeof( CCB ), TAG_CCB )

#define NwDeallocateCcb(IC,C) \
    ExFreePool( C )

#define NwAllocateFileLock(IC) \
    ExAllocatePoolWithTag( NwPagedPool, sizeof( FILE_LOCK ), TAG_FILE_LOCK )

#define NwDeallocateFileLock(IC,FL) \
    ExFreePool( FL )

//
//  Local support routines
//

PFCB_NONPAGED
NwCreateFcbNonpaged (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NwDeleteFcbNonpaged (
    IN PFCB_NONPAGED FcbNonpaged
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCleanupIrpContext)
#pragma alloc_text(PAGE, NwCreateCcb)
#pragma alloc_text(PAGE, NwCreateFcb)
#pragma alloc_text(PAGE, NwCreateFcbNonpaged)
#pragma alloc_text(PAGE, NwCreateFileLock)
#pragma alloc_text(PAGE, NwCreateIrpContext)
#pragma alloc_text(PAGE, NwDeleteCcb)
#pragma alloc_text(PAGE, NwDeleteFcb)
#pragma alloc_text(PAGE, NwDeleteFcbNonpaged)
#pragma alloc_text(PAGE, NwDeleteFileLock)
#pragma alloc_text(PAGE, NwDeleteVcb)
#pragma alloc_text(PAGE, NwInitializeVcb)
#endif

VOID
NwInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN ULONG VolumeNumber,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PDEVICE_OBJECT PhysicalPartition,
    IN PVPB Vpb,
    IN ULONG TotalClusters,
    IN ULONG FreeClusters,
    IN ULONG SectorsPerCluster
    )

/*++

Routine Description:

    This routine initializes and inserts a new Vcb record into the in-memory
    data structure.  The Vcb record "hangs" off the end of the Volume device
    object and must be allocated by our caller.

Arguments:

    Vcb - Supplies the address of the Vcb record being initialized.

    TargetDeviceObject - Supplies the address of the target device object to
        associate with the Vcb record.  This is the pseudo device we create.

    Vpb - Supplies the address of the Vpb to associate with the Vcb record.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  We start by first zeroing out all of the VCB, this will guarantee
    //  that any stale data is wiped clean.
    //

    RtlZeroMemory( Vcb, sizeof( VCB ));

    //
    //  Set the proper node type code and node byte size.
    //

    Vcb->NodeTypeCode = NWFS_NTC_VCB;
    Vcb->NodeByteSize = sizeof( VCB );

    //
    //  Initialize the DirNotify structures.  Do this first so there is
    //  no cleanup if it raises.  Nothing else below will fail with
    //  a raise.
    //

    InitializeListHead( &Vcb->DirNotifyList );
    FsRtlNotifyInitializeSync( &Vcb->NotifySync );

    //
    //  Initialize the resource variable for the Vcb.
    //

    ExInitializeResource( &Vcb->VcbResource );

    //
    //  Insert this Vcb record on the NwData.VcbQueue.
    //

    InsertTailList( &NwData.VcbQueue, &Vcb->VcbLinks );

    //
    //  This is a the list of all FCBs on the volume.
    //

    InitializeListHead( &Vcb->FcbList );

    //
    //  Set the Target Device Object, Partition Device Object, and Vpb fields.
    //

    Vcb->TargetDeviceObject = TargetDeviceObject;
    Vcb->PhysicalPartition = PhysicalPartition;
    Vcb->Vpb = Vpb;

    //
    //  Set the removable media flag based on the real device's characteristics
    //  and if removable, lock it in until we dismount.  We ignore any errors.
    //

    if (FlagOn( Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA )) {

        SetFlag( Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA );

        (VOID)NwToggleMediaEjectDisable( PhysicalPartition, TRUE );
    }

    //
    //  Show that we have a mount in progress.
    //

    Vcb->VcbCondition = VcbMountInProgress;

    //
    //  Add the new Netware values.
    //

    Vcb->VolumeNumber = VolumeNumber;
    Vcb->SectorsPerCluster = SectorsPerCluster;
    Vcb->TotalClusters = TotalClusters;
    Vcb->FreeClusters = FreeClusters;

    return;
}


VOID
NwDeleteVcb (
    IN OUT PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to delete a Vcb which failed mount or has been
    dismounted.  The dismount code should have already removed all of the
    user opened Fcb's.  We do nothing here but clean up other auxilary
    structures and finally delete the device.

Arguments:

    Vcb - Vcb to delete.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT_EXCLUSIVE_NWDATA;

    //
    //  ASSERT that there are no open files on this Vcb.
    //

    ASSERT(NodeType(Vcb) == NWFS_NTC_VCB);
    ASSERT(Vcb->VcbCondition == VcbDismountInProgress);
    ASSERT(Vcb->FcbCount <= NWFS_RESIDUAL_FCB_COUNT);
    ASSERT(Vcb->FileObjectCount <= NWFS_RESIDUAL_FCB_COUNT);

    if ((Vcb->FcbCount > NWFS_RESIDUAL_FCB_COUNT) ||
        (Vcb->FileObjectCount > NWFS_RESIDUAL_FCB_COUNT)) {

        NwBugCheck( (ULONG)Vcb, Vcb->FcbCount, Vcb->FileObjectCount );
    }

    //
    //  Delete the special DASD and Root Dir Fcbs.
    //

    if (Vcb->VolumeDasdFcb) {

        NwLock();
        NwDecrementObjectCounts( Vcb->VolumeDasdFcb );
        NwUnlinkFcb( Vcb->VolumeDasdFcb );
        NwUnlock();

        NwDeleteFcb( Vcb->VolumeDasdFcb );
    }

    if (Vcb->RootDirFcb) {

        NwLock();
        NwDecrementObjectCounts( Vcb->RootDirFcb );
        NwUnlinkFcb( Vcb->RootDirFcb );
        NwUnlock();

        NwDeleteFcb( Vcb->RootDirFcb );
    }

    ASSERT(Vcb->FcbCount == 0);
    ASSERT(Vcb->FileObjectCount == 0);
    ASSERT(Vcb->Vpb);

    //
    //  Tell the NWFS code that we are dismounting a volume.
    //

    if (Vcb->Vpb) {

        ULONG i;
        UCHAR VolumeName[(MAXIMUM_VOLUME_LABEL_LENGTH / sizeof(WCHAR)) + 1];

        for (i=0; i < Vcb->Vpb->VolumeLabelLength/sizeof(WCHAR); i++) {
            VolumeName[i] = (UCHAR)Vcb->Vpb->VolumeLabel[i];
        }
        VolumeName[i] = 0;

        DbgPrint("NWFSRO: Dismounting Volume: '%s'\n", &VolumeName[0]);

        (VOID)DismountVolume(&VolumeName[0]);

        ClearFlag( Vcb->Vpb->Flags, VPB_MOUNTED );
    }

    //
    //  Remove this entry from the global queue.
    //

    RemoveEntryList( &Vcb->VcbLinks );

    //
    //  Delete the Vcb resource.
    //

    ExDeleteResource( &Vcb->VcbResource );

    //
    //  Uninitialize the notify structures.
    //

    if (Vcb->NotifySync != NULL) {

        FsRtlNotifyUninitializeSync( &Vcb->NotifySync );
    }

    //
    //  If the media was removable, unlock it at this time.  We ignore any
    //  errors.
    //

    if (FlagOn( Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA )) {

        (VOID)NwToggleMediaEjectDisable( Vcb->PhysicalPartition, FALSE );
    }

    //
    //  Now delete the volume device object.
    //

    IoDeleteDevice( (PDEVICE_OBJECT) CONTAINING_RECORD( Vcb,
                                                        VOLUME_DEVICE_OBJECT,
                                                        Vcb ));

    return;
}


PFCB
NwCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN FILE_ID FileId
    )

/*++

Routine Description:

    This routine is called to create a new Fcb and do some basic initialization.

Arguments:

    FileId - This is the Id for the target Fcb.

Return Value:

    PFCB - The Fcb found in the table or created if needed.

--*/

{
    PFCB Fcb = NULL;
    BOOLEAN UnwindFreeMcb = FALSE;

    PAGED_CODE();

    try {

        //
        //  Allocate and initialize the structure.
        //

        Fcb = NwAllocateFcb( IrpContext );

        RtlZeroMemory( Fcb, sizeof( FCB ) );

        Fcb->NodeTypeCode = NWFS_NTC_FCB;
        Fcb->NodeByteSize = sizeof( FCB );

        Fcb->FcbState = FCB_STATE_INITIALIZED;

        //
        //  Now do the common initialization.
        //

        Fcb->Vcb = IrpContext->Vcb;
        Fcb->FileId = FileId;

        //
        //  Now create the non-paged section object.
        //

        Fcb->FcbNonpaged = NwCreateFcbNonpaged( IrpContext );

        //
        //  Allocate the Mcb and initialize its mutex.
        //

        FsRtlInitializeMcb( &Fcb->Mcb, PagedPool );
        UnwindFreeMcb = TRUE;

        ExInitializeFastMutex( &Fcb->McbMutex );

        Fcb->Resource = &Fcb->FcbNonpaged->Resource;

        //
        //  Initialize the Ccb links.
        //

        InitializeListHead( &Fcb->CcbList );

    } finally {

        //
        //  Something went wrong, back out the create.
        //

        if (AbnormalTermination() && Fcb) {
            if (Fcb->FcbNonpaged) {
                NwDeleteFcbNonpaged( Fcb->FcbNonpaged );
            }

            if (UnwindFreeMcb) {
                FsRtlUninitializeMcb( &Fcb->Mcb );
            }

            NwDeallocateFcb( Fcb );
        }
    }

    //
    //  Insert ourselves.  Note that nothing after this can fail.
    //

    NwLock();

    NwIncrementFcbCount( IrpContext, Fcb->Vcb );
    InsertTailList( &Fcb->Vcb->FcbList, &Fcb->FcbLinks );

    NwUnlock();

    return Fcb;
}

//
//  Local support routine
//

VOID
NwDeleteFcb (
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to cleanup and deallocate an Fcb.  We know there
    are no references remaining.  We cleanup any auxilary structures and
    deallocate this Fcb.

Arguments:

    Fcb - This is the Fcb to deallcoate.

Return Value:

    None

--*/

{
    PVCB Vcb = Fcb->Vcb;
    PAGED_CODE();

    //
    //  Sanity check the counts.
    //

    ASSERT( Fcb->FileHandleCount == 0 );
    ASSERT( Fcb->FileObjectCount == 0 );

    ASSERT((Fcb->Hash == NULL) || (Fcb->Hash->FSContext != Fcb));

    //
    //  Start with the common structures.
    //

    if (Fcb->FileLock) {

        FsRtlUninitializeFileLock( Fcb->FileLock );

        NwDeallocateFileLock( IrpContext, Fcb->FileLock );
    }

    FsRtlUninitializeOplock( &Fcb->Oplock );

    NwDeleteFcbNonpaged( Fcb->FcbNonpaged );

    //
    //  Now check for the two special Fcbs.
    //

    if (Fcb == Vcb->RootDirFcb) {

        Vcb->RootDirFcb = NULL;

    } else if (Fcb == Vcb->VolumeDasdFcb) {

        Vcb->VolumeDasdFcb = NULL;
    }

    //
    //  Free the Mcb
    //

    FsRtlUninitializeMcb( &Fcb->Mcb );

    //
    //  and finally free the Fcb.
    //

    NwDeallocateFcb( Fcb );

    return;
}


//
//  Local support routine
//

PFCB_NONPAGED
NwCreateFcbNonpaged (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine is called to create and initialize the non-paged portion
    of an Fcb.

Arguments:

Return Value:

    PFCB_NONPAGED - Pointer to the created nonpaged Fcb.  NULL if not created.

--*/

{
    PFCB_NONPAGED FcbNonpaged;

    PAGED_CODE();

    //
    //  Allocate the non-paged pool and initialize the various
    //  synchronization objects.
    //

    FcbNonpaged = NwAllocateFcbNonpaged( IrpContext );

    if (FcbNonpaged != NULL) {

        RtlZeroMemory( FcbNonpaged, sizeof( FCB_NONPAGED ));

        FcbNonpaged->NodeTypeCode = NWFS_NTC_FCB_NONPAGED;
        FcbNonpaged->NodeByteSize = sizeof( FCB_NONPAGED );

        ExInitializeResource( &FcbNonpaged->Resource );
    }

    return FcbNonpaged;
}


//
//  Local support routine
//

VOID
NwDeleteFcbNonpaged (
    IN PFCB_NONPAGED FcbNonpaged
    )

/*++

Routine Description:

    This routine is called to cleanup the non-paged portion of an Fcb.

Arguments:

    FcbNonpaged - Structure to clean up.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ExDeleteResource( &FcbNonpaged->Resource );

    NwDeallocateFcbNonpaged( FcbNonpaged );

    return;
}

PCCB
NwCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine is called to allocate and initialize the Ccb structure.

Arguments:

    Fcb - This is the Fcb for the file being opened.

    Flags - User flags to set in this Ccb.

Return Value:

    PCCB - Pointer to the created Ccb.

--*/

{
    PCCB NewCcb;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->Irp );

    PAGED_CODE();

    //
    //  Allocate and initialize the structure.
    //

    NewCcb = NwAllocateCcb( IrpContext );

    RtlZeroMemory( NewCcb, sizeof( CCB ));

    //
    //  Set the proper node type code and node byte size
    //

    NewCcb->NodeTypeCode = NWFS_NTC_CCB;
    NewCcb->NodeByteSize = sizeof( CCB );

    //
    //  Set the initial value for the flags and Fcb
    //

    NewCcb->Flags = Flags;
    NewCcb->Fcb = Fcb;
    NewCcb->FileObject = IrpSp->FileObject;

    //
    //  Insert this Ccb record on the Fcb list
    //

    NwLock();

    InsertTailList( &Fcb->CcbList, &NewCcb->CcbLinks );

    NwUnlock();

    //
    //  Finally, stash the creating thread.
    //

    NewCcb->OpeningThread = PsGetCurrentThread();

    return NewCcb;
}


VOID
NwDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    )
/*++

Routine Description:

    This routine is called to cleanup and deallocate a Ccb structure.

Arguments:

    Ccb - This is the Ccb to delete.

Return Value:

    None

--*/

{
    PAGED_CODE();


    //
    //  Free pool owned by this Ccb.
    //

    if (Ccb->OemSearchExpression.Buffer != NULL) {
        ExFreePool( Ccb->OemSearchExpression.Buffer );
    }

    if (Ccb->UnicodeSearchExpression.Buffer != NULL) {
        ExFreePool( Ccb->UnicodeSearchExpression.Buffer );
    }

    if (Ccb->OriginalName.Buffer != NULL) {
        ExFreePool( Ccb->OriginalName.Buffer );
    }

    NwDeallocateCcb( IrpContext, Ccb );
    return;
}


BOOLEAN
NwCreateFileLock (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb,
    IN BOOLEAN RaiseOnError
    )

/*++

Routine Description:

    This routine is called when we want to attach a file lock structure to the
    given Fcb.  It is possible the file lock is already attached.

    This routine is sometimes called from the fast path and sometimes in the
    Irp-based path.  We don't want to raise in the fast path, just return FALSE.

Arguments:

    Fcb - This is the Fcb to create the file lock for.

    RaiseOnError - If TRUE, we will raise on an allocation failure.  Otherwise we
        return FALSE on an allocation failure.

Return Value:

    BOOLEAN - TRUE if the Fcb has a filelock, FALSE otherwise.

--*/

{
    BOOLEAN Result = TRUE;
    PFILE_LOCK FileLock;

    PAGED_CODE();

    //
    //  Lock the Fcb and check if there is really any work to do.
    //

    NwAcquireFcbExclusive( IrpContext, Fcb, FALSE );

    if (Fcb->FileLock != NULL) {

        NwReleaseFcb( IrpContext, Fcb );
        return TRUE;
    }

    Fcb->FileLock = FileLock = NwAllocateFileLock( IrpContext );

    NwReleaseFcb( IrpContext, Fcb );

    //
    //  Return or raise as appropriate.
    //

    if (FileLock == NULL) {

        if (RaiseOnError) {

            ASSERT( ARGUMENT_PRESENT( IrpContext ));

            NwRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        Result = FALSE;
    }

    return Result;
}


PIRP_CONTEXT
NwCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine is called to initialize an IrpContext for the current
    NWFS request.  We allocate the structure and then initialize it from
    the given Irp.

Arguments:

    Irp - Irp for this request.

    Wait - TRUE if this request is synchronous, FALSE otherwise.

Return Value:

    PIRP_CONTEXT - Allocated IrpContext.

--*/

{
    PIRP_CONTEXT NewIrpContext = NULL;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    //  Look in our lookaside list for an IrpContext.
    //

    if (NwData.IrpContextDepth) {

        NwLock();
        NewIrpContext = (PIRP_CONTEXT) PopEntryList( &NwData.IrpContextList );
        if (NewIrpContext != NULL) {

            NwData.IrpContextDepth--;
        }

        NwUnlock();
    }

    if (NewIrpContext == NULL) {

        //
        //  We didn't get it from our private list so allocate it from pool.
        //

        NewIrpContext = FsRtlAllocatePoolWithTag( NonPagedPool, sizeof( IRP_CONTEXT ), TAG_IRP_CONTEXT );
    }

    RtlZeroMemory( NewIrpContext, sizeof( IRP_CONTEXT ));

    //
    //  Set the proper node type code and node byte size
    //

    NewIrpContext->NodeTypeCode = NWFS_NTC_IRP_CONTEXT;
    NewIrpContext->NodeByteSize = sizeof( IRP_CONTEXT );

    //
    //  Set the originating Irp field
    //

    NewIrpContext->Irp = Irp;

    //
    //  Copy RealDevice for workque algorithms.  We will update this in the Mount or
    //  Verify since they have no file objects to use here.
    //

    if (IrpSp->FileObject != NULL) {

        NewIrpContext->RealDevice = IrpSp->FileObject->DeviceObject;
    }

    //
    //  Locate the volume device object and Vcb that we are trying to access.
    //  This may be our filesystem device object.  In that case don't initialize
    //  the Vcb field.
    //

    if (IrpSp->DeviceObject->Size != sizeof(DEVICE_OBJECT)) {

        NewIrpContext->Vcb =  &((PVOLUME_DEVICE_OBJECT) IrpSp->DeviceObject)->Vcb;
    }

    //
    //  Major/Minor Function codes
    //

    NewIrpContext->MajorFunction = IrpSp->MajorFunction;
    NewIrpContext->MinorFunction = IrpSp->MinorFunction;

    //
    //  Set the wait parameter
    //

    if (Wait) {

        SetFlag( NewIrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    } else {

        SetFlag( NewIrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );
    }

    //
    //  return and tell the caller
    //

    return NewIrpContext;
}


VOID
NwCleanupIrpContext (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN Post
    )

/*++

Routine Description:

    This routine is called to cleanup and possibly deallocate the Irp Context.
    If the request is being posted or this Irp Context is possibly on the
    stack then we only cleanup any auxilary structures.

Arguments:

    Post - TRUE if we are posting this request, FALSE if we are deleting
        or retrying this in the current thread.

Return Value:

    None.

--*/

{
    PAGED_CODE();


    //
    //  If there is a NwIoContext that was allocated, free it.
    //

    if (IrpContext->NwIoContext != NULL) {

        ExFreePool( IrpContext->NwIoContext );
        IrpContext->NwIoContext = NULL;
    }

    //
    //  If we aren't doing more processing then deallocate this as appropriate.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING)) {

        //
        //  If this context is the top level NWFS context then we need to
        //  restore the top level thread context.
        //

        if (IrpContext->ThreadContext != NULL) {

            NwRestoreThreadContext( IrpContext );
        }

        //
        //  Deallocate the IrpContext if not from the stack.
        //

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ON_STACK )) {

            if (NwData.IrpContextDepth < NwData.IrpContextMaxDepth) {

                NwLock();

                PushEntryList( &NwData.IrpContextList, (PSINGLE_LIST_ENTRY) IrpContext );
                NwData.IrpContextDepth++;

                NwUnlock();

            } else {

                //
                //  We couldn't add this to our lookaside list so free it to
                //  pool.
                //

                ExFreePool( IrpContext );
            }
        }

    //
    //  Clear the appropriate flags.
    //

    } else if (Post) {

        //
        //  If this context is the top level NWFS context then we need to
        //  restore the top level thread context.
        //

        if (IrpContext->ThreadContext != NULL) {

            NwRestoreThreadContext( IrpContext );
        }

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_POST );

    } else {

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_RETRY );
    }

    return;
}


VOID
NwUnlinkFcb (
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine removes an Fcb from the any list it may be in, as well as
    clearing any pointers to it from the hash.  This is the first step in 
    deleting an Fcb.

Arguments:

    Fcb - The Fcb to uinlink.

Return Value:

    None.

--*/

{
    ASSERT_LOCKED_NW();
    ASSERT( Fcb->FileObjectCount == 0 );
    ASSERT( Fcb->FileHandleCount == 0 );

    if (Fcb->Hash && (Fcb->Hash->FSContext == Fcb)) {
        Fcb->Hash->FSContext = NULL;
    }

    if (FlagOn(Fcb->FcbState, FCB_STATE_CLOSE_DELAYED)) {
        RemoveEntryList( &Fcb->DelayedCloseLinks );
        NwData.DelayedCloseCount -= 1;
    }

    RemoveEntryList( &Fcb->FcbLinks );
    NwDecrementFcbCount( Fcb->Vcb );
    return;
}

VOID
NwDetachDevice (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
    )

/*++

Routine Description:

    This routine is the driver's entry point for FastIoDetachDevice.  It is
    used to correctly synchronize a dismount volume call.  Since Nwfsro is 
    unique in that the "real" device must vanish as well as the volume device,
    we use an attachment technique to perfectly sequence the deletion of these
    two devices.
    
    This routine will be invoked recursively underneath the IoDeleteDevice()
    call in NwDismountVolume() since we attached the Vcb device object to the
    pseudo device object.

Arguments:

    SourceDevice - This is volume device object (aka, the Vcb).
    
    TargetDevice - This is the target device object (aka, our pseudo device).

Return Value:

    None.

--*/

{
    PVCB Vcb;

    ASSERT(SourceDevice->Size != sizeof(DEVICE_OBJECT));

    Vcb = &((PVOLUME_DEVICE_OBJECT)SourceDevice)->Vcb;

    ExAcquireResourceExclusive( &NwData.DataResource, TRUE );

    NwDeleteVcb( Vcb );

    ExReleaseResource( &NwData.DataResource );

    TargetDevice->AttachedDevice = NULL;

    return;
}

VOID
NwUnload (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This is our unload routine.  It is the final code that will be called
    before our driver will be unloaded, and as such must make sure anything
    allocated by the driver is freed.

Arguments:

    DriverObject - A pointer to our NWFSRO driver object.
    
Return Value:

    None.

--*/

{
    UNICODE_STRING DosDeviceName;
    PLIST_ENTRY Entry;
    PVCB Vcb;

    //
    //  The fact that we have been called means that there are no open
    //  file by anyone (even Mm) on any of our volumes.  However our
    //  delayed close queues may have lots of Fcbs that need to be
    //  freed.
    //

    NwData.MinDelayedCloseCount = 0;
    NwFspClose( NULL );

    //
    //  Now clean up all the mounted volumes.
    //

    ExAcquireResourceExclusive( &NwData.DataResource, TRUE );

    Entry = NwData.VcbQueue.Flink;

    while (Entry != &NwData.VcbQueue) {
        Vcb = CONTAINING_RECORD(Entry, VCB, VcbLinks);
        Entry = Entry->Flink;

        Vcb->VcbCondition = VcbDismountInProgress;

        IoDeleteDevice( Vcb->TargetDeviceObject );

        NwDeleteVcb( Vcb );
    }

    ExReleaseResource( &NwData.DataResource );

    //
    //  The following three routines cause the nwfs.lib code to free all the
    //  resources it owns.  They must be called in the following order.
    //

    NWFSVolumeClose();
    RemoveDiskDevices();
    FreePartitionResources();

    //
    //  Delete the symbolic link to our driver and free our resource.
    //

    RtlInitUnicodeString( &DosDeviceName, DOS_DEVICE_NAME );

    IoDeleteSymbolicLink( &DosDeviceName );

    ASSERT(IsListEmpty( &NwData.VcbQueue ));

    ExDeleteResource( &NwData.DataResource );

    //
    //  Free our look aside list for IrpContexts.
    //

    while (NwData.IrpContextDepth--) {

        PIRP_CONTEXT IrpContext;

        IrpContext = (PIRP_CONTEXT)PopEntryList( &NwData.IrpContextList );

        ASSERT(IrpContext);

        if (IrpContext != NULL) {
            ASSERT(!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ON_STACK ));
            ExFreePool( IrpContext );
        }
    }

    //
    //  Free our stashed array of file objects, if present.
    //

    if (NwData.SavedDiskArray) {
        ULONG i;
        for (i=0; i<NwData.SavedDiskArray->PartitionCount; i++) {
            ObDereferenceObject( NwData.SavedDiskArray->Pointers[i].Partition0Pointer );
            ObDereferenceObject( NwData.SavedDiskArray->Pointers[i].PartitionPointer );
        }

        ExFreePool( NwData.SavedDiskArray );
    }

    //
    //  and finally delete our main named device object.
    //

    IoDeleteDevice( NwData.FileSystemDeviceObject );

    return;
}

ULONG
NwFsVolumeIsInvalid (
    IN ULONG VolumeNumber
    )

/*++

Routine Description:

    This routine tells the IFS that a previously mounted volume is not
    physically present anymore.  This routine is called from within the
    NWFSVolumeScan() call.  
    
Arguments:

    VolumeNumber - The slot number in the volume table that is gone.

Return Value:

    BOOLEAN - If TRUE, then we acknowledge that the volume is gone, but have
        not torn down the volume structures yet because there are still handles
        open on the volume.  If FALSE, then we have have completely torn down
        the volume, so any structures backing the volume may be freed.

--*/

{
    return TRUE;
}

VOID
NwMarkPartitionBad (
    IN PDEVICE_OBJECT BadPartition
    )

/*++

Routine Description:

    This routine marks a partition as bad in the bad partition table.

Arguments:

    BadPartition - The partition to mark bad.

Return Value:

    None

--*/

{
    ULONG i;
    KIRQL SavedIrql;

    //
    //  Now just run through the list looking to see if we are on it.  If so,
    //  then just return, otherwise add ourselves to the end of the list.
    //

    BadPartition = IoGetAttachedDevice(BadPartition);

    KeAcquireSpinLock( &NwData.BadPartitionSpinLock, &SavedIrql );

    for (i=0; i < MAX_BAD_PARTITIONS; i++) {

        if (NwData.BadPartitions[i] == BadPartition) {
            break;
        }

        if (NwData.BadPartitions[i] == NULL) {
            NwData.BadPartitions[i] = BadPartition;
            break;
        }
    }

    KeReleaseSpinLock( &NwData.BadPartitionSpinLock, SavedIrql );

    return;
}

BOOLEAN
NwIsPartitionBad (
    IN PFILE_OBJECT BadPartition
    )

/*++

Routine Description:

    This routine looks in the bad partition table to see if an I/O has
    previously failed.  Note that a file pointer to a partition is actually
    passed in.

Arguments:

    BadPartition - A open file object to a partition.

Return Value:

    None

--*/

{
    ULONG i;
    PDEVICE_OBJECT BadPartitionDevice;

    //
    //  Now just run through the list looking to see if we are on it.  Note
    //  that we don't need to synchronize with anyone adding to the list since
    //  all additions are done to the end of the list.
    //

    BadPartitionDevice = IoGetAttachedDevice(BadPartition->DeviceObject); 

    for (i=0; i < MAX_BAD_PARTITIONS; i++) {

        if (NwData.BadPartitions[i] == NULL) {
            return FALSE;
        }

        if (NwData.BadPartitions[i] == BadPartitionDevice) {
            return TRUE;
        }
    }

    return FALSE;
}


