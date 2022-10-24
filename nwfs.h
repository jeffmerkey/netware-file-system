
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
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWFS.H
*   DESCRIP  :   Volume and System On Disk Structures
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#ifndef _NWFS_
#define _NWFS_

#define NWFS_GET_ATTRIBUTES   0x11
#define NWFS_GET_TRUSTEES     0x12
#define NWFS_GET_QUOTA        0x13
#define NWFS_SET_ATTRIBUTES   0x14
#define NWFS_SET_TRUSTEES     0x15
#define NWFS_SET_QUOTA        0x16

#define NETWARE_286_ID     0x64
#define NETWARE_386_ID     0x65
#define PARTITION_ID     0xAA55
#define NW3X_PARTITION        1
#define NW4X_PARTITION        0

#define FOUR_K_BLOCK          3
#define EIGHT_K_BLOCK         4
#define SIXTEEN_K_BLOCK       5
#define THIRTYTWO_K_BLOCK     6
#define SIXTYFOUR_K_BLOCK     7

// these defines describe the location of the Hot Fix and Mirroring Tables

#define MULTI_TRACK_COUNT     4

#define VOLUME_TABLE_LOCATION_0   0x20
#define VOLUME_TABLE_LOCATION_1   0x40
#define VOLUME_TABLE_LOCATION_2   0x60
#define VOLUME_TABLE_LOCATION_3   0x80

#define HOTFIX_LOCATION_0   0x20
#define HOTFIX_LOCATION_1   0x40
#define HOTFIX_LOCATION_2   0x60
#define HOTFIX_LOCATION_3   0x80

#define MIRROR_LOCATION_0   0x21
#define MIRROR_LOCATION_1   0x41
#define MIRROR_LOCATION_2   0x61
#define MIRROR_LOCATION_3   0x81

#define HOTFIX_SIGNATURE "HOTFIX00"
#define MIRROR_SIGNATURE "MIRROR00"
#define VOLUME_SIGNATURE "NetWare Volumes"

#define USER_ADDRESS_SPACE        0
#define KERNEL_ADDRESS_SPACE      1

//
//  memory tracking labels
//

typedef struct _TRACKING
{
    BYTE *label;
    ULONG units;
    ULONG count;
} TRACKING;

