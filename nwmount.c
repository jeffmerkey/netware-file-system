
/***************************************************************************
*
*   Copyright (c) 1999-2000 Jeff V. Merkey
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jeffmerkey@gmail.com
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the TRG Windows NT/2000(tm) Public License (WPL) as
*   published by the Jeff V. Merkey  (Windows NT/2000
*   is a registered trademark of Microsoft Corporation).
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
*   You should have received a copy of the WPL Public License along
*   with this program; if not, a copy can be obtained from
*   repo.icapsql.com.
*
*   You are free to modify and re-distribute this program in accordance
*   with the terms specified in the TRG Windows NT/2000 Public License (WPL).
*   The copyright contained in this code is required to be present in
*   any derivative works and you are required to provide the source code
*   for this program as part of any commercial or non-commercial
*   distribution. You are required to respect the rights of the
*   Copyright holders named within this code.
*
*   jeffmerkey@gmail.com is the official maintainer of
*   this code.  You are encouraged to report any bugs, problems, fixes,
*   suggestions, and comments about this software to jeffmerkey@gmail.com
*   .  New releases, patches, bug fixes, and
*   technical documentation can be found at repo.icapsql.com.  We will
*   periodically post new releases of this software to repo.icapsql.com
*   that contain bug fixes and enhanced capabilities.
*
****************************************************************************
*
*  Module Name:
*
*    NwMount.c
*
*  Abstract:
*
*    This command line APP attempts to mount a read-only netware volume.
*
*  Authors:
*
*    David Goebel (davidg@balder.com) 19-Jan-1999
*    Jeff Merkey (jeffmerkey@gmail.com) 19-Jan-1999
*
***************************************************************************/

#include "windows.h"
#include "winioctl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "NwIoctl.h"

#define MAX_DEV_NAME_LENGTH     128
#define SCAN_DISKS              256
#define NETWARE_286_ID          0x64
#define NETWARE_386_ID          0x65
#define PARTITION_ID            0xAA55

#define FileHandleReadOnly(lpszFilePath)     \
   (HANDLE) CreateFile (lpszFilePath,        \
      GENERIC_READ,                          \
      FILE_SHARE_READ | FILE_SHARE_WRITE,    \
      NULL, OPEN_EXISTING,                   \
      FILE_ATTRIBUTE_NORMAL, NULL)

/* Partition Table Entry info. 16 bytes */

struct PartitionTableEntry
{
    BYTE fBootable;
    BYTE HeadStart;
    BYTE SecStart;
    BYTE CylStart;
    BYTE FATType;
    BYTE HeadEnd;
    BYTE SecEnd;
    BYTE CylEnd;
    ULONG StartLBA;
    ULONG nSectorsTotal;
};

BYTE DiskName[256]= "";
BYTE PartitionName[256]= "";
HANDLE DiskHandles[SCAN_DISKS];
HANDLE PartitionHandles[SCAN_DISKS];
LONGLONG AlignedBuffer[512/sizeof(LONGLONG)];
BYTE *Buffer;

HANDLE PartitionOpen(IN DWORD DiskNum, IN DWORD PartNum);

BOOL CommonRead (HANDLE hFile, VOID *lpMemory, DWORD nAmtToRead)
{
   BOOL           bSuccess ;
   DWORD          nAmtRead ;

   bSuccess = ReadFile (hFile, lpMemory, nAmtToRead, &nAmtRead, NULL) ;
   return (bSuccess && (nAmtRead == nAmtToRead)) ;

}

ULONG PartitionRead(HANDLE Handle, ULONG LBA, BYTE *Sector, ULONG sectors)
{
    register ULONG retCode;
    LARGE_INTEGER offset;

    offset.QuadPart = UInt32x32To64(LBA, 512);

    retCode = SetFilePointer(Handle, offset.LowPart,
			     &offset.HighPart, FILE_BEGIN);

    if (retCode == (ULONG) -1)
	return 0;

    retCode = CommonRead(Handle, Sector, sectors * 512);

    return retCode;

}

