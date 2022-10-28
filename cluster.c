
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
*   FILE     :  CLUSTER.C
*   DESCRIP  :  Volume Cluster Routines
*   DATE     :  December 1, 1998
*
*
***************************************************************************/

#include "globals.h"

ULONG TruncateClusterChain(VOLUME *volume, ULONG *Chain, ULONG Index,
			   ULONG PrevChain, ULONG size, ULONG SAFlag,
			   ULONG Attributes)
{
    register ULONG cluster, fcluster, prev_cluster, cbytes, NewCluster;
    register ULONG index, FileIndex, FileOffset, prev_index;
    MIRROR_LRU *lru = 0;
    ULONG retCode = 0;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    register VOLUME_WORKSPACE *WorkSpace;
    register ULONG SuballocSize;

    if (!Chain)
       return NwInvalidParameter;

    if (!(*Chain))
    {
       NWFSPrint("nwfs:  fat chain was NULL (truncate)\n");
       return NwFileCorrupt;
    }

    if (*Chain == (ULONG) -1)
       return 0;

    index = 0;
    prev_cluster = (ULONG) -1;
    prev_index = (ULONG) -1;
    cluster = *Chain;

    if (PrevChain)
       prev_cluster = PrevChain;

    if (Index)
       index = Index;

    FileIndex = size / volume->ClusterSize;
    FileOffset = size % volume->ClusterSize;

#if (VERBOSE)
    NWFSPrint("nwfs_truncate:  size-%d chain-%08X index-%X offset-%X\n",
	     (int) size,
	     (unsigned int)*Chain,
	     (unsigned int)FileIndex,
	     (unsigned int)FileOffset);
#endif

    if (cluster & 0x80000000)
    {
       if (cluster == (ULONG) -1)
	  return 0;

       // check if suballocation is legal for this file.
       if ((!SAFlag) || (Attributes & NO_SUBALLOC) || (Attributes & TRANSACTION))
	  return NwNotPermitted;

       // if we are at our target index
       if (index == FileIndex)
       {
	  SuballocSize = GetSuballocSize(volume, cluster);

	  // this case assumes we will free the current suballoc
	  // fragment.

	  if (!FileOffset)
	  {
	     // if there was a previous cluster, then set as EOF
	     if (prev_cluster != (ULONG) -1)
	     {
		if (SetClusterValue(volume, prev_cluster, (ULONG) -1))
		   return NwVolumeCorrupt;
	     }
	     else
		*Chain = (ULONG) -1;

	     FreeSuballocRecord(volume, cluster);
	     return 0;
	  }

	  if ((FileOffset + (SUBALLOC_BLOCK_SIZE - 1)) < SuballocSize)
	  {
	     // for this case, we are going to free the current suballoc
	     // element, and replace it with a suballoc element of a
	     // smaller size.

	     WorkSpace = AllocateWorkspace(volume);
	     if (WorkSpace)
	     {
		NewCluster = AllocateSuballocRecord(volume, FileOffset,
						    &retCode);
		if (NewCluster)
		{
		   // here we read the previous data from the suballoc element
		   cbytes = ReadSuballocRecord(volume, 0, cluster,
					       &WorkSpace->Buffer[0],
					       FileOffset,
					       KERNEL_ADDRESS_SPACE,
					       &retCode);
		   if (cbytes != FileOffset)
		   {
		      FreeSuballocRecord(volume, NewCluster);
		      FreeWorkspace(volume, WorkSpace);
		      goto SkipFirstSuballocReplace;
		   }

		   // write the previous data to the new suballoc element
		   cbytes = WriteSuballocRecord(volume, 0, NewCluster,
						&WorkSpace->Buffer[0],
						FileOffset,
						KERNEL_ADDRESS_SPACE,
						&retCode);
		   if (cbytes != FileOffset)
		   {
		      FreeSuballocRecord(volume, NewCluster);
		      FreeWorkspace(volume, WorkSpace);
		      goto SkipFirstSuballocReplace;
		   }
		   FreeWorkspace(volume, WorkSpace);

		   // if there was a previous cluster, then set as EOF
		   if (prev_cluster != (ULONG) -1)
		   {
		      if (SetClusterValue(volume, prev_cluster, NewCluster))
		      {
			 FreeSuballocRecord(volume, NewCluster);
			 return NwVolumeCorrupt;
		      }
		   }
		   else
		      *Chain = NewCluster;

		   FreeSuballocRecord(volume, cluster);
		   return 0;
		}
		FreeWorkspace(volume, WorkSpace);
	     }
	  }
       }

SkipFirstSuballocReplace:;
       return 0;
    }

    FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
    if (FAT)
    {
       prev_index = index;
       index = FAT->FATIndex;
    }

    while (FAT && FAT->FATCluster)
    {
       // if we are past our target index in the fat chain, then
       // truncate the remaining elements in the chain.

       if (index > FileIndex)
       {
	  // if there was a previous cluster, then set as EOF
	  if (prev_cluster != (ULONG) -1)
	  {
	     if (SetClusterValue(volume, prev_cluster, (ULONG) -1))
		return NwVolumeCorrupt;
	  }
	  else
	     *Chain = (ULONG) -1;

	  goto FreeChain;
       }

       // check if we have found the cluster that matches our target
       // index.

       if (index == FileIndex)
       {
	  // if size is on a cluster boundry, then free the current
	  // cluster and all entries following it unless the file has
	  // holes.

	  if (!FileOffset)
	  {
	     // if there was a previous cluster, then set as EOF
	     if (prev_cluster != (ULONG) -1)
	     {
		if (SetClusterValue(volume, prev_cluster, (ULONG) -1))
		   return NwVolumeCorrupt;
	     }
	     else
		*Chain = (ULONG) -1;

	     goto FreeChain;
	  }

	  // This case assumes that we have detected a file element
	  // that is the first element in the fat chain and that
	  // is not at the expected index of zero.  This means that
	  // the first portion of the file is sparse and has a hole in
	  // it.  It is invalid to suballocate the first non-zero
	  // indexed block within a fat chain because the index value
	  // cannot be stored by a suballocation element.  As such,
	  // this is the one of two cases where we do not suballocate a
	  // partial cluster.  Since the file is sparse, we are already
	  // getting significantly more efficient storage than the
	  // count of clusters reflected in the index count.

	  // If index is non-zero and we are at the head of the fat
	  // chain, then do not suballocate this cluster.

	  if (index && (prev_cluster == (ULONG) -1))
	  {
#if (VERBOSE)
	     NWFSPrint("detected sparse element  %08X-[%08X] (head)\n",
		       (unsigned int) index,
		       (unsigned int) cluster);
#endif
	     goto DontSuballocate;
	  }

	  // if we are not the first cluster in the chain, but if we are
	  // the next cluster immediately following an allocation hole
	  // in the file, then do not suballocate this cluster.  we
	  // keep track of the previous index value, and if the index
	  // numbers in the fat table are not sequential, then we know
	  // that this cluster immediately follows a hole in the
	  // file.

	  if ((prev_cluster != (ULONG) -1) && ((prev_index + 1) != index))
	  {
#if (VERBOSE)
	     NWFSPrint("detected sparse element  %08X-[%08X] (chain)\n",
		       (unsigned int) index,
		       (unsigned int) cluster);
#endif
	     goto DontSuballocate;
	  }

	  // check if suballocation is legal for this file.

	  if ((!SAFlag) || (Attributes & NO_SUBALLOC) ||
	      (Attributes & TRANSACTION))
	     goto DontSuballocate;

	  // If this cluster has a sequential index count, and is not
	  // a sparse cluster at the chain head, and its current
	  // allocation size is not within volume->ClusterSize - 511,
	  // and the file is not flagged transactional, and suballocation
	  // has not been disabled at the file level, then convert
	  // this cluster into a suballocation element, copy the
	  // data from the partially filled cluster, and free
	  // everything in the chain, including this cluster.

	  if ((FileOffset + (SUBALLOC_BLOCK_SIZE - 1)) < volume->ClusterSize)
	  {
	     // for this case, we are going to free the current cluster
	     // and replace it with a suballoc element of a
	     // smaller size.

	     WorkSpace = AllocateWorkspace(volume);
	     if (WorkSpace)
	     {
		NewCluster = AllocateSuballocRecord(volume, FileOffset, &retCode);
		if (NewCluster)
		{
		   // here we read the previous data from the cluster
		   cbytes = ReadClusterWithOffset(volume,
						  cluster,
						  0,
						  &WorkSpace->Buffer[0],
						  FileOffset,
						  KERNEL_ADDRESS_SPACE,
						  &retCode,
						  DATA_PRIORITY);
		   if (cbytes != FileOffset)
		   {
		      FreeSuballocRecord(volume, NewCluster);
		      FreeWorkspace(volume, WorkSpace);
		      goto DontSuballocate;
		   }

		   // write the previous data to the new suballoc element
		   cbytes = WriteSuballocRecord(volume,
						0,
						NewCluster,
						&WorkSpace->Buffer[0],
						FileOffset,
						KERNEL_ADDRESS_SPACE,
						&retCode);
		   if (cbytes != FileOffset)
		   {
		      FreeSuballocRecord(volume, NewCluster);
		      FreeWorkspace(volume, WorkSpace);
		      goto DontSuballocate;
		   }
		   FreeWorkspace(volume, WorkSpace);

		   // if there was a previous cluster, then set as EOF
		   if (prev_cluster != (ULONG) -1)
		   {
		      if (SetClusterValue(volume, prev_cluster, NewCluster))
		      {
			 FreeSuballocRecord(volume, NewCluster);
			 return NwVolumeCorrupt;
		      }
		   }
		   else
		      *Chain = NewCluster;

		   goto FreeChain;
		}
		FreeWorkspace(volume, WorkSpace);
	     }
	  }

DontSuballocate:;
	  // set this cluster as end of file
	  if (SetClusterValue(volume, cluster, (ULONG) -1))
	     return NwVolumeCorrupt;

	  // bump to next cluster
	  cluster = FAT->FATCluster;

	  FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
	  if (FAT)
	  {
	     prev_index = index;
	     index = FAT->FATIndex;
	  }
	  goto FreeChain;
       }

       // save previous cluster
       prev_cluster = cluster;

       // bump to next cluster
       cluster = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF
       if (cluster & 0x80000000)
       {
	  if (cluster == (ULONG) -1)
	     return 0;

	  // check if suballocation is legal for this file.
	  if ((!SAFlag) || (Attributes & NO_SUBALLOC) || (Attributes & TRANSACTION))
	     return NwNotPermitted;

	  // if we are at our target index
	  if ((index + 1) == FileIndex)
	  {
	     SuballocSize = GetSuballocSize(volume, cluster);

	     // this case assumes we will free the current suballoc
	     // fragment.

	     if (!FileOffset)
	     {
		// if there was a previous cluster, then set as EOF
		if (prev_cluster != (ULONG) -1)
		{
		   if (SetClusterValue(volume, prev_cluster, (ULONG) -1))
		      return NwVolumeCorrupt;
		}
		else
		   *Chain = (ULONG) -1;

		FreeSuballocRecord(volume, cluster);
		return 0;
	     }

	     if ((FileOffset + (SUBALLOC_BLOCK_SIZE - 1)) < SuballocSize)
	     {
		// for this case, we are going to free the current suballoc
		// element, and replace it with a suballoc element of a
		// smaller size.

		WorkSpace = AllocateWorkspace(volume);
		if (WorkSpace)
		{
		   NewCluster = AllocateSuballocRecord(volume, FileOffset,
						    &retCode);
		   if (NewCluster)
		   {
		      // here we read the previous data from the suballoc element
		      cbytes = ReadSuballocRecord(volume, 0, cluster,
					       &WorkSpace->Buffer[0],
					       FileOffset,
					       KERNEL_ADDRESS_SPACE,
					       &retCode);
		      if (cbytes != FileOffset)
		      {
			 FreeSuballocRecord(volume, NewCluster);
			 FreeWorkspace(volume, WorkSpace);
			 goto SkipSuballocReplace;
		      }

		      // write the previous data to the new suballoc element
		      cbytes = WriteSuballocRecord(volume, 0, NewCluster,
						&WorkSpace->Buffer[0],
						FileOffset,
						KERNEL_ADDRESS_SPACE,
						&retCode);
		      if (cbytes != FileOffset)
		      {
			 FreeSuballocRecord(volume, NewCluster);
			 FreeWorkspace(volume, WorkSpace);
			 goto SkipSuballocReplace;
		      }
		      FreeWorkspace(volume, WorkSpace);

		      // if there was a previous cluster, then set as EOF
		      if (prev_cluster != (ULONG) -1)
		      {
			 if (SetClusterValue(volume, prev_cluster, NewCluster))
			 {
			    FreeSuballocRecord(volume, NewCluster);
			    return NwVolumeCorrupt;
			 }
		      }
		      else
			 *Chain = NewCluster;

		      FreeSuballocRecord(volume, cluster);
		      return 0;
		   }
		   FreeWorkspace(volume, WorkSpace);
		}
	     }
	  }

SkipSuballocReplace:;
	  return 0;
       }

       FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
       if (FAT)
       {
	  prev_index = index;
	  index = FAT->FATIndex;
       }
    }
    return 0;


FreeChain:;
    while (FAT && FAT->FATCluster)
    {
       // save current cluster
       fcluster = cluster;

       // bump to next cluster
       cluster = FAT->FATCluster;

       // clear this cluster in the FAT table
       retCode = NWUpdateFat(volume, FAT, lru, 0, 0, fcluster);
       if (retCode)
       {
	  NWFSPrint("*** error updating FAT [%X/%X] ***\n",
		    (unsigned int)lru->Cluster1,
		    (unsigned int)lru->Cluster2);
	  return NwVolumeCorrupt;
       }

       // set current cluster free in bit block list
       SetFreeClusterValue(volume, fcluster, 0);

       volume->VolumeFreeClusters++;
       if (volume->VolumeAllocatedClusters)
	  volume->VolumeAllocatedClusters--;

       // check if the next cluster is a suballoc element or EOF
       if (cluster & 0x80000000)
       {
	  if (cluster == (ULONG) -1)
	     return 0;

	  if ((!SAFlag) || (Attributes & NO_SUBALLOC) || (Attributes & TRANSACTION))
	     return NwNotPermitted;

	  // set suballoc element free in list
	  FreeSuballocRecord(volume, cluster);

	  return 0;
       }

       FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
       if (FAT)
	  index = FAT->FATIndex;
    }
    return 0;

}

