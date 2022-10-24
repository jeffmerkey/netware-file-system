
/***************************************************************************
*
*   Copyright (c) 1999-2000 Timpanogas Research Group, Inc.
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jmerkey@timpanogas.com
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the TRG Windows NT/2000(tm) Public License (WPL) as
*   published by the Timpanogas Research Group, Inc.  (Windows NT/2000
*   is a registered trademark of Microsoft Corporation).
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
*   You should have received a copy of the WPL Public License along
*   with this program; if not, a copy can be obtained from
*   www.timpanogas.com.
*
*   You are free to modify and re-distribute this program in accordance
*   with the terms specified in the TRG Windows NT/2000 Public License (WPL).
*   The copyright contained in this code is required to be present in
*   any derivative works and you are required to provide the source code
*   for this program as part of any commercial or non-commercial
*   distribution. You are required to respect the rights of the
*   Copyright holders named within this code.
*
*   jmerkey@timpanogas.com and TRG, Inc. are the official maintainers of
*   this code.  You are encouraged to report any bugs, problems, fixes,
*   suggestions, and comments about this software to jmerkey@timpanogas.com
*   or linux-kernel@vger.rutgers.edu.  New releases, patches, bug fixes, and
*   technical documentation can be found at www.timpanogas.com.  TRG will
*   periodically post new releases of this software to www.timpanogas.com
*   that contain bug fixes and enhanced capabilities.
*
****************************************************************************

Module Name:

    DevCtrl.c

Abstract:

    This module implements the File System Device Control routines for Nwfs
    called by the dispatch driver.

  Authors:

    Jeff Merkey (jmerkey@timpanogas.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

***************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_DEVCTRL)

//
//  Local support routines
//

NTSTATUS
NwDevCtrlCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
NwProcessDevCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCommonDevControl)
#pragma alloc_text(PAGE, NwProcessDevCtrl)
#endif


NTSTATUS
NwCommonDevControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine services IRP_MJ_DEVICE_CONTROL requests.  Note that in NT4,
    we may receive FSCTLs in this path instead of fsctrl.c (due to a change
    in the Win32 API DeviceIoControl), so we must handle them as well here.
    
    If this is a real IOCTL, we must actually perform the operations here
    that would normally be done by the "real" device since there isn't a 
    real physical device for Netware volumes. 

Arguments:

    PIRP - The request.
    
Return Value:

    NTSTATUS - The result of the operation.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PVCB Vcb;
    LONGLONG VolumeLength;
    NTSTATUS Status;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  First check for our file system device object.
    //

    if (IrpSp->DeviceObject == NwData.FileSystemDeviceObject) {

        if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_QUERY_ALL_DISKS) {

            return NwProcessVolumeLayout( IrpContext,
                                          Irp,
                                          IrpSp );
        } else {

            NwCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    //
    //  If we were called with either our volume device object or a pseudo
    //  device object, we need to perform the DevCtrls by hand as there is
    //  no really "real" device to send them to.
    //

    if (IrpContext->Vcb == NULL) {

        //
        //  First deal with the case of a direct device open.
        //  Get the Vcb associated with this Pseudo device.
        //

        ASSERT( IrpSp->DeviceObject->Vpb && IrpSp->DeviceObject->Vpb->DeviceObject );

        if (!IrpSp->DeviceObject->Vpb ||
            !IrpSp->DeviceObject->Vpb->DeviceObject) {

            NwCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        Vcb = &((PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject->Vpb->DeviceObject)->Vcb;

        ASSERT(Vcb->TargetDeviceObject == IrpSp->DeviceObject);

    } else {

        TYPE_OF_OPEN TypeOfOpen;
        PFCB Fcb;
        PCCB Ccb;

        //
        //  Extract and decode the file object.
        //

        TypeOfOpen = NwDecodeFileObject( IrpContext,
                                         IrpSp->FileObject,
                                         &Fcb,
                                         &Ccb );

        //
        //  The only type of opens we accept are user volume opens.
        //

        if (TypeOfOpen != UserVolumeOpen) {

            NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
            return STATUS_INVALID_PARAMETER;
        }

        Vcb = Fcb->Vcb;
    }

    //
    //
    //  Assume failure.
    //

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Information = 0;

    //
    // Determine which I/O control code was specified.
    //

    VolumeLength = Vcb->TotalClusters * Vcb->SectorsPerCluster * 0x200;

    switch ( IrpSp->Parameters.DeviceIoControl.IoControlCode ) {

    case IOCTL_DISK_GET_MEDIA_TYPES:
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:

        //
        //  Return the drive geometry for the pseudo disk.  Note that
        //  we return values which were made up to suit the volume size.
        //

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(DISK_GEOMETRY)) {

            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

        } else {

            PDISK_GEOMETRY OutputBuffer;

            OutputBuffer = (PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer;

            OutputBuffer->MediaType = FixedMedia;

            OutputBuffer->BytesPerSector = 0x200;
            OutputBuffer->SectorsPerTrack = 0x20;
            OutputBuffer->TracksPerCylinder = 0x2;
            OutputBuffer->Cylinders.QuadPart = VolumeLength / (0x200 * 0x20 * 0x2);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
        }
        break;

    case IOCTL_DISK_GET_PARTITION_INFO:

        //
        // Return the information about the 'partition'.
        //

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARTITION_INFORMATION)) {

            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

        } else {

            PPARTITION_INFORMATION OutputBuffer;

            OutputBuffer = (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

            OutputBuffer->PartitionType = NETWARE_PARTITION_TYPE;
            OutputBuffer->BootIndicator = FALSE;
            OutputBuffer->RecognizedPartition = TRUE;
            OutputBuffer->RewritePartition = FALSE;
            OutputBuffer->StartingOffset.QuadPart = 0;
            OutputBuffer->PartitionLength.QuadPart = VolumeLength;
            OutputBuffer->HiddenSectors =  1L;

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof( PARTITION_INFORMATION );
        }
        break;

    case IOCTL_DISK_VERIFY:

        //
        //  This is basically a no-op except that we sanity check the input
        //  buffer.
        //

        {
            PVERIFY_INFORMATION VerifyInformation;

            VerifyInformation = Irp->AssociatedIrp.SystemBuffer;

            if ((VerifyInformation->StartingOffset.QuadPart < 0) ||
                (VerifyInformation->StartingOffset.QuadPart +
                 VerifyInformation->Length < 0)) {

                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

            } else {

                Irp->IoStatus.Status = STATUS_SUCCESS;
            }
        }
        break;

    default:
        //
        //  The specified I/O control code is unrecognized by this driver.
        //  The I/O status field in the IRP has already been set so just
        //  terminate the switch.
        //

        break;
    }

    Status = Irp->IoStatus.Status;

    NwCompleteRequest(IrpContext, Irp, Irp->IoStatus.Status);

    return Status;
}
