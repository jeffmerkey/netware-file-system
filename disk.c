
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
*   FILE     :  DISK.C
*   DESCRIP  :  PC Platform Disk Support
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#include "globals.h"


ULONG TotalDisks = 0;
ULONG TotalNWParts = 0;
ULONG MaximumDisks = 0;
ULONG MaximumNWParts = 0;
ULONG FirstValidDisk = 0;


#if	(MULTI_MACHINE_EMULATION)
#define	TotalDisks 	machine->TotalDisks
#define	TotalNWParts	machine->TotalNWParts
#define	MaximumDisks	machine->MaximumDisks
#define	MaximumNWParts	machine->MaximumNWParts
#define	FirstValidDisk	machine->FirstValidDisk
uint8	disk_file[] = "./m2machXX/m2diskXX.dat";
BYTE NwPartSignature[16] = { 0, 'N', 'w', '_', 'P', 'a', 'R', 't', 'I',
				 't', 'I', 'o', 'N', 0, 0, 0 };
#else
NWDISK	*SystemDisk[MAX_DISKS];
uint8	disk_file[] = "m2diskXX.dat";
#endif

//
//   Linux Block Device Assignments
//
//   Linux Defines 8 IDE + 2 ESDI + 16 SCSI = 26 disks total
//
//   block device 3  (ATA and IDE Disks and CD-ROMs, 1st IDE Adapter)
//             /dev/hda   3:0
//             /dev/hdb   3:64
//
//   block device 8 (SCSI Disks, 1st SCSI Adapter)
//            /dev/sda    8:0
//            /dev/sdb    8:16
//            /dev/sdc    8:32
//            /dev/sdd    8:48
//            /dev/sde    8:64
//            /dev/sdf    8:80
//            /dev/sdg    8:96
//            /dev/sdh    8:112
//
//            /dev/sdi    8:128
//            /dev/sdj    8:144
//            /dev/sdk    8:160
//            /dev/sdl    8:176
//            /dev/sdm    8:192
//            /dev/sdn    8:208
//            /dev/sdo    8:224
//            /dev/sdp    8:240
//
//   block device 22  (ATA and IDE Disks and CD-ROMs, 2nd IDE Adapter)
//             /dev/hdc   22:0
//             /dev/hdd   22:64
//
//   block device 33  (ATA and IDE Disks and CD-ROMs, 3rd IDE Adapter)
//             /dev/hde   33:0
//             /dev/hdf   33:64
//
//   block device 34  (ATA and IDE Disks and CD-ROMs, 4th IDE Adapter)
//             /dev/hdg   34:0
//             /dev/hdh   34:64
//
//   block device 36  (MCA ESDI Disks)
//             /dev/eda   36:0
//             /dev/edb   36:64
//
//   Partition Types
//
//       0x01  FAT 12 Partition
//       0x02  Xenix Root Partition
//       0x03  Xenix Usr Partition
//       0x04  FAT 16 Partition < 32MB
//       0x05  Extended DOS Partition
//       0x06  FAT 16 Partition >= 32MB
//       0x07  IFS (HPFS) Partition
//       0x08  AIX Partition
//       0x09  AIX Boot Partition
//       0x0A  OS/2 Boot Manager
//       0x0B  FAT 32 Partition
//       0x0C  FAT 32 Partition (Int 13 Extensions)
//       0x0E  FAT 16 Partition (Win95 Int 13 Extensions)
//       0x0F  Extended DOS Partition (Int 13 Extensions)
//       0x40  Venix 286 Partition
//       0x41  Power PC (PReP) Partition
//       0x51  Novell [???]
//       0x52  Microport/CPM Partition
//       0x57  VNDI Partition
//       0x63  Unix Partition (GNU Hurd)
//       0x64  Netware 286 Partition
//       0x65  Netware 386 Partition
//       0x66  Netware SMS Partition
//       0x69  Netware NSS Partition
//       0x75  PC/IX
//       0x77  VNDI Partition
//       0x80  MINIX Partition
//       0x81  Linux/MINIX Partition
//       0x82  Linux SWAP Partition
//       0x83  Linux Native Partition
//       0x93  Amoeba Partition
//       0x94  Amoeba Bad Block Table Partition
//       0xA5  BSD/386 Partition
//       0xB7  BSDI File System Partition
//       0xB8  BSDI Swap Partition
//       0xC0  NTFT Partition
//       0xC7  Syrinx Partition
//       0xDB  CP/M Partition
//       0xE1  DOS access Parititon
//       0xE3  DOS R/O Partition
//       0xF2  DOS Secondary Partition
//       0xFF  Bad Block Table Partition
//
//



#if ((MANOS) || (DOS_UTIL) || (LINUX_UTIL) || (WINDOWS_NT_UTIL) || (WINDOWS_CONVERT) || (WINDOWS_98_UTIL))
#define	SECTOR_SIZE		512
#define	SECTORS_PER_BLOCK	8

#define	M2READ_ONLY	0x00000000
#define	M2READ_WRITE	0x00000001
#define	M2CREATE	0x00000002

uint8	*flags_table[] = {"rb", "r+b", "w+b"};

uint32	m2_fopen(
	uint32	*file_handle,
	uint8	*file_name,
	uint32	flags)
{
	if (flags < 3)
	{
		if ((*file_handle = (uint32) fopen(file_name, flags_table[flags])) != 0)
			return(0);
	}
	return(-1);
}

uint32	m2_fclose(
	uint32	file_handle)
{
	return(fclose((FILE *)file_handle));
}

uint32	m2_fsize(
	uint32	file_handle)
{
	fseek((FILE *)file_handle, 0, 2);
	return(ftell((FILE *)file_handle));
}


uint32	m2_fread(
	uint32	file_handle,
	uint32	sector_size,
	uint32	sector_offset,
	uint32	sector_count,
	uint8	*buffer)
{
	fseek((FILE *) file_handle, sector_offset * sector_size, 0);
	if (fread(buffer, sector_size, sector_count, (FILE *) file_handle) == sector_count)
		return(0);
	return(-1);
}

uint32	m2_fwrite(
	uint32	file_handle,
	uint32	sector_size,
	uint32	sector_offset,
	uint32	sector_count,
	uint8	*buffer)
{
	fseek((FILE *) file_handle, sector_offset * sector_size, 0);
	if (fwrite(buffer, sector_size, sector_count, (FILE *) file_handle) == sector_count)
		return(0);
	return(-1);
}
#endif

// This function verifies the reported partition extants and checks
// for any overlapping or out of bounds partition definitions.

extern ULONG FirstValidDisk;

ULONG ValidatePartitionExtants(ULONG disk)
{
    register ULONG i;
    register ULONG cEnd, nEnd, cStart, nStart, cSize, nSize;

#if (MULTI_MACHINE_EMULATION)
	m2machine	*machine;
	NWDISK	**SystemDisk;
	m2group_get_context(&machine);
	SystemDisk = (NWDISK **) &machine->LSystemDisk;
#endif
    if ((!SystemDisk[disk]) || (!SystemDisk[disk]->PhysicalDiskHandle))
       return -1;

    for (i=0; i < 4; i++)
    {
       if (!SystemDisk[disk]->PartitionTable[i].SysFlag)
	  continue;

       if ((SystemDisk[disk]->PartitionTable[i].StartLBA +
	    SystemDisk[disk]->PartitionTable[i].nSectorsTotal) > (ULONG)
	    (SystemDisk[disk]->Cylinders *
	     SystemDisk[disk]->TracksPerCylinder *
	     SystemDisk[disk]->SectorsPerTrack))
	  return -1;

       if (SystemDisk[disk]->PartitionTable[i].StartLBA <
	   SystemDisk[disk]->SectorsPerTrack)
	  return -1;

       if ((i + 1) < 4)
       {
	  for (; (i + 1) < 4; i++)
	  {
	     if (SystemDisk[disk]->PartitionTable[i + 1].SysFlag)
	     {
		cEnd = SystemDisk[disk]->PartitionTable[i].StartLBA +
		       SystemDisk[disk]->PartitionTable[i].nSectorsTotal;
		nEnd = SystemDisk[disk]->PartitionTable[i + 1].StartLBA +
		       SystemDisk[disk]->PartitionTable[i + 1].nSectorsTotal;
		cStart = SystemDisk[disk]->PartitionTable[i].StartLBA;
		nStart = SystemDisk[disk]->PartitionTable[i + 1].StartLBA;
		cSize = SystemDisk[disk]->PartitionTable[i].nSectorsTotal;
		nSize = SystemDisk[disk]->PartitionTable[i + 1].nSectorsTotal;

		if ((cStart >= nStart) && (cStart < nEnd))
		   return -1;

		if ((cEnd > nStart) && (cEnd < nEnd))
		   return -1;

		if ((cStart < nStart) && (cEnd > nEnd))
		   return -1;
	     }
	  }
       }
    }
    return 0;

}

//
//  Netware partitions use their own unique method for recording
//  cyl/head/sector addressing for big partitions.  This method
//  is detailed below.
//

ULONG SetPartitionTableGeometry(BYTE *hd, BYTE *sec, BYTE *cyl,
				ULONG LBA, ULONG TracksPerCylinder,
				ULONG SectorsPerTrack)
{
    ULONG offset, cylinders, head, sector;

#if (MULTI_MACHINE_EMULATION)
	m2machine	*machine;
	NWDISK	**SystemDisk;
	m2group_get_context(&machine);
	SystemDisk = (NWDISK **) &machine->LSystemDisk;
#endif
    if (!cyl || !hd || !sec)
       return -1;

    cylinders = (LBA / (TracksPerCylinder * SectorsPerTrack));
    offset = LBA % (TracksPerCylinder * SectorsPerTrack);
    head = (WORD)(offset / SectorsPerTrack);
    sector = (WORD)(offset % SectorsPerTrack) + 1;

    if (cylinders < 1023)
    {
       *sec = (BYTE)sector;
       *hd = (BYTE)head;
       *cyl = (BYTE)(cylinders & 0xff);
       *sec |= (BYTE)((cylinders >> 2) & 0xC0);
    }
    else
    {
	*sec = (BYTE)(SectorsPerTrack | 0xC0);
	*hd = (BYTE)(TracksPerCylinder - 1);
	*cyl = (BYTE)0xFE;
    }

    return 0;
}

