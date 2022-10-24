
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
*   FILE     :  SUBALLOC.C
*   DESCRIP  :   NetWare File System Suballocation Support
*   DATE     :  January 9, 2022
*
*
***************************************************************************/

#include "globals.h"

ULONG TrgDebugSA = 0;

void NWLockSuballoc(VOLUME *volume, ULONG blockCount)
{
    if (blockCount >= volume->SuballocListCount)
    {
       NWFSPrint("nwfs:  suballoc blockCount too big (%d/%d) in LockSuballoc\n",
		(int)blockCount, (int)volume->SuballocListCount);
       return;
    }
#if (LINUX_SLEEP)
    if (WaitOnSemaphore(&volume->SuballocSemaphore[blockCount]) == -EINTR)
       NWFSPrint("lock suballoc-%d was interrupted\n", (int)blockCount);
#endif
}

void NWUnlockSuballoc(VOLUME *volume, ULONG blockCount)
{
    if (blockCount >= volume->SuballocListCount)
    {
       NWFSPrint("nwfs:  suballoc blockCount too big (%d/%d) in UnlockSuballoc\n",
		(int)blockCount, (int)volume->SuballocListCount);
       return;
    }
#if (LINUX_SLEEP)
    SignalSemaphore(&volume->SuballocSemaphore[blockCount]);
#endif
}

ULONG NWCreateSuballocRecords(VOLUME *volume)
{
    register ULONG retCode;
    register SUBALLOC *suballoc;
    register ULONG i, LastDirNo;
    register ROOT *root;

    // if suballocation is disabled for this volume, then
    // return.
    if (!(volume->VolumeFlags & SUB_ALLOCATION_ON))
       return 0;

    if (!volume->ClusterSize)
       return -1;

    volume->VolumeSuballocRoot = 0;
    volume->SuballocChainComplete = 0;

    root = NWFSAlloc(sizeof(ROOT), DIR_WORKSPACE_TAG);
    if (!root)
    {
       NWFSPrint("nwfs:  could not get workspace memory\n");
       return NwInsufficientResources;
    }

    retCode = ReadDirectoryRecord(volume, (DOS *)root, 0);
    if (retCode)
    {
       NWFSPrint("nwfs:  could not read rootdir during suballoc create\n");
       NWFSFree(root);
       return NwDiskIoError;
    }

    volume->SuballocCurrentCount = 0;
    volume->SuballocListCount = (volume->ClusterSize / SUBALLOC_BLOCK_SIZE);
    volume->SuballocCount = (volume->SuballocListCount + 27) / 28;

    for (i=0; i < volume->SuballocCount; i++)
    {
       volume->SuballocChainFlag[i] = 0;
       if (volume->SuballocChain[i])
	  NWFSSet(volume->SuballocChain[i], 0, sizeof(SUBALLOC));
    }

    for (i=0; i < volume->SuballocCount; i++)
    {
       if (volume->SuballocChainFlag[i])
       {
	  NWFSPrint("nwfs:  suballoc node sequence collision (#%d)\n", (int)i);
	  NWFSFree(root);
	  return NwHashError;
       }

       volume->SuballocDirNo[i] = 0;
       volume->SuballocChainFlag[i] = (ULONG) -1;

       if (!volume->SuballocChain[i])
       {
	  volume->SuballocChain[i] = NWFSAlloc(sizeof(SUBALLOC),
					       SUBALLOC_HEAD_TAG);
	  if (!volume->SuballocChain[i])
	  {
	     NWFSPrint("nwfs:  suballoc dir node alloc failure\n");
	     NWFSFree(root);
	     return NwHashError;
	  }
	  NWFSSet(volume->SuballocChain[i], 0, sizeof(SUBALLOC));
       }
       volume->SuballocCurrentCount++;
    }

    // create the directory records
    LastDirNo = (ULONG) -1;
    for (i=0; i < volume->SuballocCount; i++)
    {
       volume->SuballocDirNo[i] = AllocateDirectoryRecord(volume, 0);

       if ((volume->SuballocDirNo[i] == (ULONG) -1) ||
	   (volume->SuballocDirNo[i] == LastDirNo) ||
	   (!volume->SuballocDirNo[i]))
       {
	  retCode = FreeDirectoryRecords(volume, volume->SuballocDirNo,
					 0, i);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not release dir record in CreateSuballoc\n");
	     NWFSFree(root);
	     return retCode;
	  }
	  NWFSPrint("nwfs:  could not allocate directory record\n");
	  NWFSFree(root);
	  return NwInsufficientResources;
       }
       LastDirNo = volume->SuballocDirNo[i];
    }

    // initialize the suballoc root;
    volume->VolumeSuballocRoot = volume->SuballocDirNo[0];
    root->SubAllocationList = volume->VolumeSuballocRoot;

    for (i=0; i < (volume->SuballocCount - 1); i++)
    {
       // store the DirNo for next record into SuballocationList
       suballoc = (SUBALLOC *) volume->SuballocChain[i];
       suballoc->Flag = SUBALLOC_NODE;
       suballoc->ID = 0;
       suballoc->Reserved1 = 0;
       suballoc->SubAllocationList = volume->SuballocDirNo[i + 1];
       suballoc->SequenceNumber = (BYTE) i;
    }
    // null terminate last suballoc directory record
    suballoc = (SUBALLOC *) volume->SuballocChain[i];
    suballoc->Flag = SUBALLOC_NODE;
    suballoc->ID = 0;
    suballoc->Reserved1 = 0;
    suballoc->SubAllocationList = 0;
    suballoc->SequenceNumber = (BYTE) i;

    retCode = WriteDirectoryRecord(volume, (DOS *)root, 0);
    if (retCode)
    {
       NWFSPrint("nwfs:  could not write rootdir during suballoc create\n");
       NWFSFree(root);
       return NwDiskIoError;
    }

    NWFSFree(root);

    // Write the suballocation chain heads to the volume directory.
    for (i=0; i < volume->SuballocCount; i++)
    {
       retCode = WriteDirectoryRecord(volume,
				     (DOS *)volume->SuballocChain[i],
				      volume->SuballocDirNo[i]);
       if (retCode)
       {
	  NWFSPrint("nwfs:  could not write rootdir during suballoc create\n");
	  NWFSFree(root);
	  return NwDiskIoError;
       }
    }
    volume->SuballocChainComplete = (ULONG) -1;

    return 0;

}

