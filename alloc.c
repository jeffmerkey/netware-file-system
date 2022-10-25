
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
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  ALLOC.C
*   DESCRIP  :   Operating System Abstraction Layer
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#include "globals.h"

// we track our memory to check for leaks at unload.

#define ALLOC_LIMIT   -1

typedef struct _MEM_HEADER
{
   struct _MEM_HEADER *next;
   struct _MEM_HEADER *prior;
   ULONG Size;
   TRACKING *Type;
   ULONG Pool;
   void *context;
} MEM_HEADER;

MEM_HEADER *MemoryListHead = 0;
MEM_HEADER *MemoryListTail = 0;

ULONG AddToMemoryList(MEM_HEADER *mem);
ULONG RemoveFromMemoryList(MEM_HEADER *mem);

extern TRACKING *tracked_memory[];
extern ULONG tracked_memory_count;

void displayMemoryInfo(void)
{
   register ULONG i, count;

   NWFSPrint("NWFS Memory Usage\n");
   NWFSPrint("total_memory_in_use     : %d bytes\n", (int)(MemoryInUse + 
		                                          PagedMemoryInUse));
   NWFSPrint("atomic_memory_in_use    : %d bytes\n", (int)MemoryInUse);
   NWFSPrint("paged_memory_in_use     : %d bytes\n", (int)PagedMemoryInUse);
   NWFSPrint("atomic_memory_allocated : %d bytes\n", (int)MemoryAllocated);
   NWFSPrint("atomic_memory_freed     : %d bytes\n", (int)MemoryFreed);
   NWFSPrint("paged_memory_allocated  : %d bytes\n", (int)PagedMemoryAllocated);
   NWFSPrint("paged_memory_freed      : %d bytes\n", (int)PagedMemoryFreed);

   for (count=i=0; i < tracked_memory_count; i++)
   {
      if ((tracked_memory[i]) && 
	 (tracked_memory[i]->count || tracked_memory[i]->units))
      {
         NWFSPrint("%s count-%-10d units-%-10d  ", 
		 tracked_memory[i]->label, (int)tracked_memory[i]->count,
		 (int)tracked_memory[i]->units);
         if (count++ & 1)
	    NWFSPrint("\n");
      }
   }
   NWFSPrint("\n");

}

#if (LINUX_24)

#define toupper(c) (((c) >= 'a' && (c) <= 'z') ? (c) + ('A'-'a') : (c))
#define tolower(c) (((c) >= 'A' && (c) <= 'Z') ? (c)-('A'-'a') : (c))

ULONG AllocateSemaphore(void *sema, ULONG value)
{
    sema_init(sema, value);
    return 0;
}

long WaitOnSemaphore(void *sema)
{
#if (DEBUG_DEADLOCKS)
    register ULONG ccode;

    ccode = down_interruptible(sema);
    if (ccode == -EINTR)
       return ccode;
    return 0;
#else
    down(sema);
    return 0;
#endif
}

long SignalSemaphore(void *sema)
{
    up(sema);
    return 0;
}

void *NWFSAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = kmalloc((size + sizeof(MEM_HEADER)), GFP_KERNEL);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 1;
   mem->context = 0;
   
   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSIOAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = kmalloc((size + sizeof(MEM_HEADER)), GFP_KERNEL);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 3;
   mem->context = 0;
   
   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSCacheAlloc(ULONG size, TRACKING *Tag)
{
   register ULONG nSize;
   register MEM_HEADER *mem;
   register BYTE *p;

   nSize = (((size + sizeof(MEM_HEADER)) + 4095) / 4096) * 4096;
   mem = vmalloc(nSize);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 2;
   mem->context = 0;
   
   PagedMemoryAllocated += size;
   PagedMemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;
}

void NWFSFree(void *p)
{
    register MEM_HEADER *mem, *mem1;

    mem1 = p;
    mem = &mem1[-1];

    mem->Type->count -= mem->Size;
    mem->Type->units--;

    switch (mem->Pool)
    {
       case 1:
      MemoryFreed += mem->Size;
      if (MemoryInUse)
         MemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      kfree((void *)mem);
      break;

       case 2:
      PagedMemoryFreed += mem->Size;
      if (PagedMemoryInUse)
         PagedMemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      vfree((void *)mem);
      break;

       case 3:
      MemoryFreed += mem->Size;
      if (MemoryInUse)
         MemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      kfree((void *)mem);
      break;

       default:
      NWFSPrint("nwfs:  NWFSFree reports free on invalid memory\n");
      break;
    }
}