//
//   Cluster Allocation bitmap routines
//

ULONG InitializeClusterFreeList(VOLUME *volume)
{
   return (CreateBitBlockList(&volume->FreeBlockList,
			      volume->VolumeClusters,
			      BIT_BLOCK_SIZE,
			      "cluster free"));
}

ULONG ExtendClusterFreeList(VOLUME *volume, ULONG Amount)
{
#if (VERBOSE)
    NWFSPrint("extend_cluster_free_list\n");
#endif 
    return (AdjustBitBlockList(&volume->FreeBlockList, Amount));
}

ULONG FreeClusterFreeList(VOLUME *volume)
{
#if (VERBOSE)
    NWFSPrint("free_cluster_free_list\n"); 
#endif 
    return (FreeBitBlockList(&volume->FreeBlockList));
}

ULONG GetFreeClusterValue(VOLUME *volume, ULONG Cluster)
{
#if (VERBOSE)
    NWFSPrint("get_free_cluster_value\n"); 
#endif 
    return (GetBitBlockValue(&volume->FreeBlockList, Cluster));
}

ULONG SetFreeClusterValue(VOLUME *volume, ULONG Cluster, ULONG flag)
{
    register int i;

#if (VERBOSE)
    NWFSPrint("set_free_cluster_value\n"); 
#endif 
    for (i=0; i < volume->MountedNumberOfSegments; i++)
    {
       if (Cluster < (volume->SegmentClusterStart[i] + 
	              volume->SegmentClusterSize[i]))
       {
          if (Cluster >= volume->SegmentClusterStart[i])
	  {
             if (Cluster < volume->LastAllocatedIndex[i])
                volume->LastAllocatedIndex[i] = Cluster;
             break; 
	  }
       }
    }
    return (SetBitBlockValue(&volume->FreeBlockList, Cluster, flag));
}

