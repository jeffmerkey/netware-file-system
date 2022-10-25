
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
*   FILE     :  NWFILE.C
*   DESCRIP  :   NetWare File Management
*   DATE     :  December 14, 1998
*
*
***************************************************************************/

#include "globals.h"

ULONG NWLockFile(HASH *hash)
{
#if (LINUX_SLEEP)
    if (WaitOnSemaphore((struct semaphore *)&hash->Semaphore) == -EINTR)
       NWFSPrint("lock file (%s) was interrupted\n", hash->Name);
#endif
    return 0;
}

ULONG NWLockFileExclusive(HASH *hash)
{
#if (LINUX_SLEEP)
   if (WaitOnSemaphore((struct semaphore *)&hash->Semaphore) == -EINTR)
       NWFSPrint("lock file (%s) was interrupted\n", hash->Name);
#endif
    return 0;
}

void NWUnlockFile(HASH *hash)
{
#if (LINUX_SLEEP)
    SignalSemaphore(&hash->Semaphore);
#endif
}

#if (WINDOWS_NT_UTIL | DOS_UTIL | LINUX_UTIL | LINUX_20 | LINUX_22 | LINUX_24)

ULONG NWSyncFile(VOLUME *volume, HASH *hash)
{
    register long FATChain;
    register ULONG ccode;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    SUBALLOC_MAP map;
    extern ULONG SyncCluster(VOLUME *, ULONG);
#if (!HASH_FAT_CHAINS)
    DOS dos;
#endif
    
    if (!hash || (hash->Flags & SUBDIRECTORY_FILE))
       return (ULONG) -1;

#if (HASH_FAT_CHAINS)
    FATChain = hash->FirstBlock;
#else
    ccode = ReadDirectoryRecord(volume, &dos, hash->DirNo);
    if (ccode)
       return -1;

    FATChain = dos.FirstBlock;
#endif

    if (FATChain & 0x80000000)
    {
       // check for EOF
       if (FATChain == (ULONG) -1)
          return 0;

       ccode = MapSuballocNode(volume, &map, FATChain);
       if (ccode)
          return (ULONG) ccode;

       if (map.Count == 1)
       {
          ccode = SyncCluster(volume, map.clusterNumber[0]);
          if (ccode)
             return ccode;
       }
       else
       {
          ccode = SyncCluster(volume, map.clusterNumber[0]);
          if (ccode)
             return ccode;
             
	  ccode = SyncCluster(volume, map.clusterNumber[1]);
          if (ccode)
             return ccode;
       }
    }

    FAT = GetFatEntry(volume, FATChain, &FAT_S);
    while (FAT && FAT->FATCluster)
    {
       ccode = SyncCluster(volume, FATChain);
       if (ccode)
          return ccode;
     
       // bump to the next cluster
       FATChain = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF marker
       if (FATChain & 0x80000000)
       {
	  // end of file
	  if (FATChain == (ULONG) -1)
             return 0;

	  ccode = MapSuballocNode(volume, &map, FATChain);
	  if (ccode)
             return (ULONG) ccode;

          if (map.Count == 1)
	  {
             ccode = SyncCluster(volume, map.clusterNumber[0]);
             if (ccode)
                return ccode;
	  }
	  else
	  {
             ccode = SyncCluster(volume, map.clusterNumber[0]);
             if (ccode)
                return ccode;
             
	     ccode = SyncCluster(volume, map.clusterNumber[1]);
             if (ccode)
                return ccode;
	  }
          return 0; 
       }

       FAT = GetFatEntry(volume, FATChain, &FAT_S);
    }
    return 0;

}

ULONG NWReadFile(VOLUME *volume, ULONG *Chain, ULONG Flags, ULONG FileSize,
		 ULONG offset, BYTE *buf, long count, long *Context,
		 ULONG *Index, ULONG *retCode, ULONG as, ULONG SAFlag,
		 ULONG Attributes, ULONG readAhead)
{
    register long PrevContext = 0;
    register ULONG PrevIndex = 0;
    register ULONG FATChain, index;
    register long bytesRead = 0, bytesLeft = 0;
    register ULONG StartIndex, StartOffset;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    register ULONG voffset, vsize, vindex, cbytes;

    if (!Chain)
       return 0;

    if (!(*Chain))
       return 0;

    if (retCode)
       *retCode = 0;

    // adjust size and range check for EOF

    if ((offset + count) > FileSize)
       count = FileSize - offset;

    if (count <= 0)
       return 0;

    // if a subdirectory then return 0
    if (Flags & SUBDIRECTORY_FILE)
    {
       if (retCode)
	  *retCode = NwInvalidParameter;
       return 0;
    }

    bytesLeft = count;
    StartIndex = offset / volume->ClusterSize;
    StartOffset = offset % volume->ClusterSize;

    // we always start with an index of zero
    index = 0;
    FATChain = *Chain;
    vindex = StartIndex;

    // see if we are starting on or after the supplied index/cluster pointer.
    // if (StartIndex < *Index), then we are being asked to start before
    // the user supplied context.  In this case, start at the beginning of
    // the chain.

    if (Index && (StartIndex >= (*Index)))
    {
       if (Context && (*Context))
       {
	  PrevContext = FATChain = *Context;
	  PrevIndex = index = *Index;
       }
    }

    if ((bytesLeft > 0) && (FATChain & 0x80000000))
    {
       // check for EOF
       if (FATChain == (ULONG) -1)
       {
	  // filesize may exceed allocation, which means the rest of
	  // the file is sparse.  fill zeros into the requested
	  // size.  bytesLeft will have been set by count, which is
	  // range checked to the actual length of the file.

	  if (bytesLeft > 0)
	  {
             if (buf)
	     {
	        if (NWFSSetUserSpace(buf, 0, bytesLeft))
		{
                   if (retCode)
		      *retCode = NwMemoryFault;
     		   return bytesRead;
		}
	     }
	     bytesRead += bytesLeft;
	  }
	  if (Context)
	     *Context = PrevContext;
	  if (Index)
	     *Index = PrevIndex;
	  return bytesRead;
       }

       // if we got here, then we detected a suballocation element.  See
       // if this is legal for this file.
       if ((!SAFlag) || (Attributes & NO_SUBALLOC) || 
	  (Attributes & TRANSACTION) || (Flags & RAW_FILE))
       {
	  if (retCode)
	     *retCode = NwNotPermitted;
	  return bytesRead;
       }

       // check for valid index
       if (index == vindex)
       {
	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  // if this value exceeds the suballoc record size,
	  // ReadSuballoc record will reduce this size to the
	  // current record allocation.
	  vsize = bytesLeft;

          if (buf)
	     cbytes = ReadSuballocRecord(volume, voffset, FATChain, buf, vsize, as, retCode);
	  else
	     cbytes = vsize;

	  bytesRead += cbytes;
	  bytesLeft -= cbytes;
	  buf += cbytes;

	  if (Context)
	     *Context = FATChain;
	  if (Index)
	     *Index = index;
       }

       // filesize may exceed allocation, which means the rest of
       // the file is sparse.  fill zeros into the requested
       // size.

       if (bytesLeft > 0)
       {
          if (buf)
	  {
	     if (NWFSSetUserSpace(buf, 0, bytesLeft))
	     {
                if (retCode)
		   *retCode = NwMemoryFault;
     		return bytesRead;
	     }
	  }
	  bytesRead += bytesLeft;
       }
       return bytesRead;
    }

    vindex = StartIndex;
    FAT = GetFatEntry(volume, FATChain, &FAT_S);
    if (FAT)
    {
       if (Context)
	  *Context = FATChain;
       index = FAT->FATIndex;
       if (Index)
	  *Index = index;
    }

    while (FAT && FAT->FATCluster && (bytesLeft > 0))
    {
       //  if we found a hole, then return zeros until we
       //  either satisfy the requested read size or
       //  we span to the next valid index entry

       while ((bytesLeft > 0) && (vindex < index))
       {
	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

          if (buf)
	  {
	     if (NWFSSetUserSpace(buf, 0, vsize))
	     {
                if (retCode)
		   *retCode = NwMemoryFault;
     		return bytesRead;
	     }
	  }
	  bytesRead += vsize;
	  bytesLeft -= vsize;
	  buf += vsize;
	  vindex++;
       }

       // found our index block, perform the copy operation

       if ((bytesLeft > 0) && (vindex == index))
       {
	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

          if (buf)
	     cbytes = ReadClusterWithOffset(volume, FATChain, voffset, buf, vsize,
					 as, retCode, 
                                         ((Flags & RAW_FILE)
					 ? RAW_PRIORITY 
					 : DATA_PRIORITY));
          else
             cbytes = vsize;

#if (READ_AHEAD_ON)
	  // do read ahead on the next valid cluster
	  if (((FAT->FATCluster & 0x80000000) == 0) && (buf) && readAhead &&
	       (!(Flags & RAW_FILE)))
	  {
             ULONG mirror = (ULONG)-1;
		  
     	     PerformBlockReadAhead(&DataLRU, volume,
				  (FAT->FATCluster * volume->BlocksPerCluster),
				   volume->BlocksPerCluster, 1, &mirror);
	  }
#endif

	  bytesRead += cbytes;
	  bytesLeft -= cbytes;
	  buf += cbytes;
	  vindex++;
       }

       // bump to the next cluster
       PrevContext = FATChain;
       PrevIndex = index;
       FATChain = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF marker
       if ((bytesLeft > 0) && (FATChain & 0x80000000))
       {
	  // end of file
	  if (FATChain == (ULONG) -1)
	  {
	     // filesize may exceed allocation, which means the rest of
	     // the file is sparse.  fill zeros into the requested
	     // size.
	     if (bytesLeft > 0)
	     {
                if (buf)
		{
		   if (NWFSSetUserSpace(buf, 0, bytesLeft))
		   {
                      if (retCode)
		         *retCode = NwMemoryFault;
     		      return bytesRead; 
		   }
		}
		bytesRead += bytesLeft;
	     }
	     if (Context)
		*Context = PrevContext;
	     if (Index)
		*Index = PrevIndex;
	     return bytesRead;
	  }

	  // if we got here, then we detected a suballocation element.  See
	  // if this is legal for this file.

	  if ((!SAFlag) || (Attributes & NO_SUBALLOC) || 
	      (Attributes & TRANSACTION) || (Flags & RAW_FILE))
	  {
	     if (retCode)
		*retCode = NwNotPermitted;
	     return bytesRead;
	  }

	  // check for valid index
	  if ((index + 1) == vindex)
	  {
	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     // if this value exceeds the suballoc record size,
	     // ReadSuballoc record will reduce this size to the
	     // current record allocation.
	     vsize = bytesLeft;

             if (buf)
	        cbytes = ReadSuballocRecord(volume, voffset, FATChain, buf,
					 vsize, as, retCode);
             else
                cbytes = vsize;

	     bytesRead += cbytes;
	     bytesLeft -= cbytes;
	     buf += cbytes;

	     if (Context)
		*Context = FATChain;
	     if (Index)
		*Index = (index + 1);
	  }

	  // filesize may exceed allocation, which means the rest of
	  // the file is sparse.  fill zeros into the requested
	  // size.
	  if (bytesLeft > 0)
	  {
             if (buf)
	     {
		if (NWFSSetUserSpace(buf, 0, bytesLeft))
		{
                   if (retCode)
		      *retCode = NwMemoryFault;
     		   return bytesRead; 
	        }
	     }
	     bytesRead += bytesLeft;
	  }
	  return bytesRead;
       }

       // get next fat table entry and index
       FAT = GetFatEntry(volume, FATChain, &FAT_S);
       if (FAT)
       {
	  if (Context)
	     *Context = FATChain;
	  index = FAT->FATIndex;
	  if (Index)
	     *Index = index;
       }
    }

    // filesize may exceed allocation, which means the rest of
    // the file is sparse.  fill zeros into the requested
    // size.
    if (bytesLeft > 0)
    {
       if (buf)
       {
	  if (NWFSSetUserSpace(buf, 0, bytesLeft))
	  {
             if (retCode)
	        *retCode = NwMemoryFault;
     	     return bytesRead; 
          }
       }
       bytesRead += bytesLeft;
    }
    return bytesRead;

}

