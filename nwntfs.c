
/***************************************************************************
*
*   Copyright (c) 1997-2022 Jeff V. Merkey
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jeffmerkey@gmail.com
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the Lesser GNU Public License as published by the
*   Free Software Foundation, version 2.1, or any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   General Public License for more details.
*
*   Original Authorship      :
*      source code written by Jeff V. Merkey
*
*   Original Contributors    :
*      Jeff V. Merkey
*      
*      
*
****************************************************************************
*
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWNTFS.C
*   DESCRIP  :   NTFS Conversion Utility
*   DATE     :  November 23, 2022
*
*
***************************************************************************/

#include "globals.h"
#include "goebel.h"
#include "common.h"

extern	W_VOLUME_INFO	structNetwareVolumesToConvert[MAX_VOLUMES];
extern	ULONG	convert_handle;

extern NWSCREEN ConsoleScreen;
ULONG ntfs_menu = 0;
ULONG	map_table[MAX_VOLUMES];
BYTE	volume_display_buffer[MAX_VOLUMES][80];

ULONG ntfs_menuAction(NWSCREEN *screen, ULONG value, BYTE *option, ULONG index)
{
	ULONG	i, j, k;
	ULONG	rpart_count;
	ULONG	rpart_handles[32];
	ULONG	temp_count;
	ULONG	temp_handles[32];

	if (value < MAX_VOLUMES)
	{
		nwvp_get_physical_partitions(VolumeTable[value]->nwvp_handle, &rpart_count, &rpart_handles[0]);
		if (map_table[value] == 0)
		{
			for (i=0; i<MaximumNumberOfVolumes; i++)
			{
				if (VolumeTable[i] != 0)
				{
					nwvp_get_physical_partitions(VolumeTable[i]->nwvp_handle, &temp_count, &temp_handles[0]);
					for (j=0; j<rpart_count; j++)
					{
						for (k=0; k<temp_count; k++)
						{
							if (rpart_handles[j] == temp_handles[k])
							{
								map_table[i] = 0xFFFFFFFF;
								volume_display_buffer[i][2] = 'X';
								volume_display_buffer[i][3] = 'X';
								volume_display_buffer[i][4] = 'X';
								break;
							}
						}
					}
				}
			}
		}
		else
		{
			for (i=0; i<MaximumNumberOfVolumes; i++)
			{
				if (VolumeTable[i] != 0)
				{
					nwvp_get_physical_partitions(VolumeTable[i]->nwvp_handle, &temp_count, &temp_handles[0]);
					for (j=0; j<rpart_count; j++)
					{
						for (k=0; k<temp_count; k++)
						{
							if (rpart_handles[j] == temp_handles[k])
							{
								map_table[i] = 0;
								volume_display_buffer[i][2] = ' ';
								volume_display_buffer[i][3] = ' ';
								volume_display_buffer[i][4] = ' ';
								break;
							}
						}
					}
				}
			}

		}
	}
	display_menu(ntfs_menu);
   return 0;
}

ULONG netmigKeyPart(NWSCREEN *screen, ULONG key, ULONG index)
{
	ULONG	i;
	ULONG	number_of_netware_volumes_selected;


    switch (key)
    {
       case F2:
		number_of_netware_volumes_selected = 0;
		for (i=0; i<MaximumNumberOfVolumes; i++)
		{
			if (map_table[i] != 0)
			{
				if ((structNetwareVolumesToConvert[number_of_netware_volumes_selected].volume = VolumeTable[i]) != 0)
				{
					if (nwvp_get_physical_partitions(structNetwareVolumesToConvert[number_of_netware_volumes_selected].volume->nwvp_handle,
						&structNetwareVolumesToConvert[number_of_netware_volumes_selected].rpart_count,
						&structNetwareVolumesToConvert[number_of_netware_volumes_selected].rpart_handle[0]) == 0)
					{
						number_of_netware_volumes_selected ++;
					}
				}
			}
		}
		if (number_of_netware_volumes_selected > 0)
		{
			if (NTFSConvert(number_of_netware_volumes_selected) == 0)
				ntfs_convert_commit(convert_handle);
			ntfs_convert_end(convert_handle);
		}
		break;
    }
    return 0;
}

#ifndef NWNTFS_LIB
int main(void)
#else
int NWNTFS(void)
#endif
{
    register ULONG i, retCode;
    BYTE displayBuffer[255];

    InitNWFS();
    NWFSVolumeScan();

#ifndef NWNTFS_LIB
    if (InitConsole())
       return 0;
#endif

#if (LINUX_UTIL)
    for (i=0; i < ConsoleScreen.nLines - 1; i++)
       put_char_cleol(&ConsoleScreen, ' ', i, CYAN | BGBLUE);
#endif

#if (DOS_UTIL | WINDOWS_NT_UTIL)
    for (i=0; i < ConsoleScreen.nLines - 1; i++)
       put_char_cleol(&ConsoleScreen, 0xB0, i, CYAN | BGBLUE);
#endif

#if (WINDOWS_NT_UTIL)
    sprintf(displayBuffer, "  NetMigrate for Windows NT");
    put_string_cleol(&ConsoleScreen, displayBuffer, 0, BLUE | BGCYAN);
#endif

#if (DOS_UTIL)
    sprintf(displayBuffer, "  NetMigrate for DOS");
    put_string_cleol(&ConsoleScreen, displayBuffer, 0, BLUE | BGCYAN);
#endif

#if (LINUX_UTIL)
    sprintf(displayBuffer, "  NetMigrate for Linux");
    put_string_cleol(&ConsoleScreen, displayBuffer, 0, BLUE | BGCYAN);
#endif

    sprintf(displayBuffer,
	   "  Copyright (c) 1999, 2000 Jeff V. Merkey  All Rights Reserved.");
    put_string_cleol(&ConsoleScreen, displayBuffer, 1, BLUE | BGCYAN);


#if (LINUX_UTIL)
    sprintf(displayBuffer, "  F1-Help  F2-Convert ENTER-Mark F3-Exit ");
#else
    sprintf(displayBuffer, "  F1-Help  F2-Convert ENTER-Mark ESC-Exit");
#endif
    write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

    ntfs_menu = make_menu(&ConsoleScreen,
		     "   Mark  Volume Name                          ",
		     3,
		     4,
		     5,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     ntfs_menuAction,
		     0,
		     netmigKeyPart,
		     TRUE,
		     MAX_VOLUMES);

    if (!ntfs_menu)
	  goto ErrorExit;

    for (i=0; i<MaximumNumberOfVolumes; i++)
    {
	if (VolumeTable[i] != 0)
	{
		if (MountUtilityVolume(VolumeTable[i]) == 0)
		{
		    sprintf(&volume_display_buffer[i][0], " %s   %s ", ((map_table[i] & 0x80000000) != 0) ? "XXX" : "   ", VolumeTable[i]->VolumeName);
		    add_item_to_menu(ntfs_menu, &volume_display_buffer[i][0], i);
                }
	}
    }

    retCode = activate_menu(ntfs_menu);

ErrorExit:;
    sprintf(displayBuffer, " Exiting ... ");
    write_screen_comment_line(&ConsoleScreen, displayBuffer, BLUE | BGWHITE);

    if (ntfs_menu)
       free_menu(ntfs_menu);

#ifndef NWNTFS_LIB
    ReleaseConsole();
#endif

    return 0;

}





