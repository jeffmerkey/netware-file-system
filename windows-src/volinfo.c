
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

    VolInfo.c

Abstract:

    This module implements the volume information routines for Nwfs called by
    the dispatch driver.

  Authors:

    Jeff Merkey (jeffmerkey@gmail.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

****************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_VOLINFO)

//
//  Local support routines
//

NTSTATUS
NwQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NwQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NwQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NwQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCommonQueryVolInfo)
#pragma alloc_text(PAGE, NwQueryFsAttributeInfo)
#pragma alloc_text(PAGE, NwQueryFsDeviceInfo)
#pragma alloc_text(PAGE, NwQueryFsSizeInfo)
#pragma alloc_text(PAGE, NwQueryFsVolumeInfo)
#endif


NTSTATUS
NwCommonQueryVolInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying volume information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    ULONG Length;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryVolume.Length;

    //
    //  Decode the file object and fail if this an unopened file object.
    //

    TypeOfOpen = NwDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb );

    if (TypeOfOpen == UnopenedFileObject) {

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire the Vcb for this volume.
    //

    NwAcquireVcbShared( IrpContext, Fcb->Vcb, FALSE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify that the Vcb is a usable condition.
        //

        if (Fcb->Vcb->VcbCondition != VcbMounted) {
            try_return( Status = STATUS_FILE_INVALID );
        }

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedures that we're calling fills up the output buffer
        //  if possible and returns true if it successfully filled the buffer
        //  and false if it couldn't wait for any I/O to complete.
        //

        switch (IrpSp->Parameters.QueryVolume.FsInformationClass) {

        case FileFsSizeInformation:

            Status = NwQueryFsSizeInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;

        case FileFsVolumeInformation:

            Status = NwQueryFsVolumeInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;

        case FileFsDeviceInformation:

            Status = NwQueryFsDeviceInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;

        case FileFsAttributeInformation:

            Status = NwQueryFsAttributeInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

    try_exit: NOTHING;
    } finally {

        //
        //  Release the Vcb.
        //

        NwReleaseVcb( IrpContext, Fcb->Vcb );
    }

    //
    //  Complete the request if we didn't raise.
    //

    NwCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NwQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume info call

Arguments:

    Vcb - Vcb for this volume.

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    ULONG BytesToCopy;

    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    //  Fill in the data from the Vcb.
    //

    Buffer->VolumeCreationTime = *((PLARGE_INTEGER) &Vcb->VolumeDasdFcb->CreationTime);
    Buffer->VolumeSerialNumber = Vcb->Vpb->SerialNumber;

    Buffer->SupportsObjects = FALSE;

    *Length -= FIELD_OFFSET( FILE_FS_VOLUME_INFORMATION, VolumeLabel[0] );

    //
    //  Check if the buffer we're given is long enough
    //

    if (*Length >= (ULONG) Vcb->Vpb->VolumeLabelLength) {

        BytesToCopy = Vcb->Vpb->VolumeLabelLength;

    } else {

        BytesToCopy = *Length;

        Status = STATUS_BUFFER_OVERFLOW;
    }

    //
    //  Copy over what we can of the volume label, and adjust *Length
    //

    Buffer->VolumeLabelLength = Vcb->Vpb->VolumeLabelLength;

    if (BytesToCopy) {

        RtlCopyMemory( &Buffer->VolumeLabel[0],
                       &Vcb->Vpb->VolumeLabel[0],
                       BytesToCopy );
    }

    *Length -= BytesToCopy;

    //
    //  Set our status and return to our caller
    //

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NwQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume size call.

Arguments:

    Vcb - Vcb for this volume.

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    PAGED_CODE();

    //
    //  Fill in the output buffer.
    //

    Buffer->TotalAllocationUnits.QuadPart = Vcb->TotalClusters;

    Buffer->AvailableAllocationUnits.QuadPart = Vcb->FreeClusters;
    Buffer->SectorsPerAllocationUnit = Vcb->SectorsPerCluster;
    Buffer->BytesPerSector = SECTOR_SIZE;

    //
    //  Adjust the length variable
    //

    *Length -= sizeof( FILE_FS_SIZE_INFORMATION );

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
NwQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume device call.

Arguments:

    Vcb - Vcb for this volume.

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    PAGED_CODE();

    //
    //  Update the output buffer.
    //

    Buffer->Characteristics = Vcb->TargetDeviceObject->Characteristics;
    Buffer->DeviceType = FILE_DEVICE_DISK;

    //
    //  Adjust the length variable
    //

    *Length -= sizeof( FILE_FS_DEVICE_INFORMATION );

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
NwQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume attribute call.

Arguments:

    Vcb - Vcb for this volume.

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    ULONG BytesToCopy;

    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    //  Fill out the fixed portion of the buffer.
    //

    Buffer->FileSystemAttributes = FILE_CASE_SENSITIVE_SEARCH |
                                   FILE_CASE_PRESERVED_NAMES;

    Buffer->MaximumComponentNameLength = 80;

    *Length -= FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName );

    //
    //  Determine how much of the file system name will fit.
    //

    Buffer->FileSystemNameLength = 6 * sizeof(WCHAR);

    if (*Length >= 6 * sizeof(WCHAR)) {

        BytesToCopy = 6 * sizeof(WCHAR);

    } else {

        BytesToCopy = *Length;
        Status = STATUS_BUFFER_OVERFLOW;
    }

    *Length -= BytesToCopy;

    //
    //  Do the file system name.
    //

    RtlCopyMemory( &Buffer->FileSystemName[0], L"NWFSRO", BytesToCopy );

    //
    //  And return to our caller
    //

    return Status;
}
