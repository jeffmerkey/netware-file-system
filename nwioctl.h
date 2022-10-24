/*++

Copyright (c) 1989-1999  Microsoft Corporation
Copyright (c) 1998-1999  Jeff V. Merkey

Module Name:

    NwIoctl.h

Abstract:

    This module defines the data structures used to communicate the
    Disk/Segment/Volume relationships of Netware volumes, Trustee Database
    information, and NetWare Name Space Information from User Mode
    to Kernel Mode.

    It is included by both the user mode MountVolume tool and the kernel
    NwfsRo.sys driver.

Authors:

    David Goebel (davidg@balder.com) 19-Jan-1999
    Jeff Merkey (jeffmerkey@gmail.com) 19-Jan-1999

--*/

#ifndef _NWIOCTL_
#define _NWIOCTL_

//
//  First define the name of the file system device.
//

#define NWFS_DEVICE_NAME        L"NwFsRo"
#define NWFS_DEVICE_NAME_W32    "\\\\.\\NwFsRo"

//
//  These are the current versions that must be in sync.
//

#define NWFSRO_MAJOR_VERSION    2
#define NWFSRO_MINOR_VERSION    0

//
//  First define the Ioctl code to be used to transmit the information
//  from user mode to kernel mode.  Note that we need hooks for both
//  IRP_MJ_DEVICE_CONTROL and IRP_MJ_FILE_SYSTEM_CONTROL as the former
//  gets invoked on NT 4 and the later on NT 5.
//

#define IOCTL_QUERY_ALL_DISKS   CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, \
                         0xD414,                   \
                         METHOD_BUFFERED,          \
                         FILE_READ_ACCESS)
#define FSCTL_QUERY_ALL_DISKS    IOCTL_QUERY_ALL_DISKS

#define IOCTL_MOUNT_VOLUME          CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, \
                         0xD415,                       \
                         METHOD_BUFFERED,              \
                         FILE_READ_ACCESS)
#define FSCTL_MOUNT_VOLUME          IOCTL_MOUNT_VOLUME

#define IOCTL_VOLUME_TRUSTEE_INFO   CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, \
                         0xD416,                       \
                         METHOD_BUFFERED,              \
                         FILE_READ_ACCESS)
#define FSCTL_VOLUME_TRUSTEE_INFO   IOCTL_VOLUME_TRUSTEE_INFO

#define IOCTL_VOLUME_USER_RESTRICT  CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, \
                         0xD417,                       \
                         METHOD_BUFFERED,              \
                         FILE_READ_ACCESS)
#define FSCTL_VOLUME_USER_RESTRICT  IOCTL_VOLUME_USER_RESTRICT

#define IOCTL_VOLUME_NAME_SPACE     CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, \
                         0xD418,                       \
                         METHOD_BUFFERED,              \
                         FILE_READ_ACCESS)
#define FSCTL_VOLUME_NAME_SPACE     IOCTL_VOLUME_NAME_SPACE

//
//  The following structues passed in to IOCTL_QUERY_ALL_DISKS
//

typedef struct _DISK_HANDLES
{
    HANDLE Partition0Handle;  // disk
    HANDLE PartitionHandle;   // partition
} DISK_HANDLES, *PDISK_HANDLES;

typedef struct _DISK_POINTERS
{
    PVOID Partition0Pointer;  // disk
    PVOID PartitionPointer;   // partition
} DISK_POINTERS, *PDISK_POINTERS;

typedef struct _DISK_ARRAY
{
    //
    //  Version info so that the file system can detect a mis-match.
    //

    USHORT MajorVersion;
    USHORT MinorVersion;

    //
    //  The total size of the structure.  Used for sanity checking when
    //  enumerating through the segment layout records.
    //

    ULONG StructureSize;

    //
    //  Function code to enhace functionality for the future.  Currently
    //  only two values are legal.
    //

    ULONG FunctionCode;
    ULONG PartitionCount;

    //
    //  If mounting a volume, here is the label
    //

    UCHAR VolumeLabel[16];

    union {

        DISK_HANDLES Handles[1];
        DISK_POINTERS Pointers[1];
    };

} DISK_ARRAY, *PDISK_ARRAY;

#define NWFS_MOUNT_VOLUME       0x00000101
#define NWFS_MOUNT_ALL_VOLUMES  0x00000102

#endif // _NWIOCTL_