extern TRACKING HOTFIX_TRACKING;
extern TRACKING HOTFIX_DATA_TRACKING;
extern TRACKING HOTFIX_TABLE_TRACKING;
extern TRACKING MIRROR_TRACKING;
extern TRACKING MIRROR_DATA_TRACKING;
extern TRACKING HASH_TRACKING;
extern TRACKING EXT_HASH_TRACKING;
extern TRACKING BITBLOCK_TRACKING;
extern TRACKING BITBUFFER_TRACKING;
extern TRACKING NWPART_TRACKING;
extern TRACKING NWDISK_TRACKING;
extern TRACKING DISKBUF_TRACKING;
extern TRACKING TABLE_TRACKING;
extern TRACKING FAT_TRACKING;
extern TRACKING FATLRU_TRACKING;
extern TRACKING FATHASH_TRACKING;
extern TRACKING DIRLRU_TRACKING;
extern TRACKING DIRHASH_TRACKING;
extern TRACKING DIR_TRACKING;
extern TRACKING DIR_BLOCK_TRACKING;
extern TRACKING DIR_BLOCKHASH_TRACKING;
extern TRACKING EXT_BLOCK_TRACKING;
extern TRACKING EXT_BLOCKHASH_TRACKING;
extern TRACKING ASSN_BLOCK_TRACKING;
extern TRACKING ASSN_BLOCKHASH_TRACKING;
extern TRACKING BLOCK_TRACKING;
extern TRACKING CREATE_TRACKING;
extern TRACKING TRUSTEE_TRACKING;
extern TRACKING USER_TRACKING;
extern TRACKING NWDIR_TRACKING;
extern TRACKING NWEXT_TRACKING;
extern TRACKING FAT_WORKSPACE_TRACKING;
extern TRACKING DIR_WORKSPACE_TRACKING;
extern TRACKING EXT_WORKSPACE_TRACKING;
extern TRACKING CRT_WORKSPACE_TRACKING;
extern TRACKING SA_WORKSPACE_TRACKING;
extern TRACKING TRUNC_WORK_TRACKING;
extern TRACKING SECTOR_TRACKING;
extern TRACKING CLUSTER_TRACKING;
extern TRACKING VOLUME_TRACKING;
extern TRACKING VOLUME_WS_TRACKING;
extern TRACKING VOLUME_DATA_TRACKING;
extern TRACKING LRU_TRACKING;
extern TRACKING LRU_HASH_TRACKING;
extern TRACKING LRU_BUFFER_TRACKING;
extern TRACKING ALIGN_BUF_TRACKING;
extern TRACKING NAM_HASH_TRACKING;
extern TRACKING DNM_HASH_TRACKING;
extern TRACKING PAR_HASH_TRACKING;
extern TRACKING EXD_HASH_TRACKING;
extern TRACKING TRS_HASH_TRACKING;
extern TRACKING DEL_HASH_TRACKING;
extern TRACKING QUO_HASH_TRACKING;
extern TRACKING SUBALLOC_HEAD_TRACKING;
extern TRACKING PAGED_FAT_BUF_TRACKING;
extern TRACKING PAGED_WORK_BUF_TRACKING;
extern TRACKING PAGED_PART_BUF_TRACKING;
extern TRACKING PAGED_VOL_BUF_TRACKING;
extern TRACKING PAGED_SECT_BUF_TRACKING;
extern TRACKING SEMAPHORE_TRACKING;
extern TRACKING NAME_STORAGE_TRACKING;
extern TRACKING NWVP_TRACKING;
extern TRACKING BH_TRACKING;
extern TRACKING ASYNCH_HASH_TRACKING;