ULONG ReadSuballocTable(VOLUME *volume)
{
    register ULONG i, SuballocLink, sequence;
    ULONG chainSize;
    register ULONG binSize, totalBlocks;
    register ULONG cbytes, count;
    register DOS *dos1, *dos2;
    register SUBALLOC *suballoc;
    register ROOT *root;
    register BYTE *Buffer;
    ULONG retCode = 0;

    Buffer = NWFSIOAlloc(IO_BLOCK_SIZE, ALIGN_BUF_TAG);
    if (!Buffer)
       return 0;

    retCode = nwvp_vpartition_block_read(volume->nwvp_handle,
					(volume->FirstDirectory *
					 volume->BlocksPerCluster),
					 1, Buffer);
    if (retCode)
    {
       NWFSFree(Buffer);
       return 0;
    }

    root = (ROOT *) Buffer;
    if ((root->Subdirectory != ROOT_NODE) ||
	(root->NameSpace != DOS_NAME_SPACE))
    {
       NWFSFree(Buffer);
       return 0;
    }

    if ((root->VolumeFlags & NDS_FLAG) ||
	(root->VolumeFlags & NEW_TRUSTEE_COUNT))
    {
       volume->VolumeSuballocRoot = root->SubAllocationList;
    }
    NWFSFree(Buffer);

    // If suballocation has not been initialized for this volume,
    // then return success without any further processing at this
    // point during volume mount.  Suballoc chains will be created
    // after the directory chain has been processed.

    if (!volume->VolumeSuballocRoot)
       return 0;

#if (MOUNT_VERBOSE)
    NWFSPrint("*** Verifying Suballocation Tables ***\n");
#endif

    dos1 = NWFSAlloc(sizeof(ROOT), DIR_WORKSPACE_TAG);
    if (!dos1)
       return NwInsufficientResources;

    dos2 = NWFSAlloc(sizeof(ROOT), DIR_WORKSPACE_TAG);
    if (!dos2)
    {
       NWFSFree(dos1);
       return NwInsufficientResources;
    }

    count = 0;
    volume->SuballocCurrentCount = 0;
    volume->SuballocListCount = (volume->ClusterSize / SUBALLOC_BLOCK_SIZE);
    volume->SuballocCount = (volume->SuballocListCount + 27) / 28;

    SuballocLink = volume->VolumeSuballocRoot;
    while (SuballocLink)
    {
#if (VERBOSE)
       NWFSPrint("SuballocLink -> %X\n", (unsigned int)SuballocLink);
#endif

       if (count++ > MAX_SUBALLOC_NODES)
       {
	  NWFSPrint("nwfs:  encountered too many suballoc dir records\n");
	  NWFSFree(dos1);
	  NWFSFree(dos2);
	  return NwHashError;
       }

       cbytes = NWReadFile(volume,
			   &volume->FirstDirectory,
			   0,
			   (ULONG)-1,   // set the filesize to infinity
			   (SuballocLink * sizeof(ROOT)),
			   (BYTE *)dos1,
			   sizeof(ROOT),
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   NO_SUBALLOC,
			   0);

       if (cbytes != sizeof(ROOT))
       {
	  NWFSFree(dos1);
	  NWFSFree(dos2);
	  return NwHashError;
       }

       cbytes = NWReadFile(volume,
			   &volume->SecondDirectory,
			   0,
			   (ULONG)-1,    // set the filesize to infinity
			   (SuballocLink * sizeof(ROOT)),
			   (BYTE *)dos2,
			   sizeof(ROOT),
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   NO_SUBALLOC,
			   0);

       if (cbytes != sizeof(ROOT))
       {
	  NWFSFree(dos1);
	  NWFSFree(dos2);
	  return NwHashError;
       }

       if (NWFSCompare(dos1, dos2, sizeof(ROOT)))
       {
	  NWFSFree(dos1);
	  NWFSFree(dos2);
	  NWFSPrint("nwfs:  dir data mismatch during suballoc table read\n");
	  return NwHashError;
       }

       suballoc = (SUBALLOC *) dos1;
       sequence = suballoc->SequenceNumber;
       if (volume->SuballocChainFlag[sequence])
       {
	  NWFSPrint("nwfs:  suballoc node sequence collision (#%u)\n",
		     (unsigned int)sequence);
	  NWFSFree(dos1);
	  NWFSFree(dos2);
	  return NwHashError;
       }

       if (sequence > 4)
       {
	  NWFSPrint("nwfs:  suballoc node sequence error\n");
	  NWFSFree(dos1);
	  NWFSFree(dos2);
	  return NwHashError;
       }

       volume->SuballocDirNo[sequence] = SuballocLink;
       volume->SuballocChainFlag[sequence] = (ULONG) -1;

#if (VERBOSE)
       NWFSPrint("count-%d list_count-%d curr_count-%d\n",
		(int)volume->SuballocCount,
		(int)volume->SuballocListCount,
		(int)volume->SuballocCurrentCount);
#endif

       if (!volume->SuballocChain[sequence])
       {
	  volume->SuballocChain[sequence] = NWFSAlloc(sizeof(SUBALLOC),
							 SUBALLOC_HEAD_TAG);
	  if (!volume->SuballocChain[sequence])
	  {
	     NWFSPrint("nwfs:  suballoc dir node alloc failure\n");
	     NWFSFree(dos1);
	     NWFSFree(dos2);
	     return NwHashError;
	  }
       }

       NWFSCopy(volume->SuballocChain[sequence], suballoc, sizeof(SUBALLOC));
       volume->SuballocCurrentCount++;
       if (volume->SuballocCurrentCount >= volume->SuballocCount)
       {
	  for (i=0; i < volume->SuballocListCount; i++)
	  {
             volume->SuballocSearchIndex[i] = 0;
	     
     	     // allocate a block free list for each suballoc fat chain.
	     if (volume->SuballocChain[i / 28]->StartingFATChain[i % 28])
	     {
		// determine the total length of the suballoc chain
		chainSize = GetChainSize(volume,
		  volume->SuballocChain[i / 28]->StartingFATChain[i % 28]);

		// map the current cluster allocation for this suballoc
		// chain.
		retCode = BuildChainAssignment(volume,
		     volume->SuballocChain[i / 28]->StartingFATChain[i % 28],
		     0);

		if (retCode)
		{
		   register FAT_ENTRY *fat;
                   FAT_ENTRY FAT_S; 

		   fat = GetFatEntry(volume,
		     volume->SuballocChain[i / 28]->StartingFATChain[i % 28],
		     &FAT_S);

		   NWFSPrint("%d:  %X ", (int)i,
		     (unsigned int) volume->SuballocChain[i / 28]->StartingFATChain[i % 28]);

		   while (fat && fat->FATCluster)
		   {
		      if (fat->FATCluster)
			 NWFSPrint("-> ");
		      NWFSPrint("%X ", (unsigned int)fat->FATCluster);
		      fat = GetFatEntry(volume, fat->FATCluster, &FAT_S);
		   }
		   NWFSPrint("\n");

		   NWFSPrint("nwfs:  suballoc fat chain (%d)-%d is corrupt\n",
			     (int)i, (int)(i * SUBALLOC_BLOCK_SIZE));
		   NWFSFree(dos1);
		   NWFSFree(dos2);
		   return NwHashError;
		}

		// calculate the total number of logical blocks in this
		// suballoc chain.  we do not count partial blocks
		// or round to the next logical block at this point
		// because we need an accurate count of whole blocks.

		binSize = (i * SUBALLOC_BLOCK_SIZE);
		volume->SuballocAssignedBlocks[i] = 0;
		volume->SuballocTotalBlocks[i] = totalBlocks = chainSize / binSize;

#if (VERBOSE)
		NWFSPrint("size-%d blocks-%d\n",
			  (int)chainSize, (int)totalBlocks);
#endif
		volume->FreeSuballoc[i] = NWFSAlloc(sizeof(BIT_BLOCK_HEAD),
						    BITBLOCK_TAG);
		if (!volume->FreeSuballoc[i])
		{
		   NWFSFree(dos1);
		   NWFSFree(dos2);
		   return NwInsufficientResources;
		}
		NWFSSet(volume->FreeSuballoc[i], 0, sizeof(BIT_BLOCK_HEAD));

		retCode = CreateBitBlockList(volume->FreeSuballoc[i],
					     totalBlocks,
					     SUBALLOC_BLOCK_SIZE,
					     "suballoc list");
		if (retCode)
		{
		   NWFSFree(volume->FreeSuballoc[i]);
		   volume->FreeSuballoc[i] = 0;

		   NWFSFree(dos1);
		   NWFSFree(dos2);
		   return NwInsufficientResources;
		}
	     }
	  }
	  volume->SuballocChainComplete = (ULONG) -1;
	  NWFSFree(dos1);
	  NWFSFree(dos2);
	  return 0;
       }
       SuballocLink = suballoc->SubAllocationList;
    }
    NWFSFree(dos1);
    NWFSFree(dos2);

    return 0;

}

