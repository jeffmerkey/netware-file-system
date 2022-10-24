
/***************************************************************************
*
*   Copyright (c) 1998, 2022 Jeff V. Merkey
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
*   FILE     :  FAT.C
*   DESCRIP  :   FAT Module
*   DATE     :  November 16, 1998
*
*
***************************************************************************/

#include "globals.h"

void NWLockFat(VOLUME *volume)
{
#if (LINUX_SLEEP)
    if (WaitOnSemaphore(&volume->FatSemaphore) == -EINTR)
       NWFSPrint("lock fat was interrupted\n");
#endif
}

void NWUnlockFat(VOLUME *volume)
{
#if (LINUX_SLEEP)
    SignalSemaphore(&volume->FatSemaphore);
#endif
}

MIRROR_LRU *RemoveFAT(VOLUME *volume, MIRROR_LRU *lru)
{
    NWLockFat(volume);
    if (volume->FATListHead == lru)
    {
       volume->FATListHead = (void *) lru->next;
       if (volume->FATListHead)
	  volume->FATListHead->prior = NULL;
       else
	  volume->FATListTail = NULL;
    }
    else
    {
       lru->prior->next = lru->next;
       if (lru != volume->FATListTail)
	  lru->next->prior = lru->prior;
       else
	  volume->FATListTail = lru->prior;
    }
    if (volume->FATListBlocks)
       volume->FATListBlocks--;
    NWUnlockFat(volume);
    return lru;

}

void InsertFAT(VOLUME *volume, MIRROR_LRU *lru)
{
    NWLockFat(volume);
    if (!volume->FATListHead)
    {
       volume->FATListHead = lru;
       volume->FATListTail = lru;
       lru->next = lru->prior = 0;
    }
    else
    {
       volume->FATListTail->next = lru;
       lru->next = 0;
       lru->prior = volume->FATListTail;
       volume->FATListTail = lru;
    }
    volume->FATListBlocks++;
    NWUnlockFat(volume);
    return;

}

void InsertFATTop(VOLUME *volume, MIRROR_LRU *lru)
{
    NWLockFat(volume);
    if (!volume->FATListHead)
    {
       volume->FATListHead = lru;
       volume->FATListTail = lru;
       lru->next = lru->prior = 0;
    }
    else
    {
       lru->next = volume->FATListHead;
       lru->prior = 0;
       volume->FATListHead->prior = lru;
       volume->FATListHead = lru;
    }
    volume->FATListBlocks++;
    NWUnlockFat(volume);
    return;

}

ULONG InitializeFAT_LRU(VOLUME *volume)
{
    volume->FATListHead = volume->FATListTail = 0;
    volume->FATListBlocks = 0;

    volume->FATBlockHash = NWFSCacheAlloc(BLOCK_NUMBER_HASH_SIZE, FATHASH_TAG);
    if (!volume->FATBlockHash)
       return -1;

    volume->FATBlockHashLimit = NUMBER_OF_BLOCK_HASH_ENTRIES;
    NWFSSet(volume->FATBlockHash, 0, BLOCK_NUMBER_HASH_SIZE);

    return 0;
}

ULONG FreeFATLists(VOLUME *volume)
{
    register MIRROR_LRU *lru;

    while (volume->FATListHead)
    {
       lru = RemoveFAT(volume, (MIRROR_LRU *)volume->FATListHead);
       if (lru)
       {
	  NWFSFree(lru->CacheBuffer);
	  NWFSFree(lru);
       }
    }

    if (volume->FATBlockHash)
       NWFSFree(volume->FATBlockHash);
    volume->FATBlockHashLimit =0;
    volume->FATBlockHash = 0;

    return 0;

}

#if (CACHE_FAT_TABLES)

ULONG FreeFAT_LRU(VOLUME *volume)
{
    register MIRROR_LRU *lru;

    while (volume->FATListHead)
    {
       lru = volume->FATListHead;
       volume->FATListHead = volume->FATListHead->next;
       if (!volume->FATListHead)
          volume->FATListTail = 0;
       
       if (lru->BlockState == L_DIRTY)
       {
	  if (lru->Cluster1 != (ULONG) -1)
	  {
	     if (WritePhysicalVolumeCluster(volume,
					    lru->Cluster1,
					    lru->CacheBuffer,
					    lru->ClusterSize,
					    FAT_PRIORITY))
	     {
		   NWFSPrint("dirty fat LRU block #%u [%X] write failed\n",
			(unsigned int)lru->Cluster1,
			(unsigned int)lru->Cluster1);
	     }
	  }

	  if (lru->Cluster2 != (ULONG) -1)
	  {
	     if (WritePhysicalVolumeCluster(volume,
					    lru->Cluster2,
					    lru->CacheBuffer,
					    lru->ClusterSize,
					    FAT_PRIORITY))
	     {
		   NWFSPrint("dirty fat LRU mirror #%u [%X] write failed\n",
			(unsigned int)lru->Cluster2, (unsigned int)lru->Cluster2);
	     }
	  }
	  lru->BlockState = L_DATAVALID;
       }
       NWFSFree(lru->CacheBuffer);
       NWFSFree(lru);
    }

    if (volume->FATBlockHash)
       NWFSFree(volume->FATBlockHash);
    volume->FATBlockHashLimit =0;
    volume->FATBlockHash = 0;

    return 0;

}