ULONG FindFreeCluster(VOLUME *volume, ULONG Cluster)
{
#if (VERBOSE)
    NWFSPrint("find_free_cluster\n"); 
#endif 
    return (ScanAndSetBitBlockValueWithIndex(&volume->FreeBlockList, 0, Cluster,
			    1));
}

//
//
//

ULONG InitializeClusterAssignedList(VOLUME *volume)
{
   return (CreateBitBlockList(&volume->AssignedBlockList,
			      volume->VolumeClusters,
			      BIT_BLOCK_SIZE,
			      "cluster assign"));
}

ULONG ExtendClusterAssignedList(VOLUME *volume, ULONG Amount)
{
    return (AdjustBitBlockList(&volume->AssignedBlockList, Amount));
}

ULONG FreeClusterAssignedList(VOLUME *volume)
{
    return (FreeBitBlockList(&volume->AssignedBlockList));
}

ULONG GetAssignedClusterValue(VOLUME *volume, ULONG Cluster)
{
    return (GetBitBlockValue(&volume->AssignedBlockList, Cluster));
}

ULONG SetAssignedClusterValue(VOLUME *volume, ULONG Cluster, ULONG flag)
{
    return (SetBitBlockValue(&volume->AssignedBlockList, Cluster, flag));
}

ULONG FindAssignedCluster(VOLUME *volume, ULONG Cluster)
{
    return (ScanBitBlockValueWithIndex(&volume->AssignedBlockList, 0, Cluster, 
			    1));
}

//
//  This routine compares the free list built from the fat against
//  a free list built from reported allocations in the directory,
//  suballoc file chains, and DOS/MAC namespace data chains to
//  determine if any orphaned or broken fat chains are wasting
//  space on the device.
//

ULONG AdjustAllocatedClusters(VOLUME *volume)
{
    register ULONG total = 0;

#if (!WINDOWS_NT_RO)
    register ULONG i, val1, val2, ccode, retries;
#if (CACHE_FAT_TABLES)
    extern ULONG FlushFAT(VOLUME *volume);
#endif
#endif

#if (MOUNT_VERBOSE)
    NWFSPrint("*** Verifying Volume Allocation ***\n");
#endif

    // if the count of empty subdirectories on this volume is greater
    // than the current free directory blocks, then preallocate
    // free space in the directory file until the count of empty
    // directories is greater than or equal to the number of
    // free directory blocks in the directory file.  Netware treats
    // this as a fatal error if any empty subdirectories exist for
    // which there is no corresponding free directory block.  We
    // don't treat this as a fatal error, and will attempt to
    // pre-allocate free records until a reasonable limit
    // is reached.  What is odd about this case is that Netware 4.2
    // will actually break this rule, and create empty subdirectories
    // without populating them properly, then vrepair the volume
    // when it mounts.

#if (!WINDOWS_NT_RO)

#if (VERBOSE)
    NWFSPrint("nwfs:  free dir block count-%d free dir count-%d\n",
	     (int)volume->FreeDirectoryBlockCount,
	     (int)volume->FreeDirectoryCount);
#endif

    if (volume->FreeDirectoryBlockCount < volume->FreeDirectoryCount)
    {
       retries = 0;
       while (volume->FreeDirectoryBlockCount < volume->FreeDirectoryCount)
       {
	  ccode = PreAllocateEmptyDirectorySpace(volume);
	  if (ccode)
	     break;

	  if (retries++ > 10)  // if we get stuck in a loop here, then exit
	  {
	     NWFSPrint("nwfs:  exceeded pre-allocated record limit (verify) [%u-%u]\n",
			(unsigned int)volume->FreeDirectoryCount,
			(unsigned int)volume->FreeDirectoryBlockCount);
	     break;
	  }
       }
    }

    for (i=0; i < volume->MountedVolumeClusters; i++)
    {
       val1 = GetFreeClusterValue(volume, i);
       val2 = GetAssignedClusterValue(volume, i);

       if (val1 != val2)
       {
	  // if the record is unallocated in the free list but we found
	  // an allocation for this cluster in either the directory,
	  // extdirectory, suballoc, or macintosh chain, then mark this
	  // cluster as free in the cluster free list.

	  if (!val1 && val2)
	  {
	     // this means the volume is corrupted.
	     NWFSPrint("nwfs:  directory reports allocation not in fat table\n");
	     return 0;
	  }

	  // set this cluster as free and record that we have freed an
	  // unassigned cluster.
	  SetFreeClusterValue(volume, i, 0);

	  // clear the cluster fields in the fat table
	  FreeClusterNoFlush(volume, i);

	  // track total clusters freed
	  total++;
       }
    }

#if (CACHE_FAT_TABLES)
    // flush out dirty fat buffers
    FlushFAT(volume);
#endif
    
#endif
    return total;
}

ULONG NWUpdateFat(VOLUME *volume, FAT_ENTRY *FAT, MIRROR_LRU *lru,
		  ULONG index, ULONG value, ULONG cluster)
{
    register ULONG retCode;
    
    if (!FAT || !lru)
       return -1;

    FAT->FATIndex = index;
    FAT->FATCluster = value;
    
    retCode = WriteFATEntry(volume, lru, FAT, cluster);
    if (retCode)
       NWFSPrint("WriteFATEntry cluster-%X error\n", (unsigned)cluster);

#if (CACHE_FAT_TABLES)
    FlushFATBuffer(volume, lru, cluster);    
#endif

    return retCode;
}

ULONG NWUpdateFatNoFlush(VOLUME *volume, FAT_ENTRY *FAT, MIRROR_LRU *lru,
			 ULONG index, ULONG value, ULONG cluster)
{
    register ULONG retCode;
    
    if (!FAT || !lru)
       return -1;

    FAT->FATIndex = index;
    FAT->FATCluster = value;
    
    retCode = WriteFATEntry(volume, lru, FAT, cluster);
    if (retCode)
       NWFSPrint("WriteFATEntry cluster-%X error\n", (unsigned)cluster);

    if (lru)
       lru->BlockState = L_DIRTY;

    return 0;
}