#define HOTFIX_TAG                &HOTFIX_TRACKING         
#define HOTFIX_DATA_TAG           &HOTFIX_DATA_TRACKING    
#define HOTFIX_TABLE_TAG          &HOTFIX_TABLE_TRACKING   
#define MIRROR_TAG                &MIRROR_TRACKING         
#define MIRROR_DATA_TAG           &MIRROR_DATA_TRACKING    
#define HASH_TAG                  &HASH_TRACKING           
#define EXT_HASH_TAG              &EXT_HASH_TRACKING       
#define BITBLOCK_TAG              &BITBLOCK_TRACKING       
#define BITBUFFER_TAG             &BITBUFFER_TRACKING      
#define NWPART_TAG                &NWPART_TRACKING         
#define NWDISK_TAG                &NWDISK_TRACKING         
#define DISKBUF_TAG               &DISKBUF_TRACKING        
#define TABLE_TAG                 &TABLE_TRACKING          
#define FAT_TAG                   &FAT_TRACKING            
#define FATLRU_TAG                &FATLRU_TRACKING         
#define FATHASH_TAG               &FATHASH_TRACKING        
#define DIRLRU_TAG                &DIRLRU_TRACKING         
#define DIRHASH_TAG               &DIRHASH_TRACKING        
#define DIR_TAG                   &DIR_TRACKING            
#define DIR_BLOCK_TAG             &DIR_BLOCK_TRACKING      
#define DIR_BLOCKHASH_TAG         &DIR_BLOCKHASH_TRACKING  
#define EXT_BLOCK_TAG             &EXT_BLOCK_TRACKING      
#define EXT_BLOCKHASH_TAG         &EXT_BLOCKHASH_TRACKING  
#define ASSN_BLOCK_TAG            &ASSN_BLOCK_TRACKING     
#define ASSN_BLOCKHASH_TAG        &ASSN_BLOCKHASH_TRACKING 
#define BLOCK_TAG                 &BLOCK_TRACKING          
#define CREATE_TAG                &CREATE_TRACKING         
#define TRUSTEE_TAG               &TRUSTEE_TRACKING        
#define USER_TAG                  &USER_TRACKING           
#define NWDIR_TAG                 &NWDIR_TRACKING          
#define NWEXT_TAG                 &NWEXT_TRACKING          
#define FAT_WORKSPACE_TAG         &FAT_WORKSPACE_TRACKING  
#define DIR_WORKSPACE_TAG         &DIR_WORKSPACE_TRACKING  
#define EXT_WORKSPACE_TAG         &EXT_WORKSPACE_TRACKING  
#define CRT_WORKSPACE_TAG         &CRT_WORKSPACE_TRACKING  
#define SA_WORKSPACE_TAG          &SA_WORKSPACE_TRACKING   
#define TRUNC_WORK_TAG            &TRUNC_WORK_TRACKING     
#define SECTOR_TAG                &SECTOR_TRACKING         
#define CLUSTER_TAG               &CLUSTER_TRACKING        
#define VOLUME_TAG                &VOLUME_TRACKING         
#define VOLUME_WS_TAG             &VOLUME_WS_TRACKING      
#define VOLUME_DATA_TAG           &VOLUME_DATA_TRACKING    
#define LRU_TAG                   &LRU_TRACKING            
#define LRU_HASH_TAG              &LRU_HASH_TRACKING       
#define LRU_BUFFER_TAG            &LRU_BUFFER_TRACKING     
#define ALIGN_BUF_TAG             &ALIGN_BUF_TRACKING      
#define NAM_HASH_TAG              &NAM_HASH_TRACKING       
#define DNM_HASH_TAG              &DNM_HASH_TRACKING       
#define PAR_HASH_TAG              &PAR_HASH_TRACKING       
#define EXD_HASH_TAG              &EXD_HASH_TRACKING       
#define TRS_HASH_TAG              &TRS_HASH_TRACKING       
#define DEL_HASH_TAG              &DEL_HASH_TRACKING       
#define QUO_HASH_TAG              &QUO_HASH_TRACKING       
#define SUBALLOC_HEAD_TAG         &SUBALLOC_HEAD_TRACKING  
#define PAGED_FAT_BUF             &PAGED_FAT_BUF_TRACKING      
#define PAGED_WORK_BUF            &PAGED_WORK_BUF_TRACKING     
#define PAGED_PART_BUF            &PAGED_PART_BUF_TRACKING     
#define PAGED_VOL_BUF             &PAGED_VOL_BUF_TRACKING
#define PAGED_SECT_BUF            &PAGED_SECT_BUF_TRACKING     
#define SEMAPHORE_TAG             &SEMAPHORE_TRACKING      
#define NAME_STORAGE_TAG          &NAME_STORAGE_TRACKING   
#define NWVP_TAG                  &NWVP_TRACKING           
#define BH_TAG                    &BH_TRACKING             
#define ASYNCH_HASH_TAG           &ASYNCH_HASH_TRACKING    

#if (WINDOWS_NT | WINDOWS_NT_RO)
#define NT_POOL_TAG        'sFwN'
#endif