ULONG AllocateSuballocRecord(VOLUME *volume, ULONG size, ULONG *retCode)
{
    register ULONG blockIndex, blockOffset, blockCount, i;
    register ULONG ccode, blockSize, SAEntry, blockNumber;

    if (retCode)
       *retCode = 0;

    if (!volume->VolumeSuballocRoot)
    {
       if (retCode)
	  *retCode = NwSuballocMissing;
       return 0;
    }

    if (size >= volume->ClusterSize)
    {
       if (retCode)
	  *retCode = NwInvalidParameter;
       return 0;
    }

    blockIndex = size / SUBALLOC_BLOCK_SIZE;
    blockOffset = size % SUBALLOC_BLOCK_SIZE;
    i = blockCount = blockIndex + (blockOffset ? 1 : 0);
    blockSize = blockCount * SUBALLOC_BLOCK_SIZE;

    if (blockCount >= volume->SuballocListCount)
    {
       NWFSPrint("nwfs:  suballoc blockCount too big (%d/%d)\n",
		(int)blockCount, (int)volume->SuballocListCount);
       if (retCode)
	  *retCode = NwInvalidParameter;
       return 0;
    }

    // lock this suballoc block bin
    NWLockSuballoc(volume, blockCount);

    // if this element has a free list
    if (volume->FreeSuballoc[i])
    {
       // if the last allocated index was at the end of the suballoc 
       // file, go ahead and extend the bit block list since we will
       // extend it anyway on the next bit block search operation.
       if ((volume->SuballocSearchIndex[i] + 1) >= 
	   volume->SuballocTotalBlocks[i])
       {
          // extend the suballocation chain
	  volume->SuballocTotalBlocks[i]++;

	  // if we exceed the max blocks, report we have exceeded
	  // the maximum number of suballoc blocks supported in
	  // a single chain.  The IA32 limit is 16,777,215
	  // (sixteen million blocks).

	  if (volume->SuballocTotalBlocks[i] >= 0xFFFFFF)
	  {
	     volume->SuballocTotalBlocks[i]--;
	     if (retCode)
		*retCode = NwSuballocExceeded;
	     NWUnlockSuballoc(volume, blockCount);
	     return 0;
	  }

	  // adjust the length of the free list
	  ccode = AdjustBitBlockList(volume->FreeSuballoc[i],
				     volume->SuballocTotalBlocks[i]);
	  if (ccode)
	  {
	        if (retCode)
		   *retCode = NwOtherError;
	        NWUnlockSuballoc(volume, blockCount);
	        return 0;
	  }

	  blockNumber = 
	         ScanAndSetBitBlockValueWithIndex(volume->FreeSuballoc[i], 
	                                  0, volume->SuballocSearchIndex[i],
					  1);
	  if (blockNumber == (ULONG) -1)
	  {
	     if (retCode)
		*retCode = NwInsufficientResources;
	     NWUnlockSuballoc(volume, blockCount);
	     return 0;
	  }
       }
       else
       {
          // see if there are any free blocks
          blockNumber = 
		  ScanAndSetBitBlockValueWithIndex(volume->FreeSuballoc[i], 
	                                    0, volume->SuballocSearchIndex[i],
					    1);
          if (blockNumber == (ULONG) -1)
          {
             // extend the suballocation chain
	     volume->SuballocTotalBlocks[i]++;

	     // if we exceed the max blocks, report we have exceeded
	     // the maximum number of suballoc blocks supported in
	     // a single chain.  The IA32 limit is 16,777,215
	     // (sixteen million blocks).

	     if (volume->SuballocTotalBlocks[i] >= 0xFFFFFF)
	     {
	        volume->SuballocTotalBlocks[i]--;
	        if (retCode)
		   *retCode = NwSuballocExceeded;
	        NWUnlockSuballoc(volume, blockCount);
	        return 0;
	     }

	     // adjust the length of the free list
	     ccode = AdjustBitBlockList(volume->FreeSuballoc[i],
				     volume->SuballocTotalBlocks[i]);
	     if (ccode)
	     {
	        if (retCode)
		   *retCode = NwOtherError;
	        NWUnlockSuballoc(volume, blockCount);
	        return 0;
	     }

	     blockNumber = 
	         ScanAndSetBitBlockValueWithIndex(volume->FreeSuballoc[i], 
	                                  0, volume->SuballocSearchIndex[i],
					  1);
	     if (blockNumber == (ULONG) -1)
	     {
	        if (retCode)
		   *retCode = NwInsufficientResources;
	        NWUnlockSuballoc(volume, blockCount);
	        return 0;
	     }
          }
       }
       volume->SuballocSearchIndex[i] = blockNumber;

       // construct the suballoc entry
       SAEntry = 0x80000000;
       SAEntry |= (blockCount << 24);
       SAEntry |= (blockNumber & 0x00FFFFFF);

#if (VERBOSE)
       NWFSPrint("SuballocGet -> %08X  TotalBlocks-%d  %s\n",
		(unsigned int)SAEntry, (int)volume->SuballocTotalBlocks[i],
		(blockNumber == volume->SuballocTotalBlocks[i])
		? "blockNumber = TotalBlocks" : "");
#endif
       NWUnlockSuballoc(volume, blockCount);
       return SAEntry;
    }
    else
    {
       volume->FreeSuballoc[i] = NWFSAlloc(sizeof(BIT_BLOCK_HEAD),
						    BITBLOCK_TAG);
       if (!volume->FreeSuballoc[i])
       {
	  if (retCode)
	     *retCode = NwInsufficientResources;
	  NWUnlockSuballoc(volume, blockCount);
	  return 0;
       }
       NWFSSet(volume->FreeSuballoc[i], 0, sizeof(BIT_BLOCK_HEAD));

       volume->SuballocTotalBlocks[i] = 0;
       volume->SuballocTotalBlocks[i]++;

       ccode = CreateBitBlockList(volume->FreeSuballoc[i],
				  volume->SuballocTotalBlocks[i],
				  SUBALLOC_BLOCK_SIZE,
				  "suballoc list");
       if (ccode)
       {
	  NWFSFree(volume->FreeSuballoc[i]);
	  volume->FreeSuballoc[i] = 0;
	  if (retCode)
	     *retCode = NwInsufficientResources;
	  NWUnlockSuballoc(volume, blockCount);
	  return 0;
       }

       blockNumber = ScanAndSetBitBlockValueWithIndex(volume->FreeSuballoc[i], 
                                    0, volume->SuballocSearchIndex[i],
				    1);
       if (blockNumber == (ULONG) -1)
       {
	  if (retCode)
	     *retCode = NwInsufficientResources;
	  NWUnlockSuballoc(volume, blockCount);
	  return 0;
       }
       volume->SuballocSearchIndex[i] = blockNumber;

       // construct the suballoc entry
       SAEntry = 0x80000000;
       SAEntry |= (blockCount << 24);
       SAEntry |= (blockNumber & 0x00FFFFFF);

#if (VERBOSE)
       NWFSPrint("SuballocGet -> %08X  TotalBlocks-%d  %s\n",
		(unsigned int)SAEntry, (int)volume->SuballocTotalBlocks[i],
		(blockNumber == volume->SuballocTotalBlocks[i])
		? "blockNumber = TotalBlocks" : "");
#endif
       NWUnlockSuballoc(volume, blockCount);
       return SAEntry;
    }

    if (retCode)
       *retCode = NwInvalidParameter;
    return 0;
}