int NWFSCompare(void *dest, void *src, ULONG size)
{
    return (memcmp(dest, src, size));
}

void NWFSSet(void *dest, ULONG value, ULONG size)
{
    memset(dest, value, size);
}

void NWFSCopy(void *dest, void *src, ULONG size)
{
    memmove(dest, src, size);
}

ULONG NWFSCopyFromUserSpace(void *dest, void *src, ULONG size)
{
    return (copy_from_user(dest, src, size));
}

ULONG NWFSCopyToUserSpace(void *dest, void *src, ULONG size)
{
    return (copy_to_user(dest, src, size));
}

ULONG NWFSSetUserSpace(void *dest, ULONG value, ULONG size)
{
    if (size > 65536)
       return NwMemoryFault;

    if (ZeroBuffer)
       return (copy_to_user(dest, ZeroBuffer, size));
    return NwMemoryFault;
}

BYTE NWFSUpperCase(BYTE c)
{
    return (toupper(c));
}

BYTE NWFSLowerCase(BYTE c)
{
    return (tolower(c));
}

void NWFSClean(void *p)
{
   kfree((void *)p);
}

#endif


#if (LINUX_22)

#define toupper(c) (((c) >= 'a' && (c) <= 'z') ? (c) + ('A'-'a') : (c))
#define tolower(c) (((c) >= 'A' && (c) <= 'Z') ? (c)-('A'-'a') : (c))

ULONG AllocateSemaphore(void *sema, ULONG value)
{
    struct semaphore *semaphore = sema;

    NWFSSet(semaphore, 0, sizeof(struct semaphore));
    semaphore->count.counter = value;
    return 0;
}

long WaitOnSemaphore(void *sema)
{
#if (DEBUG_DEADLOCKS)
    register ULONG ccode;

    ccode = down_interruptible(sema);
    if (ccode == -EINTR)
       return ccode;
    return 0;
#else
    down(sema);
    return 0;
#endif
}

long SignalSemaphore(void *sema)
{
    up(sema);
    return 0;
}

void *NWFSAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   if ((size + sizeof(MEM_HEADER)) > ALLOC_LIMIT)
      NWFSPrint("alloc > limit (%d) %s\n", (int)(size + sizeof(MEM_HEADER)),
		      (BYTE *)Tag->label);
   
   mem = kmalloc((size + sizeof(MEM_HEADER)), GFP_ATOMIC);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 1;

   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSIOAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   if ((size + sizeof(MEM_HEADER)) > ALLOC_LIMIT)
      NWFSPrint("io_alloc > limit (%d) %s\n", (int)(size + sizeof(MEM_HEADER)),
		      (BYTE *)Tag->label);
   
   mem = kmalloc((size + sizeof(MEM_HEADER)), GFP_ATOMIC);
   if (!mem)
      return 0;
   
   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 1;

   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSCacheAlloc(ULONG size, TRACKING *Tag)
{
   register ULONG nSize;
   register MEM_HEADER *mem;
   register BYTE *p;

   nSize = size + sizeof(MEM_HEADER);
   if (nSize > ALLOC_LIMIT)
      NWFSPrint("cache_alloc > limit (%d) %s\n", (int)nSize, 
		      (BYTE *)Tag->label);

   nSize = (((size + sizeof(MEM_HEADER)) + 4095) / 4096) * 4096;
   mem = vmalloc(nSize);

//   mem = kmalloc(nSize, GFP_ATOMIC);
//   if (!mem)
//      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 2;

   PagedMemoryAllocated += size;
   PagedMemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;
}

void NWFSFree(void *p)
{
    register MEM_HEADER *mem, *mem1;

    mem1 = p;
    mem = &mem1[-1];

    mem->Type->count -= mem->Size;
    mem->Type->units--;

    switch (mem->Pool)
    {
       case 1:
      MemoryFreed += mem->Size;
      if (MemoryInUse)
         MemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      kfree((void *)mem);
      break;

       case 2:
      PagedMemoryFreed += mem->Size;
      if (PagedMemoryInUse)
         PagedMemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      vfree((void *)mem);
      break;

       default:
      NWFSPrint("nwfs:  NWFSFree reports free on invalid memory\n");
      break;
    }
}

