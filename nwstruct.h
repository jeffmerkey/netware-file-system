
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
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWSTRUCT.H
*   DESCRIP  :   File System Structures
*   DATE     :  January 20, 2022
*
*
***************************************************************************/

#ifndef _NWFS_NWSTRUCT_
#define _NWFS_NWSTRUCT_

// directory object attributes

#define NW_SUBDIRECTORY_FILE  0x01
#define NW_SYMBOLIC_FILE      0x02
#define NW_HARD_LINKED_FILE   0x04
#define NW_NAMED_FILE         0x08
#define NW_DEVICE_FILE        0x10

typedef struct _NWDISK
{
   void *PhysicalDiskHandle;

   LONGLONG Cylinders;
   ULONG TracksPerCylinder;
   ULONG SectorsPerTrack;
   ULONG BytesPerSector;

   struct PartitionTableEntry PartitionTable[4];
   void *PartitionContext[4];
   ULONG PartitionFlags[4];
   ULONG PartitionVersion[4];  // 1-Netware 3.x, 2-Netware 4.x/5.x

   LONGLONG driveSize;
   ULONG DiskNumber;
   ULONG ChangeStatus;
   ULONG NumberOfPartitions;

#if (DOS_UTIL | WINDOWS_98_UTIL)
   ULONG Int13Extensions;

   int DataSelector;
   WORD DataSegment;
   int DriveInfoSelector;
   WORD DriveInfoSegment;
   int RequestSelector;
   WORD RequestSegment;
#endif

#if (LINUX_20 || LINUX_22 || LINUX_24)
   struct inode inode;
   struct file filp;
   ULONG DeviceBlockSize; 
#endif
   WORD partitionSignature;

} NWDISK;

typedef struct _BIT_BLOCK
{
   struct _BIT_BLOCK *next;
   struct _BIT_BLOCK *prior;
   BYTE *BlockBuffer;
   ULONG BlockIndex;
} BIT_BLOCK;

typedef struct _BIT_BLOCK_HEAD
{
   BIT_BLOCK *Head;
   BIT_BLOCK *Tail;
   BYTE Name[16];
#if (PROFILE_BIT_SEARCH)
   ULONG bit_search_count;
   ULONG skip_search_count;
#endif
   ULONG BlockSize;
   ULONG Count;
   ULONG Limit;
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
   spinlock_t BB_spinlock;
   ULONG BB_flags;
#else
   struct semaphore BBSemaphore;
#endif
#endif

} BIT_BLOCK_HEAD;

// LRU Block states

#define L_AVAIL     0x0000001
#define L_FREE      0x0000002
#define L_DATAVALID 0x0000004
#define L_DIRTY     0x0000008
#define L_FLUSHING  0x0000010
#define L_LOADING   0x0000020
#define L_UPTODATE  0x0000040
#define L_MAPPED    0x0000080
#define L_MODIFIED  0x0000100

// LRU List States

#define QUEUE_LRU    0x0000001
#define QUEUE_FREE   0x0000002
#define QUEUE_DIRTY  0x0000004
#define QUEUE_HASH   0x0000008
#define QUEUE_ALL    (QUEUE_LRU | QUEUE_FREE | QUEUE_DIRTY)

#define ASYNCH_READ   0x1111
#define ASYNCH_WRITE  0x2222
#define ASYNCH_FILL   0x4444
#define ASYNCH_TEST   0x8888

#define NWFS_FLUSH_ALL        1

#define ASIO_BAD_COMMAND     -10
#define ASIO_IO_ERROR        -11
#define ASIO_BAD_PARM        -12
#define ASIO_BAD_SIGNATURE   -13
#define ASIO_SUBMIT_IO       0xBEEFBEEF
#define ASIO_AIO_POST        0xCAFECAFE
#define ASIO_CALLBACK_POST   0xCAFEBEEF
#define ASIO_COMPLETED       0xDEEDBEEF
#define ASIO_SUBMIT_CB       0xFEEDBEEF