ULONG SetPartitionTableValues(struct PartitionTableEntry *Part,
			      ULONG Type,
			      ULONG StartingLBA,
			      ULONG EndingLBA,
			      ULONG Flag,
			      ULONG TracksPerCylinder,
			      ULONG SectorsPerTrack)
{

    Part->SysFlag = (BYTE) Type;
    Part->fBootable = (BYTE) Flag;
    Part->StartLBA = StartingLBA;
    Part->nSectorsTotal = (EndingLBA - StartingLBA) + 1;

    SetPartitionTableGeometry(&Part->HeadStart, &Part->SecStart, &Part->CylStart,
			      StartingLBA, TracksPerCylinder, SectorsPerTrack);

    SetPartitionTableGeometry(&Part->HeadEnd, &Part->SecEnd, &Part->CylEnd,
			      EndingLBA, TracksPerCylinder, SectorsPerTrack);

    return 0;

}

BYTE *get_partition_type(ULONG type)
{
    switch (type)
    {
       case 0x01:
	  return "FAT 12";

       case 0x02:
	  return "Xenix Root";

       case 0x03:
	  return "Xenix USR";

       case 0x04:
	  return "FAT 16 < 32MB";

       case 0x05:
	  return "Extended DOS";

       case 0x06:
	  return "FAT 16 >= 32MB";

       case 0x07:
	  return "IFS (HPFS/NTFS)";

       case 0x08:
	  return "AIX Partition";

       case 0x09:
	  return "AIX Boot";

       case 0x0A:
	  return "OS/2 Boot Manager";

       case 0x0B:
	  return "FAT 32";

       case 0x0C:
	  return "FAT 32 (Int 13 Ext)";

       case 0x0E:
	  return "FAT 16 (Int 13 Ext)";

       case 0x0F:
	  return "Ext DOS (Int 13 Ext)";

       case 0x40:
	  return "Venix 286";

       case 0x41:
	  return "Power PC (PReP)";

       case 0x51:
	  return "Novell [???]";

       case 0x52:
	  return "Microport/CPM";

       case 0x57:
	  return "NWFS";

       case 0x63:
	  return "Unix (GNU Hurd)";

       case 0x64:
	  return "Netware 286";

       case 0x65:
	  return "Netware 386";

       case 0x66:
	  return "Netware SMS";

       case 0x75:
	  return "PC/IX";

       case 0x77:
	  return "M2FS";

       case 0x80:
	  return "MINIX";

       case 0x81:
	  return "Linux/MINIX";

       case 0x82:
	  return "LINUX SWAP";

       case 0x83:
	  return "LINUX Native";

       case 0x93:
	  return "Amoeba";

       case 0x94:
	  return "Amoeba BBT";

       case 0xA5:
	  return "BSD/386";

       case 0xB7:
	  return "BSDI File System";

       case 0xB8:
	  return "BSDI Swap";

       case 0xC0:
	  return "NTFT";

       case 0xC7:
	  return "Syrinx";

       case 0xDB:
	  return "CP/M";

       case 0xE1:
	  return "DOS access";

       case 0xE3:
	  return "DOS R/O";

       case 0xF2:
	  return "DOS Secondary";

       case 0xFF:
	  return "BBT";

       default:
	  return "Unknown";
    }
}


void GetNumberOfDisks(ULONG *disk_count)
{
#if (MULTI_MACHINE_EMULATION)
	m2machine	*machine;
	m2group_get_context(&machine);
#endif
    if (disk_count)
       *disk_count = TotalDisks;
}

ULONG GetDiskInfo(ULONG disk_number, DISK_INFO *disk_info)
{
    register ULONG i;
#if (MULTI_MACHINE_EMULATION)
	m2machine	*machine;
	NWDISK	**SystemDisk;
	m2group_get_context(&machine);
	SystemDisk = (NWDISK **) &machine->LSystemDisk;
#endif

    if (SystemDisk[disk_number] != 0)
    {
       disk_info->disk_number = disk_number;
       disk_info->partition_count = 0;

       for (i=0; i < 4; i++)
       {
	  if (SystemDisk[disk_number]->PartitionTable[i].nSectorsTotal != 0)
	     disk_info->partition_count ++;
       }

       disk_info->total_sectors = (ULONG)
	    (SystemDisk[disk_number]->Cylinders *
	     SystemDisk[disk_number]->TracksPerCylinder *
	     SystemDisk[disk_number]->SectorsPerTrack);

       if ((disk_info->cylinder_size = (ULONG)
			  (SystemDisk[disk_number]->TracksPerCylinder *
			  SystemDisk[disk_number]->SectorsPerTrack)) == 0)
	  disk_info->cylinder_size = 0x40;

       disk_info->track_size = (ULONG) SystemDisk[disk_number]->SectorsPerTrack;
       disk_info->bytes_per_sector = (ULONG) SystemDisk[disk_number]->BytesPerSector;

       return (0);
    }
    return (-1);
}

ULONG GetPartitionInfo(ULONG disk_number,
		       ULONG partition_number,
		       PARTITION_INFO *part_info)
{
#if (MULTI_MACHINE_EMULATION)
	m2machine	*machine;
	NWDISK	**SystemDisk;
	m2group_get_context(&machine);
	SystemDisk = (NWDISK **) &machine->LSystemDisk;
#endif
    if (SystemDisk[disk_number] != 0)
    {
       if (SystemDisk[disk_number]->PartitionTable[partition_number].nSectorsTotal != 0)
       {
	  part_info->disk_number = disk_number;
	  part_info->partition_number = partition_number;

	  part_info->total_sectors = (ULONG)
	       (SystemDisk[disk_number]->Cylinders *
		SystemDisk[disk_number]->TracksPerCylinder *
		SystemDisk[disk_number]->SectorsPerTrack);

	  if ((part_info->cylinder_size = (ULONG)
			(SystemDisk[disk_number]->TracksPerCylinder *
			SystemDisk[disk_number]->SectorsPerTrack)) == 0)
	     part_info->cylinder_size = 0x40;

	  part_info->partition_type = SystemDisk[disk_number]->PartitionTable[partition_number].SysFlag;
	  part_info->boot_flag = SystemDisk[disk_number]->PartitionTable[partition_number].fBootable;
	  part_info->sector_offset = SystemDisk[disk_number]->PartitionTable[partition_number].StartLBA;
	  part_info->sector_count = SystemDisk[disk_number]->PartitionTable[partition_number].nSectorsTotal;

	  return (0);
       }
    }
    return (-1);
}

int get_number_of_partitions(int j)
{
   if (SystemDisk[j])
      return (SystemDisk[j]->NumberOfPartitions);
   return 0;
}

#if (FILE_DISK_EMULATION)

void RemoveDiskDevices(void)
{
	register ULONG j;
#if (MULTI_MACHINE_EMULATION)
	m2machine	*machine;
	NWDISK		**SystemDisk;
	m2group_get_context(&machine);
	SystemDisk = (NWDISK **) &machine->LSystemDisk;
#endif
	for (j=0; j < MAX_DISKS; j++)
    {
	   if (SystemDisk[j])
	   {
		m2_fclose((uint32) SystemDisk[j]->PhysicalDiskHandle);
	  NWFSFree(SystemDisk[j]);
	  SystemDisk[j] = 0;
	  if (TotalDisks)
		 TotalDisks--;
	   }
	}
}

uint32	quick_parse_disk_file(
	uint8	*bptr,
	uint32	*capacity,
	uint32	*part_info)
{
	uint32	index = 0;
	uint32	value;
	uint32	base_index;
	NWFSSet(part_info, 0, 16 * 4);
	*capacity = 0;
	while ((bptr[0] != 0) && (index < 16))
	{
		if ((bptr[0] == '0') && ((bptr[1] == 'x') || (bptr[1] == 'X')))
		{
			bptr += 2;
			value = 0;
			while (1)
			{
				if ((bptr[0] >= '0') && (bptr[0] <= '9'))
					value = (value * 16) + (bptr[0] - '0');
				else
				if ((bptr[0] >= 'a') && (bptr[0] <= 'f'))
					value = (value * 16) + (bptr[0] - 'a') + 10;
				else
				if ((bptr[0] >= 'A') && (bptr[0] <= 'F'))
					value = (value * 16) + (bptr[0] - 'A') + 10;
				else
					break;
				bptr ++;
			}
			if ((index % 4) == 0)
			{
				base_index = value;
			}
			if (base_index < 4)
			{
				part_info[(base_index * 4) + (index % 4)] = value;
				if ((index % 4) == 3)
				{
					if ((part_info[(base_index * 4) + 2] + part_info[(base_index * 4) + 3]) > *capacity)
						*capacity = part_info[(base_index * 4) + 2] + part_info[(base_index * 4) + 3];
				}

			}
			index ++;
		}
		bptr ++;
	}

	return((*capacity == 0) ? -1 : 0);
}