int NWFSCompare(void *dest, void *src, ULONG size)
{
    return (memcmp(dest, src, size));
}

void NWFSSet(void *dest, ULONG value, ULONG size)
{
    memset(dest, value, size);
}

void NWFSCopy(void *dest, void *src, ULONG size)
{
    memmove(dest, src, size);
}

ULONG NWFSCopyFromUserSpace(void *dest, void *src, ULONG size)
{
    return (copy_from_user(dest, src, size));
}

ULONG NWFSCopyToUserSpace(void *dest, void *src, ULONG size)
{
    return (copy_to_user(dest, src, size));
}

ULONG NWFSSetUserSpace(void *dest, ULONG value, ULONG size)
{
    if (size > 65536)
       return NwMemoryFault;

    if (ZeroBuffer)
       return (copy_to_user(dest, ZeroBuffer, size));
    return NwMemoryFault;
}

BYTE NWFSUpperCase(BYTE c)
{
    return (toupper(c));
}

BYTE NWFSLowerCase(BYTE c)
{
    return (tolower(c));
}

void NWFSClean(void *p)
{
   kfree((void *)p);
}

#endif


#if (LINUX_20)

#define toupper(c) (((c) >= 'a' && (c) <= 'z') ? (c) + ('A'-'a') : (c))
#define tolower(c) (((c) >= 'A' && (c) <= 'Z') ? (c)-('A'-'a') : (c))

ULONG AllocateSemaphore(void *sema, ULONG value)
{
    struct semaphore *semaphore = sema;

    NWFSSet(semaphore, 0, sizeof(struct semaphore));
    semaphore->count = value;
    return 0;
}

long WaitOnSemaphore(void *sema)
{
#if (DEBUG_DEADLOCKS)
    register ULONG ccode;

    ccode = down_interruptible(sema);
    if (ccode == -EINTR)
       return ccode;
    return 0;
#else
    down(sema);
    return 0;
#endif
}

long SignalSemaphore(void *sema)
{
    up(sema);
    return 0;
}

void *NWFSAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = kmalloc((size + sizeof(MEM_HEADER)), GFP_KERNEL);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 1;

   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSIOAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = kmalloc((size + sizeof(MEM_HEADER)), GFP_BUFFER);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 1;

   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSCacheAlloc(ULONG size, TRACKING *Tag)
{
   register ULONG nSize;
   register MEM_HEADER *mem;
   register BYTE *p;

   nSize = (((size + sizeof(MEM_HEADER)) + 4095) / 4096) * 4096;
   mem = vmalloc(nSize);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 2;

   PagedMemoryAllocated += size;
   PagedMemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;
}

void NWFSFree(void *p)
{
    register MEM_HEADER *mem, *mem1;

    mem1 = p;
    mem = &mem1[-1];

    mem->Type->count -= mem->Size;
    mem->Type->units--;

    switch (mem->Pool)
    {
       case 1:
      MemoryFreed += mem->Size;
      if (MemoryInUse)
	 MemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      kfree((void *)mem);
      break;

       case 2:
      PagedMemoryFreed += mem->Size;
      if (PagedMemoryInUse)
         PagedMemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      vfree((void *)mem);
      break;

       default:
      NWFSPrint("nwfs:  NWFSFree reports free on invalid memory\n");
      break;
    }
}

int NWFSCompare(void *dest, void *src, ULONG size)
{
    return (memcmp(dest, src, size));
}

void NWFSSet(void *dest, ULONG value, ULONG size)
{
    memset(dest, value, size);
}

void NWFSCopy(void *dest, void *src, ULONG size)
{
    memmove(dest, src, size);
}

ULONG NWFSCopyFromUserSpace(void *dest, void *src, ULONG size)
{
    return (memcpy_fromfs(dest, src, size));
}

ULONG NWFSCopyToUserSpace(void *dest, void *src, ULONG size)
{
    return (memcpy_tofs(dest, src, size));
}