MIRROR_LRU *AllocateFAT_LRUElement(VOLUME *volume)
{
    register MIRROR_LRU *lru;

    lru = NWFSAlloc(sizeof(MIRROR_LRU), FATLRU_TAG);
    if (!lru)
       return 0;

    NWFSSet(lru, 0, sizeof(MIRROR_LRU));
    lru->BlockState = L_FREE;
    lru->Cluster1 = -1;
    lru->Cluster2 = -1;
    lru->ClusterSize = volume->ClusterSize;

    lru->CacheBuffer = NWFSCacheAlloc(volume->ClusterSize, FAT_TAG);
    if (!lru->CacheBuffer)
    {
       NWFSFree(lru);
       return 0;
    }
    NWFSSet(lru->CacheBuffer, 0, volume->ClusterSize);
    return lru;
}

void FreeFAT_LRUElement(MIRROR_LRU *lru)
{
    if (lru->CacheBuffer)
       NWFSFree(lru->CacheBuffer);
    NWFSFree(lru);
    return;
}

ULONG FlushFAT(VOLUME *volume)
{
    register MIRROR_LRU *lru;

    lru = volume->FATListHead;
    while (lru)
    {
       if (lru->BlockState == L_DIRTY)
       {

	  if (lru->Cluster1 != (ULONG) -1)
	  {
	     if (WritePhysicalVolumeCluster(volume,
					 lru->Cluster1,
					 lru->CacheBuffer,
					 lru->ClusterSize,
					 FAT_PRIORITY))
		continue;
	  }

	  if (lru->Cluster2 != (ULONG) -1)
	  {
	     if (WritePhysicalVolumeCluster(volume,
					 lru->Cluster2,
					 lru->CacheBuffer,
					 lru->ClusterSize,
					 FAT_PRIORITY))
		continue;
	  }
	  lru->BlockState = L_DATAVALID;
       }
       lru = lru->next;
    }

    return 0;
}

ULONG FlushFATBuffer(VOLUME *volume, MIRROR_LRU *lru, ULONG cluster)
{
    register ULONG EntriesPerCluster;
    register ULONG Offset, cbytes;
    register FAT_ENTRY *Table;
    ULONG retCode = 0;
    
    EntriesPerCluster = volume->ClusterSize / sizeof(FAT_ENTRY);
    Offset = cluster % EntriesPerCluster;
    
    Table = (FAT_ENTRY *) lru->CacheBuffer;
    if (lru->Cluster1 != (ULONG) -1)
    {
       cbytes = WriteClusterWithOffset(volume,
       				          lru->Cluster1,
                                          Offset * sizeof(FAT_ENTRY),
				          (BYTE *)&Table[Offset],
				          sizeof(FAT_ENTRY),
                                          KERNEL_ADDRESS_SPACE,
                                          &retCode,
				          FAT_PRIORITY);

       if ((cbytes == sizeof(FAT_ENTRY)) && !retCode)
       {
          if (lru->Cluster2 != (ULONG) -1)
          {
#if (!CREATE_FAT_MISMATCH)
             cbytes = WriteClusterWithOffset(volume,
       				          lru->Cluster2,
                                          Offset * sizeof(FAT_ENTRY),
				          (BYTE *)&Table[Offset],
				          sizeof(FAT_ENTRY),
                                          KERNEL_ADDRESS_SPACE,
                                          &retCode,
				          FAT_PRIORITY);

             if ((cbytes == sizeof(FAT_ENTRY)) && !retCode)
#endif
     		  return 0;
          }
       }
    }
    return -1;

}

ULONG WriteFATEntry(VOLUME *volume, MIRROR_LRU *lru,
		    FAT_ENTRY *fat, ULONG Cluster)
{
    register FAT_ENTRY *Table;
    register ULONG EntriesPerCluster;
    register ULONG Offset;

    EntriesPerCluster = volume->ClusterSize / sizeof(FAT_ENTRY);
    Offset = Cluster % EntriesPerCluster;

    Table = (FAT_ENTRY *) lru->CacheBuffer;
    if (fat)
    {
       NWFSCopy(&Table[Offset], fat, sizeof(FAT_ENTRY));
       return 0;
    }
    return -1;
}

MIRROR_LRU *CreateFatLRU(VOLUME *volume, ULONG cluster,
			 ULONG mirror, ULONG index)
{
    register MIRROR_LRU *lru;
    register ULONG retCode;

    lru = AllocateFAT_LRUElement(volume);
    if (lru)
    {
       retCode = ReadPhysicalVolumeCluster(volume, cluster, lru->CacheBuffer,
					   volume->ClusterSize,
					   FAT_PRIORITY);
       if (retCode)
       {
	  FreeFAT_LRUElement(lru);
	  return 0;
       }
       lru->ModLock = 0;
       lru->BlockState = L_DATAVALID;
       lru->Cluster1 = cluster;
       lru->Cluster2 = mirror;
       lru->BlockIndex = index;
       InsertFAT(volume, lru);

       retCode = AddToFATHash(volume, lru);
       if (retCode)
       {
	  RemoveFAT(volume, lru);
	  FreeFAT_LRUElement(lru);
	  return 0;
       }

       retCode = SetAssignedClusterValue(volume, cluster, 1);
       if (retCode)
       {
	  NWFSPrint("nwfs:  assigned bit block value not set cluster-%X\n",
		    (unsigned int)cluster);
       }

       retCode = SetAssignedClusterValue(volume, mirror, 1);
       if (retCode)
       {
	  NWFSPrint("nwfs:  assigned bit block value not set mirror-%X\n",
		    (unsigned int)mirror);
       }
       return lru;
    }
    return 0;

}