VOID __cdecl main(int argc, char *argv[])
{
    register ULONG i, j, retCode;
    DWORD BytesReturned;
    ULONG arg = 1;
    HANDLE Device;
    HANDLE DiskHandle;
    HANDLE PartitionHandle;
    PDISK_ARRAY DiskArray;
    WORD partitionSignature;
    struct PartitionTableEntry PartitionTable[4];
    ULONG TotalPartitions;
    BOOLEAN AssignDriveLetters = FALSE;
    PUCHAR DriveLetterSpecified = NULL;
    PUCHAR VolumeName = NULL;
    ULONG NewMountedVolumes = 0;

    if (argc < 2)
    {
usage:
		printf("USAGE: %s [VolumeName] {drive_letter | *}\n\n", argv[0]);
        printf("       If VolumeName is not specified, all volumes are mounted.\n");
        printf("       If * is specified for drive letter, %s picks the drive letters.\n", argv[0]);
		return;
    }

    while (arg < (ULONG)argc)
    {
		if (argv[arg][0] == '*')
		{
		    AssignDriveLetters = TRUE;
		}
		else
		if (argv[arg][1] == ':')
		{
		    argv[arg][0] = toupper(argv[arg][0]);
		    if ((argv[arg][0] >= 'A') &&
			(argv[arg][0] < 'Z') &&
			(((1 << (argv[arg][0] - 'A')) & GetLogicalDrives()) == 0))
			{
				DriveLetterSpecified = argv[arg];
		    }
		}
		else
		{
		    VolumeName = argv[arg];
		    strupr(VolumeName);
		}
		arg++;
    }

    if ((VolumeName == NULL) && !AssignDriveLetters && !DriveLetterSpecified) {
	goto usage;
    }

    Buffer = (BYTE *)&AlignedBuffer[0];

    //
    //  Second, open the Netware Read-Only File System device.
    //

    if ((Device = CreateFile( NWFS_DEVICE_NAME_W32,
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              0 )) == INVALID_HANDLE_VALUE) {
        printf("NWMOUNT-Error [%d] attempting to open device NWFSRO.\n", GetLastError());
        printf("please make sure the driver is loaded.\n");
        return;
    }

    //
    //  now scan and build a table of disk and partition objects to
    //  pass to the file system
    //

    for (TotalPartitions = i = 0; i < SCAN_DISKS; i++)
    {
        sprintf(DiskName, "\\\\.\\PhysicalDrive%d", i);
        DiskHandle = FileHandleReadOnly(DiskName);
        if (DiskHandle == INVALID_HANDLE_VALUE)
        {	
		    if (GetLastError() == ERROR_FILE_NOT_FOUND)
		    {
		       break;
		    }
		    else
		    {
				printf("NWMOUNT-Error [%d] attempting to open %s.\n",
				       GetLastError(), DiskName);
				continue;
		    }
		}

		retCode = PartitionRead(DiskHandle, 0, Buffer, 1);
		if (!retCode)
		{
		    printf("NWMOUNT-Error [%d] attempting to read sector 0\n", GetLastError());
		    continue;
		}

		memmove(&partitionSignature, &Buffer[0x01FE], 2);
		if (partitionSignature != PARTITION_ID)
		{
		    printf("NWMOUNT-partition table signature invalid disk(%d)\n", i);
		    continue;
		}

		memmove(&PartitionTable[0].fBootable, &Buffer[0x01BE], 64);
		for (j=0; j < 4; j++)
		{
		    if ((!PartitionTable[j].nSectorsTotal) ||
			(PartitionTable[j].FATType != NETWARE_386_ID))
		    {
			// only report disks with NetWare partitions on them
				continue;
		    }

			printf("\\Device\\HardDisk%d\\Partition%d is a Netware partition.\n", i, j+1);

		    PartitionHandle = PartitionOpen(i,j);
		    if (PartitionHandle == INVALID_HANDLE_VALUE)
				break;

		    DiskHandles[TotalPartitions] = DiskHandle;
			PartitionHandles[TotalPartitions] = PartitionHandle;
			TotalPartitions++;
		}
    }

    //
    //  now that we have our disk table, allocate the disk array and
    //  pass the array to the file system.
    //

    printf("%d partitions detected\n", TotalPartitions);

    if (!TotalPartitions)
    {
        return;
    }

    DiskArray = (PDISK_ARRAY) malloc(sizeof(DISK_ARRAY) + (sizeof(DISK_HANDLES) * TotalPartitions - 1));
    if (!DiskArray)
        return;
    memset(DiskArray, 0, sizeof(DISK_ARRAY) + (sizeof(DISK_HANDLES) * TotalPartitions - 1));

    DiskArray->PartitionCount = TotalPartitions;
    DiskArray->StructureSize = sizeof(DISK_ARRAY) + (sizeof(DISK_HANDLES) * (TotalPartitions - 1));
    for (i=0; i < TotalPartitions; i++)
    {
        printf("disk(0x%lx) part(0x%lx)\n", DiskHandles[i], PartitionHandles[i]);
        DiskArray->Handles[i].Partition0Handle = DiskHandles[i];
        DiskArray->Handles[i].PartitionHandle = PartitionHandles[i];
    }

    if (VolumeName)
    {
	strncpy(DiskArray->VolumeLabel, VolumeName, 16);
	DiskArray->FunctionCode = NWFS_MOUNT_VOLUME;
    }
    else
    {
	DiskArray->FunctionCode = NWFS_MOUNT_ALL_VOLUMES;
    }

    //
    //  Now Pass Value Information to the Driver.
    //

    DiskArray->MajorVersion = NWFSRO_MAJOR_VERSION;
    DiskArray->MinorVersion = NWFSRO_MINOR_VERSION;

    if (!DeviceIoControl(Device,
			 (DWORD)IOCTL_QUERY_ALL_DISKS,
			 DiskArray,
			 DiskArray->StructureSize,
			 &NewMountedVolumes,
			 sizeof(ULONG),
			 &BytesReturned,
			 NULL ) ||
	(BytesReturned != sizeof(ULONG)))
    {
	printf("NWMOUNT-Error [%d] from DeviceIoControl().\n", GetLastError());
	free(DiskArray);
	return;
    }

    printf("NewMountedVolumes = 0x%08lx.\n", NewMountedVolumes);

    free(DiskArray);

    for (i=0; i < 32; i++)
    {
	PUCHAR DriveLetter;
	PUCHAR Proto = "c:";
	UCHAR TargetDevice[32];

	if ((NewMountedVolumes & (1 << i)) == 0)
	{
	    continue;
	}

	if (DriveLetterSpecified)
	{
	    DriveLetter = DriveLetterSpecified;
	}
	else
	{
	    ULONG DrivesAvailable;

	    DrivesAvailable = ~(GetLogicalDrives() | 3);
	    for (j=0; j < 26;j++)
	    {
		if (DrivesAvailable & (1 << j))
		{
		    break;
		}
	    }
	    if (j == 26)
	    {
		printf("Ran out of drive letters.\n");
		return;
	    }
	    Proto[0] = 'a'+ (UCHAR)j;
	    DriveLetter = Proto;
	}

	sprintf(TargetDevice, "\\Device\\NetwareDisk%02d", i);

	printf("Using drive letter %c: for device %s.\n", DriveLetter[0], TargetDevice);

	if (!DefineDosDevice(DDD_RAW_TARGET_PATH, DriveLetter, TargetDevice))
	{
	    printf("NWMOUNT-Error [%d] setting drive letter from DefineDosDevice\n",
		       GetLastError());
	}
    }
    return;

}