ULONG GetSuballocListValue(VOLUME *volume, ULONG SAEntry)
{
    register ULONG blockCount, blockNumber, retCode;

    if (!volume->VolumeSuballocRoot)
       return NwSuballocMissing;

    if (!(SAEntry & 0x80000000) || (SAEntry == (ULONG) -1))
       return NwInvalidParameter;

    blockCount = (SAEntry & 0x7FFFFFFF) >> 24;
    if (blockCount >= volume->SuballocListCount)
       return NwInvalidParameter;

    // if there is no suballoc free list, return error;
    if (!volume->FreeSuballoc[blockCount])
       return NwInvalidParameter;

    blockNumber = (SAEntry & 0x00FFFFFF);

    retCode = GetBitBlockValue(volume->FreeSuballoc[blockCount], blockNumber);

    return retCode;
}

// this function is called only during volume mount, and does not
// need to be locked.

ULONG SetSuballocListValue(VOLUME *volume, ULONG SAEntry, ULONG Value)
{
    register ULONG blockCount, blockNumber, retCode;

    if (!volume->VolumeSuballocRoot)
       return NwSuballocMissing;

    if (!(SAEntry & 0x80000000) || (SAEntry == (ULONG) -1))
       return NwInvalidParameter;

    blockCount = (SAEntry & 0x7FFFFFFF) >> 24;
    if (blockCount >= volume->SuballocListCount)
       return NwInvalidParameter;

    // if there is no suballoc free list, return error;
    if (!volume->FreeSuballoc[blockCount])
       return NwInvalidParameter;

    blockNumber = (SAEntry & 0x00FFFFFF);

    // keep a running counter of the highest allocated
    // block number seen for this suballoc file chain.
    // add 1 to block number since we are converting a zero
    // based index into a count of elements.

    if ((blockNumber + 1) > volume->SuballocTotalBlocks[blockCount])
    {
       NWFSPrint("nwfs:  suballoc fat error [%08X] block-%d > total-%d clust-%d\n",
		(unsigned int)SAEntry, (int)(blockNumber + 1),
		(int)volume->SuballocTotalBlocks[blockCount],
		(int)((volume->SuballocTotalBlocks[blockCount] *
		     (blockCount * SUBALLOC_BLOCK_SIZE)) /
		     volume->ClusterSize));
    }

    if ((blockNumber + 1) > volume->SuballocAssignedBlocks[blockCount])
       volume->SuballocAssignedBlocks[blockCount] = (blockNumber + 1);

#if (DEBUG_SA)
    if (GetBitBlockValue(volume->FreeSuballoc[blockCount], blockNumber))
    {
       NWFSPrint("nwfs:  duplicate suballoc entry detected/bin-%d/%d limit-%d\n",
		(int)blockNumber, (int)blockCount,
		(int)GetBitBlockLimit(volume->FreeSuballoc[blockCount]));
       return NwInvalidParameter;
    }
#endif

    retCode = SetBitBlockValue(volume->FreeSuballoc[blockCount], blockNumber, Value);
    if (retCode)
    {
       NWFSPrint("nwfs:  invalid suballoc node detected block/bin-%d/%d limit-%d\n",
		(int)blockNumber, (int)blockCount,
		(int)GetBitBlockLimit(volume->FreeSuballoc[blockCount]));
       return NwInvalidParameter;
    }
    return 0;
}