FAT_ENTRY *GetFatEntry(VOLUME *volume, ULONG Cluster, FAT_ENTRY *fat)
{
    register LRU_HASH_LIST *HashTable;
    register FAT_ENTRY *Table;
    register MIRROR_LRU *lru;
    register ULONG hash;
    register ULONG EntriesPerCluster;
    register ULONG Block, Offset;

    if ((Cluster == (ULONG) -1) || (Cluster & 0x80000000))
       return 0;

    if (Cluster > (volume->MountedVolumeClusters - 1))
       return 0;

    EntriesPerCluster = volume->ClusterSize / sizeof(FAT_ENTRY);
    Block = Cluster / EntriesPerCluster;
    Offset = Cluster % EntriesPerCluster;

    hash = (Block & (volume->FATBlockHashLimit - 1));
    HashTable = (LRU_HASH_LIST *) volume->FATBlockHash;
    if (!HashTable)
    {
       return 0;
    }

    lru = (MIRROR_LRU *) HashTable[hash].head;
    while (lru)
    {
       if (lru->BlockIndex == Block)
       {
	  Table = (FAT_ENTRY *) lru->CacheBuffer;
          if (fat)
          {
	     NWFSCopy(fat, &Table[Offset], sizeof(FAT_ENTRY));
             return fat;
	  }
          return 0;
       }
       lru = lru->hashNext;
    }
    return 0;

}

FAT_ENTRY *GetFatEntryAndLRU(VOLUME *volume, ULONG Cluster,
			     MIRROR_LRU **rlru, FAT_ENTRY *fat)
{
    register LRU_HASH_LIST *HashTable;
    register FAT_ENTRY *Table;
    register MIRROR_LRU *lru;
    register ULONG hash;
    register ULONG EntriesPerCluster;
    register ULONG Block, Offset;

    if ((Cluster == (ULONG) -1) || (Cluster & 0x80000000))
       return 0;

    if (Cluster > (volume->MountedVolumeClusters - 1))
       return 0;

    if (rlru)
       *rlru = 0;

    EntriesPerCluster = volume->ClusterSize / sizeof(FAT_ENTRY);
    Block = Cluster / EntriesPerCluster;
    Offset = Cluster % EntriesPerCluster;

    hash = (Block & (volume->FATBlockHashLimit - 1));
    HashTable = (LRU_HASH_LIST *) volume->FATBlockHash;
    if (!HashTable)
       return 0;

    lru = (MIRROR_LRU *) HashTable[hash].head;
    while (lru)
    {
       if (lru->BlockIndex == Block)
       {
	  Table = (FAT_ENTRY *) lru->CacheBuffer;
	  if (rlru)
	     *rlru = lru;
          if (fat)
          {
	     NWFSCopy(fat, &Table[Offset], sizeof(FAT_ENTRY));
             return fat;
	  }
          return 0;
       }
       lru = lru->hashNext;
    }
    return 0;

}

