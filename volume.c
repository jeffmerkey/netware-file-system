
/***************************************************************************
*
*   Copyright (c) 1998, 1999 Jeff V. Merkey
*   895 West Center Street
*   Orem, Utah  84057
*   jmerkey@utah-nac.org
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by the
*   Free Software Foundation, version 2, or any later version.
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
*   jmerkey@utah-nac.org is the official maintainer of
*   this code.  You are encouraged to report any bugs, problems, fixes,
*   suggestions, and comments about this software to jmerkey@utah-nac.org
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
*      Darren Major
*      
*
****************************************************************************
*
*
*   AUTHOR   :  Jeff V. Merkey (jmerkey@utah-nac.org)
*   FILE     :  VOLUME.C
*   DESCRIP  :  FENRIS Netware Volume Manager
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#include "globals.h"

#if (LINUX_SLEEP)
NWFSInitMutex(ScanSemaphore);
#endif

void NWLockScan(void)
{
#if (LINUX_SLEEP)
    if (WaitOnSemaphore(&ScanSemaphore) == -EINTR)
       NWFSPrint("lock scan was interrupted\n");
#endif
}

void NWUnlockScan(void)
{
#if (LINUX_SLEEP)
    SignalSemaphore(&ScanSemaphore);
#endif
}

void NWLockWK(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&volume->WK_spinlock, volume->WK_flags);
#else
    if (WaitOnSemaphore(&volume->WKSemaphore) == -EINTR)
       NWFSPrint("lock workspace was interupted\n");
#endif
#endif
}

void NWUnlockWK(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&volume->WK_spinlock, volume->WK_flags);
#else
    SignalSemaphore(&volume->WKSemaphore);
#endif
#endif
}

void NWLockNSWK(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&volume->NS_spinlock, volume->NS_flags);
#else
    if (WaitOnSemaphore(&volume->NSSemaphore) == -EINTR)
       NWFSPrint("lock ns workspace was interupted\n");
#endif
#endif
}

void NWUnlockNSWK(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&volume->NS_spinlock, volume->NS_flags);
#else
    SignalSemaphore(&volume->NSSemaphore);
#endif
#endif
}

//
//  suballocation workspace memory
//

void FreeWorkspace(VOLUME *volume, VOLUME_WORKSPACE *wk)
{
    NWLockWK(volume);

#if (CONSERVE_MEMORY)
    if (volume->WKCount > MAX_WORKSPACE_BUFFERS)
    {
       NWUnlockWK(volume);
       NWFSFree(wk);
       return;
    }
#endif

    if (!volume->WKHead)
    {
       volume->WKHead = volume->WKTail = wk;
       wk->next = wk->prior = 0;
    }
    else
    {
       volume->WKTail->next = wk;
       wk->next = 0;
       wk->prior = volume->WKTail;
       volume->WKTail = wk;
    }
    volume->WKCount++;

    NWUnlockWK(volume);
    return;
}

VOLUME_WORKSPACE *AllocateWorkspace(VOLUME *volume)
{
    register VOLUME_WORKSPACE *wk;

    NWLockWK(volume);
    if (volume->WKHead)
    {
       wk = volume->WKHead;
       volume->WKHead = wk->next;
       if (volume->WKHead)
	  volume->WKHead->prior = NULL;
       else
	  volume->WKTail = NULL;

       if (volume->WKCount)
	  volume->WKCount--;

       NWUnlockWK(volume);
       return wk;
    }
    else
    {
       NWUnlockWK(volume);

       wk = NWFSCacheAlloc((volume->ClusterSize +
		      (sizeof(VOLUME_WORKSPACE) * 2)),
		      VOLUME_WS_TAG);
       if (!wk)
	  return 0;

       NWFSSet(wk, 0, (volume->ClusterSize +
		      (sizeof(VOLUME_WORKSPACE) * 2)));
       return wk;
    }
    NWUnlockWK(volume);
    return 0;
}

void FreeWorkspaceMemory(VOLUME *volume)
{
    register VOLUME_WORKSPACE *wk;

    NWLockWK(volume);
    while (volume->WKHead)
    {
       wk = volume->WKHead;
       volume->WKHead = wk->next;

       if (volume->WKCount)
	  volume->WKCount--;
       
       NWFSFree(wk);
    }
    volume->WKHead = volume->WKTail = 0;
    NWUnlockWK(volume);
    return;
}

//
//  namespace workspace memory
//

void FreeNSWorkspace(VOLUME *volume, VOLUME_WORKSPACE *wk)
{
    NWLockNSWK(volume);

#if (CONSERVE_MEMORY)
    if (volume->NSCount > MAX_WORKSPACE_BUFFERS)
    {
       NWUnlockNSWK(volume);
       NWFSFree(wk);
       return;
    }
#endif

    if (!volume->NSHead)
    {
       volume->NSHead = volume->NSTail = wk;
       wk->next = wk->prior = 0;
    }
    else
    {
       volume->NSTail->next = wk;
       wk->next = 0;
       wk->prior = volume->NSTail;
       volume->NSTail = wk;
    }
    volume->NSCount++;

    NWUnlockNSWK(volume);
    return;
}

VOLUME_WORKSPACE *AllocateNSWorkspace(VOLUME *volume)
{
    register VOLUME_WORKSPACE *wk;

    NWLockNSWK(volume);
    if (volume->NSHead)
    {
       wk = volume->NSHead;
       volume->NSHead = wk->next;
       if (volume->NSHead)
	  volume->NSHead->prior = NULL;
       else
	  volume->NSTail = NULL;

       if (volume->NSCount)
	  volume->NSCount--;

       NWUnlockNSWK(volume);
       return wk;
    }
    else
    {
       NWUnlockNSWK(volume);

       wk = NWFSCacheAlloc(((sizeof(ROOT) * (volume->NameSpaceCount + 1)) +
		       (sizeof(VOLUME_WORKSPACE) * 2)),
			VOLUME_WS_TAG);
       if (!wk)
	  return 0;

       NWFSSet(wk, 0, ((sizeof(ROOT) * (volume->NameSpaceCount + 1)) +
	      (sizeof(VOLUME_WORKSPACE) * 2)));
       return wk;
    }
    NWUnlockNSWK(volume);
    return 0;
}

void FreeNSWorkspaceMemory(VOLUME *volume)
{
    register VOLUME_WORKSPACE *wk;

    NWLockNSWK(volume);
    while (volume->NSHead)
    {
       wk = volume->NSHead;
       volume->NSHead = wk->next;

       if (volume->NSCount)
	  volume->NSCount--;

       NWFSFree(wk);
    }
    volume->NSHead = volume->NSTail = 0;
    NWUnlockNSWK(volume);
    return;
}


//
//  NOTE:  This code is here for debugging purposes.
//

BYTE *dumpRecordBytes(BYTE *p, ULONG size)
{
   register ULONG i, r, total, count;
   register BYTE *op = p;

   count = size / 16;

   NWFSPrint("           0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
   for (r=0; r < count; r++)
   {
      NWFSPrint("%08X ", (unsigned int)(p - op));
      for (total = 0, i=0; i < 16; i++, total++)
      {
	 NWFSPrint(" %02X", (BYTE) p[i]);
      }
      NWFSPrint("  ");
      for (i=0; i < total; i++)
      {
	 if ((p[i] < 32) || (p[i] > 126))
	    NWFSPrint(".");
	 else
	    NWFSPrint("%c", p[i]);
      }
      NWFSPrint("\n");

      p = (void *)((ULONG) p + (ULONG) total);
   }
   return p;

}

//
//  NOTE:  This code is here for debugging purposes.
//

BYTE *dumpRecord(BYTE *p, ULONG size)
{
   register int i, r, count;
   register BYTE *op = p;
   ULONG *lp;

   count = size / 16;

   lp = (ULONG *) p;

   for (r=0; r < count; r++)
   {
      NWFSPrint("%08X ", (unsigned int)(p - op));
      for (i=0; i < (16 / 4); i++)
      {
	 NWFSPrint(" %08X", (unsigned int) lp[i]);
      }
      NWFSPrint("  ");
      for (i=0; i < 16; i++)
      {
	 if ((p[i] < 32) || (p[i] > 126))
	    NWFSPrint(".");
	 else
	    NWFSPrint("%c", p[i]);
      }
      NWFSPrint("\n");

      p = (void *)((ULONG) p + (ULONG) 16);
      lp = (ULONG *) p;
   }
   return p;

}

// This is the standard mount routine for mounting a NetWare volume.

static ULONG MountFileVolume(VOLUME *volume)
{
    register ULONG retCode, ccode, r, NameSpace, i;
    extern ULONG ReadFATTable(VOLUME *volume);
    extern ULONG ReadDirectoryTable(VOLUME *volume);
    extern ULONG ReadSuballocTable(VOLUME *volume);
    extern ULONG ReadExtendedDirectory(VOLUME *volume);
    extern void FreeVolumeNamespaces(VOLUME *volume);
    ULONG month, day, year, hour, minute, second;
    BYTE FoundNameSpace[MAX_NAMESPACES];

    if (!volume->VolumePresent)
    {
       NWFSPrint("nwfs:  volume %s is missing one or more segments\n", volume->VolumeName);
       return -5;
    }

    if (VolumeMountedTable[volume->VolumeNumber])
    {
#if (ALLOW_REMOUNT)
       volume->InUseCount++;
       return 0;
#else
       NWFSPrint("nwfs:  volume %s disk(%d) already mounted\n",
	         volume->VolumeName, (int)volume->VolumeDisk);
       return -1;
#endif
    }

#if (ALLOW_REMOUNT)
    volume->InUseCount++;
#endif

    month = day = year = hour = minute = second = 0;
    GetNWTime(NWFSSystemToNetwareTime(NWFSGetSystemTime()),
	      &second, &minute, &hour, &day, &month, &year);
    NWFSPrint("Mounting Volume %s  %02d/%02d/%02d %02d:%02d:%02d\n",
	       volume->VolumeName,
	       (int)month, (int)day, (int)year,
	       (int)hour, (int)minute, (int)second);

    if (nwvp_vpartition_open(volume->nwvp_handle) != 0)
    {
       NWFSPrint("nwfs:  volume %s could not attach to NVMP\n",
		 volume->VolumeName);
#if (ALLOW_REMOUNT)
       if (volume->InUseCount)
          volume->InUseCount--;
#endif
       return -1;
    }

#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_init(&volume->WK_spinlock);
    spin_lock_init(&volume->NS_spinlock);
    spin_lock_init(&volume->NameHash_spinlock);
    spin_lock_init(&volume->DirHash_spinlock);
    spin_lock_init(&volume->ParentHash_spinlock);
    spin_lock_init(&volume->TrusteeHash_spinlock);
    spin_lock_init(&volume->QuotaHash_spinlock);
    spin_lock_init(&volume->ExtHash_spinlock);
    spin_lock_init(&volume->NameSpace_spinlock);

    volume->WK_flags = 0;
    volume->NS_flags = 0;
    volume->NameHash_flags = 0;
    volume->DirHash_flags = 0;
    volume->ParentHash_flags = 0;
    volume->TrusteeHash_flags = 0;
    volume->QuotaHash_flags = 0;
    volume->ExtHash_flags = 0;
    volume->NameSpace_flags = 0;

#else
    AllocateSemaphore(&volume->WKSemaphore, 1);
    AllocateSemaphore(&volume->NSSemaphore, 1);
    AllocateSemaphore(&volume->NameHashSemaphore, 1);
    AllocateSemaphore(&volume->DirHashSemaphore, 1);
    AllocateSemaphore(&volume->ParentHashSemaphore, 1);
    AllocateSemaphore(&volume->TrusteeHashSemaphore, 1);
    AllocateSemaphore(&volume->QuotaHashSemaphore, 1);
    AllocateSemaphore(&volume->ExtHashSemaphore, 1);
    AllocateSemaphore(&volume->NameSpaceSemaphore, 1);
#endif
    
    for (i=0; i < 128; i++)
       AllocateSemaphore(&volume->SuballocSemaphore[i], 1);
    AllocateSemaphore(&volume->FatSemaphore, 1);
    AllocateSemaphore(&volume->VolumeSemaphore, 1);
    AllocateSemaphore(&volume->DirBlockSemaphore, 1);
    AllocateSemaphore(&volume->DirAssignSemaphore, 1);

#endif

    // Initialize orphan and deleted list heads
    volume->HashMountHead = volume->HashMountTail = 0;
    volume->DelHashMountHead = volume->DelHashMountTail = 0;

    // initialize suballocation workspaces
    volume->WKHead = volume->WKTail = 0;
    volume->WKCount = 0;

    // initialize namespace workspaces
    volume->NSHead = volume->NSTail = 0;
    volume->NSCount = 0;

    // initialize suballocation tables
    volume->SuballocCount = 0;
    volume->SuballocChainComplete = 0;
    for (i=0; i < 5; i++)
       volume->SuballocChainFlag[i] = 0;

    volume->DirectoryCount = 0;
    volume->FreeDirectoryCount = 0;
    volume->FreeDirectoryBlockCount = 0;
    volume->VolumeAllocatedClusters = 0;
    volume->VolumeFreeClusters = 0;
    volume->DeletedDirNo = 0;

    volume->MinimumFATBlocks = MIN_FAT_LRU_BLOCKS;
    volume->MaximumFATBlocks = MAX_FAT_LRU_BLOCKS;

    volume->MountedNumberOfSegments = volume->NumberOfSegments;
    volume->MountedVolumeClusters = volume->VolumeClusters;

    if (InitializeFAT_LRU(volume))
    {
       retCode = -2;
       goto FATInitFailed;
    }

    if (InitializeDirBlockHash(volume))
    {
       retCode = -2;
       goto DirBlockInitFailed;
    }

    if (InitializeDirAssignHash(volume))
    {
       retCode = -2;
       goto DirAssignInitFailed;
    }

    // allocate hash memory for namespaces and directory blocks

    volume->ParentHash = NWFSCacheAlloc(PARENT_HASH_SIZE, PAR_HASH_TAG);
    if (!volume->ParentHash)
    {
	retCode = -2;
	goto HashInitFailed;
    }
    volume->ParentHashLimit = NUMBER_OF_PARENT_ENTRIES;
    NWFSSet(volume->ParentHash, 0, PARENT_HASH_SIZE);

    volume->ExtDirHash = NWFSCacheAlloc(EXT_HASH_SIZE, EXD_HASH_TAG);
    if (!volume->ExtDirHash)
    {
	retCode = -2;
	goto HashInitFailed;
    }
    volume->ExtDirHashLimit = NUMBER_OF_EXT_HASH_ENTRIES;
    NWFSSet(volume->ExtDirHash, 0, EXT_HASH_SIZE);

    volume->TrusteeHash = NWFSCacheAlloc(TRUSTEE_HASH_SIZE, TRS_HASH_TAG);
    if (!volume->TrusteeHash)
    {
	retCode = -2;
	goto HashInitFailed;
    }
    volume->TrusteeHashLimit = NUMBER_OF_TRUSTEE_ENTRIES;
    NWFSSet(volume->TrusteeHash, 0, TRUSTEE_HASH_SIZE);

    volume->UserQuotaHash = NWFSCacheAlloc(USER_QUOTA_HASH_SIZE, QUO_HASH_TAG);
    if (!volume->UserQuotaHash)
    {
	retCode = -2;
	goto HashInitFailed;
    }
    volume->UserQuotaHashLimit = NUMBER_OF_QUOTA_ENTRIES;
    NWFSSet(volume->UserQuotaHash, 0, USER_QUOTA_HASH_SIZE);

    volume->DirectoryNumberHash = NWFSCacheAlloc(DIR_NUMBER_HASH_SIZE, DNM_HASH_TAG);
    if (!volume->DirectoryNumberHash)
    {
	retCode = -2;
	goto HashInitFailed;
    }
    volume->DirectoryNumberHashLimit = NUMBER_OF_DIR_NUMBER_ENTRIES;
    NWFSSet(volume->DirectoryNumberHash, 0, DIR_NUMBER_HASH_SIZE);

    // set default name space to MSDOS
    volume->NameSpaceDefault = DOS_NAME_SPACE;

    // check for duplicate namespaces
    NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
    for (r=0; r < volume->NameSpaceCount; r++)
    {
       if (!r && volume->NameSpaceID[r])
       {
	  NWFSPrint("nwfs:  dos name space not present\n");
	  retCode = -2;
	  goto HashInitFailed;
       }

       if (r && (FoundNameSpace[volume->NameSpaceID[r] & 0xF]))
       {
	  NWFSPrint("nwfs:  duplicate volume namespaces detected\n");
	  retCode = -2;
	  goto HashInitFailed;
       }
       FoundNameSpace[volume->NameSpaceID[r] & 0xF] = TRUE;
    }

    // look for unix namespace, if found use as default.
    for (r=0; r < volume->NameSpaceCount; r++)
    {
       NameSpace = volume->NameSpaceID[r];
       if (NameSpace == UNIX_NAME_SPACE)
       {
	  volume->NameSpaceDefault = UNIX_NAME_SPACE;
	  break;
       }
    }

    // if no unix namespace, default to LONG namespace
    if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
    {
       for (r=0; r < volume->NameSpaceCount; r++)
       {
	  NameSpace = volume->NameSpaceID[r];
	  if (NameSpace == LONG_NAME_SPACE)
	  {
	     volume->NameSpaceDefault = LONG_NAME_SPACE;
	     break;
	  }
       }
    }

#if (MOUNT_VERBOSE)
    switch (volume->NameSpaceDefault)
    {
       case DOS_NAME_SPACE:
	  NWFSPrint("*** DOS Name Space ***\n");
	  break;

       case MAC_NAME_SPACE:
	  NWFSPrint("*** MAC Name Space ***\n");
	  break;

       case UNIX_NAME_SPACE:
	  NWFSPrint("*** UNIX Name Space ***\n");
	  break;

       case LONG_NAME_SPACE:
	  NWFSPrint("*** LONG Name Space ***\n");
	  break;

       case NT_NAME_SPACE:
	  NWFSPrint("*** NT Name_Space ***\n");
	  break;
    }
#endif

    for (r=0; r < volume->NameSpaceCount; r++)
    {
       NameSpace = volume->NameSpaceID[r];
       if (volume->VolumeNameHash[NameSpace & 0xF])
       {
	  NWFSPrint("nwfs:  name space entries detected that are identical\n");
	  retCode = -2;
	  goto HashInitFailed;
       }

       volume->VolumeNameHash[NameSpace & 0xF] = NWFSCacheAlloc(VOLUME_NAME_HASH_SIZE,
							   NAM_HASH_TAG);
       if (!volume->VolumeNameHash[NameSpace & 0xF])
       {
	  retCode = -2;
	  goto HashInitFailed;
       }
       volume->VolumeNameHashLimit[NameSpace & 0xF] = NUMBER_OF_NAME_HASH_ENTRIES;
       NWFSSet(volume->VolumeNameHash[NameSpace & 0xF], 0, VOLUME_NAME_HASH_SIZE);
    }

    if (InitializeClusterFreeList(volume))
    {
       retCode = -2;
       goto FreeBlockInitFailed;
    }

    if (InitializeClusterAssignedList(volume))
    {
       retCode = -2;
       goto AssignedBlockInitFailed;
    }

    retCode = ReadFATTable(volume);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  error reading FAT tables\n");
       goto ReadFATInitFailed;
    }

    retCode = ReadExtendedDirectory(volume);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  error reading extended directory\n");
       goto ReadExtendedDirectoryInitFailed;
    }

    retCode = ReadSuballocTable(volume);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  error reading suballoc tables\n");
       goto ReadSuballocInitFailed;
    }

    retCode = ReadDirectoryTable(volume);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  error reading directory tables\n");
       goto ReadDirectoryInitFailed;
    }

    // Process any hash records that were placed in the mount
    // list because their DirNo was less than their assigned
    // parent.

    retCode = ProcessOrphans(volume);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  error processing orphan hash records\n");
       goto LinkNameSpacesFailed;
    }

    // now link the namespace records together
    retCode = LinkVolumeNameSpaces(volume);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  error linking volume namespaces\n");
       goto LinkNameSpacesFailed;
    }

    // this procedure performs two functions.  It builds the suballocation
    // free list maps and checks the fat table allocation map and frees
    // any unassigned clusters for which there is no valid fat chain entry
    // for the volume.

    retCode = AdjustAllocatedClusters(volume);
    if (retCode)
       NWFSPrint("%d clusters have been freed\n", (int)retCode);

    // if the DELETED.SAV and _NETWARE directories do not exist,
    // then create them.  We also check for suballocation chain
    // heads, if they do not exist and suballocation is enabled,
    // we create these as well.

    retCode = InitializeVolumeDirectory(volume);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  could not initialize volume directory\n");
       goto LinkNameSpacesFailed;
    }

    if (volume->VolumeFlags & SUB_ALLOCATION_ON)
    {
       if (!volume->SuballocChainComplete)
       {
	  retCode = -2;
	  NWFSPrint("nwfs:  suballoc tables are malformed\n");
	  goto LinkNameSpacesFailed;
       }
    }

    volume->MountTime = NWFSGetSystemTime();
    volume->FileSeed = 0;
    volume->AllocSegment = 0;
    VolumeMountedTable[volume->VolumeNumber] = TRUE;

    month = day = year = hour = minute = second = 0;
    GetNWTime(NWFSSystemToNetwareTime(NWFSGetSystemTime()),
	      &second, &minute, &hour, &day, &month, &year);
    NWFSPrint("Volume %s Mounted  %02d/%02d/%02d %02d:%02d:%02d\n",
	       volume->VolumeName,
	       (int)month, (int)day, (int)year,
	       (int)hour, (int)minute, (int)second);
#if (VERBOSE)
    NWFSPrint("dirs-%d  empty_dirs-%d  free_dir_blocks-%d\n",
	      (int)volume->DirectoryCount,
	      (int)volume->FreeDirectoryCount,
	      (int)volume->FreeDirectoryBlockCount);
#endif

    return 0;

// error exit code paths

LinkNameSpacesFailed:;
ReadDirectoryInitFailed:;
ReadSuballocInitFailed:;
    // free any suballoc directory records
    for (i=0; i < MAX_SUBALLOC_NODES; i++)
    {
       if (volume->SuballocChain[i])
	  NWFSFree(volume->SuballocChain[i]);
       volume->SuballocChain[i] = 0;
    }

    // release suballoc free lists
    for (i=0; i < volume->SuballocListCount; i++)
    {
       if (volume->FreeSuballoc[i])
       {
	  ccode = FreeBitBlockList(volume->FreeSuballoc[i]);
	  if (ccode)
	     NWFSPrint("nwfs:  could not free suballoc bit list (%d)\n", (int)i);
	  NWFSFree(volume->FreeSuballoc[i]);
       }
       volume->FreeSuballoc[i] = 0;
    }

ReadExtendedDirectoryInitFailed:;
ReadFATInitFailed:;
   FreeClusterAssignedList(volume);

AssignedBlockInitFailed:;
   FreeClusterFreeList(volume);

FreeBlockInitFailed:;
HashInitFailed:;
    FreeVolumeNamespaces(volume);
    FreeDirAssignHash(volume);

DirAssignInitFailed:;
    FreeDirBlockHash(volume);

DirBlockInitFailed:;
    FreeFAT_LRU(volume);
    FlushVolumeLRU(volume);

FATInitFailed:;
    FreeWorkspaceMemory(volume);
    FreeNSWorkspaceMemory(volume);
    nwvp_vpartition_close(volume->nwvp_handle);

#if (ALLOW_REMOUNT)
    if (volume->InUseCount)
       volume->InUseCount--;
#endif

    return retCode;

}

ULONG MountVolumeByHandle(VOLUME *volume)
{
    register ULONG ccode;

    NWLockScan();
    ccode = MountFileVolume(volume);
    NWUnlockScan();

    return ccode;
}

ULONG MountVolume(BYTE *VolumeName)
{
    register ULONG retCode, i, len;
    register BYTE *p;

    len = 0;
    p = VolumeName;
    while (*p++)
       len++;

    NWLockScan();
    for (i=0; i < MaximumNumberOfVolumes; i++)
    {
       if (VolumeTable[i])
       {
          if (len == VolumeTable[i]->VolumeNameLength)
          {
	     if (!NWFSCompare(VolumeName, VolumeTable[i]->VolumeName,
			   VolumeTable[i]->VolumeNameLength))
	     {
		retCode = MountFileVolume(VolumeTable[i]);
		NWUnlockScan();
		return retCode;
	     }
	  }
       }
    }
    NWUnlockScan();
    return -1;

}

VOLUME *MountHandledVolume(BYTE *VolumeName)
{
    register ULONG i, retCode, len;
    register BYTE *p;

    len = 0;
    p = VolumeName;
    while (*p++)
       len++;

    NWLockScan();
    for (i=0; i < MaximumNumberOfVolumes; i++)
    {
       if (VolumeTable[i])
       {
          if (len == VolumeTable[i]->VolumeNameLength)
          {
	     if (!NWFSCompare(VolumeName, VolumeTable[i]->VolumeName,
			   VolumeTable[i]->VolumeNameLength))
	     {
	        retCode = MountFileVolume(VolumeTable[i]);
	        if (!retCode)
	        {
                   NWUnlockScan();
		   return VolumeTable[i];
	        }
		NWUnlockScan();
		return 0;
	     }
	  }
       }
    }
    NWUnlockScan();
    return 0;

}

ULONG MountAllVolumes(void)
{
    register ULONG retCode, finalCode = 0;
    register ULONG i;

    NWLockScan();
    for (i=0; i < MaximumNumberOfVolumes; i++)
    {
       if (VolumeTable[i])
       {
	  retCode = MountFileVolume(VolumeTable[i]);
	  if (retCode)
	  {
	     finalCode = retCode;
	     NWFSPrint("nwfs:  error mounting %s\n", VolumeTable[i]->VolumeName);
	  }
       }
    }
    NWUnlockScan();
    return finalCode;

}

static ULONG DismountFileVolume(VOLUME *volume, ULONG force)
{
    register ULONG i, retCode;
    extern void FreeVolumeNamespaces(VOLUME *volume);
    extern ULONG CompareDirectoryTable(VOLUME *volume);
    extern ULONG SyncVolumeLRU(LRU_HANDLE *lru_handle, VOLUME *volume);
    extern void DisplayLRUInfo(LRU_HANDLE *lru_handle);
    
    if (!VolumeMountedTable[volume->VolumeNumber])
    {
       NWFSPrint("Volume %s disk(%d) is not mounted\n",
	      volume->VolumeName, (int)volume->VolumeDisk);
       return -1;
    }

#if (ALLOW_REMOUNT)
    if (!force)
    {
       if (!volume->InUseCount)
       { 
          NWFSPrint("Volume %s disk(%d) is not in use\n",
	      volume->VolumeName, (int)volume->VolumeDisk);
          return -1;
       }

       volume->InUseCount--;
       if (volume->InUseCount)
          return 0;
    }
#endif
    
    NWFSPrint("Dismounting Volume %s\n", volume->VolumeName);

#if (VERBOSE)
    DisplayLRUInfo(&DataLRU);
    DisplayLRUInfo(&JournalLRU);
    DisplayLRUInfo(&FATLRU);
#endif

    SyncVolumeLRU(&DataLRU, volume);
    SyncVolumeLRU(&JournalLRU, volume);
    
    FreeFAT_LRU(volume);
    FlushVolumeLRU(volume);
    
    FreeClusterAssignedList(volume);
    FreeClusterFreeList(volume);
    FreeVolumeNamespaces(volume);

    FreeDirBlockHash(volume);
    FreeDirAssignHash(volume);

    // free any suballoc directory records
    for (i=0; i < MAX_SUBALLOC_NODES; i++)
    {
       if (volume->SuballocChain[i])
	  NWFSFree(volume->SuballocChain[i]);
       volume->SuballocChain[i] = 0;
    }

    // release suballoc free lists
    for (i=0; i < volume->SuballocListCount; i++)
    {
       if (volume->FreeSuballoc[i])
       {
	  retCode = FreeBitBlockList(volume->FreeSuballoc[i]);
	  if (retCode)
	     NWFSPrint("nwfs:  could not free suballoc bit list (%d)\n", (int)i);
	  NWFSFree(volume->FreeSuballoc[i]);
       }
       volume->FreeSuballoc[i] = 0;
    }

    FreeWorkspaceMemory(volume);
    FreeNSWorkspaceMemory(volume);
    nwvp_vpartition_close(volume->nwvp_handle);

    VolumeMountedTable[volume->VolumeNumber] = 0;

    return 0;

}


ULONG DismountVolume(BYTE *VolumeName)
{
    register ULONG retCode, i, len;
    register BYTE *p;

    len = 0;
    p = VolumeName;
    while (*p++)
       len++;

    NWLockScan();
    for (i=0; i < MaximumNumberOfVolumes; i++)
    {
       if (VolumeTable[i])
       {
          if (len == VolumeTable[i]->VolumeNameLength)
          {
	     if (!NWFSCompare(VolumeName, VolumeTable[i]->VolumeName,
			   VolumeTable[i]->VolumeNameLength))
	     {
	        retCode = DismountFileVolume(VolumeTable[i], 0);
                NWUnlockScan();
	        return retCode;
	     }
	  }
       }
    }
    NWUnlockScan();
    return -1;
}

ULONG DismountVolumeByHandle(VOLUME *volume)
{
    register ULONG ccode;

    NWLockScan();
    ccode = DismountFileVolume(volume, 0);
    NWUnlockScan();

    return ccode;
}

VOLUME *GetVolumeHandle(BYTE *VolumeName)
{
    register VOLUME *volume;
    register BYTE *p;
    register ULONG i, len;

    len = 0;
    p = VolumeName;
    while (*p++)
       len++;

    NWLockScan();
    for (i=0; i < MaximumNumberOfVolumes; i++)
    {
       if (VolumeTable[i])
       {
	  if (len == VolumeTable[i]->VolumeNameLength)
	  {
	     if (!NWFSCompare(VolumeName, VolumeTable[i]->VolumeName, len))
	     {
                volume = VolumeTable[i];
                NWUnlockScan();
	        return volume;
             }
	  }
       }
    }
    NWUnlockScan();
    return 0;
}

ULONG DismountAllVolumes(void)
{
    register ULONG retCode, finalCode = 0;
    register ULONG i;

    NWLockScan();
    for (i=0; i < MaximumNumberOfVolumes; i++)
    {
       if (VolumeTable[i])
       {
	  if (!VolumeMountedTable[i])
	     continue;
	  retCode = DismountFileVolume(VolumeTable[i], TRUE);
	  if (retCode)
	  {
	     finalCode = retCode;
	     NWFSPrint("nwfs:  error dismounting %s\n", VolumeTable[i]->VolumeName);
	  }
       }
    }
    NWUnlockScan();
    return finalCode;
}

//  WARNING!!!.  This mount routine is used to completely wipe all
//  file system data from a particular volume.  If you call it,
//  all your file system data will be cleared.  This routine is used
//  by the utilities to re-initialize a volume during imaging
//  and restore operations.

ULONG MountUtilityVolume(VOLUME *volume)
{
    register ULONG retCode, i;
    extern ULONG ReadFATTable(VOLUME *volume);
    extern ULONG ReadSuballocTable(VOLUME *volume);
    ULONG month, day, year, hour, minute, second;

    if (!volume->VolumePresent)
    {
       NWFSPrint("nwfs:  volume %s is missing one or more segments\n", volume->VolumeName);
       return -5;
    }

    if (VolumeMountedTable[volume->VolumeNumber])
    {
       NWFSPrint("nwfs:  volume %s disk(%d) already mounted\n",
	      volume->VolumeName, (int)volume->VolumeDisk);
       return -1;
    }

    NWFSPrint("Mounting FAT Tables Volume %s\n", volume->VolumeName);

    if (nwvp_vpartition_open(volume->nwvp_handle) != 0)
    {
       NWFSPrint("nwfs:  volume %s could not attach to NVMP\n",
		 volume->VolumeName);
       return -1;
    }

    month = day = year = hour = minute = second = 0;
    GetNWTime(NWFSSystemToNetwareTime(NWFSGetSystemTime()),
	      &second, &minute, &hour, &day, &month, &year);
    NWFSPrint("Mounting Volume %s  %02d/%02d/%02d %02d:%02d:%02d\n",
	       volume->VolumeName,
	       (int)month, (int)day, (int)year,
	       (int)hour, (int)minute, (int)second);

    volume->SuballocCount = 0;
    volume->SuballocChainComplete = 0;
    for (i=0; i < 5; i++)
       volume->SuballocChainFlag[i] = 0;

    volume->DirectoryCount = 0;
    volume->FreeDirectoryCount = 0;
    volume->FreeDirectoryBlockCount = 0;
    volume->VolumeAllocatedClusters = 0;
    volume->VolumeFreeClusters = 0;
    volume->DeletedDirNo = 0;

    volume->MinimumFATBlocks = MIN_FAT_LRU_BLOCKS;
    volume->MaximumFATBlocks = MAX_FAT_LRU_BLOCKS;

    volume->MountedNumberOfSegments = volume->NumberOfSegments;
    volume->MountedVolumeClusters = volume->VolumeClusters;

    if (InitializeFAT_LRU(volume))
    {
       retCode = -2;
       goto FATInitFailed;
    }

    if (InitializeClusterFreeList(volume))
    {
       retCode = -2;
       goto FreeBlockInitFailed;
    }

    if (InitializeClusterAssignedList(volume))
    {
       retCode = -2;
       goto AssignedBlockInitFailed;
    }

    retCode = ReadFATTable(volume);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  error reading FAT tables\n");
       goto ReadFATInitFailed;
    }

    if (volume->VolumeFlags & SUB_ALLOCATION_ON)
    {
       retCode = NWCreateSuballocRecords(volume);
       if (retCode)
       {
	  NWFSPrint("nwfs:  error creating suballoc tables (%d)\n",
		    (int)retCode);
	  retCode = -2;
	  goto ReadSuballocInitFailed;
       }

       if (!volume->SuballocChainComplete)
       {
	  retCode = -2;
	  NWFSPrint("nwfs:  suballoc tables are malformed (um)\n");
	  goto ReadSuballocInitFailed;
       }
    }

    retCode = BuildChainAssignment(volume, volume->FirstDirectory, 0);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  error checking primary directory chain\n");
       goto ReadSuballocInitFailed;
    }

    retCode = BuildChainAssignment(volume, volume->SecondDirectory, 0);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  error checking mirror directory chain\n");
       goto ReadSuballocInitFailed;
    }

    // this function will free all unused clusters from a volume.
    retCode = AdjustAllocatedClusters(volume);
    if (retCode)
       NWFSPrint("%d clusters have been freed\n", (int)retCode);

    volume->MountTime = NWFSGetSystemTime();
    volume->FileSeed = 0;
    volume->AllocSegment = 0;
    VolumeMountedTable[volume->VolumeNumber] = TRUE;

    NWFSPrint("Volume %s FAT Tables Mounted\n", volume->VolumeName);

    return 0;

// error exit code paths

ReadSuballocInitFailed:;
    // free any suballoc directory records
    for (i=0; i < MAX_SUBALLOC_NODES; i++)
    {
       if (volume->SuballocChain[i])
	  NWFSFree(volume->SuballocChain[i]);
       volume->SuballocChain[i] = 0;
    }

    // release suballoc free lists
    for (i=0; i < volume->SuballocListCount; i++)
    {
       if (volume->FreeSuballoc[i])
       {
	  retCode = FreeBitBlockList(volume->FreeSuballoc[i]);
	  if (retCode)
	     NWFSPrint("nwfs:  could not free suballoc bit list (%d)\n", (int)i);
	  NWFSFree(volume->FreeSuballoc[i]);
       }
       volume->FreeSuballoc[i] = 0;
    }

ReadFATInitFailed:;
   FreeClusterAssignedList(volume);

AssignedBlockInitFailed:;
   FreeClusterFreeList(volume);

FreeBlockInitFailed:;
    FreeFAT_LRU(volume);
    FlushVolumeLRU(volume);

FATInitFailed:;
    FreeWorkspaceMemory(volume);
    FreeNSWorkspaceMemory(volume);
    nwvp_vpartition_close(volume->nwvp_handle);

    return retCode;

}

ULONG DismountUtilityVolume(VOLUME *volume)
{
    register ULONG i, retCode;
    extern void FreeVolumeNamespaces(VOLUME *volume);

    if (!VolumeMountedTable[volume->VolumeNumber])
    {
       NWFSPrint("Volume %s disk(%d) is not mounted\n",
	      volume->VolumeName, (int)volume->VolumeDisk);
       return -1;
    }

    NWFSPrint("Dismounting FAT Tables Volume %s\n", volume->VolumeName);

    FreeFAT_LRU(volume);
    FlushVolumeLRU(volume);

    FreeClusterAssignedList(volume);
    FreeClusterFreeList(volume);

    // free any suballoc directory records
    for (i=0; i < MAX_SUBALLOC_NODES; i++)
    {
       if (volume->SuballocChain[i])
	  NWFSFree(volume->SuballocChain[i]);
       volume->SuballocChain[i] = 0;
    }

    // release suballoc free lists
    for (i=0; i < volume->SuballocListCount; i++)
    {
       if (volume->FreeSuballoc[i])
       {
	  retCode = FreeBitBlockList(volume->FreeSuballoc[i]);
	  if (retCode)
	     NWFSPrint("nwfs:  could not free suballoc bit list (%d)\n", (int)i);
	  NWFSFree(volume->FreeSuballoc[i]);
       }
       volume->FreeSuballoc[i] = 0;
    }
    FreeWorkspaceMemory(volume);
    FreeNSWorkspaceMemory(volume);
    nwvp_vpartition_close(volume->nwvp_handle);

    VolumeMountedTable[volume->VolumeNumber] = 0;

    return 0;

}

//  This mount routine is used to mount a volume with damaged directory
//  files after the FAT tables have been scanned and any mirror mismatches
//  dealt with, and the directory file needs to be repaired.  It is also
//  used to add namespaces to a volume from utility mode as well.  This
//  routine only mounts the FAT tables, and allows you to access the
//  NetWare volume meta-data as a collection of files with NWReadFile
//  and NWWriteFile.

ULONG MountRawVolume(VOLUME *volume)
{
    register ULONG retCode, i;
    extern ULONG ReadFATTable(VOLUME *volume);
    extern ULONG ReadSuballocTable(VOLUME *volume);
    ULONG month, day, year, hour, minute, second;

    if (!volume->VolumePresent)
    {
       NWFSPrint("nwfs:  volume %s is missing one or more segments\n", volume->VolumeName);
       return -5;
    }

    if (VolumeMountedTable[volume->VolumeNumber])
    {
       NWFSPrint("nwfs:  volume %s disk(%d) already mounted\n",
	      volume->VolumeName, (int)volume->VolumeDisk);
       return -1;
    }

#if (VERBOSE)
    NWFSPrint("Mounting Raw Volume %s\n", volume->VolumeName);
#endif

    month = day = year = hour = minute = second = 0;
    GetNWTime(NWFSSystemToNetwareTime(NWFSGetSystemTime()),
	      &second, &minute, &hour, &day, &month, &year);
    NWFSPrint("Mounting Volume %s  %02d/%02d/%02d %02d:%02d:%02d\n",
	       volume->VolumeName,
	       (int)month, (int)day, (int)year,
	       (int)hour, (int)minute, (int)second);

    if (nwvp_vpartition_open(volume->nwvp_handle) != 0)
    {
       NWFSPrint("nwfs:  volume %s could not attach to NVMP\n",
		 volume->VolumeName);
       return -1;
    }

    volume->SuballocCount = 0;
    volume->SuballocChainComplete = 0;
    for (i=0; i < 5; i++)
       volume->SuballocChainFlag[i] = 0;

    volume->DirectoryCount = 0;
    volume->FreeDirectoryCount = 0;
    volume->FreeDirectoryBlockCount = 0;
    volume->VolumeAllocatedClusters = 0;
    volume->VolumeFreeClusters = 0;
    volume->DeletedDirNo = 0;

    volume->MinimumFATBlocks = MIN_FAT_LRU_BLOCKS;
    volume->MaximumFATBlocks = MAX_FAT_LRU_BLOCKS;

    volume->MountedNumberOfSegments = volume->NumberOfSegments;
    volume->MountedVolumeClusters = volume->VolumeClusters;

    if (InitializeFAT_LRU(volume))
    {
       retCode = -2;
       goto FATInitFailed;
    }

    if (InitializeClusterFreeList(volume))
    {
       retCode = -2;
       goto FreeBlockInitFailed;
    }

    if (InitializeClusterAssignedList(volume))
    {
       retCode = -2;
       goto AssignedBlockInitFailed;
    }

    retCode = ReadFATTable(volume);
    if (retCode)
    {
       retCode = -2;
       NWFSPrint("nwfs:  error reading FAT tables\n");
       goto ReadFATInitFailed;
    }

    retCode = ReadSuballocTable(volume);
    if (retCode)
       NWFSPrint("nwfs:  error reading suballoc tables\n");

    if (volume->VolumeFlags & SUB_ALLOCATION_ON)
    {
       if (!volume->VolumeSuballocRoot)
	  retCode = NWCreateSuballocRecords(volume);
	  if (retCode)
	     NWFSPrint("nwfs:  error creating suballoc tables (%d)\n",
		       (int)retCode);

       if (!volume->SuballocChainComplete)
	  NWFSPrint("nwfs:  suballoc tables are malformed (raw)\n");
    }

    retCode = BuildChainAssignment(volume, volume->FirstDirectory, 0);
    if (retCode)
       NWFSPrint("nwfs:  error checking primary directory chain\n");

    retCode = BuildChainAssignment(volume, volume->SecondDirectory, 0);
    if (retCode)
       NWFSPrint("nwfs:  error checking mirror directory chain\n");

    volume->MountTime = NWFSGetSystemTime();
    volume->FileSeed = 0;
    volume->AllocSegment = 0;
    VolumeMountedTable[volume->VolumeNumber] = TRUE;

#if (VERBOSE)
    NWFSPrint("Raw Volume %s Mounted\n", volume->VolumeName);
#endif

    return 0;

// error exit code paths

ReadFATInitFailed:;
   FreeClusterAssignedList(volume);

AssignedBlockInitFailed:;
   FreeClusterFreeList(volume);

FreeBlockInitFailed:;
    FreeFAT_LRU(volume);
    FlushVolumeLRU(volume);

FATInitFailed:;
    FreeWorkspaceMemory(volume);
    FreeNSWorkspaceMemory(volume);
    nwvp_vpartition_close(volume->nwvp_handle);

    return retCode;

}

ULONG DismountRawVolume(VOLUME *volume)
{
    register ULONG i, retCode;
    extern void FreeVolumeNamespaces(VOLUME *volume);

    if (!VolumeMountedTable[volume->VolumeNumber])
    {
       NWFSPrint("Volume %s disk(%d) is not mounted\n",
	      volume->VolumeName, (int)volume->VolumeDisk);
       return -1;
    }

#if (VERBOSE)
    NWFSPrint("Dismounting Raw Volume %s\n", volume->VolumeName);
#endif

    FreeFAT_LRU(volume);
    FlushVolumeLRU(volume);

    FreeClusterAssignedList(volume);
    FreeClusterFreeList(volume);
    
    // free any suballoc directory records
    for (i=0; i < MAX_SUBALLOC_NODES; i++)
    {
       if (volume->SuballocChain[i])
	  NWFSFree(volume->SuballocChain[i]);
       volume->SuballocChain[i] = 0;
    }

    // release suballoc free lists
    for (i=0; i < volume->SuballocListCount; i++)
    {
       if (volume->FreeSuballoc[i])
       {
	  retCode = FreeBitBlockList(volume->FreeSuballoc[i]);
	  if (retCode)
	     NWFSPrint("nwfs:  could not free suballoc bit list (%d)\n", (int)i);
	  NWFSFree(volume->FreeSuballoc[i]);
       }
       volume->FreeSuballoc[i] = 0;
    }
    FreeWorkspaceMemory(volume);
    FreeNSWorkspaceMemory(volume);
    nwvp_vpartition_close(volume->nwvp_handle);

    VolumeMountedTable[volume->VolumeNumber] = 0;

    return 0;

}

ULONG CreateNWFSVolume(ULONG nwvp_handle, nwvp_vpartition_info *vpinfo)
{
    extern ULONG get_block_value(ULONG Type);
    register ULONG i, DirNo, j, k;
    register VOLUME *volume;
    register BYTE *Buffer;
    BYTE FoundNameSpace[MAX_NAMESPACES];

    for (volume = 0, i=0; i < MaximumNumberOfVolumes; i++)
    {
       volume = VolumeTable[i];
       if (volume)
       {
	  if (volume->nwvp_handle == nwvp_handle)
	  {
	     volume->ScanFlag = 0;
	     volume->volume_segments_ok_flag = (vpinfo->volume_valid_flag == 0xFFFFFFFF) ? 0 : 0xFFFFFFFF;
	     volume->fat_mirror_ok_flag = (vpinfo->volume_valid_flag == 1) ? 0 : 0xFFFFFFFF;
	     if (volume->VolumePresent == TRUE)
             {
                volume->VolumeClusters = vpinfo->cluster_count;
                volume->VolumeSectors = volume->VolumeClusters * volume->SectorsPerCluster;
                volume->NumberOfSegments = vpinfo->segment_count;
                volume->NumberOfFATEntries = volume->VolumeClusters;
                volume->MirrorFlag = vpinfo->mirror_flag;

                for(j=0; j < vpinfo->segment_count; j++)
                {
	           volume->SegmentClusterStart[j] = vpinfo->segments[j].relative_cluster_offset;
	           volume->SegmentClusterSize[j] = vpinfo->segments[j].segment_block_count / vpinfo->blocks_per_cluster;
                }
		return 0;
             }
	     break;
	  }
       }
    }

    for (i=0; i < MAX_VOLUMES; i++)
    {
       if (!VolumeTable[i])
       {
          volume = NWFSAlloc(sizeof(VOLUME), VOLUME_TAG);
          if (!volume)
          {
	     NWFSPrint("out of memory in CreateNWFSVolume\n");
	     return (ULONG) -1;
          }
          NWFSSet(volume, 0, sizeof(VOLUME));

	  NumberOfVolumes++;
	  if (NumberOfVolumes > MaximumNumberOfVolumes)
	     MaximumNumberOfVolumes = NumberOfVolumes;

	  VolumeTable[i] = volume;
	  volume->VolumeNumber = i;
	  volume->nwvp_handle = nwvp_handle;
	  volume->VolumeNameLength = vpinfo->volume_name[0];
	  NWFSCopy(volume->VolumeName, &vpinfo->volume_name[1], vpinfo->volume_name[0]);
	  break;
       }
    }

    volume->VolumeClusters = vpinfo->cluster_count;
    volume->ClusterSize = IO_BLOCK_SIZE * vpinfo->blocks_per_cluster;
    volume->BlockSize = IO_BLOCK_SIZE;
    volume->SectorsPerCluster = volume->ClusterSize / 512;
    volume->SectorsPerBlock = volume->BlockSize / 512;
    volume->VolumeSectors = volume->VolumeClusters * volume->SectorsPerCluster;

    volume->BlocksPerCluster = vpinfo->blocks_per_cluster;
    volume->NumberOfSegments = vpinfo->segment_count;
    volume->FirstFAT = vpinfo->FAT1;
    volume->SecondFAT = vpinfo->FAT2;
    volume->FirstDirectory = vpinfo->Directory1;
    volume->SecondDirectory = vpinfo->Directory2;
    volume->NumberOfFATEntries = volume->VolumeClusters;
    volume->MirrorFlag = vpinfo->mirror_flag;
    volume->DeletedDirNo = 0;

    for(i=0; i < vpinfo->segment_count; i++)
    {
	volume->SegmentClusterStart[i] = vpinfo->segments[i].relative_cluster_offset;
	volume->SegmentClusterSize[i] = vpinfo->segments[i].segment_block_count / vpinfo->blocks_per_cluster;
    }

    volume->volume_segments_ok_flag = (vpinfo->volume_valid_flag == 0xFFFFFFFF) ? 0 : 0xFFFFFFFF;
    volume->fat_mirror_ok_flag = (vpinfo->volume_valid_flag == 1) ? 0 : 0xFFFFFFFF;

    if (vpinfo->volume_valid_flag == 0)
    {
       volume->VolumePresent = TRUE;

       Buffer = NWFSIOAlloc(IO_BLOCK_SIZE, ALIGN_BUF_TAG);
       if (Buffer)
       {
	  register DOS *dos;
	  register ROOT *root;
	  register ROOT3X *root3x;
	  register ULONG DirCount, retCode;

	  nwvp_vpartition_open(volume->nwvp_handle);

	  retCode = nwvp_vpartition_block_read(volume->nwvp_handle,
					       (volume->FirstDirectory *
						volume->BlocksPerCluster),
					       1,
					       Buffer);
	  if (retCode == 0)
	  {
	     DirCount = IO_BLOCK_SIZE / 128;
	     for (DirNo=0; DirNo < DirCount; DirNo++)
	     {
		dos = (DOS *) &Buffer[DirNo * 128];
		if ((dos->Subdirectory == ROOT_NODE) && (!dos->NameSpace))
		{
		   root = (ROOT *) dos;
#if (VOLUME_DUMP)
		   dumpRecord((BYTE *)root, sizeof(ROOT));
		   dumpRecordBytes((BYTE *)root, sizeof(ROOT));
#endif
		   volume->VolumeFlags = root->VolumeFlags;
		   volume->ExtDirectory1 = root->ExtendedDirectoryChain0;
		   volume->ExtDirectory2 = root->ExtendedDirectoryChain1;

		   if ((volume->VolumeFlags & NDS_FLAG) ||
		       (volume->VolumeFlags & NEW_TRUSTEE_COUNT))
		   {
		      volume->VolumeSerialNumber = root->CreateDateAndTime;
		   }
		   else
		   {
		      root3x = (ROOT3X *) root;
		      volume->VolumeSerialNumber = root3x->CreateDateAndTime;
		   }

		   if (!root->NameSpaceCount)
		   {
		      volume->NameSpaceCount = 1;
		      volume->NameSpaceID[0] = DOS_NAME_SPACE;
		   }
		   else
		   {
		      volume->NameSpaceCount = root->NameSpaceCount;

		      // check if a 4.x/5.x volume

		      NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
		      if ((volume->VolumeFlags & NDS_FLAG) ||
			 (volume->VolumeFlags & NEW_TRUSTEE_COUNT))
		      {
			 volume->VolumeSuballocRoot = root->SubAllocationList;
			 for (j=0, k=0; j < volume->NameSpaceCount; j++)
			 {
			    if (j & 1)
			       volume->NameSpaceID[j] =
				(root->SupportedNameSpacesNibble[k++] >> 4) & 0xF;
			    else
			       volume->NameSpaceID[j] =
				 root->SupportedNameSpacesNibble[k] & 0xF;

			    if (!j && volume->NameSpaceID[j])
			    {
			       NWFSPrint("nwfs:  dos namespace not present\n");
			       volume->VolumePresent = FALSE;
			       NWFSFree(Buffer);
			       nwvp_vpartition_close(volume->nwvp_handle);
			       return -1;
			    }

			    if (j && (FoundNameSpace[volume->NameSpaceID[j] & 0xF]))
			    {
			       NWFSPrint("nwfs:  duplicate root namespaces detected (4x/5x)\n");
			       NWFSFree(Buffer);
			       nwvp_vpartition_close(volume->nwvp_handle);
			       return -1;
			    }
			    FoundNameSpace[volume->NameSpaceID[j] & 0xF] = TRUE;
			 }
		      }
		      else
		      {
			 // default to 3.x volume layout
			 volume->VolumeSuballocRoot = 0;
			 root3x = (ROOT3X *) root;
			 for (j=0; j < volume->NameSpaceCount; j++)
			 {
			    volume->NameSpaceID[j] =
			       root3x->NameSpaceTable[j] & 0xF;

			    if (!j && volume->NameSpaceID[j])
			    {
			       NWFSPrint("nwfs:  dos namespace not present\n");
			       volume->VolumePresent = FALSE;
			       NWFSFree(Buffer);
			       nwvp_vpartition_close(volume->nwvp_handle);
			       return -1;
			    }

			    if (j && (FoundNameSpace[volume->NameSpaceID[j] & 0xF]))
			    {
			       NWFSPrint("nwfs:  duplicate root namespaces detected (3x)\n");
			       NWFSFree(Buffer);
			       nwvp_vpartition_close(volume->nwvp_handle);
			       return -1;
			    }
			    FoundNameSpace[volume->NameSpaceID[j] & 0xF] = TRUE;
			 }
		      }
		   }
		}
	     }
	  }
	  else
	     NWFSPrint("read failed in CreateNWFSVolume\n");

	  NWFSFree(Buffer);
	  nwvp_vpartition_close(volume->nwvp_handle);
       }
       else
	  NWFSPrint("alloc failed in CreateNWFSVolume\n");
    }
    else
       NWFSPrint("nwfs:  invalid volume definition [%s] detected\n", volume->VolumeName);
    return 0;
}

void NWFSVolumeClose(void)
{
    nwvp_unscan_routine();
}

void NWFSVolumeScan(void)
{
    register VOLUME *volume;
    register ULONG j, i;
    ULONG handles[7];
    nwvp_payload payload;
    nwvp_vpartition_info vpinfo;

#if (WINDOWS_NT_RO | WINDOWS_NT)
    extern ULONG NwFsVolumeIsInvalid(IN ULONG VolumeNumber);
#endif

    NWLockScan();

    MaximumDisks = TotalDisks = 0;
    for (volume = 0, i=0; i < MaximumNumberOfVolumes; i++)
    {
       volume = VolumeTable[i];
       if (volume)
	  volume->ScanFlag = TRUE;
    }

    nwvp_scan_routine(0);

    payload.payload_object_count = 0;
    payload.payload_index = 0;
    payload.payload_object_size_buffer_size = (7 << 20) + sizeof(handles);
    payload.payload_buffer = (BYTE *) &handles[0];
    do
    {
       register ULONG ccode;

       nwvp_vpartition_scan(&payload);
       for (j=0; j < payload.payload_object_count; j++)
       {
	  ccode = nwvp_vpartition_return_info(handles[j], &vpinfo);
          if (!ccode)
          {
     	     CreateNWFSVolume(handles[j], &vpinfo);
	  }
       }
    } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));

    for (volume = 0, i=0; i < MaximumNumberOfVolumes; i++)
    {
       volume = VolumeTable[i];
       if (volume)
       {
	  if (volume->ScanFlag == TRUE)
	  {
#if (WINDOWS_NT | WINDOWS_NT_RO)
	     if (!NwFsVolumeIsInvalid(volume->VolumeNumber))
             {
	        VolumeMountedTable[volume->VolumeNumber] = 0;
	        VolumeTable[i] = 0;
	        NWFSFree(volume);
             }
#else
	     if (VolumeMountedTable[volume->VolumeNumber])
             {
		DismountFileVolume(volume, TRUE);
		VolumeMountedTable[volume->VolumeNumber] = 0;
	     }
	     VolumeTable[i] = 0;
	     NWFSFree(volume);
#endif
	  }
       }
    }

    NWUnlockScan();
    return;
}

//
//   0 - Netware 3.x, 1 - Netware 4.x/5.x
//
//  NOTE:  if LINUX_UTIL is set, we create the NFS namespace by default,
//         otherwise, we create the DOS and LONG namespace only by default.
//         At present, we are always creating Netware 4.x/5.x volumes with
//         the following default flags:
//
//         SUB_ALLOCATION_ON, FILE_COMPRESISON_ON, AUDITING_OFF
//         DATA_MIGRATION_OFF, NDS_PRESENT
//

ULONG CreateRootDirectory(BYTE *Buffer, ULONG size, ULONG Flags, ULONG ClusterSize)
{
    register ULONG i, j, Version, VolumeFlags;
    register DOS *dos;
    register ROOT *root, *volume_root;
    register ROOT3X *root3x;
    register LONGNAME *longname;
#if (LINUX_UTIL)
    register NFS *nfs;
#endif
    register SUBALLOC *suballoc;
    register ULONG SuballocListCount, SuballocCount;

    if (size != IO_BLOCK_SIZE || !Buffer)
       return -1;

    // set default volume type to 4.x/5.x
    Version = TRUE;

    // mask volume flags
    VolumeFlags = (Flags & 0x3F);

    // create 3.x volume if set
    if (Flags & 0x80000000)
       Version = 0;

    volume_root = (ROOT *) &Buffer[0];
    for (i=0; i < (IO_BLOCK_SIZE / sizeof(ROOT)); i++)
    {
       switch (i)
       {
	  case 0:
	     // create DOS_NAME_SPACE root directory record
	     if (Version)
	     {
		// create for 4.x/5.x NetWare
		root = (ROOT *) &Buffer[i * sizeof(ROOT)];
		NWFSSet(root, 0, sizeof(ROOT));
		root->Subdirectory = ROOT_NODE;
		root->Flags = SUBDIRECTORY_FILE | PRIMARY_NAMESPACE;
		root->NameSpace = DOS_NAME_SPACE;
#if (LINUX_UTIL)
		root->NameSpaceCount = 3;
		root->SupportedNameSpacesNibble[0] = (BYTE)((LONG_NAME_SPACE << 4) |
							    (DOS_NAME_SPACE & 0x0F));
		root->SupportedNameSpacesNibble[1] = (BYTE)(UNIX_NAME_SPACE & 0x0F);
#else
		root->NameSpaceCount = 2;
		root->SupportedNameSpacesNibble[0] = (BYTE)((LONG_NAME_SPACE << 4) |
							    (DOS_NAME_SPACE & 0x0F));
#endif
		root->FileAttributes = SUBDIRECTORY;
		root->VolumeFlags = (BYTE)(VolumeFlags | NEW_TRUSTEE_COUNT | NDS_FLAG);
		root->CreateDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());
		root->OwnerID = SUPERVISOR;
		root->LastModifiedDateAndTime = root->CreateDateAndTime;
		root->NameList = 1;
	     }
	     else
	     {
		// create for 3.x NetWare
		root3x = (ROOT3X *) &Buffer[i * sizeof(ROOT)];

		NWFSSet(root3x, 0, sizeof(ROOT));
		root3x->Subdirectory = ROOT_NODE;
		root3x->Flags = SUBDIRECTORY_FILE | PRIMARY_NAMESPACE;
		root3x->NameSpace = DOS_NAME_SPACE;
#if (LINUX_UTIL)
		root3x->NameSpaceCount = 3;
		root3x->NameSpaceTable[0] = DOS_NAME_SPACE;
		root3x->NameSpaceTable[1] = LONG_NAME_SPACE;
		root3x->NameSpaceTable[2] = UNIX_NAME_SPACE;
#else
		root3x->NameSpaceCount = 2;
		root3x->NameSpaceTable[0] = DOS_NAME_SPACE;
		root3x->NameSpaceTable[1] = LONG_NAME_SPACE;
#endif
		root3x->FileAttributes = SUBDIRECTORY;
		root3x->VolumeFlags = (BYTE) VolumeFlags;
		root3x->CreateDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());
		root3x->OwnerID = SUPERVISOR;
		root3x->LastModifiedDateAndTime = root3x->CreateDateAndTime;
		root3x->NameList = 1;
	     }
	     break;

	  case 1:
	     // create LONG_NAME_SPACE root directory record
	     longname = (LONGNAME *) &Buffer[i * sizeof(ROOT)];
	     NWFSSet(longname, 0, sizeof(LONGNAME));
	     longname->Subdirectory = ROOT_NODE;
	     longname->NameSpace = LONG_NAME_SPACE;
	     longname->Flags = SUBDIRECTORY_FILE;
#if (LINUX_UTIL)
	     longname->NameList = 2;
#else
	     longname->NameList = 0;
#endif
	     break;

#if (LINUX_UTIL)
	  case 2:
	     // create UNIX_NAME_SPACE root directory record
	     nfs = (NFS *) &Buffer[i * sizeof(ROOT)];
	     NWFSSet(nfs, 0, sizeof(NFS));
	     nfs->Subdirectory = ROOT_NODE;
	     nfs->NameSpace = UNIX_NAME_SPACE;
	     nfs->Flags = SUBDIRECTORY_FILE;
             nfs->flags = 6;
	     nfs->nlinks = 2;
	     break;
#endif

	  // NOTE:  Netware does *NOT* create suballocation records during
	  //        volume creation.  Instead, Netware creates these records
	  //        the first time the volume is mounted.  We create these
	  //        suballoc chains by default during volume create if the
	  //        SUBALLOCATION_ON flags is set to allow R/O volumes to
	  //        be mounted under NT.

	  case 9:
	     if (Version && (volume_root->VolumeFlags & SUB_ALLOCATION_ON))
	     {
		SuballocListCount = (ClusterSize / SUBALLOC_BLOCK_SIZE) - 1;
		SuballocCount = (SuballocListCount + 27) / 28;

		// initialize the suballoc root;
		volume_root->SubAllocationList = i;

		for (j=0; j < (SuballocCount - 1); j++)
		{
		   // store the DirNo for next record into SuballocationList
		   suballoc = (SUBALLOC *) &Buffer[(j + i) * sizeof(ROOT)];
		   NWFSSet(suballoc, 0, sizeof(SUBALLOC));
		   suballoc->Flag = SUBALLOC_NODE;
		   suballoc->SubAllocationList = (j + i) + 1;
		   suballoc->SequenceNumber = (BYTE) j;
		}

		// null terminate last suballoc directory record
		suballoc = (SUBALLOC *) &Buffer[(j + i) * sizeof(ROOT)];
		NWFSSet(suballoc, 0, sizeof(SUBALLOC));
		suballoc->Flag = SUBALLOC_NODE;
		suballoc->SubAllocationList = 0;
		suballoc->SequenceNumber = (BYTE) j;
		i = (j + i);
		break;
	     };
	     // fall through if suballocation is disabled or we are creating
	     // a Netware 3.x volume.

	  default:
	     // mark all other entries as free
	     dos = (DOS *) &Buffer[i * sizeof(DOS)];
	     NWFSSet(dos, 0, sizeof(DOS));
	     dos->Subdirectory = (ULONG) -1;
	     break;
       }
    }
    return 0;

}

BYTE *get_ns_string(ULONG ns)
{
    switch (ns)
    {
       case 0:
	  return "DOS ";

       case 1:
	  return "MAC ";

       case 2:
	  return "NFS ";

       case 3:
	  return "FTAM ";

       case 4:
	  return "LONG ";

       case 5:
	  return "NT ";

       case 6:
	  return "FEN ";

       default:
	  return "";
    }

}

void ReportVolumes(void)
{
    register ULONG i, TotalVolumes, TotalSegments;
    extern ULONG TotalDisks;

    for (TotalSegments = TotalVolumes = 0, i = 0; i < MAX_VOLUMES; i++)
    {
       if (VolumeTable[i])
       {
	  if (VolumeTable[i]->VolumePresent)
	  {
	     TotalVolumes++;
	     TotalSegments += VolumeTable[i]->NumberOfSegments;
	  }
       }
    }
    NWFSPrint("%d NetWare volume(s) located,  %d volume segment(s),  %d disk(s)\n",
	   (int)TotalVolumes,
	   (int)TotalSegments,
	   (int)TotalDisks);
    return;
}

#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)

void displayNetwareVolumes(void)
{
    register ULONG i, r, lines = 0;
    extern ULONG Pause(void);

    NWFSPrint("NetWare Volume(s)\n");

    lines++;
    for (i=0; i < MaximumNumberOfVolumes; i++)
    {
       if (VolumeTable[i])
       {
	  NWFSPrint("[%-15s] sz-%08X (%uMB) blk-%d  F/D-%04X/%04X  (%s)\n",
		 VolumeTable[i]->VolumeName,
		 (int)VolumeTable[i]->VolumeClusters,
		 (unsigned int)
		   (((LONGLONG)VolumeTable[i]->VolumeClusters *
		     (LONGLONG)VolumeTable[i]->ClusterSize) /
		      0x100000),
		 (unsigned int)VolumeTable[i]->ClusterSize,
		 (unsigned int)VolumeTable[i]->FirstFAT,
		 (unsigned int)VolumeTable[i]->FirstDirectory,
		 VolumeTable[i]->VolumePresent
		 ? "OK"
		 : "NO");
	  if (lines++ > 22)
	  {
	     lines = 0;
	     if (Pause())
		return;
	  }

	  NWFSPrint(" NAMESPACES  [");

	  if (VolumeTable[i]->NameSpaceCount)
	  {
	     for (r=0; r < (VolumeTable[i]->NameSpaceCount & 0xF); r++)
		NWFSPrint(" %s ", get_ns_string(VolumeTable[i]->NameSpaceID[r]));
	  }
	  else
	  {
	     // if NameSpaceCount = 0, then we assume a single DOS namespace
	     NWFSPrint(" DOS ");
	  }

	  NWFSPrint("]\n");
	  if (lines++ > 22)
	  {
	     lines = 0;
	     if (Pause())
		return;
	  }

	  NWFSPrint(" COMPRESS-%s SUBALLOC-%s MIGRATE-%s AUDIT-%s\n",
		 (VolumeTable[i]->VolumeFlags & FILE_COMPRESSION_ON)
		 ? "YES"
		 : "NO ",
		 (VolumeTable[i]->VolumeFlags & SUB_ALLOCATION_ON)
		 ? "YES"
		 : "NO ",
		 (VolumeTable[i]->VolumeFlags & DATA_MIGRATION_ON)
		 ? "YES"
		 : "NO ",
		 (VolumeTable[i]->VolumeFlags & AUDITING_ON)
		 ? "YES"
		 : "NO ");
	  if (lines++ > 22)
	  {
	     lines = 0;
	     if (Pause())
		return;
	  }

	  for (r=0; r < VolumeTable[i]->NumberOfSegments; r++)
	  {
	     if (VolumeTable[i]->SegmentClusterSize[r])
	     {
		NWFSPrint("  segment #%d Start-%08X size-%08X (%uMB)\n",
		     (int) r,
		     (unsigned int)VolumeTable[i]->SegmentClusterStart[r],
		     (unsigned int)VolumeTable[i]->SegmentClusterSize[r],
		     (unsigned int)
		       (((LONGLONG)VolumeTable[i]->SegmentClusterSize[r] *
			 (LONGLONG)VolumeTable[i]->ClusterSize) /
			 0x100000));

		if (lines++ > 22)
		{
		   lines = 0;
		   if (Pause())
		      return;
		}
	     }
	     else
	     {
		NWFSPrint("  segment #%d *** error: defined segment missing ***\n",
			 (int) r);
		if (lines++ > 22)
		{
		   lines = 0;
		   if (Pause())
		      return;
		}
	     }
	  }
       }
    }
}


#endif
