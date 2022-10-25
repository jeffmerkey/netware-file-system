
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
*   .  New releases, patches, bug fixes, and
*   technical documentation can be found at repo.icapsql.com.  We will
*   periodically post new releases of this software to repo.icapsql.com
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
*   FILE     :  NWPART.C
*   DESCRIP  :   Netware Partition Module
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#include "globals.h"

ULONG segment_table_count;
segment_info_table segment_table[256];
ULONG segment_mark_table[256];
ULONG hotfix_table_count;
hotfix_info_table hotfix_table[256];

BYTE NwPartSignature[16] = { 0, 'N', 'w', '_', 'P', 'a', 'R', 't', 'I',
			     't', 'I', 'o', 'N', 0, 0, 0 };

BYTE NetwareBootSector[512] =
{
   0xFA, 0xEB, 0x74, 0x43, 0x4F, 0x50, 0x59, 0x52,
   0x49, 0x47, 0x48, 0x54, 0x20, 0x28, 0x43, 0x29,
   0x20, 0x32, 0x30, 0x30, 0x30, 0x20, 0x54, 0x52,
   0x47, 0x2C, 0x20, 0x49, 0x4E, 0x43, 0x20, 0x00,
   0x1A, 0x54, 0x52, 0x47, 0x00, 0x1A, 0x18, 0x49,
   0x4E, 0x56, 0x41, 0x4C, 0x49, 0x44, 0x20, 0x50,
   0x41, 0x52, 0x54, 0x49, 0x54, 0x49, 0x4F, 0x4E,
   0x20, 0x54, 0x41, 0x42, 0x4C, 0x45, 0x00, 0x4D,
   0x49, 0x53, 0x53, 0x49, 0x4E, 0x47, 0x20, 0x4F,
   0x50, 0x45, 0x52, 0x41, 0x54, 0x49, 0x4E, 0x47,
   0x20, 0x53, 0x59, 0x53, 0x54, 0x45, 0x4D, 0x00,
   0x45, 0x52, 0x52, 0x4F, 0x52, 0x20, 0x4C, 0x4F,
   0x41, 0x44, 0x49, 0x4E, 0x47, 0x20, 0x4F, 0x50,
   0x45, 0x52, 0x41, 0x54, 0x49, 0x4E, 0x47, 0x20,
   0x53, 0x59, 0x53, 0x54, 0x45, 0x4D, 0x00, 0x33,
   0xC0, 0x8E, 0xC0, 0x8E, 0xD8, 0x8E, 0xD0, 0xBC,
   0x00, 0x7C, 0xFB, 0xBE, 0x00, 0x7C, 0xBF, 0x00,
   0x06, 0xB9, 0x00, 0x02, 0xFC, 0xF3, 0xA4, 0xEA,
   0x94, 0x06, 0x00, 0x00, 0xB9, 0x04, 0x00, 0xBE,
   0xBE, 0x07, 0x80, 0x3C, 0x80, 0x74, 0x16, 0x83,
   0xC6, 0x10, 0xE2, 0xF6, 0xBE, 0x27, 0x06, 0xAC,
   0x0A, 0xC0, 0x74, 0x06, 0xB4, 0x0E, 0xCD, 0x10,
   0xEB, 0xF5, 0xFB, 0xEB, 0xFE, 0x8B, 0xFE, 0x49,
   0x74, 0x0D, 0x83, 0xC6, 0x10, 0x80, 0x3C, 0x80,
   0x75, 0xF5, 0xBE, 0x27, 0x06, 0xEB, 0xE0, 0xBE,
   0x05, 0x00, 0x8B, 0x15, 0x8B, 0x4D, 0x02, 0xBB,
   0x00, 0x7C, 0xB8, 0x01, 0x02, 0xCD, 0x13, 0x73,
   0x0C, 0x33, 0xC0, 0xCD, 0x13, 0x4E, 0x75, 0xEF,
   0xBE, 0x58, 0x06, 0xEB, 0xC2, 0x59, 0x81, 0x3E,
   0xFE, 0x7D, 0x55, 0xAA, 0x74, 0x05, 0xBE, 0x3F,
   0x06, 0xEB, 0xB4, 0x8B, 0xF7, 0xEA, 0x00, 0x7C,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void SyncDisks(void)
{
#if (LINUX_UTIL)
    register ULONG j;

    for (j=0; j < MAX_DISKS; j++)
    {
       if (SystemDisk[j] && SystemDisk[j]->PhysicalDiskHandle)
	  SyncDevice((ULONG)SystemDisk[j]->PhysicalDiskHandle);
    }
#endif
}

BYTE *get_block_descrip(ULONG Type)
{
    switch(Type)
    {
       case FOUR_K_BLOCK:       return "4K ClusterSize";
       case EIGHT_K_BLOCK:      return "8K ClusterSize";
       case SIXTEEN_K_BLOCK:    return "16K ClusterSize";
       case THIRTYTWO_K_BLOCK:  return "32K ClusterSize";
       case SIXTYFOUR_K_BLOCK:  return "64K ClusterSize";
       default:                 return "??? ClusterSize";
    }
}

ULONG get_block_value(ULONG Type)
{
    switch(Type)
    {
       case FOUR_K_BLOCK:       return 4096;
       case EIGHT_K_BLOCK:      return 8192;
       case SIXTEEN_K_BLOCK:    return 16384;
       case THIRTYTWO_K_BLOCK:  return 32768;
       case SIXTYFOUR_K_BLOCK:  return 65536;
       default:                 return 0;
    }
}

// hotfix tables are laid out at 20 sector intervals
// starting at offset 20 (dec) from the start of a Netware partition
// mirroring tables follow the same layout rules, except these tables
// start at sector 21 (dec) from the start of the partition and continue
// at 20 sector intervals.  There are four copies of these tables.
//
// The current layout of hotfixing is in fact a remnant of the days
// of legacy disk drives.  In the early days of PC architecture devices,
// using this layout guaranteed that at least one copy of these tables
// would be recoverable because this addressing insured that multiple
// copies of the hot fix and mirroring tables would reside on separate
// spindles of an HDD device.  Today, drives are all translated and
// sector:head:cylinder addressing is now logical rather than physical.
// Modern HDD devices also provide internal hot fix support for
// redirection of defective sectors.  Hotfixing does, however, still
// provide an excellent mechanism for performing sector error
// redirection as an internal file system component for those systems
// with older or less sophisticated hardware, and may on some
// manufacturers hardware provide some benefit.  With most modern
// disk devices, however, hotfixing just wastes disk space.
//

void FreePartitionResources(void)
{
    register ULONG i;
    extern void displayMemoryList(void);
    extern void free_bh_list(void);
    extern BYTE *LRUBlockHash;
    extern ULONG LRUBlockHashLimit;

    for (i=0; i < MAX_VOLUMES; i++)
    {
       if (VolumeTable[i])
	  NWFSFree(VolumeTable[i]);
       VolumeTable[i] = 0;
    }

    if (LRUBlockHash)
       NWFSFree(LRUBlockHash);
    LRUBlockHash = 0;
    LRUBlockHashLimit = 0;

    FreeLRU();

    if (ZeroBuffer)
       NWFSFree(ZeroBuffer);
    ZeroBuffer = 0;

    // release hash node free list
    FreeHashNodes();

#if (LINUX_20 | LINUX_22 | LINUX_24)
    // release IO buffer heads
    free_bh_list();
#endif

    if (MemoryAllocated != MemoryFreed)
    {
       NWFSPrint("nwfs:  %d bytes of memory leaked!!!  alloc-%d free-%d\n",
		(int)(MemoryAllocated - MemoryFreed),
		(int)MemoryAllocated, (int)MemoryFreed);
       displayMemoryList();
    }
    else
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  physical memory  allocated-%d freed-%d inuse-%d\n",
		(int)MemoryAllocated, (int)MemoryFreed, (int)MemoryInUse);
#endif
    }

    if (PagedMemoryAllocated != PagedMemoryFreed)
    {
       NWFSPrint("nwfs:  %d bytes of paged memory leaked!!!  alloc-%d free-%d\n",
		(int)(PagedMemoryAllocated - PagedMemoryFreed),
		(int)PagedMemoryAllocated, (int)PagedMemoryFreed);
       displayMemoryList();
    }
    else
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  virtual memory   allocated-%d freed-%d inuse-%d\n",
		(int)PagedMemoryAllocated, (int)PagedMemoryFreed,
		(int)PagedMemoryInUse);
#endif
    }
    return;

}

