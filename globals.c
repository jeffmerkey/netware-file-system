
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
*   FILE     :  GLOBALS.C
*   DESCRIP  :   Global Code Variables
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#include "globals.h"

// memory tracking labels

TRACKING HOTFIX_TRACKING         =  { "HTFX", 0, 0 };
TRACKING HOTFIX_DATA_TRACKING    =  { "HFDT", 0, 0 };
TRACKING HOTFIX_TABLE_TRACKING   =  { "HFTB", 0, 0 };
TRACKING MIRROR_TRACKING         =  { "MIRR", 0, 0 };
TRACKING MIRROR_DATA_TRACKING    =  { "MIRD", 0, 0 };
TRACKING HASH_TRACKING           =  { "HASH", 0, 0 };
TRACKING EXT_HASH_TRACKING       =  { "EHSH", 0, 0 };
TRACKING BITBLOCK_TRACKING       =  { "BITB", 0, 0 };
TRACKING BITBUFFER_TRACKING      =  { "BBUF", 0, 0 };
TRACKING NWPART_TRACKING         =  { "PART", 0, 0 };
TRACKING NWDISK_TRACKING         =  { "DISK", 0, 0 };
TRACKING DISKBUF_TRACKING        =  { "DSKB", 0, 0 };
TRACKING TABLE_TRACKING          =  { "TBLE", 0, 0 };
TRACKING FAT_TRACKING            =  { "FATM", 0, 0 };
TRACKING FATLRU_TRACKING         =  { "FATL", 0, 0 };
TRACKING FATHASH_TRACKING        =  { "FATH", 0, 0 };
TRACKING DIRLRU_TRACKING         =  { "DIRL", 0, 0 };
TRACKING DIRHASH_TRACKING        =  { "DIRH", 0, 0 };
TRACKING DIR_TRACKING            =  { "DIRM", 0, 0 };
TRACKING DIR_BLOCK_TRACKING      =  { "DIRB", 0, 0 };
TRACKING DIR_BLOCKHASH_TRACKING  =  { "DRBH", 0, 0 };
TRACKING EXT_BLOCK_TRACKING      =  { "EXTB", 0, 0 };
TRACKING EXT_BLOCKHASH_TRACKING  =  { "EXBH", 0, 0 };
TRACKING ASSN_BLOCK_TRACKING     =  { "ASNB", 0, 0 };
TRACKING ASSN_BLOCKHASH_TRACKING =  { "ASBH", 0, 0 };
TRACKING BLOCK_TRACKING          =  { "BLKM", 0, 0 };
TRACKING CREATE_TRACKING         =  { "CRET", 0, 0 };
TRACKING TRUSTEE_TRACKING        =  { "TRST", 0, 0 };
TRACKING USER_TRACKING           =  { "USER", 0, 0 };
TRACKING NWDIR_TRACKING          =  { "NWDT", 0, 0 };
TRACKING NWEXT_TRACKING          =  { "NWET", 0, 0 };
TRACKING FAT_WORKSPACE_TRACKING  =  { "WRKF", 0, 0 };
TRACKING DIR_WORKSPACE_TRACKING  =  { "WRKD", 0, 0 };
TRACKING EXT_WORKSPACE_TRACKING  =  { "WRKE", 0, 0 };
TRACKING CRT_WORKSPACE_TRACKING  =  { "WRKC", 0, 0 };
TRACKING SA_WORKSPACE_TRACKING   =  { "WKSA", 0, 0 };
TRACKING TRUNC_WORK_TRACKING     =  { "WKST", 0, 0 };
TRACKING SECTOR_TRACKING         =  { "SECT", 0, 0 };
TRACKING CLUSTER_TRACKING        =  { "CLST", 0, 0 };
TRACKING VOLUME_TRACKING         =  { "VOLS", 0, 0 };
TRACKING VOLUME_WS_TRACKING      =  { "VOLW", 0, 0 };
TRACKING VOLUME_DATA_TRACKING    =  { "VOLD", 0, 0 };
TRACKING LRU_TRACKING            =  { "LRUS", 0, 0 };
TRACKING LRU_HASH_TRACKING       =  { "LRUH", 0, 0 };
TRACKING LRU_BUFFER_TRACKING     =  { "LRUB", 0, 0 };
TRACKING ALIGN_BUF_TRACKING      =  { "ALGN", 0, 0 };
TRACKING NAM_HASH_TRACKING       =  { "NAMH", 0, 0 };
TRACKING DNM_HASH_TRACKING       =  { "DNMH", 0, 0 };
TRACKING PAR_HASH_TRACKING       =  { "PARH", 0, 0 };
TRACKING EXD_HASH_TRACKING       =  { "EXDH", 0, 0 };
TRACKING TRS_HASH_TRACKING       =  { "TRSH", 0, 0 };
TRACKING DEL_HASH_TRACKING       =  { "DELH", 0, 0 };
TRACKING QUO_HASH_TRACKING       =  { "QUOH", 0, 0 };
TRACKING SUBALLOC_HEAD_TRACKING  =  { "SUBH", 0, 0 };
TRACKING PAGED_FAT_BUF_TRACKING  =  { "PGFT", 0, 0 };
TRACKING PAGED_WORK_BUF_TRACKING =  { "PGWK", 0, 0 };
TRACKING PAGED_PART_BUF_TRACKING =  { "PGPT", 0, 0 };
TRACKING PAGED_VOL_BUF_TRACKING  =  { "PGVL", 0, 0 };
TRACKING PAGED_SECT_BUF_TRACKING =  { "PGSC", 0, 0 };
TRACKING SEMAPHORE_TRACKING      =  { "SEMA", 0, 0 };
TRACKING NAME_STORAGE_TRACKING   =  { "NAME", 0, 0 };
TRACKING NWVP_TRACKING           =  { "NWVP", 0, 0 };
TRACKING BH_TRACKING             =  { "BHBH", 0, 0 };
TRACKING ASYNCH_HASH_TRACKING    =  { "AIOH", 0, 0 };

