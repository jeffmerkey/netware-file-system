
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
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  LRU.C
*   DESCRIP  :   LRU Module
*   DATE     :  November 19, 1998
*
*   This is how NetWare does asynch I/O with an LRU.  Somewhat different
*   than what's in Linux today.  This is a much more compact version
*   than what is in NetWare, and a little slower since it's not
*   written in hand-optimized IA32 assembly language.
*
***************************************************************************/

#include "globals.h"

ULONG lru_active = 0;
ULONG lru_state = 0;
ULONG lru_signal = 0;
BYTE *LRUBlockHash = 0;
ULONG LRUBlockHashLimit = 0;
LRU *ListHead = 0;
LRU *ListTail = 0;
LRU *LRUFreeHead = 0;
LRU *LRUFreeTail = 0;
LRU *LRUDirtyHead = 0;
LRU *LRUDirtyTail = 0;
ULONG LRUDirtyBuffers = 0;
ULONG LRUBlocks = 0;
ULONG PERCENT_FACTOR = LRU_FACTOR;

#if (LINUX_SLEEP)
atomic_t lock_counter = { 0 };
atomic_t in_section;
ULONG section_owner = 0;

#if (LINUX_SPIN && DO_ASYNCH_IO)
spinlock_t LRU_spinlock = SPIN_LOCK_UNLOCKED;
long LRU_flags = 0;
#else
NWFSInitSemaphore(LRUSemaphore);
#endif
NWFSInitSemaphore(LRUSyncSemaphore);
#endif

extern ULONG insert_io(ULONG disk, ASYNCH_IO *io);
extern void RunAsynchIOQueue(ULONG disk);

// mirror round robin counter
ULONG mirror_counter = 0;
BYTE *get_taddress(ULONG addr);

void NWLockLRU(ULONG whence)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN && DO_ASYNCH_IO)
    spin_lock_irqsave(&LRU_spinlock, LRU_flags);
#else
    if (WaitOnSemaphore(&LRUSemaphore) == -EINTR)
       NWFSPrint("lock lru was interupted addr-0x%X\n", (unsigned)whence);
#endif
#endif
    
}

void NWUnlockLRU(ULONG whence)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN && DO_ASYNCH_IO)
    spin_unlock_irqrestore(&LRU_spinlock, LRU_flags);
#else
    SignalSemaphore(&LRUSemaphore);
#endif
#endif

}

ULONG NWLockBuffer(LRU *lru)
{
#if (LINUX_SLEEP)
    if (WaitOnSemaphore(&lru->Semaphore) == -EINTR)
    {
       NWFSPrint("lock buffer was interupted\n");
       return -1;
    }
#endif
    return 0;
}

void NWUnlockBuffer(LRU *lru)
{
#if (LINUX_SLEEP)
    SignalSemaphore(&lru->Semaphore);
#endif
}

void ReleaseWaiters(LRU *lru)
{
    while (lru->Waiters)
    {
#if (LINUX_SLEEP)
       SignalSemaphore(&lru->Semaphore);
#endif
       lru->Waiters--;
    }
}

#if (LINUX_20 | LINUX_22 | LINUX_24)

ULONG GetLRUTime(void)
{
    ULONG year, month, day, hour, minute, second;

    GetUnixTime(NWFSGetSystemTime(),
                &second, &minute, &hour, &day, &month, &year);
    return (ULONG)(second + (minute * 60));
}

#endif

LRU *RemoveListHead(LRU *lru)
{
    if (ListHead == lru)
    {
       ListHead = (void *) lru->listnext;
       if (ListHead)
	  ListHead->listprior = NULL;
       else
	  ListTail = NULL;
    }
    else
    {
       lru->listprior->listnext = lru->listnext;
       if (lru != ListTail)
	  lru->listnext->listprior = lru->listprior;
       else
	  ListTail = lru->listprior;
    }
    lru->listnext = lru->listprior = 0;
    return lru;

}

void InsertListHead(LRU *lru)
{
    if (!ListHead)
    {
       ListHead = lru;
       ListTail = lru;
       lru->listnext = lru->listprior = 0;
    }
    else
    {
       ListTail->listnext = lru;
       lru->listnext = 0;
       lru->listprior = ListTail;
       ListTail = lru;
    }
    return;

}

// this is a very SLOW brute force ordering routine.  We only use
// it when we are doing synchronous I/O (debug mode).

LRU *IndexDirtyLRU(LRU *i)
{
    register int j;
    ULONG count = 0, ccode;
    LRU *old, *p;
    nwvp_asynch_map map[8];

    for (j=0; j < 8; j++)
    {
       i->lba[j] = 0;
       i->disk[j] = 0;
    }

    // This function returns a mirror map of all mirror
    // group members, whether IN_SYNC or not.  For the
    // write case, we always attempt ot update all the
    // mirrored members asynchronously.  For the read case,
    // we only allow reads from IN_SYNC mirror members.

    ccode = nwvp_vpartition_map_asynch_write(i->nwvp_handle,
                                             i->block,
                                             &count,
					     &map[0]);
    if (!ccode && count)
    {
       i->mirror_count = (count % 8);
       for (j=0; j < (int)(count % 8); j++)
       {
          i->lba[j] = map[j].sector_offset;
          i->disk[j] = map[j].disk_id;
       }
    }

    if (!LRUDirtyTail)
    {
       i->dnext = i->dprior = NULL;
       LRUDirtyTail = i;
       return i;
    }
    p = LRUDirtyHead;
    old = NULL;
    while (p)
    {
       if ((p->disk < i->disk) && (p->lba[0] < i->lba[0]))
       {
	  old = p;
	  p = p->dnext;
       }
       else
       {
	  if (p->dprior)
	  {
	     p->dprior->dnext = i;
	     i->dnext = p;
	     i->dprior = p->dprior;
	     p->dprior = i;
	     return LRUDirtyHead;
	  }
	  i->dnext = p;
	  i->dprior = NULL;
	  p->dprior = i;
	  return i;
       }
    }
    old->dnext = i;
    i->dnext = NULL;
    i->dprior = old;
    LRUDirtyTail = i;
    return LRUDirtyHead;
}


LRU *RemoveDirty(LRU *lru)
{
#if (!SINGLE_DIRTY_LIST)
    if (LRUDirtyHead == lru)
    {
       LRUDirtyHead = (void *) lru->dnext;
       if (LRUDirtyHead)
	  LRUDirtyHead->dprior = NULL;
       else
	  LRUDirtyTail = NULL;
    }
    else
    {
       lru->dprior->dnext = lru->dnext;
       if (lru != LRUDirtyTail)
	  lru->dnext->dprior = lru->dprior;
       else
	  LRUDirtyTail = lru->dprior;
    }
#endif
    
    lru->dnext = lru->dprior = (LRU *)-3;
    lru->state &= ~L_DIRTY;
    lru->state |= L_UPTODATE;
    lru->owner &= ~QUEUE_DIRTY;
    if (LRUDirtyBuffers)
       LRUDirtyBuffers--;

    return lru;
}

#if (DO_ASYNCH_IO & !DOS_UTIL & !WINDOWS_NT_UTIL & !LINUX_UTIL)

// the asynch io manager orders requests. so we don't need to do
// this here.

void InsertDirty(LRU *lru)
{
    if (lru->owner & QUEUE_DIRTY)
    {
       NWFSPrint("dirty element was already on list\n");
       return;
    }

#if (!SINGLE_DIRTY_LIST)
    if (!LRUDirtyHead)
    {
       LRUDirtyHead = lru;
       LRUDirtyTail = lru;
       lru->dnext = lru->dprior = 0;
    }
    else
    {
       LRUDirtyTail->dnext = lru;
       lru->dnext = 0;
       lru->dprior = LRUDirtyTail;
       LRUDirtyTail = lru;
    }
#endif
    
    lru->owner |= QUEUE_DIRTY;
    lru->state |= L_DIRTY;
    LRUDirtyBuffers++;
    return;

}

