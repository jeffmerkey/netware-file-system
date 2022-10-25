
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
*   FILE     :  NWPROC.H
*   DESCRIP  :   External Declarations
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#ifndef _NWFS_EXTERNS_
#define _NWFS_EXTERNS_

extern ULONG MajorVersion;
extern ULONG MinorVersion;
extern ULONG BuildVersion;
extern ULONG exitFlag;
extern ULONG consoleHandle;
extern ULONG MaximumNumberOfVolumes;
extern ULONG NumberOfVolumes;
extern ULONG TotalDisks;
extern ULONG MaximumDisks;
extern ULONG TotalNWParts;
extern ULONG MaximumNWParts;
extern ULONG InstanceCounter;
extern ULONG LogicalBlockSize;
extern NWDISK *SystemDisk[MAX_DISKS];
extern VOLUME *VolumeTable[MAX_VOLUMES];
extern ULONG VolumeMountedTable[MAX_VOLUMES];

extern ULONG MemoryAllocated;
extern ULONG MemoryFreed;
extern ULONG MemoryInUse;
extern ULONG PagedMemoryAllocated;
extern ULONG PagedMemoryFreed;
extern ULONG PagedMemoryInUse;
extern BYTE *ZeroBuffer;
extern BYTE NwPartSignature[16];
extern ULONG (*RemirrorProcessPtr)(ULONG);
extern ULONG cHandle;

#if (LINUX_UTIL)
extern ULONG HasColor;
#endif

extern ULONG HOTFIX_TRACK_OFFSET[MULTI_TRACK_COUNT];
extern ULONG MIRROR_TRACK_OFFSET[MULTI_TRACK_COUNT];
extern ULONG VOLTABLE_TRACK_OFFSET[MULTI_TRACK_COUNT];

extern ULONG DiskDevices[];
extern MIRROR_LRU *VolumeLRUListHead;
extern MIRROR_LRU *VolumeLRUListTail;

extern ASYNCH_IO *aio_free_head;
extern ASYNCH_IO *aio_free_tail;
extern ULONG aio_free_count;

extern ULONG DataLRUActive;
extern ULONG JournalLRUActive;
extern ULONG FATLRUActive;
extern LRU_HANDLE DataLRU;
extern LRU_HANDLE JournalLRU;
extern LRU_HANDLE FATLRU;

extern void InitNWFS(void);
extern void CloseNWFS(void);

extern void NWLockNWVP(void);
extern void NWUnlockNWVP(void);
extern void NWLockHOTFIX(void);
extern void NWUnlockHOTFIX(void);
extern void NWFSVolumeScan(void);
extern void NWFSVolumeClose(void);
extern void NWFSRemirror(void);
extern void NWFSFlush(void);
extern void NWEditVolumes(void);

extern void FreeWorkspace(VOLUME *volume, VOLUME_WORKSPACE *wk);
extern VOLUME_WORKSPACE *AllocateWorkspace(VOLUME *volume);
extern void FreeNSWorkspace(VOLUME *volume, VOLUME_WORKSPACE *wk);
extern VOLUME_WORKSPACE *AllocateNSWorkspace(VOLUME *volume);

// CONSOLE.C

extern void ClearScreen(ULONG context);
extern void WaitForKey(void);

extern ULONG AllocateSemaphore(void *sema, ULONG value);
extern long WaitOnSemaphore(void *sema);
extern long SignalSemaphore(void *sema);

extern ULONG SetPartitionTableGeometry(BYTE *hd, BYTE *sec, BYTE *cyl,
				       ULONG sector, ULONG diskHeads,
				       ULONG diskSectors);
extern ULONG SetPartitionTableValues(struct PartitionTableEntry *Part,
				     ULONG Type, ULONG StartingSector,
				     ULONG EndingSector, ULONG Flag,
				     ULONG diskHeads, ULONG diskSectors);
extern ULONG ValidatePartitionExtants(ULONG disk);

extern BYTE *get_partition_type(ULONG PartitionType);
extern void GetNumberOfDisks(ULONG *disk_count);
extern ULONG GetDiskInfo(ULONG disk_number, DISK_INFO *disk_info);
extern ULONG GetPartitionInfo(ULONG disk_number, ULONG partition_number,
			      PARTITION_INFO *part_info);