uint32	scan_init_flag = 0;
void ScanDiskDevices(void)
{
	uint32	i, j, k;
	uint32	free_index;
	uint32	retCode;
	uint32	file_handle;
	uint32	capacity;
	uint8	*Sector;
	uint32	part_info[16];
#if (MULTI_MACHINE_EMULATION)
	m2machine	*machine;
	NWDISK		**SystemDisk;
	m2group_get_context(&machine);
	SystemDisk = (NWDISK **) &machine->LSystemDisk;
	disk_file[8] = ((machine->file_number / 10) % 10) + '0';
	disk_file[9] = (machine->file_number % 10) + '0';
#endif

	if (scan_init_flag == 0)
	{
		NWFSSet(&SystemDisk[0], 0, MAX_DISKS * 4);
		scan_init_flag = 1;
	}

	Sector = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);

	for (i=0; i<32; i++)
	{
		free_index = 0xFFFFFFFF;
		for (j=0; j<MAX_DISKS; j++)
		{
			if (SystemDisk[j] != 0)
			{
				if (SystemDisk[j]->DiskNumber == i)
					break;
			}
			else
			{
				if (free_index == 0xFFFFFFFF)
					free_index = j;
			}
		}
		if ((j == MAX_DISKS) && (free_index != 0xFFFFFFFF))
		{
			j = free_index;
#if (MULTI_MACHINE_EMULATION)
			disk_file[17] = ((i / 10) % 10) + '0';
			disk_file[18] = (i % 10) + '0';
#else
			disk_file[6] = ((i / 10) % 10) + '0';
			disk_file[7] = (i % 10) + '0';
#endif

			if (m2_fopen(&file_handle, &disk_file[0], M2READ_WRITE) == 0)
			{
				Sector[510] = 0;
				Sector[511] = 0;
				fread(Sector, 1, 512, (FILE *) file_handle);
				if ((Sector[510] != 0x55) || (Sector[511] != 0xAA))
				{
					if (quick_parse_disk_file(Sector, &capacity, &part_info[0]) == 0)
					{
						for (k=0; k<4; k++)
						{
							if (part_info[(k * 4) + 1] != 0)
								SetPartitionTableValues((PARTITION_INFO *) &Sector[(512-66) + (k*16)], part_info[(k*4)+1], part_info[(k*4)+2], part_info[(k*4)+3] + part_info[(k*4)+2] - 1, 0, 32, 32);
						}
						Sector[510] = 0x55;
						Sector[511] = 0xAA;
						m2_fwrite(file_handle, SECTOR_SIZE, 0, SECTORS_PER_BLOCK, (uint8 *) Sector);
						m2_fwrite(file_handle, SECTOR_SIZE, capacity - SECTORS_PER_BLOCK, SECTORS_PER_BLOCK, (uint8 *) Sector);
					}
				}


				   capacity = m2_fsize(file_handle) / 512;
				   SystemDisk[j] = (NWDISK *) NWFSAlloc(sizeof(NWDISK), NWDISK_TAG);
				   NWFSSet(SystemDisk[j], 0, sizeof(NWDISK));

				   SystemDisk[j]->DiskNumber = i;
				   SystemDisk[j]->PhysicalDiskHandle = (void *) file_handle;
				   SystemDisk[j]->Cylinders = capacity / (32 * 32);
				   SystemDisk[j]->TracksPerCylinder = 32;
				   SystemDisk[j]->SectorsPerTrack = 32;
				   SystemDisk[j]->BytesPerSector = 512;
				   SystemDisk[j]->driveSize = (LONGLONG)
					  (SystemDisk[j]->Cylinders *
					   SystemDisk[j]->TracksPerCylinder *
					   SystemDisk[j]->SectorsPerTrack *
					   SystemDisk[j]->BytesPerSector);
				TotalDisks ++;



				NWFSCopy(&SystemDisk[j]->partitionSignature, &Sector[0x01FE], 2);

				if (SystemDisk[j]->partitionSignature == 0xAA55)
				{
					NWFSCopy(&SystemDisk[j]->PartitionTable[0].fBootable,
						  &Sector[0x01BE], 64);

// scan for Netware Partitions and detect 3.x and 4.x/5.x partition types
                                        for (k=0, SystemDisk[j]->NumberOfPartitions = 0; k < 4; k++)
					{
  	                                   if (SystemDisk[j]->PartitionTable[k].nSectorsTotal)
                                              SystemDisk[j]->NumberOfPartitions++;

						if (SystemDisk[j]->PartitionTable[k].SysFlag == NETWARE_386_ID)
						{
							retCode = ReadDiskSectors(j,
								SystemDisk[j]->PartitionTable[k].StartLBA,
								Sector,
								IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
								IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
							if (!retCode)
							{
								   NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices (part)\n", (int)j);
							}

							if (!NWFSCompare(Sector, NwPartSignature, 16))
								SystemDisk[j]->PartitionVersion[k] = NW4X_PARTITION;
							else
								SystemDisk[j]->PartitionVersion[k] = NW3X_PARTITION;
						}
					}
				}
			}
		}
	}
	NWFSFree(Sector);
	return;
}

#else

#if (LINUX_20 | LINUX_22 | LINUX_24)

ULONG LocateDevice(kdev_t dev)
{
    register ULONG major = MAJOR(dev);
    register ULONG minor = MINOR(dev);
    register ULONG dev_number, end_minor;
    struct gendisk *search;

#if (LINUX_22 | LINUX_24)
    lock_kernel();
#endif
    search = gendisk_head;
    while (search)
    {
        if (search->major == major)
        {
           // get the minor device index
     	   dev_number = (minor >> search->minor_shift);

           // determine the last valid minor device number
#if (LINUX_20 | LINUX_22)
	   end_minor = search->max_nr * search->max_p;
#elif (LINUX_24)
	   end_minor = search->max_p;
#endif
           // check if this minor device is present
           // and that it begins at sector 0 (Physical HDD Device)
	   if ((dev_number < end_minor) && (search->part) && 
	       (search->part[dev_number].nr_sects))
           {
#if (VERBOSE)
	      NWFSPrint("dev-%s major-%d:%d ns-%d ss-%d t-%d\n",
	                search->major_name, (int)search->major,(int)minor,
			(int)search->part[dev_number].nr_sects,
			(int)search->part[dev_number].start_sect,
			(int)search->part[dev_number].type);
#endif

#if (LINUX_22 | LINUX_24)
              unlock_kernel();
#endif
              return TRUE;
           }
#if (LINUX_22 | LINUX_24)
           unlock_kernel();
#endif
           return 0;
        }
        search = search->next;
    }
#if (LINUX_22 | LINUX_24)
    unlock_kernel();
#endif
    return 0;
}

#endif

#if (LINUX_24)

extern ULONG MaxDisksSupported;
extern ULONG MaxHandlesSupported;

void RemoveDiskDevices(void)
{
    register ULONG j;

    for (j=0; j < MAX_DISKS; j++)
    {
       if (SystemDisk[j])
       {
	  SyncDevice((ULONG)SystemDisk[j]->PhysicalDiskHandle);
          blkdev_put(SystemDisk[j]->inode.i_bdev, BDEV_FILE);
	  NWFSFree(SystemDisk[j]);
	  SystemDisk[j] = 0;
	  if (TotalDisks)
	     TotalDisks--;
       }
    }
}

extern struct blk_dev_struct blk_dev[MAX_BLKDEV];
extern int *blk_size[MAX_BLKDEV];
extern int *blksize_size[MAX_BLKDEV];
extern int *hardsect_size[MAX_BLKDEV];