#define ASIO_INTR_CALLBACK   0x01
#define ASIO_SLEEP_CALLBACK  0x02

typedef	struct _ASYNCH_IO
{
    struct _ASYNCH_IO *next;
    struct _ASYNCH_IO *prior;
    struct _ASYNCH_IO *hnext;
    struct _ASYNCH_IO *hprior;
    ULONG (*call_back_routine)(struct _ASYNCH_IO *);
    ULONG call_back_parameter;
    ULONG flags;
    ULONG command;
    ULONG disk;
    ULONG sector_offset;
    ULONG sector_count;
    BYTE *buffer;
    ULONG return_code;
    ULONG ccode;
    ULONG signature;
    ULONG count;
    ULONG complete;
#if (LINUX_22 | LINUX_24)
    struct buffer_head *bh[8];
#endif
} ASYNCH_IO;

typedef	struct _ASYNCH_IO_HEAD
{
    ASYNCH_IO *hash_head[16];
    ASYNCH_IO *hash_tail[16];
} ASYNCH_IO_HEAD;

typedef struct _ASYNCH_HASH_LIST
{
    ASYNCH_IO *head;
    ASYNCH_IO *tail;
} ASYNCH_HASH_LIST;


typedef struct _MIRROR_LRU
{
   struct _MIRROR_LRU *next;
   struct _MIRROR_LRU *prior;
   struct _MIRROR_LRU *hashNext;
   struct _MIRROR_LRU *hashPrior;
   ULONG BlockIndex;
   ULONG BlockState;
   ULONG Cluster1;
   ULONG Cluster2;
   void *CacheBuffer;
   ULONG ClusterSize;
   ULONG ModLock;

} MIRROR_LRU;

typedef struct _LRU_HASH_LIST
{
   MIRROR_LRU *head;
   MIRROR_LRU *tail;
} LRU_HASH_LIST;

#define WRITE_THROUGH_LRU    1
#define WRITE_BACK_LRU       2

typedef struct _LRU
{
   struct _LRU *listnext;
   struct _LRU *listprior;
   struct _LRU *next;
   struct _LRU *prior;
   struct _LRU *dnext;
   struct _LRU *dprior;
   struct _LRU *hashNext;
   struct _LRU *hashPrior;
   ULONG state;
   ULONG owner;
   ULONG block;
   ULONG volnum;
   ULONG nwvp_handle;
   BYTE *buffer;
   ULONG timestamp;
   ULONG bad_bits;
   ULONG io_signature;
   
#if (LINUX_SLEEP)
   struct semaphore Semaphore;
#endif

   ASYNCH_IO io[8];
   ULONG lba[8];
   ULONG disk[8];
   ULONG mirror_count;
   ULONG mirrors_completed;
   ULONG aio_ccode;
   ULONG lock_count;
   ULONG Waiters;
   void *lru_handle;
   ULONG AGING_INTERVAL;
   
#if (LINUX_24)
   struct page *page;
#endif
   
} LRU;

typedef struct _DATA_LRU_HASH_LIST
{
   LRU *head;
   LRU *tail;
} DATA_LRU_HASH_LIST;

typedef struct _VOLUME_WORKSPACE
{
   struct _VOLUME_WORKSPACE *next;
   struct _VOLUME_WORKSPACE *prior;
   BYTE Buffer[1];
} VOLUME_WORKSPACE;