//
//  if you pass a NULL buffer address, this function will extend the
//  meta-data for a file and preallocate mapped space for memory mapped
//  file support.
//

ULONG NWWriteFile(VOLUME *volume, ULONG *Chain, ULONG Flags,
		  ULONG offset, BYTE *buf, long count, long *Context,
		  ULONG *Index, ULONG *retCode, ULONG as, ULONG SAFlag,
		  ULONG Attributes)
{
    register long PrevContext = 0;
    register ULONG PrevIndex = 0;
    register ULONG FATChain, index;
    register long bytesWritten = 0, bytesLeft = 0;
    register ULONG lcount = 0;
    register ULONG StartIndex, StartOffset, SuballocSize;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    register ULONG voffset, vsize, vindex, cbytes;
    register ULONG PrevCluster, NewCluster;
    register VOLUME_WORKSPACE *WorkSpace;
    MIRROR_LRU *lru = 0;

    if (!Chain)
       return 0;

    if (!(*Chain))
    {
       NWFSPrint("nwfs:  fat chain was NULL (write)\n");
       if (retCode)
	  *retCode = NwFileCorrupt;
       return 0;
    }

    if (retCode)
       *retCode = 0;

    // if a subdirectory then return 0
    if (Flags & SUBDIRECTORY_FILE)
    {
       if (retCode)
	  *retCode = NwInvalidParameter;
       return 0;
    }

    bytesLeft = count;
    StartIndex = offset / volume->ClusterSize;
    StartOffset = offset % volume->ClusterSize;

    // we always start with an index of zero
    index = 0;
    PrevCluster = (ULONG) -1;
    FATChain = *Chain;
    vindex = StartIndex;

#if (VERBOSE)
    NWFSPrint("nwfs_write:  count-%d offset-%d ctx-%08X ndx-%08X chn-%08X\n",
	      (int)count, (int)offset,
	      (unsigned int) (Context ? *Context : 0),
	      (unsigned int) (Index ? *Index : 0),
	      (unsigned int) *Chain);
#endif

    // see if we are starting on or after the supplied index/cluster pointer.
    // if (StartIndex < *Index), then we are being asked to start before
    // the user supplied context.  In this case, start at the beginning of
    // the chain.

    if (Index && (StartIndex >= (*Index)))
    {
       if (Context && (*Context))
       {
	  PrevContext = FATChain = *Context;
	  PrevIndex = index = *Index;
       }
    }

    if ((bytesLeft > 0) && (FATChain & 0x80000000))
    {
       // check for EOF
       if (FATChain == (ULONG) -1)
       {
	  while (bytesLeft > 0)
	  {
	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		     ? (volume->ClusterSize - voffset) : bytesLeft;

	     // allocate cluster and point forward link to EOF
	     NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
	     if (NewCluster == -1)
	     {
		// if we could not get a free cluster, then return
		// (out of drive space)
		if (retCode)
		   *retCode = NwVolumeFull;

		return bytesWritten;
	     }

#if (ZERO_FILL_SECTORS)
	     // zero fill the new cluster
	     ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	     // set previous cluster chain to point to this entry
	     if (PrevCluster == (ULONG) -1)
		*Chain = NewCluster;
	     else
		SetClusterValue(volume, PrevCluster, NewCluster);

	     // update previous cluster to new cluster
	     // this will force inserts after the end of this cluster
	     PrevCluster = NewCluster;

             if (buf)
	        cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
					     as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
             else
                cbytes = vsize;

	     bytesWritten += cbytes;
	     bytesLeft -= cbytes;
	     buf += cbytes;

	     // update context pointer with adjusted offset
	     if (Context)
		*Context = NewCluster;
	     if (Index)
		*Index = vindex;
	     vindex++;
	  }
	  return bytesWritten;
       }

       // we have detected a suballoc element in the fat chain if we
       // get to this point

       // if we got here, then we detected a suballocation element.  See
       // if this is legal for this file.
       if ((!SAFlag) || (Attributes & NO_SUBALLOC) || 
           (Attributes & TRANSACTION) || (Flags & RAW_FILE))
       {
	  if (retCode)
	     *retCode = NwNotPermitted;
	  return bytesWritten;
       }

       SuballocSize = GetSuballocSize(volume, FATChain);
       voffset = 0;
       if (vindex == StartIndex)
	  voffset = StartOffset;

       // these cases assume we will free the current suballoc element
       // because our write either exceeds the current suballoc size
       // or we are writing our starting index beyond the suballocation
       // element itself in the chain.  In either case, we must free
       // the current suballocation element.

       // if we are asked to write beyond the current suballoc element
       if (vindex > index)
       {
#if (VERBOSE)
	  NWFSPrint("nwfs:  we were asked to write to the next index\n");
#endif
	  // convert the suballoc record into an allocated cluster
	  // and copy the data from the suballoc record.

	  WorkSpace = AllocateWorkspace(volume);
	  if (!WorkSpace)
	  {
	     // if we could not get memory to copy the suballoc record,
	     // then return (out of drive space)
	     if (retCode)
		*retCode = NwInsufficientResources;
	     return bytesWritten;
	  }

	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

	  // allocate cluster and point forward link to EOF
	  NewCluster = AllocateClusterSetIndexSetChain(volume, index, -1);
	  if (NewCluster == -1)
	  {
	     // if we could not get a free cluster, then return
	     // (out of drive space)
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwVolumeFull;
	     return bytesWritten;
	  }

#if (ZERO_FILL_SECTORS)
	  // zero fill the new cluster
	  ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	  // here we read the previous data from the suballoc element
	  cbytes = ReadSuballocRecord(volume, 0, FATChain,
				      &WorkSpace->Buffer[0],
				      SuballocSize, KERNEL_ADDRESS_SPACE,
				      retCode);
	  if (cbytes != SuballocSize)
	  {
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwDiskIoError;
	     return bytesWritten;
	  }

	  // now write the previous data from the suballoc element
	  // into the newly allocated cluster.
	  cbytes = WriteClusterWithOffset(volume, NewCluster, 0, &WorkSpace->Buffer[0], SuballocSize,
					  KERNEL_ADDRESS_SPACE, retCode,
					  ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	  if (cbytes != SuballocSize)
	  {
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwDiskIoError;
	     return bytesWritten;
	  }

	  // free the suballoc element in bit block list
	  FreeSuballocRecord(volume, FATChain);

	  // set previous cluster chain to point to this entry
	  if (PrevCluster == (ULONG) -1)
	     *Chain = NewCluster;
	  else
	     SetClusterValue(volume, PrevCluster, NewCluster);

	  PrevCluster = NewCluster;

	  // update context pointer with adjusted offset
	  if (Context)
	     *Context = NewCluster;
	  if (Index)
	     *Index = index;

	  FreeWorkspace(volume, WorkSpace);

	  while (bytesLeft > 0)
	  {
	     voffset = 0;
	     if (vindex == StartIndex)
		 voffset = StartOffset;

	     vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		     ? (volume->ClusterSize - voffset) : bytesLeft;

	     // allocate cluster and point forward link to EOF
	     NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
	     if (NewCluster == -1)
	     {
		// if we could not get a free cluster, then return
		// (out of drive space)
		if (retCode)
		   *retCode = NwVolumeFull;
		return bytesWritten;
	     }

#if (ZERO_FILL_SECTORS)
	     // zero fill the new cluster
	     ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	     // set previous cluster chain to point to this entry
	     SetClusterValue(volume, PrevCluster, NewCluster);

	     // update previous cluster to new cluster
	     // this will force inserts after the end of this cluster
	     PrevCluster = NewCluster;

             if (buf)
	        cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
						as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
             else
                cbytes = vsize;

	     bytesWritten += cbytes;
	     bytesLeft -= cbytes;
	     buf += cbytes;

	     // update context pointer with adjusted offset
	     if (Context)
		*Context = NewCluster;
	     if (Index)
		*Index = vindex;
	     vindex++;
	  }
	  return bytesWritten;
       }
       else
       if ((vindex == index) && ((bytesLeft + voffset) >= SuballocSize))
       {
#if (VERBOSE)
	  NWFSPrint("nwfs:  we were asked to write beyond the suballoc\n");
#endif
	  WorkSpace = AllocateWorkspace(volume);
	  if (!WorkSpace)
	  {
	     // if we could not get memory to copy the suballoc record,
	     // then return (out of drive space)
	     if (retCode)
		*retCode = NwInsufficientResources;

	     return bytesWritten;
	  }

	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

	  // allocate cluster and point forward link to EOF
	  NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, (ULONG) -1);
	  if (NewCluster == -1)
	  {
	     // if we could not get a free cluster, then return
	     // (out of drive space)
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwVolumeFull;
	     return bytesWritten;
	  }

#if (ZERO_FILL_SECTORS)
	  // zero fill the new cluster
	  ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	  // here we read the previous data from the suballoc element
	  cbytes = ReadSuballocRecord(volume, 0, FATChain, &WorkSpace->Buffer[0],
				      SuballocSize, KERNEL_ADDRESS_SPACE,
				      retCode);
	  if (cbytes != SuballocSize)
	  {
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwDiskIoError;
	     return bytesWritten;
	  }

	  // now write the previous data from the suballoc element
	  // into the newly allocated cluster.
	  cbytes = WriteClusterWithOffset(volume, NewCluster, 0, &WorkSpace->Buffer[0], SuballocSize,
					  KERNEL_ADDRESS_SPACE, retCode,
					  ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	  if (cbytes != SuballocSize)
	  {
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwDiskIoError;
	     return bytesWritten;
	  }

	  // free the suballoc element in bit block list
	  FreeSuballocRecord(volume, FATChain);

	  // if we are the first cluster, then save the chain head in
	  // the directory record

	  if (PrevCluster == (ULONG) -1)
	     *Chain = NewCluster;
	  else
	     SetClusterValue(volume, PrevCluster, NewCluster);

	  PrevCluster = NewCluster;

	  // now write the user data into the suballoc element
          if (buf)
	     cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
					  as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
          else
             cbytes = vsize;

	  bytesWritten += cbytes;
	  bytesLeft -= cbytes;
	  buf += cbytes;

	  // update context pointer with adjusted offset
	  if (Context)
	     *Context = NewCluster;
	  if (Index)
	     *Index = vindex;
	  vindex++;

	  FreeWorkspace(volume, WorkSpace);

	  while (bytesLeft > 0)
	  {
	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		     ? (volume->ClusterSize - voffset) : bytesLeft;

	     // allocate cluster and point forward link to EOF
	     NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
	     if (NewCluster == -1)
	     {
		// if we could not get a free cluster, then return
		// (out of drive space)
		if (retCode)
		   *retCode = NwVolumeFull;
		return bytesWritten;
	     }

#if (ZERO_FILL_SECTORS)
	     // zero fill the new cluster
	     ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	     // set previous cluster chain to point to this entry
	     SetClusterValue(volume, PrevCluster, NewCluster);

	     // update previous cluster to new cluster
	     // this will force inserts after the end of this cluster
	     PrevCluster = NewCluster;

             if (buf)
	        cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
					     as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
             else
                cbytes = vsize;

	     bytesWritten += cbytes;
	     bytesLeft -= cbytes;
	     buf += cbytes;

	     // update context pointer with adjusted offset
	     if (Context)
		*Context = NewCluster;
	     if (Index)
		*Index = vindex;
	     vindex++;
	  }
	  return bytesWritten;
       }
       else
       if (vindex == index)
       {
	  // for this case, since our target write size fits within
	  // the previously allocated suballoc element, then just
	  // write the data.
#if (VERBOSE)
	  NWFSPrint("nwfs:  we were asked to write within the suballoc\n");
#endif
	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  // at this point, bytesLeft is either equal to or
	  // less than the size of the current suballocation
	  // record.

	  vsize = bytesLeft;

          if (buf)
	     cbytes = WriteSuballocRecord(volume, voffset, FATChain,
				       buf, vsize, as, retCode);
          else
             cbytes = vsize;

	  bytesWritten += cbytes;
	  bytesLeft -= cbytes;
       }
       else
	  NWFSPrint("nwfs:  suballoc write was index(%d) < vindex(%d) (0)\n",
		    (int)index, (int)vindex);

       return bytesWritten;
    }

    FAT = GetFatEntryAndLRU(volume, FATChain, &lru, &FAT_S);
    if (FAT)
    {
       if (Context)
	  *Context = FATChain;
       index = FAT->FATIndex;
    }

    while (FAT && FAT->FATCluster && (bytesLeft > 0))
    {
       //  if we found a hole, then allocate and add a new cluster
       //  to the file and continue to add clusters and write until
       //  bytesLeft is < 0 or we find the next valid cluster in the
       //  fat chain

       while ((bytesLeft > 0) && (vindex < index))
       {
	  // we can only get here if we detected the next
	  // fat element is greater than the target index
	  // (the file has holes, and we hit an index
	  // larger than we expected).

	  // we simply extend the file by allocating clusters
	  // until we complete the write or the target index
	  // equals the current cluster.  obvioulsy, we must
	  // insert nodes into the fat chain for each element we
	  // allocate.

	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

	  // allocate cluster and point forward link to next cluster
	  NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, FATChain);
	  if (NewCluster == -1)
	  {
	     // if we could not get a free cluster, then return
	     // (out of drive space)
	     if (retCode)
		*retCode = NwVolumeFull;
	     return bytesWritten;
	  }

#if (ZERO_FILL_SECTORS)
	  // zero fill the new cluster
	  ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	  // set previous cluster chain to point to this entry
	  // if PrevCluster is not equal to -1, then we are
	  // inserting at the front of the cluster chain
	  // so save the new directory chain head.

	  if (PrevCluster == (ULONG) -1)
	     *Chain = NewCluster;
	  else
	     SetClusterValue(volume, PrevCluster, NewCluster);

	  // update previous cluster to new cluster
	  PrevCluster = NewCluster;

          if (buf)
	     cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
					  as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
          else
             cbytes = vsize;

	  bytesWritten += cbytes;
	  bytesLeft -= cbytes;
	  buf += cbytes;

	  // update context pointer with adjusted offset
	  if (Context)
	     *Context = NewCluster;
	  if (Index)
	     *Index = vindex;
	  vindex++;
       }

       // found our index block, perform the copy operation

       if ((bytesLeft > 0) && (vindex == index))
       {
	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

          if (buf)
	     cbytes = WriteClusterWithOffset(volume, FATChain, voffset, buf, vsize,
					  as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
          else
             cbytes = vsize;

	  bytesWritten += cbytes;
	  bytesLeft -= cbytes;
	  buf += cbytes;

	  // update context pointer with adjusted offset
	  if (Context)
	     *Context = FATChain;
	  if (Index)
	     *Index = vindex;
	  vindex++;
       }

       // save the previous cluster
       PrevCluster = FATChain;

       // bump to the next cluster
       FATChain = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF
       if ((bytesLeft > 0) && (FATChain & 0x80000000))
       {
	  // end of file
	  if (FATChain == (ULONG) -1)
	  {
	     while (bytesLeft > 0)
	     {
		voffset = 0;
		if (vindex == StartIndex)
		   voffset = StartOffset;

		vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
			? (volume->ClusterSize - voffset) : bytesLeft;

		// allocate cluster and point forward link to EOF
		NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
		if (NewCluster == -1)
		{
		   // if we could not get a free cluster, then return
		   // (out of drive space)
		   if (retCode)
		      *retCode = NwVolumeFull;
		   return bytesWritten;
		}

#if (ZERO_FILL_SECTORS)
		// zero fill the new cluster
		ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

		// set previous cluster chain to point to this entry
		if (PrevCluster == (ULONG) -1)
		   *Chain = NewCluster;
		else
		   SetClusterValue(volume, PrevCluster, NewCluster);

		// update previous cluster to new cluster
		// this will force inserts after the end of this cluster
		PrevCluster = NewCluster;

                if (buf)
		   cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
						as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
                else
                   cbytes = vsize;

		bytesWritten += cbytes;
		bytesLeft -= cbytes;
		buf += cbytes;

		// update context pointer with adjusted offset
		if (Context)
		   *Context = NewCluster;
		if (Index)
		   *Index = vindex;
		vindex++;
	     }
	     return bytesWritten;
	  }

	  // we have detected a suballoc element in the fat chain if we
	  // get to this point

	  // if we got here, then we detected a suballocation element.  See
	  // if this is legal for this file.
	  if ((!SAFlag) || (Attributes & NO_SUBALLOC) || 
	     (Attributes & TRANSACTION) || (Flags & RAW_FILE))
	  {
	     if (retCode)
		*retCode = NwNotPermitted;
	     return bytesWritten;
	  }

	  SuballocSize = GetSuballocSize(volume, FATChain);

	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  // this case assumes we will free the current suballoc element
	  // because our write either exceeds the current suballoc size
	  // or we are writing our starting index beyond the suballocation
	  // element itself in the chain.  In either case, we must free
	  // the current suballocation element.

	  if (vindex > (index + 1))
	  {
#if (VERBOSE)
	     NWFSPrint("nwfs:  we were asked to write to the next index\n");
#endif
	     // convert the suballoc record into an allocated cluster
	     // and copy the data from the suballoc record.

	     WorkSpace = AllocateWorkspace(volume);
	     if (!WorkSpace)
	     {
		// if we could not get memory to copy the suballoc record,
		// then return (out of drive space)
		if (retCode)
		   *retCode = NwInsufficientResources;
		return bytesWritten;
	     }

	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		     ? (volume->ClusterSize - voffset) : bytesLeft;

	     // allocate cluster and point forward link to EOF
	     NewCluster = AllocateClusterSetIndexSetChain(volume, (index + 1),
							  -1);
	     if (NewCluster == -1)
	     {
		// if we could not get a free cluster, then return
		// (out of drive space)
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwVolumeFull;
		return bytesWritten;
	     }

#if (ZERO_FILL_SECTORS)
	     // zero fill the new cluster
	     ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	     // here we read the previous data from the suballoc element
	     cbytes = ReadSuballocRecord(volume, 0, FATChain, &WorkSpace->Buffer[0],
					 SuballocSize, KERNEL_ADDRESS_SPACE,
					 retCode);
	     if (cbytes != SuballocSize)
	     {
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwDiskIoError;
		return bytesWritten;
	     }

	     // now write the previous data from the suballoc element
	     // into the newly allocated cluster.
	     cbytes = WriteClusterWithOffset(volume, NewCluster, 0, &WorkSpace->Buffer[0], SuballocSize,
					  KERNEL_ADDRESS_SPACE, retCode,
					  ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	     if (cbytes != SuballocSize)
	     {
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwDiskIoError;
		return bytesWritten;
	     }

	     // free the suballoc element in bit block list
	     FreeSuballocRecord(volume, FATChain);

	     // set previous cluster chain to point to this entry
	     if (PrevCluster == (ULONG) -1)
		*Chain = NewCluster;
	     else
		SetClusterValue(volume, PrevCluster, NewCluster);

	     // update previous cluster to new cluster
	     // this will force inserts after the end of this cluster
	     PrevCluster = NewCluster;

	     // update context pointer with adjusted offset
	     if (Context)
		*Context = NewCluster;
	     if (Index)
		*Index = (index + 1);

	     FreeWorkspace(volume, WorkSpace);

	     while (bytesLeft > 0)
	     {
		voffset = 0;
		if (vindex == StartIndex)
		   voffset = StartOffset;

		vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
			? (volume->ClusterSize - voffset) : bytesLeft;

		// allocate cluster and point forward link to EOF
		NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
		if (NewCluster == -1)
		{
		   // if we could not get a free cluster, then return
		   // (out of drive space)
		   if (retCode)
		      *retCode = NwVolumeFull;
		   return bytesWritten;
		}

#if (ZERO_FILL_SECTORS)
		// zero fill the new cluster
		ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

		// set previous cluster chain to point to this entry
		SetClusterValue(volume, PrevCluster, NewCluster);

		// update previous cluster to new cluster
		// this will force inserts after the end of this cluster
		PrevCluster = NewCluster;

                if (buf)
		   cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
						as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
                else
                   cbytes = vsize;

		bytesWritten += cbytes;
		bytesLeft -= cbytes;
		buf += cbytes;

		// update context pointer with adjusted offset
		if (Context)
		   *Context = NewCluster;
		if (Index)
		   *Index = vindex;
		vindex++;
	     }
	     return bytesWritten;
	  }
	  else
	  if ((vindex == (index + 1)) && ((bytesLeft + voffset) >= SuballocSize))
	  {
#if (VERBOSE)
	     NWFSPrint("nwfs:  we were asked to write beyond the end of the suballoc\n");
#endif
	     WorkSpace = AllocateWorkspace(volume);
	     if (!WorkSpace)
	     {
		// if we could not get memory to copy the suballoc record,
		// then return (out of drive space)
		if (retCode)
		   *retCode = NwInsufficientResources;
		return bytesWritten;
	     }

	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		     ? (volume->ClusterSize - voffset) : bytesLeft;

	     // allocate cluster and point forward link to EOF
	     NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
	     if (NewCluster == -1)
	     {
		// if we could not get a free cluster, then return
		// (out of drive space)
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwVolumeFull;
		return bytesWritten;
	     }

#if (ZERO_FILL_SECTORS)
	     // zero fill the new cluster
	     ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	     // here we read the previous data from the suballoc element
	     cbytes = ReadSuballocRecord(volume, 0, FATChain, &WorkSpace->Buffer[0],
					 SuballocSize, KERNEL_ADDRESS_SPACE,
					 retCode);
	     if (cbytes != SuballocSize)
	     {
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwDiskIoError;
		return bytesWritten;
	     }

	     // now write the previous data from the suballoc element
	     // into the newly allocated cluster.
	     cbytes = WriteClusterWithOffset(volume, NewCluster, 0,
					  &WorkSpace->Buffer[0], SuballocSize,
					  KERNEL_ADDRESS_SPACE, retCode,
					  ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	     if (cbytes != SuballocSize)
	     {
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwDiskIoError;
		return bytesWritten;
	     }

	     // free the suballoc element in bit block list
	     FreeSuballocRecord(volume, FATChain);

	     // set previous cluster chain to point to this entry
	     if (PrevCluster == (ULONG) -1)
		*Chain = NewCluster;
	     else
		SetClusterValue(volume, PrevCluster, NewCluster);

	     // update previous cluster to new cluster
	     // this will force inserts after the end of this cluster
	     PrevCluster = NewCluster;

             if (buf)
	        cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
					     as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
             else
                cbytes = vsize;

	     bytesWritten += cbytes;
	     bytesLeft -= cbytes;
	     buf += cbytes;

	     // update context pointer with adjusted offset
	     if (Context)
		*Context = NewCluster;
	     if (Index)
		*Index = vindex;
	     vindex++;

	     FreeWorkspace(volume, WorkSpace);

	     while (bytesLeft > 0)
	     {
		voffset = 0;
		if (vindex == StartIndex)
		   voffset = StartOffset;

		vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
			? (volume->ClusterSize - voffset) : bytesLeft;

		// allocate cluster and point forward link to EOF
		NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
		if (NewCluster == -1)
		{
		   // if we could not get a free cluster, then return
		   // (out of drive space)
		   if (retCode)
		      *retCode = NwVolumeFull;
		   return bytesWritten;
		}

#if (ZERO_FILL_SECTORS)
		// zero fill the new cluster
		ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

		// set previous cluster chain to point to this entry
		SetClusterValue(volume, PrevCluster, NewCluster);

		// update previous cluster to new cluster
		// this will force inserts after the end of this cluster
		PrevCluster = NewCluster;

                if (buf)
		   cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
						as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
                else
                   cbytes = vsize;

		bytesWritten += cbytes;
		bytesLeft -= cbytes;
		buf += cbytes;

		// update context pointer with adjusted offset
		if (Context)
		   *Context = NewCluster;
		if (Index)
		   *Index = vindex;
		vindex++;
	     }
	     return bytesWritten;
	  }
	  else
	  if (vindex == (index + 1))
	  {
#if (VERBOSE)
	     NWFSPrint("nwfs:  we were asked to write the current suballoc\n");
#endif
	     // for this case, since our target write size fits within
	     // the previously allocated suballoc element, then just
	     // write the data.

	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     // at this point, bytesLeft is either equal to or
	     // less than the size of the current suballocation
	     // record.

	     vsize = bytesLeft;

             if (buf)
	        cbytes = WriteSuballocRecord(volume, voffset, FATChain,
					  buf, vsize, as, retCode);
             else
                cbytes = vsize;

	     bytesWritten += cbytes;
	     bytesLeft -= cbytes;
	  }
	  else
	     NWFSPrint("nwfs:  suballoc write was index(%d) < vindex(%d) (0)\n",
		       (int)index, (int)vindex);

	  return bytesWritten;
       }

       // get next fat table entry and index
       FAT = GetFatEntryAndLRU(volume, FATChain, &lru, &FAT_S);
       if (FAT)
       {
	  if (Context)
	     *Context = FATChain;
	  index = FAT->FATIndex;
	  if (Index)
	     *Index = vindex;
       }

       // if the fat chain terminates, then exit
       if (!FAT)
	  return bytesWritten;

       lcount++;
    }
    return bytesWritten;

}

#endif


#if (WINDOWS_NT | WINDOWS_NT_RO)

ULONG TrgDebugTrans = 0;

VOID NwReportDiskIoError(
    IN void *PartitionPointer)
{
	nwvp_nt_partition_shutdown((ULONG) PartitionPointer);
}

extern	BOOLEAN NwIsPartitionBad(
	void	*PartitionPointer);

NW_STATUS TrgGetPartitionInfoFromVolumeInfo(IN   ULONG VolumeNumber,
					    IN   ULONG SectorOffsetInVolume,
					    IN	 ULONG SectorCount,
					    OUT  void **PartitionPointer,
					    OUT  ULONG *SectorOffsetInPartition,
					    OUT  ULONG *SectorsReturned)
{
	register VOLUME *volume = VolumeTable[VolumeNumber];
	ULONG	i, j;
//	ULONG	partition_offset;
//	ULONG	disk_id;
	ULONG	bad_bits;
	ULONG	count;
	ULONG	relative_offset;
	ULONG	relative_block;
	ULONG	relative_count;
	ULONG	block_number;
	nwvp_lpart	*lpart;
	nwvp_lpart	*original_part;
	nwvp_vpartition	*vpartition;


	if ((volume->nwvp_handle & 0x0000FFFF) < virtual_partition_table_count)
	{
	    if (((vpartition = virtual_partition_table[volume->nwvp_handle & 0x0000FFFF]) != 0) &&
		(vpartition->vpartition_handle == volume->nwvp_handle))
	    {
		if (vpartition->open_flag != 0)
		{
		    relative_block = SectorOffsetInVolume / 8;
		    relative_offset = SectorOffsetInVolume % 8;
		    relative_count = (relative_offset + SectorCount + 7) / 8;
		    for (i=0; i<vpartition->segment_count; i++)
		    {
			if (relative_block < vpartition->segments[i].segment_block_count)
			{
				if ((vpartition->segments[i].segment_block_count - relative_block) < relative_count)
					relative_count = vpartition->segments[i].segment_block_count - relative_block;

				block_number = relative_block + vpartition->segments[i].partition_block_offset;
				original_part = vpartition->segments[i].lpart_link;
				count = (original_part->read_round_robin ++) & 0x3;
				for (j=0; j< count; j++)
				original_part = original_part->mirror_link;

				lpart = original_part;
				do
				{
				if (((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0) &&
					((lpart->mirror_status & MIRROR_INSYNCH_BIT) != 0))
				{
					for (j=0; j<relative_count; j++)
					{
						if (lpart->hotfix.bit_table[(block_number + j) >> 15][((block_number + j) >> 5) & 0x3FF] & (1 << ((block_number + j) & 0x1F)))
							break;
					}
					if (j == relative_count)
					{
						if (NwIsPartitionBad((void *) lpart->low_level_handle) == 0)
						{
//						nwvp_part_map_to_physical(lpart->rpart_handle, &disk_id, &partition_offset);
							*SectorOffsetInPartition = ((block_number + lpart->hotfix.block_count) * 8) + relative_offset;
							*SectorsReturned = (relative_count * 8) - relative_offset;
							*PartitionPointer = (void *) lpart->low_level_handle;
							return(0);
						}
					}
				}
				lpart = lpart->mirror_link;
				} while (lpart != original_part);

//
//	there are one or more hotfixed blocks in this region
//

				do
				{
				if (((lpart->mirror_status & MIRROR_PRESENT_BIT) != 0) &&
					((lpart->mirror_status & MIRROR_INSYNCH_BIT) != 0))
				{
					if (NwIsPartitionBad((void *) lpart->low_level_handle) == 0)
					{
						for (j=0; j<relative_count; j++)
						{
						if (lpart->hotfix.bit_table[(block_number + j) >> 15][((block_number + j) >> 5) & 0x3FF] & (1 << ((block_number + j) & 0x1F)))
						{
							if (j == 0)
							{
							nwvp_hotfix_block_lookup(lpart, block_number, &relative_block, &bad_bits);
//						nwvp_part_map_to_physical(lpart->rpart_handle, &disk_id, &partition_offset);
							*SectorOffsetInPartition = (relative_block * 8) + relative_offset;
							*SectorsReturned = 8 - relative_offset;
							*PartitionPointer = (void *) lpart->low_level_handle;
							return((bad_bits == 0) ? 0 : -1);
							}
							else
							{
//							nwvp_part_map_to_physical(lpart->rpart_handle, &disk_id, &partition_offset);
							*SectorOffsetInPartition = ((block_number + lpart->hotfix.block_count) * 8) + relative_offset;
							*SectorsReturned = (j * 8) - relative_offset;
							*PartitionPointer = (void *) lpart->low_level_handle;
							return(0);
							}
						}
						}
					}
				}
				lpart = lpart->mirror_link;
				} while (lpart != original_part);

//
//	the code should never reach this point
//
				return NwDiskIoError;

			}
			relative_block -= vpartition->segments[i].segment_block_count;
			}
		}
		}
	}
	return NwInvalidParameter;
}

NW_STATUS TrgLookupFileAllocation(IN   ULONG VolumeNumber,
				  IN   HASH *hash,
				  IN   ULONG *StartingSector,
				  IN   ULONG *Context,
				  OUT  ULONG *SectorOffsetInVolume,
				  OUT  ULONG *SectorCount)
{
    register ULONG StreamSize, StreamOffset, SpanningClusters;
    register VOLUME *volume = VolumeTable[VolumeNumber];
    register long FATChain, index;
    register ULONG StartIndex, StartOffset, rc;
    register FAT_ENTRY *FAT, *LFAT;
    FAT_ENTRY FAT_S, LFAT_S;
    SUBALLOC_MAP map;

    // if a subdirectory then return error
    if (!hash || (hash->Flags & SUBDIRECTORY_FILE))
       return NwInvalidParameter;

    // get the proper namespace stream cluster and size
    switch (hash->NameSpace)
    {
       case DOS_NAME_SPACE:
       case MAC_NAME_SPACE:
	  FATChain = hash->FirstBlock;
	  StreamSize = hash->FileSize;
	  break;

       default:
	  return NwInvalidParameter;
    }

    // calculate a size offset and range check for EOF
    StreamOffset = ((*StartingSector) * 512);

    // make certain to round the file size up to the next sector boundry
    if (StreamOffset > ((StreamSize + 511) & ~511))
       return NwEndOfFile;

    StartIndex = StreamOffset / volume->ClusterSize;
    StartOffset = StreamOffset % volume->ClusterSize;

    // we always start with an index of zero
    index = 0;

    if (FATChain & 0x80000000)
    {
       // check for EOF
       if (FATChain == (ULONG) -1)
       {
	  *Context = 0;
	  return NwEndOfFile;
       }

       // index must be null here
       if (StartIndex)
       {
	  *Context = 0;
	  return NwEndOfFile;
       }

       rc = MapSuballocNode(volume, &map, FATChain);
       if (rc)
       {
	  *Context = 0;
	  return NwFileCorrupt;
       }

       if (StartOffset >= map.Size)
       {
	  *Context = 0;
	  return NwEndOfFile;
       }

       if (StartOffset >= map.clusterSize[0])
       {
	  if (map.Count == 1)
	  {
	     *Context = 0;
	     return NwEndOfFile;
	  }
	  *StartingSector = (StartIndex * volume->SectorsPerCluster) +
			       (map.clusterSize[0] / 512);
	  *SectorCount = (map.clusterSize[1] / 512);
	  *SectorOffsetInVolume = (map.clusterNumber[1] * volume->SectorsPerCluster) +
				     (map.clusterOffset[1] / 512);
	  *Context = 0;
	  return NwSuccess;
       }
       else
       {
	  *StartingSector = (StartIndex * volume->SectorsPerCluster);
	  *SectorCount = (map.clusterSize[0] / 512);
	  *SectorOffsetInVolume = (map.clusterNumber[0] * volume->SectorsPerCluster) +
				 (map.clusterOffset[0] / 512);
	  *Context = 0;
	  return NwSuccess;
       }
    }

    FAT = GetFatEntry(volume, FATChain, &FAT_S);
    if (FAT)
    {
       *Context = FATChain;
       index = FAT->FATIndex;
    }

    while (FAT && FAT->FATCluster)
    {
       // we detected a hole in the file
       if (StartIndex < index)
       {
	  *StartingSector = (StartIndex * volume->SectorsPerCluster);
	  *SectorOffsetInVolume = 0;

	  // return the full size of the hole as a single run
	  *SectorCount = ((index - StartIndex) * volume->SectorsPerCluster);
	  *Context = FATChain;  // point to this cluster
	  return NwSuccess;
       }

       // we found our cluster in the chain
       if (StartIndex == index)
       {
	  *StartingSector = (StartIndex * volume->SectorsPerCluster);
	  *SectorOffsetInVolume = (FATChain * volume->SectorsPerCluster);

	  // see how far this span runs continguous by checking both
	  // the index and the cluster number of this fat entry.
	  
	  if (TrgDebugTrans)	
	  NWFSPrint("clstr-%X index-%d nclstr-%X nindex-%d\n",
		    (unsigned int) FATChain,
		    (int) index,
		    (unsigned int) FAT->FATCluster,
		    (int) FAT->FATIndex);

	  for (SpanningClusters = 0;
	       FAT && FAT->FATCluster &&
	       (FAT->FATCluster == (FATChain + 1)) &&
	       (FAT->FATCluster > 0) && FATChain > 0; )
	  {
	     // check if next index number is also in sequence
	     LFAT = GetFatEntry(volume, FAT->FATCluster, &LFAT_S);
	     if (LFAT)
	     {
		if (LFAT->FATIndex != (index + 1))
		   break;
	     }

	     SpanningClusters++;
	     FATChain = FAT->FATCluster;

	     if (FATChain < 0)
		break;

	     // get next fat table entry and index
	     FAT = GetFatEntry(volume, FATChain, &FAT_S);
	     if (FAT)
		index = FAT->FATIndex;
	  }

	  //  return size of run plus any contiguous clusters we detected
	  *SectorCount = volume->SectorsPerCluster +
			 (SpanningClusters * volume->SectorsPerCluster);

	  *Context = FAT->FATCluster;  // point to next cluster
	  return NwSuccess;
       }

       // bump to the next cluster
       FATChain = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF marker
       if (FATChain & 0x80000000)
       {
	  // end of file
	  if (FATChain == (ULONG) -1)
	  {
	     *Context = 0;
	     return NwEndOfFile;
	  }

	  // check for valid index
	  if ((index + 1) == StartIndex)
	  {
	     rc = MapSuballocNode(volume, &map, FATChain);
	     if (rc)
	     {
		*Context = 0;
		return NwFileCorrupt;
	     }

	     if (StartOffset >= map.Size)
	     {
		*Context = 0;
		return NwEndOfFile;
	     }

	     if (StartOffset >= map.clusterSize[0])
	     {
		if (map.Count == 1)
		{
		   *Context = 0;
		   return NwEndOfFile;
		}

		*StartingSector = (StartIndex * volume->SectorsPerCluster) +
				  (map.clusterSize[0] / 512);
		*SectorCount = (map.clusterSize[1] / 512);
		*SectorOffsetInVolume = (map.clusterNumber[1] * volume->SectorsPerCluster) +
				     (map.clusterOffset[1] / 512);
		*Context = 0;
		return NwSuccess;
	     }
	     else
	     {
		*StartingSector = (StartIndex * volume->SectorsPerCluster);
		*SectorCount = (map.clusterSize[0] / 512);
		*SectorOffsetInVolume = (map.clusterNumber[0] * volume->SectorsPerCluster) +
				     (map.clusterOffset[0] / 512);
		*Context = 0;
		return NwSuccess;
	     }
	  }
	  return NwEndOfFile;
       }

       // get next fat table entry and index
       FAT = GetFatEntry(volume, FATChain, &FAT_S);
       if (FAT)
       {
	  *Context = FATChain;
	  index = FAT->FATIndex;
       }
    }
    return NwEndOfFile;

}

ULONG NWReadFile(VOLUME *volume, ULONG *Chain, ULONG Flags, ULONG FileSize,
		 ULONG offset, BYTE *buf, long count, long *Context,
		 ULONG *Index, ULONG *retCode, ULONG as, ULONG SAFlag,
		 ULONG Attributes, ULONG readAhead)
{
    register long PrevContext = 0;
    register ULONG PrevIndex = 0;
    register ULONG FATChain, index;
    register long bytesRead = 0, bytesLeft = 0;
    register ULONG StartIndex, StartOffset;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    register ULONG voffset, vsize, vindex, cbytes;

    if (!Chain)
       return 0;

    if (!(*Chain))
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  fat chain was NULL (read)\n");
#endif
       return 0;
    }

    if (retCode)
       *retCode = 0;

    // adjust size and range check for EOF

    if ((offset + count) > FileSize)
       count = FileSize - offset;

    if (count <= 0)
       return 0;

    // if a subdirectory then return 0
    if (Flags & SUBDIRECTORY_FILE)
    {
       if (retCode)
	  *retCode = NwInvalidParameter;
       return 0;
    }

    bytesLeft = count;
    StartIndex = offset / volume->ClusterSize;
    StartOffset = offset % volume->ClusterSize;

    // we always start with an index of zero
    index = 0;
    FATChain = *Chain;
    vindex = StartIndex;

    // see if we are starting on or after the supplied index/cluster pointer.
    // if (StartIndex < *Index), then we are being asked to start before
    // the user supplied context.  In this case, start at the beginning of
    // the chain.

    if (Index && (StartIndex >= (*Index)))
    {
       if (Context && (*Context))
       {
	  PrevContext = FATChain = *Context;
	  PrevIndex = index = *Index;
       }
    }

    if ((bytesLeft > 0) && (FATChain & 0x80000000))
    {
       // check for EOF
       if (FATChain == (ULONG) -1)
       {
	  // filesize may exceed allocation, which means the rest of
	  // the file is sparse.  fill zeros into the requested
	  // size.  bytesLeft will have been set by count, which is
	  // range checked to the actual length of the file.

	  if (bytesLeft > 0)
	  {
             if (buf)
     	     {
		if (NWFSSetUserSpace(buf, 0, bytesLeft))
		{
                   if (retCode)
		      *retCode = NwMemoryFault;
     		   return bytesRead; 
	        }
	     }
	     bytesRead += bytesLeft;
	  }
	  if (Context)
	     *Context = PrevContext;
	  if (Index)
	     *Index = PrevIndex;
	  return bytesRead;
       }

       // if we got here, then we detected a suballocation element.  See
       // if this is legal for this file.
       if ((!SAFlag) || (Attributes & NO_SUBALLOC) || 
	   (Attributes & TRANSACTION) || (Flags & RAW_FILE))
       {
	  if (retCode)
	     *retCode = NwNotPermitted;
	  return bytesRead;
       }

       // check for valid index
       if (index == vindex)
       {
	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  // if this value exceeds the suballoc record size,
	  // ReadSuballoc record will reduce this size to the
	  // current record allocation.
	  vsize = bytesLeft;

	  cbytes = ReadSuballocRecord(volume, voffset, FATChain, buf,
				      vsize, as, retCode);
	  bytesRead += cbytes;
	  bytesLeft -= cbytes;
	  buf += cbytes;

	  if (Context)
	     *Context = FATChain;
	  if (Index)
	     *Index = index;
       }

       // filesize may exceed allocation, which means the rest of
       // the file is sparse.  fill zeros into the requested
       // size.

       if (bytesLeft > 0)
       {
          if (buf)
          {
	     if (NWFSSetUserSpace(buf, 0, bytesLeft))
	     {
                if (retCode)
		   *retCode = NwMemoryFault;
     		return bytesRead;
	     }
	  }
	  bytesRead += bytesLeft;
       }
       return bytesRead;
    }

    vindex = StartIndex;
    FAT = GetFatEntry(volume, FATChain, &FAT_S);
    if (FAT)
    {
       if (Context)
	  *Context = FATChain;
       index = FAT->FATIndex;
       if (Index)
	  *Index = index;
    }

    while (FAT && FAT->FATCluster && (bytesLeft > 0))
    {
       //  if we found a hole, then return zeros until we
       //  either satisfy the requested read size or
       //  we span to the next valid index entry

       while ((bytesLeft > 0) && (vindex < index))
       {
	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

          if (buf)
	  {
	     if (NWFSSetUserSpace(buf, 0, vsize))
	     {
                if (retCode)
		   *retCode = NwMemoryFault;
     		return bytesRead;
	     }
	  }
	  bytesRead += vsize;
	  bytesLeft -= vsize;
	  buf += vsize;
	  vindex++;
       }

       // found our index block, perform the copy operation

       if ((bytesLeft > 0) && (vindex == index))
       {
	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

	  cbytes = ReadClusterWithOffset(volume, FATChain, voffset, buf, vsize,
					 as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));

#if (READ_AHEAD_ON)
	  // do read ahead on the next valid cluster
	  if (((FAT->FATCluster & 0x80000000) == 0) && (readAhead) && 
	       (!(Flags & RAW_FILE)))
	  {
             ULONG mirror = (ULONG)-1; 
     
	     PerformBlockReadAhead(&DataLRU, volume,
				  (FAT->FATCluster * volume->BlocksPerCluster),
				   volume->BlocksPerCluster, &mirror);
	  }
#endif

	  bytesRead += cbytes;
	  bytesLeft -= cbytes;
	  buf += cbytes;
	  vindex++;
       }

       // bump to the next cluster
       PrevContext = FATChain;
       PrevIndex = index;
       FATChain = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF marker
       if ((bytesLeft > 0) && (FATChain & 0x80000000))
       {
	  // end of file
	  if (FATChain == (ULONG) -1)
	  {
	     // filesize may exceed allocation, which means the rest of
	     // the file is sparse.  fill zeros into the requested
	     // size.
	     if (bytesLeft > 0)
	     {
                if (buf)
     	        {
	           if (NWFSSetUserSpace(buf, 0, bytesLeft))
		   {
                      if (retCode)
		         *retCode = NwMemoryFault;
     		      return bytesRead;
		   } 
	        }
		bytesRead += bytesLeft;
	     }
	     if (Context)
		*Context = PrevContext;
	     if (Index)
		*Index = PrevIndex;
	     return bytesRead;
	  }

	  // if we got here, then we detected a suballocation element.  See
	  // if this is legal for this file.

	  if ((!SAFlag) || (Attributes & NO_SUBALLOC) || 
	      (Attributes & TRANSACTION) || (Flags & RAW_FILE))
	  {
	     if (retCode)
		*retCode = NwNotPermitted;
	     return bytesRead;
	  }

	  // check for valid index
	  if ((index + 1) == vindex)
	  {
	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     // if this value exceeds the suballoc record size,
	     // ReadSuballoc record will reduce this size to the
	     // current record allocation.
	     vsize = bytesLeft;

	     cbytes = ReadSuballocRecord(volume, voffset, FATChain, buf,
					 vsize, as, retCode);
	     bytesRead += cbytes;
	     bytesLeft -= cbytes;
	     buf += cbytes;

	     if (Context)
		*Context = FATChain;
	     if (Index)
		*Index = (index + 1);
	  }

	  // filesize may exceed allocation, which means the rest of
	  // the file is sparse.  fill zeros into the requested
	  // size.
	  if (bytesLeft > 0)
	  {
             if (buf)
     	     {
	        if (NWFSSetUserSpace(buf, 0, bytesLeft))
		{
                   if (retCode)
		      *retCode = NwMemoryFault;
     		   return bytesRead;
		}
	     }
	     bytesRead += bytesLeft;
	  }
	  return bytesRead;
       }

       // get next fat table entry and index
       FAT = GetFatEntry(volume, FATChain, &FAT_S);
       if (FAT)
       {
	  if (Context)
	     *Context = FATChain;
	  index = FAT->FATIndex;
	  if (Index)
	     *Index = index;
       }
    }

    // filesize may exceed allocation, which means the rest of
    // the file is sparse.  fill zeros into the requested
    // size.
    if (bytesLeft > 0)
    {
       if (buf)
       {
	  if (NWFSSetUserSpace(buf, 0, bytesLeft))
	  {
             if (retCode)
	        *retCode = NwMemoryFault;
     	     return bytesRead;
	  }
       }
       bytesRead += bytesLeft;
    }
    return bytesRead;

}

ULONG NWWriteFile(VOLUME *volume, ULONG *Chain, ULONG Flags,
		  ULONG offset, BYTE *buf, long count, long *Context,
		  ULONG *Index, ULONG *retCode, ULONG as, ULONG SAFlag,
		  ULONG Attributes)
{
    register long PrevContext = 0;
    register ULONG PrevIndex = 0;
    register ULONG FATChain, index;
    register long bytesWritten = 0, bytesLeft = 0;
    register ULONG lcount = 0;
    register ULONG StartIndex, StartOffset, SuballocSize;
    register FAT_ENTRY *FAT;
    FAT_ENTRY FAT_S;
    register ULONG voffset, vsize, vindex, cbytes;
    register ULONG PrevCluster, NewCluster;
    register VOLUME_WORKSPACE *WorkSpace;
    MIRROR_LRU *lru = 0;

    if (!Chain)
       return 0;

    if (!(*Chain))
    {
       NWFSPrint("nwfs:  fat chain was NULL (write)\n");
       return 0;
    }

    if (retCode)
       *retCode = 0;

    // if a subdirectory then return 0
    if (Flags & SUBDIRECTORY_FILE)
    {
       if (retCode)
	  *retCode = NwInvalidParameter;
       return 0;
    }

    bytesLeft = count;
    StartIndex = offset / volume->ClusterSize;
    StartOffset = offset % volume->ClusterSize;

    // we always start with an index of zero
    index = 0;
    PrevCluster = (ULONG) -1;
    FATChain = *Chain;
    vindex = StartIndex;

#if (VERBOSE)
    NWFSPrint("nwfs_write:  count-%d offset-%d ctx-%08X ndx-%08X chn-%08X\n",
	      (int)count, (int)offset,
	      (unsigned int) (Context ? *Context : 0),
	      (unsigned int) (Index ? *Index : 0),
	      (unsigned int) *Chain);
#endif

    // see if we are starting on or after the supplied index/cluster pointer.
    // if (StartIndex < *Index), then we are being asked to start before
    // the user supplied context.  In this case, start at the beginning of
    // the chain.

    if (Index && (StartIndex >= (*Index)))
    {
       if (Context && (*Context))
       {
	  PrevContext = FATChain = *Context;
	  PrevIndex = index = *Index;
       }
    }

    if ((bytesLeft > 0) && (FATChain & 0x80000000))
    {
       // check for EOF
       if (FATChain == (ULONG) -1)
       {
	  while (bytesLeft > 0)
	  {
	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		     ? (volume->ClusterSize - voffset) : bytesLeft;

	     // allocate cluster and point forward link to EOF
	     NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
	     if (NewCluster == -1)
	     {
		// if we could not get a free cluster, then return
		// (out of drive space)
		if (retCode)
		   *retCode = NwVolumeFull;

		return bytesWritten;
	     }

#if (ZERO_FILL_SECTORS)
	     // zero fill the new cluster
	     ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	     // set previous cluster chain to point to this entry
	     if (PrevCluster == (ULONG) -1)
		*Chain = NewCluster;
	     else
		SetClusterValue(volume, PrevCluster, NewCluster);

	     // update previous cluster to new cluster
	     // this will force inserts after the end of this cluster
	     PrevCluster = NewCluster;

	     cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
					     as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	     bytesWritten += cbytes;
	     bytesLeft -= cbytes;
	     buf += cbytes;

	     // update context pointer with adjusted offset
	     if (Context)
		*Context = NewCluster;
	     if (Index)
		*Index = vindex;
	     vindex++;
	  }
	  return bytesWritten;
       }

       // we have detected a suballoc element in the fat chain if we
       // get to this point

       // if we got here, then we detected a suballocation element.  See
       // if this is legal for this file.
       if ((!SAFlag) || (Attributes & NO_SUBALLOC) || 
	   (Attributes & TRANSACTION) || (Flags & RAW_FILE))
       {
	  if (retCode)
	     *retCode = NwNotPermitted;
	  return bytesWritten;
       }

       SuballocSize = GetSuballocSize(volume, FATChain);
       voffset = 0;
       if (vindex == StartIndex)
	  voffset = StartOffset;

       // these cases assume we will free the current suballoc element
       // because our write either exceeds the current suballoc size
       // or we are writing our starting index beyond the suballocation
       // element itself in the chain.  In either case, we must free
       // the current suballocation element.

       // if we are asked to write beyond the current suballoc element
       if (vindex > index)
       {
#if (VERBOSE)
	  NWFSPrint("nwfs:  we were asked to write to the next index\n");
#endif
	  // convert the suballoc record into an allocated cluster
	  // and copy the data from the suballoc record.

	  WorkSpace = AllocateWorkspace(volume);
	  if (!WorkSpace)
	  {
	     // if we could not get memory to copy the suballoc record,
	     // then return (out of drive space)
	     if (retCode)
		*retCode = NwInsufficientResources;
	     return bytesWritten;
	  }

	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

	  // allocate cluster and point forward link to EOF
	  NewCluster = AllocateClusterSetIndexSetChain(volume, index, -1);
	  if (NewCluster == -1)
	  {
	     // if we could not get a free cluster, then return
	     // (out of drive space)
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwVolumeFull;
	     return bytesWritten;
	  }

#if (ZERO_FILL_SECTORS)
	  // zero fill the new cluster
	  ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	  // here we read the previous data from the suballoc element
	  cbytes = ReadSuballocRecord(volume, 0, FATChain,
				      &WorkSpace->Buffer[0],
				      SuballocSize, KERNEL_ADDRESS_SPACE,
				      retCode);
	  if (cbytes != SuballocSize)
	  {
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwDiskIoError;
	     return bytesWritten;
	  }

	  // now write the previous data from the suballoc element
	  // into the newly allocated cluster.
	  cbytes = WriteClusterWithOffset(volume, NewCluster, 0, &WorkSpace->Buffer[0], SuballocSize,
					  KERNEL_ADDRESS_SPACE, retCode,
					  ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	  if (cbytes != SuballocSize)
	  {
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwDiskIoError;
	     return bytesWritten;
	  }

	  // free the suballoc element in bit block list
	  FreeSuballocRecord(volume, FATChain);

	  // set previous cluster chain to point to this entry
	  if (PrevCluster == (ULONG) -1)
	     *Chain = NewCluster;
	  else
	     SetClusterValue(volume, PrevCluster, NewCluster);

	  PrevCluster = NewCluster;

	  // update context pointer with adjusted offset
	  if (Context)
	     *Context = NewCluster;
	  if (Index)
	     *Index = index;

	  FreeWorkspace(volume, WorkSpace);

	  while (bytesLeft > 0)
	  {
	     voffset = 0;
	     if (vindex == StartIndex)
		 voffset = StartOffset;

	     vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		     ? (volume->ClusterSize - voffset) : bytesLeft;

	     // allocate cluster and point forward link to EOF
	     NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
	     if (NewCluster == -1)
	     {
		// if we could not get a free cluster, then return
		// (out of drive space)
		if (retCode)
		   *retCode = NwVolumeFull;
		return bytesWritten;
	     }

#if (ZERO_FILL_SECTORS)
	     // zero fill the new cluster
	     ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	     // set previous cluster chain to point to this entry
	     SetClusterValue(volume, PrevCluster, NewCluster);

	     // update previous cluster to new cluster
	     // this will force inserts after the end of this cluster
	     PrevCluster = NewCluster;

	     cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
						as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	     bytesWritten += cbytes;
	     bytesLeft -= cbytes;
	     buf += cbytes;

	     // update context pointer with adjusted offset
	     if (Context)
		*Context = NewCluster;
	     if (Index)
		*Index = vindex;
	     vindex++;
	  }
	  return bytesWritten;
       }
       else
       if ((vindex == index) && ((bytesLeft + voffset) >= SuballocSize))
       {
#if (VERBOSE)
	  NWFSPrint("nwfs:  we were asked to write beyond the suballoc\n");
#endif
	  WorkSpace = AllocateWorkspace(volume);
	  if (!WorkSpace)
	  {
	     // if we could not get memory to copy the suballoc record,
	     // then return (out of drive space)
	     if (retCode)
		*retCode = NwInsufficientResources;

	     return bytesWritten;
	  }

	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

	  // allocate cluster and point forward link to EOF
	  NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, (ULONG) -1);
	  if (NewCluster == -1)
	  {
	     // if we could not get a free cluster, then return
	     // (out of drive space)
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwVolumeFull;
	     return bytesWritten;
	  }

#if (ZERO_FILL_SECTORS)
	  // zero fill the new cluster
	  ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	  // here we read the previous data from the suballoc element
	  cbytes = ReadSuballocRecord(volume, 0, FATChain, &WorkSpace->Buffer[0],
				      SuballocSize, KERNEL_ADDRESS_SPACE,
				      retCode);
	  if (cbytes != SuballocSize)
	  {
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwDiskIoError;
	     return bytesWritten;
	  }

	  // now write the previous data from the suballoc element
	  // into the newly allocated cluster.
	  cbytes = WriteClusterWithOffset(volume, NewCluster, 0, &WorkSpace->Buffer[0], SuballocSize,
					  KERNEL_ADDRESS_SPACE, retCode,
					  ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	  if (cbytes != SuballocSize)
	  {
	     FreeWorkspace(volume, WorkSpace);
	     if (retCode)
		*retCode = NwDiskIoError;
	     return bytesWritten;
	  }

	  // free the suballoc element in bit block list
	  FreeSuballocRecord(volume, FATChain);

	  // if we are the first cluster, then save the chain head in
	  // the directory record

	  if (PrevCluster == (ULONG) -1)
	     *Chain = NewCluster;
	  else
	     SetClusterValue(volume, PrevCluster, NewCluster);

	  PrevCluster = NewCluster;

	  // now write the user data into the suballoc element
	  cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
					  as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	  bytesWritten += cbytes;
	  bytesLeft -= cbytes;
	  buf += cbytes;

	  // update context pointer with adjusted offset
	  if (Context)
	     *Context = NewCluster;
	  if (Index)
	     *Index = vindex;
	  vindex++;

	  FreeWorkspace(volume, WorkSpace);

	  while (bytesLeft > 0)
	  {
	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		     ? (volume->ClusterSize - voffset) : bytesLeft;

	     // allocate cluster and point forward link to EOF
	     NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
	     if (NewCluster == -1)
	     {
		// if we could not get a free cluster, then return
		// (out of drive space)
		if (retCode)
		   *retCode = NwVolumeFull;
		return bytesWritten;
	     }

#if (ZERO_FILL_SECTORS)
	     // zero fill the new cluster
	     ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	     // set previous cluster chain to point to this entry
	     SetClusterValue(volume, PrevCluster, NewCluster);

	     // update previous cluster to new cluster
	     // this will force inserts after the end of this cluster
	     PrevCluster = NewCluster;

	     cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
					     as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	     bytesWritten += cbytes;
	     bytesLeft -= cbytes;
	     buf += cbytes;

	     // update context pointer with adjusted offset
	     if (Context)
		*Context = NewCluster;
	     if (Index)
		*Index = vindex;
	     vindex++;
	  }
	  return bytesWritten;
       }
       else
       if (vindex == index)
       {
	  // for this case, since our target write size fits within
	  // the previously allocated suballoc element, then just
	  // write the data.
#if (VERBOSE)
	  NWFSPrint("nwfs:  we were asked to write within the suballoc\n");
#endif
	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  // at this point, bytesLeft is either equal to or
	  // less than the size of the current suballocation
	  // record.

	  vsize = bytesLeft;
	  cbytes = WriteSuballocRecord(volume, voffset, FATChain,
				       buf, vsize, as, retCode);
	  bytesWritten += cbytes;
	  bytesLeft -= cbytes;
       }
       else
	  NWFSPrint("nwfs:  suballoc write was index(%d) < vindex(%d) (0)\n",
		    (int)index, (int)vindex);

       return bytesWritten;
    }

    FAT = GetFatEntryAndLRU(volume, FATChain, &lru, &FAT_S);
    if (FAT)
    {
       if (Context)
	  *Context = FATChain;
       index = FAT->FATIndex;
    }

    while (FAT && FAT->FATCluster && (bytesLeft > 0))
    {
       //  if we found a hole, then allocate and add a new cluster
       //  to the file and continue to add clusters and write until
       //  bytesLeft is < 0 or we find the next valid cluster in the
       //  fat chain

       while ((bytesLeft > 0) && (vindex < index))
       {
	  // we can only get here if we detected the next
	  // fat element is greater than the target index
	  // (the file has holes, and we hit an index
	  // larger than we expected).

	  // we simply extend the file by allocating clusters
	  // until we complete the write or the target index
	  // equals the current cluster.  obvioulsy, we must
	  // insert nodes into the fat chain for each element we
	  // allocate.

	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

	  // allocate cluster and point forward link to next cluster
	  NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, FATChain);
	  if (NewCluster == -1)
	  {
	     // if we could not get a free cluster, then return
	     // (out of drive space)
	     if (retCode)
		*retCode = NwVolumeFull;
	     return bytesWritten;
	  }

#if (ZERO_FILL_SECTORS)
	  // zero fill the new cluster
	  ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	  // set previous cluster chain to point to this entry
	  // if PrevCluster is not equal to -1, then we are
	  // inserting at the front of the cluster chain
	  // so save the new directory chain head.

	  if (PrevCluster == (ULONG) -1)
	     *Chain = NewCluster;
	  else
	     SetClusterValue(volume, PrevCluster, NewCluster);

	  // update previous cluster to new cluster
	  PrevCluster = NewCluster;

	  cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
					  as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	  bytesWritten += cbytes;
	  bytesLeft -= cbytes;
	  buf += cbytes;

	  // update context pointer with adjusted offset
	  if (Context)
	     *Context = NewCluster;
	  if (Index)
	     *Index = vindex;
	  vindex++;
       }

       // found our index block, perform the copy operation

       if ((bytesLeft > 0) && (vindex == index))
       {
	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		  ? (volume->ClusterSize - voffset) : bytesLeft;

	  cbytes = WriteClusterWithOffset(volume, FATChain, voffset, buf, vsize,
					  as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	  bytesWritten += cbytes;
	  bytesLeft -= cbytes;
	  buf += cbytes;

	  // update context pointer with adjusted offset
	  if (Context)
	     *Context = FATChain;
	  if (Index)
	     *Index = vindex;
	  vindex++;
       }

       // save the previous cluster
       PrevCluster = FATChain;

       // bump to the next cluster
       FATChain = FAT->FATCluster;

       // check if the next cluster is a suballoc element or EOF
       if ((bytesLeft > 0) && (FATChain & 0x80000000))
       {
	  // end of file
	  if (FATChain == (ULONG) -1)
	  {
	     while (bytesLeft > 0)
	     {
		voffset = 0;
		if (vindex == StartIndex)
		   voffset = StartOffset;

		vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
			? (volume->ClusterSize - voffset) : bytesLeft;

		// allocate cluster and point forward link to EOF
		NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
		if (NewCluster == -1)
		{
		   // if we could not get a free cluster, then return
		   // (out of drive space)
		   if (retCode)
		      *retCode = NwVolumeFull;
		   return bytesWritten;
		}

#if (ZERO_FILL_SECTORS)
		// zero fill the new cluster
		ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

		// set previous cluster chain to point to this entry
		if (PrevCluster == (ULONG) -1)
		   *Chain = NewCluster;
		else
		   SetClusterValue(volume, PrevCluster, NewCluster);

		// update previous cluster to new cluster
		// this will force inserts after the end of this cluster
		PrevCluster = NewCluster;

		cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
						as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
		bytesWritten += cbytes;
		bytesLeft -= cbytes;
		buf += cbytes;

		// update context pointer with adjusted offset
		if (Context)
		   *Context = NewCluster;
		if (Index)
		   *Index = vindex;
		vindex++;
	     }
	     return bytesWritten;
	  }

	  // we have detected a suballoc element in the fat chain if we
	  // get to this point

	  // if we got here, then we detected a suballocation element.  See
	  // if this is legal for this file.
	  if ((!SAFlag) || (Attributes & NO_SUBALLOC) || 
	      (Attributes & TRANSACTION) || (Flags & RAW_FILE))
	  {
	     if (retCode)
		*retCode = NwNotPermitted;
	     return bytesWritten;
	  }

	  SuballocSize = GetSuballocSize(volume, FATChain);

	  voffset = 0;
	  if (vindex == StartIndex)
	     voffset = StartOffset;

	  // this case assumes we will free the current suballoc element
	  // because our write either exceeds the current suballoc size
	  // or we are writing our starting index beyond the suballocation
	  // element itself in the chain.  In either case, we must free
	  // the current suballocation element.

	  if (vindex > (index + 1))
	  {
#if (VERBOSE)
	     NWFSPrint("nwfs:  we were asked to write to the next index\n");
#endif
	     // convert the suballoc record into an allocated cluster
	     // and copy the data from the suballoc record.

	     WorkSpace = AllocateWorkspace(volume);
	     if (!WorkSpace)
	     {
		// if we could not get memory to copy the suballoc record,
		// then return (out of drive space)
		if (retCode)
		   *retCode = NwInsufficientResources;
		return bytesWritten;
	     }

	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		     ? (volume->ClusterSize - voffset) : bytesLeft;

	     // allocate cluster and point forward link to EOF
	     NewCluster = AllocateClusterSetIndexSetChain(volume, (index + 1),
							  -1);
	     if (NewCluster == -1)
	     {
		// if we could not get a free cluster, then return
		// (out of drive space)
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwVolumeFull;
		return bytesWritten;
	     }

#if (ZERO_FILL_SECTORS)
	     // zero fill the new cluster
	     ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	     // here we read the previous data from the suballoc element
	     cbytes = ReadSuballocRecord(volume, 0, FATChain, &WorkSpace->Buffer[0],
					 SuballocSize, KERNEL_ADDRESS_SPACE,
					 retCode);
	     if (cbytes != SuballocSize)
	     {
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwDiskIoError;
		return bytesWritten;
	     }

	     // now write the previous data from the suballoc element
	     // into the newly allocated cluster.
	     cbytes = WriteClusterWithOffset(volume, NewCluster, 0, &WorkSpace->Buffer[0], SuballocSize,
					  KERNEL_ADDRESS_SPACE, retCode,
					  ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	     if (cbytes != SuballocSize)
	     {
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwDiskIoError;
		return bytesWritten;
	     }

	     // free the suballoc element in bit block list
	     FreeSuballocRecord(volume, FATChain);

	     // set previous cluster chain to point to this entry
	     if (PrevCluster == (ULONG) -1)
		*Chain = NewCluster;
	     else
		SetClusterValue(volume, PrevCluster, NewCluster);

	     // update previous cluster to new cluster
	     // this will force inserts after the end of this cluster
	     PrevCluster = NewCluster;

	     // update context pointer with adjusted offset
	     if (Context)
		*Context = NewCluster;
	     if (Index)
		*Index = (index + 1);

	     FreeWorkspace(volume, WorkSpace);

	     while (bytesLeft > 0)
	     {
		voffset = 0;
		if (vindex == StartIndex)
		   voffset = StartOffset;

		vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
			? (volume->ClusterSize - voffset) : bytesLeft;

		// allocate cluster and point forward link to EOF
		NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
		if (NewCluster == -1)
		{
		   // if we could not get a free cluster, then return
		   // (out of drive space)
		   if (retCode)
		      *retCode = NwVolumeFull;
		   return bytesWritten;
		}

#if (ZERO_FILL_SECTORS)
		// zero fill the new cluster
		ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

		// set previous cluster chain to point to this entry
		SetClusterValue(volume, PrevCluster, NewCluster);

		// update previous cluster to new cluster
		// this will force inserts after the end of this cluster
		PrevCluster = NewCluster;

		cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
						as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
		bytesWritten += cbytes;
		bytesLeft -= cbytes;
		buf += cbytes;

		// update context pointer with adjusted offset
		if (Context)
		   *Context = NewCluster;
		if (Index)
		   *Index = vindex;
		vindex++;
	     }
	     return bytesWritten;
	  }
	  else
	  if ((vindex == (index + 1)) && ((bytesLeft + voffset) >= SuballocSize))
	  {
#if (VERBOSE)
	     NWFSPrint("nwfs:  we were asked to write beyond the end of the suballoc\n");
#endif
	     WorkSpace = AllocateWorkspace(volume);
	     if (!WorkSpace)
	     {
		// if we could not get memory to copy the suballoc record,
		// then return (out of drive space)
		if (retCode)
		   *retCode = NwInsufficientResources;
		return bytesWritten;
	     }

	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
		     ? (volume->ClusterSize - voffset) : bytesLeft;

	     // allocate cluster and point forward link to EOF
	     NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
	     if (NewCluster == -1)
	     {
		// if we could not get a free cluster, then return
		// (out of drive space)
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwVolumeFull;
		return bytesWritten;
	     }

#if (ZERO_FILL_SECTORS)
	     // zero fill the new cluster
	     ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

	     // here we read the previous data from the suballoc element
	     cbytes = ReadSuballocRecord(volume, 0, FATChain, &WorkSpace->Buffer[0],
					 SuballocSize, KERNEL_ADDRESS_SPACE,
					 retCode);
	     if (cbytes != SuballocSize)
	     {
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwDiskIoError;
		return bytesWritten;
	     }

	     // now write the previous data from the suballoc element
	     // into the newly allocated cluster.
	     cbytes = WriteClusterWithOffset(volume, NewCluster, 0,
					  &WorkSpace->Buffer[0], SuballocSize,
					  KERNEL_ADDRESS_SPACE, retCode,
					  ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	     if (cbytes != SuballocSize)
	     {
		FreeWorkspace(volume, WorkSpace);
		if (retCode)
		   *retCode = NwDiskIoError;
		return bytesWritten;
	     }

	     // free the suballoc element in bit block list
	     FreeSuballocRecord(volume, FATChain);

	     // set previous cluster chain to point to this entry
	     if (PrevCluster == (ULONG) -1)
		*Chain = NewCluster;
	     else
		SetClusterValue(volume, PrevCluster, NewCluster);

	     // update previous cluster to new cluster
	     // this will force inserts after the end of this cluster
	     PrevCluster = NewCluster;

	     cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
					     as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
	     bytesWritten += cbytes;
	     bytesLeft -= cbytes;
	     buf += cbytes;

	     // update context pointer with adjusted offset
	     if (Context)
		*Context = NewCluster;
	     if (Index)
		*Index = vindex;
	     vindex++;

	     FreeWorkspace(volume, WorkSpace);

	     while (bytesLeft > 0)
	     {
		voffset = 0;
		if (vindex == StartIndex)
		   voffset = StartOffset;

		vsize = (bytesLeft > (long)(volume->ClusterSize - voffset))
			? (volume->ClusterSize - voffset) : bytesLeft;

		// allocate cluster and point forward link to EOF
		NewCluster = AllocateClusterSetIndexSetChain(volume, vindex, -1);
		if (NewCluster == -1)
		{
		   // if we could not get a free cluster, then return
		   // (out of drive space)
		   if (retCode)
		      *retCode = NwVolumeFull;
		   return bytesWritten;
		}

#if (ZERO_FILL_SECTORS)
		// zero fill the new cluster
		ZeroPhysicalVolumeCluster(volume, NewCluster, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
#endif

		// set previous cluster chain to point to this entry
		SetClusterValue(volume, PrevCluster, NewCluster);

		// update previous cluster to new cluster
		// this will force inserts after the end of this cluster
		PrevCluster = NewCluster;

		cbytes = WriteClusterWithOffset(volume, NewCluster, voffset, buf, vsize,
						as, retCode, ((Flags & RAW_FILE) ? RAW_PRIORITY : DATA_PRIORITY));
		bytesWritten += cbytes;
		bytesLeft -= cbytes;
		buf += cbytes;

		// update context pointer with adjusted offset
		if (Context)
		   *Context = NewCluster;
		if (Index)
		   *Index = vindex;
		vindex++;
	     }
	     return bytesWritten;
	  }
	  else
	  if (vindex == (index + 1))
	  {
#if (VERBOSE)
	     NWFSPrint("nwfs:  we were asked to write the current suballoc\n");
#endif
	     // for this case, since our target write size fits within
	     // the previously allocated suballoc element, then just
	     // write the data.

	     voffset = 0;
	     if (vindex == StartIndex)
		voffset = StartOffset;

	     // at this point, bytesLeft is either equal to or
	     // less than the size of the current suballocation
	     // record.

	     vsize = bytesLeft;
	     cbytes = WriteSuballocRecord(volume, voffset, FATChain,
					  buf, vsize, as, retCode);
	     bytesWritten += cbytes;
	     bytesLeft -= cbytes;
	  }
	  else
	     NWFSPrint("nwfs:  suballoc write was index(%d) < vindex(%d) (0)\n",
		       (int)index, (int)vindex);

	  return bytesWritten;
       }

       // get next fat table entry and index
       FAT = GetFatEntryAndLRU(volume, FATChain, &lru, &FAT_S);
       if (FAT)
       {
	  if (Context)
	     *Context = FATChain;
	  index = FAT->FATIndex;
	  if (Index)
	     *Index = vindex;
       }

       // if the fat chain terminates, then exit
       if (!FAT)
	  return bytesWritten;

       lcount++;
    }
    return bytesWritten;


}

#endif