#else

void InsertDirty(LRU *lru)
{
    if (lru->owner & QUEUE_DIRTY)
    {
       NWFSPrint("dirty element was already on list\n");
       return;
    }

#if (!SINGLE_DIRTY_LIST)
    LRUDirtyHead = IndexDirtyLRU(lru);
#endif
    
    lru->owner |= QUEUE_DIRTY;
    lru->state |= L_DIRTY;
    LRUDirtyBuffers++;
    return;
}

#endif

LRU *RemoveLRU(LRU_HANDLE *lru_handle, LRU *lru)
{
    if (lru_handle->LRUListHead == lru)
    {
       lru_handle->LRUListHead = (void *) lru->next;
       if (lru_handle->LRUListHead)
	  lru_handle->LRUListHead->prior = NULL;
       else
	  lru_handle->LRUListTail = NULL;
    }
    else
    {
       lru->prior->next = lru->next;
       if (lru != lru_handle->LRUListTail)
	  lru->next->prior = lru->prior;
       else
	  lru_handle->LRUListTail = lru->prior;
    }

    lru->next = lru->prior = 0;
    lru->owner &= ~QUEUE_LRU;
    if (lru_handle->LocalLRUBlocks)
       lru_handle->LocalLRUBlocks--;
    return lru;

}

void InsertLRU(LRU_HANDLE *lru_handle, LRU *lru)
{
    if (lru->owner & QUEUE_LRU)
    {
       NWFSPrint("LRU element was already on list\n");
       return;
    }

    if (!lru_handle->LRUListHead)
    {
       lru_handle->LRUListHead = lru;
       lru_handle->LRUListTail = lru;
       lru->next = lru->prior = 0;
    }
    else
    {
       lru_handle->LRUListTail->next = lru;
       lru->next = 0;
       lru->prior = lru_handle->LRUListTail;
       lru_handle->LRUListTail = lru;
    }
    lru->owner |= QUEUE_LRU;
    lru_handle->LocalLRUBlocks++;
    return;

}

void InsertLRUTop(LRU_HANDLE *lru_handle, LRU *lru)
{
    if (lru->owner & QUEUE_LRU)
    {
       NWFSPrint("LRU element was already on list\n");
       return;
    }

    if (!lru_handle->LRUListHead)
    {
       lru_handle->LRUListHead = lru;
       lru_handle->LRUListTail = lru;
       lru->next = lru->prior = 0;
    }
    else
    {
       lru->next = lru_handle->LRUListHead;
       lru->prior = 0;
       lru_handle->LRUListHead->prior = lru;
       lru_handle->LRUListHead = lru;
    }
    lru->owner |= QUEUE_LRU;
    lru_handle->LocalLRUBlocks++;
    return;

}

LRU *GetFreeLRU(void)
{
    register LRU *lru;

    if (LRUFreeHead)
    {
       lru = LRUFreeHead;
       LRUFreeHead = (void *) lru->next;
       if (LRUFreeHead)
	  LRUFreeHead->prior = NULL;
       else
	  LRUFreeTail = NULL;

       lru->next = lru->prior = 0;
       lru->owner &= ~QUEUE_FREE;
       return lru;
    }
    return 0;

}

void PutFreeLRU(LRU *lru)
{
    if (lru->owner & QUEUE_FREE)
    {
       NWFSPrint("free element was already on list\n");
       return;
    }

    if (!LRUFreeHead)
    {
       LRUFreeHead = lru;
       LRUFreeTail = lru;
       lru->next = lru->prior = 0;
    }
    else
    {
       LRUFreeTail->next = lru;
       lru->next = 0;
       lru->prior = LRUFreeTail;
       LRUFreeTail = lru;
    }
    lru->owner |= QUEUE_FREE;
    return;
}

LRU *SearchHash(VOLUME *volume, ULONG block)
{
    register LRU *lru;
    register ULONG hash;
    register DATA_LRU_HASH_LIST *HashTable;

    hash = (block & (LRUBlockHashLimit - 1));
    HashTable = (DATA_LRU_HASH_LIST *) LRUBlockHash;
    if (HashTable)
    {
       lru = HashTable[hash].head;
       while (lru)
       {
	  if ((lru->block == block) && (lru->volnum == volume->VolumeNumber))
	     return lru;
	  lru = lru->hashNext;
       }
    }
    return 0;
}

ULONG AddToHash(LRU *lru)
{
    register ULONG hash;
    register DATA_LRU_HASH_LIST *HashTable;

    hash = (lru->block & (LRUBlockHashLimit - 1));
    HashTable = (DATA_LRU_HASH_LIST *) LRUBlockHash;
    if (HashTable)
    {
       if (lru->owner & QUEUE_HASH)
       {
	  NWFSPrint("hash element was already on list\n");
	  return -1;
       }

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
       lru->owner |= QUEUE_HASH;
       return 0;
    }
    return -1;
}

ULONG RemoveHash(LRU *lru)
{
    register ULONG hash;
    register DATA_LRU_HASH_LIST *HashTable;

    hash = (lru->block & (LRUBlockHashLimit - 1));
    HashTable = (DATA_LRU_HASH_LIST *) LRUBlockHash;
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

       lru->hashNext = lru->hashPrior = 0;
       lru->owner &= ~QUEUE_HASH;
       return 0;
    }
    return -1;
}

void FreeLRUElement(LRU *lru)
{
    if (lru)
    {
       if (lru->buffer)
       {
#if (LINUX_20 | LINUX_22)
     	  kfree(lru->buffer);

	  LRU_BUFFER_TRACKING.count -= IO_BLOCK_SIZE;
          LRU_BUFFER_TRACKING.units--;
          MemoryInUse -= IO_BLOCK_SIZE;

#elif (LINUX_24)
          __free_page(lru->page);

	  LRU_BUFFER_TRACKING.count -= IO_BLOCK_SIZE;
          LRU_BUFFER_TRACKING.units--;
          MemoryInUse -= IO_BLOCK_SIZE;

#else
	  NWFSFree(lru->buffer);
#endif
       }
       NWFSFree(lru);
       if (LRUBlocks)
	  LRUBlocks--;
    }
    return;
}

LRU *AllocateLRUElement(VOLUME *volume, ULONG block, ULONG *ccode)
{
    register LRU *lru;
    
    if (ccode)
       *ccode = 0;

    lru = NWFSAlloc(sizeof(LRU), LRU_TAG);
    if (!lru)
       return 0;

    NWFSSet(lru, 0, sizeof(LRU));

    lru->state = L_FREE;
    lru->block = -1;
    lru->volnum = (ULONG) -1;

#if (LINUX_SLEEP)
    AllocateSemaphore(&lru->Semaphore, 0);  // this is a waiters queue
#endif

    // if we are a linux driver, then the buffer must be page aligned
    // or the lower layers may get DMA timeouts (drivers in Linux are
    // optimized for 512-byte aligned memory).

#if (LINUX_20 | LINUX_22)
    
    lru->buffer = kmalloc(IO_BLOCK_SIZE, GFP_ATOMIC);
    if (!lru->buffer)
    {
       NWFSFree(lru);
       return 0;
    }
    
    LRU_BUFFER_TRACKING.count += IO_BLOCK_SIZE;
    LRU_BUFFER_TRACKING.units++;
    MemoryInUse += IO_BLOCK_SIZE; 
    
#elif (LINUX_24)
    lru->page = alloc_page(GFP_ATOMIC);
    if (!lru->page)
    {
       NWFSFree(lru);
       return 0;
    }
    lru->buffer = (BYTE *)page_address(lru->page);
    
    LRU_BUFFER_TRACKING.count += IO_BLOCK_SIZE;
    LRU_BUFFER_TRACKING.units++;
    MemoryInUse += IO_BLOCK_SIZE;
    
#else
    lru->buffer = NWFSIOAlloc(IO_BLOCK_SIZE, LRU_BUFFER_TAG);
    if (!lru->buffer)
    {
       NWFSFree(lru);
       return 0;
    }
#endif

    NWFSSet(lru->buffer, 0, IO_BLOCK_SIZE);
    LRUBlocks++;
    InsertListHead(lru);
    return lru;
}