ULONG NWFSSetUserSpace(void *dest, ULONG value, ULONG size)
{
    if (size > 65536)
       return NwMemoryFault;

    if (ZeroBuffer)
       return (memcpy_tofs(dest, ZeroBuffer, size));
    return NwMemoryFault;
}

BYTE NWFSUpperCase(BYTE c)
{
    return (toupper(c));
}

BYTE NWFSLowerCase(BYTE c)
{
    return (tolower(c));
}

void NWFSClean(void *p)
{
   kfree((void *)p);
}

#endif



#if (LINUX_UTIL)

ULONG AllocateSemaphore(void *sema, ULONG value)
{
    return 0;
}

long WaitOnSemaphore(void *sema)
{
    return 0;
}

long SignalSemaphore(void *sema)
{
    return 0;
}

void *NWFSAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = malloc((size + sizeof(MEM_HEADER)));
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 1;

   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSIOAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = malloc((size + sizeof(MEM_HEADER)));
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 1;

   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSCacheAlloc(ULONG size, TRACKING *Tag)
{
   register ULONG nSize;
   register MEM_HEADER *mem;
   register BYTE *p;

   nSize = (((size + sizeof(MEM_HEADER)) + 4095) / 4096) * 4096;
   mem = malloc(nSize);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 2;

   PagedMemoryAllocated += size;
   PagedMemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;
}

void NWFSFree(void *p)
{
    register MEM_HEADER *mem, *mem1;

    mem1 = p;
    mem = &mem1[-1];

    mem->Type->count -= mem->Size;
    mem->Type->units--;

    switch (mem->Pool)
    {
       case 1:
      MemoryFreed += mem->Size;
      if (MemoryInUse)
         MemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      free((void *)mem);
      break;

       case 2:
      PagedMemoryFreed += mem->Size;
      if (PagedMemoryInUse)
         PagedMemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      free((void *)mem);
      break;

       default:
      NWFSPrint("nwfs:  NWFSFree reports free on invalid memory\n");
      break;
    }
}

int NWFSCompare(void *dest, void *src, ULONG size)
{
    return (memcmp(dest, src, size));
}

void NWFSSet(void *dest, ULONG value, ULONG size)
{
    memset(dest, value, size);
}

void NWFSCopy(void *dest, void *src, ULONG size)
{
    memmove(dest, src, size);
}

ULONG NWFSCopyFromUserSpace(void *dest, void *src, ULONG size)
{
    memcpy(dest, src, size);
    return 0;
}

ULONG NWFSCopyToUserSpace(void *dest, void *src, ULONG size)
{
    memcpy(dest, src, size);
    return 0;
}

ULONG NWFSSetUserSpace(void *src, ULONG value, ULONG size)
{
    if (size > 65536)
       return NwMemoryFault;

    memset(src, value, size);
    return 0;
}

BYTE NWFSUpperCase(BYTE c)
{
    return (toupper(c));
}

BYTE NWFSLowerCase(BYTE c)
{
    return (tolower(c));
}

void NWFSClean(void *p)
{
   free((void *)p);
}

#endif



#if (WINDOWS_NT | WINDOWS_NT_RO)

ULONG AllocateSemaphore(void *sema, ULONG value)
{
    return 0;
}

long WaitOnSemaphore(void *sema)
{
    return 0;
}

long SignalSemaphore(void *sema)
{
    return 0;
}

void *NWFSAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = ExAllocatePoolWithTag(PagedPool,
                   size + sizeof(MEM_HEADER),
                   NT_POOL_TAG);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSIOAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = ExAllocatePoolWithTag(PagedPoolCacheAligned,
                   size + sizeof(MEM_HEADER),
                   NT_POOL_TAG);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSCacheAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = ExAllocatePoolWithTag(PagedPoolCacheAligned,
                   size + sizeof(MEM_HEADER),
                   NT_POOL_TAG);
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;
}

void NWFSFree(void *p)
{
   register MEM_HEADER *mem, *mem1;

   mem1 = p;
   mem = &mem1[-1];

   mem->Type->count -= mem->Size;
   mem->Type->units--;

   MemoryFreed += mem->Size;
   if (MemoryInUse)
      MemoryInUse -= mem->Size;

   RemoveFromMemoryList(mem);

   ExFreePool((void *)mem);
}

int NWFSCompare(void *dest, void *src, ULONG size)
{
    return (memcmp(dest, src, size));
}