ULONG FreeSuballocRecord(VOLUME *volume, ULONG SAEntry)
{
    register ULONG blockCount, blockNumber, retCode;

#if (VERBOSE)
    NWFSPrint("SuballocFree -> %08X\n", (unsigned int)SAEntry);
#endif

    if (!volume->VolumeSuballocRoot)
       return NwSuballocMissing;

    if (!(SAEntry & 0x80000000) || (SAEntry == (ULONG) -1))
       return NwInvalidParameter;

    blockCount = (SAEntry & 0x7FFFFFFF) >> 24;
    if (blockCount >= volume->SuballocListCount)
       return NwInvalidParameter;

    blockNumber = (SAEntry & 0x00FFFFFF);

    if (volume->FreeSuballoc[blockCount])
    {
#if (DEBUG_SA)
       if (!GetBitBlockValue(volume->FreeSuballoc[blockCount], blockNumber))
       {
	  NWFSPrint("nwfs:  suballoc entry already free bin-%d/%d limit-%d\n",
		   (int)blockNumber, (int)blockCount,
		   (int)GetBitBlockLimit(volume->FreeSuballoc[blockCount]));
	  return NwInvalidParameter;
       }
#endif
      
       retCode = SetBitBlockValue(volume->FreeSuballoc[blockCount], blockNumber, 0);
       if (retCode)
       {
	  NWFSPrint("nwfs:  invalid suballoc node detected block/bin-%d/%d limit-%d\n",
		   (int)blockNumber, (int)blockCount,
		   (int)GetBitBlockLimit(volume->FreeSuballoc[blockCount]));
	  return NwInvalidParameter;
       }

       if ((blockNumber) && 
          ((blockNumber - 1) <= volume->SuballocSearchIndex[blockCount]))
          volume->SuballocSearchIndex[blockCount] = (blockNumber - 1);

       return 0;
    }
    return NwInvalidParameter;
}