ULONG GetChainSize(VOLUME *volume, long ClusterChain)
{
    register ULONG cluster;
    ULONG total = 0;
    register ULONG index = 0;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;

    if ((!ClusterChain) || (ClusterChain == (ULONG) -1))
       return total;

    cluster = ClusterChain;
    if (cluster & 0x80000000)
    {
       if (cluster == (ULONG) -1)
	  return total;

       if (cluster & 0x80000000)
	  total += GetSuballocSize(volume, cluster);

       return total;
    }

    FAT = GetFatEntry(volume, cluster, &FAT_S);
    if (FAT)
       index = FAT->FATIndex;

    while (FAT && FAT->FATCluster)
    {
       // If we detect a non-ascending index sequence, then we assume
       // that the fat chain is corrupt.
       if (total > ((index + 1) * volume->ClusterSize))
	  return total;

       // set total to indexed offset in fat chain plus the current
       // cluster allocation.
       total = ((index + 1) * volume->ClusterSize);

       // bump to next cluster
       cluster = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF
       if (cluster & 0x80000000)
       {
	  if (cluster == (ULONG) -1)
	     return total;

	  if (cluster & 0x80000000)
	     total += GetSuballocSize(volume, cluster);

	  return total;
       }

       FAT = GetFatEntry(volume, cluster, &FAT_S);
       if (FAT)
	  index = FAT->FATIndex;
    }
    return total;

}

ULONG GetChainSizeValidate(VOLUME *volume, 
		           long ClusterChain, 
		           ULONG SAFlag, 
		           ULONG *total)    
{
    register ULONG cluster, index = 0;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;

    if (!total)
       return NwInvalidParameter;
    *total = 0;
    
    if (!ClusterChain)
       return NwChainBad;

    if (ClusterChain == (ULONG) -1)
       return 0;

    cluster = ClusterChain;
    if (cluster & 0x80000000)
    {
       if (cluster == (ULONG) -1)
	  return 0;

       if (cluster & 0x80000000)
       {
          if (!SAFlag)
             return NwInvalidParameter;

	  (*total) += GetSuballocSize(volume, cluster);
       }
       return 0;
    }

    FAT = GetFatEntry(volume, cluster, &FAT_S);
    if (FAT)
       index = FAT->FATIndex;

    if (!FAT->FATCluster)
       return NwChainBad;

    while (FAT && FAT->FATCluster)
    {
       // If we detect a non-ascending index sequence, then we assume
       // that the fat chain is corrupt.
       if ((*total) > ((index + 1) * volume->ClusterSize))
	  return NwChainBad;

       // set total to indexed offset in fat chain plus the current
       // cluster allocation.
       (*total) = ((index + 1) * volume->ClusterSize);

       // bump to next cluster
       cluster = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF
       if (cluster & 0x80000000)
       {
	  if (cluster == (ULONG) -1)
	     return 0;

	  if (cluster & 0x80000000)
	  {
             if (!SAFlag)
                return NwInvalidParameter;
	     
	     (*total) += GetSuballocSize(volume, cluster);
	  }
	  return 0;
       }

       FAT = GetFatEntry(volume, cluster, &FAT_S);
       if (FAT)
	  index = FAT->FATIndex;
        
       if (!FAT->FATCluster)
          return NwChainBad;
    }
    return 0;

}

// this function builds a detailed allocation map of all clusters
// and suballoc elements that exist within the specified fat
// chain.

ULONG BuildChainAssignment(VOLUME *volume, ULONG Chain, ULONG SAFlag)
{
    register ULONG cluster, retCode;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;

    if ((!Chain) || (Chain == (ULONG) -1))
       return 0;

    cluster = Chain;
    if (cluster & 0x80000000)
    {
       if (cluster == (ULONG) -1)
	  return 0;

       if (cluster & 0x80000000)
       {
	  if (!SAFlag)
	     return NwInvalidParameter;

	  retCode = SetSuballocListValue(volume, cluster, 1);
	  if (retCode)
	     NWFSPrint("nwfs:  multiple files point to suballoc record\n");

	  return retCode;
       }

       return 0;
    }

    FAT = GetFatEntry(volume, cluster, &FAT_S);
    while (FAT && FAT->FATCluster)
    {
       retCode = GetAssignedClusterValue(volume, cluster);
       if (retCode)
       {
	  NWFSPrint("nwfs:  detected invalid assigned cluster-%X\n",
		    (unsigned int)cluster);
	  return -1;
       }

       retCode = SetAssignedClusterValue(volume, cluster, 1);
       if (retCode)
       {
	  NWFSPrint("nwfs:  assigned bit block value not set cluster-%X\n",
		    (unsigned int)cluster);
       }

       // bump to next cluster
       cluster = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF
       if (cluster & 0x80000000)
       {
	  if (cluster == (ULONG) -1)
	     return 0;

	  if (cluster & 0x80000000)
	  {
	     if (!SAFlag)
		return NwInvalidParameter;

	     retCode = SetSuballocListValue(volume, cluster, 1);
	     if (retCode)
		NWFSPrint("nwfs:  multiple files point to suballoc record\n");

	     return retCode;
	  }
	  return 0;
       }
       FAT = GetFatEntry(volume, cluster, &FAT_S);
    }
    return 0;

}

ULONG VerifyChainAssignment(VOLUME *volume, ULONG Chain, ULONG SAFlag)
{
    register ULONG cluster, retCode = 0;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;

    if ((!Chain) || (Chain == (ULONG) -1))
       return 0;

    cluster = Chain;
    if (cluster & 0x80000000)
    {
       if (cluster == (ULONG) -1)
	  return retCode;

       if (cluster & 0x80000000)
       {
	  if (!SAFlag)
	     return NwInvalidParameter;

	  if (!GetSuballocListValue(volume, cluster))
	  {
	     NWFSPrint("nwfs:  suballoc [%08X] record verifies as free\n",
		      (unsigned int)cluster);
	     retCode++;
	  }
       }
       return retCode;
    }

    FAT = GetFatEntry(volume, cluster, &FAT_S);
    while (FAT && FAT->FATCluster)
    {
       retCode = GetAssignedClusterValue(volume, cluster);
       if (!retCode)
       {
	  NWFSPrint("nwfs:  cluster [%08X] verifies as free\n",
		    (unsigned int)cluster);
	  retCode++;
       }

       // bump to next cluster
       cluster = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF
       if (cluster & 0x80000000)
       {
	  if (cluster == (ULONG) -1)
	     return retCode;

	  if (cluster & 0x80000000)
	  {
	     if (!SAFlag)
		return NwInvalidParameter;

	     if (!GetSuballocListValue(volume, cluster))
	     {
		NWFSPrint("nwfs:  suballoc [%08X] record verifies as free\n",
			 (unsigned int)cluster);
		retCode++;
	     }
	  }
	  return retCode;
       }
       FAT = GetFatEntry(volume, cluster, &FAT_S);
    }
    return retCode;

}