ULONG ReadFATTable(VOLUME *volume)
{
    register MIRROR_LRU *lru;
    register FAT_ENTRY *FAT1, *FAT2, *verify;
    FAT_ENTRY FAT1_S, FAT2_S, VFAT_S;
    register ULONG retCode;
    register long index1, index2;
    register ULONG Block, cluster1, cluster2, i, rebuild_flag = 0;
    register ULONG last_cluster1, last_cluster2;
    register BYTE *FATCopy;

    FATCopy = NWFSCacheAlloc(volume->ClusterSize, FAT_WORKSPACE_TAG);
    if (!FATCopy)
       return -3;

#if (MOUNT_VERBOSE)
    NWFSPrint("*** Reading FAT Tables ***\n");
#endif

ReadTable:;
    Block = 0;
    last_cluster1 = 0;
    last_cluster2 = 0;
    cluster1 = volume->FirstFAT;
    cluster2 = volume->SecondFAT;

    if (!cluster2)
    {
       NWFSPrint("ffat-%X  sfat-%x  fdir-%x  sdir-%X\n",
		 (unsigned int) volume->FirstFAT,
		 (unsigned int) volume->SecondFAT,
		 (unsigned int) volume->FirstDirectory,
		 (unsigned int) volume->SecondDirectory);

       NWFSPrint("nwfs:  second FAT is NULL\n");
       NWFSFree(FATCopy);
       return -1;
    }

    lru = CreateFatLRU(volume, cluster1, cluster2, Block);
    if (!lru)
    {
       NWFSPrint("nwfs:  could not allocate LRU element in ReadFATTables\n");
       NWFSFree(FATCopy);
       return -1;
    }

    retCode = ReadPhysicalVolumeCluster(volume, cluster2, FATCopy,
					volume->ClusterSize,
					FAT_PRIORITY);
    if (retCode)
    {
       NWFSPrint("nwfs:  could not read volume cluster in ReadFATTables\n");
       NWFSFree(FATCopy);
       return retCode;
    }

    FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
    FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);
    if (!FAT1 || !FAT2)
    {
       NWFSPrint("nwfs:  error reading fat entry ReadFATTables\n");
       NWFSFree(FATCopy);
       return -1;
    }

    index1 = FAT1->FATIndex;
    index2 = FAT2->FATIndex;

    while (TRUE)
    {
       if (NWFSCompare(lru->CacheBuffer, FATCopy, volume->ClusterSize))
       {
          nwvp_fat_fix_info fix_info;
	       
          // if we get a mirror mismatch, rebuild the
          // FAT tables and attempt remount of the volume fat. 

          // if we have already attempted fat table repair once, and it
	  // failed, then abort the mount and report a mirror mismatch.  

	  if (rebuild_flag)
          {
	     extern void dumpRecordBytes(BYTE *, ULONG);
	     register BYTE *p = lru->CacheBuffer;

	     for (i=0; i < (volume->ClusterSize / 64); i++)
	     {
	        if (NWFSCompare(&p[i * 64], &FATCopy[i * 64], 64))
	        {
		   NWFSPrint("nwfs:  FAT data mirror mismatch\n");
		   NWFSPrint("offset into cluster is 0x%X (%u bytes)\n",
			    (unsigned)(i * 64), (unsigned)(i * 64));

		   NWFSPrint("index1-%d cluster1-[%08X] next1-[%08X]\n", (int)index1,
		            (unsigned int)cluster1,
		            (unsigned int)FAT1->FATCluster);
		   dumpRecordBytes(&p[i * 64], 64);

		   NWFSPrint("index2-%d cluster2-[%08X] next2-[%08X]\n", (int)index2,
		            (unsigned int)cluster2,
		            (unsigned int)FAT2->FATCluster);
		   dumpRecordBytes(&FATCopy[i * 64], 64);
		   break;
	        }
	     }
	     NWFSFree(FATCopy);
	     return -3;
          }
          rebuild_flag++;

          NWFSPrint("nwfs:  Fat Mismatch Volume %s - Recovering Fat Journal\n",
		     volume->VolumeName);

          FlushVolumeLRU(volume);
	  FreeFATLists(volume);

          retCode = nwvp_vpartition_fat_fix(volume->nwvp_handle, &fix_info, 1);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not resolve volume fat table errors\n");
	     NWFSFree(FATCopy);
	     return -3;
	  }

	  retCode = InitializeFAT_LRU(volume);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not allocate space for volume fat\n");
	     NWFSFree(FATCopy);
	     return -3;
	  }

          goto ReadTable;
       }

       if (FAT1->FATCluster == (ULONG) -1)
	  break;

       if (FAT2->FATCluster == (ULONG) -1)
	  break;

       if (!FAT1->FATCluster)
       {
	  NWFSPrint("nwfs:  Free Cluster Detected in FAT1 Chain\n");
	  NWFSFree(FATCopy);
	  return -3;
       }

       if (!FAT2->FATCluster)
       {
	  NWFSPrint("nwfs:  Free Cluster Detected in FAT2 Chain\n");
	  NWFSFree(FATCopy);
	  return -3;
       }

       if (FAT1->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  SubAlloc Node Detected in FAT1\n");
	  NWFSFree(FATCopy);
	  return -3;
       }

       if (FAT2->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  SubAlloc Node Detected in FAT2\n");
	  NWFSFree(FATCopy);
	  return -3;
       }

       Block++;
       last_cluster1 = cluster1;
       last_cluster2 = cluster2;
       cluster1 = FAT1->FATCluster;
       cluster2 = FAT2->FATCluster;

       if (last_cluster1 > cluster1)
       {
	  NWFSPrint("nwfs:  FAT1 chain overlaps on itself\n");
	  NWFSFree(FATCopy);
	  return -1;
       }

       if (last_cluster2 > cluster2)
       {
	  NWFSPrint("nwfs:  FAT2 chain overlaps on itself\n");
	  NWFSFree(FATCopy);
	  return -1;
       }

       lru = CreateFatLRU(volume, cluster1, cluster2, Block);
       if (!lru)
       {
	  NWFSPrint("nwfs:  could not allocate LRU element in ReadFATTables\n");
	  NWFSFree(FATCopy);
	  return -1;
       }

       retCode = ReadPhysicalVolumeCluster(volume,
					   cluster2,
					   FATCopy,
					   volume->ClusterSize,
					   FAT_PRIORITY);
       if (retCode)
       {
	  NWFSPrint("nwfs:  Read Error FAT2 returned %d\n", (int)retCode);
	  NWFSFree(FATCopy);
	  return retCode;
       }

       FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
       FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);
       if (!FAT1 || !FAT2)
       {
	  NWFSPrint("nwfs:  error reading fat entry ReadFATTables\n");
	  NWFSFree(FATCopy);
	  return -1;
       }

       if (index1 > FAT1->FATIndex)
       {
	  NWFSPrint("nwfs:  FAT1 index overlaps on itself\n");
	  NWFSFree(FATCopy);
	  return -1;
       }

       if (index2 > FAT2->FATIndex)
       {
	  NWFSPrint("nwfs:  FAT2 index overlaps on itself\n");
	  NWFSFree(FATCopy);
	  return -1;
       }

       index1 = FAT1->FATIndex;
       index2 = FAT2->FATIndex;

    }
    NWFSFree(FATCopy);

    if (FAT1->FATCluster != FAT2->FATCluster)
    {
       NWFSPrint("nwfs:  FAT Table chain mirror mismatch\n");
       return -3;
    }

#if (MOUNT_VERBOSE)
    NWFSPrint("*** Verifying FAT Tables ***\n");