ULONG GetSuballocSize(VOLUME *volume, ULONG SAEntry)
{
    register ULONG blockCount;

    blockCount = (SAEntry & 0x7FFFFFFF) >> 24;
    if (blockCount >= volume->SuballocListCount)
    {
       NWFSPrint("block count error in GetSuballocSize\n");
       return 0;
    }
    return (blockCount * SUBALLOC_BLOCK_SIZE);
}

ULONG ReadSuballocRecord(VOLUME *volume, long offset,
			ULONG SAEntry, BYTE *buf, long count,
			ULONG as, ULONG *retCode)
{
    register ULONG blockCount, blockNumber;
    ULONG blockOffset, blockSize;
    register ULONG cbytes, i;
    ULONG FATChain;

#if (VERBOSE)
    NWFSPrint("ReadSuballoc -> %08X-%d\n", (unsigned int)SAEntry, (int)count);
#endif

    if (!(SAEntry & 0x80000000) || (SAEntry == (ULONG) -1))
       return 0;

    i = blockCount = (SAEntry & 0x7FFFFFFF) >> 24;
    if (blockCount >= volume->SuballocListCount)
       return 0;

    blockSize = blockCount * SUBALLOC_BLOCK_SIZE;
    if (offset > (long)blockSize)
       return 0;

    if ((offset + count) > (long)blockSize)
       count = blockSize - offset;

    if (count <= 0)
       return 0;

    blockNumber = (SAEntry & 0x00FFFFFF);
    blockOffset = blockNumber * blockSize;

    NWLockSuballoc(volume, blockCount);
    if (!volume->SuballocChain[i / 28])
    {
       NWUnlockSuballoc(volume, blockCount);
       return 0;
    }

    // Suballoc chains are zero relative when empty as compared to file
    // records, which use a -1 to indicate End of File.  We check and
    // see if the suballoc chain was null.  If so, then pass -1 as the fat
    // chain for this suballoc.

    FATChain = volume->SuballocChain[i / 28]->StartingFATChain[i % 28];
    if (!FATChain)
       FATChain = (ULONG) -1;

    cbytes = NWReadFile(volume,
			&FATChain,
			0,
			volume->SuballocTotalBlocks[blockCount] * blockSize,
			(blockOffset + offset),
			buf,
			count,
			&volume->SuballocTurboFATCluster[i],
			&volume->SuballocTurboFATIndex[i],
			retCode,
			as,
			0,
			NO_SUBALLOC,
			0);

    NWUnlockSuballoc(volume, blockCount);
    return cbytes;
}