ULONG InitializeLRU(LRU_HANDLE *lru_handle, BYTE *name, ULONG mode,
		    ULONG max, ULONG min, ULONG age)
{
    NWFSSet(lru_handle, 0, sizeof(LRU_HANDLE));
    lru_handle->LRUName = name;
    lru_handle->LRUMode = mode;

    if (!LRUBlockHash)
    {
       LRUBlockHash = NWFSCacheAlloc(BLOCK_LRU_HASH_SIZE, LRU_HASH_TAG);
       if (!LRUBlockHash)
          return -1;

       LRUBlockHashLimit = NUMBER_OF_LRU_HASH_ENTRIES;
       NWFSSet(LRUBlockHash, 0, BLOCK_LRU_HASH_SIZE);
    }

    lru_handle->MinimumLRUBlocks = min;
    lru_handle->MaximumLRUBlocks = max;
    lru_handle->LocalLRUBlocks = 0;
    lru_handle->AGING_INTERVAL = age;
    
    return 0;
}

ULONG FreeLRU(void)
{
    register ULONG ccode;
    register LRU *lru, *list;

    NWLockLRU((ULONG)FreeLRU);
    lru_active = TRUE;
    list = ListHead;
    ListHead = ListTail = 0;
    while (list)
    {
       lru = list;
       list = list->listnext;

#if (LINUX_20 || LINUX_22 || LINUX_24)
       if (lru->state & L_FLUSHING)
       {
          NWUnlockLRU((ULONG)FreeLRU);

    	  while (lru->state & L_FLUSHING)
             schedule();

	  NWLockLRU((ULONG)FreeLRU);
       }
#endif
       
       if ((lru->state & L_DIRTY)      &&
	   (lru->nwvp_handle)          &&
	   (lru->state & L_DATAVALID))
       {
	  lru->state |= L_FLUSHING;
	  lru->state &= ~L_MODIFIED;
          NWUnlockLRU((ULONG)FreeLRU);

	  ccode = nwvp_vpartition_block_write(lru->nwvp_handle, lru->block, 1, lru->buffer);

          NWLockLRU((ULONG)FreeLRU);
	  lru->state &= ~L_FLUSHING;
	  lru->timestamp = 0;

#if (VERBOSE)
	  if (ccode)
	     NWFSPrint("nwfs:  I/O Error (close) block-%d volume-%d\n",
		       (int)lru->block, (int)lru->volnum);
#endif
          if (!(lru->state & L_MODIFIED))
	  {
             NWFSPrint("lru was modified -- free lru\n"); 
     	     RemoveDirty(lru);
	  }
       }
       FreeLRUElement(lru);
    }

    list = LRUFreeHead;
    LRUFreeHead = LRUFreeTail = 0;
    while (list)
    {
       lru = list;
       list = list->next;
       FreeLRUElement(lru);
    }
    lru_active = FALSE;
    NWUnlockLRU((ULONG)FreeLRU);

    return 0;
}

ULONG retry_write_aio(ASYNCH_IO *io)
{
    register ULONG ccode;
    register LRU *lru = (LRU *) io->call_back_parameter;

    // if we had a write failure for any of the mirrored asynch
    // writes, then we retry the write operation synchronously with
    // retry logic to attempt to recover lost sectors (hotfixing).
    // nwvp_() function calls perform hotfixing and mirror failover
    // operations.

    NWFSPrint("write retry\n");
    
    ccode = nwvp_vpartition_block_write(lru->nwvp_handle, lru->block, 1, lru->buffer);

    NWLockLRU((ULONG)retry_write_aio);
    lru->state &= ~L_FLUSHING;
    lru->timestamp = 0;

    // if no I/O error and the buffer was not dirtied while I/O was
    // pending, then remove from the dirty list.
    if (!ccode)
    {
       if (!(lru->state & L_MODIFIED))
          RemoveDirty(lru);
    }        
    lru->io_signature = 0;
    NWUnlockLRU((ULONG)retry_write_aio);

    return 0;
}

ULONG release_write_aio(ASYNCH_IO *io)
{
    register LRU *lru = (LRU *) io->call_back_parameter;

    NWLockLRU((ULONG)release_write_aio);
    if (io->ccode)
       lru->aio_ccode = io->ccode;

    // count the number of aoi's completed.  signal when the
    // last mirror member has returned completed.

    lru->mirrors_completed++;
    if (lru->mirror_count == lru->mirrors_completed)
    {
       lru->state &= ~L_FLUSHING;
       lru->timestamp = 0;

       if (!lru->aio_ccode)
       {
          if (!(lru->state & L_MODIFIED))
             RemoveDirty(lru);
       }
       else
       {
          // post this write for retry and failover operations
          io->call_back_routine = retry_write_aio;
          io->flags = ASIO_SLEEP_CALLBACK;
          NWUnlockLRU((ULONG)release_write_aio);
	  return 1;
       }
       lru->io_signature = 0;
    }
    NWUnlockLRU((ULONG)release_write_aio);
    return 0;
}

