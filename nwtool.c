
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
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWTOOL.C
*   DESCRIP  :   NWFS Tools
*   DATE     :  March 27, 2000
*
*
***************************************************************************/

#include "globals.h"

extern ULONG segment_table_count;
extern segment_info_table segment_table[256];
extern ULONG segment_mark_table[256];
extern ULONG hotfix_table_count;
extern hotfix_info_table hotfix_table[256];
extern BYTE NwPartSignature[16];
extern BYTE NetwareBootSector[512];
extern ULONG FirstValidDisk;
extern BYTE *NSDescription(ULONG);


#if (DOS_UTIL | LINUX_UTIL | WINDOWS_NT_UTIL)

void UtilScanVolumes(void)
{
    NWFSPrint("\nScanning Disks ... \n");
    NWFSVolumeScan();
}

ULONG input_get_number(ULONG *new_value)
{
    ULONG hex_value = 0;
    ULONG dec_value = 0;
    ULONG dec_flag = 1;
    BYTE *bptr;
    BYTE input_buffer[20] = { "" };

    fgets(input_buffer, sizeof(input_buffer), stdin);

    bptr = input_buffer;
    while (bptr[0] == ' ')
       bptr++;

    if ((bptr[0] == '0') && ((bptr[1] == 'x') || (bptr[1] == 'X')))
    {
       bptr += 2;
       dec_flag = 0;
    }

    if (((bptr[0] >= '0') && (bptr[0] <= '9')) ||
	((bptr[0] >= 'a') && (bptr[0] <= 'f')) ||
	((bptr[0] >= 'A') && (bptr[0] <= 'F')))
    {
       while ((*bptr != 0) && (*bptr != 0xA) && (*bptr != 0xD))
       {
	  if ((bptr[0] >= '0') && (bptr[0] <= '9'))
	  {
	     dec_value = (dec_value * 10) + (bptr[0] - '0');
	     hex_value = (hex_value * 16) + (bptr[0] - '0');
	  }
	  else
	  if ((bptr[0] >= 'a') && (bptr[0] <= 'f'))
	  {
	     dec_value = (dec_value * 10) + (bptr[0] - 'a') + 10;
	     hex_value = (hex_value * 16) + (bptr[0] - 'a') + 10;
	     dec_flag = 0;
	  }
	  else
	  if ((bptr[0] >= 'A') && (bptr[0] <= 'F'))
	  {
	     dec_value = (dec_value * 10) + (bptr[0] - 'A') + 10;
	     hex_value = (hex_value * 16) + (bptr[0] - 'A') + 10;
	     dec_flag = 0;
	  }
	  else
	  {
	     if (bptr[0] == ' ')
		break;
	     return (-1);
	  }
	  bptr ++;
       }
       *new_value = (dec_flag == 0) ? hex_value : dec_value;
       return (0);
    }
    return (-1);
}

// this function will allocate all available space on all disks as Netware
// partitions.  NOTE:  this function creates Netware 4.x/5.x style
// partitions.

ULONG NWAllocateUnpartitionedSpace(void)
{
    ULONG disk, i, j, vhandle, raw_ids[4];
    ULONG TotalSectors, FreeSectors, AdjustedSectors;
    ULONG AllocatedSectors, freeSlots;
    ULONG LBAStart, Length, LBAWork, LBAEnd;
    BYTE *Buffer;
    register ULONG retCode;
    register PART_SIG *ps;
    ULONG SlotCount;
    ULONG SlotIndex;
    ULONG SlotMask[4];
    ULONG SlotLBA[4];
    ULONG SlotLength[4];
    ULONG FreeCount;
    ULONG FreeLBA[4];
    ULONG FreeLength[4];
    ULONG FreeSlotIndex[4];
    nwvp_rpartition_scan_info	rlist[5];
    nwvp_payload payload;
    ULONG logical_capacity;
    BYTE workBuffer[20] = {""};

    if (FirstValidDisk == (ULONG)-1)
    {
       NWFSPrint("\nNo fixed disks could be located.\n");
       return -1;
    }
    
    Buffer = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Buffer)
    {
       NWFSPrint("could not allocate partition workspace\n");
       return -1;
    }
    NWFSSet(Buffer, 0, IO_BLOCK_SIZE);

    UtilScanVolumes();

    NWFSPrint("Allocating all unpartitioned space for Netware 4.x/5.x\n");
    for (disk=FirstValidDisk; disk < TotalDisks; disk++)
    {
       if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
       {
#if (VERBOSE)
	  NWFSPrint("disk found-%d total-%d\n", (int)disk, (int)TotalDisks);
#endif
	  // init the partition marker fields
	  for (i=0; i < 4; i++)
	     SystemDisk[disk]->PartitionFlags[i] = 0;

	  if (ValidatePartitionExtants(disk))
	  {
	     NWFSPrint("disk-(%d) partition table is invalid - reinitializing table\n",
		   (int)disk);

	     NWFSPrint("WARNING !!! this operation will erase all data on \nthis drive.  Continue? [Y/N]: ");
	     fgets(workBuffer, sizeof(workBuffer), stdin);
	     if (toupper(workBuffer[0]) != 'Y')
                goto ScanNextDisk;

	     retCode = ReadDiskSectors(disk, 0, Buffer, 1, 1);
	     if (!retCode)
	     {
		NWFSPrint("FATAL:  disk-%d error reading boot sector\n", (int)disk);
                goto ScanNextDisk;
	     }
	     NWFSCopy(&SystemDisk[disk]->partitionSignature, &Buffer[0x01FE], 2);

	     // if there is no valid partition signature, then
	     // write a new master boot record.

	     if (SystemDisk[disk]->partitionSignature != 0xAA55)
	     {
		NWFSSet(Buffer, 0, 512);
		NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
		SystemDisk[disk]->partitionSignature = 0xAA55;
		NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
	     }

	     // zero the partition table and partition signature
	     NWFSSet(&SystemDisk[disk]->PartitionTable[0].fBootable, 0, 64);
	     NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);

	     // write the partition table to sector 0
	     retCode = WriteDiskSectors(disk, 0, Buffer, 1, 1);
	     if (!retCode)
	     {
		NWFSPrint("FATAL:  disk-%d error writing boot sector\n", (int)disk);
                goto ScanNextDisk;
	     }
	     UtilScanVolumes();
	  }

          //
          //
          //

	  TotalSectors = (ULONG)((SystemDisk[disk]->Cylinders) *
			         SystemDisk[disk]->TracksPerCylinder *
			         SystemDisk[disk]->SectorsPerTrack);

	  if (TotalSectors < SystemDisk[disk]->SectorsPerTrack)
	  {
             NWFSPrint("nwfs:  sector total range error\n");
             goto ScanNextDisk;
	  }

          // the first cylinder of all hard drives is reserved.
	  AdjustedSectors = (TotalSectors - SystemDisk[disk]->SectorsPerTrack);

          // scan for free slots and count the total free sectors
	  // for this drive.

	  for (freeSlots = AllocatedSectors = 0, i = 0; i < 4; i++)
	  {
             SlotMask[i] = 0;
             SlotLBA[i] = 0;
             SlotLength[i] = 0;
             FreeLBA[i] = 0;
             FreeLength[i] = 0;

	     if (SystemDisk[disk]->PartitionTable[i].SysFlag &&
                 SystemDisk[disk]->PartitionTable[i].nSectorsTotal &&
                 SystemDisk[disk]->PartitionTable[i].StartLBA)
	     {
		AllocatedSectors +=
                       SystemDisk[disk]->PartitionTable[i].nSectorsTotal;
	     }
	     else
	     {
                FreeSlotIndex[freeSlots] = i;
		freeSlots++;
	     }
	  }

          // now sort the partition table into ascending order
          for (SlotCount = i = 0; i < 4; i++)
          {
             // look for the lowest starting lba
             for (SlotIndex = LBAStart = (ULONG) -1, j = 0; j < 4; j++)
             {
		if (SystemDisk[disk]->PartitionTable[j].SysFlag &&
                    !SlotMask[j])
                {
                   if (SystemDisk[disk]->PartitionTable[j].StartLBA < LBAStart)
                   {
                      LBAStart = SystemDisk[disk]->PartitionTable[j].StartLBA;
                      SlotIndex = j;
                   }
                }
             }

             if (SlotIndex < 4)
             {
                SlotMask[SlotIndex] = TRUE;
                SlotLBA[SlotCount] =
                    SystemDisk[disk]->PartitionTable[SlotIndex].StartLBA;
                SlotLength[SlotCount] =
                    SystemDisk[disk]->PartitionTable[SlotIndex].nSectorsTotal;
                SlotCount++;
             }
	  }

          // now determine how many contiguous free data areas are present
          // and what their sizes are in sectors.  we align the starting
          // LBA and size to cylinder boundries.

	  LBAStart = SystemDisk[disk]->SectorsPerTrack;
	  for (FreeCount = i = 0; i < SlotCount; i++)
	  {
	     if (LBAStart < SlotLBA[i])
	     {
#if (TRUE_NETWARE_MODE)
     	        // check if we are currently pointing to the
		// first track on this device.  If so, then
		// we begin our first defined partition as SPT.
		if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
		{
		   LBAWork = LBAStart;
		   FreeLBA[FreeCount] = LBAWork;
		}
		else
		{
	           // round up to the next cylinder boundry
		   LBAWork = (((LBAStart +
		    ((SystemDisk[disk]->SectorsPerTrack *
		      SystemDisk[disk]->TracksPerCylinder) - 1))
		    / (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder))
		    * (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder));
		   FreeLBA[FreeCount] = LBAWork;
		}
#else
	        // round up to the next cylinder boundry
		LBAWork = (((LBAStart +
		    ((SystemDisk[disk]->SectorsPerTrack *
		      SystemDisk[disk]->TracksPerCylinder) - 1))
		    / (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder))
		    * (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder));
		FreeLBA[FreeCount] = LBAWork;
#endif
		// adjust length to correspond to cylinder boundries
		LBAEnd = (((SlotLBA[i] +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		// if we rounded into the next partition, then adjust
		// the ending LBA to the beginning of the next partition.
		if (LBAEnd > SlotLBA[i])
		   LBAEnd = SlotLBA[i];

		// if we rounded off the end of the device, then adjust
		// the ending LBA to the total sector count for the device.
		if (LBAEnd > TotalSectors)
		   LBAEnd = TotalSectors;

		Length = LBAEnd - LBAWork;
		FreeLength[FreeCount] = Length;

		FreeCount++;
	     }
	     LBAStart = (SlotLBA[i] + SlotLength[i]);
	  }

	  // determine how much free space exists less that claimed
	  // by the partition table after we finish scanning.  this
	  // case is the fall through case when the partition table
	  // is empty.

	  if (LBAStart < TotalSectors)
	  {
#if (TRUE_NETWARE_MODE)
     	     // check if we are currently pointing to the
	     // first track on this device.  If so, then
	     // we begin our first defined partition as SPT.
	     if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
	     {
		LBAWork = LBAStart;
		FreeLBA[FreeCount] = LBAWork;
	     }
	     else
	     {
		// round up to the next cylinder boundry
		LBAWork = (((LBAStart +
		    ((SystemDisk[disk]->SectorsPerTrack *
		      SystemDisk[disk]->TracksPerCylinder) - 1))
		    / (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder))
		    * (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder));
		FreeLBA[FreeCount] = LBAWork;
	     }
#else
	     // round up to the next cylinder boundry
	     LBAWork = (((LBAStart +
		    ((SystemDisk[disk]->SectorsPerTrack *
		      SystemDisk[disk]->TracksPerCylinder) - 1))
		    / (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder))
		    * (SystemDisk[disk]->SectorsPerTrack *
		       SystemDisk[disk]->TracksPerCylinder));
	     FreeLBA[FreeCount] = LBAWork;
#endif

	     // adjust length to correspond to cylinder boundries
	     LBAEnd = (((TotalSectors +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

	     // if we rounded off the end of the device, then adjust
	     // the ending LBA to the total sector cout for the device.
	     if (LBAEnd > TotalSectors)
		LBAEnd = TotalSectors;

	     Length = LBAEnd - LBAWork;
	     FreeLength[FreeCount] = Length;

	     FreeCount++;
	  }
	  else
	  if (LBAStart > TotalSectors)
	  {
	     // if LBAStart is greater than the TotalSectors for this
	     // device, then we have an error in the disk geometry
	     // dimensions and they are invalid.

             goto ScanNextDisk;
          }

#if (VERBOSE)
	  NWFSPrint("disk-%d slot_count-%d free_count-%d slot_index-%d\n",
                   (int)disk, (int)SlotCount, (int)FreeCount,
                   (int)SlotIndex);

          for (i=0; i < SlotCount; i++)
          {
             NWFSPrint("part-#%d LBA-%08X Length-%08X Mask-%d\n",
                       (int)i,
                       (unsigned int)SlotLBA[i],
                       (unsigned int)SlotLength[i],
		       (int)SlotMask[i]);
          }

          for (i=0; i < FreeCount; i++)
          {
             NWFSPrint("free-#%d LBA-%08X Length-%08X\n",
                       (int)i,
                       (unsigned int)FreeLBA[i],
		       (unsigned int)FreeLength[i]);
          }
#endif

          // drive geometry range error or partition table data error,
          // abort the operation

	  if (AdjustedSectors < AllocatedSectors)
	  {
             NWFSPrint("nwfs:  drive geometry range error\n");
             goto ScanNextDisk;
	  }

          // if there are no free sectors, then abort the operation
	  FreeSectors = (AdjustedSectors - AllocatedSectors);
	  if (!FreeSectors)
             goto ScanNextDisk;

          // if there were no free slots, then abort the operation
	  if (!freeSlots)
	     goto ScanNextDisk;

	  // make certain we have at least 10 MB of free space
	  // available or abort the operation.  don't allow the
          // creation of a Netware partition that is smaller than
          // 10 MB.

	  if (FreeSectors < 20480)
             goto ScanNextDisk;

          // now allocate all available free space on this device
          // to Netware.  if there are multiple contiguous
	  // free areas on the device, and free slots in
          // the partition table, then continue to allocate
          // Netware partitions until we have exhausted the
          // space on this device, or we have run out of
          // free slots in the partition table.

          for (i=0; (i < FreeCount) && freeSlots; i++)
          {
	     // get the first free slot available
             j = FreeSlotIndex[0];

	     SetPartitionTableValues(
                      &SystemDisk[disk]->PartitionTable[j],
		      NETWARE_386_ID,
		      FreeLBA[i],
		      (FreeLBA[i] + FreeLength[i]) - 1,
		      0,
		      SystemDisk[disk]->TracksPerCylinder,
		      SystemDisk[disk]->SectorsPerTrack);

	     // validate partition extants
	     if (ValidatePartitionExtants(disk))
	     {
                NWFSPrint("partition table entry (%d) is invalid\n", (int)j);
                goto ScanNextDisk;
	     }
	     SystemDisk[disk]->PartitionFlags[j] = TRUE;
	     NWFSPrint("Netware Partition disk-%d:(%d) LBA-%08X size-%08X (%5dMB) Created\n",
		      (int)disk, (int)j,
                      (unsigned int)SystemDisk[disk]->PartitionTable[j].StartLBA,
		      (unsigned int)SystemDisk[disk]->PartitionTable[j].nSectorsTotal,
                      (unsigned int)
                          (((LONGLONG)SystemDisk[disk]->PartitionTable[j].nSectorsTotal
                          * (LONGLONG)SystemDisk[disk]->BytesPerSector)
                          / (LONGLONG)0x100000));
          }

	  NWFSPrint("Updating BOOT Sector disk-(%d)\n", (int)disk);
	  retCode = ReadDiskSectors(disk, 0, Buffer, 1, 1);
	  if (!retCode)
	  {
	     NWFSPrint("FATAL:  disk-%d error reading boot sector\n", (int)disk);
             goto ScanNextDisk;
	  }
	  NWFSCopy(&SystemDisk[disk]->partitionSignature, &Buffer[0x01FE], 2);

	  // if there is no valid partition signature, then
	  // write a new master boot record

	  if (SystemDisk[disk]->partitionSignature != 0xAA55)
	  {
	     NWFSSet(Buffer, 0, 512);
	     NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
	     SystemDisk[disk]->partitionSignature = 0xAA55;
	     NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
	  }

	  // copy the partition table and partition signature
	  NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);

	  // write the partition table to sector 0
	  retCode = WriteDiskSectors(disk, 0, Buffer, 1, 1);
	  if (!retCode)
	  {
	     NWFSPrint("FATAL:  disk-%d error writing boot sector\n", (int)disk);
             goto ScanNextDisk;
	  }

	  // if any of the partitions are being newly created, then
	  // read in their partition boot sectors and write out a valid
	  // Netware partition stamp.  We create partitions in
	  // Netware 5.x format.

	  for (i=0; i < 4; i++)
	  {
	     if ((SystemDisk[disk]->PartitionFlags[i]) &&
                 (SystemDisk[disk]->PartitionTable[i].SysFlag == 0x65) &&
                 (SystemDisk[disk]->PartitionTable[i].nSectorsTotal) &&
                 (SystemDisk[disk]->PartitionTable[i].StartLBA))
	     {
		NWFSPrint("Formating Netware Partition disk-%d LBA-%08X size-%08X (%5dMB)\n",
                          (int)disk,
                          (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
                          (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
                          (unsigned int)
                               (((LONGLONG)SystemDisk[disk]->PartitionTable[j].nSectorsTotal
                               * (LONGLONG)SystemDisk[disk]->BytesPerSector)
                               / (LONGLONG)0x100000));

		retCode = ReadDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA,
			    Buffer, 1, 1);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d error reading part sector\n", (int)disk);
                   goto ScanNextDisk;
		}

		NWFSSet(Buffer, 0, IO_BLOCK_SIZE);
		ps = (PART_SIG *) Buffer;

                // create partition sector 0
		NWFSCopy(ps->NetwareSignature, NwPartSignature, 16);
		ps->PartitionType = TYPE_SIGNATURE;
		ps->PartitionSize = SystemDisk[disk]->PartitionTable[i].nSectorsTotal;
		ps->CreationDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());

		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA,
		      Buffer, 1, 1);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
                   goto ScanNextDisk;
		}

		// write a netware boot sector to sector 1 on this
		// partition along with a complete copy of the
		// partition table for this drive.

		NWFSSet(Buffer, 0, 512);

                // create partition sector 1
		NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
		SystemDisk[disk]->partitionSignature = 0xAA55;
		NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
		NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);

		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 1,
		      Buffer, 1, 1);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
                   goto ScanNextDisk;
		}

                // zero out the partition hotfix and mirror tables

                // hotfix 0
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x20,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
		   goto ScanNextDisk;
		}

                // mirror 0
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x21,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
		   goto ScanNextDisk;
		}

                // hotfix 1
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x40,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
                   goto ScanNextDisk;
		}

                // mirror 1
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x41,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
                   goto ScanNextDisk;
		}

                // hotfix 2
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x60,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
                   goto ScanNextDisk;
		}

                // mirror 2
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x61,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
		   goto ScanNextDisk;
		}

                // hotfix 3
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x80,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
		   goto ScanNextDisk;
		}

                // mirror 3
		retCode = WriteDiskSectors(disk,
		      SystemDisk[disk]->PartitionTable[i].StartLBA + 0x81,
		      ZeroBuffer,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
		      IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		if (!retCode)
		{
		   NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
                   goto ScanNextDisk;
		}
	     }
	     SystemDisk[disk]->PartitionFlags[i] = 0;
	  }

	  UtilScanVolumes();
	  payload.payload_object_count = 0;
	  payload.payload_index = 0;
	  payload.payload_object_size_buffer_size = (sizeof(nwvp_rpartition_scan_info) << 20) + sizeof(rlist);
	  payload.payload_buffer = (BYTE *) &rlist[0];
	  do
	  {
	     nwvp_rpartition_scan(&payload);
	     for (j=0; j < payload.payload_object_count; j++)
	     {
		if ((rlist[j].lpart_handle == 0xFFFFFFFF) && ((rlist[j].partition_type == 0x65) || (rlist[j].partition_type ==0x77)))
		{
		   logical_capacity = (rlist[j].physical_block_count & 0xFFFFFF00) - 242;
		   raw_ids[0] = rlist[j].rpart_handle;
		   raw_ids[1] = 0xFFFFFFFF;
		   raw_ids[2] = 0xFFFFFFFF;
		   raw_ids[3] = 0xFFFFFFFF;
		   nwvp_lpartition_format(&vhandle, logical_capacity, &raw_ids[0]);
		}
	     }
	  } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
       }
       SyncDevice((ULONG)SystemDisk[disk]->PhysicalDiskHandle);