TRACKING *tracked_memory[]=
{
    &HOTFIX_TRACKING,         
    &HOTFIX_DATA_TRACKING,    
    &HOTFIX_TABLE_TRACKING,   
    &MIRROR_TRACKING,         
    &MIRROR_DATA_TRACKING,    
    &HASH_TRACKING,           
    &EXT_HASH_TRACKING,       
    &BITBLOCK_TRACKING,       
    &BITBUFFER_TRACKING,      
    &NWPART_TRACKING,         
    &NWDISK_TRACKING,         
    &DISKBUF_TRACKING,        
    &TABLE_TRACKING,          
    &FAT_TRACKING,            
    &FATLRU_TRACKING,         
    &FATHASH_TRACKING,        
    &DIRLRU_TRACKING,         
    &DIRHASH_TRACKING,        
    &DIR_TRACKING,            
    &DIR_BLOCK_TRACKING,      
    &DIR_BLOCKHASH_TRACKING,  
    &EXT_BLOCK_TRACKING,      
    &EXT_BLOCKHASH_TRACKING,  
    &ASSN_BLOCK_TRACKING,     
    &ASSN_BLOCKHASH_TRACKING, 
    &BLOCK_TRACKING,          
    &CREATE_TRACKING,         
    &TRUSTEE_TRACKING,        
    &USER_TRACKING,           
    &NWDIR_TRACKING,          
    &NWEXT_TRACKING,          
    &FAT_WORKSPACE_TRACKING,  
    &DIR_WORKSPACE_TRACKING,  
    &EXT_WORKSPACE_TRACKING,  
    &CRT_WORKSPACE_TRACKING,  
    &SA_WORKSPACE_TRACKING,   
    &TRUNC_WORK_TRACKING,     
    &SECTOR_TRACKING,         
    &CLUSTER_TRACKING,        
    &VOLUME_TRACKING,         
    &VOLUME_WS_TRACKING,      
    &VOLUME_DATA_TRACKING,    
    &LRU_TRACKING,            
    &LRU_HASH_TRACKING,       
    &LRU_BUFFER_TRACKING,     
    &ALIGN_BUF_TRACKING,      
    &NAM_HASH_TRACKING,       
    &DNM_HASH_TRACKING,       
    &PAR_HASH_TRACKING,       
    &EXD_HASH_TRACKING,       
    &TRS_HASH_TRACKING,       
    &DEL_HASH_TRACKING,       
    &QUO_HASH_TRACKING,       
    &SUBALLOC_HEAD_TRACKING,  
    &PAGED_FAT_BUF_TRACKING,      
    &PAGED_WORK_BUF_TRACKING,     
    &PAGED_PART_BUF_TRACKING,     
    &PAGED_VOL_BUF_TRACKING,
    &PAGED_SECT_BUF_TRACKING,     
    &SEMAPHORE_TRACKING,      
    &NAME_STORAGE_TRACKING,   
    &NWVP_TRACKING,           
    &BH_TRACKING,             
    &ASYNCH_HASH_TRACKING,    
};

ULONG tracked_memory_count = sizeof(tracked_memory) / sizeof(TRACKING *);

ULONG MajorVersion = 0, MinorVersion = 0, BuildVersion = 0;
ULONG exitFlag = 0;
ULONG consoleHandle = 0;
ULONG MaximumNumberOfVolumes = 0;
ULONG NumberOfVolumes = 0;

ULONG MemoryAllocated = 0;
ULONG MemoryFreed = 0;
ULONG MemoryInUse = 0;

ULONG PagedMemoryAllocated = 0;
ULONG PagedMemoryFreed = 0;
ULONG PagedMemoryInUse = 0;

NWDISK *SystemDisk[MAX_DISKS];
VOLUME *VolumeTable[MAX_VOLUMES];
ULONG VolumeMountedTable[MAX_VOLUMES];
ULONG InstanceCounter = 1;
ULONG (*RemirrorProcessPtr)(ULONG) = 0;

ULONG DataLRUActive = 0;
ULONG JournalLRUActive = 0;
ULONG FATLRUActive = 0;
ULONG LogicalBlockSize = 512;

LRU_HANDLE DataLRU;
LRU_HANDLE JournalLRU;
LRU_HANDLE FATLRU;

ASYNCH_IO *aio_free_head = 0;
ASYNCH_IO *aio_free_tail = 0;
ULONG aio_free_count;

ULONG HOTFIX_TRACK_OFFSET[MULTI_TRACK_COUNT]=
{
    HOTFIX_LOCATION_0, HOTFIX_LOCATION_1, HOTFIX_LOCATION_2,
    HOTFIX_LOCATION_3
};

ULONG MIRROR_TRACK_OFFSET[MULTI_TRACK_COUNT]=
{
    MIRROR_LOCATION_0, MIRROR_LOCATION_1, MIRROR_LOCATION_2,
    MIRROR_LOCATION_3
};

ULONG VOLTABLE_TRACK_OFFSET[MULTI_TRACK_COUNT]=
{
    VOLUME_TABLE_LOCATION_0, VOLUME_TABLE_LOCATION_1, VOLUME_TABLE_LOCATION_2,
    VOLUME_TABLE_LOCATION_3
};