typedef struct _VOLUME
{
   // base volume information

   ULONG VolumeNumber;
   ULONG VolumeNameLength;
   BYTE VolumeName[16];
   ULONG VolumeDisk;
   ULONG VolumeClusters;
   ULONG MountedVolumeClusters;
   ULONG VolumeSectors;
   ULONG VolumeFlags;          // from ROOT directory entry
   ULONG VolumeSuballocRoot;   // from ROOT directory entry
   ULONG ClusterSize;
   ULONG BlockSize;
   ULONG BlocksPerCluster;
   ULONG SectorsPerCluster;
   ULONG SectorsPerBlock;
   ULONG VolumeStartLBA;
   ULONG FirstFAT;
   ULONG SecondFAT;
   ULONG FirstDirectory;
   ULONG SecondDirectory;
   ULONG ExtDirectory1;
   ULONG ExtDirectory2;
   ULONG NumberOfFATEntries;
   ULONG VolumeSerialNumber;
   ULONG EndingDirCluster1;
   ULONG EndingDirCluster2;
   ULONG EndingDirIndex;
   ULONG EndingBlockNo;
   ULONG LastValidBlockNo;
   ULONG MirrorFlag;
   ULONG nwvp_handle;
   ULONG ScanFlag;
   ULONG DeletedDirNo;
   ULONG InUseCount;
   ULONG AutoRepair;
   
   ULONG volume_segments_ok_flag;
   ULONG fat_mirror_ok_flag;
   ULONG fat_table_ok_flag;
   ULONG directory_table_ok_flag;

   ULONG DirectoryCount;
   ULONG FreeDirectoryCount;
   ULONG FreeDirectoryBlockCount;

   VOLUME_WORKSPACE *WKHead;
   VOLUME_WORKSPACE *WKTail;
   ULONG WKCount;

#if (LINUX_SLEEP)
#if (LINUX_SPIN)
   spinlock_t WK_spinlock;
   ULONG WK_flags;
#else
   struct semaphore WKSemaphore;
#endif
#endif

   VOLUME_WORKSPACE *NSHead;
   VOLUME_WORKSPACE *NSTail;
   ULONG NSCount;

#if (LINUX_SLEEP)
#if (LINUX_SPIN)
   spinlock_t NS_spinlock;
   ULONG NS_flags;
#else
   struct semaphore NSSemaphore;
#endif
#endif

   // these fields temporarily hold hash elements during mount
   HASH *HashMountHead;
   HASH *HashMountTail;

   // these fields temporarily hold deleted hash elements during mount
   HASH *DelHashMountHead;
   HASH *DelHashMountTail;

   // file allocation table LRU tables

   MIRROR_LRU *FATBlockHash;
   ULONG FATBlockHashLimit;
   MIRROR_LRU *FATListHead;
   MIRROR_LRU *FATListTail;
   ULONG FATListBlocks;
   ULONG MinimumFATBlocks;
   ULONG MaximumFATBlocks;

   // logical directory block hash

   DIR_BLOCK_HASH *DirBlockHash;
   ULONG DirBlockHashLimit;
   ULONG DirListBlocks;

   // directory record assignment hash

   DIR_ASSIGN_HASH *DirAssignHash;
   ULONG DirAssignHashLimit;
   ULONG DirAssignBlocks;

   // segment table

   ULONG SegmentClusterStart[MAX_SEGMENTS];
   ULONG SegmentClusterSize[MAX_SEGMENTS];
   ULONG LastAllocatedIndex[MAX_SEGMENTS];
   ULONG AllocSegment;
   ULONG MountedNumberOfSegments;
   ULONG NumberOfSegments;
   ULONG CurrentSegments;
   ULONG VolumePresent;

   // bit block free tables

   BIT_BLOCK_HEAD FreeBlockList;
   BIT_BLOCK_HEAD *ExtDirList;
   ULONG ExtDirTotalBlocks;
   ULONG ExtDirSearchIndex;
   BIT_BLOCK_HEAD AssignedBlockList;

   // volume name space information

   ULONG NameSpaceCount;
   ULONG NameSpaceID[MAX_NAMESPACES];
   ULONG NameSpaceDefault;

   // name hash tables

   void *VolumeNameHash[MAX_NAMESPACES];
   ULONG VolumeNameHashLimit[MAX_NAMESPACES];

   // directory hash tables

   void *DirectoryNumberHash;
   ULONG DirectoryNumberHashLimit;

   // parent hash tables

   void *ParentHash;
   ULONG ParentHashLimit;

   // extended directory hash tables

   void *ExtDirHash;
   ULONG ExtDirHashLimit;

   // trustee hash tables

   void *TrusteeHash;
   ULONG TrusteeHashLimit;

   // user restriction tables

   void *UserQuotaHash;
   ULONG UserQuotaHashLimit;

   // volume suballocation tables

   ULONG SuballocListCount;
   ULONG SuballocCount;
   ULONG SuballocCurrentCount;
   ULONG SuballocChainComplete;
   ULONG SuballocChainFlag[MAX_SUBALLOC_NODES];
   ULONG SuballocDirNo[MAX_SUBALLOC_NODES];
   SUBALLOC_DIR *SuballocChain[MAX_SUBALLOC_NODES];
   BIT_BLOCK_HEAD *FreeSuballoc[128];
   ULONG SuballocTotalBlocks[128];
   ULONG SuballocAssignedBlocks[128];
   ULONG SuballocTurboFATCluster[128];
   ULONG SuballocTurboFATIndex[128];
   ULONG SuballocSearchIndex[128];

#if (LINUX_SLEEP)
   struct semaphore SuballocSemaphore[128];
#endif

   // volume stats

   ULONG VolumeAllocatedClusters;
   ULONG VolumeFreeClusters;

   // salvageable file system

   ULONG MountTime;
   ULONG FileSeed;
   ULONG dblock_sequence;
   DIR_BLOCK_HASH *dblock_head;
   DIR_BLOCK_HASH *dblock_tail;
   
   // synch primitives

#if (LINUX_SLEEP)
#if (LINUX_SPIN)
   spinlock_t NameHash_spinlock;
   spinlock_t DirHash_spinlock;
   spinlock_t ParentHash_spinlock;
   spinlock_t TrusteeHash_spinlock;
   spinlock_t QuotaHash_spinlock;
   spinlock_t ExtHash_spinlock;
   spinlock_t NameSpace_spinlock;

   ULONG NameHash_flags;
   ULONG DirHash_flags;
   ULONG ParentHash_flags;
   ULONG TrusteeHash_flags;
   ULONG QuotaHash_flags;
   ULONG ExtHash_flags;
   ULONG NameSpace_flags;
#else
   struct semaphore NameHashSemaphore;
   struct semaphore DirHashSemaphore;
   struct semaphore ParentHashSemaphore;
   struct semaphore TrusteeHashSemaphore;
   struct semaphore QuotaHashSemaphore;
   struct semaphore ExtHashSemaphore;
   struct semaphore NameSpaceSemaphore;
#endif

   struct semaphore FatSemaphore;
   struct semaphore VolumeSemaphore;
   struct semaphore ExtSemaphore;
   struct semaphore DirBlockSemaphore;
   struct semaphore DirAssignSemaphore;
#endif

   ULONG gid;
   ULONG uid;
   ULONG mode;

} VOLUME;

