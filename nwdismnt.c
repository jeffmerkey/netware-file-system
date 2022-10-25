
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