extern ULONG SetPartitionInfo(ULONG disk_number, PARTITION_INFO *part);
extern ULONG NWAllocateUnpartitionedSpace(void);
extern void NWEditPartitions(void);

extern ULONG ReadPhysicalVolumeCluster(VOLUME *volume, ULONG Cluster, BYTE *Buffer,
					ULONG size, ULONG priority);
extern ULONG WritePhysicalVolumeCluster(VOLUME *volume, ULONG Cluster, BYTE *Buffer,
					ULONG size, ULONG priority);
extern ULONG ZeroPhysicalVolumeCluster(VOLUME *volume, ULONG Cluster, ULONG priority);
extern ULONG ReadClusterWithOffset(VOLUME *volume, ULONG Cluster, ULONG offset,
				   BYTE *buf, ULONG size, ULONG as,
				   ULONG *retCode, ULONG priority);
extern ULONG WriteClusterWithOffset(VOLUME *volume, ULONG Cluster, ULONG offset,
				    BYTE *buf, ULONG size, ULONG as,
				    ULONG *retCode, ULONG priority);

extern ULONG ReadAbsoluteVolumeCluster(VOLUME *volume, ULONG Cluster, 
		                       BYTE *Buffer);
extern ULONG WriteAbsoluteVolumeCluster(VOLUME *volume, ULONG Cluster, 
		                       BYTE *Buffer);

extern ULONG FreeVolumeLRU(VOLUME *volume);
extern void NWLockLRU(ULONG whence);
extern void NWUnlockLRU(ULONG whence);
extern void NWLockSync(void);
extern void NWUnlockSync(void);
extern ULONG NWLockBuffer(LRU *lru);
extern void NWUnlockBuffer(LRU *lru);
extern void FreeLRUElement(LRU *lru);
extern LRU *AllocateLRUElement(VOLUME *volume, ULONG block, ULONG *ccode);
extern LRU *RemoveDirty(LRU *lru);
extern void InsertDirty(LRU *lru);
extern LRU *RemoveLRU(LRU_HANDLE *lru_handle, LRU *lru);
extern void InsertLRU(LRU_HANDLE *lru_handle, LRU *lru);
extern void InsertLRUTop(LRU_HANDLE *lru_handle, LRU *lru);
extern LRU *GetFreeLRU(void);
extern void PutFreeLRU(LRU *lru);
extern LRU *SearchHash(VOLUME *volume, ULONG block);
extern ULONG AddToHash(LRU *lru);
extern ULONG RemoveHash(LRU *lru);
extern ULONG InitializeLRU(LRU_HANDLE *lru_handle, BYTE *name, ULONG mode,
			   ULONG max, ULONG min, ULONG age);
extern ULONG FreeLRU(void);
extern ULONG FlushLRU(void);
extern ULONG FlushEligibleLRU(void);
extern ULONG FlushVolumeLRU(VOLUME *volume);
extern ULONG ReleaseLRU(LRU_HANDLE *lru_handle, LRU *lru);
extern ULONG ReleaseDirtyLRU(LRU_HANDLE *lru_handle, LRU *lru);

extern LRU *ReadLRU(LRU_HANDLE *lru_handle, VOLUME *volume, ULONG block,
		    ULONG fill, ULONG ccount, ULONG readAhead, ULONG *mirror);
extern ULONG SyncLRUBlocks(VOLUME *volume, ULONG block, ULONG blocks);
extern ULONG PerformBlockReadAhead(LRU_HANDLE *lru_handle, VOLUME *volume, ULONG block,
				   ULONG blockReadAhead, ULONG ccount,
				   ULONG *mirror);

extern MIRROR_LRU *RemoveFAT_LRU(VOLUME *volume, MIRROR_LRU *lru);
extern void InsertFAT_LRU(VOLUME *volume, MIRROR_LRU *lru);
extern void InsertFAT_LRUTop(VOLUME *volume, MIRROR_LRU *lru);
extern ULONG InitializeFAT_LRU(VOLUME *volume);
extern ULONG FreeFAT_LRU(VOLUME *volume);
extern ULONG FlushFAT_LRU(VOLUME *volume);
extern MIRROR_LRU *SearchFAT_LRU(VOLUME *volume, ULONG Cluster);
extern ULONG AddToFATHash(VOLUME *volume, MIRROR_LRU *lru);
extern ULONG RemoveFATHash(VOLUME *volume, MIRROR_LRU *lru);
extern void FreePartitionResources(void);
extern void RemoveDiskDevices(void);
extern void ScanDiskDevices(void);