//
//  Hotfix and Mirroring Tables occupy the first sections
//  of a Netware 386 Partition.  There are four copies of these
//  tables.  Netware treats all on-disk structures as zero relative
//  from the beginning of any segment of the disk.  Following
//  these tables are the Volume Segment Tables that define
//  specific information about a volume.  There are four copies of these
//  tables as well.  All of these copies are aligned on track boundries
//  for fault-tolerance so in the event a track fails, at least one good
//  copy of these tables is preserved.
//
//  The Volume Tables contain all the information about volume
//  segments on that drive, and volumes that occupy the partition.
//  Also in these tables are stored the logical starting block for
//  the volume FAT and the first Directory Entry.  The Netware directory
//  is actually treated just like a file, and is mapped via the FAT
//  table.  This is different that the way MS-DOS implements their
//  directory, which chains directory blocks and stores an LBA in each
//  directory object.
//
//  Volumes that have more than one volume segment require that the
//  host operating system read all the disks present in the system
//  and their corresponding VolumeTables to locate the segments for
//  a particular Netware Volume.  If any segments or disks are missing
//  then it is impossible to properly mount a Netware Volume unless a
//  mirror is present.
//
//  The Netware FAT (File Allocation Table) begins at logical
//  block 0 on a logical Netware volume.  There are always two
//  copies of the FAT per logical volume.  The first field of the FAT
//  points to the file index.  Netware allows files to have "holes"
//  to preserve disk space.  For example, if someone creates a file
//  that is 4MB, then only writes two blocks, one a 0 and another at 16K,
//  the index field will point to the actual block index within the
//  logical file (i.e. FAT enties would be 0, 4).  The second field is
//  the continuation index into the FAT to the next entry for the file.
//  The Netware FAT is identical to a DOS FAT in that it is a true
//  one-to-one mapping block-to-fat entry.  If someone tries to read an
//  offset within a file that overlaps a "hole", zeros are returned
//  indicating that no data has actually been written there.
//

#if (LINUX | LINUX_UTIL | DOS_UTIL)

/* Boot sector info (62 byte structure) */

struct BOOT_SECTOR
{
    BYTE Jmp[3]         __attribute__ ((packed));
    BYTE OEMname[8]     __attribute__ ((packed));
    WORD bps            __attribute__ ((packed));
    BYTE SecPerClstr    __attribute__ ((packed));
    WORD ResSectors     __attribute__ ((packed));
    BYTE FATs           __attribute__ ((packed));
    WORD RootDirEnts    __attribute__ ((packed));
    WORD Sectors        __attribute__ ((packed));
    BYTE Media          __attribute__ ((packed));
    WORD SecPerFAT      __attribute__ ((packed));
    WORD SecPerTrack    __attribute__ ((packed));
    WORD Heads          __attribute__ ((packed));
    ULONG HiddenSecs    __attribute__ ((packed));
    ULONG HugeSecs      __attribute__ ((packed));
    BYTE DriveNum       __attribute__ ((packed));
    BYTE Rsvd1          __attribute__ ((packed));
    BYTE BootSig        __attribute__ ((packed));
    ULONG VolID         __attribute__ ((packed));
    BYTE VolLabel[11]   __attribute__ ((packed));
    BYTE FileSysType[8] __attribute__ ((packed));       /* 62 bytes */
};

/* Partition Table Entry info. 16 bytes */

struct PartitionTableEntry
{
    BYTE fBootable      __attribute__ ((packed));
    BYTE HeadStart      __attribute__ ((packed));
    BYTE SecStart       __attribute__ ((packed));
    BYTE CylStart       __attribute__ ((packed));
    BYTE SysFlag        __attribute__ ((packed));
    BYTE HeadEnd        __attribute__ ((packed));
    BYTE SecEnd         __attribute__ ((packed));
    BYTE CylEnd         __attribute__ ((packed));
    ULONG StartLBA      __attribute__ ((packed));
    ULONG nSectorsTotal __attribute__ ((packed));
};

typedef struct _VOLUME_TABLE_ENTRY
{
   BYTE VolumeName[16]        __attribute__ ((packed));  // length preceded string format
   ULONG LastVolumeSegment    __attribute__ ((packed));
   ULONG VolumeSignature      __attribute__ ((packed));
   ULONG VolumeRoot           __attribute__ ((packed));
   ULONG SegmentSectors       __attribute__ ((packed));
   ULONG VolumeClusters       __attribute__ ((packed));
   ULONG SegmentClusterStart  __attribute__ ((packed)); // Logical Block Segment Start
   ULONG FirstFAT             __attribute__ ((packed));            // FAT index for 1st FAT Table
   ULONG SecondFAT            __attribute__ ((packed));           // FAT index for 2nd FAT Table
   ULONG FirstDirectory       __attribute__ ((packed));      // FAT index for 1st Directory Block
   ULONG SecondDirectory      __attribute__ ((packed));     // FAT index for 2nd Directory Block
   ULONG Padding              __attribute__ ((packed));
} VOLUME_TABLE_ENTRY;