#endif

    for (i=0; i < volume->MountedVolumeClusters; i++)
    {
       verify = GetFatEntry(volume, i, &VFAT_S);
       if (verify)
       {
	  if (verify->FATCluster)
	  {
	     retCode = SetFreeClusterValue(volume, i, 1);  // block is allocated
	     if (retCode)
	     {
		NWFSPrint("nwfs:  bit block value not set cluster-%X [1]\n",
		       (unsigned int)i);
	     }

	     // this is a sanity check for ECC memory bit errors
	     if (!GetFreeClusterValue(volume, i))
	     {
		NWFSPrint("nwfs:  bit block value incorrect cluster-%X [1]\n",
		       (unsigned int)i);
	     }
	     volume->VolumeAllocatedClusters++;
	  }
	  else
	  {
	     retCode = SetFreeClusterValue(volume, i, 0);  // block is free
	     if (retCode)
	     {
		NWFSPrint("nwfs:  bit block value not set cluster-%X [0]\n",
		       (unsigned int)i);
	     }

	     // this is a sanity check for ECC memory bit errors
	     if (GetFreeClusterValue(volume, i))
	     {
		NWFSPrint("nwfs:  bit block value incorrect cluster-%X [0]\n",
		       (unsigned int)i);
	     }
	     volume->VolumeFreeClusters++;
	  }
       }
       else
       {
	  NWFSPrint("nwfs:  FAT Table data errors detected\n");
	  return -1;
       }
    }

    return 0;

}

#else

ULONG FreeFAT_LRU(VOLUME *volume)
{
    register MIRROR_LRU *lru;

    while (volume->FATListHead)
    {
       lru = RemoveFAT(volume, (MIRROR_LRU *)volume->FATListHead);
       if (lru)
	  NWFSFree(lru);
    }

    if (volume->FATBlockHash)
       NWFSFree(volume->FATBlockHash);
    volume->FATBlockHashLimit =0;
    volume->FATBlockHash = 0;

    return 0;

}

MIRROR_LRU *AllocateFAT_LRUElement(VOLUME *volume)
{
    register MIRROR_LRU *lru;

    lru = NWFSAlloc(sizeof(MIRROR_LRU), FATLRU_TAG);
    if (!lru)
       return 0;

    NWFSSet(lru, 0, sizeof(MIRROR_LRU));
    lru->BlockState = L_FREE;
    lru->Cluster1 = -1;
    lru->Cluster2 = -1;
    lru->ClusterSize = volume->ClusterSize;

    return lru;
}

void FreeFAT_LRUElement(MIRROR_LRU *lru)
{
    NWFSFree(lru);
    return;
}

ULONG WriteFATEntry(VOLUME *volume, MIRROR_LRU *lru,
		    FAT_ENTRY *fat, ULONG Cluster)
{
    register ULONG EntriesPerCluster;
    register ULONG Offset, cbytes;
    ULONG retCode = 0;

    EntriesPerCluster = volume->ClusterSize / sizeof(FAT_ENTRY);
    Offset = Cluster % EntriesPerCluster;

    if (lru->Cluster1 != (ULONG) -1)
    {
       cbytes = WriteClusterWithOffset(volume,
       				          lru->Cluster1,
                                          Offset * sizeof(FAT_ENTRY),
				          (BYTE *)fat,
				          sizeof(FAT_ENTRY),
                                          KERNEL_ADDRESS_SPACE,
                                          &retCode,
				          FAT_PRIORITY);

       if ((cbytes == sizeof(FAT_ENTRY)) && !retCode)
       {
          if (lru->Cluster2 != (ULONG) -1)
          {
#if (!CREATE_FAT_MISMATCH)
             cbytes = WriteClusterWithOffset(volume,
       				          lru->Cluster2,
                                          Offset * sizeof(FAT_ENTRY),
				          (BYTE *)fat,
				          sizeof(FAT_ENTRY),
                                          KERNEL_ADDRESS_SPACE,
                                          &retCode,
				          FAT_PRIORITY);

             if ((cbytes == sizeof(FAT_ENTRY)) && !retCode)
#endif
     		  return 0;
          }
       }
    }
    return -1;
}

MIRROR_LRU *CreateFatLRU(VOLUME *volume, ULONG cluster,
			 ULONG mirror, ULONG index)
{
    register MIRROR_LRU *lru;
    register ULONG retCode;

    lru = AllocateFAT_LRUElement(volume);
    if (lru)
    {
       lru->ModLock = 0;
       lru->BlockState = L_DATAVALID;
       lru->Cluster1 = cluster;
       lru->Cluster2 = mirror;
       lru->BlockIndex = index;
       InsertFAT(volume, lru);

       retCode = AddToFATHash(volume, lru);
       if (retCode)
       {
	  RemoveFAT(volume, lru);
	  FreeFAT_LRUElement(lru);
	  return 0;
       }

       retCode = SetAssignedClusterValue(volume, cluster, 1);
       if (retCode)
       {
	  NWFSPrint("nwfs:  assigned bit block value not set cluster-%X\n",
		    (unsigned int)cluster);
       }

       retCode = SetAssignedClusterValue(volume, mirror, 1);
       if (retCode)
       {
	  NWFSPrint("nwfs:  assigned bit block value not set mirror-%X\n",
		    (unsigned int)mirror);
       }
       return lru;
    }
    return 0;

}