void ScanDiskDevices(void)
{
    register kdev_t dev;
    register ULONG size;
    register ULONG j, i, retCode, len, major, minor;
    struct hd_geometry geometry;
    struct page *page;
    BYTE *Sector;

    page = alloc_page(GFP_ATOMIC);
    if (!page)
       return;

    Sector = (BYTE *)page_address(page);
    if (!Sector)
    {
       NWFSPrint("nwfs:  allocation error in AddDiskDevices\n");
       return;
    }

    for (FirstValidDisk = (ULONG)-1, j = 0; j < MAX_DISKS; j++)
    {
       if (!SystemDisk[j])
       {
          if (!DiskDevices[j])
             break;

	  dev = DiskDevices[j];
          major = MAJOR(dev);
          minor = MINOR(dev);

          // search the gendisk head and see if this device exists
          if (!LocateDevice(dev))
             continue;

	  SystemDisk[j] = (NWDISK *) NWFSAlloc(sizeof(NWDISK), NWDISK_TAG);
	  if (!SystemDisk[j])
	  {
	     NWFSPrint("nwfs: memory allocation failure in AddDiskDevices\n");
	     continue;
	  }
	  NWFSSet(SystemDisk[j], 0, sizeof(NWDISK));

          init_special_inode(&SystemDisk[j]->inode, S_IFBLK | S_IRUSR, dev);
          retCode = blkdev_open(&SystemDisk[j]->inode, &SystemDisk[j]->filp);
          if (retCode)
          {
             if (SystemDisk[j])
	        NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
             continue;
          }

          // perform a get drive geometry call to determine if it's a hard
          // disk or CDROM

#if (LINUX_22 | LINUX_24)
          lock_kernel();
#endif
	  retCode = ioctl_by_bdev(SystemDisk[j]->inode.i_bdev, HDIO_GETGEO,
	                          (ULONG)&geometry);
#if (LINUX_22 | LINUX_24)
          unlock_kernel();
#endif

	  if (retCode)
          {
             if (SystemDisk[j])
	        NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
	     continue;
          }

	  set_blocksize(dev, LogicalBlockSize);

          if (FirstValidDisk == (ULONG)-1)
             FirstValidDisk = j;

	  SystemDisk[j]->DiskNumber = j;
	  SystemDisk[j]->DeviceBlockSize = LogicalBlockSize;
          SystemDisk[j]->Cylinders = (LONGLONG) geometry.cylinders;
          SystemDisk[j]->TracksPerCylinder = geometry.heads;
          SystemDisk[j]->SectorsPerTrack = geometry.sectors;
	  SystemDisk[j]->PhysicalDiskHandle = (void *)((ULONG)dev);
	  SystemDisk[j]->BytesPerSector = hardsect_size[major] ? hardsect_size[major][minor] : 512;

	  size = (blk_size[major] ? (ULONG) blk_size[major][minor] : (ULONG) blk_size[major]);
	  SystemDisk[j]->driveSize = (LONGLONG)((LONGLONG) size *
				     (LONGLONG) SystemDisk[j]->BytesPerSector);

	  TotalDisks++;
	  if (TotalDisks > MaximumDisks)
	     MaximumDisks = TotalDisks;

       }

       retCode = ReadDiskSectors(j, 0, Sector,
				 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
				 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
       if (!retCode)
       {
	  NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices\n", (int)j);
	  continue;
       }

       len = SystemDisk[j]->BytesPerSector;

       NWFSCopy(&SystemDisk[j]->partitionSignature,
		   &Sector[0x01FE], 2);

       if (SystemDisk[j]->partitionSignature != 0xAA55)
       {
#if (VERBOSE)
	  NWFSPrint("nwfs:  partition signature 0xAA55 not found disk(%d)\n", j);
#endif
	  NWFSSet(&SystemDisk[j]->PartitionTable[0].fBootable, 0, 64);
	  continue;
       }
       else
       {
	  NWFSCopy(&SystemDisk[j]->PartitionTable[0].fBootable,
		   &Sector[0x01BE], 64);
       }

       // scan for Netware Partitions and detect 3.x and 4.x/5.x partition types
       for (i=0, SystemDisk[j]->NumberOfPartitions = 0; i < 4; i++)
       {
	  if (SystemDisk[j]->PartitionTable[i].nSectorsTotal)
             SystemDisk[j]->NumberOfPartitions++;

	  if (SystemDisk[j]->PartitionTable[i].SysFlag == NETWARE_386_ID)
	  {
	     retCode = ReadDiskSectors(j,
			  SystemDisk[j]->PartitionTable[i].StartLBA,
			  Sector,
			  IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
			  IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
	     if (!retCode)
	     {
		NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices (part)\n", (int)j);
		continue;
	     }

	     if (!NWFSCompare(Sector, NwPartSignature, 16))
		SystemDisk[j]->PartitionVersion[i] = NW4X_PARTITION;
	     else
		SystemDisk[j]->PartitionVersion[i] = NW3X_PARTITION;
	  }
       }
    }
    __free_page(page);
    return;
}

#endif


#if (LINUX_22)

extern ULONG MaxDisksSupported;
extern ULONG MaxHandlesSupported;

void RemoveDiskDevices(void)
{
    register ULONG j;

    for (j=0; j < MAX_DISKS; j++)
    {
       if (SystemDisk[j])
       {
	  SyncDevice((ULONG)SystemDisk[j]->PhysicalDiskHandle);

          if (SystemDisk[j]->filp.f_op)
             blkdev_release(&SystemDisk[j]->inode);

	  NWFSFree(SystemDisk[j]);
	  SystemDisk[j] = 0;

	  if (TotalDisks)
	     TotalDisks--;
       }
    }
}

extern struct blk_dev_struct blk_dev[MAX_BLKDEV];
extern int *blk_size[MAX_BLKDEV];
extern int *blksize_size[MAX_BLKDEV];
extern int *hardsect_size[MAX_BLKDEV];
extern struct file_operations *get_blkfops(unsigned int major);

void ScanDiskDevices(void)
{
    register kdev_t dev;
    register ULONG size;
    register ULONG j, i, retCode, len, major, minor;
    struct hd_geometry geometry;
    mm_segment_t old_fs;
    BYTE *Sector;

    Sector = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Sector)
    {
       NWFSPrint("nwfs:  allocation error in AddDiskDevices\n");
       return;
    }

    for (FirstValidDisk = (ULONG)-1, j = 0; j < MAX_DISKS; j++)
    {
       if (!SystemDisk[j])
       {
          if (!DiskDevices[j])
             break;

	  dev = DiskDevices[j];
          major = MAJOR(dev);
          minor = MINOR(dev);

          // search the gendisk head and see if this device exists
          if (!LocateDevice(dev))
             continue;

	  SystemDisk[j] = (NWDISK *) NWFSAlloc(sizeof(NWDISK), NWDISK_TAG);
	  if (!SystemDisk[j])
	  {
	     NWFSPrint("nwfs: memory allocation failure in AddDiskDevices\n");
	     continue;
	  }
	  NWFSSet(SystemDisk[j], 0, sizeof(NWDISK));

          SystemDisk[j]->inode.i_rdev = dev;
          retCode = blkdev_open(&SystemDisk[j]->inode, &SystemDisk[j]->filp);
          if (retCode)
          {
             if (SystemDisk[j])
	        NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
             continue;
          }

	  if ((!SystemDisk[j]->filp.f_op) || (!SystemDisk[j]->filp.f_op->ioctl))
          {
             if (SystemDisk[j])
	        NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
	     continue;
          }

          // perform a get drive geometry call to determine if it's a hard
          // disk or CDROM

#if (LINUX_22 | LINUX_24)
          lock_kernel();
#endif
          old_fs = get_fs();
          set_fs(KERNEL_DS);
	  retCode = SystemDisk[j]->filp.f_op->ioctl(&SystemDisk[j]->inode,
			                   0, HDIO_GETGEO, (ULONG)&geometry);
          set_fs(old_fs);
#if (LINUX_22 | LINUX_24)
          unlock_kernel();
#endif

	  if (retCode)
          {
             if (SystemDisk[j])
	        NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
	     continue;
          }

	  set_blocksize(dev, LogicalBlockSize);

          if (FirstValidDisk == (ULONG)-1)
             FirstValidDisk = j;

	  SystemDisk[j]->DiskNumber = j;
	  SystemDisk[j]->DeviceBlockSize = LogicalBlockSize;
          SystemDisk[j]->Cylinders = (LONGLONG) geometry.cylinders;
          SystemDisk[j]->TracksPerCylinder = geometry.heads;
          SystemDisk[j]->SectorsPerTrack = geometry.sectors;
	  SystemDisk[j]->PhysicalDiskHandle = (void *)((ULONG)dev);
	  SystemDisk[j]->BytesPerSector = hardsect_size[major] ? hardsect_size[major][minor] : 512;

	  size = (blk_size[major] ? (ULONG) blk_size[major][minor] : (ULONG) blk_size[major]);
	  SystemDisk[j]->driveSize = (LONGLONG)((LONGLONG) size *
				     (LONGLONG) SystemDisk[j]->BytesPerSector);

	  TotalDisks++;
	  if (TotalDisks > MaximumDisks)
	     MaximumDisks = TotalDisks;

       }

       retCode = ReadDiskSectors(j, 0, Sector,
				 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
				 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
       if (!retCode)
       {
	  NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices\n", (int)j);
	  continue;
       }

       len = SystemDisk[j]->BytesPerSector;

       NWFSCopy(&SystemDisk[j]->partitionSignature,
		   &Sector[0x01FE], 2);

       if (SystemDisk[j]->partitionSignature != 0xAA55)
       {
#if (VERBOSE)
	  NWFSPrint("nwfs:  partition signature 0xAA55 not found disk(%d)\n", j);
#endif
	  NWFSSet(&SystemDisk[j]->PartitionTable[0].fBootable, 0, 64);
	  continue;
       }
       else
       {
	  NWFSCopy(&SystemDisk[j]->PartitionTable[0].fBootable,
		   &Sector[0x01BE], 64);
       }

       // scan for Netware Partitions and detect 3.x and 4.x/5.x partition types
       for (i=0, SystemDisk[j]->NumberOfPartitions = 0; i < 4; i++)
       {
	  if (SystemDisk[j]->PartitionTable[i].nSectorsTotal)
             SystemDisk[j]->NumberOfPartitions++;

	  if (SystemDisk[j]->PartitionTable[i].SysFlag == NETWARE_386_ID)
	  {
	     retCode = ReadDiskSectors(j,
			  SystemDisk[j]->PartitionTable[i].StartLBA,
			  Sector,
			  IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
			  IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
	     if (!retCode)
	     {
		NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices (part)\n", (int)j);
		continue;
	     }

	     if (!NWFSCompare(Sector, NwPartSignature, 16))
		SystemDisk[j]->PartitionVersion[i] = NW4X_PARTITION;
	     else
		SystemDisk[j]->PartitionVersion[i] = NW3X_PARTITION;
	  }
       }
    }
    NWFSFree(Sector);
    return;
}

#endif

#if (LINUX_20)

extern ULONG MaxDisksSupported;
extern ULONG MaxHandlesSupported;

void RemoveDiskDevices(void)
{
    register ULONG j;

    for (j=0; j < MAX_DISKS; j++)
    {
       if (SystemDisk[j])
       {
	  SyncDevice((ULONG)SystemDisk[j]->PhysicalDiskHandle);

          if (SystemDisk[j]->filp.f_op)
             blkdev_release(&SystemDisk[j]->inode);

	  NWFSFree(SystemDisk[j]);
	  SystemDisk[j] = 0;
	  if (TotalDisks)
	     TotalDisks--;
       }
    }
}

extern struct blk_dev_struct blk_dev[MAX_BLKDEV];
extern int *blk_size[MAX_BLKDEV];
extern int *blksize_size[MAX_BLKDEV];
extern int *hardsect_size[MAX_BLKDEV];

void ScanDiskDevices(void)
{
    register kdev_t dev;
    register ULONG size;
    register ULONG j, i, retCode, len, major, minor;
    struct hd_geometry geometry;
    unsigned short old_fs;
    BYTE *Sector;

    Sector = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Sector)
    {
       NWFSPrint("nwfs:  allocation error in AddDiskDevices\n");
       return;
    }

    for (FirstValidDisk = (ULONG)-1, j = 0; j < MAX_DISKS; j++)
    {
       if (!SystemDisk[j])
       {
          if (!DiskDevices[j])
             break;

	  dev = DiskDevices[j];
          major = MAJOR(dev);
          minor = MINOR(dev);

          // search the gendisk head and see if this device exists
          if (!LocateDevice(dev))
             continue;

	  SystemDisk[j] = (NWDISK *) NWFSAlloc(sizeof(NWDISK), NWDISK_TAG);
	  if (!SystemDisk[j])
	  {
	     NWFSPrint("nwfs: memory allocation failure in AddDiskDevices\n");
	     continue;
	  }
	  NWFSSet(SystemDisk[j], 0, sizeof(NWDISK));

          SystemDisk[j]->inode.i_rdev = dev;
          retCode = blkdev_open(&SystemDisk[j]->inode, &SystemDisk[j]->filp);

          if (retCode)
          {
             if (SystemDisk[j])
	        NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
             continue;
          }

	  if ((!SystemDisk[j]->filp.f_op) || (!SystemDisk[j]->filp.f_op->ioctl))
          {
             if (SystemDisk[j])
	        NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
	     continue;
          }

          // perform a get drive geometry call to determine if it's a hard
          // disk or CDROM

#if (LINUX_22 | LINUX_24)
          lock_kernel();
#endif
          old_fs = get_fs();
          set_fs(KERNEL_DS);
	  retCode = SystemDisk[j]->filp.f_op->ioctl(&SystemDisk[j]->inode,
			                   0, HDIO_GETGEO, (ULONG)&geometry);
          set_fs(old_fs);
#if (LINUX_22 | LINUX_24)
          unlock_kernel();
#endif

	  if (retCode)
          {
             if (SystemDisk[j])
	        NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
	     continue;
          }

	  set_blocksize(dev, LogicalBlockSize);

          if (FirstValidDisk == (ULONG)-1)
             FirstValidDisk = j;

	  SystemDisk[j]->DiskNumber = j;
	  SystemDisk[j]->DeviceBlockSize = LogicalBlockSize;
          SystemDisk[j]->Cylinders = (LONGLONG) geometry.cylinders;
          SystemDisk[j]->TracksPerCylinder = geometry.heads;
          SystemDisk[j]->SectorsPerTrack = geometry.sectors;
	  SystemDisk[j]->PhysicalDiskHandle = (void *)((ULONG)dev);
	  SystemDisk[j]->BytesPerSector = hardsect_size[major] ? hardsect_size[major][minor] : 512;

	  size = (blk_size[major] ? (ULONG) blk_size[major][minor] : (ULONG) blk_size[major]);
	  SystemDisk[j]->driveSize = (LONGLONG)((LONGLONG) size *
				     (LONGLONG) SystemDisk[j]->BytesPerSector);

	  TotalDisks++;
	  if (TotalDisks > MaximumDisks)
	     MaximumDisks = TotalDisks;

       }

       retCode = ReadDiskSectors(j, 0, Sector,
				 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
				 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
       if (!retCode)
       {
	  NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices\n", (int)j);
	  continue;
       }

       len = SystemDisk[j]->BytesPerSector;

       NWFSCopy(&SystemDisk[j]->partitionSignature,
		   &Sector[0x01FE], 2);

       if (SystemDisk[j]->partitionSignature != 0xAA55)
       {
#if (VERBOSE)
	  NWFSPrint("nwfs:  partition signature 0xAA55 not found disk(%d)\n", (int)j);
#endif
	  NWFSSet(&SystemDisk[j]->PartitionTable[0].fBootable, 0, 64);
	  continue;
       }
       else
       {
	  NWFSCopy(&SystemDisk[j]->PartitionTable[0].fBootable,
		   &Sector[0x01BE], 64);
       }

       // scan for Netware Partitions and detect 3.x and 4.x/5.x partition types
       for (i=0, SystemDisk[j]->NumberOfPartitions = 0; i < 4; i++)
       {
	  if (SystemDisk[j]->PartitionTable[i].nSectorsTotal)
             SystemDisk[j]->NumberOfPartitions++;

	  if (SystemDisk[j]->PartitionTable[i].SysFlag == NETWARE_386_ID)
	  {
	     retCode = ReadDiskSectors(j,
			  SystemDisk[j]->PartitionTable[i].StartLBA,
			  Sector,
			  IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
			  IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
	     if (!retCode)
	     {
		NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices (part)\n", (int)j);
		continue;
	     }

	     if (!NWFSCompare(Sector, NwPartSignature, 16))
		SystemDisk[j]->PartitionVersion[i] = NW4X_PARTITION;
	     else
		SystemDisk[j]->PartitionVersion[i] = NW3X_PARTITION;
	  }
       }
    }
    NWFSFree(Sector);
    return;
}

#endif


#if (LINUX_UTIL)

extern BYTE *PhysicalDiskNames[];
extern ULONG MaxDisksSupported;
extern ULONG MaxHandlesSupported;

void RemoveDiskDevices(void)
{
    register ULONG j;

    for (j=0; j < MAX_DISKS; j++)
    {
       if (SystemDisk[j])
       {
	  SyncDevice((ULONG)SystemDisk[j]->PhysicalDiskHandle);

	  close((int)SystemDisk[j]->PhysicalDiskHandle);

	  NWFSFree(SystemDisk[j]);
	  SystemDisk[j] = 0;
	  if (TotalDisks)
	     TotalDisks--;
       }
    }
}

void ScanDiskDevices(void)
{
    register ULONG j, i, retCode, len;
    struct hd_geometry geometry;
    BYTE *Sector;

    Sector = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Sector)
    {
       NWFSPrint("nwfs:  allocation error in AddDiskDevices\n");
       return;
    }

    for (FirstValidDisk = (ULONG)-1, j = 0; j < MAX_DISKS; j++)
    {
       if (!PhysicalDiskNames[j])
          break;

       if (!SystemDisk[j])
       {
	  SystemDisk[j] = (NWDISK *) NWFSAlloc(sizeof(NWDISK), NWDISK_TAG);
	  if (!SystemDisk[j])
	  {
	     NWFSPrint("nwfs: memory allocation failure in AddDiskDevices\n");
	     continue;
	  }
	  NWFSSet(SystemDisk[j], 0, sizeof(NWDISK));
       }

       if (!SystemDisk[j]->PhysicalDiskHandle)
       {
	  SystemDisk[j]->PhysicalDiskHandle = (void *)open(PhysicalDiskNames[j], O_RDWR);
	  if ((long) SystemDisk[j]->PhysicalDiskHandle < 0)
	  {
	     NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
	     continue;
	  }

	  SystemDisk[j]->DiskNumber = j;
       }

#ifdef HDIO_REQ
       retCode = ioctl((int)SystemDisk[j]->PhysicalDiskHandle, HDIO_REQ,
		       &geometry);
#else
       retCode = ioctl((int)SystemDisk[j]->PhysicalDiskHandle, HDIO_GETGEO,
		       &geometry);
#endif
       if ((retCode == -EINVAL) || (retCode == -ENODEV))
       {
	  close((int)SystemDisk[j]->PhysicalDiskHandle);
	  NWFSFree(SystemDisk[j]);
	  SystemDisk[j] = 0;
	  continue;
       }

       if (FirstValidDisk == (ULONG)-1)
          FirstValidDisk = j;

       SystemDisk[j]->BytesPerSector = 512;
       SystemDisk[j]->Cylinders = (LONGLONG) geometry.cylinders;
       SystemDisk[j]->TracksPerCylinder = geometry.heads;
       SystemDisk[j]->SectorsPerTrack = geometry.sectors;
       SystemDisk[j]->driveSize = (LONGLONG)
		 (geometry.cylinders * geometry.heads *
		  geometry.sectors * SystemDisk[j]->BytesPerSector);

       TotalDisks++;
       if (TotalDisks > MaximumDisks)
	  MaximumDisks = TotalDisks;

       retCode = ReadDiskSectors(j, 0, Sector,
				 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
				 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
       if (!retCode)
       {
	  NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices\n", (int)j);
	  continue;
       }

       len = SystemDisk[j]->BytesPerSector;

       NWFSCopy(&SystemDisk[j]->partitionSignature,
		   &Sector[0x01FE], 2);

       if (SystemDisk[j]->partitionSignature != 0xAA55)
       {
#if (VERBOSE)
	  NWFSPrint("nwfs:  partition signature 0xAA55 not found disk(%d)\n", (int)j);
#endif
	  NWFSSet(&SystemDisk[j]->PartitionTable[0].fBootable, 0, 64);
	  continue;
       }
       else
       {
	  NWFSCopy(&SystemDisk[j]->PartitionTable[0].fBootable,
		   &Sector[0x01BE], 64);
       }

       // scan for Netware Partitions and detect 3.x and 4.x/5.x partition types
       for (i=0, SystemDisk[j]->NumberOfPartitions = 0; i < 4; i++)
       {
	  if (SystemDisk[j]->PartitionTable[i].nSectorsTotal)
             SystemDisk[j]->NumberOfPartitions++;

	  if (SystemDisk[j]->PartitionTable[i].SysFlag == NETWARE_386_ID)
	  {
	     retCode = ReadDiskSectors(j,
			  SystemDisk[j]->PartitionTable[i].StartLBA,
			  Sector,
			  IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
			  IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
	     if (!retCode)
	     {
		NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices (part)\n", (int)j);
		continue;
	     }

	     if (!NWFSCompare(Sector, NwPartSignature, 16))
		SystemDisk[j]->PartitionVersion[i] = NW4X_PARTITION;
	     else
		SystemDisk[j]->PartitionVersion[i] = NW3X_PARTITION;
	  }
       }
    }
    NWFSFree(Sector);
    return;
}

#endif


#if (WINDOWS_NT | WINDOWS_NT_RO)

void RemoveDiskDevices(void)
{
    register ULONG j;

    for (j=0; j < MAX_DISKS; j++)
    {
       if (SystemDisk[j])
       {
	  SyncDevice((ULONG)SystemDisk[j]->PhysicalDiskHandle);
	  NWFSFree(SystemDisk[j]);
	  SystemDisk[j] = 0;

	  if (TotalDisks)
	     TotalDisks--;
       }
    }
}

PDISK_ARRAY PartitionArray = 0;

void ScanDiskDevices(void)
{
    register ULONG size;
    register ULONG j, retCode, len, i;
    register BYTE *Sector;
    register void **DiskHandles, *lastDisk;
    register ULONG DiskCount;
    register ULONG PartitionPointerIndex = 0;

    if (!PartitionArray)
       return;

    Sector = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Sector)
       return;

    DiskHandles = NWFSCacheAlloc((sizeof(void *) * 256), DISKBUF_TAG);
    if (!DiskHandles)
    {
       NWFSFree(Sector);
       return;
    }

    // build a table of valid partition handles
    for (DiskCount=0, lastDisk=0, j=0; j < PartitionArray->PartitionCount; j++)
    {
       if (PartitionArray->Pointers[j].Partition0Pointer)
       {
	  if (PartitionArray->Pointers[j].Partition0Pointer != lastDisk)
	  {
	     lastDisk = DiskHandles[DiskCount] =
		  PartitionArray->Pointers[j].Partition0Pointer;

#if (VERBOSE)
	     NWFSPrint("disk-%X count-%d\n",
		       (unsigned int)DiskHandles[DiskCount],
		       (int)DiskCount);
#endif

	     DiskCount++;
	  }
       }
    }

    for (FirstValidDisk = (ULONG)-1, j=0; j < DiskCount; j++)
    {
       if (!SystemDisk[j])
       {
	  SystemDisk[j] = (NWDISK *) NWFSAlloc(sizeof(NWDISK), NWDISK_TAG);
	  if (!SystemDisk[j])
	  {
	     DbgPrint("nwfs: memory allocation failure in AddDiskDevices\n");
	     continue;
	  }

          if (FirstValidDisk == (ULONG)-1)
             FirstValidDisk = j;

	  SystemDisk[j]->DiskNumber = j;
	  SystemDisk[j]->PhysicalDiskHandle = DiskHandles[j];
	  SystemDisk[j]->BytesPerSector = 512;

	  retCode = ReadDiskSectors(j, 0, Sector, 1, 1);
	  if (retCode)
	  {
	     len = SystemDisk[j]->BytesPerSector;

	     memmove(&SystemDisk[j]->partitionSignature,
			&Sector[0x01FE], 2);

	     if (SystemDisk[j]->partitionSignature != 0xAA55)
	     {
#if (VERBOSE)
		NWFSPrint("nwfs:  partition signature 0xAA55 not found disk(%d)\n", j);
#endif
		NWFSSet(&SystemDisk[j]->PartitionTable[0].fBootable, 0, 64);
		continue;
	     }
	     else
	     {
		NWFSCopy(&SystemDisk[j]->PartitionTable[0].fBootable,
			 &Sector[0x01BE], 64);
	     }

	     for (i=0, SystemDisk[j]->NumberOfPartitions = 0; i < 4; i++)
	     {
		if (SystemDisk[j]->PartitionTable[i].nSectorsTotal)
		{
                    SystemDisk[j]->NumberOfPartitions++;

		   if (SystemDisk[j]->PartitionTable[i].SysFlag == NETWARE_386_ID)
		   {
		      // NOTE:  it is assumed that the table of
		      // partition contexts passed down to us by the
		      // NWMOUNT utility will match what is really
		      // on the disk drives.  We assume that it does,
		      // and simply enumerate the partition context table
		      // as we detect and process partitions.

		      if (PartitionPointerIndex >= PartitionArray->PartitionCount)
                         break;

		      SystemDisk[j]->PartitionContext[i] =
		         PartitionArray->Pointers[PartitionPointerIndex++].PartitionPointer;
		   }
		}
	     }
	     TotalDisks++;
	  }
	  else
	  {
	     NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
	  }
       }
    }
    NWFSFree(DiskHandles);
    NWFSFree(Sector);
    return;

}

NTSTATUS NwMountAll(PDISK_ARRAY PartitionArray)
{
    register ULONG retCode;

    retCode = MountAllVolumes(FALSE);
    if (retCode)
       return STATUS_UNRECOGNIZED_VOLUME;

    return STATUS_SUCCESS;
}

NTSTATUS NwDismountAll(void)
{
    register ULONG retCode;

    retCode = DismountAllVolumes();
    if (retCode)
       return STATUS_UNRECOGNIZED_VOLUME;

    return STATUS_SUCCESS;
}

#endif

#if (DOS_UTIL)


// check for Int 0x13 Extensions for large drives (> 8 GB)
ULONG Int13ExtensionsPresent(ULONG disk)
{
    static union REGS r;

    r.h.ah = 0x41;
    r.x.bx = 0x55AA;
    r.h.dl = (0x80 | (disk & 0x7F));

    int86(0x13, &r, &r);

    return ((r.h.cflag & 1) ? FALSE : TRUE);

}

void RemoveDiskDevices(void)
{
    register ULONG j;

    for (j=0; j < MAX_DISKS; j++)
    {
       if (SystemDisk[j])
       {
	  if (SystemDisk[j]->RequestSelector)
	     __dpmi_free_dos_memory(SystemDisk[j]->RequestSelector);
	  SystemDisk[j]->RequestSelector = 0;

	  if (SystemDisk[j]->DataSelector)
	     __dpmi_free_dos_memory(SystemDisk[j]->DataSelector);
	  SystemDisk[j]->DataSelector = 0;

	  if (SystemDisk[j]->DriveInfoSelector)
	     __dpmi_free_dos_memory(SystemDisk[j]->DriveInfoSelector);
	  SystemDisk[j]->DriveInfoSelector = 0;

	  SyncDevice((ULONG)SystemDisk[j]->PhysicalDiskHandle);
	  SystemDisk[j]->PhysicalDiskHandle = 0;
	  NWFSFree(SystemDisk[j]);
	  SystemDisk[j] = 0;
	  if (TotalDisks)
	     TotalDisks--;
       }
    }
}

void ScanDiskDevices(void)
{
    register ULONG j, i, retCode;
    BYTE *Sector;
    static union REGS r;
    DRIVE_INFO dinfo;

    Sector = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Sector)
    {
       NWFSPrint("nwfs:  allocation error in AddDiskDevices\n");
       return;
    }

    for (FirstValidDisk = (ULONG)-1, j = 0; j < MAX_DOS_DISKS; j++)
    {

#if (WINDOWS_98_UTIL)
       r.x.ax = 0x440D;    // generic IOCTL
       r.h.bh = 1;         // locks levels 0, 1, 2 or 3
       r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
       r.h.ch = 0x08;      // device category
       r.h.cl = 0x4B;      // lock physical volume
       r.x.dx = 1;         // permissions
       r.h.cflag = 0x0001;

       int86(0x21, &r, &r);
#endif

       // test fixed disk status
       r.h.ah = 0x10;  // test drive status
       r.h.dl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
       int86(0x13, &r, &r);

       // if we got the drive status, and the drive is active,
       // assume the disk is valid.
       if ((!r.h.cflag) && (!r.h.ah))
       {
          // now check if this device supports removable media.  This
	  // would indicate this device is a CDROM drive.  If we detect
          // that this device supports removable meida, then do not
          // add it -- it's most likely a CDROM or tape device and
	  // we should not add it.

          // see if a change line is available for this device.
          r.h.ah = 0x15;  // test drive status
          r.h.dl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
          int86(0x13, &r, &r);

          // 00-no drive, 01-diskette, 02-change line, 03-fixed disk
	  if (r.h.ch == 0x02)
	  {

#if (WINDOWS_98_UTIL)
	     r.x.ax = 0x440D;    // generic IOCTL
	     r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
	     r.h.ch = 0x08;      // device category
	     r.h.cl = 0x6B;      // unlock physical volume
	     int86(0x21, &r, &r);
#endif
	     continue;
	  }

	  if (!SystemDisk[j])
	  {
	     SystemDisk[j] = (NWDISK *) NWFSAlloc(sizeof(NWDISK), NWDISK_TAG);
	     if (!SystemDisk[j])
	     {
		NWFSPrint("nwfs: memory allocation failure in AddDiskDevices\n");

#if (WINDOWS_98_UTIL)
		r.x.ax = 0x440D;    // generic IOCTL
		r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
		r.h.ch = 0x08;      // device category
		r.h.cl = 0x6B;      // unlock physical volume
		int86(0x21, &r, &r);
#endif
		continue;
	     }
	     NWFSSet(SystemDisk[j], 0, sizeof(NWDISK));
	  }

	  SystemDisk[j]->PhysicalDiskHandle = (void *)(0x80 | (j & 0x7F));
	  SystemDisk[j]->DiskNumber = j;
	  SystemDisk[j]->Int13Extensions = Int13ExtensionsPresent(j);

	  SystemDisk[j]->DataSegment =
	       __dpmi_allocate_dos_memory(PARAGRAPH_ALIGN(IO_BLOCK_SIZE),
					  &SystemDisk[j]->DataSelector);
	  if (SystemDisk[j]->DataSegment == (WORD) -1)
	  {
	     SystemDisk[j]->PhysicalDiskHandle = 0;
	     NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;

#if (WINDOWS_98_UTIL)
	     r.x.ax = 0x440D;    // generic IOCTL
	     r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
	     r.h.ch = 0x08;      // device category
	     r.h.cl = 0x6B;      // unlock physical volume
	     int86(0x21, &r, &r);
#endif
	     continue;
	  }

	  SystemDisk[j]->DriveInfoSegment =
	       __dpmi_allocate_dos_memory(PARAGRAPH_ALIGN(IO_BLOCK_SIZE),
					  &SystemDisk[j]->DriveInfoSelector);
	  if (SystemDisk[j]->DriveInfoSegment == (WORD) -1)
	  {
	     if (SystemDisk[j]->DataSelector)
		__dpmi_free_dos_memory(SystemDisk[j]->DataSelector);
	     SystemDisk[j]->DataSelector = 0;

	     SystemDisk[j]->PhysicalDiskHandle = 0;
	     NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;

#if (WINDOWS_98_UTIL)
	     r.x.ax = 0x440D;    // generic IOCTL
	     r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
	     r.h.ch = 0x08;      // device category
	     r.h.cl = 0x6B;      // unlock physical volume
	     int86(0x21, &r, &r);
#endif
	     continue;
	  }

	  SystemDisk[j]->RequestSegment =
	       __dpmi_allocate_dos_memory(PARAGRAPH_ALIGN(sizeof(EXT_REQUEST)),
					  &SystemDisk[j]->RequestSelector);
	  if (SystemDisk[j]->RequestSegment == (WORD) -1)
	  {
	     if (SystemDisk[j]->DataSelector)
		__dpmi_free_dos_memory(SystemDisk[j]->DataSelector);
	     SystemDisk[j]->DataSelector = 0;

	     if (SystemDisk[j]->DriveInfoSelector)
		__dpmi_free_dos_memory(SystemDisk[j]->DriveInfoSelector);
	     SystemDisk[j]->DriveInfoSelector = 0;

	     SystemDisk[j]->PhysicalDiskHandle = 0;
	     NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;

#if (WINDOWS_98_UTIL)
	     r.x.ax = 0x440D;    // generic IOCTL
	     r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
	     r.h.ch = 0x08;      // device category
	     r.h.cl = 0x6B;      // unlock physical volume
	     int86(0x21, &r, &r);
#endif
	     continue;
	  }

	  // if int 13 extensions are present, then adjust drive geometry
	  // based on reported drive info.

	  if (SystemDisk[j]->Int13Extensions)
	  {
	     register ULONG cylinders, heads, sectors;
	     _go32_dpmi_registers r32;

	     NWFSSet(&r32, 0, sizeof(_go32_dpmi_registers));
	     NWFSSet(&dinfo, 0, sizeof(DRIVE_INFO));

	     dinfo.Size = sizeof(DRIVE_INFO);

	     movedata(_my_ds(), (unsigned)&dinfo,
		      SystemDisk[j]->DriveInfoSelector, 0,
		      sizeof(DRIVE_INFO));

	     // get int 13 extension drive info
	     r32.h.ah = 0x48;
	     r32.h.dl = (0x80 | (j & 0x7F));
	     r32.x.si = 0;
	     r32.x.ds = SystemDisk[j]->DriveInfoSegment;

	     _go32_dpmi_simulate_int(0x13, &r32);

	     movedata(SystemDisk[j]->DriveInfoSelector, 0,
		      _my_ds(), (unsigned)&dinfo,
		      sizeof(DRIVE_INFO));

	     // if TotalSectors is 0, then there are no drives
	     // attached to this controller.

	     if (!dinfo.TotalSectors)
	     {
		if (SystemDisk[j]->RequestSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->RequestSelector);
		SystemDisk[j]->RequestSelector = 0;

		if (SystemDisk[j]->DataSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->DataSelector);
		SystemDisk[j]->DataSelector = 0;

		if (SystemDisk[j]->DriveInfoSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->DriveInfoSelector);
		SystemDisk[j]->DriveInfoSelector = 0;

		SystemDisk[j]->PhysicalDiskHandle = 0;
		NWFSFree(SystemDisk[j]);
		SystemDisk[j] = 0;

#if (WINDOWS_98_UTIL)
		r.x.ax = 0x440D;    // generic IOCTL
		r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
		r.h.ch = 0x08;      // device category
		r.h.cl = 0x6B;      // unlock physical volume
		int86(0x21, &r, &r);
#endif
		continue;
	     }

	     r.h.ah = 0x08;   // read drive parameters
	     r.h.dl = (0x80 | (j & 0x7F));

	     int86(0x13, &r, &r);

	     if (r.h.cflag)
	     {
		if (SystemDisk[j]->RequestSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->RequestSelector);
		SystemDisk[j]->RequestSelector = 0;

		if (SystemDisk[j]->DataSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->DataSelector);
		SystemDisk[j]->DataSelector = 0;

		if (SystemDisk[j]->DriveInfoSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->DriveInfoSelector);
		SystemDisk[j]->DriveInfoSelector = 0;

		SystemDisk[j]->PhysicalDiskHandle = 0;
		NWFSFree(SystemDisk[j]);
		SystemDisk[j] = 0;

#if (WINDOWS_98_UTIL)
		r.x.ax = 0x440D;    // generic IOCTL
		r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
		r.h.ch = 0x08;      // device category
		r.h.cl = 0x6B;      // unlock physical volume
		int86(0x21, &r, &r);
#endif
		continue;
	     }

	     // translate drive geometry based on the total sectors
	     // reported by the Int 13 extension procedure.

	     heads = r.h.dh + 1;
	     sectors = ((WORD)r.h.cl & 0x003F);
	     cylinders = (dinfo.TotalSectors / (heads * sectors));

	     // make sure that cylinders are within the maximum value
	     // allowed by the Int 13 Extended Architecture.  At present,
	     // this limit is 16 mega-tera sectors (2^64).

	     // assume the current IDE limits for cylinder count
	     if (cylinders > (ULONG) 65536)
	     {
		if (SystemDisk[j]->RequestSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->RequestSelector);
		SystemDisk[j]->RequestSelector = 0;

		if (SystemDisk[j]->DataSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->DataSelector);
		SystemDisk[j]->DataSelector = 0;

		if (SystemDisk[j]->DriveInfoSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->DriveInfoSelector);
		SystemDisk[j]->DriveInfoSelector = 0;

		SystemDisk[j]->PhysicalDiskHandle = 0;
		NWFSFree(SystemDisk[j]);
		SystemDisk[j] = 0;

#if (WINDOWS_98_UTIL)
		r.x.ax = 0x440D;    // generic IOCTL
		r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
		r.h.ch = 0x08;      // device category
		r.h.cl = 0x6B;      // unlock physical volume
		int86(0x21, &r, &r);
#endif
		continue;
	     }

	     //   Use the Phoenix method of drive translation
	     //   to translate drive geometry for those drives
	     //   that exceed 1024 cylinders in size.
	     //
	     //   Phoenix Geometry Translation Table
	     //   -----------------------------------
	     //
	     //   Phys Cylinders    Phys Heads  Trans Cyl Trans Heads  Max
	     //    --------------------------------------------------------
	     //   1     <C<= 1024    1 <H<= 16   C = C     H = H     528 MB
	     //   1024  <C<= 2048    1 <H<= 16   C = C/2   H = H*2   1.0 GB
	     //   2048  <C<= 4096    1 <H<= 16   C = C/4   H = H*4   2.1 GB
	     //   4096  <C<= 8192    1 <H<= 16   C = C/8   H = H*8   4.2 GB
	     //   8192  <C<= 16384   1 <H<= 16   C = C/16  H = H*16  8.4 GB
	     //   16384 <C<= 32768   1 <H<= 8    C = C/32  H = H*32  8.4 GB
	     //   32768 <C<= 65536   1 <H<= 4    C = C/64  H = H*64  8.4 GB
	     //
	     //   LBA Assisted Translation Table
	     //   ------------------------------
	     //
	     //   (NOTE:  The method below is an alternate method for
	     //   translating large drives that does not place any limits
	     //   on reported drive geometry.  It has the disadvantage
	     //   of always assuming 63 Sectors Per Track.)
	     //
	     //   Range              Sectors  Heads   Cylinders
	     //   -----------------------------------------------------
	     //   1 MB   <X< 528 MB    63       16    X/(63 * 16 * 512)
	     //   528 MB <X< 1.0 GB    63       32    X/(63 * 32 * 512)
	     //   1.0 GB <X< 2.1 GB    63       64    X/(63 * 64 * 512)
	     //   2.1 GB <X< 4.2 GB    63      128    X/(63 * 128 * 512)
	     //   4.2 GB <X< 8.4 GB    63      255    X/(63 * 255 * 512)
	     //

	     // Adjust cylinder and head dimensions until this drive
	     // presents a geometry with a cylinder count that is less
	     // than 1024 or Heads less than or equal to 255.

	     if (cylinders >= 1024)
	     {
		while ((heads * 2) <= 256)
		{
		   heads *= 2;
		   cylinders /= 2;
		   if (cylinders < 1024)
		      break;
		}
	     }

             if (FirstValidDisk == (ULONG)-1)
                FirstValidDisk = j;

	     SystemDisk[j]->BytesPerSector = 512;
	     SystemDisk[j]->Cylinders = (LONGLONG) cylinders;
	     SystemDisk[j]->TracksPerCylinder = heads;
	     SystemDisk[j]->SectorsPerTrack = sectors;
	     SystemDisk[j]->driveSize = (LONGLONG)
			      (SystemDisk[j]->Cylinders *
			       SystemDisk[j]->TracksPerCylinder *
			       SystemDisk[j]->SectorsPerTrack *
			       SystemDisk[j]->BytesPerSector);
#if (VERBOSE)
	     NWFSPrint("disk-%d cyl-%d head-%d sector-%d (Int 13)\n",
		      (int)j,
		      (int)SystemDisk[j]->Cylinders,
		      (int)SystemDisk[j]->TracksPerCylinder,
		      (int)SystemDisk[j]->SectorsPerTrack);
#endif
	  }
	  else
	  {
	     // query the drive parameters with standard bios
	     r.h.ah = 0x08;
	     r.h.dl = (0x80 | (j & 0x7F));

	     int86(0x13, &r, &r);

	     if (r.h.cflag)
	     {
		if (SystemDisk[j]->RequestSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->RequestSelector);
		SystemDisk[j]->RequestSelector = 0;

		if (SystemDisk[j]->DataSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->DataSelector);
		SystemDisk[j]->DataSelector = 0;

		if (SystemDisk[j]->DriveInfoSelector)
		   __dpmi_free_dos_memory(SystemDisk[j]->DriveInfoSelector);
		SystemDisk[j]->DriveInfoSelector = 0;

		SystemDisk[j]->PhysicalDiskHandle = 0;
		NWFSFree(SystemDisk[j]);
		SystemDisk[j] = 0;

#if (WINDOWS_98_UTIL)
		r.x.ax = 0x440D;    // generic IOCTL
		r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
		r.h.ch = 0x08;      // device category
		r.h.cl = 0x6B;      // unlock physical volume
		int86(0x21, &r, &r);
#endif
		continue;
	     }

             if (FirstValidDisk == (ULONG)-1)
                FirstValidDisk = j;

	     SystemDisk[j]->BytesPerSector = 512;
	     SystemDisk[j]->Cylinders =
		 (LONGLONG)(((WORD)((WORD)r.h.cl << 2) & 0x0300) |
			    (WORD)r.h.ch) + 1;
	     SystemDisk[j]->TracksPerCylinder = (r.h.dh + 1);
	     SystemDisk[j]->SectorsPerTrack = ((WORD)r.h.cl & 0x003F);
	     SystemDisk[j]->driveSize = (LONGLONG)
			      (SystemDisk[j]->Cylinders *
			       SystemDisk[j]->TracksPerCylinder *
			       SystemDisk[j]->SectorsPerTrack *
			       SystemDisk[j]->BytesPerSector);
#if (VERBOSE)
	     NWFSPrint("disk-%d cyl-%d head-%d sector-%d\n",
		      (int)j,
		      (int)SystemDisk[j]->Cylinders,
		      (int)SystemDisk[j]->TracksPerCylinder,
		      (int)SystemDisk[j]->SectorsPerTrack);
#endif
	  }

#if (WINDOWS_98_UTIL)
	  r.x.ax = 0x440D;    // generic IOCTL
	  r.h.bl = (0x80 | (j & 0x7F)); // dos limit is 128 drives
	  r.h.ch = 0x08;      // device category
	  r.h.cl = 0x6B;      // unlock physical volume
	  int86(0x21, &r, &r);
#endif
	  TotalDisks++;
	  if (TotalDisks > MaximumDisks)
	     MaximumDisks = TotalDisks;

	  retCode = ReadDiskSectors(j, 0, Sector,
			 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
			 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
	  if (!retCode)
	  {
	     NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices\n", (int)j);
	     continue;
	  }

	  NWFSCopy(&SystemDisk[j]->partitionSignature, &Sector[0x01FE], 2);

	  if (SystemDisk[j]->partitionSignature != 0xAA55)
	  {
#if (VERBOSE)
	     NWFSPrint("nwfs:  partition signature 0xAA55 not found disk(%d)\n", (int)j);
#endif
	     NWFSSet(&SystemDisk[j]->PartitionTable[0].fBootable, 0, 64);
	     continue;
	  }
	  else
	  {
	     NWFSCopy(&SystemDisk[j]->PartitionTable[0].fBootable,
		      &Sector[0x01BE], 64);
	  }

	  // scan for Netware Partitions and detect 3.x and 4.x/5.x partition types
	  for (i=0, SystemDisk[j]->NumberOfPartitions = 0; i < 4; i++)
	  {
	     if (SystemDisk[j]->PartitionTable[i].nSectorsTotal)
                SystemDisk[j]->NumberOfPartitions++;            

	     if (SystemDisk[j]->PartitionTable[i].SysFlag == NETWARE_386_ID)
	     {
		retCode = ReadDiskSectors(j,
			     SystemDisk[j]->PartitionTable[i].StartLBA,
			     Sector,
			     IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
			     IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
		if (!retCode)
		{
		   NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices (part)\n", (int)j);
		   continue;
		}

		if (!NWFSCompare(Sector, NwPartSignature, 16))
		   SystemDisk[j]->PartitionVersion[i] = NW4X_PARTITION;
		else
		   SystemDisk[j]->PartitionVersion[i] = NW3X_PARTITION;
	     }
	  }
       }
    }
    NWFSFree(Sector);
    return;
}

#endif

#if (WINDOWS_NT_UTIL)

BYTE PhysicalDiskName[255] = "";

void RemoveDiskDevices(void)
{
    register ULONG j;

    for (j=0; j < MAX_DISKS; j++)
    {
       if (SystemDisk[j])
       {
	  SyncDevice((ULONG)SystemDisk[j]->PhysicalDiskHandle);
	  CloseHandle(SystemDisk[j]->PhysicalDiskHandle);
	  NWFSFree(SystemDisk[j]);
	  SystemDisk[j] = 0;
	  if (TotalDisks)
	     TotalDisks--;
       }
    }
}

void ScanDiskDevices(void)
{
    register ULONG j, i, retCode, len;
    DISK_GEOMETRY geometry;
    BYTE *Sector;
    ULONG bytesReturned = 0;

    Sector = NWFSIOAlloc(IO_BLOCK_SIZE, DISKBUF_TAG);
    if (!Sector)
    {
       NWFSPrint("nwfs:  allocation error in AddDiskDevices\n");
       return;
    }

    for (FirstValidDisk = (ULONG)-1, j = 0; j < MAX_DISKS; j++)
    {
       if (!SystemDisk[j])
       {
	  SystemDisk[j] = (NWDISK *) NWFSAlloc(sizeof(NWDISK), NWDISK_TAG);
	  if (!SystemDisk[j])
	  {
	     NWFSPrint("nwfs: memory allocation failure in AddDiskDevices\n");
	     continue;
	  }
	  NWFSSet(SystemDisk[j], 0, sizeof(NWDISK));
       }

       if (!SystemDisk[j]->PhysicalDiskHandle)
       {
	  sprintf(PhysicalDiskName, "\\\\.\\PhysicalDrive%d", (int)j);
	  SystemDisk[j]->PhysicalDiskHandle = (void *) FileHandleOpen(PhysicalDiskName);
	  if (!SystemDisk[j]->PhysicalDiskHandle)
	  {
	     NWFSFree(SystemDisk[j]);
	     SystemDisk[j] = 0;
	     continue;
	  }
	  SystemDisk[j]->DiskNumber = j;
       }

       retCode = DeviceIoControl(SystemDisk[j]->PhysicalDiskHandle,
				 IOCTL_DISK_GET_DRIVE_GEOMETRY,
				 0,
				 0,
				 &geometry,
				 sizeof(DISK_GEOMETRY),
				 &bytesReturned,
				 0);

       if (!retCode)
       {
#if (VERBOSE)
	  NWFSPrint("nwfs:  could not get disk geometry (%d)\n", j);
#endif
	  CloseHandle(SystemDisk[j]->PhysicalDiskHandle);
	  NWFSFree(SystemDisk[j]);
	  SystemDisk[j] = 0;
	  continue;
       }

       if (FirstValidDisk == (ULONG)-1)
          FirstValidDisk = j;

       SystemDisk[j]->BytesPerSector = geometry.BytesPerSector;
       SystemDisk[j]->Cylinders = (LONGLONG) geometry.Cylinders.QuadPart;
       SystemDisk[j]->TracksPerCylinder = geometry.TracksPerCylinder;
       SystemDisk[j]->SectorsPerTrack = geometry.SectorsPerTrack;
       SystemDisk[j]->driveSize = (LONGLONG)
			      (geometry.Cylinders.QuadPart *
			       geometry.TracksPerCylinder *
			       geometry.SectorsPerTrack *
			       geometry.BytesPerSector);

       TotalDisks++;
       if (TotalDisks > MaximumDisks)
	  MaximumDisks = TotalDisks;

       retCode = ReadDiskSectors(j, 0, Sector,
				 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
				 IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
       if (!retCode)
       {
	  NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices\n", (int)j);
	  continue;
       }

       len = (ULONG) SystemDisk[j]->BytesPerSector;

       NWFSCopy(&SystemDisk[j]->partitionSignature,
		   &Sector[0x01FE], 2);

       if (SystemDisk[j]->partitionSignature != 0xAA55)
       {
#if (VERBOSE)
	  NWFSPrint("nwfs:  partition signature 0xAA55 not found disk(%d)\n", (int)j);
#endif
	  NWFSSet(&SystemDisk[j]->PartitionTable[0].fBootable, 0, 64);
	  continue;
       }
       else
       {
	  NWFSCopy(&SystemDisk[j]->PartitionTable[0].fBootable,
		   &Sector[0x01BE], 64);
       }

       // scan for Netware Partitions and detect 3.x and 4.x/5.x partition types
       for (i=0, SystemDisk[j]->NumberOfPartitions = 0; i < 4; i++)
       {
	  if (SystemDisk[j]->PartitionTable[i].SysFlag == NETWARE_386_ID)
	  {
	     retCode = ReadDiskSectors(j,
			  SystemDisk[j]->PartitionTable[i].StartLBA,
			  Sector,
			  IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector,
			  IO_BLOCK_SIZE / SystemDisk[j]->BytesPerSector);
	     if (!retCode)
	     {
		NWFSPrint("nwfs:  disk-%d read error in ScanDiskDevices (part)\n", (int)j);
		continue;
	     }

	     if (!NWFSCompare(Sector, NwPartSignature, 16))
		SystemDisk[j]->PartitionVersion[i] = NW4X_PARTITION;
	     else
		SystemDisk[j]->PartitionVersion[i] = NW3X_PARTITION;
	  }
       }
    }
    NWFSFree(Sector);
    return;
}

#endif
#endif