// VOLUME.C

extern void displayNetwareVolumes(void);
extern void ReportVolumes(void);
extern ULONG MountAllVolumes(void);
extern ULONG DismountAllVolumes(void);
extern ULONG DismountVolumeByHandle(VOLUME *volume);
extern ULONG MountVolumeByHandle(VOLUME *volume);
extern VOLUME *MountHandledVolume(BYTE *name);
extern ULONG DismountVolume(BYTE *name);
extern ULONG MountVolume(BYTE *name);
extern ULONG MountUtilityVolume(VOLUME *volume);
extern ULONG DismountUtilityVolume(VOLUME *volume);
extern ULONG MountRawVolume(VOLUME *volume);
extern ULONG DismountRawVolume(VOLUME *volume);
extern VOLUME *GetVolumeHandle(BYTE *VolumeName);
extern ULONG CreateRootDirectory(BYTE *Buffer, ULONG size, ULONG VolumeFlags,
				 ULONG ClusterSize);

// HASH.C

extern ULONG InsertNameSpaceElement(VOLUME *volume, HASH *hash);
extern HASH *RemoveNameSpaceElement(VOLUME *volume, HASH *hash);
extern HASH *AllocHashNode(void);
extern void FreeHashNode(HASH *hash);
extern void FreeHashNodes(void);
extern HASH *CreateHashNode(DOS *dos, ULONG DirNo);
extern BYTE *CreateNameField(DOS *dos, HASH *hash);
extern ULONG RemoveNameHash(VOLUME *volume, HASH *hash);
extern ULONG AddToNameHash(VOLUME *volume, HASH *hash);
extern ULONG RemoveDirectoryHash(VOLUME *volume, HASH *hash);
extern ULONG AddToDirectoryHash(VOLUME *volume, HASH *hash);
extern ULONG RemoveParentHash(VOLUME *volume, HASH *hash);
extern ULONG AddToParentHash(VOLUME *volume, HASH *hash);
extern ULONG RemoveMountHash(VOLUME *volume, HASH *hash);
extern ULONG AddToMountHash(VOLUME *volume, HASH *hash);
extern ULONG AddToDelMountHash(VOLUME *volume, HASH *hash);
extern ULONG RemoveDelMountHash(VOLUME *volume, HASH *hash);
extern ULONG RemoveDeletedHash(VOLUME *volume, HASH *hash);
extern ULONG AddToDeletedHash(VOLUME *volume, HASH *hash);
extern ULONG ProcessOrphans(VOLUME *volume);

extern ULONG AddTrusteeToDirectoryHash(VOLUME *volume, THASH *name);
extern ULONG AddUserQuotaToDirectoryHash(VOLUME *volume, UHASH *name);
extern ULONG RemoveTrusteeFromDirectoryHash(VOLUME *volume, THASH *name);
extern ULONG RemoveUserQuotaFromDirectoryHash(VOLUME *volume, UHASH *name);
extern ULONG AddToTrusteeHash(VOLUME *volume, THASH *name);
extern ULONG AddToUserQuotaHash(VOLUME *volume, UHASH *name);
extern ULONG RemoveTrusteeHash(VOLUME *volume, THASH *name);
extern ULONG RemoveUserQuotaHash(VOLUME *volume, UHASH *name);
extern ULONG AddToExtHash(VOLUME *volume, EXTENDED_DIR_HASH *name);
extern ULONG RemoveExtHash(VOLUME *volume, EXTENDED_DIR_HASH *name);

extern ULONG LinkVolumeNameSpaces(VOLUME *volume);

extern ULONG HashDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo);
extern HASH *AllocHashDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo);
extern HASH *GetHashFromName(VOLUME *v, BYTE *s, ULONG len, ULONG ns, ULONG cd);
extern HASH *GetHashFromDirectoryNumber(VOLUME *v, ULONG d);
extern HASH *GetHashNameSpaceEntry(VOLUME *v, ULONG entry, ULONG NameSpace,
		                   ULONG dir, HASH **DirHash);
extern ULONG GetHashCountFromDirectory(VOLUME *volume, ULONG Directory, ULONG *count);
extern HASH *GetHashFromNameUppercase(VOLUME *volume, BYTE *PathString, ULONG len,
			       ULONG NameSpace, ULONG CurrentDirectory);