ULONG WriteSuballocRecord(VOLUME *volume, long offset,
			ULONG SAEntry, BYTE *buf, long count,
			ULONG as, ULONG *retCode)
{
    register ULONG blockCount, blockNumber;
    ULONG blockOffset, blockSize;
    register ULONG cbytes, ccode, i;
    ULONG FATChain;

#if (VERBOSE)
    NWFSPrint("WriteSuballoc -> %08X-%d\n", (unsigned int)SAEntry, (int)count);
#endif

    if (!(SAEntry & 0x80000000) || (SAEntry == (ULONG) -1))
       return 0;

    i = blockCount = (SAEntry & 0x7FFFFFFF) >> 24;
    if (blockCount >= volume->SuballocListCount)
       return 0;

    blockSize = blockCount * SUBALLOC_BLOCK_SIZE;
    if (offset > (long)blockSize)
       return 0;

    if ((offset + count) > (long)blockSize)
       count = blockSize - offset;

    if (count <= 0)
       return 0;

    blockNumber = SAEntry & 0x00FFFFFF;
    blockOffset = blockNumber * blockSize;

    NWLockSuballoc(volume, blockCount);
    if (!volume->SuballocChain[i / 28])
    {
       NWUnlockSuballoc(volume, blockCount);
       return 0;
    }

    // Suballoc chains are zero relative when empty as compared to file
    // records, which use a -1 to indicate End of File.  We check and
    // see if the suballoc chain was null.  If so, then pass -1 as the fat
    // chain for this suballoc.

    FATChain = volume->SuballocChain[i / 28]->StartingFATChain[i % 28];
    if (!FATChain)
       FATChain = (ULONG) -1;

    cbytes = NWWriteFile(volume,
			 &FATChain,
			 0,
			 (blockOffset + offset),
			 buf,
			 count,
			 &volume->SuballocTurboFATCluster[i],
			 &volume->SuballocTurboFATIndex[i],
			 retCode,
			 as,
			 0,
			 NO_SUBALLOC);

    // sanity check the suballoc dir catalog and make certain it has
    // valid data.  ditto with the suballoc directory record we intend
    // to write.

    if ((!volume->SuballocDirNo[i / 28]) || (!volume->SuballocChain[i / 28]))
    {
       NWFSPrint("nwfs:  invalid suballoc directory number\n");
       if (retCode)
	  *retCode = NwVolumeCorrupt;
       NWUnlockSuballoc(volume, blockCount);
       return 0;
    }

    // if we updated any suballocation fat chains inside of write, then
    // update the suballocation directory records and write them to the
    // volume directory file.  if the chain head was updated to EOF by
    // the write operation, then write a zero into the suballoc dir
    // chain head

    volume->SuballocChain[i / 28]->StartingFATChain[i % 28] =
				  ((FATChain == (ULONG) -1) ? 0 : FATChain);

    ccode = WriteDirectoryRecord(volume,
				 (DOS *)volume->SuballocChain[i / 28],
				 volume->SuballocDirNo[i / 28]);
    if (ccode)
    {
       NWFSPrint("nwfs:  error updating suballoc directory entry\n");
       if (retCode)
	  *retCode = NwVolumeCorrupt;
       NWUnlockSuballoc(volume, blockCount);
       return 0;
    }

    NWUnlockSuballoc(volume, blockCount);
    return cbytes;
}