FAT_ENTRY *GetFatEntry(VOLUME *volume, ULONG Cluster, FAT_ENTRY *fat)
{
    register LRU_HASH_LIST *HashTable;
    register MIRROR_LRU *lru;
    register ULONG hash, cbytes;
    register ULONG EntriesPerCluster;
    register ULONG Block, Offset;
    ULONG retCode = 0;

    if ((Cluster == (ULONG) -1) || (Cluster & 0x80000000))
       return 0;

    if (Cluster > (volume->MountedVolumeClusters - 1))
       return 0;

    EntriesPerCluster = volume->ClusterSize / sizeof(FAT_ENTRY);
    Block = Cluster / EntriesPerCluster;
    Offset = Cluster % EntriesPerCluster;

    hash = (Block & (volume->FATBlockHashLimit - 1));
    HashTable = (LRU_HASH_LIST *) volume->FATBlockHash;
    if (!HashTable)
    {
       return 0;
    }

    lru = (MIRROR_LRU *) HashTable[hash].head;
    while (lru)
    {
       if (lru->BlockIndex == Block)
       {
          if (fat)
          {
             if (lru->Cluster1 != (ULONG) -1)
	     {
	        cbytes = ReadClusterWithOffset(volume,
       				          lru->Cluster1,
                                          Offset * sizeof(FAT_ENTRY),
				          (BYTE *)fat,
				          sizeof(FAT_ENTRY),
                                          KERNEL_ADDRESS_SPACE,
                                          &retCode,
				          FAT_PRIORITY);

	        if ((cbytes == sizeof(FAT_ENTRY)) && !retCode)
                   return fat;

	        if (lru->Cluster2 != (ULONG) -1)
	        {
	              cbytes = ReadClusterWithOffset(volume,
       				          lru->Cluster2,
                                          Offset * sizeof(FAT_ENTRY),
				          (BYTE *)fat,
				          sizeof(FAT_ENTRY),
                                          KERNEL_ADDRESS_SPACE,
                                          &retCode,
				          FAT_PRIORITY);

	           if ((cbytes == sizeof(FAT_ENTRY)) && !retCode)
	     	      return fat;
	        }
	     }
	  }
          return 0;
       }
       lru = lru->hashNext;
    }
    return 0;

}

FAT_ENTRY *GetFatEntryAndLRU(VOLUME *volume, ULONG Cluster,
			     MIRROR_LRU **rlru, FAT_ENTRY *fat)
{
    register LRU_HASH_LIST *HashTable;
    register MIRROR_LRU *lru;
    register ULONG hash, cbytes;
    register ULONG EntriesPerCluster;
    register ULONG Block, Offset;
    ULONG retCode = 0;

    if ((Cluster == (ULONG) -1) || (Cluster & 0x80000000))
       return 0;

    if (Cluster > (volume->MountedVolumeClusters - 1))
       return 0;

    if (rlru)
       *rlru = 0;

    EntriesPerCluster = volume->ClusterSize / sizeof(FAT_ENTRY);
    Block = Cluster / EntriesPerCluster;
    Offset = Cluster % EntriesPerCluster;

    hash = (Block & (volume->FATBlockHashLimit - 1));
    HashTable = (LRU_HASH_LIST *) volume->FATBlockHash;
    if (!HashTable)
       return 0;

    lru = (MIRROR_LRU *) HashTable[hash].head;
    while (lru)
    {
       if (lru->BlockIndex == Block)
       {
	  if (rlru)
	     *rlru = lru;

          if (fat)
          {
             if (lru->Cluster1 != (ULONG) -1)
	     {
	        cbytes = ReadClusterWithOffset(volume,
       				          lru->Cluster1,
                                          Offset * sizeof(FAT_ENTRY),
				          (BYTE *)fat,
				          sizeof(FAT_ENTRY),
                                          KERNEL_ADDRESS_SPACE,
                                          &retCode,
				          FAT_PRIORITY);

	        if ((cbytes == sizeof(FAT_ENTRY)) && !retCode)
                   return fat;

                // if the read from the primary failed, then
                // try to read from the mirrored fat tables

	        if (lru->Cluster2 != (ULONG) -1)
	        {
	              cbytes = ReadClusterWithOffset(volume,
       				          lru->Cluster2,
                                          Offset * sizeof(FAT_ENTRY),
				          (BYTE *)fat,
				          sizeof(FAT_ENTRY),
                                          KERNEL_ADDRESS_SPACE,
                                          &retCode,
				          FAT_PRIORITY);

	           if ((cbytes == sizeof(FAT_ENTRY)) && !retCode)
	     	      return fat;
	        }
	     }
	  }
          return 0;
       }
       lru = lru->hashNext;
    }
    return 0;

}