typedef struct _CACHE_AIO
{
   struct _CACHE_AIO *next;
   struct _CACHE_AIO *prior;
   struct _CACHE_AIO *dnext;
   struct _CACHE_AIO *dprior;
   struct _CACHE_AIO *head; 
   ULONG state;
   ULONG block;
   ULONG fblock;
   ULONG vblock;
   ULONG blocks;
   ULONG cache_lock;
   ULONG owner;
   BYTE *buffer;
   VOLUME *volume;
   HASH *hash;
   ULONG bad_bits;
   ULONG io_signature;
   
   ASYNCH_IO io[8];
   ULONG lba[8];
   ULONG disk[8];
   ULONG mirror_count;
   ULONG mirrors_completed;
   ULONG aio_ccode;
#if (LINUX_22 | LINUX_24)
   struct page *page;

#if (LINUX_SLEEP)
   struct semaphore cache_aio_semaphore;
#endif

#endif
   struct _CACHE_AIO *table[1];

} CACHE_AIO;

typedef	struct	_DISK_INFO
{
    ULONG disk_number;
    ULONG partition_count;
    ULONG total_sectors;
    ULONG cylinder_size;

    ULONG track_size;
    ULONG bytes_per_sector;
    ULONG filler1;
    ULONG filler2;
} DISK_INFO;

