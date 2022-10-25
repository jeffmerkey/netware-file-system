/*++

Copyright (c) 1999-2022  Jeffrey V. Merkey

Module Name:

    StrucSup.c

Abstract:

    This module is just a hack include of Jeff's include files.

Author:

    David Goebel (davidg@balder.com) 11-Mar-1999

--*/

typedef UCHAR BYTE;
typedef USHORT WORD;

#undef _NWFS_  // this is needed to avoid some double defines

#include "..\nwfs\globals.h"

typedef HASH *PHASH;

extern PDISK_ARRAY PartitionArray;

extern
HASH *HashFindFile(IN ULONG VolumeNumber,
                   IN ULONG Parent,
                   IN POEM_STRING Name,
                   IN BOOLEAN IgnoreCase);


extern
HASH *HashFindNext(IN     ULONG VolumeNumber,
                   IN OUT ULONG *SlotNumber,
                   IN     ULONG Parent,
                   IN OUT PVOID *Context,
                   IN     BOOLEAN Current);

extern
NTSTATUS
NwMountAll (
    IN PDISK_ARRAY
    );

// this is the david goebel routine that calls from the NWFSRO code to
// mount all netware volumes on a system.

extern
ULONG NWFSConvertFromNetwareTime(IN ULONG DateAndTime,
                                 IN PTIME_FIELDS tf);

// this routine is specifically for Windows NT, and converts Netware
// date and time into something digestible by the Windows NT R/O
// driver.

extern
ULONG
DismountVolume(IN BYTE *VolumeName);

// this function will dismount a volume by name.  The name must be
// already uppercase, and no more than 15 characters in length.  This
// function is non-NT specific.

extern
VOID
AddDiskDevices(IN PDISK_ARRAY DiskArray);

//
// This is an NT specific entry point for passing a disk array
// table of pointers into the driver.  It exists because the
// ScanDiskDevices() has a standard interface semantic across
// multiple platforms, and we want to maintain this clean
// abstraction.  Since the NT R/O driver has special requirements,
// this function is used to map the NWFSRO driver to a semantic
// more natural to our ScanDiskDevices() interface.
//

extern
VOID
RemoveDiskDevices(VOID);

// this routine frees any geometry or disk related memory.  We have
// this lowest layer cleanup in the event there are dependencies
// on partition level memory objects needing to be free first.
// In the NT version of the driver, this distinction is not really
// needed, however, in the Linux version, it is needed since there
// can be sync objects blocked at the partition level that require
// all resources to be released clean prior to shutting dow the disk
// devices.  This is how we prevent someone from unloading a driver
// in Linux while folks at the VFS layer may hold resources.

extern
VOID
FreePartitionResources(VOID);

// this routine frees any geometry or disk related memory.  We have
// this lowest layer cleanup in the event there are dependencies
// on partition level memory objects needing to be free first.
// In the NT version of the driver, this distinction is not really
// needed, however, in the Linux version, it is needed since there
// can be sync objects blocked at the partition level that require
// all resources to be released clean prior to shutting dow the disk
// devices.  This is how we prevent someone from unloading a driver
// in Linux while folks at the VFS layer may hold resources.

extern
ULONG MountVolume(BYTE *VolumeName);

// this routine mounts a specific volume by text name.
// This routine is non-NT specific.  (NOTE:  the name must be
// uppercase ENGLISH text and not more than 15 characters in
// length.

extern
ULONG DismountVolume(BYTE *VolumeName);

// this routine mounts a specific volume.  it takes the address of
// the VolumeTable[i]->xxx pointer (non-NT specific).

extern
ULONG MountAllVolumes();

// this routine mounts all volumes on a server and is called from the
// NWFS library (non-NT specific)

extern
NW_STATUS
TrgGetPartitionInfoFromVolumeInfo (
    IN ULONG VolumeNumber,
    IN ULONG SectorOffsetInVolume,
    IN ULONG DesiredSectorCount,
    OUT void **PartitionPointer,
    OUT ULONG *SectorOffsetInPartition,
    OUT ULONG *SectorsAvailable
    );

/*++

Routine Description:

    This routine maps volume information to partitions.  In the case of
    mirrors, the most current mirror is used.

Arguments:

    VolumeNumber - The slot number in the volume table that contains the file.

    SectorOffsetInVolume - The starting sector offset into the volume for which
        we want mapping information to a partition.

    DesiredSectorCount - The number of sectors for which information is desired.

    PartitionPointer - Receives the device object object pointer (supplied
        during mount) of the partition that contains the supplied starting
        sector in the volume.

    SectorOffsetInPartition - Receives the starting sector offset into the
        partition corresponding to the starting sector offste in the volume.

    SectorsAvailable - Receives the number of sectors left in the segment
        decribed by SectorOffsetInPartition, up to DesiredSectorCount.  This
        will generally be DesiredSectorCount unless the request spans a
        segment boundry or a hotfix region.

Return Value:

    NW_STATUS - NwSuccess if we processed the request without error.
        NwFileCorrupt if the mapping information is malformed, or other
        error as appropriate.

--*/

extern
NW_STATUS TrgLookupFileAllocation(IN   ULONG VolumeNumber,
                                  IN   HASH *hash,
                                  IN   ULONG *StartingSector,
                                  IN   ULONG *Context,
                                  OUT  ULONG *SectorOffsetInVolume,
                                  OUT  ULONG *SectorCount);
/*++

Routine Description:

    This routine obtains the file mapping information for a file, that is,
    where a file lives on the disk.  It is intended to be called in an
    itterative fasion, thus uses Context to keep track of where it is.

    Each time this routine is called, it should return a 'run' of the file.
    A run means a contiguous set of physical sectors containing the file.
    By enumerating this function, the entire mapping for the file can be built.

Arguments:

    VolumeNumber - The slot number in the volume table that contains the file.

    Hash - Points to a file hash value.  Only the DOS hash and MAC hash are
        valid input parameters, though a value of NULL means tha we want the
        mapping information for the volume itself (i.e. for a DASD read).

    StartingSector - On input, contains the desired starting virtual offset
        in the file for which we need mapping information.  On NwSuccess
        return, receives the actual starting virtual offset of the returned run.

    Context - This parameter is treated as opaque by the caller and is used by
        the routine to keep track of where it is in the file.

    SectorOffsetInVolume - On NwSuccess return, receives the offset into
        the volume of the start of the run.

    SectorCount - On NwSuccess return, receives the number of contiguous
        sectors in the run.

Return Value:

    NW_STATUS - NwSuccess if we processed the request without error.
        NwFileCorrupt if the mapping information is malformed, or other
        error as appropriate.

--*/

extern
VOID
NwReportDiskIoError(
    IN void *FailedPartitionPointer,
    IN BOOLEAN TotalDeviceFailure
    );

/*++

Routine Description:

    This routine reports that we failed an I/O on a partition.  It allows us
    to read from an alternate mirror.

Arguments:

    FailedPartitionPointer - The partition device object that failed.

    TotalDeviceFailure - If true, then the entire device appears to have
        failed. If false, then individual sectors appear to have failed.

Return Value:

    None.

--*/