ULONG ReadFATTable(VOLUME *volume)
{
    register MIRROR_LRU *lru;
    register FAT_ENTRY *FAT1, *FAT2, *verify;
    FAT_ENTRY FAT1_S, FAT2_S, VFAT_S;
    register ULONG retCode;
    register long index1, index2;
    register ULONG Block, cluster1, cluster2, i, rebuild_flag = 0;
    register ULONG last_cluster1, last_cluster2;
    register BYTE *FATCopy, *FATPrimary;

    FATPrimary = NWFSCacheAlloc(volume->ClusterSize, FAT_WORKSPACE_TAG);
    if (!FATPrimary)
       return -3;

    FATCopy = NWFSCacheAlloc(volume->ClusterSize, FAT_WORKSPACE_TAG);
    if (!FATCopy)
    {
       NWFSFree(FATPrimary);
       return -3;
    }

#if (MOUNT_VERBOSE)
    NWFSPrint("*** Reading FAT Tables ***\n");
#endif

ReadTable:;
    Block = 0;
    last_cluster1 = 0;
    last_cluster2 = 0;
    cluster1 = volume->FirstFAT;
    cluster2 = volume->SecondFAT;

    if (!cluster2)
    {
       NWFSPrint("ffat-%X  sfat-%x  fdir-%x  sdir-%X\n",
		 (unsigned int) volume->FirstFAT,
		 (unsigned int) volume->SecondFAT,
		 (unsigned int) volume->FirstDirectory,
		 (unsigned int) volume->SecondDirectory);

       NWFSPrint("nwfs:  second FAT is NULL\n");
       NWFSFree(FATPrimary);
       NWFSFree(FATCopy);
       return -1;
    }

    lru = CreateFatLRU(volume, cluster1, cluster2, Block);
    if (!lru)
    {
       NWFSPrint("nwfs:  could not allocate LRU element in ReadFATTables\n");
       NWFSFree(FATPrimary);
       NWFSFree(FATCopy);
       return -1;
    }

    retCode = ReadPhysicalVolumeCluster(volume, cluster1, FATPrimary,
					volume->ClusterSize,
					FAT_PRIORITY);
    if (retCode)
    {
       NWFSPrint("nwfs:  could not read volume cluster in ReadFATTables\n");
       NWFSFree(FATPrimary);
       NWFSFree(FATCopy);
       return retCode;
    }

    retCode = ReadPhysicalVolumeCluster(volume, cluster2, FATCopy,
					volume->ClusterSize,
					FAT_PRIORITY);
    if (retCode)
    {
       NWFSPrint("nwfs:  could not read volume cluster in ReadFATTables\n");
       NWFSFree(FATPrimary);
       NWFSFree(FATCopy);
       return retCode;
    }

    FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
    FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);
    if (!FAT1 || !FAT2)
    {
       NWFSPrint("nwfs:  error reading fat entry ReadFATTables\n");
       NWFSFree(FATPrimary);
       NWFSFree(FATCopy);
       return -1;
    }

    index1 = FAT1->FATIndex;
    index2 = FAT2->FATIndex;

    while (TRUE)
    {
       if (NWFSCompare(FATPrimary, FATCopy, volume->ClusterSize))
       {
          nwvp_fat_fix_info fix_info;

          // if we get a mirror mismatch, rebuild the
          // FAT tables and attempt remount of the volume fat.

          // if we have already attempted fat table repair once, and it
	  // failed, then abort the mount and report a mirror mismatch.

	  if (rebuild_flag)
          {
	     extern void dumpRecordBytes(BYTE *, ULONG);
	     register BYTE *p = FATPrimary;

	     for (i=0; i < (volume->ClusterSize / 64); i++)
	     {
	        if (NWFSCompare(&p[i * 64], &FATCopy[i * 64], 64))
	        {
		   NWFSPrint("nwfs:  FAT data mirror mismatch\n");
		   NWFSPrint("offset into cluster is 0x%X (%u bytes)\n",
			    (unsigned)(i * 64), (unsigned)(i * 64));

		   NWFSPrint("index1-%d cluster1-[%08X] next1-[%08X]\n", (int)index1,
		            (unsigned int)cluster1,
		            (unsigned int)FAT1->FATCluster);
		   dumpRecordBytes(&p[i * 64], 64);

		   NWFSPrint("index2-%d cluster2-[%08X] next2-[%08X]\n", (int)index2,
		            (unsigned int)cluster2,
		            (unsigned int)FAT2->FATCluster);
		   dumpRecordBytes(&FATCopy[i * 64], 64);
		   break;
	        }
	     }
             NWFSFree(FATPrimary);
	     NWFSFree(FATCopy);
	     return -3;
          }
          rebuild_flag++;

          NWFSPrint("nwfs:  Fat Mismatch Volume %s - Recovering Fat Journal\n",
		     volume->VolumeName);

          FlushVolumeLRU(volume);
	  FreeFATLists(volume);

          retCode = nwvp_vpartition_fat_fix(volume->nwvp_handle, &fix_info, 1);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not resolve volume fat table errors\n");
             NWFSFree(FATPrimary);
	     NWFSFree(FATCopy);
	     return -3;
	  }

	  retCode = InitializeFAT_LRU(volume);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not allocate space for volume fat\n");
             NWFSFree(FATPrimary);
	     NWFSFree(FATCopy);
	     return -3;
	  }

          goto ReadTable;
       }

       if (FAT1->FATCluster == (ULONG) -1)
	  break;

       if (FAT2->FATCluster == (ULONG) -1)
	  break;

       if (!FAT1->FATCluster)
       {
	  NWFSPrint("nwfs:  Free Cluster Detected in FAT1 Chain\n");
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return -3;
       }

       if (!FAT2->FATCluster)
       {
	  NWFSPrint("nwfs:  Free Cluster Detected in FAT2 Chain\n");
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return -3;
       }

       if (FAT1->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  SubAlloc Node Detected in FAT1\n");
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return -3;
       }

       if (FAT2->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  SubAlloc Node Detected in FAT2\n");
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return -3;
       }

       Block++;
       last_cluster1 = cluster1;
       last_cluster2 = cluster2;
       cluster1 = FAT1->FATCluster;
       cluster2 = FAT2->FATCluster;

       if (last_cluster1 > cluster1)
       {
	  NWFSPrint("nwfs:  FAT1 chain overlaps on itself\n");
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return -1;
       }

       if (last_cluster2 > cluster2)
       {
	  NWFSPrint("nwfs:  FAT2 chain overlaps on itself\n");
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return -1;
       }

       lru = CreateFatLRU(volume, cluster1, cluster2, Block);
       if (!lru)
       {
	  NWFSPrint("nwfs:  could not allocate LRU element in ReadFATTables\n");
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return -1;
       }

       retCode = ReadPhysicalVolumeCluster(volume,
					   cluster1,
					   FATPrimary,
					   volume->ClusterSize,
					   FAT_PRIORITY);
       if (retCode)
       {
	  NWFSPrint("nwfs:  Read Error FAT1 returned %d\n", (int)retCode);
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return retCode;
       }

       retCode = ReadPhysicalVolumeCluster(volume,
					   cluster2,
					   FATCopy,
					   volume->ClusterSize,
					   FAT_PRIORITY);
       if (retCode)
       {
	  NWFSPrint("nwfs:  Read Error FAT2 returned %d\n", (int)retCode);
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return retCode;
       }

       FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
       FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);
       if (!FAT1 || !FAT2)
       {
	  NWFSPrint("nwfs:  error reading fat entry ReadFATTables\n");
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return -1;
       }

       if (index1 > FAT1->FATIndex)
       {
	  NWFSPrint("nwfs:  FAT1 index overlaps on itself\n");
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return -1;
       }

       if (index2 > FAT2->FATIndex)
       {
	  NWFSPrint("nwfs:  FAT2 index overlaps on itself\n");
          NWFSFree(FATPrimary);
	  NWFSFree(FATCopy);
	  return -1;
       }

       index1 = FAT1->FATIndex;
       index2 = FAT2->FATIndex;

    }
    NWFSFree(FATPrimary);
    NWFSFree(FATCopy);

    if (FAT1->FATCluster != FAT2->FATCluster)
    {
       NWFSPrint("nwfs:  FAT Table chain mirror mismatch\n");
       return -3;
    }