extern ULONG NWFSStringHash(BYTE *v, ULONG len, ULONG M);

extern ULONG get_directory_number(VOLUME *volume, const char *name, ULONG len, ULONG DirNo);
extern HASH *get_directory_hash(VOLUME *volume, const char *name, ULONG len, ULONG Parent);
extern HASH *get_directory_record(VOLUME *volume, ULONG ino);
extern HASH *get_subdirectory_record(VOLUME *volume, ULONG entry,
				     ULONG ino, HASH **dir);
extern ULONG get_parent_directory_number(VOLUME *volume, ULONG ino);
extern ULONG get_namespace_directory_number(VOLUME *, HASH *root, 
		                            ULONG NameSpace);
extern ULONG get_namespace_dir_record(VOLUME *volume, DOS *dos, HASH *hash, 
		               ULONG NameSpace);
extern ULONG is_deleted(VOLUME *volume, HASH *hash);
extern ULONG is_deleted_file(VOLUME *volume, HASH *hash);
extern ULONG is_deleted_dir(VOLUME *volume, HASH *hash);

// CLUSTER.C

extern ULONG InitializeClusterFreeList(VOLUME *volume);
extern ULONG ExtendClusterFreeList(VOLUME *volume, ULONG Amount);
extern ULONG FreeClusterFreeList(VOLUME *volume);
extern ULONG GetFreeClusterValue(VOLUME *volume, ULONG Cluster);
extern ULONG SetFreeClusterValue(VOLUME *volume, ULONG Cluster, ULONG flag);
extern ULONG FindFreeCluster(VOLUME *volume, ULONG Cluster);
extern ULONG InitializeClusterAssignedList(VOLUME *volume);
extern ULONG ExtendClusterAssignedList(VOLUME *volume, ULONG Amount);
extern ULONG FreeClusterAssignedList(VOLUME *volume);
extern ULONG GetAssignedClusterValue(VOLUME *volume, ULONG Cluster);
extern ULONG SetAssignedClusterValue(VOLUME *volume, ULONG Cluster, ULONG flag);
extern ULONG FindAssignedCluster(VOLUME *volume, ULONG Cluster);

// BIT.C

extern BIT_BLOCK *AllocateBitBlock(ULONG BlockSize);
extern void FreeBitBlock(BIT_BLOCK *blk);
extern BIT_BLOCK *RemoveBitBlock(BIT_BLOCK_HEAD *bb, BIT_BLOCK *blk);
extern BIT_BLOCK *InsertBitBlock(BIT_BLOCK_HEAD *bb, BIT_BLOCK *blk);
extern ULONG GetBitBlockValue(BIT_BLOCK_HEAD *bb, ULONG Index);
extern ULONG SetBitBlockValue(BIT_BLOCK_HEAD *bb, ULONG Index, ULONG Value);
extern ULONG SetBitBlockValueWithRange(BIT_BLOCK_HEAD *bb, ULONG Index, 
		                       ULONG Value, ULONG Range);
extern ULONG CreateBitBlockList(BIT_BLOCK_HEAD *bb, ULONG Limit, 
		                ULONG BlockSize, BYTE *Name);
extern ULONG FreeBitBlockList(BIT_BLOCK_HEAD *bb);
extern ULONG AdjustBitBlockList(BIT_BLOCK_HEAD *bb, ULONG NewLimit);
extern ULONG ScanBitBlockValueWithIndex(BIT_BLOCK_HEAD *bb, ULONG Value,
					ULONG Index, ULONG Range);
extern ULONG ScanAndSetBitBlockValueWithIndex(BIT_BLOCK_HEAD *bb, ULONG Value,
					      ULONG Index, ULONG Range);
extern ULONG GetBitBlockLimit(BIT_BLOCK_HEAD *bb);
extern ULONG GetBitBlockBlockSize(BIT_BLOCK_HEAD *bb);
extern ULONG InitBitBlockList(BIT_BLOCK_HEAD *bb, ULONG value);

// NWDIR.C