#if (LINUX)

ULONG DiskDevices[MAX_DISKS] =
{
    // IDE (hda - hdt)
    0x0300, 0x0340, 0x1600, 0x1640, 0x2100, 0x2140, 0x2200, 0x2240,
    0x3800, 0x3840, 0x3900, 0x3940, 0x5800, 0x5840, 0x5900, 0x5940,
    0x5A00, 0x5A40, 0x5B00, 0x5B40,

    // SCSI 0-15  (sda - sdp)
    0x0800, 0x0810, 0x0820, 0x0830, 0x0840, 0x0850, 0x0860, 0x0870,
    0x0880, 0x0890, 0x08a0, 0x08b0, 0x08c0, 0x08d0, 0x08e0, 0x08f0,

    // SCSI 16-31  (sdq - sdaf)
    0x4100, 0x4110, 0x4120, 0x4130, 0x4140, 0x4150, 0x4160, 0x4170,
    0x4180, 0x4190, 0x41a0, 0x41b0, 0x41c0, 0x41d0, 0x41e0, 0x41f0,

    // SCSI 32-47  (sdag - sdav)
    0x4200, 0x4210, 0x4220, 0x4230, 0x4240, 0x4250, 0x4260, 0x4270,
    0x4280, 0x4290, 0x42a0, 0x42b0, 0x42c0, 0x42d0, 0x42e0, 0x42f0,

    // SCSI 48-63  (sdaw - sdbl)
    0x4300, 0x4310, 0x4320, 0x4330, 0x4340, 0x4350, 0x4360, 0x4370,
    0x4380, 0x4390, 0x43a0, 0x43b0, 0x43c0, 0x43d0, 0x43e0, 0x43f0,

    // SCSI 64-79  (sdbm - sdcb)
    0x4400, 0x4410, 0x4420, 0x4430, 0x4440, 0x4450, 0x4460, 0x4470,
    0x4480, 0x4490, 0x44a0, 0x44b0, 0x44c0, 0x44d0, 0x44e0, 0x44f0,

    // SCSI 80-95  (sdcc - sdcr)
    0x4500, 0x4510, 0x4520, 0x4530, 0x4540, 0x4550, 0x4560, 0x4570,
    0x4580, 0x4590, 0x45a0, 0x45b0, 0x45c0, 0x45d0, 0x45e0, 0x45f0,

    // SCSI 96-111  (sdcs - sddh)
    0x4600, 0x4610, 0x4620, 0x4630, 0x4640, 0x4650, 0x4660, 0x4670,
    0x4680, 0x4690, 0x46a0, 0x46b0, 0x46c0, 0x46d0, 0x46e0, 0x46f0,

    // SCSI 112-127  (sddi - sddx)
    0x4700, 0x4710, 0x4720, 0x4730, 0x4740, 0x4750, 0x4760, 0x4770,
    0x4780, 0x4790, 0x47a0, 0x47b0, 0x47c0, 0x47d0, 0x47e0, 0x47f0,

    // MCA EDSI (eda, edb)
    0x2400, 0x2440,

#if (META_DISK)
    // metadisk groups
    0x0900, 0x0901, 0x0902, 0x0903, 0x0904, 0x0905, 0x0906, 0x0907,
    0x0908, 0x0909, 0x090a, 0x090b, 0x090c, 0x090d, 0x090e, 0x090f,
#endif

    // XT disks
    0x0D00, 0x0D40,

    // Acorn MFM disks
    0x1500, 0x1540,

    // Parallel Port ATA disks
    0x2F00, 0x2F01, 0x2F02, 0x2F03,

#if (I2O_DEVICES)
    // I2O 0-15  (hda - hdp)
    0x5000, 0x5010, 0x5020, 0x5030, 0x5040, 0x5050, 0x5060, 0x5070,
    0x5080, 0x5090, 0x50a0, 0x50b0, 0x50c0, 0x50d0, 0x50e0, 0x50f0,

    // I2O 16-31  (hdq - hdaf)
    0x5100, 0x5110, 0x5120, 0x5130, 0x5140, 0x5150, 0x5160, 0x5170,
    0x5180, 0x5190, 0x51a0, 0x51b0, 0x51c0, 0x51d0, 0x51e0, 0x51f0,

    // I2O 32-47  (hdag - hdav)
    0x5200, 0x5210, 0x5220, 0x5230, 0x5240, 0x5250, 0x5260, 0x5270,
    0x5280, 0x5290, 0x52a0, 0x52b0, 0x52c0, 0x52d0, 0x52e0, 0x52f0,

    // I2O 48-63  (hdaw - hdbl)
    0x5300, 0x5310, 0x5320, 0x5330, 0x5340, 0x5350, 0x5360, 0x5370,
    0x5380, 0x5390, 0x53a0, 0x53b0, 0x53c0, 0x53d0, 0x53e0, 0x53f0,

    // I2O 64-79  (hdbm - hdcb)
    0x5400, 0x5410, 0x5420, 0x5430, 0x5440, 0x5450, 0x5460, 0x5470,
    0x5480, 0x5490, 0x54a0, 0x54b0, 0x54c0, 0x54d0, 0x54e0, 0x54f0,

    // I2O 80-95  (hdcc - hdcr)
    0x5500, 0x5510, 0x5520, 0x5530, 0x5540, 0x5550, 0x5560, 0x5570,
    0x5580, 0x5590, 0x55a0, 0x55b0, 0x55c0, 0x55d0, 0x55e0, 0x55f0,

    // I2O 96-111  (hdcs - hddh)
    0x5600, 0x5610, 0x5620, 0x5630, 0x5640, 0x5650, 0x5660, 0x5670,
    0x5680, 0x5690, 0x56a0, 0x56b0, 0x56c0, 0x56d0, 0x56e0, 0x56f0,

    // I2O 112-127  (hddi - hddx)
    0x5700, 0x5710, 0x5720, 0x5730, 0x5740, 0x5750, 0x5760, 0x5770,
    0x5780, 0x5790, 0x57a0, 0x57b0, 0x57c0, 0x57d0, 0x57e0, 0x57f0,
#endif

#if (COMPAQ_SMART2_RAID)
    // COMPAQ SMART2 Array 0-15 (ida0a - ida0p)
    0x4800, 0x4810, 0x4820, 0x4830, 0x4840, 0x4850, 0x4860, 0x4870,
    0x4880, 0x4890, 0x48a0, 0x48b0, 0x48c0, 0x48d0, 0x48e0, 0x48f0,

    // COMPAQ SMART2 Array 16-31  (ida1a - ida1p)
    0x4900, 0x4910, 0x4920, 0x4930, 0x4940, 0x4950, 0x4960, 0x4970,
    0x4980, 0x4990, 0x49a0, 0x49b0, 0x49c0, 0x49d0, 0x49e0, 0x49f0,

    // COMPAQ SMART2 Array 32-47  (ida2a - ida2p)
    0x4A00, 0x4A10, 0x4A20, 0x4A30, 0x4A40, 0x4A50, 0x4A60, 0x4A70,
    0x4A80, 0x4A90, 0x4Aa0, 0x4Ab0, 0x4Ac0, 0x4Ad0, 0x4Ae0, 0x4Af0,

    // COMPAQ SMART2 Array 48-63  (ida3a - ida3p)
    0x4B00, 0x4B10, 0x4B20, 0x4B30, 0x4B40, 0x4B50, 0x4B60, 0x4B70,
    0x4B80, 0x4B90, 0x4Ba0, 0x4Bb0, 0x4Bc0, 0x4Bd0, 0x4Be0, 0x4Bf0,

    // COMPAQ SMART2 Array 64-79  (ida4a - ida4p)
    0x4C00, 0x4C10, 0x4C20, 0x4C30, 0x4C40, 0x4C50, 0x4C60, 0x4C70,
    0x4C80, 0x4C90, 0x4Ca0, 0x4Cb0, 0x4Cc0, 0x4Cd0, 0x4Ce0, 0x4Cf0,

    // COMPAQ SMART2 Array 80-95  (ida5a - ida5p)
    0x4D00, 0x4D10, 0x4D20, 0x4D30, 0x4D40, 0x4D50, 0x4D60, 0x4D70,
    0x4D80, 0x4D90, 0x4Da0, 0x4Db0, 0x4Dc0, 0x4Dd0, 0x4De0, 0x4Df0,

    // COMPAQ SMART2 Array 96-111  (ida6a - ida6p)
    0x4E00, 0x4E10, 0x4E20, 0x4E30, 0x4E40, 0x4E50, 0x4E60, 0x4E70,
    0x4E80, 0x4E90, 0x4Ea0, 0x4Eb0, 0x4Ec0, 0x4Ed0, 0x4Ee0, 0x4Ef0,

    // COMPAQ SMART2 Array 112-127  (ida7a - ida7p)
    0x4F00, 0x4F10, 0x4F20, 0x4F30, 0x4F40, 0x4F50, 0x4F60, 0x4F70,
    0x4F80, 0x4F90, 0x4Fa0, 0x4Fb0, 0x4Fc0, 0x4Fd0, 0x4Fe0, 0x4Ff0,
#endif

#if (DAC960_RAID)
    // DAC960 Raid  Array 0-15 (ida0a - ida0p)
    0x3000, 0x3010, 0x3020, 0x3030, 0x3040, 0x3050, 0x3060, 0x3070,
    0x3080, 0x3090, 0x30a0, 0x30b0, 0x30c0, 0x30d0, 0x30e0, 0x30f0,

    // DAC960 Raid  Array 16-31  (ida1a - ida1p)
    0x3100, 0x3110, 0x3120, 0x3130, 0x3140, 0x3150, 0x3160, 0x3170,
    0x3180, 0x3190, 0x31a0, 0x31b0, 0x31c0, 0x31d0, 0x31e0, 0x31f0,

    // DAC960 Raid  Array 32-47  (ida2a - ida2p)
    0x3200, 0x3210, 0x3220, 0x3230, 0x3240, 0x3250, 0x3260, 0x3270,
    0x3280, 0x3290, 0x32a0, 0x32b0, 0x32c0, 0x32d0, 0x32e0, 0x32f0,

    // DAC960 Raid  Array 48-63  (ida3a - ida3p)
    0x3300, 0x3310, 0x3320, 0x3330, 0x3340, 0x3350, 0x3360, 0x3370,
    0x3380, 0x3390, 0x33a0, 0x33b0, 0x33c0, 0x33d0, 0x33e0, 0x33f0,

    // DAC960 Raid  Array 64-79  (ida4a - ida4p)
    0x3400, 0x3410, 0x3420, 0x3430, 0x3440, 0x3450, 0x3460, 0x3470,
    0x3480, 0x3490, 0x34a0, 0x34b0, 0x34c0, 0x34d0, 0x34e0, 0x34f0,

    // DAC960 Raid  Array 80-95  (ida5a - ida5p)
    0x3500, 0x3510, 0x3520, 0x3530, 0x3540, 0x3550, 0x3560, 0x3570,
    0x3580, 0x3590, 0x35a0, 0x35b0, 0x35c0, 0x35d0, 0x35e0, 0x35f0,

    // DAC960 Raid  Array 96-111  (ida6a - ida6p)
    0x3600, 0x3610, 0x3620, 0x3630, 0x3640, 0x3650, 0x3660, 0x3670,
    0x3680, 0x3690, 0x36a0, 0x36b0, 0x36c0, 0x36d0, 0x36e0, 0x36f0,

    // DAC960 Raid  Array 112-127  (ida7a - ida7p)
    0x3700, 0x3710, 0x3720, 0x3730, 0x3740, 0x3750, 0x3760, 0x3770,
    0x3780, 0x3790, 0x37a0, 0x37b0, 0x37c0, 0x37d0, 0x37e0, 0x37f0,
#endif

    0, 0, 0, 0,

};