#if (MOUNT_VERBOSE)
    NWFSPrint("*** Verifying FAT Tables ***\n");
#endif

    for (i=0; i < volume->MountedVolumeClusters; i++)
    {
       verify = GetFatEntry(volume, i, &VFAT_S);
       if (verify)
       {
	  if (verify->FATCluster)
	  {
	     retCode = SetFreeClusterValue(volume, i, 1);  // block is allocated
	     if (retCode)
	     {
		NWFSPrint("nwfs:  bit block value not set cluster-%X [1]\n",
		       (unsigned int)i);
	     }

	     // this is a sanity check for ECC memory bit errors
	     if (!GetFreeClusterValue(volume, i))
	     {
		NWFSPrint("nwfs:  bit block value incorrect cluster-%X [1]\n",
		       (unsigned int)i);
	     }
	     volume->VolumeAllocatedClusters++;
	  }
	  else
	  {
	     retCode = SetFreeClusterValue(volume, i, 0);  // block is free
	     if (retCode)
	     {
		NWFSPrint("nwfs:  bit block value not set cluster-%X [0]\n",
		       (unsigned int)i);
	     }

	     // this is a sanity check for ECC memory bit errors
	     if (GetFreeClusterValue(volume, i))
	     {
		NWFSPrint("nwfs:  bit block value incorrect cluster-%X [0]\n",
		       (unsigned int)i);
	     }
	     volume->VolumeFreeClusters++;
	  }
       }
       else
       {
	  NWFSPrint("nwfs:  FAT Table data errors detected\n");
	  return -1;
       }
    }
    return 0;

}

#endif

ULONG AddToFATHash(VOLUME *volume, MIRROR_LRU *lru)
{
    register ULONG hash;
    register LRU_HASH_LIST *HashTable;

    // This hash works on FAT sequence index.  The Directory
    // Hash is identical in structure.  This method allows very
    // rapid table searches of the directory file and the FAT

    NWLockFat(volume);
    hash = (lru->BlockIndex & (volume->FATBlockHashLimit - 1));
    HashTable = (LRU_HASH_LIST *) volume->FATBlockHash;
    if (HashTable)
    {
       if (!HashTable[hash].head)
       {
	  HashTable[hash].head = lru;
	  HashTable[hash].tail = lru;
	  lru->hashNext = lru->hashPrior = 0;
       }
       else
       {
	  HashTable[hash].tail->hashNext = lru;
	  lru->hashNext = 0;
	  lru->hashPrior = HashTable[hash].tail;
	  HashTable[hash].tail = lru;
       }
       NWUnlockFat(volume);
       return 0;
    }
    NWUnlockFat(volume);
    return -1;
}

ULONG RemoveFATHash(VOLUME *volume, MIRROR_LRU *lru)
{
    register ULONG hash;
    register LRU_HASH_LIST *HashTable;

    NWLockFat(volume);
    hash = (lru->BlockIndex & (volume->FATBlockHashLimit - 1));
    HashTable = (LRU_HASH_LIST *) volume->FATBlockHash;
    if (HashTable)
    {
       if (HashTable[hash].head == lru)
       {
	  HashTable[hash].head = lru->hashNext;
	  if (HashTable[hash].head)
	     HashTable[hash].head->hashPrior = NULL;
	  else
	     HashTable[hash].tail = NULL;
       }
       else
       {
	  lru->hashPrior->hashNext = lru->hashNext;
	  if (lru != HashTable[hash].tail)
	     lru->hashNext->hashPrior = lru->hashPrior;
	  else
	     HashTable[hash].tail = lru->hashPrior;
       }
       NWUnlockFat(volume);
       return 0;
    }
    NWUnlockFat(volume);
    return -1;
}

