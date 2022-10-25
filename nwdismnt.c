
/***************************************************************************
*
*   Copyright (c) 1999-2000 Jeff V. Merkey
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jeffmerkey@gmail.com
*
****************************************************************************
*
*  Module Name:
*
*    NwDismount.c
*
*  Abstract:
*
*    This little command line APP attempts to dismount a previously mounted
*    read-only netware volume.
*
*  Authors:
*
*    David Goebel (davidg@balder.com) 11-Mar-1999
*
*
***************************************************************************/

#include "stdio.h"
#include "windows.h"
#include "winioctl.h"

void __cdecl main(int argc, char **argv)
{
    HANDLE Volume;
    PUCHAR Drive = "\\\\.\\c:\\";
    ULONG DontCare;
    PUCHAR FileSystemName = "NWFSRO";
    BOOLEAN Worked;

    if (argc != 2)
    {
	printf("USAGE: %s X: (attempts to dismount drive X:)\n\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    Drive[4] = argv[1][0];

    if (!GetVolumeInformation( Drive,
			       NULL,
			       0,
			       &DontCare,
			       &DontCare,
			       &DontCare,
			       FileSystemName,
			       7 ) ||
	(memcmp(FileSystemName, "NWFSRO", 6) != 0))
    {
	printf("%s is only to be used on read-only netware partitions.\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    Drive[6] = 0;

    if ((Volume = CreateFile( Drive,
			      GENERIC_READ,
			      0,
			      NULL,
			      OPEN_EXISTING,
			      FILE_ATTRIBUTE_NORMAL,
			      NULL)) == ((HANDLE)-1))
    {
	printf("Could not open and/or get a lock on drive %s, error %d\n",
	       Drive, GetLastError());

        exit(EXIT_FAILURE);
    }

    printf("Attempting dismount on drive %s.\n", &Drive[4]);

    Worked = DeviceIoControl( Volume,
			      FSCTL_DISMOUNT_VOLUME,
			      NULL,
			      0,
			      NULL,
			      0,
			      &DontCare,
			      NULL );
    if (!Worked)
    {
	printf("DeviceIoControl() returned error %d.\n", GetLastError());
	CloseHandle(Volume);
	exit(EXIT_FAILURE);
    }

    if (!DefineDosDevice(DDD_REMOVE_DEFINITION, &Drive[4], NULL))
    {
	printf("DefineDosDevice() returned error %d.\n", GetLastError());
	CloseHandle(Volume);
	exit(EXIT_FAILURE);
    }

    CloseHandle(Volume);
    exit(EXIT_SUCCESS);
}