typedef struct _VOLUME_TABLE
{
   BYTE VolumeTableSignature[16]   __attribute__ ((packed));  // length preceded string here
   ULONG NumberOfTableEntries      __attribute__ ((packed));
   ULONG Reserved[3]               __attribute__ ((packed));
   VOLUME_TABLE_ENTRY VolumeEntry[MAX_VOLUME_ENTRIES]  __attribute__ ((packed));
} VOLUME_TABLE;

typedef struct _HOT_FIX_BLOCK_TABLE
{
   ULONG HotFix1    __attribute__ ((packed));    // these tables are sector based offsets from
   ULONG BadBlock1  __attribute__ ((packed));
   ULONG HotFix2    __attribute__ ((packed));    // the beginning of the partition
   ULONG BadBlock2  __attribute__ ((packed));
} HOTFIX_BLOCK_TABLE;

typedef struct _HOTFIX {
   BYTE HotFixStamp[8]      __attribute__ ((packed));
   ULONG PartitionID        __attribute__ ((packed));      // Novell HDK/SDK says .....
   ULONG HotFixFlags        __attribute__ ((packed));      // (WORD / WORD - data bits/synch number)
   ULONG HotFixDateStamp    __attribute__ ((packed));  // (B/B/B/B BSize/BUMask/BShift/BMask)
   ULONG HotFixTotalSectors __attribute__ ((packed));
   ULONG HotFixSize         __attribute__ ((packed));
   HOTFIX_BLOCK_TABLE HotFixTable[MAX_HOTFIX_BLOCKS] __attribute__ ((packed));
} HOTFIX;

typedef struct _MIRROR
{
   BYTE MirrorStamp[8]      __attribute__ ((packed));
   ULONG PartitionID        __attribute__ ((packed));      // Novell's website says ....
   ULONG MirrorFlags        __attribute__ ((packed));      // (B/B/W  DataBadBits/MirrorActive/SyncNumber)
   ULONG Reserved1          __attribute__ ((packed));        // (B/B/B/B  BSize/BUMask/BShift/BMask)
   ULONG MirrorStatus       __attribute__ ((packed));     // (B/B/W DeviceBadBits/DeviceActive/SynchNumber)
   ULONG MirrorTotalSectors __attribute__ ((packed));
   ULONG MirrorGroupID      __attribute__ ((packed));
   ULONG MirrorMemberID[MAX_MIRRORS] __attribute__ ((packed));
} MIRROR;

typedef struct _FAT_ENTRY
{
   long FATIndex      __attribute__ ((packed));
   long FATCluster    __attribute__ ((packed));
} FAT_ENTRY;

typedef struct _SUBALLOC_DIR
{
    ULONG  Flag                 __attribute__ ((packed));
    ULONG  Reserved1            __attribute__ ((packed));
    BYTE   ID                   __attribute__ ((packed));
    BYTE   SequenceNumber       __attribute__ ((packed));
    BYTE   Reserved2[2]         __attribute__ ((packed));
    ULONG  SubAllocationList    __attribute__ ((packed));
    ULONG  StartingFATChain[28] __attribute__ ((packed));
} SUBALLOC_DIR;

#endif


#if (WINDOWS_NT | WINDOWS_NT_RO | WINDOWS_NT_UTIL)

/* Boot sector info (62 byte structure) */