extern ULONG AllocateDirectoryRecord(VOLUME *volume, ULONG Parent);
extern ULONG ReadDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo);
extern ULONG WriteDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo);
extern ULONG FreeDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo, ULONG Parent);
extern ULONG ReleaseDirectoryRecord(VOLUME *volume, ULONG DirNo, ULONG Parent);
extern ULONG PreAllocateFreeDirectoryRecords(VOLUME *volume);
extern ULONG PreAllocateEmptyDirectorySpace(VOLUME *volume);

// NWEXT.C

extern ULONG AllocateExtDirectoryRecords(VOLUME *volume, ULONG count);
extern ULONG ReadExtDirectoryRecords(VOLUME *volume, EXTENDED_DIR *, 
                                     BYTE *data, ULONG DirNo, ULONG count);
extern ULONG WriteExtDirectoryRecords(VOLUME *volume, EXTENDED_DIR *, 
		                     BYTE *data, ULONG DirNo, ULONG count);
extern ULONG FreeExtDirectoryRecords(VOLUME *volume, ULONG DirNo, ULONG count);
extern ULONG ReleaseExtDirectoryRecords(VOLUME *volume, ULONG DirNo, ULONG count);

// SUBALLOC.C

extern ULONG AllocateSuballocRecord(VOLUME *volume, ULONG Size, ULONG *retCode);
extern ULONG SetSuballocListValue(VOLUME *volume, ULONG cluster, ULONG Value);
extern ULONG GetSuballocListValue(VOLUME *volume, ULONG SAEntry);
extern ULONG FreeSuballocRecord(VOLUME *volume, ULONG SAEntry);
extern ULONG GetSuballocSize(VOLUME *volume, ULONG SAEntry);
extern ULONG MapSuballocNode(VOLUME *volume, SUBALLOC_MAP *map, long Cluster);
extern ULONG ReadSuballocRecord(VOLUME *volume, long offset,
			ULONG cluster, BYTE *buf, long count,
			ULONG as, ULONG *retCode);
extern ULONG WriteSuballocRecord(VOLUME *volume, long offset,
			ULONG cluster, BYTE *buf, long count,
			ULONG as, ULONG *retCode);
extern ULONG NWCreateSuballocRecords(VOLUME *volume);

extern ULONG InitializeDirBlockHash(VOLUME *volume);
extern ULONG CreateDirBlockEntry(VOLUME *volume, ULONG cluster, ULONG mirror, ULONG index);
extern ULONG FreeDirBlockHash(VOLUME *volume);
extern ULONG AddToDirBlockHash(VOLUME *volume, DIR_BLOCK_HASH *dblock);
extern ULONG RemoveDirBlockHash(VOLUME *volume, DIR_BLOCK_HASH *dblock);

extern ULONG InitializeDirAssignHash(VOLUME *volume);
extern ULONG CreateDirAssignEntry(VOLUME *volume, ULONG Parent, ULONG BlockNo, DOS *dos);
extern ULONG FreeDirAssignHash(VOLUME *volume);
extern ULONG AddToDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock);
extern ULONG RemoveDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock);

extern FAT_ENTRY *GetFatEntry(VOLUME *volume, ULONG Cluster, FAT_ENTRY *fat);
extern FAT_ENTRY *GetFatEntryAndLRU(VOLUME *volume, ULONG Cluster,
				    MIRROR_LRU **lru, FAT_ENTRY *fat);
extern ULONG FlushFATBuffer(VOLUME *volume, MIRROR_LRU *lru, ULONG cluster);
extern ULONG WriteFATEntry(VOLUME *volume, MIRROR_LRU *lru, FAT_ENTRY *fat,
                           ULONG cluster);
extern ULONG NWUpdateFat(VOLUME *volume, FAT_ENTRY *FAT, MIRROR_LRU *lru,
			 ULONG index, ULONG value, ULONG cluster);

// NWCREATE.C

extern ULONG NWCreateDirectoryEntry(VOLUME *volume, BYTE *Name, ULONG Len,
				    ULONG Attributes, ULONG Parent,
				    ULONG Flag, ULONG Uid, ULONG Gid,
				    ULONG RDev, ULONG Mode, ULONG *retDirNo,
				    ULONG DataStream, ULONG DataStreamSize,
				    ULONG DataFork, ULONG DataForkSize,
				    const char *SymbolicLinkPath, ULONG SymbolPathLength);