HANDLE PartitionOpen(IN DWORD DiskNum, IN DWORD PartNum)
{
    HANDLE Handle;

    sprintf(PartitionName, "\\Device\\Harddisk%d\\Partition%d", DiskNum, PartNum+1);

    printf("    PartitionOpen: Attemping open on %s.\n", PartitionName);

    if (DefineDosDevice(DDD_RAW_TARGET_PATH, "NwFsRoTemp",
			PartitionName))
    {

	Handle = FileHandleReadOnly("\\\\.\\NwFsRoTemp");
	if (Handle == INVALID_HANDLE_VALUE)
	{
	    printf("NWMOUNT-Error [%d] attempting to open %s.\n",
		   GetLastError(), PartitionName);
	    (VOID)DefineDosDevice(DDD_REMOVE_DEFINITION, "NwFsRoTemp", NULL);
	    return INVALID_HANDLE_VALUE;
	}

	if (!DefineDosDevice(DDD_REMOVE_DEFINITION, "NwFsRoTemp",
			     NULL))
	{
	    printf("NWMOUNT-Error [%d] detected during DefineDosDevice\n",
		   GetLastError());
	    CloseHandle( Handle );
	    return INVALID_HANDLE_VALUE;
	}

	printf("    PartitionOpen: Returning handle 0x%lx.\n", Handle);

	return Handle;

    }
    else
    {
        printf("NWMOUNT-Error [%d] detected during DefineDosDevice\n",
               GetLastError());
        return INVALID_HANDLE_VALUE;
    }
}