ScanNextDisk:;
    }
    NWFSFree(Buffer);
    UtilScanVolumes();
    WaitForKey();
    return 0;

}

void NWEditPartitions(void)
{
    ULONG CurrentDisk = 0;
    ULONG selected, disk, i, r, j, vhandle, raw_ids[4];
    ULONG TotalSectors, FreeSectors, AdjustedSectors;
    ULONG AllocatedSectors;
    ULONG LBAStart, LBAEnd, Length, freeSlots, targetSize;
    BYTE *Buffer;
    ULONG retCode;
    nwvp_rpartition_scan_info rlist[5];
    nwvp_payload payload;
    ULONG logical_capacity;
    BYTE inputBuffer[20] = { "" };
    BYTE workBuffer[20] = { "" };
    register PART_SIG *ps;
    ULONG SlotCount;
    ULONG SlotIndex;
    ULONG SlotMask[4];
    ULONG SlotLBA[4];
    ULONG SlotLength[4];
    ULONG FreeCount;
    ULONG FreeLBA[4];
    ULONG FreeLength[4];
    ULONG FreeSlotIndex[4];
    ULONG LBAWork;
    ULONG MaxLBA;
    ULONG MaxLength;
    ULONG MaxSlot;

    if (FirstValidDisk == (ULONG)-1)
    {
       NWFSPrint("\nNo fixed disks could be located.  press <Return> ...\n");
       WaitForKey();
       return;
    }
    CurrentDisk = FirstValidDisk;
    
    Buffer = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Buffer)
    {
       NWFSPrint("could not allocate partition workspace\n");
       WaitForKey();
       return;
    }
    NWFSSet(Buffer, 0, IO_BLOCK_SIZE);

    UtilScanVolumes();

    // init the partition marker field for this device
    disk = CurrentDisk;
    if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
    {
       for (i=0; i < 4; i++)
          SystemDisk[disk]->PartitionFlags[i] = 0;
    }

    while (TRUE)
    {
       ClearScreen(consoleHandle);
       NWFSPrint("NetWare Partition Manager\n");
       NWFSPrint("Copyright (c) 1999, 2000 Jeff V. Merkey  All Rights Reserved.\n");

       disk = CurrentDisk;
       if (ValidatePartitionExtants(disk))
       {
	  NWFSPrint("\ndisk-(%d) partition table is invalid\n", (int)disk);
       }

       if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
       {
#if (LINUX_UTIL)
          extern BYTE *PhysicalDiskNames[];

          NWFSPrint("\n **** Disk #%d %s (%02X:%02X) Sectors 0x%X ****\n",
		 (int)disk,
                 PhysicalDiskNames[disk],
		 (unsigned)((ULONG)SystemDisk[disk]->PhysicalDiskHandle & 0xFF),
		 (unsigned)((ULONG)SystemDisk[disk]->PhysicalDiskHandle >> 8),
		 (unsigned int)((SystemDisk[disk]->Cylinders) *
		                 SystemDisk[disk]->TracksPerCylinder *
		                 SystemDisk[disk]->SectorsPerTrack));
#else
	  NWFSPrint("\n **** Disk #%d  Sectors 0x%X ****\n",
		    (int)disk,
		    (unsigned int)((SystemDisk[disk]->Cylinders) *
		                    SystemDisk[disk]->TracksPerCylinder *
		                    SystemDisk[disk]->SectorsPerTrack));
#endif
	  
	  NWFSPrint("Cylinders       : %12u  ", (unsigned int)SystemDisk[disk]->Cylinders);
	  NWFSPrint("Tracks/Cylinder : %12u\n", (unsigned int)SystemDisk[disk]->TracksPerCylinder);
	  NWFSPrint("SectorsPerTrack : %12u  ", (unsigned int)SystemDisk[disk]->SectorsPerTrack);
	  NWFSPrint("BytesPerSector  : %12u\n", (unsigned int)SystemDisk[disk]->BytesPerSector);
	  NWFSPrint("DriveSize       : %12.0f\n", (double)SystemDisk[disk]->driveSize);
	  NWFSPrint("\n");

          if (!ValidatePartitionExtants(disk))
          {
	     for (r=0; r < 4; r++)
	     {
	        if ((SystemDisk[disk]->PartitionTable[r].SysFlag) &&
		    (SystemDisk[disk]->PartitionTable[r].nSectorsTotal))
	        {
		   if (SystemDisk[disk]->PartitionTable[r].SysFlag == NETWARE_386_ID)
		      NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s %s\n",
			    (int)(r + 1),
			    SystemDisk[disk]->PartitionTable[r].fBootable ? "A" : " ",
			    (unsigned int)SystemDisk[disk]->PartitionTable[r].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[r].nSectorsTotal,
			    (unsigned int)
			      ((((LONGLONG)SystemDisk[disk]->PartitionTable[r].nSectorsTotal *
				 (LONGLONG)SystemDisk[disk]->BytesPerSector)) /
				 (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[r].SysFlag),
			    (SystemDisk[disk]->PartitionVersion[r]
			     ? "3.x"
			     : "4.x/5.x"));
		   else
		      NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s\n",
			    (int)(r + 1),
			    SystemDisk[disk]->PartitionTable[r].fBootable ? "A" : " ",
			    (unsigned int)SystemDisk[disk]->PartitionTable[r].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[r].nSectorsTotal,
			    (unsigned int)
			      ((((LONGLONG)SystemDisk[disk]->PartitionTable[r].nSectorsTotal *
				 (LONGLONG)SystemDisk[disk]->BytesPerSector)) /
				 (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[r].SysFlag));
	        }
	        else
		   NWFSPrint("#%d  ------------- Unused ------------- \n", (int)(r + 1));
	     }
          }
       }

       NWFSPrint("\n");
       NWFSPrint("               1. Apply Partition Table Changes\n");
       NWFSPrint("               2. Select Fixed Disk\n");
       NWFSPrint("               3. Create Netware 3.x Partition\n");
       NWFSPrint("               4. Create Netware 4.x/5.x Partition\n");
       NWFSPrint("               5. Set Active Partition\n");
       NWFSPrint("               6. Delete Partition\n");
       NWFSPrint("               7. Initialize Partition Table\n");
       NWFSPrint("               8. Assign All Free Space to NetWare\n");
       NWFSPrint("               9. Return\n");
       NWFSPrint("        Enter a Menu Option: ");

       fgets(inputBuffer, sizeof(inputBuffer), stdin);
       switch (inputBuffer[0])
       {
	  case '1':
	     ClearScreen(consoleHandle);
	     disk = CurrentDisk;
	     if ((!SystemDisk[disk]) || (!SystemDisk[disk]->PhysicalDiskHandle))
                break;

	     NWFSPrint("WARNING !!! this operation will overwrite the partition table on \nthis drive.\n");
	     NWFSPrint("\n **** Disk #%d  Total Sectors 0x%X ****\n",
		       (int)disk,
		       (unsigned int)((SystemDisk[disk]->Cylinders) *
		       SystemDisk[disk]->TracksPerCylinder *
		       SystemDisk[disk]->SectorsPerTrack));

	     for (r=0; r < 4; r++)
	     {
		if ((SystemDisk[disk]->PartitionTable[r].SysFlag) &&
		    (SystemDisk[disk]->PartitionTable[r].nSectorsTotal))
		{
		   if (SystemDisk[disk]->PartitionTable[r].SysFlag == NETWARE_386_ID)
		      NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s %s\n",
			    (int)(r + 1),
			    SystemDisk[disk]->PartitionTable[r].fBootable ? "A" : " ",
			    (unsigned int)SystemDisk[disk]->PartitionTable[r].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[r].nSectorsTotal,
			    (unsigned int)
			      ((((LONGLONG)SystemDisk[disk]->PartitionTable[r].nSectorsTotal *
				 (LONGLONG)SystemDisk[disk]->BytesPerSector)) /
				 (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[r].SysFlag),
			    (SystemDisk[disk]->PartitionVersion[r]
			     ? "3.x"
			     : "4.x/5.x"));
		   else
		      NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s\n",
			    (int)(r + 1),
			    SystemDisk[disk]->PartitionTable[r].fBootable ? "A" : " ",
			    (unsigned int)SystemDisk[disk]->PartitionTable[r].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[r].nSectorsTotal,
			    (unsigned int)
			      ((((LONGLONG)SystemDisk[disk]->PartitionTable[r].nSectorsTotal *
				 (LONGLONG)SystemDisk[disk]->BytesPerSector)) /
				 (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[r].SysFlag));
		}
		else
		   NWFSPrint("#%d  ------------- Unused ------------- \n", (int)(r + 1));
	     }
	     NWFSPrint("\nContinue and write this partition table ? [Y/N]: ");
	     fgets(workBuffer, sizeof(workBuffer), stdin);
	     if (toupper(workBuffer[0]) != 'Y')
		break;

	     NWFSPrint("Updating BOOT Sector disk-(%d)\n", (int)disk);
	     retCode = ReadDiskSectors(disk, 0, Buffer, 1, 1);
	     if (!retCode)
	     {
	        NWFSPrint("FATAL:  disk-%d error reading boot sector\n", (int)disk);
                break;
	     }
	     NWFSCopy(&SystemDisk[disk]->partitionSignature, &Buffer[0x01FE], 2);

	     // if there is no valid partition signature, then
	     // write a new master boot record

	     if (SystemDisk[disk]->partitionSignature != 0xAA55)
	     {
	        NWFSSet(Buffer, 0, 512);
	        NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
	        SystemDisk[disk]->partitionSignature = 0xAA55;
	        NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
	     }

	     // copy the partition table and partition signature
	     NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);

	     // write the partition table to sector 0
	     retCode = WriteDiskSectors(disk, 0, Buffer, 1, 1);
	     if (!retCode)
	     {
	        NWFSPrint("FATAL:  disk-%d error writing boot sector\n", (int)disk);
		break;
	     }

	     // if any of the partitions are being newly created, then
	     // read in their partition boot sectors and write out a valid
	     // Netware partition stamp.  We create partitions in
	     // Netware 3.x and 4.x/5.x formats.

	     for (i=0; i < 4; i++)
	     {
	        if ((SystemDisk[disk]->PartitionFlags[i]) &&
                    (SystemDisk[disk]->PartitionTable[i].SysFlag == 0x65) &&
		    (SystemDisk[disk]->PartitionTable[i].nSectorsTotal) &&
		    (SystemDisk[disk]->PartitionTable[i].StartLBA))
	        {
		   if (SystemDisk[disk]->PartitionVersion[i] == NW4X_PARTITION)
		   {
		      NWFSPrint("Formating Netware 4.x/5.x Partition (%d)-LBA-%08X size-%08X (%5dMB)\n",
			     (int)disk,
			     (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			     (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			     (unsigned int)
			      (((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal
			      * (LONGLONG)SystemDisk[disk]->BytesPerSector)
			      / (LONGLONG)0x100000));

		      retCode = ReadDiskSectors(disk,
			       SystemDisk[disk]->PartitionTable[i].StartLBA,
			       Buffer, 1, 1);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d error reading part sector\n", (int)disk);
			 break;
		      }

		      NWFSSet(Buffer, 0, IO_BLOCK_SIZE);
		      ps = (PART_SIG *) Buffer;

		      // create partition sector 0
		      NWFSCopy(ps->NetwareSignature, NwPartSignature, 16);
		      ps->PartitionType = TYPE_SIGNATURE;
		      ps->PartitionSize = SystemDisk[disk]->PartitionTable[i].nSectorsTotal;
		      ps->CreationDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());

		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA,
			    Buffer, 1, 1);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // write a Netware boot sector to sector 1 on this
		      // partition along with a complete copy of the
		      // partition table for this drive.

		      NWFSSet(Buffer, 0, 512);

		      // create partition sector 1
		      NWFSCopy(Buffer, NetwareBootSector, sizeof(NetwareBootSector));
		      SystemDisk[disk]->partitionSignature = 0xAA55;
		      NWFSCopy(&Buffer[0x01FE], &SystemDisk[disk]->partitionSignature, 2);
		      NWFSCopy(&Buffer[0x01BE], &SystemDisk[disk]->PartitionTable[0].fBootable, 64);

		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 1,
			    Buffer, 1, 1);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // zero out the partition hotfix and mirror tables

		      // hotfix 0
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x20,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // mirror 0
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x21,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // hotfix 1
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x40,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // mirror 1
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x41,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // hotfix 2
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x60,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // mirror 2
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x61,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // hotfix 3
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x80,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // mirror 3
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x81,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }
		   }
		   else
		   if (SystemDisk[disk]->PartitionVersion[i] == NW3X_PARTITION)
		   {
		      NWFSPrint("Formating Netware 3.x Partition (%d)-LBA-%08X size-%08X (%5dMB)\n",
			     (int)disk,
			     (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			     (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			     (unsigned int)
			      (((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal
			      * (LONGLONG)SystemDisk[disk]->BytesPerSector)
			      / (LONGLONG)0x100000));

		      retCode = ReadDiskSectors(disk,
			       SystemDisk[disk]->PartitionTable[i].StartLBA,
			       Buffer, 1, 1);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d error reading part sector\n", (int)disk);
			 break;
		      }

		      // zero 3.x partition sector 0
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA,
			    ZeroBuffer, 1, 1);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // zero 3.x partition sector 1
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 1,
			    ZeroBuffer, 1, 1);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // zero out the partition hotfix and mirror tables

		      // hotfix 0
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x20,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // mirror 0
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x21,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // hotfix 1
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x40,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // mirror 1
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x41,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // hotfix 2
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x60,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // mirror 2
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x61,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // hotfix 3
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x80,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }

		      // mirror 3
		      retCode = WriteDiskSectors(disk,
			    SystemDisk[disk]->PartitionTable[i].StartLBA + 0x81,
			    ZeroBuffer,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector,
			    IO_BLOCK_SIZE / SystemDisk[disk]->BytesPerSector);
		      if (!retCode)
		      {
			 NWFSPrint("FATAL:  disk-%d hotfix/mirror init error\n", (int)disk);
			 break;
		      }
		   }
		   else
		      NWFSPrint("FATAL:  disk-%d could not determine partition version\n", (int)disk);
		}
		SystemDisk[disk]->PartitionFlags[i] = 0;
	     }

	     UtilScanVolumes();
	     payload.payload_object_count = 0;
	     payload.payload_index = 0;
	     payload.payload_object_size_buffer_size = (sizeof(nwvp_rpartition_scan_info) << 20) + sizeof(rlist);
	     payload.payload_buffer = (BYTE *) &rlist[0];
	     do
	     {
		nwvp_rpartition_scan(&payload);
		for (j=0; j< payload.payload_object_count; j++)
		{
		   if ((rlist[j].lpart_handle == 0xFFFFFFFF) && ((rlist[j].partition_type == 0x65) || (rlist[j].partition_type == 0x77)))
		   {
		      logical_capacity = (rlist[j].physical_block_count & 0xFFFFFF00) - 242;
		      raw_ids[0] = rlist[j].rpart_handle;
		      raw_ids[1] = 0xFFFFFFFF;
		      raw_ids[2] = 0xFFFFFFFF;
		      raw_ids[3] = 0xFFFFFFFF;
		      nwvp_lpartition_format(&vhandle, logical_capacity, &raw_ids[0]);
		   }
		}
	     } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
	     SyncDevice((ULONG)SystemDisk[disk]->PhysicalDiskHandle);

	     WaitForKey();
	     break;

	  case '2':
	     ClearScreen(consoleHandle);
	     NWFSPrint("Current Disk is Setting is (%d) [", (int)CurrentDisk);
	     for (i=0; i < MAX_DISKS; i++)
	     {
                extern int get_number_of_partitions(int);

		if ((SystemDisk[i]) && (SystemDisk[i]->PhysicalDiskHandle))
		{
		   if (!i)
		      NWFSPrint("%d-%d", (int)i, get_number_of_partitions(i));
		   else
		      NWFSPrint(", %d-%d", (int)i, get_number_of_partitions(i));
		}
	     }
	     NWFSPrint("]\n\n");

	     NWFSPrint("Enter New Disk Number:  ");

	     fgets(workBuffer, sizeof(workBuffer), stdin);
	     disk = atoi(workBuffer);
	     if (disk < MAX_DISKS && SystemDisk[disk] &&
		SystemDisk[disk]->PhysicalDiskHandle)
	     {
		NWFSPrint("\nCurrentDisk(%u) Set to (%u)\n",
			 (unsigned int)CurrentDisk, (unsigned int)disk);

                // init the partition marker field for this device
                CurrentDisk = disk;
                for (i=0; i < 4; i++)
                   SystemDisk[disk]->PartitionFlags[i] = 0;

                WaitForKey();
	     }
	     else
	     {
		NWFSPrint("\nInvalid Disk (%u) Specified\n", (unsigned int)disk);
		WaitForKey();
	     }
	     break;

	  case '3':
	     ClearScreen(consoleHandle);
	     disk = CurrentDisk;
	     if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
	     {
		TotalSectors = (ULONG)((SystemDisk[disk]->Cylinders) *
			               SystemDisk[disk]->TracksPerCylinder *
			               SystemDisk[disk]->SectorsPerTrack);

                if (TotalSectors < SystemDisk[disk]->SectorsPerTrack)
	        {
                   NWFSPrint("nwfs:  sector total range error\n");
		   WaitForKey();
		   break;
	        }

		NWFSPrint("\n*** Disk Device (%d)  Sectors %X ***\n",
			   (int)disk,(unsigned int)TotalSectors);

                // assume the first cylinder of all hard drives is reserved.
	        AdjustedSectors = (TotalSectors - SystemDisk[disk]->SectorsPerTrack);

		// scan for free slots and count the total free sectors
                // for this drive.

	        for (freeSlots = AllocatedSectors = 0, i = 0; i < 4; i++)
	        {
                   SlotMask[i] = 0;
                   SlotLBA[i] = 0;
                   SlotLength[i] = 0;
		   FreeLBA[i] = 0;
		   FreeLength[i] = 0;

		   if (SystemDisk[disk]->PartitionTable[i].SysFlag &&
		       SystemDisk[disk]->PartitionTable[i].nSectorsTotal &&
		       SystemDisk[disk]->PartitionTable[i].StartLBA)
		   {
		      AllocatedSectors +=
			   SystemDisk[disk]->PartitionTable[i].nSectorsTotal;

		      if (SystemDisk[disk]->PartitionTable[i].SysFlag == NETWARE_386_ID)
			 NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s %s\n",
			       (int) i + 1,
			       SystemDisk[disk]->PartitionTable[i].fBootable
			       ? "A" : " ",
			       (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			       (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			       (unsigned int)
				   (((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal *
				     (LONGLONG)SystemDisk[disk]->BytesPerSector) /
				     (LONGLONG)0x100000),
			       get_partition_type(SystemDisk[disk]->PartitionTable[i].SysFlag),
			       (SystemDisk[disk]->PartitionVersion[i]
			       ? "3.x" : "4.x/5.x"));
		      else
			 NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s\n",
			       (int) i + 1,
			       SystemDisk[disk]->PartitionTable[i].fBootable
			       ? "A" : " ",
			       (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			       (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			       (unsigned int)
				   (((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal *
				     (LONGLONG)SystemDisk[disk]->BytesPerSector) /
				     (LONGLONG)0x100000),
			       get_partition_type(SystemDisk[disk]->PartitionTable[i].SysFlag));
		   }
	           else
		   {
		      FreeSlotIndex[freeSlots] = i;
		      freeSlots++;
		      NWFSPrint("#%d  ------------- Unused ------------- \n", (int) i + 1);
		   }
	        }

                // now sort the partition table into ascending order
		for (SlotCount = i = 0; i < 4; i++)
                {
                   // look for the lowest starting lba
                   for (SlotIndex = LBAStart = (ULONG) -1, j = 0; j < 4; j++)
                   {
	              if (SystemDisk[disk]->PartitionTable[j].SysFlag &&
                          !SlotMask[j])
		      {
                         if (SystemDisk[disk]->PartitionTable[j].StartLBA <
                             LBAStart)
                         {
                            LBAStart = SystemDisk[disk]->PartitionTable[j].StartLBA;
                            SlotIndex = j;
                         }
                      }
                   }

                   if (SlotIndex < 4)
                   {
                      SlotMask[SlotIndex] = TRUE;
		      SlotLBA[SlotCount] =
                         SystemDisk[disk]->PartitionTable[SlotIndex].StartLBA;
                      SlotLength[SlotCount] =
                         SystemDisk[disk]->PartitionTable[SlotIndex].nSectorsTotal;
                      SlotCount++;
                   }
                }

                // now determine how many contiguous free data areas are
                // present and what their sizes are in sectors.  we align
                // the starting LBA and size to cylinder boundries.

		LBAStart = SystemDisk[disk]->SectorsPerTrack;
		for (FreeCount = i = 0; i < SlotCount; i++)
		{
		   if (LBAStart < SlotLBA[i])
		   {
		      // check if we are currently pointing to the
		      // first track on this device.  If so, then
		      // we begin our first defined partition on the
		      // first track of the device.

		      if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
		      {
			 LBAWork = LBAStart;
			 FreeLBA[FreeCount] = LBAWork;
		      }
		      else
		      {
			 // round up to the next cylinder boundry
			 LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
			 FreeLBA[FreeCount] = LBAWork;
		      }

		      // adjust length to correspond to cylinder boundries
		      LBAEnd = (((SlotLBA[i] +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		      // if we rounded into the next partition, then adjust
		      // the ending LBA to the beginning of the next partition.
		      if (LBAEnd > SlotLBA[i])
			 LBAEnd = SlotLBA[i];

		      // if we rounded off the end of the device, then adjust
		      // the ending LBA to the total sector cout for the device.
		      if (LBAEnd > TotalSectors)
			 LBAEnd = TotalSectors;

		      Length = LBAEnd - LBAWork;
		      FreeLength[FreeCount] = Length;

		      FreeCount++;
                   }
                   LBAStart = (SlotLBA[i] + SlotLength[i]);
                }

                // determine how much free space exists less that claimed
                // by the partition table after we finish scanning.  this
                // case is the fall through case when the partition table
                // is empty.

		if (LBAStart < TotalSectors)
		{
#if (TRUE_NETWARE_MODE)
		   // check if we are currently pointing to the
		   // first track on this device.  If so, then
		   // we begin our first defined partition as SPT.
		   if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
		   {
		      LBAWork = LBAStart;
		      FreeLBA[FreeCount] = LBAWork;
		   }
		   else
		   {
		      // round up to the next cylinder boundry
		      LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
		      FreeLBA[FreeCount] = LBAWork;
		   }
#else
		   // round up to the next cylinder boundry
		   LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
		   FreeLBA[FreeCount] = LBAWork;
#endif

		   // adjust length to correspond to cylinder boundries
		   LBAEnd = (((TotalSectors +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		   // if we rounded off the end of the device, then adjust
		   // the ending LBA to the total sector cout for the device.
		   if (LBAEnd > TotalSectors)
		      LBAEnd = TotalSectors;

		   Length = LBAEnd - LBAWork;
		   FreeLength[FreeCount] = Length;

		   FreeCount++;
		}
		else
		if (LBAStart > TotalSectors)
		{
		   // if LBAStart is greater than the TotalSectors for this
		   // device, then we have an error in the disk geometry
		   // dimensions and they are invalid.

		   WaitForKey();
		   break;
		}


#if (VERBOSE)
                NWFSPrint("disk-%d slot_count-%d free_count-%d slot_index-%d\n",
                         (int)disk, (int)SlotCount, (int)FreeCount,
                         (int)SlotIndex);

                for (i=0; i < SlotCount; i++)
		{
                   NWFSPrint("part-#%d LBA-%08X Length-%08X Mask-%d\n",
                            (int)i,
                            (unsigned int)SlotLBA[i],
                            (unsigned int)SlotLength[i],
			    (int)SlotMask[i]);
                }

                for (i=0; i < FreeCount; i++)
                {
		   NWFSPrint("free-#%d LBA-%08X Length-%08X\n",
			    (int)i,
                            (unsigned int)FreeLBA[i],
			    (unsigned int)FreeLength[i]);
		}
#endif
		// drive geometry range error or partition table data error,
                // abort the operation

	        if (AdjustedSectors < AllocatedSectors)
		{
                   NWFSPrint("\n ***  drive geometry range error ***\n");
		   WaitForKey();
		   break;
	        }

		// if there are no free sectors, then abort the operation
	        FreeSectors = (AdjustedSectors - AllocatedSectors);

		NWFSPrint("\ndata sectors     : %12.0f bytes (%08X sectors)\n",
			   (double) ((LONGLONG)SystemDisk[disk]->BytesPerSector *
				     (LONGLONG)AdjustedSectors),
			   (unsigned int) AdjustedSectors);

		NWFSPrint("reserved sectors : %12.0f bytes (%08X sectors)\n",
			   (double) ((LONGLONG)SystemDisk[disk]->BytesPerSector *
				     (LONGLONG)SystemDisk[disk]->SectorsPerTrack),
			   (unsigned int) SystemDisk[disk]->SectorsPerTrack);

		NWFSPrint("%12.0f bytes (%X sectors) are currently free\n",
			   (double) ((LONGLONG)SystemDisk[disk]->BytesPerSector *
				     (LONGLONG)FreeSectors),
			   (unsigned int) FreeSectors);

		if (!FreeSectors)
		{
		   NWFSPrint("\n *** No logical space left on device ***\n");
		   WaitForKey();
		   break;
                }

		// if there were no free slots, then abort the operation
	        if (!freeSlots)
		{
		   NWFSPrint("\n *** No partition table entries available on device ***\n");
		   WaitForKey();
		   break;
                }

	        // make certain we have at least 10 MB of free space
		// available or abort the operation.  don't allow the
                // creation of a Netware partition that is smaller than
                // 10 MB.

	        if (FreeSectors < 20480)
		{
		   NWFSPrint("\n *** you must have at least 10 MB of free space ***\n");
		   WaitForKey();
		   break;
                }

		// determine which entry has the single largest area of
                // contiguous setors for this device.

		for (MaxSlot = MaxLBA = MaxLength = i = 0;
                     i < FreeCount; i++)
                {
                   if (FreeLength[i] > MaxLength)
                   {
                      MaxLBA = FreeLBA[i];
		      MaxLength = FreeLength[i];
                   }
                }

		NWFSPrint("\nAllocate all contiguous free space for NetWare 3.x? (%5uMB) [Y/N]: ",
			   (unsigned int)
			     (((LONGLONG)SystemDisk[disk]->BytesPerSector *
                               (LONGLONG)MaxLength) /
                               (LONGLONG)0x100000));
		fgets(workBuffer, sizeof(workBuffer), stdin);
		if (toupper(workBuffer[0]) == 'Y')
		{
                   // get the first free slot available
		   j = FreeSlotIndex[0];

	           SetPartitionTableValues(
                         &SystemDisk[disk]->PartitionTable[j],
		         NETWARE_386_ID,
		         MaxLBA,
		         (MaxLBA + MaxLength) - 1,
			 0,
		         SystemDisk[disk]->TracksPerCylinder,
		         SystemDisk[disk]->SectorsPerTrack);

                   // validate partition extants
		   if (ValidatePartitionExtants(disk))
		   {
                      NWFSPrint("partition table entry (%d) is invalid\n", (int)j);
		      WaitForKey();
                      break;
		   }

	           SystemDisk[disk]->PartitionFlags[j] = TRUE;
		   SystemDisk[disk]->PartitionVersion[j] = NW3X_PARTITION;
		   NWFSPrint("Netware 3.x Partition (%d:%d) LBA-%08X size-%08X (%5dMB) Created\n",
                            (int)disk, (int)j,
                            (unsigned int)SystemDisk[disk]->PartitionTable[j].StartLBA,
                            (unsigned int)SystemDisk[disk]->PartitionTable[j].nSectorsTotal,
                            (unsigned int)
                               (((LONGLONG)SystemDisk[disk]->PartitionTable[j].nSectorsTotal
			       * (LONGLONG)SystemDisk[disk]->BytesPerSector)
                               / (LONGLONG)0x100000));
		   WaitForKey();
		   break;
		}

		while (TRUE)
		{
		   NWFSPrint("\nEnter NetWare 3.x Partition Size in MB (up to %uMB): ",
			       (unsigned int)
				 (((LONGLONG)SystemDisk[disk]->BytesPerSector *
				   (LONGLONG)MaxLength) /
                                   (LONGLONG)0x100000));
		   fgets(workBuffer, sizeof(workBuffer), stdin);
		   targetSize = atol(workBuffer);
                   if (!targetSize)
                      break;

		   if ((targetSize <= (ULONG)
                       (((LONGLONG)SystemDisk[disk]->BytesPerSector *
			 (LONGLONG)MaxLength) /
                         (LONGLONG)0x100000))
                       && (targetSize >= 10))
		      break;
		}

		if (targetSize)
                {
                   // convert bytes to sectors
		   MaxLength = (ULONG)(((LONGLONG)targetSize *
				(LONGLONG)0x100000) /
				(LONGLONG)SystemDisk[disk]->BytesPerSector);

		   // cylinder align the requested size
		   MaxLength = (((MaxLength +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));

                   // get the first free slot available
                   j = FreeSlotIndex[0];

		   SetPartitionTableValues(
			       &SystemDisk[disk]->PartitionTable[j],
			       NETWARE_386_ID,
			       MaxLBA,
			       (MaxLBA + MaxLength) - 1,
			       0,
			       SystemDisk[disk]->TracksPerCylinder,
			       SystemDisk[disk]->SectorsPerTrack);

		   SystemDisk[disk]->PartitionFlags[j] = TRUE;
		   SystemDisk[disk]->PartitionVersion[j] = NW3X_PARTITION;
		   NWFSPrint("Netware 3.x Partition (%d:%d) LBA-%08X size-%08X (%5dMB) Created\n",
                            (int)disk, (int)j,
                            (unsigned int)SystemDisk[disk]->PartitionTable[j].StartLBA,
                            (unsigned int)SystemDisk[disk]->PartitionTable[j].nSectorsTotal,
                            (unsigned int)
                              (((LONGLONG)SystemDisk[disk]->PartitionTable[j].nSectorsTotal
                              * (LONGLONG)SystemDisk[disk]->BytesPerSector)
                              / (LONGLONG)0x100000));
                }
	     }
	     WaitForKey();
	     break;

	  case '4':
	     ClearScreen(consoleHandle);
	     disk = CurrentDisk;
	     if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
	     {
		TotalSectors = (ULONG)((SystemDisk[disk]->Cylinders) *
			               SystemDisk[disk]->TracksPerCylinder *
				       SystemDisk[disk]->SectorsPerTrack);

                if (TotalSectors < SystemDisk[disk]->SectorsPerTrack)
	        {
                   NWFSPrint("nwfs:  sector total range error\n");
		   WaitForKey();
		   break;
	        }

		NWFSPrint("\n*** Disk Device (%d)  Sectors %X ***\n",
			   (int)disk,(unsigned int)TotalSectors);

                // assume the first cylinder of all hard drives is reserved.
	        AdjustedSectors = (TotalSectors - SystemDisk[disk]->SectorsPerTrack);

		// scan for free slots and count the total free sectors
                // for this drive.

	        for (freeSlots = AllocatedSectors = 0, i = 0; i < 4; i++)
	        {
		   SlotMask[i] = 0;
                   SlotLBA[i] = 0;
                   SlotLength[i] = 0;
		   FreeLBA[i] = 0;
                   FreeLength[i] = 0;

	           if (SystemDisk[disk]->PartitionTable[i].SysFlag &&
                       SystemDisk[disk]->PartitionTable[i].nSectorsTotal &&
                       SystemDisk[disk]->PartitionTable[i].StartLBA)
	           {
		      AllocatedSectors +=
			   SystemDisk[disk]->PartitionTable[i].nSectorsTotal;

		      if (SystemDisk[disk]->PartitionTable[i].SysFlag == NETWARE_386_ID)
			 NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s %s\n",
			       (int) i + 1,
			       SystemDisk[disk]->PartitionTable[i].fBootable
			       ? "A" : " ",
			       (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			       (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			       (unsigned int)
				   (((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal *
				     (LONGLONG)SystemDisk[disk]->BytesPerSector) /
				     (LONGLONG)0x100000),
			       get_partition_type(SystemDisk[disk]->PartitionTable[i].SysFlag),
			       (SystemDisk[disk]->PartitionVersion[i]
			       ? "3.x" : "4.x/5.x"));
		      else
			 NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s\n",
			       (int) i + 1,
			       SystemDisk[disk]->PartitionTable[i].fBootable
			       ? "A" : " ",
			       (unsigned int)SystemDisk[disk]->PartitionTable[i].StartLBA,
			       (unsigned int)SystemDisk[disk]->PartitionTable[i].nSectorsTotal,
			       (unsigned int)
				   (((LONGLONG)SystemDisk[disk]->PartitionTable[i].nSectorsTotal *
				     (LONGLONG)SystemDisk[disk]->BytesPerSector) /
				     (LONGLONG)0x100000),
			       get_partition_type(SystemDisk[disk]->PartitionTable[i].SysFlag));
		   }
		   else
		   {
		      FreeSlotIndex[freeSlots] = i;
		      freeSlots++;
		      NWFSPrint("#%d  ------------- Unused ------------- \n", (int) i + 1);
		   }
	        }

                // now sort the partition table into ascending order
                for (SlotCount = i = 0; i < 4; i++)
		{
                   // look for the lowest starting lba
                   for (SlotIndex = LBAStart = (ULONG) -1, j = 0; j < 4; j++)
                   {
		      if (SystemDisk[disk]->PartitionTable[j].SysFlag &&
			  !SlotMask[j])
		      {
			 if (SystemDisk[disk]->PartitionTable[j].StartLBA <
                             LBAStart)
                         {
			    LBAStart = SystemDisk[disk]->PartitionTable[j].StartLBA;
                            SlotIndex = j;
                         }
		      }
                   }

                   if (SlotIndex < 4)
                   {
                      SlotMask[SlotIndex] = TRUE;
                      SlotLBA[SlotCount] =
			 SystemDisk[disk]->PartitionTable[SlotIndex].StartLBA;
                      SlotLength[SlotCount] =
                         SystemDisk[disk]->PartitionTable[SlotIndex].nSectorsTotal;
                      SlotCount++;
		   }
		}

		// now determine how many contiguous free data areas are
                // present and what their sizes are in sectors.  we align
                // the starting LBA and size to cylinder boundries.

	        LBAStart = SystemDisk[disk]->SectorsPerTrack;
                for (FreeCount = i = 0; i < SlotCount; i++)
		{
                   if (LBAStart < SlotLBA[i])
                   {
		      // check if we are currently pointing to the
		      // first track on this device.  If so, then
		      // we begin our first defined partition on
		      // the first track of this device.

		      if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
		      {
			 LBAWork = LBAStart;
			 FreeLBA[FreeCount] = LBAWork;
		      }
		      else
		      {
			 // round up to the next cylinder boundry
			 LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
			 FreeLBA[FreeCount] = LBAWork;
		      }

		      // adjust length to correspond to cylinder boundries
		      LBAEnd = (((SlotLBA[i] +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		      // if we rounded into the next partition, then adjust
		      // the ending LBA to the beginning of the next partition.
		      if (LBAEnd > SlotLBA[i])
			 LBAEnd = SlotLBA[i];

		      // if we rounded off the end of the device, then adjust
		      // the ending LBA to the total sector cout for the device.
		      if (LBAEnd > TotalSectors)
			 LBAEnd = TotalSectors;

		      Length = LBAEnd - LBAWork;
		      FreeLength[FreeCount] = Length;
		      FreeCount++;
		   }
		   LBAStart = (SlotLBA[i] + SlotLength[i]);
		}

		// determine how much free space exists less that claimed
		// by the partition table after we finish scanning.  this
		// case is the fall through case when the partition table
                // is empty.

		if (LBAStart < TotalSectors)
		{
#if (TRUE_NETWARE_MODE)
		   // check if we are currently pointing to the
		   // first track on this device.  If so, then
		   // we begin our first defined partition as SPT.
		   if (LBAStart == SystemDisk[disk]->SectorsPerTrack)
		   {
		      LBAWork = LBAStart;
		      FreeLBA[FreeCount] = LBAWork;
		   }
		   else
		   {
		      // round up to the next cylinder boundry
		      LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
		      FreeLBA[FreeCount] = LBAWork;
		   }
#else
		   // round up to the next cylinder boundry
		   LBAWork = (((LBAStart +
			    ((SystemDisk[disk]->SectorsPerTrack *
			      SystemDisk[disk]->TracksPerCylinder) - 1))
			    / (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder))
			    * (SystemDisk[disk]->SectorsPerTrack *
			       SystemDisk[disk]->TracksPerCylinder));
		   FreeLBA[FreeCount] = LBAWork;
#endif

		   // adjust length to correspond to cylinder boundries
		   LBAEnd = (((TotalSectors +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

		   // if we rounded off the end of the device, then adjust
		   // the ending LBA to the total sector cout for the device.
		   if (LBAEnd > TotalSectors)
		      LBAEnd = TotalSectors;

		   Length = LBAEnd - LBAWork;
		   FreeLength[FreeCount] = Length;
		   FreeCount++;
		}
                else
	        if (LBAStart > TotalSectors)
                {
                   // if LBAStart is greater than the TotalSectors for this
                   // device, then we have an error in the disk geometry
                   // dimensions and they are invalid.

		   WaitForKey();
		   break;
		}


#if (VERBOSE)
		NWFSPrint("disk-%d slot_count-%d free_count-%d slot_index-%d\n",
                         (int)disk, (int)SlotCount, (int)FreeCount,
                         (int)SlotIndex);

		for (i=0; i < SlotCount; i++)
                {
		   NWFSPrint("part-#%d LBA-%08X Length-%08X Mask-%d\n",
                            (int)i,
                            (unsigned int)SlotLBA[i],
                            (unsigned int)SlotLength[i],
                            (int)SlotMask[i]);
                }

		for (i=0; i < FreeCount; i++)
                {
                   NWFSPrint("free-#%d LBA-%08X Length-%08X\n",
			    (int)i,
                            (unsigned int)FreeLBA[i],
                            (unsigned int)FreeLength[i]);
		}
#endif
		// drive geometry range error or partition table data error,
                // abort the operation

		if (AdjustedSectors < AllocatedSectors)
	        {
		   NWFSPrint("\n ***  drive geometry range error ***\n");
		   WaitForKey();
		   break;
	        }

                // if there are no free sectors, then abort the operation
	        FreeSectors = (AdjustedSectors - AllocatedSectors);

		NWFSPrint("\ndata sectors     : %12.0f bytes (%08X sectors)\n",
			   (double) ((LONGLONG)SystemDisk[disk]->BytesPerSector *
				     (LONGLONG)AdjustedSectors),
			   (unsigned int) AdjustedSectors);

		NWFSPrint("reserved sectors : %12.0f bytes (%08X sectors)\n",
			   (double) ((LONGLONG)SystemDisk[disk]->BytesPerSector *
				     (LONGLONG)SystemDisk[disk]->SectorsPerTrack),
			   (unsigned int) SystemDisk[disk]->SectorsPerTrack);

		NWFSPrint("%12.0f bytes (%X sectors) are currently free\n",
			   (double) ((LONGLONG)SystemDisk[disk]->BytesPerSector *
				     (LONGLONG)FreeSectors),
			   (unsigned int) FreeSectors);

		if (!FreeSectors)
                {
		   NWFSPrint("\n *** No logical space left on device ***\n");
		   WaitForKey();
		   break;
                }

		// if there were no free slots, then abort the operation
	        if (!freeSlots)
                {
		   NWFSPrint("\n *** No partition table entries available on device ***\n");
		   WaitForKey();
		   break;
                }

		// make certain we have at least 10 MB of free space
	        // available or abort the operation.  don't allow the
		// creation of a Netware partition that is smaller than
                // 10 MB.

	        if (FreeSectors < 20480)
                {
		   NWFSPrint("\n *** you must have at least 10 MB of free space ***\n");
		   WaitForKey();
		   break;
                }

		// determine which entry has the single largest area of
                // contiguous setors for this device.

		for (MaxSlot = MaxLBA = MaxLength = i = 0;
                     i < FreeCount; i++)
                {
                   if (FreeLength[i] > MaxLength)
                   {
		      MaxLBA = FreeLBA[i];
                      MaxLength = FreeLength[i];
		   }
                }

		NWFSPrint("\nAllocate all contiguous free space for NetWare 4.x/5.x? (%uMB) [Y/N]: ",
			   (unsigned int)
                             (((LONGLONG)SystemDisk[disk]->BytesPerSector *
                               (LONGLONG)MaxLength) /
			       (LONGLONG)0x100000));

		fgets(workBuffer, sizeof(workBuffer), stdin);
		if (toupper(workBuffer[0]) == 'Y')
		{
                   // get the first free slot available
                   j = FreeSlotIndex[0];

	           SetPartitionTableValues(
                         &SystemDisk[disk]->PartitionTable[j],
		         NETWARE_386_ID,
		         MaxLBA,
			 (MaxLBA + MaxLength) - 1,
			 0,
		         SystemDisk[disk]->TracksPerCylinder,
		         SystemDisk[disk]->SectorsPerTrack);

                   // validate partition extants
	           if (ValidatePartitionExtants(disk))
	           {
                      NWFSPrint("partition table entry (%d) is invalid\n", (int)j);
		      WaitForKey();
		      break;
	           }

	           SystemDisk[disk]->PartitionFlags[j] = TRUE;
		   SystemDisk[disk]->PartitionVersion[j] = NW4X_PARTITION;
		   NWFSPrint("Netware 4.x/5.x Partition (%d:%d) LBA-%08X size-%08X (%5dMB) Created\n",
                            (int)disk, (int)j,
                            (unsigned int)SystemDisk[disk]->PartitionTable[j].StartLBA,
                            (unsigned int)SystemDisk[disk]->PartitionTable[j].nSectorsTotal,
                            (unsigned int)
			       (((LONGLONG)SystemDisk[disk]->PartitionTable[j].nSectorsTotal
			       * (LONGLONG)SystemDisk[disk]->BytesPerSector)
                               / (LONGLONG)0x100000));
		   WaitForKey();
		   break;
		}

		while (TRUE)
		{
		   NWFSPrint("\nEnter NetWare 4.x/5.x Partition Size in MB (up to %uMB): ",
			       (unsigned int)
				 (((LONGLONG)SystemDisk[disk]->BytesPerSector *
				   (LONGLONG)MaxLength) /
				   (LONGLONG)0x100000));
		   fgets(workBuffer, sizeof(workBuffer), stdin);
		   targetSize = atol(workBuffer);
                   if (!targetSize)
                      break;

		   if ((targetSize <= (ULONG)
		       (((LONGLONG)SystemDisk[disk]->BytesPerSector *
			 (LONGLONG)MaxLength) /
                         (LONGLONG)0x100000))
                       && (targetSize >= 10))
		      break;
		}

                if (targetSize)
                {
		   // convert bytes to sectors
		   MaxLength = (ULONG)(((LONGLONG)targetSize *
                                (LONGLONG)0x100000) /
				(LONGLONG)SystemDisk[disk]->BytesPerSector);

		   // cylinder align the requested size
		   MaxLength = (((MaxLength +
			   ((SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder) - 1))
			  / (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder))
			  * (SystemDisk[disk]->SectorsPerTrack *
			     SystemDisk[disk]->TracksPerCylinder));

                   // get the first free slot available
                   j = FreeSlotIndex[0];

		   SetPartitionTableValues(
			       &SystemDisk[disk]->PartitionTable[j],
			       NETWARE_386_ID,
			       MaxLBA,
			       (MaxLBA + MaxLength) - 1,
			       0,
			       SystemDisk[disk]->TracksPerCylinder,
			       SystemDisk[disk]->SectorsPerTrack);

		   SystemDisk[disk]->PartitionFlags[j] = TRUE;
		   SystemDisk[disk]->PartitionVersion[j] = NW4X_PARTITION;
		   NWFSPrint("Netware 4.x/5.x Partition (%d:%d) LBA-%08X size-%08X (%5dMB) Created\n",
                            (int)disk, (int)j,
                            (unsigned int)SystemDisk[disk]->PartitionTable[j].StartLBA,
			    (unsigned int)SystemDisk[disk]->PartitionTable[j].nSectorsTotal,
                            (unsigned int)
                              (((LONGLONG)SystemDisk[disk]->PartitionTable[j].nSectorsTotal
                              * (LONGLONG)SystemDisk[disk]->BytesPerSector)
			      / (LONGLONG)0x100000));
                }
	     }
	     WaitForKey();
	     break;

	  case '5':
	     ClearScreen(consoleHandle);
	     disk = CurrentDisk;
	     if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
	     {
		NWFSPrint("\n*** Disk Device (%d)  Sectors %X ***\n", (int) disk,
			 (unsigned int) ((SystemDisk[disk]->Cylinders) *
			    SystemDisk[disk]->TracksPerCylinder *
			    SystemDisk[disk]->SectorsPerTrack));

		for (r=0; r < 4; r++)
		{
		   if (SystemDisk[disk]->PartitionTable[r].nSectorsTotal)
		   {
		      if (SystemDisk[disk]->PartitionTable[r].SysFlag == NETWARE_386_ID)
			 NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s %s\n",
			    (int) (r + 1),
			    (SystemDisk[disk]->PartitionTable[r].fBootable
			    ? "A" : " "),
			    (int) SystemDisk[disk]->PartitionTable[r].StartLBA,
			    (int) SystemDisk[disk]->PartitionTable[r].nSectorsTotal,
			    (unsigned int)
				(((LONGLONG)SystemDisk[disk]->PartitionTable[r].nSectorsTotal *
				  (LONGLONG)SystemDisk[disk]->BytesPerSector) /
				  (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[r].SysFlag),
			    (SystemDisk[disk]->PartitionVersion[r]
			     ? "3.x" : "4.x/5.x"));
		      else
			 NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s\n",
			    (int) (r + 1),
			    (SystemDisk[disk]->PartitionTable[r].fBootable
			    ? "A" : " "),
			    (int) SystemDisk[disk]->PartitionTable[r].StartLBA,
			    (int) SystemDisk[disk]->PartitionTable[r].nSectorsTotal,
			    (unsigned int)
				(((LONGLONG)SystemDisk[disk]->PartitionTable[r].nSectorsTotal *
				  (LONGLONG)SystemDisk[disk]->BytesPerSector) /
				  (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[r].SysFlag));
		   }
		   else
		      NWFSPrint("#%d  ------------- Unused ------------- \n", (int) r + 1);
		}

		NWFSPrint("\n     Enter Partition to Make Active (1-4): ");
		fgets(workBuffer, sizeof(workBuffer), stdin);
		selected = atoi(workBuffer);
		if (selected && selected <= 4)
		{
		   register ULONG cl;

		   for (r = 0; r < 4; r++)
		   {
		      if (SystemDisk[disk]->PartitionTable[r].nSectorsTotal)
		      {
			 if (r + 1 == selected)
			 {
			    for (cl = 0; cl < 4; cl ++)
			    {
			       if (cl + 1 != selected)
				  SystemDisk[disk]->PartitionTable[cl].fBootable = 0;
			    }
			    NWFSPrint("\nPartition (%d) Set Active\n", (int) selected);
			    SystemDisk[disk]->PartitionTable[r].fBootable = 1;
			 }
		      }
		      else
		      if (r + 1 == selected)
			 NWFSPrint("#%d is not a valid partition entry\n", (int) selected);
		   }
		}
		else
		   NWFSPrint("#%d is not a valid partition number\n", (int) selected);
	     }
	     WaitForKey();
	     break;

	  case '6':
	     ClearScreen(consoleHandle);
	     disk = CurrentDisk;
	     if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
	     {
		NWFSPrint("\n*** Disk Device (%d)  Sectors %X ***\n", (int) disk,
			 (unsigned int) ((SystemDisk[disk]->Cylinders) *
			    SystemDisk[disk]->TracksPerCylinder *
			    SystemDisk[disk]->SectorsPerTrack));

		for (r=0; r < 4; r++)
		{
		   if (SystemDisk[disk]->PartitionTable[r].nSectorsTotal)
		   {
		      if (SystemDisk[disk]->PartitionTable[r].SysFlag == NETWARE_386_ID)
			 NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s %s\n",
			    (int) (r + 1),
			    (SystemDisk[disk]->PartitionTable[r].fBootable
			    ? "A" : " "),
			    (int) SystemDisk[disk]->PartitionTable[r].StartLBA,
			    (int) SystemDisk[disk]->PartitionTable[r].nSectorsTotal,
			    (unsigned int)
				(((LONGLONG)SystemDisk[disk]->PartitionTable[r].nSectorsTotal *
				  (LONGLONG)SystemDisk[disk]->BytesPerSector) /
				  (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[r].SysFlag),
			    (SystemDisk[disk]->PartitionVersion[r]
			     ? "3.x" : "4.x/5.x"));
		      else
			 NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s\n",
			    (int) (r + 1),
			    (SystemDisk[disk]->PartitionTable[r].fBootable
			    ? "A" : " "),
			    (int) SystemDisk[disk]->PartitionTable[r].StartLBA,
			    (int) SystemDisk[disk]->PartitionTable[r].nSectorsTotal,
			    (unsigned int)
				(((LONGLONG)SystemDisk[disk]->PartitionTable[r].nSectorsTotal *
				  (LONGLONG)SystemDisk[disk]->BytesPerSector) /
				  (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[r].SysFlag));
		   }
		   else
		      NWFSPrint("#%d  ------------- Unused ------------- \n", (int) r + 1);
		}

		NWFSPrint("\n     Enter Partition to Delete (1-4): ");
		fgets(workBuffer, sizeof(workBuffer), stdin);
		selected = atoi(workBuffer);
		if (selected && selected <= 4)
		{
		   for (r = 0; r < 4; r++)
		   {
		      if (SystemDisk[disk]->PartitionTable[r].nSectorsTotal)
		      {
			 if (r + 1 == selected)
			 {
			    NWFSPrint("\nPartition (%d) Deleted\n", (int) selected);
			    NWFSSet(&SystemDisk[disk]->PartitionTable[r], 0,
				    sizeof(struct PartitionTableEntry));
			 }
		      }
		      else
		      if (r + 1 == selected)
			 NWFSPrint("#%d is not a valid partition entry\n", (int) selected);
		    }
		}
		else
		    NWFSPrint("#%d is not a valid partition number\n", (int) selected);
	     }
	     WaitForKey();
	     break;

	  case '7':
	     ClearScreen(consoleHandle);
	     disk = CurrentDisk;
	     if ((SystemDisk[disk]) && (SystemDisk[disk]->PhysicalDiskHandle))
	     {
		NWFSPrint("WARNING !!! this operation will erase all data on \nthis drive.  Continue? [Y/N]: ");
		fgets(workBuffer, sizeof(workBuffer), stdin);
		if (toupper(workBuffer[0]) != 'Y')
		   break;

		NWFSPrint("\n*** Disk Device (%d)  Sectors %X ***\n", (int) disk,
			 (unsigned int) ((SystemDisk[disk]->Cylinders) *
			    SystemDisk[disk]->TracksPerCylinder *
			    SystemDisk[disk]->SectorsPerTrack));

		for (r=0; r < 4; r++)
		{
		   if (SystemDisk[disk]->PartitionTable[r].nSectorsTotal)
		   {
		      if (SystemDisk[disk]->PartitionTable[r].SysFlag == NETWARE_386_ID)
			 NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s %s\n",
			    (int) (r + 1),
			    (SystemDisk[disk]->PartitionTable[r].fBootable
			    ? "A" : " "),
			    (int) SystemDisk[disk]->PartitionTable[r].StartLBA,
			    (int) SystemDisk[disk]->PartitionTable[r].nSectorsTotal,
			    (unsigned int)
				(((LONGLONG)SystemDisk[disk]->PartitionTable[r].nSectorsTotal *
				  (LONGLONG)SystemDisk[disk]->BytesPerSector) /
				  (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[r].SysFlag),
			    (SystemDisk[disk]->PartitionVersion[r]
			     ? "3.x" : "4.x/5.x"));
		      else
			 NWFSPrint("#%d %s LBA: 0x%08X  Sectors: 0x%08X %5uMB %s\n",
			    (int) (r + 1),
			    (SystemDisk[disk]->PartitionTable[r].fBootable
			    ? "A" : " "),
			    (int) SystemDisk[disk]->PartitionTable[r].StartLBA,
			    (int) SystemDisk[disk]->PartitionTable[r].nSectorsTotal,
			    (unsigned int)
				(((LONGLONG)SystemDisk[disk]->PartitionTable[r].nSectorsTotal *
				  (LONGLONG)SystemDisk[disk]->BytesPerSector) /
				  (LONGLONG)0x100000),
			    get_partition_type(SystemDisk[disk]->PartitionTable[r].SysFlag));
		   }
		   else
		      NWFSPrint("#%d  ------------- Unused ------------- \n", (int) r + 1);
		}

		NWFSPrint("\nWARNING !!! Are you certain you want to erase all data and partitions on \nthis drive? [Y/N]: ");
		fgets(workBuffer, sizeof(workBuffer), stdin);
		if (toupper(workBuffer[0]) != 'Y')
		   break;

		// zero the partition table and partition signature
		NWFSSet(&SystemDisk[disk]->PartitionTable[0].fBootable, 0, 64);
	     }
	     break;

	  case '8':
	     ClearScreen(0);
	     NWFSPrint("WARNING !!! this operation will assign all free space on this drive \nto Netware Partition(s)\n");
	     NWFSPrint("\nContinue to assign all free space to Netware Partition(s) ? [Y/N]: ");
	     fgets(workBuffer, sizeof(workBuffer), stdin);
	     if (toupper(workBuffer[0]) != 'Y')
		break;

	     NWAllocateUnpartitionedSpace();
	     break;

	  case '9':
	     NWFSFree(Buffer);
	     SyncDevice((ULONG)SystemDisk[disk]->PhysicalDiskHandle);
	     UtilScanVolumes();
	     return;
       }
    }
}

ULONG convert_volume_name_to_handle(
	BYTE *volume_name,
	ULONG	*vpart_handle)
{
    ULONG j, i;
    ULONG handles[7];
    nwvp_vpartition_info vpinfo;
    nwvp_payload payload;

    payload.payload_object_count = 0;
    payload.payload_index = 0;
    payload.payload_object_size_buffer_size = (4 << 20) + sizeof(handles);
    payload.payload_buffer = (BYTE *) &handles[0];
    do
    {
       nwvp_vpartition_scan(&payload);
       for (j=0; j < payload.payload_object_count; j++)
       {
	  if (nwvp_vpartition_return_info(handles[j], &vpinfo) == 0)
	  {
	     for (i=0; i < (ULONG)(vpinfo.volume_name[0] + 1); i++)
	     {
		if (volume_name[i] != vpinfo.volume_name[i])
		   break;
	     }

	     if (i== (ULONG)(vpinfo.volume_name[0] + 1))
	     {
		*vpart_handle = handles[j];
		return (0);
	     }
	  }
       }
    } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
    return (-1);
}

void	build_hotfix_tables(void)
{
    ULONG j, i;
    ULONG	group_number = 0;
    ULONG	vhandle;
    ULONG	logical_capacity;
    ULONG	raw_ids[4];
    nwvp_payload payload;

    nwvp_rpartition_scan_info 		rlist[5];
    nwvp_rpartition_info	 	rpart_info;
    nwvp_lpartition_scan_info 		llist[5];
    nwvp_lpartition_info 		lpart_info;

    UtilScanVolumes();

    payload.payload_object_count = 0;
    payload.payload_index = 0;
    payload.payload_object_size_buffer_size = (sizeof(nwvp_rpartition_scan_info) << 20) + sizeof(rlist);
    payload.payload_buffer = (BYTE *) &rlist[0];
    do
    {
	nwvp_rpartition_scan(&payload);
	for (j=0; j< payload.payload_object_count; j++)
	{
	    nwvp_rpartition_return_info(rlist[j].rpart_handle, &rpart_info);
	    if ((rpart_info.partition_type == 0x65) || (rpart_info.partition_type == 0x77))
	    {
		if (rpart_info.lpart_handle == 0xFFFFFFFF)
		{
		   logical_capacity = (rpart_info.physical_block_count & 0xFFFFFF00) - 242;
		   raw_ids[0] = rpart_info.rpart_handle;
		   raw_ids[1] = 0xFFFFFFFF;
		   raw_ids[2] = 0xFFFFFFFF;
		   raw_ids[3] = 0xFFFFFFFF;
		   nwvp_lpartition_format(&vhandle, logical_capacity, &raw_ids[0]);
		}
	    }
	}
    } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));

    hotfix_table_count = 0;
    payload.payload_object_count = 0;
    payload.payload_index = 0;
    payload.payload_object_size_buffer_size = (sizeof(nwvp_lpartition_scan_info) << 20) + sizeof(llist);
    payload.payload_buffer = (BYTE *) &llist[0];
    do
    {
       nwvp_lpartition_scan(&payload);
       for (j=0; j< payload.payload_object_count; j++)
       {
	  if (nwvp_lpartition_return_info(llist[j].lpart_handle, &lpart_info) == 0)
	  {
	     for (i=0; i<lpart_info.mirror_count; i++)
	     {
		if (hotfix_table_count <= 256)
		{
		   hotfix_table[hotfix_table_count].group_number = group_number;
		   hotfix_table[hotfix_table_count].rpart_handle = lpart_info.m[i].rpart_handle;
		   hotfix_table[hotfix_table_count].lpart_handle = llist[j].lpart_handle;
		   hotfix_table[hotfix_table_count].physical_block_count = lpart_info.logical_block_count + lpart_info.m[i].hotfix_block_count;
		   hotfix_table[hotfix_table_count].logical_block_count = lpart_info.logical_block_count;
		   hotfix_table[hotfix_table_count].segment_count = lpart_info.segment_count;
		   hotfix_table[hotfix_table_count].mirror_count = lpart_info.mirror_count;
		   hotfix_table[hotfix_table_count].hotfix_block_count = ((lpart_info.m[i].status & MIRROR_PRESENT_BIT) == 0) ? 0 : lpart_info.m[i].hotfix_block_count;
		   hotfix_table[hotfix_table_count].insynch_flag = ((lpart_info.m[i].status & MIRROR_INSYNCH_BIT) == 0) ? 0 : 0xFFFFFFFF;
		   hotfix_table[hotfix_table_count].active_flag = ((lpart_info.lpartition_status & MIRROR_GROUP_ACTIVE_BIT) == 0) ? 0 : 0xFFFFFFFF;
		   hotfix_table_count ++;
		}
	     }
	     group_number ++;
	  }
       }
    } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
}

void NWEditHotFix(void)
{
    ULONG i;
    ULONG index;
    ULONG index1;
    ULONG block_count;
    ULONG raw_ids[4];
    ULONG display_base = 0;

    BYTE input_buffer[20] = { "" };
    BYTE workBuffer[20] = { "" };

    build_hotfix_tables();

    while (TRUE)
    {
       SyncDisks();

       ClearScreen(consoleHandle);
       NWFSPrint("NetWare Mirroring Manager\n");
       NWFSPrint("Copyright (c) 1999, 2000 Jeff V. Merkey  All Rights Reserved.\n");
       NWFSPrint("\n    Mirror   Vol      Data     Hotfix  Partition  Synch/  ");
       NWFSPrint("\n #  Grp Cnt  Segs     Size      Size      Size    Active  Disk:Partition\n");

       for (i=0; i < 8; i++)
       {
	  if ((index = display_base + i) < hotfix_table_count)
	  {
	     LONGLONG size;
             nwvp_rpartition_info rpart_info;
#if (LINUX_UTIL)
	     extern ULONG MaxHandlesSupported;
             extern BYTE *PhysicalDiskNames[MAX_DISKS];
#endif

	     NWFSPrint("%2d   %2d  %2d  %2d   ",
		      (int) index,
		      (int) hotfix_table[index].group_number,
		      (int) hotfix_table[index].mirror_count,
		      (int) hotfix_table[index].segment_count);

	     size = (LONGLONG)((LONGLONG)hotfix_table[index].logical_block_count *
			       (LONGLONG)IO_BLOCK_SIZE);
	     if (size < 0x100000)
		NWFSPrint("%8uK ", (unsigned)(size / (LONGLONG)1024));
	     else
		NWFSPrint("%8uMB", (unsigned)(size / (LONGLONG)0x100000));

	     size = (LONGLONG)((LONGLONG)hotfix_table[index].hotfix_block_count *
			       (LONGLONG)IO_BLOCK_SIZE);
	     if (hotfix_table[index].hotfix_block_count != 0)
	     {
		     if (size < 0x100000)
			NWFSPrint("%8uK ", (unsigned)(size / (LONGLONG)1024));
		     else
			NWFSPrint("%8uMB", (unsigned)(size / (LONGLONG)0x100000));

		     size = (LONGLONG)((LONGLONG)hotfix_table[index].physical_block_count *
				       (LONGLONG)IO_BLOCK_SIZE);
		     if (size < 0x100000)
			NWFSPrint("%8uK   ", (unsigned)(size / (LONGLONG)1024));
		     else
			NWFSPrint("%8uMB  ", (unsigned)(size / (LONGLONG)0x100000));

	        NWFSPrint((hotfix_table[index].insynch_flag != 0) ? " IN/" : "OUT/");
	        NWFSPrint((hotfix_table[index].active_flag != 0) ? "YES  " : "NO   ");

                nwvp_rpartition_return_info(hotfix_table[index].rpart_handle, &rpart_info);
	        if ((rpart_info.partition_type == 0x65) || (rpart_info.partition_type == 0x77))
	        {
#if (LINUX_UTIL)
		   NWFSPrint("%d:%d %s\n",
			   (int)rpart_info.physical_disk_number,
			   (int)rpart_info.partition_number + 1,
			   PhysicalDiskNames[rpart_info.physical_disk_number %
	  		   MaxHandlesSupported]);
#else
		   NWFSPrint("%d:%d\n",
			   (int)rpart_info.physical_disk_number,
			   (int)rpart_info.partition_number + 1);
#endif
	        } 
	     }
	     else
	     {
	     NWFSPrint("     PARTITION NOT PRESENT ");
	     }
	  }
	  else
	     NWFSPrint("\n");
       }
       NWFSPrint("\n");

       NWFSPrint("               1. Add Mirror \n");
       NWFSPrint("               2. Delete Mirror\n");
       NWFSPrint("               3. Activate Incomplete Mirror Group\n");
       NWFSPrint("               4. Resize Hotfix\n");
       NWFSPrint("               5. Scroll Hotfix List\n");
       NWFSPrint("               6. Exit Menu\n");
       NWFSPrint("        Enter a Menu Option: ");

       fgets(input_buffer, sizeof(input_buffer), stdin);
       switch (input_buffer[0])
       {
	  case '1':
	     NWFSPrint("    Enter Primary Partition Number: ");
	     if (input_get_number(&index) == 0)
	     {
		if (index < hotfix_table_count)
		{
		   NWFSPrint("    Enter Secondary Partition Number: ");
		   if (input_get_number(&index1) == 0)
		   {
		      if (index1 < hotfix_table_count)
		      {
			 if (hotfix_table[index1].mirror_count == 1)
			 {
			    if (hotfix_table[index1].segment_count == 0)
			    {
			       if ((hotfix_table[index1].physical_block_count + 65) >= hotfix_table[index].logical_block_count)
			       {
				  nwvp_lpartition_unformat(hotfix_table[index1].lpart_handle);
				  nwvp_partition_add_mirror(hotfix_table[index].lpart_handle, hotfix_table[index1].rpart_handle);
				  build_hotfix_tables();
				  break;
			       }
			       NWFSPrint("       *********  Secondary Partition Too Small **********\n");
			       WaitForKey();
			       break;
			    }
			    NWFSPrint("       *********  Secondary Partition Has Volume Segments **********\n");
			    WaitForKey();
			    break;
			 }
			 NWFSPrint("       *********  Secondary Partition Is Already Mirrored **********\n");
			 WaitForKey();
			 break;
		      }
		   }
		   NWFSPrint("       *********  Invalid Secondary Partition Number **********\n");
		   WaitForKey();
		   break;
		}
	     }
	     NWFSPrint("       *********  Invalid Primary Partition Number **********\n");
	     WaitForKey();
	     break;

	  case '2':
	     NWFSPrint("    Enter Partition Number: ");
	     if (input_get_number(&index) == 0)
	     {
		if ((index < hotfix_table_count) && (hotfix_table[index].mirror_count > 1))
		{
		   i = 0;
		   if (hotfix_table[index].insynch_flag != 0)
		   {
		      for (i=0; i<hotfix_table_count; i++)
		      {
			 if ((i != index) &&
			     (hotfix_table[i].lpart_handle == hotfix_table[index].lpart_handle) &&
			     (hotfix_table[i].insynch_flag != 0))
			     break;
		      }
		   }
		   if (i != hotfix_table_count)
		   {
		      if (nwvp_partition_del_mirror(hotfix_table[index].lpart_handle, hotfix_table[index].rpart_handle) == 0)
		      {
			 build_hotfix_tables();
			 WaitForKey();
			 break;
		      }
		   }
		   NWFSPrint("  *********  Can not delete last valid mirror member **********\n");
		   WaitForKey();
		   break;
		}
	     }
	     NWFSPrint("       *********  Invalid Primary Partition Number **********\n");
	     WaitForKey();
	     break;

	  case '3':
	     NWFSPrint("    Enter Partition Number: ");
	     if (input_get_number(&index) == 0)
	     {
		if (index < hotfix_table_count)
		{
		   if (hotfix_table[index].active_flag == 0)
		   {
			if (nwvp_lpartition_activate(hotfix_table[index].lpart_handle) == 0)
			{
				build_hotfix_tables();
				break;
			}
			NWFSPrint("       *********  Can not Activate Logical Partition **********\n");
			WaitForKey();
			break;
		   }
		   NWFSPrint("       *********  Logical Partition Already Actuve **********\n");
		   WaitForKey();
		   break;
		}
	     }
	     NWFSPrint("       *********  Invalid Partition Number **********\n");
	     WaitForKey();
	     break;

	  case '4':
	     NWFSPrint("    Enter Partition Number: ");
	     if (input_get_number(&index) == 0)
	     {
		if (index < hotfix_table_count)
		{
		   if ((hotfix_table[index].segment_count== 0) && (hotfix_table[index].mirror_count == 1))
		   {
		      register ULONG DataSize;

		      if (hotfix_table[index].physical_block_count < 101)
		      {
			 NWFSPrint("       *********  Invalid Size **********\n");
			 WaitForKey();
			 break;
		      }

		      NWFSPrint("    Enter Logical Data Area Size in MB (up to %5uMB): ",
			       (unsigned int)
				  (((LONGLONG)(hotfix_table[index].physical_block_count - 100) *
				    (LONGLONG)IO_BLOCK_SIZE) /
                                    (LONGLONG)0x100000));

		      fgets(workBuffer, sizeof(workBuffer), stdin);
		      DataSize = atol(workBuffer);
		      block_count = (ULONG)(((LONGLONG)DataSize *
                                             (LONGLONG)0x100000) /
                                             (LONGLONG)IO_BLOCK_SIZE);
		      if (block_count)
		      {
			 if ((block_count + 100) < hotfix_table[index].physical_block_count)
			 {
			    raw_ids[0] = hotfix_table[index].rpart_handle;
			    raw_ids[1] = 0xFFFFFFFF;
			    raw_ids[2] = 0xFFFFFFFF;
			    raw_ids[3] = 0xFFFFFFFF;
			    nwvp_lpartition_unformat(hotfix_table[index].lpart_handle);
			    nwvp_lpartition_format(&hotfix_table[index].lpart_handle,
						   block_count, &raw_ids[0]);
			    build_hotfix_tables();
			    break;
			 }
		      }
		      NWFSPrint("       *********  Invalid Size **********\n");
		      WaitForKey();
		      break;
		   }
		   NWFSPrint("       *********  Cannot Resize Partition With NetWare Volumes **********\n");
		   WaitForKey();
		   break;
		}
	     }
	     NWFSPrint("       *********  Invalid Partition Number **********\n");
	     WaitForKey();
	     break;

	  case '5':
	     display_base += 8;
	     if (display_base >= hotfix_table_count)
		display_base = 0;
	     break;

	  case	'6':
	     SyncDisks();
	     return;
       }
    }
}

void build_segment_tables(void)
{
    ULONG j, i, k, l;
    ULONG block_count;
    ULONG handles[7];
    nwvp_vpartition_info vpinfo;
    nwvp_lpartition_space_return_info	slist[5];
    nwvp_lpartition_scan_info		llist[5];

    nwvp_payload payload;
    nwvp_payload payload1;
    segment_table_count = 0;

    payload1.payload_object_count = 0;
    payload1.payload_index = 0;
    payload1.payload_object_size_buffer_size = (sizeof(nwvp_lpartition_scan_info) << 20) + sizeof(llist);
    payload1.payload_buffer = (BYTE *) &llist[0];
    do
    {
       nwvp_lpartition_scan(&payload1);
       for (l=0; l < payload1.payload_object_count; l++)
       {
	  payload.payload_object_count = 0;
	  payload.payload_index = 0;
	  payload.payload_object_size_buffer_size = (sizeof(nwvp_lpartition_space_return_info) << 20) + sizeof(slist);
	  payload.payload_buffer = (BYTE *) &slist[0];
	  do
	  {
	     nwvp_lpartition_return_space_info(llist[l].lpart_handle, &payload);
	     for (j=0; j < payload.payload_object_count; j++)
	     {
		if (segment_table_count <= 256)
		{
		   segment_table[segment_table_count].lpart_handle = slist[j].lpart_handle;
		   segment_table[segment_table_count].segment_offset = slist[j].segment_offset;
		   segment_table[segment_table_count].segment_count = slist[j].segment_count;
		   segment_table[segment_table_count].free_flag = ((slist[j].status_bits & INUSE_BIT) == 0) ? 0xFFFFFFFF : 0;
		   segment_mark_table[segment_table_count] = 0;
		   segment_table_count ++;
		}
	     }
	  } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
       }
    } while ((payload1.payload_index != 0) && (payload1.payload_object_count != 0));

    payload.payload_object_count = 0;
    payload.payload_index = 0;
    payload.payload_object_size_buffer_size = (7 << 20) + sizeof(handles);
    payload.payload_buffer = (BYTE *) &handles[0];
    do
    {
       nwvp_vpartition_scan(&payload);
       for (j=0; j < payload.payload_object_count; j++)
       {
	  if (nwvp_vpartition_return_info(handles[j], &vpinfo) == 0)
	  {
	     for (i=0; i < vpinfo.segment_count; i++)
	     {
		for (k=0; k < segment_table_count; k++)
		{
		   block_count = (segment_table[k].segment_count / vpinfo.blocks_per_cluster) * vpinfo.blocks_per_cluster;

		   if ((vpinfo.segments[i].lpart_handle == segment_table[k].lpart_handle) &&
		     (vpinfo.segments[i].partition_block_offset == segment_table[k].segment_offset) &&
		     (vpinfo.segments[i].segment_block_count == block_count))
		   {
		      nwvp_memmove(&segment_table[k].VolumeName[0], &vpinfo.volume_name[0], 16);
		      segment_table[k].segment_number = i + 1;
		      segment_table[k].total_segments = vpinfo.segment_count;
		      break;
		   }
		}
	     }
	  }
       }
    } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));
}

void ChangeVolumeFlags(VOLUME *volume)
{
    ULONG ccode, WorkFlags;
    BYTE input_buffer[20] = { "" };
    ROOT3X *root3x;
    ROOT *root;
    BYTE *WorkBuffer;

    WorkBuffer = NWFSAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!WorkBuffer)
    {
       NWFSPrint("*** could not allocate workbuffer ***\n");
       return;
    }

    NWFSSet(WorkBuffer, 0, IO_BLOCK_SIZE);
    root = (ROOT *) WorkBuffer;
    root3x = (ROOT3X *) root;

    WorkFlags = volume->VolumeFlags;
    while (TRUE)
    {
       SyncDisks();

       ClearScreen(0);
       NWFSPrint("NetWare Volume Settings Manager\n");
       NWFSPrint("Copyright (c) 1999, 2000 Jeff V. Merkey  All Rights Reserved.\n");

       NWFSPrint("      Volume Name                 : [%-15s]\n",
		 volume->VolumeName);

       NWFSPrint("      Volume Type                 : %s\n",
	  ((WorkFlags & NDS_FLAG) ||
	   (WorkFlags & NEW_TRUSTEE_COUNT))
	    ? "4.x/5.x" : "3.x");

       NWFSPrint("      Suballocation               : %s\n",
	  (WorkFlags & SUB_ALLOCATION_ON) ? "ON" : "OFF");

       NWFSPrint("      File Compression            : %s\n",
	  (WorkFlags & FILE_COMPRESSION_ON) ? "ON" : "OFF");

       NWFSPrint("      Data Migration              : %s\n",
	  (WorkFlags & DATA_MIGRATION_ON) ? "ON" : "OFF");

       NWFSPrint("      Auditing                    : %s\n",
	  (WorkFlags & AUDITING_ON) ? "ON" : "OFF");

       NWFSPrint("      Netware Directory Services  : %s\n",
	  (WorkFlags & NDS_FLAG)  ? "PRESENT" : "NONE");

       NWFSPrint("\n");
       NWFSPrint("               1. Apply Volume Setting Changes\n");
       NWFSPrint("               2. Toggle Suballocation Flag\n");
       NWFSPrint("               3. Toggle File Compression Flag\n");
       NWFSPrint("               4. Toggle Data Migration Flag\n");
       NWFSPrint("               5. Toggle Auditing Flag\n");
       NWFSPrint("               6. Toggle Netware Directory Service Flag\n");
       NWFSPrint("               7. Return\n\n");
       NWFSPrint("        Enter a Menu Option: ");

       fgets(input_buffer, sizeof(input_buffer), stdin);
       switch (input_buffer[0])
       {
	  case	'1':
	     nwvp_vpartition_open(volume->nwvp_handle);

	     ccode = nwvp_vpartition_block_read(volume->nwvp_handle,
			(volume->FirstDirectory * volume->BlocksPerCluster),
			1, WorkBuffer);
	     if (ccode)
	     {
		NWFSPrint("error reading volume [%s] root entry\n", volume->VolumeName);
		nwvp_vpartition_close(volume->nwvp_handle);
		WaitForKey();
		break;
	     }

	     volume->VolumeFlags = WorkFlags;
	     if (volume->VolumeFlags & NEW_TRUSTEE_COUNT)
		root->VolumeFlags = (BYTE)volume->VolumeFlags;
	     else
		root3x->VolumeFlags = (BYTE)volume->VolumeFlags;

	     NWFSPrint("Writing volume [%s] root entry\n", volume->VolumeName);

	     ccode = nwvp_vpartition_block_write(volume->nwvp_handle,
			(volume->FirstDirectory * volume->BlocksPerCluster),
			1, WorkBuffer);
	     if (ccode)
	     {
		NWFSPrint("error writing volume [%s] root entry (primary)\n", volume->VolumeName);
		nwvp_vpartition_close(volume->nwvp_handle);
		WaitForKey();
		break;
	     }

	     ccode = nwvp_vpartition_block_write(volume->nwvp_handle,
			(volume->SecondDirectory * volume->BlocksPerCluster),
			1, WorkBuffer);
	     if (ccode)
	     {
		NWFSPrint("error writing volume [%s] root entry (mirror)\n", volume->VolumeName);
		nwvp_vpartition_close(volume->nwvp_handle);
		WaitForKey();
		break;
	     }
	     nwvp_vpartition_close(volume->nwvp_handle);
	     WaitForKey();
	     break;

	  case	'2':
	     if (!(WorkFlags & NEW_TRUSTEE_COUNT))
	     {
		NWFSPrint("Only 4.x/5.x Volumes support Suballocation\n");
		WaitForKey();
		break;
	     }
	     if (WorkFlags & SUB_ALLOCATION_ON)
	     {
		NWFSPrint("Once enabled, disabling Suballocation is not allowed\n");
		WaitForKey();
	     }
	     else
		WorkFlags |= SUB_ALLOCATION_ON;
	     break;

	  case	'3':
	     if (!(WorkFlags & NEW_TRUSTEE_COUNT))
	     {
		NWFSPrint("Only 4.x/5.x Volumes support File Compression\n");
		WaitForKey();
		break;
	     }
	     if (WorkFlags & FILE_COMPRESSION_ON)
		WorkFlags &= ~FILE_COMPRESSION_ON;
	     else
		WorkFlags |= FILE_COMPRESSION_ON;
	     break;

	  case	'4':
	     if (!(WorkFlags & NEW_TRUSTEE_COUNT))
	     {
		NWFSPrint("Only 4.x/5.x Volumes support Data Migration\n");
		WaitForKey();
		break;
	     }
	     if (WorkFlags & DATA_MIGRATION_ON)
		WorkFlags &= ~DATA_MIGRATION_ON;
	     else
		WorkFlags |= DATA_MIGRATION_ON;
	     break;

	  case	'5':
	     if (WorkFlags & AUDITING_ON)
		WorkFlags &= ~AUDITING_ON;
	     else
		WorkFlags |= AUDITING_ON;
	     break;

	  case	'6':
	     if (!(WorkFlags & NEW_TRUSTEE_COUNT))
	     {
		NWFSPrint("Only 4.x/5.x Volumes support NDS\n");
		WaitForKey();
		break;
	     }
	     if (WorkFlags & NDS_FLAG)
		WorkFlags &= ~NDS_FLAG;
	     else
		WorkFlags |= NDS_FLAG;
	     break;

	  case	'7':
	     NWFSFree(WorkBuffer);
	     SyncDisks();
	     return;
       }
    }
}


void NWEditNamespaces(void)
{
    ULONG i, r, AvailNS, ccode;
    ULONG index, CountedNamespaces;
    ULONG display_base = 0;
    BYTE input_buffer[20] = { "" };
    BYTE *p;
    VOLUME *volume;
    BYTE NSID;
    BYTE NSFlag[MAX_NAMESPACES];
    ULONG NSTotal, value;
    BYTE NSArray[MAX_NAMESPACES];
    BYTE FoundNameSpace[MAX_NAMESPACES];
    extern ULONG AddSpecificVolumeNamespace(VOLUME *, ULONG);
    extern ULONG RemoveSpecificVolumeNamespace(VOLUME *, ULONG);

    while (TRUE)
    {
       SyncDisks();

       ClearScreen(consoleHandle);
       NWFSPrint("NetWare Volume Namespace Manager\n");
       NWFSPrint("Copyright (c) 1999, 2000 Jeff V. Merkey  All Rights Reserved.\n");

       NWFSPrint("\nVolume            COMPRESS SUBALLOC MIGRATE AUDIT  NameSpaces\n\n");

       for (i=0; i < 8; i++)
       {
	  if ((index = display_base + i) < MaximumNumberOfVolumes)
	  {
	     if (VolumeTable[index])
	     {
		NWFSPrint("[%-15s] ", VolumeTable[index]->VolumeName);
		NWFSPrint("   %s     %s      %s    %s   ",
			 (VolumeTable[index]->VolumeFlags & FILE_COMPRESSION_ON)
			  ? "YES" : "NO ",
			 (VolumeTable[index]->VolumeFlags & SUB_ALLOCATION_ON)
			  ? "YES" : "NO ",
			 (VolumeTable[index]->VolumeFlags & DATA_MIGRATION_ON)
			  ? "YES" : "NO ",
			 (VolumeTable[index]->VolumeFlags & AUDITING_ON)
			  ? "YES" : "NO ");

		if (VolumeTable[index]->NameSpaceCount)
		{
		   NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
		   for (r=0; r < (VolumeTable[index]->NameSpaceCount & 0xF); r++)
		   {
		      // don't display duplicate namespaces, report error.
		      if (r && (FoundNameSpace[VolumeTable[index]->NameSpaceID[r] & 0xF]))
		      {
			 if ((r + 1) >= (VolumeTable[index]->NameSpaceCount & 0xF))
			    NWFSPrint("???");
			 else
			    NWFSPrint("???,");
			 FoundNameSpace[VolumeTable[index]->NameSpaceID[r] & 0xF] = TRUE;
			 continue;
		      }
		      FoundNameSpace[VolumeTable[index]->NameSpaceID[r] & 0xF] = TRUE;

		      if ((r + 1) >= (VolumeTable[index]->NameSpaceCount & 0xF))
			 NWFSPrint("%s", NSDescription(VolumeTable[index]->NameSpaceID[r]));
		      else
			 NWFSPrint("%s,", NSDescription(VolumeTable[index]->NameSpaceID[r]));
		   }
		}
		else
		{
		   // if NameSpaceCount = 0, then we assume a single DOS namespace
		   NWFSPrint("DOS");
		}
		NWFSPrint("\n");
	     }
	     else
		NWFSPrint("\n");
	  }
	  else
	     NWFSPrint("\n");
       }
       NWFSPrint("\n");

       NWFSPrint("               1. Add Volume Namespace\n");
       NWFSPrint("               2. Delete Volume Namespace\n");
       NWFSPrint("               3. Change Volume Flags\n");
       NWFSPrint("               4. Scroll Volume List\n");
       NWFSPrint("               5. Return\n");
       NWFSPrint("        Enter a Menu Option: ");

       fgets(input_buffer, sizeof(input_buffer), stdin);
       switch (input_buffer[0])
       {
	  case '1':
	     NWFSPrint("Enter Volume Name: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     p = input_buffer;
	     while (*p)
	     {
		*p = toupper(*p);
		p++;
	     }
	     volume = GetVolumeHandle(input_buffer);
	     if (!volume)
	     {
		NWFSPrint("***** Invalid Volume Name ****\n");
		WaitForKey();
		break;
	     }

	     ClearScreen(0);
	     NWFSPrint("Volume [%-15s] has (%d) namespaces [",
		       volume->VolumeName, (int)volume->NameSpaceCount);

	     NWFSSet(&NSFlag[0], 0, MAX_NAMESPACES);
	     NWFSSet(&NSArray[0], 0, MAX_NAMESPACES);

	     // assume 6 namespaces are allowed
	     for (AvailNS = 6, i=0; i < (volume->NameSpaceCount & 0x0F); i++)
	     {
		NSID = (BYTE)(volume->NameSpaceID[i] & 0x0F);
		if (NSFlag[NSID])
		{
		   NWFSPrint("**** Multiple Namespace entry error ****\n");
		   WaitForKey();
		   break;
		}
		NSFlag[NSID] = TRUE;
		if (AvailNS)
		   AvailNS--;

		if ((i + 1) >= (volume->NameSpaceCount & 0x0F))
		   NWFSPrint("%s", NSDescription(volume->NameSpaceID[i]));
		else
		   NWFSPrint("%s, ", NSDescription(volume->NameSpaceID[i]));
	     }
	     NWFSPrint("]\n\n");

	     for (NSTotal = i = 0; i < 6; i++)
	     {
		if (!NSFlag[i])
		{
		   if ((i != FTAM_NAME_SPACE) && (i != NT_NAME_SPACE))
		   {
		      NSArray[NSTotal] = (BYTE)i;
		      NSTotal++;
		   }
		}
	     }

	     NWFSPrint("The following Namespaces may be added to this volume\n\n");
	     for (i=0; i < NSTotal; i++)
	     {
		NWFSPrint("%d. %s\n", (int)(i + 1), NSDescription(NSArray[i]));
	     }

	     NWFSPrint("\nselect a namespace to add: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);

	     value = atoi(input_buffer);
	     if ((!value) || ((value - 1) >= NSTotal))
	     {
		NWFSPrint("**** invalid namespace option ****\n");
		WaitForKey();
		break;
	     }

	     ccode = AddSpecificVolumeNamespace(volume, NSArray[value - 1]);
	     if (ccode)
		NWFSPrint("nwfs:  error (%d) adding namespace %s\n",
		  (int) ccode, NSDescription(NSArray[value - 1]));

	     WaitForKey();
	     break;

	  case '2':
	     NWFSPrint("Enter Volume Name: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     p = input_buffer;
	     while (*p)
	     {
		*p = toupper(*p);
		p++;
	     }
	     volume = GetVolumeHandle(input_buffer);
	     if (!volume)
	     {
		NWFSPrint("***** Invalid Volume Name ****\n");
		WaitForKey();
		break;
	     }

	     if (volume->NameSpaceCount <= 1)
	     {
		NWFSPrint("**** no valid namespace to remove ****\n");
		WaitForKey();
		break;
	     }

	     ClearScreen(0);

	     NWFSPrint("Removable namespaces for volume %s\n\n",
			volume->VolumeName);

	     // start with the second valid namespace entry
	     for (CountedNamespaces = 0, i=1;
		  i < (volume->NameSpaceCount & 0x0F); i++)
	     {
		if (volume->NameSpaceID[i])
		{
		   NWFSPrint("%d.  %s\n", (int)i,
			     NSDescription(volume->NameSpaceID[i]));
		   CountedNamespaces++;
		}
	     }

	     NWFSPrint("\nselect a namespace: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);

	     value = atoi(input_buffer);
	     if ((!value) || (value > CountedNamespaces))
	     {
		NWFSPrint("**** invalid namespace option ****\n");
		WaitForKey();
		break;
	     }

	     if (volume->NameSpaceID[value] == DOS_NAME_SPACE)
	     {
		NWFSPrint("**** Removing DOS Namespace not allowed ****\n");
		WaitForKey();
		break;
	     }

	     NSID = (BYTE)volume->NameSpaceID[value];

	     NWFSPrint("\nWARNING!!! You are about to remove the %s namespace from this\n",
		       NSDescription(NSID));
	     NWFSPrint("volume.  Removing a namespace could interfere with any \n");
	     NWFSPrint("applications that depend on that namespace.  Continue? [Y/N]: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     if (toupper(input_buffer[0]) != 'Y')
	     {
		WaitForKey();
		break;
	     }

	     ccode = RemoveSpecificVolumeNamespace(volume, NSID);
	     if (ccode)
		NWFSPrint("nwfs:  error (%d) removing namespace %s\n",
		  (int) ccode, NSDescription(NSID));

	     WaitForKey();
	     break;

	  case '3':
	     NWFSPrint("Enter Volume Name: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     p = input_buffer;
	     while (*p)
	     {
		*p = toupper(*p);
		p++;
	     }
	     volume = GetVolumeHandle(input_buffer);
	     if (!volume)
	     {
		NWFSPrint("***** Invalid Volume Name ****\n");
		WaitForKey();
		break;
	     }
	     ChangeVolumeFlags(volume);
	     break;

	  case '4':
	     display_base += 8;
	     if (display_base >= hotfix_table_count)
		display_base = 0;
	     break;

	  case	'5':
	     SyncDisks();
	     return;

	  default:
	     break;
       }
    }
}


void NWEditVolumes(void)
{
    register BYTE *Buffer;
    ULONG i, j;
    ULONG vpart_handle;
    ULONG block_count;
    ULONG index;
    ULONG display_base = 0;
    ULONG cluster_size;
    segment_info stable[32];
    vpartition_info vpart_info;
    nwvp_vpartition_info vpinfo;
    BYTE input_buffer[20] = { "" };
    BYTE workBuffer[20] = { "" };
    BYTE *bptr;

    Buffer = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Buffer)
    {
       NWFSPrint("could not allocate workspace\n");
       WaitForKey();
       return;
    }

    NWFSSet(Buffer, 0, IO_BLOCK_SIZE);
    UtilScanVolumes();
    for (i=0; i < TotalDisks; i++)
       SyncDevice(i);
    build_segment_tables();

    while (TRUE)
    {
       SyncDisks();

       ClearScreen(consoleHandle);
       NWFSPrint("NetWare Volume Manager\n");
       NWFSPrint("Copyright (c) 1999, 2000 Jeff V. Merkey  All Rights Reserved.\n");

       NWFSPrint("\n Mark #   MirrorGrp        Volume       Segment       Offset        Size \n");

       for (i=0; i < 8; i++)
       {
	  if ((index = display_base + i) < segment_table_count)
	  {
	     LONGLONG size;

	     if (segment_mark_table[index] == 0)
		NWFSPrint("     %2d ", (int) index);
	     else
		NWFSPrint(" %2d  %2d ", (int) segment_mark_table[index], (int) index);

	     if (segment_table[index].free_flag == 0)
	     {
		NWFSPrint("   %4d    [%15s]   %2d of ",
			  (int) segment_table[index].lpart_handle & 0x0000FFFF,
			  &segment_table[index].VolumeName[1],
			  (int) segment_table[index].segment_number);

		if ((int) segment_table[index].total_segments < 10)
			NWFSPrint("%1d ", (int) segment_table[index].total_segments);
		else
			NWFSPrint("%2d", (int) segment_table[index].total_segments);
	     }
	     else
		NWFSPrint("   %4d      *** FREE ***              ",
			  (int) segment_table[index].lpart_handle & 0x0000FFFF);

	     size = (LONGLONG)((LONGLONG)segment_table[index].segment_offset *
			       (LONGLONG)IO_BLOCK_SIZE);
	     if (size < 0x100000)
		NWFSPrint("  %10u K ", (unsigned)(size / (LONGLONG)1024));
	     else
		NWFSPrint("  %10u MB", (unsigned)(size / (LONGLONG)0x100000));

	     size = (LONGLONG)((LONGLONG)segment_table[index].segment_count *
			       (LONGLONG)IO_BLOCK_SIZE);
	     if (size < 0x100000)
		NWFSPrint("  %10u K \n", (unsigned)(size / (LONGLONG)1024));
	     else
		NWFSPrint("  %10u MB\n", (unsigned)(size / (LONGLONG)0x100000));
	  }
	  else
	     NWFSPrint("\n");
       }

       NWFSPrint("\n");
       NWFSPrint("               1. Create or Expand Volume (with marked segments) \n");
       NWFSPrint("               2. Delete Netware Volume\n");
       NWFSPrint("               3. Edit Partition Tables\n");
       NWFSPrint("               4. Edit Hotfix and Mirroring\n");
       NWFSPrint("               5. Split Free Segment \n");
       NWFSPrint("               6. Mark Free Segment \n");
       NWFSPrint("               7. Scroll Segment List\n");
       NWFSPrint("               8. Modify Volume Namespaces/Settings\n");
       NWFSPrint("               9. Exit Menu\n");
       NWFSPrint("        Enter a Menu Option: ");

       fgets(input_buffer, sizeof(input_buffer), stdin);
       switch (input_buffer[0])
       {
	  case '1':
	     for (j=0; j < segment_table_count; j++)
	     {
		if (segment_mark_table[j] == 1)
		   break;
	     }

	     if (j == segment_table_count)
	     {
		NWFSPrint("       *********  No Marked Segments **********\n");
		WaitForKey();
		break;
	     }

	     NWFSPrint("          Enter Volume Name: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);

	     for (j=0; j < 16; j++)
		vpart_info.volume_name[j] = 0;

	     bptr = input_buffer;
	     while (bptr[0] == ' ')
		bptr ++;

	     while ((*bptr != 0) && (*bptr != 0xA) && (*bptr != 0xD) && (*bptr != ' '))
	     {
		if ((bptr[0] >= 'a') && (bptr[0] <= 'z'))
		   bptr[0] = (bptr[0] - 'a') + 'A';
		vpart_info.volume_name[vpart_info.volume_name[0] + 1] = bptr[0];
		vpart_info.volume_name[0] ++;
		if (vpart_info.volume_name[0] == 15)
		   break;
		bptr ++;
	     }

	     if (vpart_info.volume_name[0] != 0)
	     {
		if (convert_volume_name_to_handle(&vpart_info.volume_name[0], &vpart_handle) == 0)
		{
		    if ((nwvp_vpartition_return_info(vpart_handle, &vpinfo) == 0) &&
			(vpinfo.volume_valid_flag != 0))
		    {
		      NWFSPrint(" *********  Can not expand incomplete volume **********\n");
		      WaitForKey();
		      break;
		    }

		      NWFSPrint(" volume %s already exists, do you want to expand? (Y/N): ", &vpart_info.volume_name[1]);
		      fgets(input_buffer, sizeof(input_buffer), stdin);
		      if ((input_buffer[0] == 'y') || (input_buffer[0] == 'Y'))
		      {
			 index = 1;
			 for (j=0; j < segment_table_count; j++)
			 {
			    if (segment_mark_table[j] == index)
			    {
			       stable[index - 1].lpartition_id = segment_table[j].lpart_handle;
			       stable[index - 1].block_offset = segment_table[j].segment_offset;
			       stable[index - 1].block_count = segment_table[j].segment_count;
			       if (nwvp_vpartition_add_segment(vpart_handle, &stable[index-1]) != 0)
			       {
				      build_segment_tables();
				      NWFSPrint(" *********  error adding volume segments **********\n");
				      WaitForKey();
				      break;
			       }
			       index ++;
			       j = 0xFFFFFFFF;
			    }
			 }
			 build_segment_tables();
		      }
		      break;
		}
		NWFSPrint("    Enter Cluster Size in Kbytes [4,8,16,32,64] (bigger is better): ");
		if (input_get_number(&cluster_size) == 0)
		{
		   vpart_info.cluster_count = 0;
		   if (cluster_size == 64)
		      vpart_info.blocks_per_cluster = 0x10;
		   else
		   if (cluster_size == 32)
		      vpart_info.blocks_per_cluster = 0x8;
		   else
		   if (cluster_size == 16)
		      vpart_info.blocks_per_cluster = 0x4;
		   else
		   if (cluster_size == 8)
		      vpart_info.blocks_per_cluster = 0x2;
		   else
		   if (cluster_size == 4)
		      vpart_info.blocks_per_cluster = 1;
		   else
		   {
		      NWFSPrint(" *********  invalid cluster size 64K 32K 16K 8K 4K **********\n");
		      WaitForKey();
		      break;
		   }

		   index = 1;
		   for (j=0; j < segment_table_count; j++)
		   {
		      if (segment_mark_table[j] == index)
		      {
			 stable[index - 1].lpartition_id = segment_table[j].lpart_handle;
			 stable[index - 1].block_offset = segment_table[j].segment_offset;
			 stable[index - 1].block_count = segment_table[j].segment_count;
			 stable[index - 1].extra = 0;
			 vpart_info.cluster_count += segment_table[j].segment_count / vpart_info.blocks_per_cluster;
			 index ++;
			 j = 0xFFFFFFFF;
		      }
		   }

		   vpart_info.segment_count = (index - 1);
		   vpart_info.flags = (SUB_ALLOCATION_ON | NDS_FLAG | NEW_TRUSTEE_COUNT);

		   if (nwvp_vpartition_format(&vpart_handle, &vpart_info, &stable[0]) == 0)
		   {
		      WaitForKey();
		      UtilScanVolumes();
		      build_segment_tables();
		      break;
		   }
		   NWFSPrint("       *********  Volume Creation Failure **********\n");
		   WaitForKey();
		   break;
		}
		NWFSPrint("       *********  Invalid Cluster Size **********\n");
		WaitForKey();
		break;
	     }
	     NWFSPrint("       *********  Invalid Volume Name Size **********\n");
	     WaitForKey();
	     break;

	  case '2':
	     NWFSPrint("          Enter Volume Name: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);

	     for (j=0; j < 16; j++)
		vpart_info.volume_name[j] = 0;

	     bptr = input_buffer;
	     while (bptr[0] == ' ')
		bptr ++;

	     while ((*bptr != 0) && (*bptr != 0xA) && (*bptr != 0xD) && (*bptr != ' '))
	     {
		if ((bptr[0] >= 'a') && (bptr[0] <= 'z'))
		   bptr[0] = (bptr[0] - 'a') + 'A';
		vpart_info.volume_name[vpart_info.volume_name[0] + 1] = bptr[0];
		vpart_info.volume_name[0] ++;
		if (vpart_info.volume_name[0] == 15)
		   break;
		bptr ++;
	     }

	     if (vpart_info.volume_name[0] != 0)
	     {
		if (convert_volume_name_to_handle(&vpart_info.volume_name[0], &vpart_handle) == 0)
		{
		   NWFSPrint(" Are you certain you want to delete volume %s ? [Y/N]: ",
			     &vpart_info.volume_name[1]);
		   fgets(input_buffer, sizeof(input_buffer), stdin);
		   if (toupper(input_buffer[0]) != 'Y')
		   {
		      WaitForKey();
		      break;
		   }

		   if (nwvp_vpartition_unformat(vpart_handle) == 0)
		   {
		      UtilScanVolumes();
		      build_segment_tables();
		      break;
		   }
		}
	     }
	     NWFSPrint("       *********  Invalid Volume Name Size **********\n");
	     WaitForKey();
	     break;

	  case '3':
	     NWEditPartitions();
	     build_segment_tables();
	     break;

	  case '4':
	     NWEditHotFix();
	     build_segment_tables();
	     break;

	  case '5':
	     NWFSPrint("    Enter Free Segment Number: ");
	     if (input_get_number(&index) == 0)
	     {
		if ((index < segment_table_count) && (segment_table[index].free_flag != 0))
		{
		   register ULONG SegmentSize;

		   NWFSPrint("    Enter New Segment Size in MB (up to %5uMB) : ",
		       (unsigned int)
                          (((LONGLONG)segment_table[index].segment_count *
                            (LONGLONG)IO_BLOCK_SIZE) /
                            (LONGLONG)0x100000));

		   fgets(workBuffer, sizeof(workBuffer), stdin);
		   SegmentSize = atol(workBuffer);
		   block_count = (ULONG)(((LONGLONG)SegmentSize *
                                          (LONGLONG)0x100000) /
                                          (LONGLONG)IO_BLOCK_SIZE);
		   if (block_count)
		   {
		      block_count &= 0xFFFFFFF0;
		      if (block_count < segment_table[index].segment_count)
		      {
			 for (j=segment_table_count; j>index; j --)
			 {
			    nwvp_memmove(&segment_table[j], &segment_table[j-1], sizeof(segment_info_table));
			    segment_mark_table[j] = segment_mark_table[j-1];
			 }
			 segment_table[index+1].segment_count -= block_count;
			 segment_table[index].segment_count = block_count;
			 segment_table[index+1].segment_offset += block_count;
			 segment_mark_table[index+1] = 0;
			 segment_table_count ++ ;
			 break;
		      }
		   }
		   NWFSPrint("       *********  Invalid Segment Size **********\n");
		   WaitForKey();
		   break;
		}
	     }
	     NWFSPrint("       *********  Invalid Segment Number **********\n");
	     WaitForKey();
	     break;

	  case '6':
	     NWFSPrint("    Enter Free Segment Number: ");
	     if (input_get_number(&index) == 0)
	     {
		if (index < segment_table_count)
		{
		   if (segment_table[index].free_flag != 0)
		   {
		      if (segment_mark_table[index] == 0)
		      {
			block_count = 0;
			for (j=index; j>0; j--)
			{
				if (segment_table[j-1].lpart_handle != segment_table[index].lpart_handle)
					break;
				if ((segment_table[j-1].free_flag == 0) ||
				    (segment_mark_table[j-1] !=0))
					block_count ++;
			}
			for (j=index+1; j< segment_table_count; j++)
			{
				if (segment_table[j].lpart_handle != segment_table[index].lpart_handle)
					break;
				if ((segment_table[j].free_flag == 0) ||
				    (segment_mark_table[j] !=0))
					block_count ++;
			}
			if (block_count > 7)
			{
				NWFSPrint("   *********  Can not create more than 8 segments per partition **********\n");
				WaitForKey();
				break;
			}
			 block_count = 0;
			 for (j=0; j < segment_table_count; j++)
			 {
			    if (segment_mark_table[j] > block_count)
				block_count = segment_mark_table[j];
			 }
			 segment_mark_table[index] = block_count + 1;
		      }
		      else
		      {
			 for (j=0; j<segment_table_count; j++)
			 {
			    if (segment_mark_table[j] > segment_mark_table[index])
				segment_mark_table[j] --;
			 }
			 segment_mark_table[index] = 0;
		      }
		      break;
		   }
		}
	     }
	     NWFSPrint("       *********  Invalid Segment Number **********\n");
	     WaitForKey();
	     break;

	  case '7':
	     display_base += 8;
	     if (display_base >= segment_table_count)
		display_base = 0;
	     break;

	  case '8':
	     NWEditNamespaces();
	     break;

	  case '9':
	     NWFSFree(Buffer);
	     SyncDisks();
	     return;
       }
    }
}

#endif
