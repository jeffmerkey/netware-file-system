
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

    NwInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for Nwfs

  Authors:

    Jeff Merkey (jeffmerkey@gmail.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

***************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_NWINIT)

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
NwInitializeGlobalData (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FileSystemDeviceObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, NwInitializeGlobalData)
#endif


//
//  Local support routine
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Netware file system
    device driver.  This routine creates the device object for the FileSystem
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING NtDeviceName;
    UNICODE_STRING Win32DeviceName;
    PDEVICE_OBJECT NwfsFileSystemDeviceObject;

    //
    //  First check our version.
    //

    RtlZeroMemory( &NwData, sizeof( NW_DATA ));

    NwData.Checked = PsGetVersion( &NwData.MajorVersion,
                                   &NwData.MinorVersion,
                                   &NwData.BuildNumber,
                                   &NwData.CSDVersion);

    DbgPrint("Nwfsro: NT Version (%s) - Major=0x%x, Minor=0x%x, Build=0x%x, CSD=%S.\n",
             NwData.Checked ? "checked" : "free",
             NwData.MajorVersion,
             NwData.MinorVersion,
             NwData.BuildNumber,
             NwData.CSDVersion.Buffer);

    //
    // Create the device object.
    //

    RtlInitUnicodeString( &NtDeviceName, NT_DEVICE_NAME );

    Status = IoCreateDevice( DriverObject,
                             0,
                             &NtDeviceName,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             &NwfsFileSystemDeviceObject );

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    RtlInitUnicodeString( &Win32DeviceName, DOS_DEVICE_NAME );

    Status = IoCreateSymbolicLink( &Win32DeviceName, &NtDeviceName );

    if (!NT_SUCCESS(Status)) {
        IoDeleteDevice( NwfsFileSystemDeviceObject );
        return Status;
    }

    //
    //  Note that because of the way data caching is done, we set neither
    //  the Direct I/O nor Buffered I/O bit in DeviceObject->Flags.  If
    //  data is not in the cache, or the request is not buffered, we may,
    //  set up for Direct I/O by hand.
    //
    //  Initialize the driver object with this driver's entry points.
    //
    //  NOTE - Each entry in the dispatch table must have an entry in
    //  the Fsp/Fsd dispatch switch statements.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                  =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   =
    DriverObject->MajorFunction[IRP_MJ_READ]                    =
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]       =
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]         =
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION]=
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]       =
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]     =
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          =
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]            =
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                 = (PDRIVER_DISPATCH) NwFsdDispatch;

    DriverObject->FastIoDispatch = &NwFastIoDispatch;

    DriverObject->DriverUnload = NwUnload;

    //
    //  Initialize the global data structures
    //

    NwInitializeGlobalData( DriverObject, NwfsFileSystemDeviceObject );

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}


//
//  Local support routine
//

VOID
NwInitializeGlobalData (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FileSystemDeviceObject
    )

/*++

Routine Description:

    This routine initializes the global Nwfs data structures.

Arguments:

    DriverObject - Supplies the driver object for NWFS.

    FileSystemDeviceObject - Supplies the device object for NWFS.

Return Value:

    None.

--*/

{
    ULONG i;
    WCHAR TempWcharArray[256] = {0};

    //
    //  Start by initializing the FastIoDispatch Table.
    //

    RtlZeroMemory( &NwFastIoDispatch, sizeof( FAST_IO_DISPATCH ));

    NwFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);
    NwFastIoDispatch.FastIoCheckIfPossible =   NwFastIoCheckIfPossible;  //  CheckForFastIo
    NwFastIoDispatch.FastIoRead =              FsRtlCopyRead;            //  Read
    NwFastIoDispatch.FastIoQueryBasicInfo =    NwFastQueryBasicInfo;     //  QueryBasicInfo
    NwFastIoDispatch.FastIoQueryStandardInfo = NwFastQueryStdInfo;       //  QueryStandardInfo
    NwFastIoDispatch.FastIoLock =              NwFastLock;               //  Lock
    NwFastIoDispatch.FastIoUnlockSingle =      NwFastUnlockSingle;       //  UnlockSingle
    NwFastIoDispatch.FastIoUnlockAll =         NwFastUnlockAll;          //  UnlockAll
    NwFastIoDispatch.FastIoUnlockAllByKey =    NwFastUnlockAllByKey;     //  UnlockAllByKey
    NwFastIoDispatch.AcquireFileForNtCreateSection =  NwAcquireForCreateSection;
    NwFastIoDispatch.ReleaseFileForNtCreateSection =  NwReleaseForCreateSection;
    NwFastIoDispatch.FastIoDetachDevice =             NwDetachDevice;
    NwFastIoDispatch.FastIoQueryNetworkOpenInfo =     NwFastQueryNetworkInfo;   //  QueryNetworkInfo

    //
    //  Initialize the NwData structure.
    //

    NwData.NodeTypeCode = NWFS_NTC_DATA_HEADER;
    NwData.NodeByteSize = sizeof( NW_DATA );

    NwData.DriverObject = DriverObject;
    NwData.FileSystemDeviceObject = FileSystemDeviceObject;

    InitializeListHead( &NwData.VcbQueue );

    ExInitializeResource( &NwData.DataResource );

    //
    //  Initialize the cache manager callback routines
    //

    NwData.CacheManagerCallbacks.AcquireForLazyWrite  = &NwNoopAcquire;
    NwData.CacheManagerCallbacks.ReleaseFromLazyWrite = &NwNoopRelease;
    NwData.CacheManagerCallbacks.AcquireForReadAhead  = &NwAcquireForCache;
    NwData.CacheManagerCallbacks.ReleaseFromReadAhead = &NwReleaseFromCache;

    //
    //  Initialize the lock mutex and the async and delay close queues.
    //

    ExInitializeFastMutex( &NwData.NwDataMutex );

    InitializeListHead( &NwData.DelayedCloseQueue );

    ExInitializeWorkItem( &NwData.CloseItem,
                          (PWORKER_THREAD_ROUTINE) NwFspClose,
                          NULL );

    //
    //  Do the initialization based on the system size.
    //

    switch (MmQuerySystemSize()) {

    case MmSmallSystem:

        NwData.IrpContextMaxDepth = 4;
        NwData.MaxDelayedCloseCount = 8;
        NwData.MinDelayedCloseCount = 2;
        break;

    case MmMediumSystem:

        NwData.IrpContextMaxDepth = 8;
        NwData.MaxDelayedCloseCount = 24;
        NwData.MinDelayedCloseCount = 6;
        break;

    case MmLargeSystem:

        NwData.IrpContextMaxDepth = 32;
        NwData.MaxDelayedCloseCount = 72;
        NwData.MinDelayedCloseCount = 18;
        break;
    }

    //
    //  Initialize the bad partition table.
    //

    KeInitializeSpinLock( &NwData.BadPartitionSpinLock );

    //
    //  Initialize our Oem upcase table.
    //

    for (i=0; i<255; i++) {
        NwData.OemUpcaseTable[i] = (UCHAR)i;
    }

    (VOID)RtlOemToUnicodeN( TempWcharArray,
                            256 * sizeof(WCHAR),
                            NULL,
                            NwData.OemUpcaseTable,
                            256 );

    (VOID)RtlUpcaseUnicodeToOemN( NwData.OemUpcaseTable,
                                  256,
                                  NULL,
                                  TempWcharArray,
                                  256 * sizeof(WCHAR) );

    NwOemUpcaseTable = NwData.OemUpcaseTable;
}