ULONG FreeCluster(VOLUME *volume, long cluster)
{
    register ULONG retCode;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    MIRROR_LRU *lru = 0;

    if ((cluster < 0) || (cluster == -1))
       return -1;

    FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
    if (FAT)
    {
       retCode = NWUpdateFat(volume, FAT, lru, 0, 0, cluster);
       if (retCode)
       {
	  NWFSPrint("*** error updating FAT [%X/%X] ***\n",
		      (unsigned int)lru->Cluster1,
		      (unsigned int)lru->Cluster2);
	  return -1;
       }

       // set current cluster free in bit block list

       SetFreeClusterValue(volume, cluster, 0);
       volume->VolumeFreeClusters++;
       if (volume->VolumeAllocatedClusters)
	  volume->VolumeAllocatedClusters--;

       return 0;
    }
    else
       NWFSPrint("nwfs:  cluster FAT Range Error (FreeCluster)\n");
    return -1;

}

ULONG FreeClusterNoFlush(VOLUME *volume, long cluster)
{
    register ULONG retCode;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    MIRROR_LRU *lru = 0;

    if ((cluster < 0) || (cluster == -1))
       return -1;

    FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
    if (FAT)
    {
       retCode = NWUpdateFatNoFlush(volume, FAT, lru, 0, 0, cluster);
       if (retCode)
       {
	  NWFSPrint("*** error updating FAT [%X/%X] ***\n",
		      (unsigned int)lru->Cluster1,
		      (unsigned int)lru->Cluster2);
	  return -1;
       }

       // set current cluster free in bit block list

       SetFreeClusterValue(volume, cluster, 0);
       volume->VolumeFreeClusters++;
       if (volume->VolumeAllocatedClusters)
	  volume->VolumeAllocatedClusters--;

       return 0;
    }
    else
       NWFSPrint("nwfs:  cluster FAT Range Error (FreeClusterNoFlush)\n");
    return -1;

}

ULONG SetClusterValue(VOLUME *volume, long cluster, ULONG Value)
{
    register ULONG retCode;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    MIRROR_LRU *lru = 0;

    // don't allow people to free clusters with this call
    if ((cluster < 0) || (cluster == -1) || (!Value))
       return -1;

    FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
    if (FAT)
    {
       retCode = NWUpdateFat(volume, FAT, lru, FAT->FATIndex, Value, cluster);
       if (retCode)
       {
	  NWFSPrint("*** error updating FAT [%X/%X] ***\n",
			  (unsigned int)lru->Cluster1,
			  (unsigned int)lru->Cluster2);
	  return -1;
       }
       return 0;
    }
    else
       NWFSPrint("nwfs:  cluster FAT Range Error (SetClusterValue)\n");
    return -1;

}

ULONG SetClusterIndex(VOLUME *volume, long cluster, ULONG Index)
{
    register ULONG retCode;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    MIRROR_LRU *lru = 0;

    if ((cluster < 0) || (cluster == -1))
       return 0;

    FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
    if (FAT)
    {
       retCode = NWUpdateFat(volume, FAT, lru, Index, FAT->FATCluster, cluster);
       if (retCode)
       {
	  NWFSPrint("*** error updating FAT [%X/%X] ***\n",
		      (unsigned int)lru->Cluster1,
		      (unsigned int)lru->Cluster2);
	  return -1;
       }
       return 0;
    }
    else
       NWFSPrint("nwfs:  cluster FAT Range Error (SetClusterIndex)\n");
    return -1;

}

ULONG SetClusterValueAndIndex(VOLUME *volume, long cluster,
			     ULONG Value, ULONG Index)
{
    register ULONG retCode;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    MIRROR_LRU *lru = 0;

    // don't allow people to free clusters with this call
    if ((cluster < 0) || (cluster == -1) || (!Value))
       return -1;

    FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
    if (FAT)
    {
       retCode = NWUpdateFat(volume, FAT, lru, Index, Value, cluster);
       if (retCode)
       {
	  NWFSPrint("*** error updating FAT [%X/%X] ***\n",
			  (unsigned int)lru->Cluster1,
			  (unsigned int)lru->Cluster2);
	  return -1;
       }
       return 0;
    }
    else
       NWFSPrint("nwfs:  cluster FAT Range Error (SetClusterValueAndIndex)\n");
    return -1;
}

//
//  Below is the method Netware uses to perform striping for multi-
//  segmented volumes.  the method employed by Netware is more
//  efficient than raid or the block striping typical of NT and Unix,
//  and is incredibly simple.
//
//  Netware keeps track of which segment was last allocated from, it
//  then round robins between the segments during cluster allocation,
//  and evenly distributes files across individual segments.  this
//  architecture lends itself well to additive storage.  It is
//  extremely simple to splice additional segments to the end
//  of a Netware volume to increase its size over time.
//
//  one disadvantage to this method, however, is that if you lose
//  a segment in a netware volume, it renders the entire volume
//  unusable until the volume has been repaired.  mirroring
//  is advised for multi-segmented volumes.
//

ULONG AllocateCluster(VOLUME *volume)
{
    register ULONG cluster, retCode;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    MIRROR_LRU *lru = 0;

#if (VERBOSE)
    NWFSPrint("allocate cluster\n");
#endif
    
    // start looking for a free cluster on the current segment's last
    // allocated index 
    cluster = FindFreeCluster(volume,
		 volume->LastAllocatedIndex[volume->AllocSegment]);
    
    // start looking for a free cluster on the current segment
    if (cluster == (ULONG) -1)
    {
       cluster = FindFreeCluster(volume, 
		         volume->SegmentClusterStart[volume->AllocSegment]);

       // start looking at the beginning of the volume
       if (cluster == (ULONG) -1)
       {
          cluster = FindFreeCluster(volume, volume->SegmentClusterStart[0]);
          if (cluster == (ULONG) -1)
	     return -1;
       }
    }
    	
    if (cluster != (ULONG) -1)
       volume->LastAllocatedIndex[volume->AllocSegment] = cluster;

    // bump to the segment pointer to the next segment.  range check
    // and reset to zero if we wrap.

    volume->AllocSegment++;
    if (volume->AllocSegment >= volume->MountedNumberOfSegments)
       volume->AllocSegment = 0;

    if (cluster == (ULONG) -1)
       return -1;

    FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
    if (FAT)
    {
       if (volume->VolumeFreeClusters)
	  volume->VolumeFreeClusters--;
       volume->VolumeAllocatedClusters++;

       retCode = NWUpdateFat(volume, FAT, lru, 0, (ULONG) -1, cluster);
       if (retCode)
       {
	  NWFSPrint("*** error updating FAT [%X/%X] ***\n",
			  (unsigned int)lru->Cluster1,
			  (unsigned int)lru->Cluster2);
	  return -1;
       }
       return cluster;
    }
    else
       NWFSPrint("nwfs:  cluster FAT Range Error (AllocateCluster)\n");
    return -1;
}