BYTE *PhysicalDiskNames[MAX_DISKS]=
{
    // IDE disks
    "/dev/hda", "/dev/hdb", "/dev/hdc", "/dev/hdd",
    "/dev/hde", "/dev/hdf", "/dev/hdg", "/dev/hdh",
    "/dev/hdi", "/dev/hdj", "/dev/hdk", "/dev/hdl",
    "/dev/hdm", "/dev/hdn", "/dev/hdo", "/dev/hdp",
    "/dev/hdq", "/dev/hdr", "/dev/hds", "/dev/hdt",

    // *** 20 ***

    // SCSI disks 0-15
    "/dev/sda", "/dev/sdb", "/dev/sdc", "/dev/sdd",
    "/dev/sde", "/dev/sdf", "/dev/sdg", "/dev/sdh",
    "/dev/sdi", "/dev/sdj", "/dev/sdk", "/dev/sdl",
    "/dev/sdm", "/dev/sdn", "/dev/sdo", "/dev/sdp",

    // SCSI disks 15-31
    "/dev/sdq", "/dev/sdr", "/dev/sds", "/dev/sdt",
    "/dev/sdu", "/dev/sdv", "/dev/sdw", "/dev/sdx",
    "/dev/sdy", "/dev/sdz", "/dev/sdaa", "/dev/sdab",
    "/dev/sdac", "/dev/sdad", "/dev/sdae", "/dev/sdaf",

    // SCSI disks 32-47
    "/dev/sdag", "/dev/sdah", "/dev/sdai", "/dev/sdaj",
    "/dev/sdak", "/dev/sdal", "/dev/sdam", "/dev/sdan",
    "/dev/sdao", "/dev/sdap", "/dev/sdaq", "/dev/sdar",
    "/dev/sdas", "/dev/sdat", "/dev/sdau", "/dev/sdav",

    // SCSI disks 48-63
    "/dev/sdaw", "/dev/sdax", "/dev/sday", "/dev/sdaz",
    "/dev/sdba", "/dev/sdbb", "/dev/sdbc", "/dev/sdbd",
    "/dev/sdbe", "/dev/sdbf", "/dev/sdbg", "/dev/sdbh",
    "/dev/sdbi", "/dev/sdbj", "/dev/sdbk", "/dev/sdbl",

    // SCSI disks 64-79
    "/dev/sdbm", "/dev/sdbn", "/dev/sdbo", "/dev/sdbp",
    "/dev/sdbq", "/dev/sdbr", "/dev/sdbs", "/dev/sdbt",
    "/dev/sdbu", "/dev/sdbv", "/dev/sdbw", "/dev/sdbx",
    "/dev/sdby", "/dev/sdbz", "/dev/sdca", "/dev/sdcb",

    // SCSI disks 80-95
    "/dev/sdcc", "/dev/sdcd", "/dev/sdce", "/dev/sdcf",
    "/dev/sdcg", "/dev/sdch", "/dev/sdci", "/dev/sdcj",
    "/dev/sdck", "/dev/sdcl", "/dev/sdcm", "/dev/sdcn",
    "/dev/sdco", "/dev/sdcp", "/dev/sdcq", "/dev/sdcr",

    // SCSI disks 96-111
    "/dev/sdcs", "/dev/sdct", "/dev/sdcu", "/dev/sdcv",
    "/dev/sdcw", "/dev/sdcx", "/dev/sdcy", "/dev/sdcz",
    "/dev/sdda", "/dev/sddb", "/dev/sddc", "/dev/sddd",
    "/dev/sdde", "/dev/sddf", "/dev/sddg", "/dev/sddh",

    // SCSI disks 112-127
    "/dev/sddi", "/dev/sddj", "/dev/sddk", "/dev/sddl",
    "/dev/sddm", "/dev/sddn", "/dev/sddo", "/dev/sddp",
    "/dev/sddq", "/dev/sddr", "/dev/sdds", "/dev/sddt",
    "/dev/sddu", "/dev/sddv", "/dev/sddw", "/dev/sddx",

    // MCA ESDI disks
    "/dev/eda", "/dev/edb",

    // *** 150 ***

#if (META_DISK)
    // metadisk groups
    "/dev/md0", "dev/md1", "/dev/md2", "dev/md3",
    "/dev/md4", "dev/md5", "/dev/md6", "dev/md7",
    "/dev/md8", "dev/md9", "/dev/md10", "dev/md11",
    "/dev/md12", "dev/md13", "/dev/md14", "dev/md15",
#endif

    // XT disks
    "/dev/xda", "/dev/xdb",

    // Acorn MFM disks
    "/dev/mfma", "/dev/mfmb",

    // Parallel Port ATA disks
    "/dev/pf0", "/dev/pf1", "/dev/pf2", "/dev/pf3",

    // *** 174 ***

#if (I2O_DEVICES)
    // I2O disks 0-15
    "/dev/i2o/hda", "/dev/i2o/hdb", "/dev/i2o/hdc", "/dev/i2o/hdd",
    "/dev/i2o/hde", "/dev/i2o/hdf", "/dev/i2o/hdg", "/dev/i2o/hdh",
    "/dev/i2o/hdi", "/dev/i2o/hdj", "/dev/i2o/hdk", "/dev/i2o/hdl",
    "/dev/i2o/hdm", "/dev/i2o/hdn", "/dev/i2o/hdo", "/dev/i2o/hdp",

    // I2O disks 16-31
    "/dev/i2o/hdq", "/dev/i2o/hdr", "/dev/i2o/hds", "/dev/i2o/hdt",
    "/dev/i2o/hdu", "/dev/i2o/hdv", "/dev/i2o/hdw", "/dev/i2o/hdx",
    "/dev/i2o/hdy", "/dev/i2o/hdz", "/dev/i2o/hdaa", "/dev/i2o/hdab",
    "/dev/i2o/hdac", "/dev/i2o/hdad", "/dev/i2o/hdae", "/dev/i2o/hdaf",

    // I2O disks 32-47
    "/dev/i2o/hdag", "/dev/i2o/hdah", "/dev/i2o/hdai", "/dev/i2o/hdaj",
    "/dev/i2o/hdak", "/dev/i2o/hdal", "/dev/i2o/hdam", "/dev/i2o/hdan",
    "/dev/i2o/hdao", "/dev/i2o/hdap", "/dev/i2o/hdaq", "/dev/i2o/hdar",
    "/dev/i2o/hdas", "/dev/i2o/hdat", "/dev/i2o/hdau", "/dev/i2o/hdav",

    // I2O disks 48-63
    "/dev/i2o/hdaw", "/dev/i2o/hdax", "/dev/i2o/hday", "/dev/i2o/hdaz",
    "/dev/i2o/hdba", "/dev/i2o/hdbb", "/dev/i2o/hdbc", "/dev/i2o/hdbd",
    "/dev/i2o/hdbe", "/dev/i2o/hdbf", "/dev/i2o/hdbg", "/dev/i2o/hdbh",
    "/dev/i2o/hdbi", "/dev/i2o/hdbj", "/dev/i2o/hdbk", "/dev/i2o/hdbl",

    // I2O disks 64-79
    "/dev/i2o/hdbm", "/dev/i2o/hdbn", "/dev/i2o/hdbo", "/dev/i2o/hdbp",
    "/dev/i2o/hdbq", "/dev/i2o/hdbr", "/dev/i2o/hdbs", "/dev/i2o/hdbt",
    "/dev/i2o/hdbu", "/dev/i2o/hdbv", "/dev/i2o/hdbw", "/dev/i2o/hdbx",
    "/dev/i2o/hdby", "/dev/i2o/hdbz", "/dev/i2o/hdca", "/dev/i2o/hdcb",

    // I2O disks 80-95
    "/dev/i2o/hdcc", "/dev/i2o/hdcd", "/dev/i2o/hdce", "/dev/i2o/hdcf",
    "/dev/i2o/hdcg", "/dev/i2o/hdch", "/dev/i2o/hdci", "/dev/i2o/hdcj",
    "/dev/i2o/hdck", "/dev/i2o/hdcl", "/dev/i2o/hdcm", "/dev/i2o/hdcn",
    "/dev/i2o/hdco", "/dev/i2o/hdcp", "/dev/i2o/hdcq", "/dev/i2o/hdcr",

    // I2O disks 96-111
    "/dev/i2o/hdcs", "/dev/i2o/hdct", "/dev/i2o/hdcu", "/dev/i2o/hdcv",
    "/dev/i2o/hdcw", "/dev/i2o/hdcx", "/dev/i2o/hdcy", "/dev/i2o/hdcz",
    "/dev/i2o/hdda", "/dev/i2o/hddb", "/dev/i2o/hddc", "/dev/i2o/hddd",
    "/dev/i2o/hdde", "/dev/i2o/hddf", "/dev/i2o/hddg", "/dev/i2o/hddh",

    // I2O disks 112-127
    "/dev/i2o/hddi", "/dev/i2o/hddj", "/dev/i2o/hddk", "/dev/i2o/hddl",
    "/dev/i2o/hddm", "/dev/i2o/hddn", "/dev/i2o/hddo", "/dev/i2o/hddp",
    "/dev/i2o/hddq", "/dev/i2o/hddr", "/dev/i2o/hdds", "/dev/i2o/hddt",
    "/dev/i2o/hddu", "/dev/i2o/hddv", "/dev/i2o/hddw", "/dev/i2o/hddx",
#endif

    // *** 302 ***

#if (COMPAQ_SMART2_RAID)
    // COMPAQ Smart2 disks 0-15
    "/dev/ida0a", "/dev/ida0b", "/dev/ida0c", "/dev/ida0d",
    "/dev/ida0e", "/dev/ida0f", "/dev/ida0g", "/dev/ida0h",
    "/dev/ida0i", "/dev/ida0j", "/dev/ida0k", "/dev/ida0l",
    "/dev/ida0m", "/dev/ida0n", "/dev/ida0o", "/dev/ida0p",

    // COMPAQ Smart2 disks 16-31
    "/dev/ida1a", "/dev/ida1b", "/dev/ida1c", "/dev/ida1d",
    "/dev/ida1e", "/dev/ida1f", "/dev/ida1g", "/dev/ida1h",
    "/dev/ida1i", "/dev/ida1j", "/dev/ida1k", "/dev/ida1l",
    "/dev/ida1m", "/dev/ida1n", "/dev/ida1o", "/dev/ida1p",

    // COMPAQ Smart2 disks 32-47
    "/dev/ida2a", "/dev/ida2b", "/dev/ida2c", "/dev/ida2d",
    "/dev/ida2e", "/dev/ida2f", "/dev/ida2g", "/dev/ida2h",
    "/dev/ida2i", "/dev/ida2j", "/dev/ida2k", "/dev/ida2l",
    "/dev/ida2m", "/dev/ida2n", "/dev/ida2o", "/dev/ida2p",

    // COMPAQ Smart2 disks 48-63
    "/dev/ida3a", "/dev/ida3b", "/dev/ida3c", "/dev/ida3d",
    "/dev/ida3e", "/dev/ida3f", "/dev/ida3g", "/dev/ida3h",
    "/dev/ida3i", "/dev/ida3j", "/dev/ida3k", "/dev/ida3l",
    "/dev/ida3m", "/dev/ida3n", "/dev/ida3o", "/dev/ida3p",

    // COMPAQ Smart2 disks 64-79
    "/dev/ida4a", "/dev/ida4b", "/dev/ida4c", "/dev/ida4d",
    "/dev/ida4e", "/dev/ida4f", "/dev/ida4g", "/dev/ida4h",
    "/dev/ida4i", "/dev/ida4j", "/dev/ida4k", "/dev/ida4l",
    "/dev/ida4m", "/dev/ida4n", "/dev/ida4o", "/dev/ida4p",

    // COMPAQ Smart2 disks 80-95
    "/dev/ida5a", "/dev/ida5b", "/dev/ida5c", "/dev/ida5d",
    "/dev/ida5e", "/dev/ida5f", "/dev/ida5g", "/dev/ida5h",
    "/dev/ida5i", "/dev/ida5j", "/dev/ida5k", "/dev/ida5l",
    "/dev/ida5m", "/dev/ida5n", "/dev/ida5o", "/dev/ida5p",

    // COMPAQ Smart2 disks 96-111
    "/dev/ida6a", "/dev/ida6b", "/dev/ida6c", "/dev/ida6d",
    "/dev/ida6e", "/dev/ida6f", "/dev/ida6g", "/dev/ida6h",
    "/dev/ida6i", "/dev/ida6j", "/dev/ida6k", "/dev/ida6l",
    "/dev/ida6m", "/dev/ida6n", "/dev/ida6o", "/dev/ida6p",

    // COMPAQ Smart2 disks 112-127
    "/dev/ida7a", "/dev/ida7b", "/dev/ida7c", "/dev/ida7d",
    "/dev/ida7e", "/dev/ida7f", "/dev/ida7g", "/dev/ida7h",
    "/dev/ida7i", "/dev/ida7j", "/dev/ida7k", "/dev/ida7l",
    "/dev/ida7m", "/dev/ida7n", "/dev/ida7o", "/dev/ida7p",
#endif

    // *** 430 ***

#if (DAC960_RAID)
    // DAC960 Raid  disks 0-15
    "/dev/rd/c0d0",   "/dev/rd/c0d0p1", "/dev/rd/c0d0p2", "/dev/rd/c0d0p3",
    "/dev/rd/c0d0p4", "/dev/rd/c0d0p5", "/dev/rd/c0d0p6", "/dev/rd/c0d0p7",
    "/dev/rd/c0d1",   "/dev/rd/c0d1p1", "/dev/rd/c0d1p2", "/dev/rd/c0d1p3",
    "/dev/rd/c0d1p4", "/dev/rd/c0d1p5", "/dev/rd/c0d1p6", "/dev/rd/c0d1p7",

    // DAC960 Raid  disks 16-31
    "/dev/rd/c0d2",   "/dev/rd/c0d2p1", "/dev/rd/c0d2p2", "/dev/rd/c0d2p3",
    "/dev/rd/c0d2p4", "/dev/rd/c0d2p5", "/dev/rd/c0d2p6", "/dev/rd/c0d2p7",
    "/dev/rd/c0d3",   "/dev/rd/c0d3p1", "/dev/rd/c0d3p2", "/dev/rd/c0d3p3",
    "/dev/rd/c0d3p4", "/dev/rd/c0d3p5", "/dev/rd/c0d3p6", "/dev/rd/c0d3p7",

    // DAC960 Raid  disks 32-47
    "/dev/rd/c0d4",   "/dev/rd/c0d4p1", "/dev/rd/c0d4p2", "/dev/rd/c0d4p3",
    "/dev/rd/c0d4p4", "/dev/rd/c0d4p5", "/dev/rd/c0d4p6", "/dev/rd/c0d4p7",
    "/dev/rd/c0d5",   "/dev/rd/c0d5p1", "/dev/rd/c0d5p2", "/dev/rd/c0d5p3",
    "/dev/rd/c0d5p4", "/dev/rd/c0d5p5", "/dev/rd/c0d5p6", "/dev/rd/c0d5p7",

    // DAC960 Raid  disks 48-63
    "/dev/rd/c0d6",   "/dev/rd/c0d6p1", "/dev/rd/c0d6p2", "/dev/rd/c0d6p3",
    "/dev/rd/c0d6p4", "/dev/rd/c0d6p5", "/dev/rd/c0d6p6", "/dev/rd/c0d6p7",
    "/dev/rd/c0d7",   "/dev/rd/c0d7p1", "/dev/rd/c0d7p2", "/dev/rd/c0d7p3",
    "/dev/rd/c0d7p4", "/dev/rd/c0d7p5", "/dev/rd/c0d7p6", "/dev/rd/c0d7p7",

    // DAC960 Raid  disks 64-79
    "/dev/rd/c1d0",   "/dev/rd/c1d0p1", "/dev/rd/c1d0p2", "/dev/rd/c1d0p3",
    "/dev/rd/c1d0p4", "/dev/rd/c1d0p5", "/dev/rd/c1d0p6", "/dev/rd/c1d0p7",
    "/dev/rd/c1d1",   "/dev/rd/c1d1p1", "/dev/rd/c1d1p2", "/dev/rd/c1d1p3",
    "/dev/rd/c1d1p4", "/dev/rd/c1d1p5", "/dev/rd/c1d1p6", "/dev/rd/c1d1p7",

    // DAC960 Raid  disks 80-95
    "/dev/rd/c1d2",   "/dev/rd/c1d2p1", "/dev/rd/c1d2p2", "/dev/rd/c1d2p3",
    "/dev/rd/c1d2p4", "/dev/rd/c1d2p5", "/dev/rd/c1d2p6", "/dev/rd/c1d2p7",
    "/dev/rd/c1d3",   "/dev/rd/c1d3p1", "/dev/rd/c1d3p2", "/dev/rd/c1d3p3",
    "/dev/rd/c1d3p4", "/dev/rd/c1d3p5", "/dev/rd/c1d3p6", "/dev/rd/c1d3p7",

    // DAC960 Raid  disks 96-111
    "/dev/rd/c1d4",   "/dev/rd/c1d4p1", "/dev/rd/c1d4p2", "/dev/rd/c1d4p3",
    "/dev/rd/c1d4p4", "/dev/rd/c1d4p5", "/dev/rd/c1d4p6", "/dev/rd/c1d4p7",
    "/dev/rd/c1d5",   "/dev/rd/c1d5p1", "/dev/rd/c1d5p2", "/dev/rd/c1d5p3",
    "/dev/rd/c1d5p4", "/dev/rd/c1d5p5", "/dev/rd/c1d5p6", "/dev/rd/c1d5p7",

    // DAC960 Raid  disks 112-127
    "/dev/rd/c1d6",   "/dev/rd/c1d6p1", "/dev/rd/c1d6p2", "/dev/rd/c1d6p3",
    "/dev/rd/c1d6p4", "/dev/rd/c1d6p5", "/dev/rd/c1d6p6", "/dev/rd/c1d6p7",
    "/dev/rd/c1d7",   "/dev/rd/c1d7p1", "/dev/rd/c1d7p2", "/dev/rd/c1d7p3",
    "/dev/rd/c1d7p4", "/dev/rd/c1d7p5", "/dev/rd/c1d7p6", "/dev/rd/c1d7p7",
#endif

    // *** 558 ***

    NULL, NULL, NULL, NULL,

};