extern ULONG NWDeleteDirectoryEntry(VOLUME *volume, DOS *dos, HASH *hash);
extern ULONG NWDeleteDirectoryEntryAndData(VOLUME *volume, DOS *dos, HASH *hash);
extern ULONG NWShadowDirectoryEntry(VOLUME *volume, DOS *dos, HASH *hash);
extern ULONG NWRenameEntry(VOLUME *volume, DOS *dos, HASH *hash, BYTE *Name,
			   ULONG Len, ULONG Parent);
extern ULONG NWValidDOSName(VOLUME *volume, BYTE *Name, ULONG NameLength,
			    ULONG Parent, ULONG searchFlag);
extern ULONG NWValidMACName(VOLUME *volume, BYTE *Name, ULONG NameLength,
			    ULONG Parent, ULONG searchFlag);
extern ULONG NWValidNFSName(VOLUME *volume, BYTE *Name, ULONG NameLength,
			    ULONG Parent, ULONG searchFlag);
extern ULONG NWValidLONGName(VOLUME *volume, BYTE *Name, ULONG NameLength,
			     ULONG Parent, ULONG searchFlag);
extern ULONG NWSearchDOSName(VOLUME *volume, BYTE *Name, ULONG NameLength,
			     BYTE *retName, ULONG *retNameLength, ULONG Parent);
extern ULONG NWMangleDOSName(VOLUME *volume, BYTE *Name, ULONG NameLength, DOS *dos, ULONG Parent);
extern ULONG NWMangleMACName(VOLUME *volume, BYTE *Name, ULONG NameLength, MACINTOSH *mac, ULONG Parent);
extern ULONG NWMangleNFSName(VOLUME *volume, BYTE *Name, ULONG NameLength, NFS *nfs, ULONG Parent);
extern ULONG NWMangleLONGName(VOLUME *volume, BYTE *Name, ULONG NameLength, LONGNAME *longname, ULONG Parent);
extern ULONG NWCreateUniqueName(VOLUME *volume, ULONG Parent, BYTE *name, ULONG len,
				BYTE *MangledName, BYTE *MangledNameLength,
				ULONG OpFlag);
extern ULONG FreeHashRecords(VOLUME *volume, HASH **Entries, ULONG Count);
extern ULONG FreeDirectoryRecords(VOLUME *volume, ULONG *Entries, ULONG Parent, ULONG Count);
extern ULONG CreateDirectoryRecords(VOLUME *volume, ULONG *Entries, ULONG Parent, ULONG *NumRec);
extern ULONG InitializeVolumeDirectory(VOLUME *volume);

extern ULONG NWReaderWriterLock(HASH *hash, ULONG which);
extern ULONG NWReaderWriterUnlock(HASH *hash, ULONG which);
extern ULONG NWLockFile(HASH *hash);
extern ULONG NWLockFileExclusive(HASH *hash);
extern void NWUnlockFile(HASH *hash);

extern ULONG NWReadFile(VOLUME *volume, ULONG *Chain, ULONG Flags, ULONG FileSize,
			ULONG offset, BYTE *buf, long count, long *Context,
			ULONG *Index, ULONG *retCode, ULONG as, ULONG SAFlag,
			ULONG Attributes, ULONG readAhead);

extern ULONG NWWriteFile(VOLUME *volume, ULONG *Chain, ULONG Flags,
			ULONG offset, BYTE *buf, long count, long *Context,
			ULONG *Index, ULONG *retCode, ULONG as, ULONG SAFlag,
			ULONG Attributes);

extern ULONG RemoveNameSpaceLink(VOLUME *volume, HASH *name);
extern ULONG AllocateCluster(VOLUME *volume);
extern ULONG AllocateClusterSetIndex(VOLUME *volume, ULONG Index);
extern ULONG AllocateClusterSetIndexSetChain(VOLUME *volume, ULONG Index, ULONG Next);
extern ULONG FreeCluster(VOLUME *volume, long cluster);
extern ULONG FreeClusterNoFlush(VOLUME *volume, long cluster);
extern ULONG TruncateClusterChain(VOLUME *volume, ULONG *Chain, ULONG Index,
				  ULONG PrevChain, ULONG size, ULONG SAFlag,
				  ULONG Attributes);