ULONG AllocateClusterSetIndex(VOLUME *volume, ULONG Index)
{
    register ULONG cluster, retCode;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    MIRROR_LRU *lru = 0;

#if (VERBOSE)
    NWFSPrint("allocate cluster set index\n");
#endif

    // start looking for a free cluster on the current segment's last
    // allocated index 
    cluster = FindFreeCluster(volume,
		 volume->LastAllocatedIndex[volume->AllocSegment]);
    
    // start looking for a free cluster on the current segment
    if (cluster == (ULONG) -1)
    {
       cluster = FindFreeCluster(volume,
		 volume->SegmentClusterStart[volume->AllocSegment]);

       // start looking at the beginning of the volume
       if (cluster == (ULONG) -1)
       {
          cluster = FindFreeCluster(volume, volume->SegmentClusterStart[0]);
          if (cluster == (ULONG) -1)
	     return -1;
       }
    }
    	
    if (cluster != (ULONG) -1)
       volume->LastAllocatedIndex[volume->AllocSegment] = cluster;

    // bump to the segment pointer to the next segment.  range check
    // and reset to zero if we wrap.

    volume->AllocSegment++;
    if (volume->AllocSegment >= volume->MountedNumberOfSegments)
       volume->AllocSegment = 0;

    if (cluster == (ULONG) -1)
       return -1;

    FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
    if (FAT)
    {
       if (volume->VolumeFreeClusters)
	  volume->VolumeFreeClusters--;
       volume->VolumeAllocatedClusters++;

       retCode = NWUpdateFat(volume, FAT, lru, Index, (ULONG) -1, cluster);
       if (retCode)
       {
	  NWFSPrint("*** error updating FAT [%X/%X] ***\n",
			  (unsigned int)lru->Cluster1,
			  (unsigned int)lru->Cluster2);
	  return -1;
       }
       return cluster;
    }
    else
       NWFSPrint("nwfs:  cluster FAT Range Error (AllocateClusterSetIndex)\n");
    return -1;
}

ULONG AllocateClusterSetIndexSetChain(VOLUME *volume, ULONG Index, ULONG Next)
{
    register ULONG cluster, retCode;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    MIRROR_LRU *lru = 0;

#if (VERBOSE)
    NWFSPrint("allocate cluster set chain/index\n");
#endif
    
    // start looking for a free cluster on the current segment's last
    // allocated index 
    cluster = FindFreeCluster(volume,
		 volume->LastAllocatedIndex[volume->AllocSegment]);
    
    // start looking for a free cluster on the current segment
    if (cluster == (ULONG) -1)
    {
       cluster = FindFreeCluster(volume,
		 volume->SegmentClusterStart[volume->AllocSegment]);

       // start looking at the beginning of the volume
       if (cluster == (ULONG) -1)
       {
          cluster = FindFreeCluster(volume, volume->SegmentClusterStart[0]);
          if (cluster == (ULONG) -1)
	     return -1;
       }
    }
    	
    if (cluster != (ULONG) -1)
       volume->LastAllocatedIndex[volume->AllocSegment] = cluster;

    // bump to the segment pointer to the next segment.  range check
    // and reset to zero if we wrap.

    volume->AllocSegment++;
    if (volume->AllocSegment >= volume->MountedNumberOfSegments)
       volume->AllocSegment = 0;

    if (cluster == (ULONG) -1)
       return -1;

    FAT = GetFatEntryAndLRU(volume, cluster, &lru, &FAT_S);
    if (FAT)
    {
       if (volume->VolumeFreeClusters)
	  volume->VolumeFreeClusters--;
       volume->VolumeAllocatedClusters++;

       retCode = NWUpdateFat(volume, FAT, lru, Index, Next, cluster);
       if (retCode)
       {
	  NWFSPrint("*** error updating FAT [%X/%X] ***\n",
			  (unsigned int)lru->Cluster1,
			  (unsigned int)lru->Cluster2);
	  return -1;
       }
       return cluster;
    }
    else
       NWFSPrint("nwfs:  cluster FAT Range Error (AllocateClusterSetIndexSetChain)\n");
    return -1;
}

ULONG ReadAbsoluteVolumeCluster(VOLUME *volume, ULONG Cluster, BYTE *Buffer)
{
    register ULONG i, ccode;

    if (Cluster >= volume->MountedVolumeClusters)
    {
       NWFSPrint("bad cluster address in ReadVolumeCluster\n");
       NWFSPrint(" %x >= %x \n", (int) Cluster,
		(int) volume->MountedVolumeClusters);
       return NwInvalidParameter;
    }

    for (i=0; i < volume->BlocksPerCluster; i++)
    {
       ccode = nwvp_vpartition_block_read(volume->nwvp_handle, 
			            (Cluster * volume->BlocksPerCluster) + i,
                                    1, Buffer);
       if (ccode)
          return NwDiskIoError;

       Buffer += volume->BlockSize;
    }
    return 0;

}

ULONG WriteAbsoluteVolumeCluster(VOLUME *volume, ULONG Cluster, BYTE *Buffer)
{
    register ULONG i, ccode;

    if (Cluster >= volume->MountedVolumeClusters)
    {
       NWFSPrint("bad cluster address in WriteVolumeCluster\n");
       NWFSPrint(" %x >= %x \n", (int) Cluster,
		(int) volume->MountedVolumeClusters);
       return NwInvalidParameter;
    }

    for (i=0; i < volume->BlocksPerCluster; i++)
    {
       ccode = nwvp_vpartition_block_write(volume->nwvp_handle, 
			            (Cluster * volume->BlocksPerCluster) + i,
                                    1, Buffer);
       if (ccode)
          return NwDiskIoError;

       Buffer += volume->BlockSize;
    }
    return 0;
}

ULONG ReadPhysicalVolumeCluster(VOLUME *volume, ULONG Cluster,
				BYTE *Buffer, ULONG size, ULONG priority)
{
    register ULONG i, vsize;
    register long bytesLeft;
    register LRU *lru;
    register LRU_HANDLE *lru_handle = &DataLRU;
    ULONG mirror = (ULONG)-1;

    if (Cluster >= volume->MountedVolumeClusters)
    {
       NWFSPrint("bad cluster address in ReadVolumeCluster\n");
       NWFSPrint(" %x >= %x \n", (int) Cluster,
		(int) volume->MountedVolumeClusters);
       return NwInvalidParameter;
    }

    if (size > volume->ClusterSize)
    {
       NWFSPrint("cluster size bad in ReadVolumeCluster\n");
       return NwInvalidParameter;
    }

    if (priority == RAW_PRIORITY)
       return (ReadAbsoluteVolumeCluster(volume, Cluster, Buffer));
    
    switch (priority)
    {
       case FAT_PRIORITY:
          lru_handle = &FATLRU;
	  break;
		  
       case DIRECTORY_PRIORITY:
          lru_handle = &JournalLRU;
	  break;
		  
       default:
	  break;
    }

    for (bytesLeft = size, i=0; i < volume->BlocksPerCluster; i++)
    {
       vsize = (bytesLeft > (long)volume->BlockSize)
	      ? volume->BlockSize : bytesLeft;

       lru = ReadLRU(lru_handle, volume,
		    (Cluster * volume->BlocksPerCluster) + i,
		     1, priority,
		     ((i == 0) ? (volume->BlocksPerCluster - i) : 1),
		     &mirror);

       if (!lru)
	  return NwDiskIoError;

       if (lru->bad_bits)
       {
          ReleaseLRU(lru_handle, lru);
	  return NwDiskIoError;
       }
       
       NWFSCopy(Buffer, lru->buffer, vsize);
       ReleaseLRU(lru_handle, lru);

       Buffer += vsize;
       bytesLeft -= vsize;
    }
    return 0;

}