ULONG MaxDisksSupported = sizeof(DiskDevices) / sizeof(ULONG);
ULONG MaxHandlesSupported = sizeof(PhysicalDiskNames) / sizeof(BYTE *);

#endif

#if (LINUX_SPIN)
extern spinlock_t aio_spinlock;
extern spinlock_t cb_spinlock;
extern spinlock_t bh_spinlock;
extern spinlock_t HashFree_spinlock;
#endif

void InitNWFS(void)
{
    register ULONG ccode, i;
    extern ASYNCH_IO_HEAD asynch_io_head[MAX_DISKS];

    // if we are being asked to override default device blocksize
    // for NWFS, make sure the value is legal for us.
    if ((LOGICAL_BLOCK_SIZE == 512)  || (LOGICAL_BLOCK_SIZE == 1024) ||
        (LOGICAL_BLOCK_SIZE == 2048) || (LOGICAL_BLOCK_SIZE == 4096))
       LogicalBlockSize = LOGICAL_BLOCK_SIZE;

    ccode = InitializeLRU(&DataLRU, "Data LRU", WRITE_BACK_LRU,
			  MAX_VOLUME_LRU_BLOCKS,
			  MIN_VOLUME_LRU_BLOCKS,
			  LRU_AGE);
    if (!ccode)
       DataLRUActive = TRUE;

    ccode = InitializeLRU(&JournalLRU, "Dir LRU", WRITE_BACK_LRU,
			  MAX_DIRECTORY_LRU_BLOCKS,
			  MIN_DIRECTORY_LRU_BLOCKS,
			  JOURNAL_AGE);
    if (!ccode)
       JournalLRUActive = TRUE;

    ccode = InitializeLRU(&FATLRU, "Fat LRU", WRITE_BACK_LRU,
			  MAX_FAT_LRU_BLOCKS,
			  MIN_FAT_LRU_BLOCKS,
			  FAT_AGE);
    if (!ccode)
       FATLRUActive = TRUE;

    ZeroBuffer = NWFSIOAlloc(65536, DISKBUF_TAG);
    if (!ZeroBuffer)
       return;
    NWFSSet(ZeroBuffer, 0, 65536);

    // initialize the asynch io heads
    for (i=0; i < MAX_DISKS; i++)
       NWFSSet(&asynch_io_head[i], 0, sizeof(ASYNCH_IO_HEAD));

#if (LINUX_SPIN)
    spin_lock_init(&aio_spinlock);
    spin_lock_init(&cb_spinlock);
    spin_lock_init(&bh_spinlock);
    spin_lock_init(&HashFree_spinlock);
#endif

    return;
}

void CloseNWFS(void)
{
    return;
}