void NWFSSet(void *dest, ULONG value, ULONG size)
{
    memset(dest, value, size);
}

void NWFSCopy(void *dest, void *src, ULONG size)
{
    memmove(dest, src, size);
}

ULONG NWFSCopyFromUserSpace(void *dest, void *src, ULONG size)
{
   memmove(dest, src, size);
   return 0;
}

ULONG NWFSCopyToUserSpace(void *dest, void *src, ULONG size)
{
   memmove(dest, src, size);
   return 0;
}

ULONG NWFSSetUserSpace(void *dest, ULONG value, ULONG size)
{
   if (size > 65536)
      return NwMemoryFault;

   memset(dest, value, size);
   return 0;
}

void NWFSClean(void *p)
{
   ExFreePool((void *)p);
}

#endif



#if (WINDOWS_NT_UTIL)

ULONG AllocateSemaphore(void *sema, ULONG value)
{
    return 0;
}

long WaitOnSemaphore(void *sema)
{
    return 0;
}

long SignalSemaphore(void *sema)
{
    return 0;
}

void *NWFSAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = malloc(size + sizeof(MEM_HEADER));
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSIOAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = malloc(size + sizeof(MEM_HEADER));
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSCacheAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = malloc(size + sizeof(MEM_HEADER));
   if (!mem)
      return 0;

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;
}

void NWFSFree(void *p)
{
   register MEM_HEADER *mem, *mem1;

   mem1 = p;
   mem = &mem1[-1];

   mem->Type->count -= mem->Size;
   mem->Type->units--;

   MemoryFreed += mem->Size;
   if (MemoryInUse)
      MemoryInUse -= mem->Size;

   RemoveFromMemoryList(mem);

   free((void *)mem);
}

int NWFSCompare(void *dest, void *src, ULONG size)
{
    return (memcmp(dest, src, size));
}

void NWFSSet(void *dest, ULONG value, ULONG size)
{
    memset(dest, value, size);
}

void NWFSCopy(void *dest, void *src, ULONG size)
{
    memmove(dest, src, size);
}

ULONG NWFSCopyFromUserSpace(void *dest, void *src, ULONG size)
{
   memmove(dest, src, size);
   return 0;
}

ULONG NWFSCopyToUserSpace(void *dest, void *src, ULONG size)
{
   memmove(dest, src, size);
   return 0;
}

ULONG NWFSSetUserSpace(void *dest, ULONG value, ULONG size)
{
   if (size > 65536)
      return NwMemoryFault;

   memset(dest, value, size);
   return 0;
}

void NWFSClean(void *p)
{
   free((void *)p);
}

#endif


#if (DOS_UTIL)

ULONG AllocateSemaphore(void *sema, ULONG value)
{
    return 0;
}

long WaitOnSemaphore(void *sema)
{
    return 0;
}

long SignalSemaphore(void *sema)
{
    return 0;
}

void *NWFSAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = malloc((size + sizeof(MEM_HEADER)));
   if (!mem)
   {
      NWFSPrint("alloc failed [%-4s]\n", (char *)Tag ? (char *)Tag : "NONE");
      return 0;
   }

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 1;

   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSIOAlloc(ULONG size, TRACKING *Tag)
{
   register MEM_HEADER *mem;
   register BYTE *p;

   mem = malloc((size + sizeof(MEM_HEADER)));
   if (!mem)
   {
      NWFSPrint("alloc failed [%-4s]\n", (char *)Tag ? (char *)Tag : "NONE");
      return 0;
   }

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 1;

   MemoryAllocated += size;
   MemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;

}

void *NWFSCacheAlloc(ULONG size, TRACKING *Tag)
{
   register ULONG nSize;
   register MEM_HEADER *mem;
   register BYTE *p;

   nSize = (((size + sizeof(MEM_HEADER)) + 4095) / 4096) * 4096;
   mem = malloc(nSize);
   if (!mem)
   {
      NWFSPrint("cache alloc failed [%-4s]\n", (char *)Tag ? (char *)Tag : "NONE");
      return 0;
   }

   p = (BYTE *)((ULONG)mem + sizeof(MEM_HEADER));
   mem->Size = size;
   mem->Type = Tag;
   mem->next = 0;
   mem->prior = 0;
   mem->Pool = 2;

   PagedMemoryAllocated += size;
   PagedMemoryInUse += size;

   Tag->count += size;
   Tag->units++;
   
   AddToMemoryList(mem);

   return (void *) p;
}