ULONG WritePhysicalVolumeCluster(VOLUME *volume, ULONG Cluster,
				 BYTE *Buffer, ULONG size, ULONG priority)
{
    register ULONG i, vsize;
    register long bytesLeft;
    register LRU *lru;
    register LRU_HANDLE *lru_handle = &DataLRU;
    ULONG mirror = (ULONG)-1;

    if (Cluster >= volume->MountedVolumeClusters)
    {
       NWFSPrint("bad cluster address in WriteVolumeCluster\n");
       NWFSPrint(" %x >= %x \n", (int) Cluster,
		(int) volume->MountedVolumeClusters);
       return NwInvalidParameter;
    }

    if (size > volume->ClusterSize)
    {
       NWFSPrint("cluster size bad in WriteVolumeCluster\n");
       return NwInvalidParameter;
    }

    if (priority == RAW_PRIORITY)
       return (WriteAbsoluteVolumeCluster(volume, Cluster, Buffer));
    
    switch (priority)
    {
       case FAT_PRIORITY:
          lru_handle = &FATLRU;
	  break;
		  
       case DIRECTORY_PRIORITY:
          lru_handle = &JournalLRU;
	  break;
		  
       default:
	  break;
    }
    
    for (bytesLeft = size, i=0; i < volume->BlocksPerCluster; i++)
    {
       vsize = (bytesLeft > (long)volume->BlockSize)
	      ? volume->BlockSize : bytesLeft;

       if (vsize == volume->BlockSize)
	  lru = ReadLRU(lru_handle, volume,
		       (Cluster * volume->BlocksPerCluster) + i,
			0, priority, 1, &mirror);
       else
	  lru = ReadLRU(lru_handle, volume,
		       (Cluster * volume->BlocksPerCluster) + i,
			1, priority, 1, &mirror);

       if (!lru)
	  return NwDiskIoError;

       lru->bad_bits = 0;
       NWFSCopy(lru->buffer, Buffer, vsize);
       ReleaseDirtyLRU(lru_handle, lru);

       Buffer += vsize;
       bytesLeft -= vsize;
    }
    return 0;
}

ULONG ZeroPhysicalVolumeCluster(VOLUME *volume, ULONG Cluster, ULONG priority)
{
    register ULONG i, vsize;
    register long bytesLeft;
    register LRU *lru;
    register LRU_HANDLE *lru_handle = &DataLRU;
    ULONG mirror = (ULONG)-1;

    if (Cluster >= volume->MountedVolumeClusters)
    {
       NWFSPrint("bad cluster address in ZeroVolumeCluster\n");
       NWFSPrint(" %x >= %x \n", (int) Cluster,
		(int) volume->MountedVolumeClusters);
       return NwInvalidParameter;
    }

    if (priority == RAW_PRIORITY)
       return (WriteAbsoluteVolumeCluster(volume, Cluster, ZeroBuffer));
    
    switch (priority)
    {
       case FAT_PRIORITY:
          lru_handle = &FATLRU;
	  break;
		  
       case DIRECTORY_PRIORITY:
          lru_handle = &JournalLRU;
	  break;
		  
       default:
	  break;
    }

    for (bytesLeft = volume->ClusterSize, i=0;
	 i < volume->BlocksPerCluster;
	 i++)
    {
       vsize = (bytesLeft > (long)volume->BlockSize)
	      ? volume->BlockSize : bytesLeft;

       if (vsize == volume->BlockSize)
	  lru = ReadLRU(lru_handle, volume,
		       (Cluster * volume->BlocksPerCluster) + i,
			0, priority, 1, &mirror);
       else
	  lru = ReadLRU(lru_handle, volume,
		       (Cluster * volume->BlocksPerCluster) + i,
			1, priority, 1, &mirror);
       if (!lru)
	  return NwDiskIoError;

       lru->bad_bits = 0;
       NWFSSet(lru->buffer, 0, vsize);
       ReleaseDirtyLRU(lru_handle, lru);

       bytesLeft -= vsize;
    }
    return 0;
}

ULONG SyncCluster(VOLUME *volume, ULONG Cluster)
{
    register ULONG retCode = 0;

    if (Cluster >= volume->MountedVolumeClusters)
    {
       NWFSPrint("bad cluster address in SyncCluster\n");
       NWFSPrint(" %x >= %x \n", (int) Cluster,
		(int) volume->MountedVolumeClusters);
       return NwInvalidParameter;
    }

    retCode = SyncLRUBlocks(volume,
			   (Cluster * volume->BlocksPerCluster),
			   volume->BlocksPerCluster);
    return retCode;
}

ULONG ReadAheadVolumeCluster(VOLUME *volume, ULONG Cluster)
{
    register ULONG retCode = 0;
    register LRU_HANDLE *lru_handle = &DataLRU;
#if (READ_AHEAD_ON)
    ULONG mirror = (ULONG)-1;
#endif

    if (Cluster >= volume->MountedVolumeClusters)
    {
       NWFSPrint("bad cluster address in ReadAheadVolumeCluster\n");
       NWFSPrint(" %x >= %x \n", (int) Cluster,
		(int) volume->MountedVolumeClusters);
       return NwInvalidParameter;
    }

#if (READ_AHEAD_ON)
    retCode = PerformBlockReadAhead(lru_handle, volume,
				   (Cluster * volume->BlocksPerCluster),
				    volume->BlocksPerCluster, 1, &mirror);
#endif
    return retCode;
}

//
//  these functions read and write clusters with an offset and size
//  and handle wrap cases with individual logical disk blocks that
//  can span sectors
//

ULONG ReadClusterWithOffset(VOLUME *volume, ULONG Cluster,
			    ULONG offset, BYTE *buf, ULONG size,
			    ULONG as, ULONG *retCode, ULONG priority)
{
    register long bytesLeft, voffset, vsize, vindex;
    register ULONG StartIndex, StartOffset, i;
    register ULONG bytesRead = 0;
    register LRU *lru;
    register LRU_HANDLE *lru_handle = &DataLRU;
    register BYTE *workbuffer = 0; 
    ULONG mirror = (ULONG)-1;

    if (retCode)
       *retCode = 0;

    if (Cluster >= volume->MountedVolumeClusters)
    {
       NWFSPrint("bad cluster address in ReadVolumeCluster [%X]\n",
		 (unsigned int) Cluster);
       if (retCode)
	  *retCode = NwInvalidParameter;
       return 0;
    }

    if ((offset + size) > volume->ClusterSize)
    {
       NWFSPrint("read size bad in ReadClusterWithOffset\n");
       if (retCode)
	  *retCode = NwInvalidParameter;
       return 0;
    }

    if (priority == RAW_PRIORITY)
    {
       register ULONG ccode;
     
       workbuffer = NWFSAlloc(IO_BLOCK_SIZE, BLOCK_TAG);
       if (!workbuffer)
          return NwInsufficientResources;
    
       StartIndex = offset / volume->BlockSize;
       StartOffset = offset % volume->BlockSize;

       for (bytesLeft = size, vindex = StartIndex;
	   (vindex < (long)volume->BlocksPerCluster) && (bytesLeft > 0);
	    vindex++)
       {
          voffset = 0;
          if (vindex == (long)StartIndex)
	     voffset = StartOffset;

          vsize = (bytesLeft > (long)(volume->BlockSize - voffset))
	      ? (volume->BlockSize - voffset) : bytesLeft;

          ccode = nwvp_vpartition_block_read(volume->nwvp_handle, 
		            (Cluster * volume->BlocksPerCluster) + vindex,
                                    1, workbuffer);
          if (ccode)
          {
	     if (retCode)
	        *retCode = NwDiskIoError;
	     break;
          }

          if (as)
	     NWFSCopy(buf, &workbuffer[voffset], vsize);
          else
          {
	     if (NWFSCopyToUserSpace(buf, &workbuffer[voffset], vsize))
	     {
                if (retCode)
	           *retCode = NwMemoryFault;
                if (workbuffer)
	           NWFSFree(workbuffer);
	        return bytesRead;
	     }
          }
          buf += vsize;
          bytesRead += vsize;
          bytesLeft -= vsize;
       }
    }
    else
    {
       switch (priority)
       {
          case FAT_PRIORITY:
             lru_handle = &FATLRU;
	     break;
		  
	  case DIRECTORY_PRIORITY:
             lru_handle = &JournalLRU;
	     break;
		  
          default:
	     break;
       }

       StartIndex = offset / volume->BlockSize;
       StartOffset = offset % volume->BlockSize;

       for (bytesLeft = size, vindex = StartIndex;
	   (vindex < (long)volume->BlocksPerCluster) && (bytesLeft > 0);
	    vindex++)
       {
          voffset = 0;
          if (vindex == (long)StartIndex)
	     voffset = StartOffset;

          vsize = (bytesLeft > (long)(volume->BlockSize - voffset))
	         ? (volume->BlockSize - voffset) : bytesLeft;

          lru = ReadLRU(lru_handle, volume,
		    (Cluster * volume->BlocksPerCluster) + vindex,
		     1, priority,
                    ((vindex == (long)StartIndex)
		     ? (volume->BlocksPerCluster - vindex) : 1),
		     &mirror);
          if (!lru)
          {
	     if (retCode)
	        *retCode = NwDiskIoError;
	     break;
          }

          for (i = (voffset / 512); 
              (i < (voffset + vsize) / 512) && (i < 8); 
	      i++)
          {
             if ((lru->bad_bits >> i) & 1)
	     {
                ReleaseLRU(lru_handle, lru);
	        if (retCode)
	           *retCode = NwDiskIoError;
	        break;
	     }
          }

          if (as)
	     NWFSCopy(buf, &lru->buffer[voffset], vsize);
          else
          {
	     if (NWFSCopyToUserSpace(buf, &lru->buffer[voffset], vsize))
	     {
                if (retCode)
	           *retCode = NwMemoryFault;
	        return bytesRead;
	     }
          }

          ReleaseLRU(lru_handle, lru);

          buf += vsize;
          bytesRead += vsize;
          bytesLeft -= vsize;
       }
    }

    if (workbuffer)
       NWFSFree(workbuffer);
    return bytesRead;
}