struct BOOT_SECTOR
{
    BYTE Jmp[3];
    BYTE OEMname[8];
    WORD bps;
    BYTE SecPerClstr;
    WORD ResSectors;
    BYTE FATs;
    WORD RootDirEnts;
    WORD Sectors;
    BYTE Media;
    WORD SecPerFAT;
    WORD SecPerTrack;
    WORD Heads;
    ULONG HiddenSecs;
    ULONG HugeSecs;
    BYTE DriveNum;
    BYTE Rsvd1;
    BYTE BootSig;
    ULONG VolID;
    BYTE VolLabel[11];
    BYTE FileSysType[8];        /* 62 bytes */
};

/* Partition Table Entry info. 16 bytes */

struct PartitionTableEntry
{
    BYTE fBootable;
    BYTE HeadStart;
    BYTE SecStart;
    BYTE CylStart;
    BYTE SysFlag;
    BYTE HeadEnd;
    BYTE SecEnd;
    BYTE CylEnd;
    ULONG StartLBA;
    ULONG nSectorsTotal;
};

typedef struct _VOLUME_TABLE_ENTRY
{
   BYTE VolumeName[16];  // length preceded string format
   ULONG LastVolumeSegment;
   ULONG VolumeSignature;
   ULONG VolumeRoot;
   ULONG SegmentSectors;
   ULONG VolumeClusters;
   ULONG SegmentClusterStart; // Logical Block Segment Start
   ULONG FirstFAT;            // FAT index for 1st FAT Table
   ULONG SecondFAT;           // FAT index for 2nd FAT Table
   ULONG FirstDirectory;      // FAT index for 1st Directory Block
   ULONG SecondDirectory;     // FAT index for 2nd Directory Block
   ULONG Padding;
} VOLUME_TABLE_ENTRY;

typedef struct _VOLUME_TABLE
{
   BYTE VolumeTableSignature[16];  // length preceded string here
   ULONG NumberOfTableEntries;
   ULONG Reserved[3];
   VOLUME_TABLE_ENTRY VolumeEntry[MAX_VOLUME_ENTRIES];
} VOLUME_TABLE;

typedef struct _HOT_FIX_BLOCK_TABLE
{
   ULONG HotFix1;    // these tables are sector based offsets from
   ULONG BadBlock1;
   ULONG HotFix2;    // the beginning of the partition
   ULONG BadBlock2;
} HOTFIX_BLOCK_TABLE;

typedef struct _HOTFIX {
   BYTE HotFixStamp[8];
   ULONG PartitionID;      // Novell HDK/SDK says .....
   ULONG HotFixFlags;      // (WORD / WORD - data bits/synch number)
   ULONG HotFixDateStamp;  // (B/B/B/B BSize/BUMask/BShift/BMask)
   ULONG HotFixTotalSectors;
   ULONG HotFixSize;
   HOTFIX_BLOCK_TABLE HotFixTable[MAX_HOTFIX_BLOCKS];
} HOTFIX;

typedef struct _MIRROR
{
   BYTE MirrorStamp[8];
   ULONG PartitionID;      // Novell's website says ....
   ULONG MirrorFlags;      // (B/B/W  DataBadBits/MirrorActive/SyncNumber)
   ULONG Reserved1;        // (B/B/B/B  BSize/BUMask/BShift/BMask)
   ULONG MirrorStatus;     // (B/B/W DeviceBadBits/DeviceActive/SynchNumber)
   ULONG MirrorTotalSectors;
   ULONG MirrorGroupID;
   ULONG MirrorMemberID[MAX_MIRRORS];
} MIRROR;

typedef struct _FAT_ENTRY
{
   long FATIndex;
   long FATCluster;
} FAT_ENTRY;

typedef struct _SUBALLOC_DIR
{
    ULONG  Flag;
    ULONG  Reserved1;
    BYTE  ID;
    BYTE  SequenceNumber;
    BYTE  Reserved2[2];
    ULONG  SubAllocationList;
    ULONG  StartingFATChain[28];
} SUBALLOC_DIR;

#endif

#endif