extern ULONG SetClusterValue(VOLUME *volume, long cluster, ULONG Value);
extern ULONG SetClusterIndex(VOLUME *volume, long cluster, ULONG Index);
extern ULONG SetClusterValueAndIndex(VOLUME *volume, long cluster, ULONG Value, ULONG Index);
extern ULONG GetChainSize(VOLUME *volume, long ClusterChain);
extern ULONG AdjustAllocatedClusters(VOLUME *volume);
extern ULONG BuildChainAssignment(VOLUME *volume, ULONG Chain, ULONG SAFlag);
extern ULONG VerifyChainAssignment(VOLUME *volume, ULONG Chain, ULONG SAFlag);

extern ULONG ReadDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			     ULONG blocks, ULONG readAhead);
extern ULONG WriteDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			      ULONG blocks, ULONG readAhead);
extern ULONG ZeroFillDiskSectors(ULONG disk, ULONG StartingLBA,
				 ULONG blocks, ULONG readAhead);

extern ULONG pReadDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			     ULONG blocks, ULONG readAhead);
extern ULONG pWriteDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			      ULONG blocks, ULONG readAhead);
extern ULONG pZeroFillDiskSectors(ULONG disk, ULONG StartingLBA,
				 ULONG blocks, ULONG readAhead);

extern ULONG aReadDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			     ULONG blocks, ULONG readAhead, ASYNCH_IO *io);
extern ULONG aWriteDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			      ULONG blocks, ULONG readAhead, ASYNCH_IO *io);
extern ULONG aZeroFillDiskSectors(ULONG disk, ULONG StartingLBA,
				 ULONG blocks, ULONG readAhead, ASYNCH_IO *io);

extern void SyncDevice(ULONG disk);
extern void SyncDisks(void);

//
//   OS Platform specific functions
//

extern void NWFSSet(void *dest, ULONG value, ULONG size);
extern void NWFSCopy(void *dest, void *src, ULONG size);
extern void *NWFSScan(void *src, ULONG value, ULONG size);
extern int  NWFSCompare(void *dest, void *src, ULONG size);
extern ULONG NWFSCopyToUserSpace(void *dest, void *src, ULONG size);
extern ULONG NWFSCopyFromUserSpace(void *dest, void *src, ULONG size);
extern ULONG NWFSSetUserSpace(void *src, ULONG value, ULONG size);
extern void *NWFSAlloc(ULONG size, TRACKING *Tag);
extern void *NWFSIOAlloc(ULONG size, TRACKING *Tag);
extern void *NWFSCacheAlloc(ULONG size, TRACKING *Tag);
extern void NWFSFree(void *p);
extern BYTE NWFSUpperCase(BYTE c);
extern BYTE NWFSLowerCase(BYTE c);
extern void FreePoolList(void);

//
//  page pool alloc routines for using virutal memory on Linux
//

extern void *NWFSVMAlloc(ULONG size);
extern void NWFSVMFree(void *p);

extern void NWLockDirectory(HASH *hash);
extern void NWUnlockDirectory(HASH *hash);
extern void NWLockFat(VOLUME *volume);
extern void NWUnlockFat(VOLUME *volume);

extern ULONG NWFSGetSeconds(void);
extern ULONG NWFSGetSystemTime(void);
extern ULONG NWFSSystemToNetwareTime(ULONG SystemTime);
extern ULONG NWFSNetwareToSystemTime(ULONG NetwareTime);
extern ULONG MakeNWTime(ULONG second, ULONG minute, ULONG hour, ULONG day, ULONG month, ULONG year);
extern void GetNWTime(ULONG DateAndTime, ULONG *second, ULONG *minute, ULONG *hour,
		      ULONG *day, ULONG *month, ULONG *year);
extern ULONG MakeUnixTime(ULONG second, ULONG minute, ULONG hour, ULONG day, ULONG month, ULONG year);
extern void GetUnixTime(ULONG unixdate, ULONG *second, ULONG *minute, ULONG *hour,
			ULONG *day, ULONG *month, ULONG *year);
extern ULONG NWFSGetSeconds(void);
extern ULONG NWFSGetSystemTime(void);
extern ULONG NWFSSystemToNetwareTime(ULONG SystemTime);
extern ULONG NWFSNetwareToSystemTime(ULONG NetwareTime);

#if (WINDOWS_NT | WINDOWS_NT_RO)
extern ULONG NWFSConvertToNetwareTime(IN PTIME_FIELDS tf);
extern ULONG NWFSConvertFromNetwareTime(IN ULONG DateAndTime, IN PTIME_FIELDS tf);
#endif

#endif