ULONG WriteClusterWithOffset(VOLUME *volume, ULONG Cluster,
			     ULONG offset, BYTE *buf, ULONG size,
			     ULONG as, ULONG *retCode, ULONG priority)
{
    register long bytesLeft, voffset, vsize, vindex;
    register ULONG StartIndex, StartOffset, i;
    register ULONG bytesWritten = 0;
    register LRU *lru;
    register LRU_HANDLE *lru_handle = &DataLRU;
    register BYTE *workbuffer = 0; 
    ULONG mirror = (ULONG)-1;

    if (retCode)
       *retCode = 0;

    if (Cluster >= volume->MountedVolumeClusters)
    {
       NWFSPrint("bad cluster address in ReadVolumeCluster [%X]\n",
		 (unsigned int) Cluster);
       if (retCode)
	  *retCode = NwInvalidParameter;
       return 0;
    }

    if ((offset + size) > volume->ClusterSize)
    {
       NWFSPrint("read size bad in ReadClusterWithOffset\n");
       if (retCode)
	  *retCode = NwInvalidParameter;
       return 0;
    }

    if (priority == RAW_PRIORITY)
    {
       register ULONG ccode;

       workbuffer = NWFSAlloc(IO_BLOCK_SIZE, BLOCK_TAG);
       if (!workbuffer)
          return NwInsufficientResources;
    
       StartIndex = offset / volume->BlockSize;
       StartOffset = offset % volume->BlockSize;

       for (bytesLeft = size, vindex = StartIndex;
	   (vindex < (long)volume->BlocksPerCluster) && (bytesLeft > 0);
	    vindex++)
       {
          voffset = 0;
          if (vindex == (long)StartIndex)
	     voffset = StartOffset;

          vsize = (bytesLeft > (long)(volume->BlockSize - voffset))
	      ? (volume->BlockSize - voffset) : bytesLeft;

          if ((voffset != 0) ||
	     ((voffset == 0) && (vsize != (long)volume->BlockSize)))
	  {
             ccode = nwvp_vpartition_block_read(volume->nwvp_handle, 
		            (Cluster * volume->BlocksPerCluster) + vindex,
                             1, workbuffer);
             if (ccode)
             {
	        if (retCode)
	           *retCode = NwDiskIoError;
	        break;
             }
          }
       
          if (as)
	     NWFSCopy(&workbuffer[voffset], buf, vsize);
          else
          {
	     if (NWFSCopyFromUserSpace(&workbuffer[voffset], buf, vsize))
	     {
                if (retCode)
	           *retCode = NwMemoryFault;
                if (workbuffer)
	           NWFSFree(workbuffer);
     	        return bytesWritten;
	     }
          }

          ccode = nwvp_vpartition_block_write(volume->nwvp_handle, 
		            (Cluster * volume->BlocksPerCluster) + vindex,
                             1, workbuffer);
          if (ccode)
          {
	     if (retCode)
	        *retCode = NwDiskIoError;
	     break;
          }

	  buf += vsize;
          bytesWritten += vsize;
          bytesLeft -= vsize;
       }
    }
    else
    {
       switch (priority)
       {
          case FAT_PRIORITY:
             lru_handle = &FATLRU;
	     break;
		  
	  case DIRECTORY_PRIORITY:
             lru_handle = &JournalLRU;
	     break;
		  
          default:
	     break;
       }
     
       StartIndex = offset / volume->BlockSize;
       StartOffset = offset % volume->BlockSize;
       
       for (bytesLeft = size, vindex = StartIndex;
	   (vindex < (long)volume->BlocksPerCluster) && (bytesLeft > 0);
	    vindex++)
       {
          voffset = 0;
          if (vindex == (long)StartIndex)
	     voffset = StartOffset;

          vsize = (bytesLeft > (long)(volume->BlockSize - voffset))
	      ? (volume->BlockSize - voffset) : bytesLeft;

          if ((voffset == 0) && (vsize == (long)volume->BlockSize))
	     lru = ReadLRU(lru_handle, volume,
		       (Cluster * volume->BlocksPerCluster) + vindex,
			0, priority, 1, &mirror);
          else
	     lru = ReadLRU(lru_handle, volume,
		       (Cluster * volume->BlocksPerCluster) + vindex,
			1, priority, 1, &mirror);

          if (!lru)
          {
	     if (retCode)
	        *retCode = NwDiskIoError;
	     break;
          }

          for (i = (voffset / 512); 
              (i < (voffset + vsize) / 512) && (i < 8); 
	      i++)
          {
             lru->bad_bits &= ~(1 << i);
          }

          if (as)
	     NWFSCopy(&lru->buffer[voffset], buf, vsize);
          else
          {
	     if (NWFSCopyFromUserSpace(&lru->buffer[voffset], buf, vsize))
	     {
                if (retCode)
	           *retCode = NwMemoryFault;
     	        return bytesWritten;
	     }
          }

          ReleaseDirtyLRU(lru_handle, lru);

          buf += vsize;
          bytesWritten += vsize;
          bytesLeft -= vsize;
       }
    }

    if (workbuffer)
       NWFSFree(workbuffer);
    return bytesWritten;
}