ULONG FlushLRU(void)
{
#if (DO_ASYNCH_IO & !DOS_UTIL & !WINDOWS_NT_UTIL & !LINUX_UTIL)

    register ULONG i;
    register LRU *lru, *list;
    BYTE disks[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    register int j;
    ULONG count = 0, ccode;
    nwvp_asynch_map map[8];

    NWLockLRU((ULONG)FlushLRU);
    lru_active = TRUE;

#if (SINGLE_DIRTY_LIST)
    list = ListHead;
    while (list)
    {
       lru = list;
       list = list->listnext;
#else
    list = LRUDirtyHead;
    while (list)
    {
       lru = list;
       list = list->dnext;
       
       if (lru->dnext == (LRU *)-3)
          NWFSPrint("FlushLRU -- lru->dnext was already off the list\n"); 
#endif
       
       if ((lru->state & L_DIRTY)      &&
	   (lru->state & L_DATAVALID)  &&
	   (lru->nwvp_handle)          &&
           (!lru->lock_count)          &&
	   (!(lru->state & L_FLUSHING)))
       {
          lru->state |= L_FLUSHING;
	  lru->state &= ~L_MODIFIED;
	  // this flag is cleared when we dirty a buffer

	  if (lru->io_signature == ASIO_SUBMIT_IO)
	  {
             NWFSPrint("io already scheduled (f)\n");
             continue;
	  }

	  lru->io_signature = ASIO_SUBMIT_IO;
	  for (j=0; j < 8; j++)
          {
             lru->lba[j] = 0;
             lru->disk[j] = 0;
          }

          // This function returns a mirror map of all mirror
          // group members, whether IN_SYNC or not.  For the
          // write case, we always attempt ot update all the
          // mirrored members asynchronously.  For the read case,
          // we only allow reads from IN_SYNC mirror members.

          ccode = nwvp_vpartition_map_asynch_write(lru->nwvp_handle,
                                                   lru->block,
                                                   &count,
					           &map[0]);
          if (!ccode && count)
          {
             lru->mirror_count = (count % 8);
             for (j=0; j < (int)(count % 8); j++)
             {
                lru->lba[j] = map[j].sector_offset;
                lru->disk[j] = map[j].disk_id;
             }
          }
	  else
	  {
             NWFSPrint("map_asynch_write failed volume-%d block-%d\n",
			(int)lru->volnum, (int)lru->block);
     	     continue;
	  }

          // The block bad bits are always identical across mirrors for 
	  // every member of a mirror group.  if during Hotfixing, data  
	  // could not be completely recovered for a read I/O error
	  // from other mirrors or from cache, we artifically hotfix 
	  // all mirrored partitions and record any unrecoverable sectors
	  // in a bad bit block in the device hotfix data area.  
	  // The first time a read is attempted from a sector region 
	  // that was unrecoverable from a previous read hotfix, then 
	  // we know that some portion of the data is now missing and
	  // we return a hard I/O error to the user for this block region.  
	  // Writing to a read hotfixed region clears the bad bit 
	  // table for a particular hotfixed block.  If a block has 	   
	  // bad bits, we must transact the mirrored write for this 
	  // single case synchronously in order to clear the bad bit
	  // tables for all the mirrored partitions and update the 
	  // hotfix tables.

          // if there are bad bits for this block, we must update the hotfix 
	  // tables on any active mirrors.

          // if the bad bits have changed, write the new bits
	  if (lru->bad_bits != map[0].bad_bits) 
	  {
             NWUnlockLRU((ULONG)FlushLRU);

	     ccode = nwvp_vpartition_bad_bit_update(lru->nwvp_handle, 
			                            lru->block,
				                    lru->bad_bits);
	     if (ccode)
	       NWFSPrint("nwfs:  could not update bad bits vol-%d block-%d\n",
		        (int)lru->volnum, (int)lru->block);
             
	     NWLockLRU((ULONG)FlushLRU);
	  }
	 
	  lru->aio_ccode = 0; 
	  lru->mirrors_completed = 0;
	  for (i=0; i < lru->mirror_count; i++)
          {
             NWFSSet(&lru->io[i], 0, sizeof(ASYNCH_IO));
	     lru->io[i].command = ASYNCH_WRITE;
	     lru->io[i].disk = lru->disk[i];
	     lru->io[i].flags = ASIO_INTR_CALLBACK;
             lru->io[i].sector_offset = lru->lba[i];
             lru->io[i].sector_count = 8;
             lru->io[i].buffer = lru->buffer;
	     lru->io[i].call_back_routine = release_write_aio;
             lru->io[i].call_back_parameter = (ULONG)lru;
	     
             // the file system is not allowed to ever write sector
             // zero at this layer.
             if (lru->io[i].sector_offset == 0)
             {
                NWFSPrint("nwfs:  mirroring map is corrupted block-%d vol-%d.  Aborting I/O\n",
		         (int)lru->block, (int)lru->volnum);
                continue;
             }
             insert_io(lru->disk[i], &lru->io[i]);
             disks[(lru->disk[i] % 8)] = TRUE;
	  }
       }
    }
    lru_active = FALSE;
    NWUnlockLRU((ULONG)FlushLRU);

    // signal disk processes for any queues we gave work to.
    for (i=0; i < 8; i++)
    {
       if (disks[i])
	  RunAsynchIOQueue(i);
    }
    return 0;

#else

    register ULONG ccode;
    register LRU *lru, *list;

    NWLockLRU((ULONG)FlushLRU);
    lru_active = TRUE;

#if (SINGLE_DIRTY_LIST)
    list = ListHead;
    while (list)
    {
       lru = list;
       list = list->listnext;
#else
    list = LRUDirtyHead;
    while (list)
    {
       lru = list;
       list = list->dnext;
       
       if (lru->dnext == (LRU *)-3)
          NWFSPrint("FlushLRU -- lru->dnext was already off the list\n"); 
#endif

       if ((lru->state & L_DIRTY)      &&
	   (lru->state & L_DATAVALID)  &&
	   (lru->nwvp_handle)          &&
	   (!lru->lock_count)          &&
	   (!(lru->state & L_FLUSHING)))
       {
	  lru->state |= L_FLUSHING;
	  lru->state &= ~L_MODIFIED;
          NWUnlockLRU((ULONG)FlushLRU);

	  ccode = nwvp_vpartition_block_write(lru->nwvp_handle, lru->block, 1, lru->buffer);

          NWLockLRU((ULONG)FlushLRU);
	  lru->state &= ~L_FLUSHING;
	  lru->timestamp = 0;

	  if (!ccode)
	  {
             if (!(lru->state & L_MODIFIED))
	        RemoveDirty(lru);
	  }
       }
    }
    lru_active = FALSE;
    NWUnlockLRU((ULONG)FlushLRU);
    return 0;

#endif
}

ULONG FlushEligibleLRU(void)
{
#if (DO_ASYNCH_IO & !DOS_UTIL & !WINDOWS_NT_UTIL & !LINUX_UTIL)

    register ULONG i;
    register LRU *lru, *list;
    BYTE disks[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    register int j;
    ULONG count = 0, ccode;
    nwvp_asynch_map map[8];

    NWLockLRU((ULONG)FlushEligibleLRU);
    lru_active = TRUE;

#if (SINGLE_DIRTY_LIST)
    list = ListHead;
    while (list)
    {
       lru = list;
       list = list->listnext;
#else
    list = LRUDirtyHead;
    while (list)
    {
       lru = list;
       list = list->dnext;
       
       if (lru->dnext == (LRU *)-3)
          NWFSPrint("FlushEligLRU -- lru->dnext was already off the list\n"); 
#endif

#if (LINUX_20 | LINUX_22 | LINUX_24)
       if ((lru_state != NWFS_FLUSH_ALL) &&
           (lru->timestamp + lru->AGING_INTERVAL) > GetLRUTime())
          continue;
#endif
       if ((lru->state & L_DIRTY)      &&
           (lru->state & L_DATAVALID)  &&
	   (lru->nwvp_handle)          &&
           (!lru->lock_count)          &&
	   (!(lru->state & L_FLUSHING)))
       {
	  lru->state |= L_FLUSHING;
	  lru->state &= ~L_MODIFIED;

          NWUnlockLRU((ULONG)FlushEligibleLRU);

	  if (lru->io_signature == ASIO_SUBMIT_IO)
	  {
             NWFSPrint("io already scheduled (fe)\n");
             NWLockLRU((ULONG)FlushEligibleLRU);
             continue;
	  }

	  for (j=0; j < 8; j++)
          {
             lru->lba[j] = 0;
             lru->disk[j] = 0;
          }

          // This function returns a mirror map of all mirror
          // group members, whether IN_SYNC or not.  For the
          // write case, we always attempt ot update all the
          // mirrored members asynchronously.  For the read case,
          // we only allow reads from IN_SYNC mirror members.

	  ccode = nwvp_vpartition_map_asynch_write(lru->nwvp_handle,
                                                   lru->block,
                                                   &count,
					           &map[0]);

	  if (!ccode && count)
          {
             lru->mirror_count = (count % 8);
             for (j=0; j < (int)(count % 8); j++)
             {
                lru->lba[j] = map[j].sector_offset;
                lru->disk[j] = map[j].disk_id;
             }
          }
	  else
	  {
             NWFSPrint("map_asynch_write failed volume-%d block-%d\n",
			(int)lru->volnum, (int)lru->block);
             NWLockLRU((ULONG)FlushEligibleLRU);
     	     continue;
	  }
	   

          // The block bad bits are always identical across mirrors for 
	  // every member of a mirror group.  if during Hotfixing, data  
	  // could not be completely recovered for a read I/O error
	  // from other mirrors or from cache, we artifically hotfix 
	  // all mirrored partitions and record any unrecoverable sectors
	  // in a bad bit block in the device hotfix data area.  
	  // The first time a read is attempted from a sector region 
	  // that was unrecoverable from a previous read hotfix, then 
	  // we know that some portion of the data is now missing and
	  // we return a hard I/O error to the user for this block region.  
	  // Writing to a read hotfixed region clears the bad bit 
	  // table for a particular hotfixed block.  If a block has 	   
	  // bad bits, we must transact the mirrored write for this 
	  // single case synchronously in order to clear the bad bit
	  // tables for all the mirrored partitions and update the 
	  // hotfix tables.
	  
          // if there are bad bits for this block, we must update the hotfix 
	  // tables on any active mirrors.

          // if the bad bits have changed, write the new bits
	  if (lru->bad_bits != map[0].bad_bits) 
	  {
//             NWUnlockLRU((ULONG)FlushLRU);

	     ccode = nwvp_vpartition_bad_bit_update(lru->nwvp_handle, 
			                            lru->block,
				                    lru->bad_bits);
	     if (ccode)
	       NWFSPrint("nwfs:  could not update bad bits vol-%d block-%d\n",
		        (int)lru->volnum, (int)lru->block);

//	     NWLockLRU((ULONG)FlushLRU);
	  }

	  lru->aio_ccode = 0; 
	  lru->mirrors_completed = 0;
	  for (i=0; i < lru->mirror_count; i++)
          {
             NWFSSet(&lru->io[i], 0, sizeof(ASYNCH_IO));
             lru->io[i].command = ASYNCH_WRITE;
             lru->io[i].disk = lru->disk[i];
	     lru->io[i].flags = ASIO_INTR_CALLBACK;
             lru->io[i].sector_offset = lru->lba[i];
	     lru->io[i].sector_count = 8;
	     lru->io[i].buffer = lru->buffer;
	     lru->io[i].call_back_routine = release_write_aio;
             lru->io[i].call_back_parameter = (ULONG)lru;
	     
             // the file system is not allowed to ever write sector
             // zero at this layer.
             if (lru->io[i].sector_offset == 0)
             {
                NWFSPrint("nwfs:  mirroring map is corrupted block-%d vol-%d.  Aborting I/O\n",
		         (int)lru->block, (int)lru->volnum);
                NWLockLRU((ULONG)FlushEligibleLRU);
                continue;
             }

	     insert_io(lru->disk[i], &lru->io[i]);
	     disks[(lru->disk[i] % 8)] = TRUE;

	     NWLockLRU((ULONG)FlushEligibleLRU);
          }
       }
    }
    lru_state = 0;
    lru_active = FALSE;
    NWUnlockLRU((ULONG)FlushEligibleLRU);

    // signal disk processes for any queues we gave work to.
    for (i=0; i < 8; i++)
    {
       if (disks[i])
          RunAsynchIOQueue(i);
    }
    return 0;

#else

    register ULONG ccode;
    register LRU *lru, *list;

    NWLockLRU((ULONG)FlushEligibleLRU);
    lru_active = TRUE;

#if (SINGLE_DIRTY_LIST)
    list = ListHead;
    while (list)
    {
       lru = list;
       list = list->listnext;
#else
    list = LRUDirtyHead;
    while (list)
    {
       lru = list;
       list = list->dnext;
       
       if (lru->dnext == (LRU *)-3)
          NWFSPrint("FlushEligLRU -- lru->dnext was already off the list\n"); 
#endif

#if (LINUX_20 | LINUX_22 | LINUX_24)
       if ((lru_state != NWFS_FLUSH_ALL) &&
           (lru->timestamp + lru_handle->AGING_INTERVAL) > GetLRUTime())
          continue;
#endif
       if ((lru->state & L_DIRTY)      &&
           (lru->state & L_DATAVALID)  &&
	   (lru->nwvp_handle)          &&
           (!lru->lock_count)          &&
	   (!(lru->state & L_FLUSHING)))
       {
	  lru->state |= L_FLUSHING;
	  lru->state &= ~L_MODIFIED;
          NWUnlockLRU((ULONG)FlushEligibleLRU);

	  ccode = nwvp_vpartition_block_write(lru->nwvp_handle, lru->block, 1, lru->buffer);

          if (ccode)
	     NWFSPrint("write error in flush LRU\n"); 

	  NWLockLRU((ULONG)FlushEligibleLRU);
	  lru->state &= ~L_FLUSHING;
	  lru->timestamp = 0;

	  if (!ccode)
	  {
             if (!(lru->state & L_MODIFIED))
	        RemoveDirty(lru);
	  }
       }
    }
    lru_state = 0;
    lru_active = FALSE;
    NWUnlockLRU((ULONG)FlushEligibleLRU);
    return 0;

#endif
}

ULONG SyncVolumeLRU(VOLUME *volume)
{
    register ULONG ccode;
    register LRU *lru, *list;

    NWLockLRU((ULONG)FlushVolumeLRU);
    lru_active = TRUE;
    list = ListHead;
    while (list)
    {
       lru = list;
       list = list->listnext;

       if (lru->volnum == volume->VolumeNumber)
       {
#if (LINUX_20 || LINUX_22 || LINUX_24)
	  if (lru->state & L_FLUSHING)
	  {
             NWUnlockLRU((ULONG)SyncVolumeLRU);

	     while (lru->state & L_FLUSHING)
                schedule();

	     NWLockLRU((ULONG)FlushVolumeLRU);
	  }
#endif
	  if ((lru->state & L_DIRTY)        &&
	      (lru->state & L_DATAVALID)    &&
	      (lru->nwvp_handle))
	  {
	     lru->state |= L_FLUSHING;
	     lru->state &= ~L_MODIFIED;
             NWUnlockLRU((ULONG)SyncVolumeLRU);

	     ccode = nwvp_vpartition_block_write(lru->nwvp_handle, lru->block, 1, lru->buffer);

             if (ccode)
	        NWFSPrint("write error in sync volume LRU\n"); 

	     NWLockLRU((ULONG)FlushVolumeLRU);
	     lru->state &= ~L_FLUSHING;
	     lru->timestamp = 0;

             if (!(lru->state & L_MODIFIED))
	        RemoveDirty(lru);
	  }
       }
    }
    lru_active = FALSE;
    NWUnlockLRU((ULONG)SyncVolumeLRU);
    return 0;
}

ULONG FlushVolumeLRU(VOLUME *volume)
{
    register ULONG ccode;
    register LRU *lru, *list;

    NWLockLRU((ULONG)FlushVolumeLRU);
    lru_active = TRUE;
    list = ListHead;
    while (list)
    {
       lru = list;
       list = list->listnext;

       if (lru->volnum == volume->VolumeNumber)
       {
#if (LINUX_20 || LINUX_22 || LINUX_24)
	  if (lru->state & L_FLUSHING)
	  {
             NWUnlockLRU((ULONG)FlushVolumeLRU);

	     while (lru->state & L_FLUSHING)
                schedule();

	     NWLockLRU((ULONG)FlushVolumeLRU);
	  }
#endif

	  if ((lru->state & L_DIRTY)        &&
	      (lru->state & L_DATAVALID)    &&
	      (lru->nwvp_handle))
	  {
	     lru->state |= L_FLUSHING;
	     lru->state &= ~L_MODIFIED;
             NWUnlockLRU((ULONG)FlushVolumeLRU);

	     ccode = nwvp_vpartition_block_write(lru->nwvp_handle, lru->block, 1, lru->buffer);

             if (ccode)
	        NWFSPrint("write error in flush volume LRU\n"); 

	     NWLockLRU((ULONG)FlushVolumeLRU);
	     lru->state &= ~L_FLUSHING;
	     lru->timestamp = 0;

             if (!(lru->state & L_MODIFIED))
	        RemoveDirty(lru);
	  }
	  RemoveLRU(lru->lru_handle, lru);
	  RemoveHash(lru);

	  RemoveListHead(lru);
	  lru->state = L_FREE;
	  lru->volnum = (ULONG) -1;
	  PutFreeLRU(lru);
       }
    }
    lru_active = FALSE;
    NWUnlockLRU((ULONG)FlushVolumeLRU);
    return 0;
}

//
//   blocking read functions
//

#if (DO_ASYNCH_IO)
ULONG release_read_aio(ASYNCH_IO *io)
{
#if (LINUX_SLEEP)
    SignalSemaphore((struct semaphore *)io->call_back_parameter);
#endif
    return 0;
}
#endif

LRU *GetAvailableLRU(LRU_HANDLE *lru_handle, VOLUME *volume, ULONG block)
{
    register LRU *lru = 0;

    if ((lru_handle->LocalLRUBlocks < lru_handle->MaximumLRUBlocks) &&
        (LRUBlocks < MAX_LRU_BLOCKS))
    {
       ULONG ccode = 0;

       if (LRUFreeHead)
       {
          lru = GetFreeLRU();
          if (lru)
          {
             InsertListHead(lru);
             lru->lru_handle = lru_handle;
             lru->AGING_INTERVAL = lru_handle->AGING_INTERVAL;
	     return lru;
          }
       }
	    
       lru = AllocateLRUElement(volume, block, &ccode);
       if (ccode)
          return 0;

       if (lru)
       {
          lru->lru_handle = lru_handle;
          lru->AGING_INTERVAL = lru_handle->AGING_INTERVAL;
          return lru;
       }
    }

    if (lru_handle->LRUListHead)
    {
       lru = lru_handle->LRUListTail;
       while ((lru) &&
             ((lru->state & (L_DIRTY | L_LOADING | L_FLUSHING)) ||
	      (lru->lock_count)))
          lru = lru->prior;
	  
       if (lru)
       {
     	  RemoveLRU(lru->lru_handle, lru);
          RemoveHash(lru);
       }
    }
    return lru;
}

LRU *FillBlock(LRU_HANDLE *lru_handle, VOLUME *volume, ULONG block,
	       ULONG fill, ULONG ccount,
	       ULONG *mirror)
{
    register ULONG retCode;
#if (DO_ASYNCH_IO & !DOS_UTIL & !WINDOWS_NT_UTIL & !LINUX_UTIL)
    register ULONG mirror_index;
#endif
    register LRU *lru = 0;

    lru = GetAvailableLRU(lru_handle, volume, block);
    if (!lru)
       return 0;

    if (lru->io_signature == ASIO_SUBMIT_IO)
    {
       NWFSPrint("io already scheduled (f)\n");
       RemoveListHead(lru);
       lru->state = L_FREE;
       lru->volnum = (ULONG) -1;
       PutFreeLRU(lru);
       return 0;
    }

    lru->io_signature = ASIO_SUBMIT_IO;
    lru->aio_ccode = 0; 

    lru->state = L_LOADING;
    lru->block = block;
    lru->nwvp_handle = volume->nwvp_handle;
    lru->volnum = volume->VolumeNumber;

    AddToHash(lru);
    lru->lru_handle = lru_handle;
    lru->AGING_INTERVAL = lru_handle->AGING_INTERVAL;
    InsertLRUTop(lru->lru_handle, lru);

    if (fill)
    {
#if (DO_ASYNCH_IO & !DOS_UTIL & !WINDOWS_NT_UTIL & !LINUX_UTIL)
#if (LINUX_SLEEP)
       NWFSInitSemaphore(semaphore);
#endif
       register int j;
       ULONG count = 0, map_ccode;
       nwvp_asynch_map map[8];

       // remap the lru mirroring information since during recovery, it
       // may have changed.

       lru->bad_bits = 0;
       for (j=0; j < 8; j++)
       {
	  lru->lba[j] = 0;
	  lru->disk[j] = 0;
       }

       map_ccode = nwvp_vpartition_map_asynch_read(lru->nwvp_handle,
						   lru->block,
						   &count, &map[0]);
       if (!map_ccode && count)
       {
	  lru->mirror_count = (count % 8);
	  for (j=0; j < (int)(count % 8); j++)
	  {
	     lru->lba[j] = map[j].sector_offset;
	     lru->disk[j] = map[j].disk_id;
	  }
       }

       NWFSSet(&lru->io[0], 0, sizeof(ASYNCH_IO));
       lru->io[0].command = ASYNCH_READ;

       // Pick a random mirror member to read from that is IN_SYNC.
       // nwvp_vpartition_map_asynch_read() will only return a list
       // of IN_SYNC mirrors to read from, as opposed to
       // nwvp_vpartition_map_asynch_write() which will return all
       // present mirror group members with an active device.

       // We typically don't want to interleave 4K reads for a
       // complete cluster across mirrors because a cluster is
       // almost always a contiguous run of sectors.  We allow
       // the LRU reader to pass a mirror context pointer
       // where we can record the last mirror index and use
       // it for subsequent calls to fill any blocks that may
       // all reside within the same cluster.

       if (mirror)
       {
          if (*mirror < count)
             mirror_index = *mirror;
          else
          {
             mirror_index = mirror_counter % lru->mirror_count;
             *mirror = mirror_index;
          }
       }
       else
          mirror_index = mirror_counter % lru->mirror_count;
       mirror_counter++;

       lru->io[0].disk = lru->disk[mirror_index];
       lru->io[0].flags = ASIO_INTR_CALLBACK;
       lru->io[0].sector_offset = lru->lba[mirror_index];
       lru->io[0].sector_count = 8;
       lru->io[0].buffer = lru->buffer;
       lru->io[0].call_back_routine = release_read_aio;
       lru->io[0].call_back_parameter = (ULONG)&semaphore;

       lru->bad_bits = map[mirror_index].bad_bits;

       insert_io(lru->io[0].disk, &lru->io[0]);

       NWUnlockLRU((ULONG)FillBlock);

       RunAsynchIOQueue(lru->io[0].disk);

#if (LINUX_SLEEP)
       WaitOnSemaphore(&semaphore);
#endif
       // if we get an I/O error on the async read, retry the operation
       // synchronously to perform hotfixing or mirrored failover.

       retCode = lru->io[0].ccode;
       if (retCode)
	  retCode = nwvp_vpartition_block_read(lru->nwvp_handle, lru->block, 1, lru->buffer);

       NWLockLRU((ULONG)FillBlock);
#else
       NWUnlockLRU((ULONG)FillBlock);

       retCode = nwvp_vpartition_block_read(lru->nwvp_handle, lru->block, 1, lru->buffer);

       NWLockLRU((ULONG)FillBlock);
#endif
       if (retCode)
       {
	  RemoveLRU(lru->lru_handle, lru);
	  RemoveHash(lru);

	  RemoveListHead(lru);
	  lru->state = L_FREE;
	  lru->volnum = (ULONG) -1;
	  PutFreeLRU(lru);

          lru->io_signature = 0;
    
	  ReleaseWaiters(lru);
	  return 0;
       }
    }
    lru->state &= ~L_LOADING;
    lru->state |= (L_DATAVALID | L_UPTODATE);

    lru->io_signature = 0;

    ReleaseWaiters(lru);
    return lru;

}

//
//  Asynch Read Functions
//

ULONG retry_asynch_read_ahead(ASYNCH_IO *io)
{
    register ULONG ccode;
    register LRU *lru = (LRU *) io->call_back_parameter;

    // if we get an I/O error on the async read, retry the operation
    // synchronously to perform hotfixing or mirrored failover.

    ccode = nwvp_vpartition_block_read(lru->nwvp_handle, lru->block, 
		                       1, lru->buffer);

    NWLockLRU((ULONG)retry_asynch_read_ahead);
    if (ccode)
    {
       RemoveLRU(lru->lru_handle, lru);
       RemoveHash(lru);

       RemoveListHead(lru);
       lru->state = L_FREE;
       lru->volnum = (ULONG) -1;
       PutFreeLRU(lru);

       ReleaseWaiters(lru);
       lru->io_signature = 0;

       NWUnlockLRU((ULONG)retry_asynch_read_ahead);
       return 0;
    }
    lru->state &= ~L_LOADING;
    lru->state |= (L_DATAVALID | L_UPTODATE);
    ReleaseWaiters(lru);
    lru->io_signature = 0;

    NWUnlockLRU((ULONG)retry_asynch_read_ahead);
    return 0;

}

ULONG release_asynch_read_ahead(ASYNCH_IO *io)
{
    register ULONG ccode;
    register LRU *lru = (LRU *) io->call_back_parameter;

    NWLockLRU((ULONG)release_asynch_read_ahead);
    ccode = io->ccode;
    if (ccode)
    {
       io->call_back_routine = retry_asynch_read_ahead;
       io->flags = ASIO_SLEEP_CALLBACK;
       NWUnlockLRU((ULONG)release_asynch_read_ahead);
       return 1;
    }
    lru->state &= ~L_LOADING;
    lru->state |= (L_DATAVALID | L_UPTODATE);
    ReleaseWaiters(lru);
    lru->io_signature = 0;

    NWUnlockLRU((ULONG)release_asynch_read_ahead);
    return 0;

}

LRU *FillBlockAsynch(LRU_HANDLE *lru_handle, VOLUME *volume, ULONG block,
		     ULONG ccount, BYTE *disks,
		     ULONG *mirror)
{
    register ULONG mirror_index;
    register LRU *lru = 0;
    register int j;
    ULONG count = 0, map_ccode;
    nwvp_asynch_map map[8];

    lru = GetAvailableLRU(lru_handle, volume, block);
    if (!lru)
       return 0;

    if (lru->io_signature == ASIO_SUBMIT_IO)
    {
       NWFSPrint("io already scheduled (f)\n");
       RemoveListHead(lru);
       lru->state = L_FREE;
       lru->volnum = (ULONG) -1;
       PutFreeLRU(lru);
       return 0;
    }

    lru->io_signature = ASIO_SUBMIT_IO;
    lru->aio_ccode = 0; 

    lru->state = L_LOADING;
    lru->block = block;
    lru->nwvp_handle = volume->nwvp_handle;
    lru->volnum = volume->VolumeNumber;

    AddToHash(lru);
    lru->lru_handle = lru_handle;
    lru->AGING_INTERVAL = lru_handle->AGING_INTERVAL;
    InsertLRUTop(lru->lru_handle, lru);

    lru->bad_bits = 0;
    for (j=0; j < 8; j++)
    {
       lru->lba[j] = 0;
       lru->disk[j] = 0;
    }

    map_ccode = nwvp_vpartition_map_asynch_read(lru->nwvp_handle,
						lru->block,
						&count, &map[0]);
    if (!map_ccode && count)
    {
       lru->mirror_count = (count % 8);
       for (j=0; j < (int)(count % 8); j++)
       {
	  lru->lba[j] = map[j].sector_offset;
	  lru->disk[j] = map[j].disk_id;
       }
    }

    NWFSSet(&lru->io[0], 0, sizeof(ASYNCH_IO));
    lru->io[0].command = ASYNCH_READ;

    // We typically don't want to interleave 4K reads for a
    // complete cluster across mirrors because a cluster is
    // almost always a contiguous run of sectors.  We allow
    // the LRU reader to pass a mirror context pointer
    // where we can record the last mirror index and use
    // it for subsequent calls to fill any blocks that may
    // all reside within the same cluster.

    if (mirror)
    {
       if (*mirror < count)
          mirror_index = *mirror;
       else
       {
          mirror_index = mirror_counter % lru->mirror_count;
          *mirror = mirror_index;
       }
    }
    else
       mirror_index = mirror_counter % lru->mirror_count;
    mirror_counter++;

    lru->io[0].disk = lru->disk[mirror_index];
    lru->io[0].flags = ASIO_INTR_CALLBACK;
    lru->io[0].sector_offset = lru->lba[mirror_index];
    lru->io[0].sector_count = 8;
    lru->io[0].buffer = lru->buffer;
    lru->io[0].call_back_routine = release_asynch_read_ahead;
    lru->io[0].call_back_parameter = (ULONG)lru;

    lru->bad_bits = map[mirror_index].bad_bits;

    insert_io(lru->io[0].disk, &lru->io[0]);
    mirror_counter++;

    disks[(lru->io[0].disk % 8)] = TRUE;

    return lru;
}

ULONG ReleaseLRU(LRU_HANDLE *lru_handle, LRU *lru)
{
    NWLockLRU((ULONG)ReleaseLRU);

    lru->lock_count--;
    RemoveLRU(lru->lru_handle, lru);
    if (lru->lru_handle == lru_handle) 
       InsertLRUTop(lru->lru_handle, lru);
    else
       InsertLRU(lru->lru_handle, lru);

    NWUnlockLRU((ULONG)ReleaseLRU);
    return 0;
}

ULONG ReleaseDirtyLRU(LRU_HANDLE *lru_handle, LRU *lru)
{
    NWLockLRU((ULONG)ReleaseDirtyLRU);
    
    lru->state &= ~L_UPTODATE;
    lru->state |= L_MODIFIED;
    lru->lock_count--;

#if (LINUX_20 | LINUX_22 | LINUX_24)
    lru->timestamp = GetLRUTime();
#endif
    if (!(lru->state & L_DIRTY))
       InsertDirty(lru);

    RemoveLRU(lru->lru_handle, lru);
    if (lru->lru_handle == lru_handle) 
       InsertLRUTop(lru->lru_handle, lru);
    else
       InsertLRU(lru->lru_handle, lru);
    
    NWUnlockLRU((ULONG)ReleaseDirtyLRU);
    return 0;
}

ULONG SyncLRUBlocks(VOLUME *volume, ULONG block, ULONG blocks)
{
    register ULONG ccode, retCode = 0;
    register ULONG i;
    register LRU *lru;

    if (!blocks)
       return -1;

    NWLockLRU((ULONG)SyncLRUBlocks);
    for (i = 0; i < blocks; i++)
    {
       lru = SearchHash(volume, block + i);
       if (lru)
       {
          if ((lru->state & L_DIRTY)      &&
	      (lru->state & L_DATAVALID)  &&
	      (lru->nwvp_handle)          &&
	      (!(lru->state & L_FLUSHING)))
          {
	     lru->state |= L_FLUSHING;
	     lru->state &= ~L_MODIFIED;
             NWUnlockLRU((ULONG)SyncLRUBlocks);

	     ccode = nwvp_vpartition_block_write(lru->nwvp_handle, 
			                     lru->block, 1, lru->buffer);

             NWLockLRU((ULONG)SyncLRUBlocks);
	     lru->state &= ~L_FLUSHING;
	     lru->timestamp = 0;


	     if (!ccode)
	     {
	        if (!(lru->state & L_MODIFIED))
	           RemoveDirty(lru);
	     }
             else
		retCode = ccode;
	  }
       }
    }
    NWUnlockLRU((ULONG)SyncLRUBlocks);
    return retCode;

}

#if (READ_AHEAD_ON)

ULONG PerformBlockReadAhead(LRU_HANDLE *lru_handle, VOLUME *volume,
			    ULONG block, ULONG blockReadAhead, ULONG ccount,
			    ULONG *mirror)
{
    register ULONG i;
    register LRU *lru;
#if (DO_ASYNCH_IO && ASYNCH_READ_AHEAD && !DOS_UTIL && !WINDOWS_NT_UTIL && !LINUX_UTIL)
    BYTE disks[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
#endif

    if (!blockReadAhead)
       return -1;

    NWLockLRU((ULONG)PerformBlockReadAhead);
    for (i = 0; i < blockReadAhead; i++)
    {
       lru = SearchHash(volume, block + i);
       if (!lru)
#if (DO_ASYNCH_IO && ASYNCH_READ_AHEAD && !DOS_UTIL && !WINDOWS_NT_UTIL && !LINUX_UTIL)
	  FillBlockAsynch(lru_handle, volume, block + i, ccount, disks,
	  		  mirror);
#else
	  FillBlock(lru_handle, volume, block + i, 1, ccount, mirror);
#endif
    }
    NWUnlockLRU((ULONG)PerformBlockReadAhead);

#if (DO_ASYNCH_IO && ASYNCH_READ_AHEAD && !DOS_UTIL && !WINDOWS_NT_UTIL && !LINUX_UTIL)
    for (i=0; i < 8; i++)
    {
       if (disks[i])
          RunAsynchIOQueue(i);
    }
#endif

    return 0;

}

#endif

LRU *ReadLRU(LRU_HANDLE *lru_handle, VOLUME *volume, ULONG block, 
	     ULONG fill, ULONG ccount, ULONG readAhead,
	     ULONG *mirror)
{
#if (DOS_UTIL | LINUX_UTIL | WINDOWS_NT_UTIL)
    register ULONG retries = 0;
#endif
    register LRU *lru;

    // validate the priority of this request and adjust it to something
    // reasonable if we are passed an out of range value.
    // don't allow blocks to recycle more than three times prior to
    // being evicted from the LRU.

    if (!ccount || (ccount > 3))
       ccount = 1;

#if (READ_AHEAD_ON)
    // perform read ahead if we were asked to fill any blocks.
    if (fill && readAhead)
       PerformBlockReadAhead(lru_handle, volume, block + 1, readAhead - 1,
                             ccount, mirror);
#endif

repeat:;
    NWLockLRU((ULONG)ReadLRU);

    lru = SearchHash(volume, block);
    if (lru)
    {
       if (lru->state & L_LOADING)
	  goto WaitOnBuffer;

       // this indicates an I/O error
       if (!(lru->state & L_DATAVALID))
       {
           NWFSPrint("I/O error in Read LRU\n");
           NWUnlockLRU((ULONG)ReadLRU);
           return 0;
       }

       lru->lock_count++;
       NWUnlockLRU((ULONG)ReadLRU);
       return lru;
    }

#if (LINUX_20 | LINUX_22 | LINUX_24)
    if ((LRUDirtyBuffers * PERCENT_FACTOR) >= LRUBlocks)
    {
       if (!lru_signal)
       {
          if (!lru_active)
          {
	     lru_signal = TRUE;
	     NWFSFlush();
          }
       }
    }
#endif
    
    // this call is blocking.
    lru = FillBlock(lru_handle, volume, block, fill,
    		    ccount, mirror);
    if (lru)
    {
       lru->lock_count++;
       NWUnlockLRU((ULONG)ReadLRU);
       return lru;
    }

    NWUnlockLRU((ULONG)ReadLRU);

#if (LINUX_20 | LINUX_22 | LINUX_24)
    lru_state = NWFS_FLUSH_ALL;
    if (!lru_signal)
    {
       if (!lru_active)
       {
	  lru_signal = TRUE;
          lru_state = NWFS_FLUSH_ALL;
	  NWFSFlush();
       }
    }

    schedule();
    goto repeat;

#else
    FlushLRU();
    goto repeat;
#endif

#if (LINUX_20 | LINUX_22 | LINUX_24)

WaitOnBuffer:;
    lru->Waiters++;
    NWUnlockLRU((ULONG)ReadLRU);

    NWLockBuffer(lru);
    if (!(lru->state & L_DATAVALID) ||
	 (lru->block != block)      ||
	 (lru->volnum != volume->VolumeNumber))
       return 0;

    goto repeat;

#endif

#if (DOS_UTIL | LINUX_UTIL | WINDOWS_NT_UTIL)

WaitOnBuffer:;
    FlushLRU();

    if (retries++ > 5)
       return 0;

    goto repeat;
#endif

}

void DisplayLRUInfo(LRU_HANDLE *lru_handle)
{
    register LRU *lru;

    lru_handle->dirty = 0;
    lru_handle->loading = 0;
    lru_handle->flushing = 0;
    lru_handle->locked = 0;
    lru_handle->valid = 0;
    lru_handle->uptodate = 0;
    lru_handle->total = 0;
    lru_handle->free = 0;

    lru = ListHead;
    while (lru)
    {
       if (lru->lru_handle == lru_handle)
       {
          lru_handle->total++;
	    
          if (lru->state & L_FREE)      
	     lru_handle->free++;

          if (lru->state & L_DIRTY)      
	     lru_handle->dirty++;

          if (lru->state & L_DATAVALID)  
	     lru_handle->valid++;

          if (lru->state & L_UPTODATE)  
             lru_handle->uptodate++;

          if (lru->state & L_FLUSHING)
	     lru_handle->flushing++;

          if (lru->state & L_LOADING)
	     lru_handle->loading++;
       
          if (lru->lock_count)
	     lru_handle->locked++;
       }
       lru = lru->listnext;
    }

    NWFSPrint("%-10s drt-%d vld-%d utod-%d flsh-%d ld-%d lk-%d free-%d t-%d/%d\n",
              lru_handle->LRUName, 
	      (int)lru_handle->dirty, 
	      (int)lru_handle->valid, 
	      (int)lru_handle->uptodate,
	      (int)lru_handle->flushing, 
	      (int)lru_handle->loading, 
	      (int)lru_handle->locked, 
	      (int)lru_handle->free, 
	      (int)lru_handle->total,
	      (int)lru_handle->LocalLRUBlocks);

    return;
}

BYTE *get_taddress(ULONG addr)
{
   if (addr == (ULONG)FreeLRU)
      return "FreeLRU";
   else
   if (addr == (ULONG)FlushLRU)
      return "FlushLRU";
   else
   if (addr == (ULONG)FlushEligibleLRU)
      return "FlushEligibleLRU";
   else
   if (addr == (ULONG)release_write_aio)
      return "release_write_aio";
   else
   if (addr == (ULONG)retry_write_aio)
      return "retry_write_aio";
   else
   if (addr == (ULONG)FlushVolumeLRU)
      return "FlushVolumeLRU";
   else
   if (addr == (ULONG)GetAvailableLRU)
      return "GetAvailableLRU";
   else
   if (addr == (ULONG)FillBlock)
      return "FillBlock";
   else
   if (addr == (ULONG)retry_asynch_read_ahead)
      return "retry_asynch_read_ahead";
   else
   if (addr == (ULONG)release_asynch_read_ahead)
      return "release_asynch_read_ahead";
   else
   if (addr == (ULONG)ReleaseLRU)
      return "ReleaseLRU";
   else
   if (addr == (ULONG)ReleaseDirtyLRU)
      return "ReleaseDirtyLRU";
   else
   if (addr == (ULONG)SyncLRUBlocks)
      return "SyncLRUBlocks";
   else
   if (addr == (ULONG)PerformBlockReadAhead)
      return "PerformBlockReadAhead";
   else
   if (addr == (ULONG)ReadLRU)
      return "ReadLRU";
   else
      return "unknown";
	   
}