void NWFSFree(void *p)
{
    register MEM_HEADER *mem, *mem1;

    mem1 = p;
    mem = &mem1[-1];

    mem->Type->count -= mem->Size;
    mem->Type->units--;

    switch (mem->Pool)
    {
       case 1:
      MemoryFreed += mem->Size;
      if (MemoryInUse)
         MemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      free((void *)mem);
      break;

       case 2:
      PagedMemoryFreed += mem->Size;
      if (PagedMemoryInUse)
         PagedMemoryInUse -= mem->Size;
      RemoveFromMemoryList(mem);
      free((void *)mem);
      break;

       default:
      NWFSPrint("nwfs:  NWFSFree reports free on invalid memory\n");
      break;
    }
}

int NWFSCompare(void *dest, void *src, ULONG size)
{
    return (memcmp(dest, src, size));
}

void NWFSSet(void *dest, ULONG value, ULONG size)
{
    memset(dest, value, size);
}

void NWFSCopy(void *dest, void *src, ULONG size)
{
    memmove(dest, src, size);
}

ULONG NWFSCopyFromUserSpace(void *dest, void *src, ULONG size)
{
    memcpy(dest, src, size);
    return 0;
}

ULONG NWFSCopyToUserSpace(void *dest, void *src, ULONG size)
{
    memcpy(dest, src, size);
    return 0;
}

ULONG NWFSSetUserSpace(void *src, ULONG value, ULONG size)
{
    if (size > 65536)
       return NwMemoryFault;

    memset(src, value, size);
    return 0;
}

BYTE NWFSUpperCase(BYTE c)
{
    return (toupper(c));
}

BYTE NWFSLowerCase(BYTE c)
{
    return (tolower(c));
}

void NWFSClean(void *p)
{
   free((void *)p);
}

#endif

void displayMemoryList(void)
{
    BYTE *p;
    MEM_HEADER *mem, *temp;
    extern BYTE *dumpRecord(BYTE *, ULONG);
    extern BYTE *dumpRecordBytes(BYTE *, ULONG);
    
    mem = MemoryListHead;
    while (mem)
    {
       p = (BYTE *) &mem->Type;
#if (DUMP_LEAKED_MEMORY)
       NWFSPrint("addr-%08X type-[%s] size-%d\n",
         (unsigned)((ULONG)mem + sizeof(MEM_HEADER)), 
	 (char *)mem->Type->label ? (char *)mem->Type->label : "NONE",
         (int)mem->Size);

       NWFSPrint("address-0x%08X (DWORD dump)\n", 
		 (unsigned)((ULONG)mem + sizeof(MEM_HEADER)));
       dumpRecord((BYTE *)((ULONG)mem + sizeof(MEM_HEADER)), 64);
       NWFSPrint("address-0x%08X (BYTE dump)\n", 
		 (unsigned)((ULONG)mem + sizeof(MEM_HEADER)));
       dumpRecordBytes((BYTE *)((ULONG)mem + sizeof(MEM_HEADER)), 64);
#endif
       
       temp = mem;
       mem = mem->next;
       NWFSClean(temp);  // free the tracked memory
    }
    return;
}

ULONG AddToMemoryList(MEM_HEADER *mem)
{
    if (!MemoryListHead)
    {
       MemoryListHead = mem;
       MemoryListTail = mem;
       mem->next = mem->prior = 0;
    }
    else
    {
       MemoryListTail->next = mem;
       mem->next = 0;
       mem->prior = MemoryListTail;
       MemoryListTail = mem;
    }
    return 0;
}

ULONG RemoveFromMemoryList(MEM_HEADER *mem)
{
    if (MemoryListHead == mem)
    {
       MemoryListHead = mem->next;
       if (MemoryListHead)
      MemoryListHead->prior = NULL;
       else
      MemoryListTail = NULL;
    }
    else
    {
       mem->prior->next = mem->next;
       if (mem != MemoryListTail)
      mem->next->prior = mem->prior;
       else
      MemoryListTail = mem->prior;
    }
    return 0;
}