ULONG MapSuballocNode(VOLUME *volume, SUBALLOC_MAP *map, long SAEntry)
{
    register ULONG blockNumber, blockCount, blockOffset;
    register ULONG startElement, startOffset;
    register ULONG nextElement, nextOffset;
    register ULONG sacluster, i;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    register ULONG spc = volume->SectorsPerCluster;

    if (!map || (SAEntry == (ULONG) -1))
       return -1;

    // suballoc chains are organized into bins based on size.  we extract
    // the bin number from the suballoc cluster entry

    i = blockCount = (SAEntry & 0x7FFFFFFF) >> 24;
    if (blockCount >= volume->SuballocListCount)
    {
       NWFSPrint("block count error in MapSuballocNode\n");
       return -1;
    }
    blockNumber = SAEntry & 0x00FFFFFF;
    blockOffset = (blockNumber * blockCount);
    startElement = blockOffset / spc;
    startOffset = blockOffset % spc;
    nextElement = (blockOffset + blockCount) / spc;
    nextOffset = (blockOffset + blockCount) % spc;

    if ((startOffset + blockCount) > spc) // if greater, span is two elements
    {
       map->Count = 2;
       map->Size = 0;
       map->clusterIndex[0] = startElement;
       map->clusterOffset[0] = startOffset * SUBALLOC_BLOCK_SIZE;
       map->clusterSize[0] = (blockCount * SUBALLOC_BLOCK_SIZE) -
			     (nextOffset * SUBALLOC_BLOCK_SIZE);
       map->Size += map->clusterSize[0];

       map->clusterIndex[1] = nextElement;
       map->clusterOffset[1] = 0;
       map->clusterSize[1] = nextOffset * SUBALLOC_BLOCK_SIZE;
       map->Size += map->clusterSize[1];

       NWLockSuballoc(volume, blockCount);
       if (!volume->SuballocChain[i / 28])
       {
	  NWFSPrint("nwfs:  suballoc head error in MapSuballocNode\n");
	  NWUnlockSuballoc(volume, blockCount);
	  return -1;
       }

       // check is there is a Turbo FAT entry for this suballoc chain
       if ((startElement >= volume->SuballocTurboFATIndex[i]) &&
	   (volume->SuballocTurboFATCluster[i]))
          sacluster = volume->SuballocTurboFATCluster[i];
       else
          sacluster = volume->SuballocChain[i / 28]->StartingFATChain[i % 28];

       FAT = GetFatEntry(volume, sacluster, &FAT_S);
       while (FAT && FAT->FATCluster)
       {
	  if (FAT->FATIndex == (long)startElement)
	     break;

	  if (FAT->FATCluster == (ULONG) -1)
	  {
	     NWFSPrint("nwfs:  fat chain error in suballoc map\n");
	     NWUnlockSuballoc(volume, blockCount);
	     return -1;
	  }
	  if (FAT->FATCluster & 0x80000000)
	  {
	     NWFSPrint("nwfs:  suballoc fat chain error in suballoc map\n");
	     NWUnlockSuballoc(volume, blockCount);
	     return -1;
	  }
	  sacluster = FAT->FATCluster;
	  FAT = GetFatEntry(volume, sacluster, &FAT_S);
       }

       if (!FAT)
       {
	  NWFSPrint("nwfs:  fat table error in suballoc map\n");
	  NWUnlockSuballoc(volume, blockCount);
	  return -1;
       }

       if (FAT->FATCluster == (ULONG) -1)
       {
	  NWFSPrint("nwfs:  fat entry error in suballoc map\n");
	  NWUnlockSuballoc(volume, blockCount);
	  return -1;
       }

       if (FAT->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  fat entry suballoc error in suballoc map\n");
	  NWUnlockSuballoc(volume, blockCount);
	  return -1;
       }
       map->clusterNumber[0] = sacluster;
       map->clusterNumber[1] = FAT->FATCluster;

       NWUnlockSuballoc(volume, blockCount);

#if DEBUG_SA
       if (TrgDebugSA)
       {
	  NWFSPrint("map[1,2](%d/%d)  index-%d offset-%d/%d size-%d clstr-%X [0x%08X]\n",
	      (int)blockCount,
	      (int)(blockCount * SUBALLOC_BLOCK_SIZE),
	      (int)map->clusterIndex[0],
	      (int)map->clusterOffset[0],
	      (int)startOffset,
	      (int)map->clusterSize[0],
	      (int)map->clusterNumber[0],
	      (unsigned int)SAEntry);

	  NWFSPrint("map[2,2](%d/%d)  index-%d offset-%d/0 size-%d clstr-%X\n",
	      (int)blockCount,
	      (int)(blockCount * SUBALLOC_BLOCK_SIZE),
	      (int)map->clusterIndex[1],
	      (int)map->clusterOffset[1],
	      (int)map->clusterSize[1],
	      (int)map->clusterNumber[1]);
       }
#endif

       return 0;
    }
    else
    {
       map->Count = 1;
       map->clusterIndex[0] = startElement;
       map->clusterOffset[0] = startOffset * SUBALLOC_BLOCK_SIZE;
       map->clusterSize[0] = blockCount * SUBALLOC_BLOCK_SIZE;
       map->Size = map->clusterSize[0];

       NWLockSuballoc(volume, blockCount);

       if (!volume->SuballocChain[i / 28])
       {
	  NWFSPrint("nwfs:  suballoc head error in MapSuballocNode\n");
	  NWUnlockSuballoc(volume, blockCount);
	  return -1;
       }

       // check is there is a Turbo FAT entry for this suballoc chain
       if ((startElement >= volume->SuballocTurboFATIndex[i]) &&
	   (volume->SuballocTurboFATCluster[i]))
          sacluster = volume->SuballocTurboFATCluster[i];
       else
          sacluster = volume->SuballocChain[i / 28]->StartingFATChain[i % 28];
       
       FAT = GetFatEntry(volume, sacluster, &FAT_S);
       while (FAT && FAT->FATCluster)
       {
	  if (FAT->FATIndex == (long)startElement)
	     break;

	  if (FAT->FATCluster == (ULONG) -1)
	  {
	     NWFSPrint("nwfs:  fat chain error in suballoc map\n");
	     NWUnlockSuballoc(volume, blockCount);
	     return -1;
	  }

	  if (FAT->FATCluster & 0x80000000)
	  {
	     NWFSPrint("nwfs:  suballoc fat chain error in suballoc map\n");
	     NWUnlockSuballoc(volume, blockCount);
	     return -1;
	  }
	  sacluster = FAT->FATCluster;
	  FAT = GetFatEntry(volume, sacluster, &FAT_S);
       }
       if (!FAT)
       {
	  NWFSPrint("nwfs:  fat table error in suballoc map\n");
	  NWUnlockSuballoc(volume, blockCount);
	  return -1;
       }
       map->clusterNumber[0] = sacluster;

       NWUnlockSuballoc(volume, blockCount);

#if DEBUG_SA
       if (TrgDebugSA)
       {
	  NWFSPrint("map[1,1](%d/%d)  index-%d offset-%d/%d size-%d clstr-%X [0x%08X]\n",
	      (int)blockCount,
	      (int)(blockCount * SUBALLOC_BLOCK_SIZE),
	      (int)map->clusterIndex[0],
	      (int)map->clusterOffset[0],
	      (int)startOffset,
	      (int)map->clusterSize[0],
	      (int)map->clusterNumber[0],
	      (unsigned int)SAEntry);
       }
#endif

       return 0;
    }
    return -1;

}