typedef	struct _PARTITION_INFO
{
    ULONG disk_number;
    ULONG partition_number;
    ULONG total_sectors;
    ULONG cylinder_size;
    ULONG partition_type;
    ULONG boot_flag;
    ULONG sector_offset;
    ULONG sector_count;
} PARTITION_INFO;

typedef struct _LRU_HANDLE
{
   ULONG LRUMode;
   BYTE *LRUName;
   LRU *LRUListHead;
   LRU *LRUListTail;
   ULONG MinimumLRUBlocks;
   ULONG MaximumLRUBlocks;
   ULONG LocalLRUBlocks;
   ULONG AGING_INTERVAL;
   
   ULONG dirty;
   ULONG loading;
   ULONG flushing;
   ULONG locked;
   ULONG valid;
   ULONG uptodate;
   ULONG total;
   ULONG free;

} LRU_HANDLE;

typedef struct segment_table_def
{
    ULONG lpart_handle;
    ULONG segment_offset;
    ULONG segment_count;
    ULONG free_flag;
    ULONG segment_number;
    ULONG total_segments;
    BYTE  VolumeName[20];
    ULONG disk;
} segment_info_table;

typedef struct hotfix_table_def
{
    ULONG lpart_handle;
    ULONG rpart_handle;
    ULONG segment_count;
    ULONG mirror_count;

    ULONG insynch_flag;
    ULONG hotfix_block_count;
    ULONG logical_block_count;
    ULONG physical_block_count;

    ULONG active_flag;
    ULONG group_number;
} hotfix_info_table;

#define TYPE_SIGNATURE    0x000000165

typedef struct _NETWARE_PART_SIG
{
    BYTE NetwareSignature[16];
    ULONG PartitionType;
    ULONG PartitionSize;
    ULONG CreationDateAndTime;
} PART_SIG;


#if (DOS_UTIL | WINDOWS_98_UTIL)

#define PARAGRAPH_ALIGN(size) ((size + 15) / 16)

typedef struct _DRIVE_INFO
{
    WORD     Size                   __attribute__((packed));
    WORD     InfoFlags              __attribute__((packed));
    ULONG    Cylinders              __attribute__((packed));
    ULONG    Heads                  __attribute__((packed));
    ULONG    SectorsPerTrack        __attribute__((packed));
    LONGLONG TotalSectors           __attribute__((packed));
    WORD     BytesPerSector         __attribute__((packed));
    ULONG    DeviceParameterTable   __attribute__((packed));
    WORD     DevicePathInfo         __attribute__((packed));
    BYTE     DevicePathInfoLen      __attribute__((packed));
    BYTE     Reserved1              __attribute__((packed));
    WORD     Reserved2              __attribute__((packed));
    BYTE     BusType[4]             __attribute__((packed));
    BYTE     InterfaceType[8]       __attribute__((packed));
    LONGLONG InterfacePath          __attribute__((packed));
    LONGLONG DevicePath             __attribute__((packed));
    BYTE     Reserved3              __attribute__((packed));
    BYTE     DevicePathInfoCheckSum __attribute__((packed));
} DRIVE_INFO;

typedef struct _EXT_REQUEST
{
    BYTE Size             __attribute__((packed));
    BYTE Reserved1        __attribute__((packed));
    BYTE Blocks           __attribute__((packed));
    BYTE Reserved2        __attribute__((packed));
    ULONG TransferBuffer  __attribute__((packed));
    LONGLONG LBA          __attribute__((packed));
    LONGLONG *FlatBuffer  __attribute__((packed));
} EXT_REQUEST;

#endif

#endif

