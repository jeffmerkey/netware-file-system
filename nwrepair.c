

/***************************************************************************
*
*   Copyright (c) 1998, 1999 Jeff V. Merkey
*   895 West Center Street
*   Orem, Utah  84057
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
*   You are free to modify and re-distribute this program in accordance
*   with the terms specified in the GNU Public License.  The copyright
*   contained in this code is required to be present in any derivative
*   works and you are required to provide the source code for this
*   program as part of any commercial or non-commercial distribution.
*   You are required to respect the rights of the Copyright holders
*   named within this code.
*
*   jeffmerkey@gmail.com is the official maintainer of
*   this code.  You are encouraged to report any bugs, problems, fixes,
*   suggestions, and comments about this software to jeffmerkey@gmail.com
*   or linux-kernel@vger.kernel.org.  New releases, patches, bug fixes, and
*   technical documentation can be found at www.kernel.org.  We will
*   periodically post new releases of this software to www.kernel.org
*   that contain bug fixes and enhanced capabilities.
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
*   AUTHOR   :  , Jeff V. Merkey
*   FILE     :  NWREPAIR.C
*   DESCRIP  :   NetWare File System Repair Tool
*   DATE     :  December 10, 1999
*
*
***************************************************************************/

#include "globals.h"


extern	ULONG input_get_number(ULONG *new_value);

int main(int argc, char *argv[])
{

    ULONG i;
    ULONG index;
    ULONG display_base = 0;
    BYTE input_buffer[20] = { "" };
    nwvp_fat_fix_info   fix_info;
    extern void UtilScanVolumes(void);
    
    InitNWFS();
    UtilScanVolumes();

    while (TRUE)
    {
       SyncDisks();

       ClearScreen(consoleHandle);
       NWFSPrint("NetWare Volume Repair Utility\n");
       NWFSPrint("Copyright (c) 1999, 2000 TRG, Inc.  All Rights Reserved.\n");

       NWFSPrint("\n  #        Volume Name   Segment   FAT Mirror  FAT Table  Dir Table \n");

       for (i=0; i < 8; i++)
       {
	  if ((index = (display_base + i)) < MaximumNumberOfVolumes)
	  {
		NWFSPrint("\n %3d ", (int) index);
		if (VolumeTable[index] != 0)
		{
			NWFSPrint(" %16s ", VolumeTable[index]->VolumeName);
			if (VolumeTable[index]->volume_segments_ok_flag == 0)
			{
				NWFSPrint("  MISSING ");
				NWFSPrint("    ****   ");
				NWFSPrint("    ****   ");
				NWFSPrint("    ****   ");

			}
			else
			{
				NWFSPrint("  COMPLETE ");
				if (VolumeTable[index]->fat_mirror_ok_flag == 0)
				{
					NWFSPrint("  MISMATCH ");
					NWFSPrint("    ****   ");
					NWFSPrint("    ****   ");
				}
				else
				{
					NWFSPrint("     OK    ");
					if (VolumeTable[index]->fat_table_ok_flag == 0)
					{
						NWFSPrint("   INVALID ");
						NWFSPrint("    ****   ");
					}
					else
					{
						NWFSPrint("     OK    ");
						if (VolumeTable[index]->directory_table_ok_flag == 0)
						{
							NWFSPrint("   INVALID ");
						}
						else
						{
							NWFSPrint("     OK    ");
						}
					}

				}
			}
		}
		else
		{
			NWFSPrint(" EMPTY ");
		}
	  }
	  else
	     NWFSPrint("\n");
       }

       NWFSPrint("\n\n\n");
       NWFSPrint("               1. Fix FAT Mirrors/Table\n");
       NWFSPrint("               2. Fix Volume Mirrors/Tables\n");
       NWFSPrint("               3. Scroll Segment List\n");
       NWFSPrint("               4. Exit Menu\n");
       NWFSPrint("        Enter a Menu Option: ");
       fgets(input_buffer, sizeof(input_buffer), stdin);
       switch (input_buffer[0])
       {
	  case '1':
	     NWFSPrint("    Enter Volume Number: ");
	     if (input_get_number(&index) == 0)
	     {
		if ((index < MaximumNumberOfVolumes) && (VolumeTable[index] != 0))
		{
			if (nwvp_vpartition_fat_fix(VolumeTable[index]->nwvp_handle, &fix_info, 1) == 0)
			{
				VolumeTable[index]->fat_mirror_ok_flag = 0xFFFFFFFF;
                                FlushLRU();
				break;
			}
			NWFSPrint("       *********  Error Fixing Fat Mirrors **********\n");
			WaitForKey();
			break;
		}
	     }
	     NWFSPrint("       *********  Invalid Volume Number **********\n");
	     WaitForKey();
	     break;
	     
	  case '2':
	     NWFSPrint("    Enter Volume Number: ");
	     if (input_get_number(&index) == 0)
	     {
		if ((index < MaximumNumberOfVolumes) && (VolumeTable[index] != 0))
		{
                        extern ULONG NWRepairVolume(VOLUME *, ULONG);
			
                        ClearScreen(consoleHandle);
			NWRepairVolume(VolumeTable[index], TRUE);
                        FlushLRU();
	                WaitForKey();
			break;
		}
	     }
	     NWFSPrint("       *********  Invalid Volume Number **********\n");
	     WaitForKey();
	     break;

	  case '3':
	     display_base += 8;
	     if (display_base >= MaximumNumberOfVolumes)
		display_base = 0;
	     break;

	  case '4':
	    NWFSVolumeClose();
	    RemoveDiskDevices();
	    FreePartitionResources();
	    return 0;
       }
    }
}




