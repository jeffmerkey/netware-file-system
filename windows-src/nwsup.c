
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

    NwSup.c

Abstract:

    This module implements routines that are specific to either netware
    on disk structures, or in memory structures.

  Authors:

    Jeff Merkey (jeffmerkey@gmail.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

****************************************************************************/

#include "NwProcs.h"
#include "Fenris.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_NWSUP)


BOOLEAN NwExtraReference = FALSE;

//
//  Local support routines
//

NTSTATUS
NwReadSectors (
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer,
    IN PDEVICE_OBJECT TargetDeviceObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwReadSectors)
#endif

//
//  This is the routine that receives the IOCTL
//

NTSTATUS
NwProcessVolumeLayout (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDISK_ARRAY DiskArray;
    ULONG DiskArrayLength;
    ULONG OutputBufferLength;
    ULONG i;
    ULONG MountedBefore;
    ULONG MountedAfter;
    ULONG NewVolumesMounted;

    UNICODE_STRING NwDeviceName;

    WCHAR NwDeviceNameBuffer[] = L"\\Device\\NetwareDisk00";

    PDEVICE_OBJECT Device;

    //
    //  First let's sanity check the structure.
    //

    DiskArrayLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    DiskArray = (PDISK_ARRAY)Irp->AssociatedIrp.SystemBuffer;

    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    if ((DiskArrayLength < sizeof(DISK_ARRAY)) ||
        (DiskArray->StructureSize != DiskArrayLength) ||
        (DiskArray->PartitionCount > 32) ||
        (FIELD_OFFSET(DISK_ARRAY, Pointers[DiskArray->PartitionCount - 1].PartitionPointer) + sizeof(PVOID) > DiskArrayLength) ||
        (FIELD_OFFSET(DISK_ARRAY, Handles[DiskArray->PartitionCount - 1].PartitionHandle) + sizeof(HANDLE) > DiskArrayLength) ||
        (OutputBufferLength < sizeof(ULONG))) {

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Do some version checking here.  ***MAKE BETTER***
    //

    if ((DiskArray->MajorVersion != NWFSRO_MAJOR_VERSION) ||
        (DiskArray->MinorVersion != NWFSRO_MINOR_VERSION)) {

        NwCompleteRequest( IrpContext, Irp, STATUS_REVISION_MISMATCH );
        return STATUS_REVISION_MISMATCH;
    }

    //
    //  Acquire the global resource to do mount operations.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    NwAcquireNwData( IrpContext );

    if (NwData.SavedDiskArray == NULL) {

        //
        //  Allocate the array to hold the pointers.  This will raise if we
        //  receive an allocation failure.
        //

        NwData.SavedDiskArray =
            FsRtlAllocatePoolWithTag( NwPagedPool,
                                      DiskArrayLength,
                                      TAG_DISK_ARRAY );

        //
        //  Now run through table and turn the handles into pointers.
        //

        for (i=0; i<DiskArray->PartitionCount; i++) {

            //
            //  Try to get a pointer to the file object from the handle passed in.
            //

            Status = ObReferenceObjectByHandle( DiskArray->Handles[i].PartitionHandle,
                                                0,
                                                *IoFileObjectType,
                                                Irp->RequestorMode,
                                                &DiskArray->Pointers[i].PartitionPointer,
                                                NULL );

            if (!NT_SUCCESS(Status)) {

                //
                //  Yikes we need to dereference all the file object that came before.
                //

                while (i) {
                    ObDereferenceObject( DiskArray->Pointers[i].PartitionPointer );
                    i -= 1;
                }

                NwReleaseNwData(IrpContext);

                NwCompleteRequest( IrpContext, Irp, Status );
                return Status;
            }

            //
            //  We need to find the device with the largest number of stack
            //  locations and then use that for all our virtual volumes since
            //  segments can span multiple physical disks.
            //

            Device = IoGetAttachedDevice(((PFILE_OBJECT)DiskArray->Pointers[i].PartitionPointer)->DeviceObject);

            if (NwData.MaxDeviceStackSize < Device->StackSize) {
                NwData.MaxDeviceStackSize = Device->StackSize;
            }
        }

        for (i=0; i<DiskArray->PartitionCount; i++) {

            //
            //  Try to get a pointer to the file object from the handle passed in.
            //

            Status = ObReferenceObjectByHandle( DiskArray->Handles[i].Partition0Handle,
                                                0,
                                                *IoFileObjectType,
                                                Irp->RequestorMode,
                                                &DiskArray->Pointers[i].Partition0Pointer,
                                                NULL );

            if (!NT_SUCCESS(Status)) {

                //
                //  Yikes we need to dereference all the file object that came before.
                //

                while (i) {
                    ObDereferenceObject( DiskArray->Pointers[i].Partition0Pointer );
                    i -= 1;
                }

                i = DiskArray->PartitionCount;

                while (i) {
                    ObDereferenceObject( DiskArray->Pointers[i].PartitionPointer );
                    i -= 1;
                }

                NwReleaseNwData(IrpContext);

                NwCompleteRequest( IrpContext, Irp, Status );
                return Status;
            }

            //
            //  We need to find the device with the largest number of stack
            //  locations and then use that for all our virtual volumes since
            //  segments can span multiple physical disks.
            //

            Device = IoGetAttachedDevice(((PFILE_OBJECT)DiskArray->Pointers[i].Partition0Pointer)->DeviceObject);

            if (NwData.MaxDeviceStackSize < Device->StackSize) {
                NwData.MaxDeviceStackSize = Device->StackSize;
            }
        }

        RtlCopyMemory( NwData.SavedDiskArray, DiskArray, DiskArrayLength );

        PartitionArray = NwData.SavedDiskArray;
        NWFSVolumeScan();
    }

    //
    //  At this point the volume scan is complete, so we will not back it out
    //  if we receive an error trying to mount volumes.
    //

    MountedBefore = 0;
    for (i=0; i<32; i++) {
        if (VolumeMountedTable[i]) {
            MountedBefore |= (1<<i);
        }
    }

    if (DiskArray->FunctionCode == NWFS_MOUNT_ALL_VOLUMES) {

        if (MountAllVolumes()) {
            Status = STATUS_UNRECOGNIZED_VOLUME;
        }

    } else if (DiskArray->FunctionCode == NWFS_MOUNT_VOLUME) {

        if (MountVolume( DiskArray->VolumeLabel )) {
            Status = STATUS_UNRECOGNIZED_VOLUME;
        }

    } else {

        Status = STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(Status)) {

        NwReleaseNwData(IrpContext);

        NwCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    MountedAfter = 0;
    for (i=0; i<32; i++) {
        if (VolumeMountedTable[i]) {
            MountedAfter |= (1<<i);
        }
    }

    NewVolumesMounted = MountedBefore ^ MountedAfter;

    ASSERT( (NewVolumesMounted & MountedAfter) == NewVolumesMounted );

    if (NT_SUCCESS(Status)) {

        PVOLUME_DEVICE_OBJECT VolDo;
        UNICODE_STRING VolumeLabel;
        OEM_STRING OemVolumeLabel;
        PVCB Vcb;
        PVPB Vpb;                  
        PFILE_OBJECT FileObject;
        PDEVICE_OBJECT ProtoDevice;
        PDEVICE_OBJECT PseudoDevice;
        PDEVICE_OBJECT VolumeDevice;
        ULONG DigitOffset;
        ULONG DontCare;
        PVCB NewVcbs[32] = {NULL};

        //
        //  The mount worked.  Now we have to create pseudo device objects for
        //  all the volumes and mount them in NT land.
        //

        NwDeviceName.MaximumLength = sizeof(NwDeviceNameBuffer);
        NwDeviceName.Length = NwDeviceName.MaximumLength - sizeof(WCHAR);
        NwDeviceName.Buffer =  NwDeviceNameBuffer;

        DigitOffset = NwDeviceName.Length / sizeof(WCHAR) - 2;

        try {

            for (i=0; i < 32; i++) {

                if ((NewVolumesMounted & (1<<i)) == 0) {
                    continue;
                }

                //
                //  Show that we initialized the Vcb and can cleanup with the Vcb.
                //

                VolDo = NULL;
                Vcb = NULL;
                Vpb = NULL;
                PseudoDevice = NULL;
                VolumeDevice = NULL;

                //
                //  For our proto device, try to use a partition that will actually
                //  hold this volume.
                //
                
                if (TrgGetPartitionInfoFromVolumeInfo( i,
                                                       0,
                                                       1,
                                                       &FileObject,
                                                       &DontCare,
                                                       &DontCare ) != NwSuccess) {
        
                    try_return( Status = STATUS_DISK_CORRUPT_ERROR );
                }
        
                ProtoDevice = FileObject->DeviceObject;
        
                //
                //  First we are going to try and create a pseudo device.
                //

                NwDeviceNameBuffer[DigitOffset] = (WCHAR)(((i/16) < 10) ?
                                                          ((i/16) + '0') :
                                                          ((i/16) - 10 + 'A'));

                NwDeviceNameBuffer[DigitOffset+1] = (WCHAR)(((i%16) < 10) ?
                                                            ((i%16) + '0') :
                                                            ((i%16) - 10 + 'A'));

                Status = IoCreateDevice( NwData.DriverObject,
                                         0,
                                         &NwDeviceName,
                                         FILE_DEVICE_DISK,
                                         0,
                                         FALSE,
                                         &PseudoDevice );
                if (!NT_SUCCESS(Status)) {
                    try_return(Status);
                }

                //
                //  Now fill in the required fields in the pseudo device object.
                //

                PseudoDevice->AlignmentRequirement = ProtoDevice->AlignmentRequirement;
                PseudoDevice->Characteristics = ProtoDevice->Characteristics;
                PseudoDevice->StackSize = NwData.MaxDeviceStackSize + 1;
                PseudoDevice->SectorSize = ProtoDevice->SectorSize;
                PseudoDevice->Flags = ProtoDevice->Flags | DO_DEVICE_INITIALIZING;

                Vpb = PseudoDevice->Vpb;

                //
                //  Initialize the Vpb on the pseudo device, starting with
                //  the volume label.
                //

                VolumeLabel.Length = 0;
                VolumeLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
                VolumeLabel.Buffer = &Vpb->VolumeLabel[0];

                OemVolumeLabel.Length = 0;
                OemVolumeLabel.MaximumLength = 15;
                OemVolumeLabel.Buffer = VolumeTable[i]->VolumeName;

                while (*OemVolumeLabel.Buffer++) {
                    OemVolumeLabel.Length++;
                }
                OemVolumeLabel.Buffer = VolumeTable[i]->VolumeName;

                Status = RtlOemStringToCountedUnicodeString( &VolumeLabel,
                                                             &OemVolumeLabel,
                                                             FALSE );

                if (!NT_SUCCESS(Status)) {
                    try_return(Status);
                }
                VolumeLabel.Buffer[OemVolumeLabel.Length] = UNICODE_NULL;

                Vpb->VolumeLabelLength = VolumeLabel.Length;
                Vpb->Flags |= VPB_MOUNTED;
                Vpb->SerialNumber = VolumeTable[i]->VolumeSerialNumber;

                //
                //  Now we have to create our volume device object.
                //

                Status = IoCreateDevice( NwData.DriverObject,
                                         sizeof( VOLUME_DEVICE_OBJECT ) - sizeof( DEVICE_OBJECT ),
                                         NULL,
                                         FILE_DEVICE_DISK_FILE_SYSTEM,
                                         0,
                                         FALSE,
                                         &VolumeDevice );

                if (!NT_SUCCESS( Status )) { try_return( Status ); }

                VolDo = (PVOLUME_DEVICE_OBJECT)VolumeDevice;

                //
                //  Our alignment requirement is the larger of the processor
                //  alignment requirement already in the volume device object
                //  and that in the DeviceObjectWeTalkTo
                //

                if (PseudoDevice->AlignmentRequirement > VolumeDevice->AlignmentRequirement) {

                    VolumeDevice->AlignmentRequirement = PseudoDevice->AlignmentRequirement;
                }

                //
                //  Initialize the overflow queue for the volume
                //

                VolDo->OverflowQueueCount = 0;
                VolDo->PostedRequestCount = 0;
                InitializeListHead( &VolDo->OverflowQueue );

                //
                //  Now before we can initialize the Vcb we need to set up the
                //  device object field in the VPB to point to our new volume
                //  device object.
                //

                Vpb->DeviceObject = VolumeDevice;

                //
                //  Initialize the Vcb.  This routine will raise on an allocation
                //  failure.
                //

                NwInitializeVcb( IrpContext,
                                 &VolDo->Vcb,
                                 i,
                                 PseudoDevice,
                                 ProtoDevice,
                                 Vpb,
                                 VolumeTable[i]->VolumeClusters,
                                 VolumeTable[i]->VolumeFreeClusters,
                                 VolumeTable[i]->SectorsPerCluster );

                //
                //  Initialize Vcb now so that we know to free it in an error
                //  path.
                //

                Vcb = &VolDo->Vcb;

                //
                //  We must initialize the stack size in our device object before
                //  the following reads, because the I/O system has not done it yet.
                //

                VolumeDevice->StackSize = NwData.MaxDeviceStackSize + 1;

                //
                //  Clear the verify bit for the start of mount.
                //

                ClearFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

                //
                //  We need to create the two special FCBs
                //

                IrpContext->Vcb = Vcb;

                Vcb->VolumeDasdFcb = NwCreateFcb( IrpContext, FileIdZero );

                Vcb->VolumeDasdFcb->FileSize.QuadPart =
                Vcb->VolumeDasdFcb->ValidDataLength.QuadPart =
                Vcb->VolumeDasdFcb->AllocationSize.QuadPart =
                    VolumeTable[i]->VolumeClusters *
                    VolumeTable[i]->SectorsPerCluster *
                    SECTOR_SIZE;
                Vcb->VolumeDasdFcb->FileObjectCount = 1;

                FsRtlAddMcbEntry( &Vcb->VolumeDasdFcb->Mcb,
                                  0,
                                  0,
                                  VolumeTable[i]->VolumeClusters *
                                  VolumeTable[i]->SectorsPerCluster );

                Vcb->RootDirFcb = NwCreateFcb( IrpContext, FileIdZero );
                Vcb->RootDirFcb->FileAttributes |= NETWARE_ATTR_DIRECTORY;
                Vcb->RootDirFcb->FileSize.QuadPart =
                Vcb->RootDirFcb->ValidDataLength.QuadPart =
                Vcb->RootDirFcb->AllocationSize.QuadPart = 0;
                Vcb->RootDirFcb->FileObjectCount = 1;

                Vcb->FcbCount =
                Vcb->FileObjectCount = 2;

                if (NwExtraReference) {
                    ObReferenceObject( PseudoDevice );
                }

                //
                //  OK, we are now ready to open this up for mounting.
                //

                DbgPrint("NWFSRO: Mounted Volume: '%S'\n", &Vcb->Vpb->VolumeLabel[0]);

                Vcb->VcbCondition = VcbMounted;

                VolumeDevice->Flags &= ~DO_DEVICE_INITIALIZING;
                PseudoDevice->Flags &= ~DO_DEVICE_INITIALIZING;

                NewVcbs[i] = Vcb;
            }

        try_exit:  NOTHING;
        } finally {

            //
            //  Status was SUCCESS on entry to the loop, so if it is
            //  not now, we have to undo everything.
            //

            if (!NT_SUCCESS(Status) || AbnormalTermination()) {

                //
                //  First loop on all the fully created Vcbs, freeing them.
                //

                for (i=0; i < 32; i++) {
                    if (NewVcbs[i]) {
                        IoDeleteDevice( NewVcbs[i]->TargetDeviceObject );
                        NewVcbs[i]->VcbCondition = VcbDismountInProgress;
                        NwDeleteVcb( NewVcbs[i] );
                    }
                }

                //
                //  Now see if there is any partially created junk from
                //  above to clean up.  The below code is based on the
                //  order that strcutures are initialized above.
                //

                if (Vcb) {

                    ClearFlag( Vpb->Flags, VPB_MOUNTED );
                    IoDeleteDevice( PseudoDevice );
                    Vcb->VcbCondition = VcbDismountInProgress;
                    NwDeleteVcb( Vcb );

                } else if (VolumeDevice) {

                    ClearFlag( Vpb->Flags, VPB_MOUNTED );
                    IoDeleteDevice( PseudoDevice );
                    IoDeleteDevice( VolumeDevice );
                    (VOID)DismountVolume(VolumeTable[i]->VolumeName);

                } else if (PseudoDevice) {

                    ClearFlag( Vpb->Flags, VPB_MOUNTED );
                    IoDeleteDevice( PseudoDevice );
                    (VOID)DismountVolume(VolumeTable[i]->VolumeName);

                } else {

                    (VOID)DismountVolume(VolumeTable[i]->VolumeName);
                }

                //
                //  Finally, if an exception was raised, we need to release
                //  the NwData resource now.
                //

                if (AbnormalTermination()) {
                    NwReleaseNwData( IrpContext );
                }
            }
        }

        NwReleaseNwData( IrpContext );
    }

    //
    //  No data is returned on FAILURE
    //

    if (NT_SUCCESS(Status)) {
        *((PULONG)Irp->AssociatedIrp.SystemBuffer) = NewVolumesMounted;
        Irp->IoStatus.Information = sizeof(ULONG);

    } else {

        Irp->IoStatus.Information = 0;
    }

    NwCompleteRequest( IrpContext, Irp, Status );

    return Status;
}

NTSTATUS
ReadDisk (
    PVOID DiskPointer,
    ULONG SectorOffset,
    PUCHAR Buffer,
    ULONG SectorCount
    )

/*++

Routine Description:

    This routine is called to transfer sectors from the disk to a
    specified buffer.  It is used for mount operations.

    This routine is synchronous, it will not return until the operation
    is complete or until the operation fails.

    The routine allocates an IRP and then passes this IRP to a lower
    level driver.  Errors may occur in the allocation of this IRP or
    in the operation of the lower driver.

Arguments:

    DiskPointer - The device object for the volume to be read.
    
    SectorOffset - Logical offset on the disk to start the read in sectors.

    Buffer - Buffer to transfer the disk data into.

    SectorCount - Number of sectors to read.

Return Value:

    NTSTATUS - The result of the operation.

--*/

{
    PDEVICE_OBJECT Device;
    LONGLONG StartingOffset;

    Device = IoGetAttachedDevice(((PFILE_OBJECT)DiskPointer)->DeviceObject);

    StartingOffset = LlBytesFromSectors( SectorOffset );

    return NwReadSectors( StartingOffset,
                          BytesFromSectors( SectorCount ),
                          Buffer,
                          Device );
}

NTSTATUS
NwReadSectors (
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer,
    IN PDEVICE_OBJECT TargetDeviceObject
    )

/*++

Routine Description:

    This routine is called to transfer sectors from the disk to a
    specified buffer.  It is used for mount operations.

    This routine is synchronous, it will not return until the operation
    is complete or until the operation fails.

    The routine allocates an IRP and then passes this IRP to a lower
    level driver.  Errors may occur in the allocation of this IRP or
    in the operation of the lower driver.

Arguments:

    StartingOffset - Logical offset on the disk to start the read.  This
        must be on a sector boundary, no check is made here.

    ByteCount - Number of bytes to read.  This is an integral number of
        512 byte sectors, no check is made here to confirm this.

    Buffer - Buffer to transfer the disk data into.

    TargetDeviceObject - The device object for the volume to be read.

Return Value:

    NTSTATUS - The result of the operation.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    KEVENT  Event;
    PIRP Irp = NULL;

    PAGED_CODE();

    //
    //  Initialize the event.
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Attempt to allocate the IRP.
    //

    Irp = IoBuildAsynchronousFsdRequest( IRP_MJ_READ,
                                         TargetDeviceObject,
                                         Buffer,
                                         ByteCount,
                                         (PLARGE_INTEGER) &StartingOffset,
                                         &IoStatus );

    if (Irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(Irp->MdlAddress);

    //
    //  Set up the completion routine
    //

    IoSetCompletionRoutine( Irp,
                            NwResyncCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Ignore the change line (verify) for mount requests
    //

    SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    //
    //  Send the request down to the driver.  If an error occurs return
    //  it to the caller.
    //

    (VOID)IoCallDriver( TargetDeviceObject, Irp );

    //
    //  If the status was STATUS_PENDING then wait on the event.
    //

    (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );

    //
    //  Grab the Status.
    //

    Status = Irp->IoStatus.Status;

    //
    //  Clean up the Irp and Mdl
    //

    if (Irp) {

        //
        //  If there is an MDL (or MDLs) associated with this I/O
        //  request, Free it (them) here.  This is accomplished by
        //  walking the MDL list hanging off of the IRP and deallocating
        //  each MDL encountered.
        //

        while (Irp->MdlAddress != NULL) {

            PMDL NextMdl;

            NextMdl = Irp->MdlAddress->Next;

            MmUnlockPages( Irp->MdlAddress );

            IoFreeMdl( Irp->MdlAddress );

            Irp->MdlAddress = NextMdl;
        }

        IoFreeIrp( Irp );
    }

    return Status;
}

