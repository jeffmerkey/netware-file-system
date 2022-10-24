
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
*   AUTHORS  :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  BLOCK.C
*   DESCRIP  :   Block Device Interface Module
*   DATE     :  December 2, 1998
*
*
***************************************************************************/

#include "globals.h"

BYTE *ZeroBuffer = 0;

#if (FILE_DISK_EMULATION)

void SyncDevice(ULONG disk)
{
    return;
}

uint32	pReadDiskSectors(
	uint32	disk_id,
	uint32	sector_offset,
	uint8	*buffer,
	uint32	sector_count,
	uint32	extra)
{
#if (MULTI_MACHINE_EMULATION)
	m2machine	*machine;
	NWDISK	**SystemDisk;
	m2group_get_context(&machine);
	SystemDisk = (NWDISK **) &machine->LSystemDisk;
#endif
	if (extra) {};
	if (SystemDisk[disk_id] != 0)
	{
		fseek((FILE *) SystemDisk[disk_id]->PhysicalDiskHandle, sector_offset * 512, 0);
		return (fread(buffer, 512, sector_count, (FILE *) SystemDisk[disk_id]->PhysicalDiskHandle));
	}
	return(0);
}

uint32	pWriteDiskSectors(
	uint32	disk_id,
	uint32	sector_offset,
	uint8	*buffer,
	uint32	sector_count,
	uint32	extra)
{
#if (MULTI_MACHINE_EMULATION)
	m2machine	*machine;
	NWDISK	**SystemDisk;
	m2group_get_context(&machine);
	SystemDisk = (NWDISK **) &machine->LSystemDisk;
#endif
	if (extra) {};
	if (SystemDisk[disk_id] != 0)
	{
		fseek((FILE *) SystemDisk[disk_id]->PhysicalDiskHandle, sector_offset * 512, 0);
		return (fwrite(buffer, 512, sector_count, (FILE *) SystemDisk[disk_id]->PhysicalDiskHandle));

	}
	return(0);
}


ULONG pZeroFillDiskSectors(
	uint32	disk_id,
	uint32	sector_offset,
	uint32	sector_count,
	uint32	extra)
{
#if (MULTI_MACHINE_EMULATION)
	m2machine	*machine;
	NWDISK	**SystemDisk;
	m2group_get_context(&machine);
	SystemDisk = (NWDISK **) &machine->LSystemDisk;
#endif
	if (extra) {};
	if (SystemDisk[disk_id] != 0)
	{
		fseek((FILE *) SystemDisk[disk_id]->PhysicalDiskHandle, sector_offset * 512, 0);
		return (fwrite(ZeroBuffer, 512, sector_count, (FILE *) SystemDisk[disk_id]->PhysicalDiskHandle));

	}
	return(0);
}

ULONG ReadDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
				 ULONG sectors, ULONG readAhead)
{
	return (pReadDiskSectors(disk, LBA, Sector, sectors, readAhead));
}

ULONG WriteDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
				  ULONG sectors, ULONG readAhead)
{
	return (pWriteDiskSectors(disk, LBA, Sector, sectors, readAhead));
}

ULONG ZeroFillDiskSectors(ULONG disk, ULONG StartingLBA,
				 ULONG sectors, ULONG readAhead)
{
	return (pZeroFillDiskSectors(disk, StartingLBA, sectors, readAhead));
}



#else

#if (LINUX_20 | LINUX_22 | LINUX_24)

extern ULONG insert_io(ULONG disk, ASYNCH_IO *io);
extern void RunAsynchIOQueue(ULONG disk);
extern void profile_complete(void);
void insert_callback(ASYNCH_IO *io);

#if (LINUX_24)
DECLARE_WAIT_QUEUE_HEAD(wait_for_bh);
#elif (LINUX_20 | LINUX_22)
struct wait_queue *wait_for_bh = NULL;
#endif

#if (LINUX_22 | LINUX_24)
kmem_cache_t *bh_p = 0;
#endif

//
//  low level disk sector interface routines
//

#define AIO_SIGNATURE  (ULONG)"AIOD"

struct buffer_head *bh_head = 0;
struct buffer_head *bh_tail = 0;
ULONG bh_count = 0;
ULONG bh_waiters = 0;

#if (LINUX_SLEEP)
#if (LINUX_SPIN)
spinlock_t bh_spinlock = SPIN_LOCK_UNLOCKED;
long bh_flags = 0;
#else
NWFSInitMutex(bh_semaphore);
#endif
#endif

void lock_bh_free(void)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&bh_spinlock, bh_flags);
#else
    if (WaitOnSemaphore(&bh_semaphore) == -EINTR)
       NWFSPrint("lock bh was interrupted\n");
#endif
#endif
}

void unlock_bh_free(void)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&bh_spinlock, bh_flags);
#else
    SignalSemaphore(&bh_semaphore);
#endif
#endif
}


void nwfs_put_bh(struct buffer_head *bh)
{
    lock_bh_free();
    if (!bh_head)
    {
       bh_head = bh_tail = bh;
       bh->b_next_free = bh->b_prev_free = 0;
    }
    else
    {
       bh_tail->b_next_free = bh;
       bh->b_next_free = 0;
       bh->b_prev_free = bh_tail;
       bh_tail = bh;
    }
    unlock_bh_free();

    // wake up any waiters
    wake_up(&wait_for_bh);
    
    return;
}

struct buffer_head *int_get_bh(void)
{
    struct buffer_head *bh;

    lock_bh_free();
    if (bh_head)
    {
       bh = bh_head;
       bh_head = bh->b_next_free;
       if (bh_head)
	  bh_head->b_prev_free = NULL;
       else
	  bh_tail = NULL;

       bh->b_next_free = bh->b_prev_free = 0;
       unlock_bh_free();
       return bh;
    }
    else 
    if (bh_count < MAX_BUFFER_HEADS)
    {
       unlock_bh_free();

#if (LINUX_20)
       bh = (struct buffer_head *)kmalloc(sizeof(struct buffer_head), 
		                          GFP_ATOMIC);
       
       BH_TRACKING.count += sizeof(struct buffer_head);
       BH_TRACKING.units++;
       MemoryInUse += sizeof(struct buffer_head);

#elif (LINUX_22 | LINUX_24)
       bh = (struct buffer_head *)kmem_cache_alloc(bh_p, SLAB_KERNEL);
       
       BH_TRACKING.count += sizeof(struct buffer_head);
       BH_TRACKING.units++;
       MemoryInUse += sizeof(struct buffer_head);

#else
       bh = (struct buffer_head *)NWFSIOAlloc(sizeof(struct buffer_head), BH_TAG);
#endif
       if (!bh)
	  return 0;

       bh_count++; 

       NWFSSet(bh, 0, sizeof(struct buffer_head));

#if (LINUX_24)
       init_waitqueue_head(&bh->b_wait); 
#endif
       return bh;
    }
    unlock_bh_free();
    return 0;
}

void free_bh_list(void)
{
    struct buffer_head *bh;

    lock_bh_free();
    while (bh_head)
    {
       bh = bh_head;
       bh_head = bh->b_next_free;

#if (LINUX_20)
       kfree(bh);

       BH_TRACKING.count -= sizeof(struct buffer_head);
       BH_TRACKING.units--;
       MemoryInUse -= sizeof(struct buffer_head);

#elif (LINUX_22 | LINUX_24)
       if (bh_p)
       {
          kmem_cache_free(bh_p, bh);

	  BH_TRACKING.count -= sizeof(struct buffer_head);
          BH_TRACKING.units--;
          MemoryInUse -= sizeof(struct buffer_head);
       }
#else
       NWFSFree(bh);
#endif
    }
    bh_head = bh_tail = 0;

#if (LINUX_22 | LINUX_24)
    if (bh_p)
    {
       kmem_cache_shrink(bh_p);
#if (LINUX_24)
       kmem_cache_destroy(bh_p);
#endif
    }
    bh_p = 0;
#endif

    bh_count = 0;
    unlock_bh_free();
    return;
}

struct buffer_head *__get_bh_wait(void)
{
    struct buffer_head *bh;
#if (LINUX_20 | LINUX_22)   
    struct wait_queue wait = { current, NULL };
#elif (LINUX_24)
    DECLARE_WAITQUEUE(wait, current);
#endif
    
#if (LINUX_20 | LINUX_22)
    add_wait_queue(&wait_for_bh, &wait);
#elif (LINUX_24)
    add_wait_queue_exclusive(&wait_for_bh, &wait);
#endif
    
    for (;;)
    {
       current->state = TASK_UNINTERRUPTIBLE;
       bh = int_get_bh();
       if (bh)
	  break;
#if (!POST_IMMEDIATE)

#if (LINUX_22)
       lock_kernel();
#endif       
       run_task_queue(&tq_disk);
#if (LINUX_22)
       unlock_kernel();
#endif       

#endif
       bh_waiters++;
       schedule();
       bh_waiters--;
    }
    remove_wait_queue(&wait_for_bh, &wait);
    current->state = TASK_RUNNING;
    return bh;
}

struct buffer_head *nwfs_get_bh(void)
{
    struct buffer_head *bh;

#if (LINUX_22 | LINUX_24)
    if (!bh_p)
    {
       bh_p = kmem_cache_create("nwfs_buffer_head", sizeof(struct buffer_head),
			        0, SLAB_HWCACHE_ALIGN, NULL, NULL);
       if (!bh_p)
       {
          NWFSPrint("Cannot create buffer head SLAB cache\n");
          unlock_bh_free();
          return 0;
       }
    }
#endif
    
    bh = int_get_bh();
    if (bh)
       return bh;
    return __get_bh_wait();
}

#if (LINUX_BUFFER_CACHE)

//
//  The 2.2.X buffer cache is not SMP thread safe,
//  so lock the kernel when we enter it to prevent memory corruption
//  from occurring on SMP systems.
//

ULONG pReadDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		      ULONG sectors, ULONG readAhead)
{
    register ULONG i, bytesRead = 0;
    register ULONG bps, lba;
    register NWDISK *NWDisk;
    register ULONG read_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    struct buffer_head *bh[sectors];

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;
    
#if (VERBOSE)
    NWFSPrint("read disk-%d lba-%d for %d sectors\n", 
	     (int)disk, (int)StartingLBA, (int)sectors);
#endif

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;

    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read-bc)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }
    
#if (LINUX_22)
    lock_kernel();
#endif

    for (i=0; i < blocks; i++)
    {
       if (!(bh[i] = getblk((ULONG)NWDisk->PhysicalDiskHandle,
			     lba + i, blocksize)))
       {
	  for (i=0; i < blocks; i++)
	     if (bh[i])
		brelse(bh[i]);
#if (LINUX_22)
          unlock_kernel();
#endif
	  return 0;
       }
    }

#if (DUMP_BUFFER_HEADS)
    if (1)
    {
       for (i=0; i < blocks; i++)
       {
  	  NWFSPrint("getblk buffer head (read) %d-0x%08X\n", (int)i, 
		        (unsigned)bh[i]);
          dumpRecordBytes(bh[i], sizeof(struct buffer_head));
          dumpRecord(bh[i], sizeof(struct buffer_head));
       }
#if (LINUX_22)
       unlock_kernel();
#endif
       return 0;
    }
#endif
    
    ll_rw_block(READ, blocks, bh);
    for(i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (buffer_uptodate(bh[i]))
       {
	  NWFSCopy(&Sector[i * blocksize], bh[i]->b_data, blocksize);
	  bytesRead += blocksize;
       }
       else
       {
          NWFSPrint("read error %d of %d  bytes read %d blksize-%d\n", 
		   (int)i, (int)blocks, (int)bytesRead,
		   (int)blocksize);
          read_error = 1;
       }
       brelse(bh[i]);
    }
#if (LINUX_22)
    unlock_kernel();
#endif

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (read_error ? 0 : bytesRead);

}

ULONG pWriteDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		       ULONG sectors, ULONG readAhead)
{
    register ULONG i, bytesWritten = 0;
    register ULONG bps, lba;
    struct buffer_head *bh[sectors];
    register ULONG write_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    register NWDISK *NWDisk;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

#if (VERBOSE)
    NWFSPrint("write disk-%d lba-%d for %d sectors\n",
	     (int)disk, (int)StartingLBA, (int)sectors);
#endif

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (write-bc)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

#if (LINUX_22)
    lock_kernel();
#endif
    for (i=0; i < blocks; i++)
    {
       if (!(bh[i] = getblk((ULONG)NWDisk->PhysicalDiskHandle,
			    lba + i, blocksize)))
       {
	  for (i=0; i < blocks; i++)
	     if (bh[i])
		brelse(bh[i]);
#if (LINUX_22)
          unlock_kernel();
#endif
	  return 0;
       }
       NWFSCopy(bh[i]->b_data, &Sector[i * blocksize], blocksize);
       mark_buffer_uptodate(bh[i], 1);

#if (LINUX_24)
       mark_buffer_dirty(bh[i]);
#else
       mark_buffer_dirty(bh[i], 0);
#endif
       bytesWritten += blocksize;
    }

#if (DUMP_BUFFER_HEADS)
    if (1)
    {
       for (i=0; i < blocks; i++)
       {
     	  NWFSPrint("getblk buffer head (write) %d-0x%08X\n", (int)i, 
		    (unsigned)bh[i]);
          dumpRecordBytes(bh[i], sizeof(struct buffer_head));
          dumpRecord(bh[i], sizeof(struct buffer_head));
       }
#if (LINUX_22)
       unlock_kernel();
#endif
       return 0;
    }
#endif
    
    ll_rw_block(WRITE, blocks, bh);
    for (i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (!buffer_uptodate(bh[i]))
	  write_error = 1;
       brelse(bh[i]);
    }
#if (LINUX_22)
    unlock_kernel();
#endif

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (write_error ? 0 : bytesWritten);

}

ULONG pZeroFillDiskSectors(ULONG disk, ULONG StartingLBA, ULONG sectors,
			  ULONG readAhead)
{
    register ULONG i, bytesWritten = 0;
    register ULONG bps, lba;
    struct buffer_head *bh[sectors];
    register ULONG write_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    register NWDISK *NWDisk;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

#if (VERBOSE)
    NWFSPrint("zero disk-%d lba-%d for %d sectors\n", 
	     (int)disk, (int)StartingLBA, (int)sectors);
#endif
    
    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;

    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {

       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (fill-bc)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }
    
#if (LINUX_22)
    lock_kernel();
#endif
    for (i=0; i < blocks; i++)
    {
       if (!(bh[i] = getblk((ULONG)NWDisk->PhysicalDiskHandle,
			  lba + i, blocksize)))
       {
	  for (i=0; i < blocks; i++)
	     if (bh[i])
		brelse(bh[i]);
#if (LINUX_22)
          unlock_kernel();
#endif
	  return 0;
       }
       NWFSSet(bh[i]->b_data, 0, blocksize);
       mark_buffer_uptodate(bh[i], 1);
#if (LINUX_24)
       mark_buffer_dirty(bh[i]);
#else
       mark_buffer_dirty(bh[i], 0);
#endif
       bytesWritten += blocksize;
    }

#if (DUMP_BUFFER_HEADS)
    if (1)
    {
       for (i=0; i < blocks; i++)
       {
     	  NWFSPrint("getblk buffer head (fill) %d-0x%08X\n", (int)i, 
		    (unsigned)bh[i]);
          dumpRecordBytes(bh[i], sizeof(struct buffer_head));
          dumpRecord(bh[i], sizeof(struct buffer_head));
       }
#if (LINUX_22)
       unlock_kernel();
#endif
       return 0;
    }
#endif
    
    ll_rw_block(WRITE, blocks, bh);
    for (i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (!buffer_uptodate(bh[i]))
	  write_error = 1;
       brelse(bh[i]);
    }
#if (LINUX_22)
    unlock_kernel();
#endif

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (write_error ? 0 : bytesWritten);
}

#else

//
// this case assumes we will use the NWFS buffer cache and not allow
// cache sharing with the linux buffer cache.
//

//
//   Linux 2.0
//

#if (LINUX_20)

ULONG pReadDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		      ULONG sectors, ULONG readAhead)
{
    register ULONG i, j, bytesRead = 0;
    register ULONG bps, lba;
    register NWDISK *NWDisk;
    register ULONG read_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    struct buffer_head *bh[sectors];

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       bh[i] = nwfs_get_bh();
       if (!bh[i])
       {
	  for (j=0; j < i; j++)
	     if (bh[j])
		nwfs_put_bh(bh[j]);
	  return 0;
       }
    }

    for (i=0; i < blocks; i++)
    {
       bh[i]->b_this_page = bh[(i + 1) % blocks]; // create circular list
       bh[i]->b_state = 0;
       bh[i]->b_next_free = (struct buffer_head *)NULL;
       bh[i]->b_size = blocksize;
       bh[i]->b_data = (char *)&Sector[i * blocksize];
       bh[i]->b_list = BUF_CLEAN;
       bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       bh[i]->b_blocknr = lba + i;
       bh[i]->b_count = 1;
       bh[i]->b_flushtime = 0;
       clear_bit(BH_Uptodate, &bh[i]->b_state);
    }

    ll_rw_block(READ, blocks, bh);
    for(i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (buffer_uptodate(bh[i]))
	  bytesRead += blocksize;
       else
	  read_error = 1;
       nwfs_put_bh(bh[i]);
    }

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (read_error ? 0 : bytesRead);

}

ULONG pWriteDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		       ULONG sectors, ULONG readAhead)
{
    register ULONG i, j, bytesWritten = 0;
    register ULONG bps, lba;
    register ULONG write_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    register NWDISK *NWDisk;
    struct buffer_head *bh[sectors];

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       bh[i] = nwfs_get_bh();
       if (!bh[i])
       {
	  for (j=0; j < i; j++)
	     if (bh[j])
		nwfs_put_bh(bh[j]);
	  return 0;
       }
    }

    for (i=0; i < blocks; i++)
    {
       bh[i]->b_this_page = bh[(i + 1) % blocks]; // create circular list
       bh[i]->b_state = 0;
       bh[i]->b_next_free = (struct buffer_head *) NULL;
       bh[i]->b_size = blocksize;
       bh[i]->b_data = (char *)&Sector[i * blocksize];
       bh[i]->b_list = BUF_CLEAN;
       bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       bh[i]->b_blocknr = lba + i;
       bh[i]->b_count = 1;
       bh[i]->b_flushtime = 0;
       set_bit(BH_Uptodate, &bh[i]->b_state);
       set_bit(BH_Dirty, &bh[i]->b_state);
    }

    ll_rw_block(WRITE, blocks, bh);
    for (i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (buffer_uptodate(bh[i]))
	  bytesWritten += blocksize;
       else
	  write_error = 1;
       nwfs_put_bh(bh[i]);
    }

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (write_error ? 0 : bytesWritten);

}

ULONG pZeroFillDiskSectors(ULONG disk, ULONG StartingLBA, ULONG sectors,
			  ULONG readAhead)
{
    register ULONG i, j, bytesWritten = 0;
    register ULONG bps, lba;
    register ULONG write_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    register NWDISK *NWDisk;
    struct buffer_head *bh[sectors];

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       bh[i] = nwfs_get_bh();
       if (!bh[i])
       {
	  for (j=0; j < i; j++)
	     if (bh[j])
		nwfs_put_bh(bh[j]);
	  return 0;
       }
    }

    for (i=0; i < blocks; i++)
    {
       bh[i]->b_this_page = bh[(i + 1) % blocks]; // create circular list
       bh[i]->b_state = 0;
       bh[i]->b_next_free = (struct buffer_head *) NULL;
       bh[i]->b_size = blocksize;
       bh[i]->b_data = (char *) ZeroBuffer;
       bh[i]->b_list = BUF_CLEAN;
       bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       bh[i]->b_blocknr = lba + i;
       bh[i]->b_count = 1;
       bh[i]->b_flushtime = 0;
       set_bit(BH_Uptodate, &bh[i]->b_state);
       set_bit(BH_Dirty, &bh[i]->b_state);
    }

    ll_rw_block(WRITE, blocks, bh);
    for (i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (buffer_uptodate(bh[i]))
	  bytesWritten += blocksize;
       else
	  write_error = 1;
       nwfs_put_bh(bh[i]);
    }

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (write_error ? 0 : bytesWritten);
}

#endif

//
//  Linux 2.2
//

#if (LINUX_22)

void end_io(struct buffer_head *bh, int uptodate)
{
    mark_buffer_uptodate(bh, uptodate);
    unlock_buffer(bh);
    return;
}

ULONG pReadDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		      ULONG sectors, ULONG readAhead)
{
    register ULONG i, j, bytesRead = 0;
    register ULONG bps, lba;
    register NWDISK *NWDisk;
    register ULONG read_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    struct buffer_head *bh[sectors];

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       bh[i] = nwfs_get_bh();
       if (!bh[i])
       {
	  for (j=0; j < i; j++)
	     if (bh[j])
		nwfs_put_bh(bh[j]);
	  return 0;
       }
    }

    for (i=0; i < blocks; i++)
    {
       bh[i]->b_this_page = bh[(i + 1) % blocks]; // create circular list
       bh[i]->b_state = 0;
       bh[i]->b_next_free = (struct buffer_head *)NULL;
       bh[i]->b_size = blocksize;
       bh[i]->b_data = (char *)&Sector[i * blocksize];
       bh[i]->b_list = BUF_CLEAN;
       bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       bh[i]->b_blocknr = lba + i;
       bh[i]->b_count = 1;
       bh[i]->b_flushtime = 0;
       bh[i]->b_end_io = end_io;
       clear_bit(BH_Uptodate, &bh[i]->b_state);
    }

    lock_kernel();
    ll_rw_block(READ, blocks, bh);

    for(i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (buffer_uptodate(bh[i]))
	  bytesRead += blocksize;
       else
	  read_error = 1;
       nwfs_put_bh(bh[i]);
    }
    unlock_kernel();

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (read_error ? 0 : bytesRead);

}

ULONG pWriteDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		       ULONG sectors, ULONG readAhead)
{
    register ULONG i, j, bytesWritten = 0;
    register ULONG bps, lba;
    register ULONG write_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    register NWDISK *NWDisk;
    struct buffer_head *bh[sectors];

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       bh[i] = nwfs_get_bh();
       if (!bh[i])
       {
	  for (j=0; j < i; j++)
	     if (bh[j])
		nwfs_put_bh(bh[j]);
	  return 0;
       }
    }

    for (i=0; i < blocks; i++)
    {
       bh[i]->b_this_page = bh[(i + 1) % blocks]; // create circular list
       bh[i]->b_state = 0;
       bh[i]->b_next_free = (struct buffer_head *) NULL;
       bh[i]->b_size = blocksize;
       bh[i]->b_data = (char *)&Sector[i * blocksize];
       bh[i]->b_list = BUF_CLEAN;
       bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       bh[i]->b_blocknr = lba + i;
       bh[i]->b_count = 1;
       bh[i]->b_flushtime = 0;
       bh[i]->b_end_io = end_io;
       set_bit(BH_Uptodate, &bh[i]->b_state);
       set_bit(BH_Dirty, &bh[i]->b_state);
    }

    lock_kernel();
    ll_rw_block(WRITE, blocks, bh);
    for (i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (buffer_uptodate(bh[i]))
	  bytesWritten += blocksize;
       else
	  write_error = 1;
       nwfs_put_bh(bh[i]);
    }
    unlock_kernel();

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (write_error ? 0 : bytesWritten);

}

ULONG pZeroFillDiskSectors(ULONG disk, ULONG StartingLBA, ULONG sectors,
			  ULONG readAhead)
{
    register ULONG i, j, bytesWritten = 0;
    register ULONG bps, lba;
    register ULONG write_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    register NWDISK *NWDisk;
    struct buffer_head *bh[sectors];

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       bh[i] = nwfs_get_bh();
       if (!bh[i])
       {
	  for (j=0; j < i; j++)
	     if (bh[j])
		nwfs_put_bh(bh[j]);
	  return 0;
       }
    }

    for (i=0; i < blocks; i++)
    {
       bh[i]->b_this_page = bh[(i + 1) % blocks]; // create circular list
       bh[i]->b_state = 0;
       bh[i]->b_next_free = (struct buffer_head *) NULL;
       bh[i]->b_size = blocksize;
       bh[i]->b_data = (char *) ZeroBuffer;
       bh[i]->b_list = BUF_CLEAN;
       bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       bh[i]->b_blocknr = lba + i;
       bh[i]->b_count = 1;
       bh[i]->b_flushtime = 0;
       bh[i]->b_end_io = end_io;
       set_bit(BH_Uptodate, &bh[i]->b_state);
       set_bit(BH_Dirty, &bh[i]->b_state);
    }

    lock_kernel();
    ll_rw_block(WRITE, blocks, bh);
    for (i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (buffer_uptodate(bh[i]))
	  bytesWritten += blocksize;
       else
	  write_error = 1;
       nwfs_put_bh(bh[i]);
    }
    unlock_kernel();

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (write_error ? 0 : bytesWritten);
}

//
//
//

void end_asynch_io(struct buffer_head *bh, int uptodate)
{
    register int i;
    ASYNCH_IO *io = (ASYNCH_IO *)bh->b_dev_id;
    bh->b_dev_id = 0;
    
    mark_buffer_uptodate(bh, uptodate);
    unlock_buffer(bh);

    if (!io)
    {
       NWFSPrint("nwfs:  aio callback has NULL handle\n");
       return;
    }

    if (!test_bit(BH_Uptodate, &bh->b_state))
       io->ccode = ASIO_IO_ERROR;

    atomic_inc((volatile atomic_t *)&io->complete);
    if (io->complete == io->count)
    {
       for (i=0; i < io->count; i++)
       {
          nwfs_put_bh(io->bh[i]);
          io->bh[i] = 0;
       }

       insert_callback(io);

#if (PROFILE_AIO)
       profile_complete();
#endif
    }
    return;
}

ULONG aReadDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		      ULONG sectors, ULONG readAhead, ASYNCH_IO *io)
{
    register ULONG i, j, bytesRead = 0;
    register ULONG bps, lba;
    register NWDISK *NWDisk;
    register ULONG rsize, blocks, blocksize, spb;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       io->bh[i] = nwfs_get_bh();
       if (!io->bh[i])
       {
	  for (j=0; j < i; j++)
	     if (io->bh[j])
		nwfs_put_bh(io->bh[j]);
	  return 0;
       }
    }

    io->ccode = 0;
    io->count = blocks;
    io->complete = 0;
    for (i=0; i < blocks; i++)
    {
       io->bh[i]->b_this_page = io->bh[(i + 1) % blocks]; // create circular list
       io->bh[i]->b_state = 0;
       io->bh[i]->b_next_free = (struct buffer_head *)NULL;
       io->bh[i]->b_size = blocksize;
       io->bh[i]->b_data = (char *)&Sector[i * blocksize];
       io->bh[i]->b_list = BUF_CLEAN;
       io->bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       io->bh[i]->b_blocknr = lba + i;
       io->bh[i]->b_count = 1;
       io->bh[i]->b_flushtime = 0;
       io->bh[i]->b_end_io = end_asynch_io;
       io->bh[i]->b_dev_id = io;
       clear_bit(BH_Uptodate, &io->bh[i]->b_state);
    }

    lock_kernel();
    ll_rw_block(READ, blocks, &io->bh[0]);

#if (POST_IMMEDIATE)
    run_task_queue(&tq_disk);
#endif
    
    unlock_kernel();

#if (DEBUG_AIO)
    NWFSPrint("io asynch read disk-%d lba-%d\n", (int)disk, (int)StartingLBA);
#endif

    bytesRead = (sectors * bps);
    return (bytesRead);

}

ULONG aWriteDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		       ULONG sectors, ULONG readAhead, ASYNCH_IO *io)
{
    register ULONG i, j, bytesWritten = 0;
    register ULONG bps, lba;
    register NWDISK *NWDisk;
    register ULONG rsize, blocks, blocksize, spb;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       io->bh[i] = nwfs_get_bh();
       if (!io->bh[i])
       {
	  for (j=0; j < i; j++)
	     if (io->bh[j])
		nwfs_put_bh(io->bh[j]);
	  return 0;
       }
    }

    io->ccode = 0;
    io->count = blocks;
    io->complete = 0;
    for (i=0; i < blocks; i++)
    {
       io->bh[i]->b_this_page = io->bh[(i + 1) % blocks]; // create circular list
       io->bh[i]->b_state = 0;
       io->bh[i]->b_next_free = (struct buffer_head *) NULL;
       io->bh[i]->b_size = blocksize;
       io->bh[i]->b_data = (char *)&Sector[i * blocksize];
       io->bh[i]->b_list = BUF_CLEAN;
       io->bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       io->bh[i]->b_blocknr = lba + i;
       io->bh[i]->b_count = 1;
       io->bh[i]->b_flushtime = 0;
       io->bh[i]->b_end_io = end_asynch_io;
       io->bh[i]->b_dev_id = io;
       set_bit(BH_Uptodate, &io->bh[i]->b_state);
       set_bit(BH_Dirty, &io->bh[i]->b_state);
    }

    lock_kernel();
    ll_rw_block(WRITE, blocks, &io->bh[0]);
    
#if (POST_IMMEDIATE)
    run_task_queue(&tq_disk);
#endif

    unlock_kernel();

#if (DEBUG_AIO)
    NWFSPrint("io asynch write disk-%d lba-%d\n", (int)disk, (int)StartingLBA);
#endif

    bytesWritten = (sectors * bps);
    return (bytesWritten);

}

ULONG aZeroFillDiskSectors(ULONG disk, ULONG StartingLBA, ULONG sectors,
			  ULONG readAhead, ASYNCH_IO *io)
{
    register ULONG i, j, bytesWritten = 0;
    register ULONG bps, lba;
    register NWDISK *NWDisk;
    register ULONG rsize, blocks, blocksize, spb;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       io->bh[i] = nwfs_get_bh();
       if (!io->bh[i])
       {
	  for (j=0; j < i; j++)
	     if (io->bh[j])
		nwfs_put_bh(io->bh[j]);
	  return 0;
       }
    }

    io->ccode = 0;
    io->count = blocks;
    io->complete = 0;
    for (i=0; i < blocks; i++)
    {
       io->bh[i]->b_this_page = io->bh[(i + 1) % blocks]; // create circular list
       io->bh[i]->b_state = 0;
       io->bh[i]->b_next_free = (struct buffer_head *) NULL;
       io->bh[i]->b_size = blocksize;
       io->bh[i]->b_data = (char *) ZeroBuffer;
       io->bh[i]->b_list = BUF_CLEAN;
       io->bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       io->bh[i]->b_blocknr = lba + i;
       io->bh[i]->b_count = 1;
       io->bh[i]->b_flushtime = 0;
       io->bh[i]->b_end_io = end_asynch_io;
       io->bh[i]->b_dev_id = io;
       set_bit(BH_Uptodate, &io->bh[i]->b_state);
       set_bit(BH_Dirty, &io->bh[i]->b_state);
    }

    lock_kernel();
    ll_rw_block(WRITE, blocks, &io->bh[0]);

#if (POST_IMMEDIATE)
    run_task_queue(&tq_disk);
#endif

    unlock_kernel();

#if (DEBUG_AIO)
    NWFSPrint("io asynch fill disk-%d lba-%d\n", (int)disk, (int)StartingLBA);
#endif

    bytesWritten = (sectors * bps);
    return (bytesWritten);
}

#endif

//
//  Linux 2.4
//

#if (LINUX_24)

void end_io(struct buffer_head *bh, int uptodate)
{
    mark_buffer_uptodate(bh, uptodate);
    unlock_buffer(bh);
    return;
}

ULONG pReadDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		      ULONG sectors, ULONG readAhead)
{
    register ULONG i, j, bytesRead = 0;
    register ULONG bps, lba;
    register NWDISK *NWDisk;
    register ULONG read_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    struct buffer_head *bh[sectors];
    register struct page *page = virt_to_page(Sector);

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       bh[i] = nwfs_get_bh();
       if (!bh[i])
       {
	  for (j=0; j < i; j++)
	     if (bh[j])
		nwfs_put_bh(bh[j]);
	  return 0;
       }
    }

    for (i=0; i < blocks; i++)
    {
       bh[i]->b_this_page = bh[(i + 1) % blocks]; // create circular list
       bh[i]->b_state = 0;
       bh[i]->b_next_free = (struct buffer_head *)NULL;
       bh[i]->b_size = blocksize;
       bh[i]->b_data = (char *)&Sector[i * blocksize];
       bh[i]->b_list = BUF_CLEAN;
       bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       bh[i]->b_blocknr = lba + i;
       bh[i]->b_count.counter = 1;
       bh[i]->b_flushtime = 0;
       bh[i]->b_end_io = end_io;
       bh[i]->b_private = NULL;
       bh[i]->b_page = page;
       set_bit(BH_Mapped, &bh[i]->b_state);
       set_bit(BH_Lock, &bh[i]->b_state);
       clear_bit(BH_Uptodate, &bh[i]->b_state);
    }

#if (DUMP_BUFFER_HEADS)
    if (1)
    {
       for (i=0; i < blocks; i++)
       {
          NWFSPrint("nwfs buffer head (read) %d-0x%08X\n", (int)i, 
		 (unsigned)bh[i]);
          dumpRecordBytes(bh[i], sizeof(struct buffer_head));
          dumpRecord(bh[i], sizeof(struct buffer_head));
       }
       return 0;
    }
#endif
    
    for (i=0; i < blocks; i++)
       submit_bh(READ, bh[i]);

    for (i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (buffer_uptodate(bh[i]))
	  bytesRead += blocksize;
       else
	  read_error = 1;
       nwfs_put_bh(bh[i]);
    }

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (read_error ? 0 : bytesRead);

}

ULONG pWriteDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		       ULONG sectors, ULONG readAhead)
{
    register ULONG i, j, bytesWritten = 0;
    register ULONG bps, lba;
    register ULONG write_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    register NWDISK *NWDisk;
    struct buffer_head *bh[sectors];
    register struct page *page = virt_to_page(Sector);

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       bh[i] = nwfs_get_bh();
       if (!bh[i])
       {
	  for (j=0; j < i; j++)
	     if (bh[j])
		nwfs_put_bh(bh[j]);
	  return 0;
       }
    }

    for (i=0; i < blocks; i++)
    {
       bh[i]->b_this_page = bh[(i + 1) % blocks]; // create circular list
       bh[i]->b_state = 0;
       bh[i]->b_next_free = (struct buffer_head *) NULL;
       bh[i]->b_size = blocksize;
       bh[i]->b_data = (char *)&Sector[i * blocksize];
       bh[i]->b_list = BUF_CLEAN;
       bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       bh[i]->b_blocknr = lba + i;
       bh[i]->b_count.counter = 1;
       bh[i]->b_flushtime = 0;
       bh[i]->b_end_io = end_io;
       bh[i]->b_private = NULL;
       bh[i]->b_page = page;
       set_bit(BH_Req, &bh[i]->b_state);
       set_bit(BH_Mapped, &bh[i]->b_state);
       set_bit(BH_Uptodate, &bh[i]->b_state);
       set_bit(BH_Dirty, &bh[i]->b_state);
       set_bit(BH_Lock, &bh[i]->b_state);
    }

#if (DUMP_BUFFER_HEADS)
    if (1)
    {
       for (i=0; i < blocks; i++)
       {
          NWFSPrint("nwfs buffer head (write) %d-0x%08X\n", (int)i, 
		 (unsigned)bh[i]);
          dumpRecordBytes(bh[i], sizeof(struct buffer_head));
          dumpRecord(bh[i], sizeof(struct buffer_head));
       }
       return 0;
    }
#endif
    
    for (i=0; i < blocks; i++)
       submit_bh(WRITE, bh[i]);

    for (i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (buffer_uptodate(bh[i]))
	  bytesWritten += blocksize;
       else
	  write_error = 1;
       nwfs_put_bh(bh[i]);
    }

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (write_error ? 0 : bytesWritten);

}

ULONG pZeroFillDiskSectors(ULONG disk, ULONG StartingLBA, ULONG sectors,
			  ULONG readAhead)
{
    register ULONG i, j, bytesWritten = 0;
    register ULONG bps, lba;
    register ULONG write_error = 0;
    register ULONG rsize, blocks, blocksize, spb;
    register NWDISK *NWDisk;
    struct buffer_head *bh[sectors];
    register struct page *page = virt_to_page(ZeroBuffer);

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       bh[i] = nwfs_get_bh();
       if (!bh[i])
       {
	  for (j=0; j < i; j++)
	     if (bh[j])
		nwfs_put_bh(bh[j]);
	  return 0;
       }
    }

    for (i=0; i < blocks; i++)
    {
       bh[i]->b_this_page = bh[(i + 1) % blocks]; // create circular list
       bh[i]->b_state = 0;
       bh[i]->b_next_free = (struct buffer_head *) NULL;
       bh[i]->b_size = blocksize;
       bh[i]->b_data = (char *) ZeroBuffer;
       bh[i]->b_list = BUF_CLEAN;
       bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       bh[i]->b_blocknr = lba + i;
       bh[i]->b_count.counter = 1;
       bh[i]->b_flushtime = 0;
       bh[i]->b_end_io = end_io;
       bh[i]->b_private = NULL;
       bh[i]->b_page = page;
       set_bit(BH_Req, &bh[i]->b_state);
       set_bit(BH_Mapped, &bh[i]->b_state);
       set_bit(BH_Uptodate, &bh[i]->b_state);
       set_bit(BH_Dirty, &bh[i]->b_state);
       set_bit(BH_Lock, &bh[i]->b_state);
    }

#if (DUMP_BUFFER_HEADS)
    if (1)
    {
       for (i=0; i < blocks; i++)
       {
          NWFSPrint("nwfs buffer head (fill) %d-0x%08X\n", (int)i, 
		 (unsigned)bh[i]);
          dumpRecordBytes(bh[i], sizeof(struct buffer_head));
          dumpRecord(bh[i], sizeof(struct buffer_head));
       }
       return 0;
    }
#endif
    
    for (i=0; i < blocks; i++)
       submit_bh(WRITE, bh[i]);

    for (i=0; i < blocks; i++)
    {
       wait_on_buffer(bh[i]);
       if (buffer_uptodate(bh[i]))
	  bytesWritten += blocksize;
       else
	  write_error = 1;
       nwfs_put_bh(bh[i]);
    }

#if (PROFILE_AIO)
    profile_complete();
#endif

    return (write_error ? 0 : bytesWritten);
}

//
//
//

void end_asynch_io(struct buffer_head *bh, int uptodate)
{
    register int i;
    ASYNCH_IO *io = (ASYNCH_IO *)bh->b_private;
    bh->b_private = 0;

    mark_buffer_uptodate(bh, uptodate);
    unlock_buffer(bh);

    if (!io)
    {
       NWFSPrint("nwfs:  aio callback has NULL handle\n");
       return;
    }

    if (!test_bit(BH_Uptodate, &bh->b_state))
       io->ccode = ASIO_IO_ERROR;

    atomic_inc((atomic_t *)&io->complete);
    if (io->complete == io->count)
    {
       for (i=0; i < io->count; i++)
       {
          nwfs_put_bh(io->bh[i]);
          io->bh[i] = 0;
       }

       insert_callback(io);

#if (PROFILE_AIO)
       profile_complete();
#endif
    }
    return;
}

ULONG aReadDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		      ULONG sectors, ULONG readAhead, ASYNCH_IO *io)
{
    register ULONG i, j, bytesRead = 0;
    register ULONG bps, lba;
    register NWDISK *NWDisk;
    register struct page *page = virt_to_page(Sector);
    register ULONG rsize, blocks, blocksize, spb;

    if (!page)
    {
       NWFSPrint("nwfs:  page context was NULL in aReadDiskSectors\n");
       return 0;
    }

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       io->bh[i] = nwfs_get_bh();
       if (!io->bh[i])
       {
	  for (j=0; j < i; j++)
	     if (io->bh[j])
		nwfs_put_bh(io->bh[j]);
	  return 0;
       }
    }

    io->ccode = 0;
    io->count = blocks;
    io->complete = 0;
    for (i=0; i < blocks; i++)
    {
       io->bh[i]->b_this_page = io->bh[(i + 1) % blocks]; 
       io->bh[i]->b_state = 0;
       io->bh[i]->b_next_free = (struct buffer_head *)NULL;
       io->bh[i]->b_size = blocksize;
       io->bh[i]->b_data = (char *)&Sector[i * blocksize];
       io->bh[i]->b_list = BUF_CLEAN;
       io->bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       io->bh[i]->b_blocknr = lba + i;
       io->bh[i]->b_count.counter = 1;
       io->bh[i]->b_flushtime = 0;
       io->bh[i]->b_end_io = end_asynch_io;
       io->bh[i]->b_private = io;
       io->bh[i]->b_page = page;
       set_bit(BH_Mapped, &io->bh[i]->b_state);
       set_bit(BH_Lock, &io->bh[i]->b_state);
       clear_bit(BH_Uptodate, &io->bh[i]->b_state);
    }

    for (i=0; i < blocks; i++)
       submit_bh(READ, io->bh[i]);

#if (POST_IMMEDIATE)
    run_task_queue(&tq_disk);
#endif

    bytesRead = (sectors * bps);
    return (bytesRead);

}

ULONG aWriteDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		       ULONG sectors, ULONG readAhead, ASYNCH_IO *io)
{
    register ULONG i, j, bytesWritten = 0;
    register ULONG bps, lba;
    register NWDISK *NWDisk;
    register struct page *page = virt_to_page(Sector);
    register ULONG rsize, blocks, blocksize, spb;

    if (!page)
    {
       NWFSPrint("nwfs:  page context was NULL in aWriteDiskSectors\n");
       return 0;
    }

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       io->bh[i] = nwfs_get_bh();
       if (!io->bh[i])
       {
	  for (j=0; j < i; j++)
	     if (io->bh[j])
		nwfs_put_bh(io->bh[j]);
	  return 0;
       }
    }

    io->ccode = 0;
    io->count = blocks;
    io->complete = 0;
    for (i=0; i < blocks; i++)
    {
       io->bh[i]->b_this_page = io->bh[(i + 1) % blocks]; 
       io->bh[i]->b_state = 0;
       io->bh[i]->b_next_free = (struct buffer_head *) NULL;
       io->bh[i]->b_size = blocksize;
       io->bh[i]->b_data = (char *)&Sector[i * blocksize];
       io->bh[i]->b_list = BUF_CLEAN;
       io->bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       io->bh[i]->b_blocknr = lba + i;
       io->bh[i]->b_count.counter = 1;
       io->bh[i]->b_flushtime = 0;
       io->bh[i]->b_end_io = end_asynch_io;
       io->bh[i]->b_private = io;
       io->bh[i]->b_page = page;
       set_bit(BH_New, &io->bh[i]->b_state);
       set_bit(BH_Mapped, &io->bh[i]->b_state);
       set_bit(BH_Uptodate, &io->bh[i]->b_state);
       set_bit(BH_Dirty, &io->bh[i]->b_state);
       set_bit(BH_Lock, &io->bh[i]->b_state);
    }

    for (i=0; i < blocks; i++)
       submit_bh(WRITE, io->bh[i]);

#if (POST_IMMEDIATE)
    run_task_queue(&tq_disk);
#endif
    
    bytesWritten = (sectors * bps);
    return (bytesWritten);

}

ULONG aZeroFillDiskSectors(ULONG disk, ULONG StartingLBA, ULONG sectors,
			  ULONG readAhead, ASYNCH_IO *io)
{
    register ULONG i, j, bytesWritten = 0;
    register ULONG bps, lba;
    register NWDISK *NWDisk;
    register struct page *page = virt_to_page(ZeroBuffer);
    register ULONG rsize, blocks, blocksize, spb;

    if (!page)
    {
       NWFSPrint("nwfs:  page context was NULL in aZeroFillDiskSectors\n");
       return 0;
    }

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    blocksize = NWDisk->DeviceBlockSize;

    rsize = sectors * bps;
    blocks = rsize / blocksize;
    if (!blocks)
       return 0;
    spb = blocksize / bps;
    
    lba = StartingLBA / spb;
    if (StartingLBA % spb)
    {
       NWFSPrint("request not %d block aligned (%d) sectors-%d lba-%d (read)\n",
	       (int)blocksize, (int)(StartingLBA % spb), (int)sectors,
	       (int)StartingLBA);
       return 0;
    }

    for (i=0; i < blocks; i++)
    {
       io->bh[i] = nwfs_get_bh();
       if (!io->bh[i])
       {
	  for (j=0; j < i; j++)
	     if (io->bh[j])
		nwfs_put_bh(io->bh[j]);
	  return 0;
       }
    }

    io->ccode = 0;
    io->count = blocks;
    io->complete = 0;
    for (i=0; i < blocks; i++)
    {
       io->bh[i]->b_this_page = io->bh[(i + 1) % blocks]; 
       io->bh[i]->b_state = 0;
       io->bh[i]->b_next_free = (struct buffer_head *) NULL;
       io->bh[i]->b_size = blocksize;
       io->bh[i]->b_data = (char *) ZeroBuffer;
       io->bh[i]->b_list = BUF_CLEAN;
       io->bh[i]->b_dev = (int)NWDisk->PhysicalDiskHandle;
       io->bh[i]->b_blocknr = lba + i;
       io->bh[i]->b_count.counter = 1;
       io->bh[i]->b_flushtime = 0;
       io->bh[i]->b_end_io = end_asynch_io;
       io->bh[i]->b_private = io;
       io->bh[i]->b_page = page;
       set_bit(BH_New, &io->bh[i]->b_state);
       set_bit(BH_Mapped, &io->bh[i]->b_state);
       set_bit(BH_Uptodate, &io->bh[i]->b_state);
       set_bit(BH_Dirty, &io->bh[i]->b_state);
       set_bit(BH_Lock, &io->bh[i]->b_state);
    }

    for (i=0; i < blocks; i++)
       submit_bh(WRITE, io->bh[i]);

#if (POST_IMMEDIATE)
    run_task_queue(&tq_disk);
#endif

    bytesWritten = (sectors * bps);
    return (bytesWritten);
}

#endif

#endif

void SyncDevice(ULONG disk)
{
    sync_dev((int)disk);
    return;
}

#if (DO_ASYNCH_IO)
ULONG release_aio(ASYNCH_IO *io)
{
#if (LINUX_SLEEP)
    SignalSemaphore((struct semaphore *)io->call_back_parameter);
#endif
    return 0;
}
#endif

ULONG ReadDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			     ULONG sectors, ULONG readAhead)
{
#if (DO_ASYNCH_IO)
    ASYNCH_IO io;
#if (LINUX_SLEEP)
    NWFSInitSemaphore(raw_semaphore);
#endif

    NWFSSet(&io, 0, sizeof(ASYNCH_IO));

    io.command = ASYNCH_READ;
    io.disk = disk;
    io.flags = ASIO_INTR_CALLBACK;
    io.sector_offset = LBA;
    io.sector_count = sectors;
    io.buffer = Sector;
    io.call_back_routine = release_aio;
#if (LINUX_SLEEP)
    io.call_back_parameter = (ULONG) &raw_semaphore;
#endif

    insert_io(disk, &io);

    RunAsynchIOQueue(disk);

#if (LINUX_SLEEP)
    WaitOnSemaphore(&raw_semaphore);
#endif

    return (ULONG)(io.return_code);
#else
    return (pReadDiskSectors(disk, LBA, Sector, sectors, readAhead));
#endif
}

ULONG WriteDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			      ULONG sectors, ULONG readAhead)
{
#if (DO_ASYNCH_IO)
    ASYNCH_IO io;
#if (LINUX_SLEEP)
    NWFSInitSemaphore(raw_semaphore);
#endif

    NWFSSet(&io, 0, sizeof(ASYNCH_IO));

    io.command = ASYNCH_WRITE;
    io.disk = disk;
    io.flags = ASIO_INTR_CALLBACK;
    io.sector_offset = LBA;
    io.sector_count = sectors;
    io.buffer = Sector;
    io.call_back_routine = release_aio;
#if (LINUX_SLEEP)
    io.call_back_parameter = (ULONG) &raw_semaphore;
#endif

    insert_io(disk, &io);

    RunAsynchIOQueue(disk);

#if (LINUX_SLEEP)
    WaitOnSemaphore(&raw_semaphore);
#endif

    return (ULONG)(io.return_code);
#else
    return (pWriteDiskSectors(disk, LBA, Sector, sectors, readAhead));
#endif
}

ULONG ZeroFillDiskSectors(ULONG disk, ULONG StartingLBA,
				 ULONG sectors, ULONG readAhead)
{
#if (DO_ASYNCH_IO)
    ASYNCH_IO io;
#if (LINUX_SLEEP)
    NWFSInitSemaphore(raw_semaphore);
#endif

    NWFSSet(&io, 0, sizeof(ASYNCH_IO));

    io.command = ASYNCH_FILL;
    io.disk = disk;
    io.flags = ASIO_INTR_CALLBACK;
    io.sector_offset = StartingLBA;
    io.sector_count = sectors;
    io.buffer = NULL;
    io.call_back_routine = release_aio;
#if (LINUX_SLEEP)
    io.call_back_parameter = (ULONG) &raw_semaphore;
#endif

    insert_io(disk, &io);

    RunAsynchIOQueue(disk);

#if (LINUX_SLEEP)
    WaitOnSemaphore(&raw_semaphore);
#endif

    return (ULONG)(io.return_code);
#else
    return (pZeroFillDiskSectors(disk, StartingLBA, sectors, readAhead));
#endif
}

#endif


#if (LINUX_UTIL)

#define FOR_UTIL_LINUX

#include <sys/types.h>

#include <errno.h>
#include <unistd.h>

#ifndef FOR_UTIL_LINUX

#include "et/com_err.h"
#include "ext2fs/io.h"

#else /* FOR_UTIL_LINUX */

#if defined(__GNUC__) || defined(HAS_LONG_LONG)
typedef long long       ext2_loff_t;
#else
typedef long            ext2_loff_t;
#endif

ext2_loff_t ext2_llseek (unsigned int, ext2_loff_t, unsigned int);

#endif /* FOR_UTIL_LINUX */

#ifdef __linux__

#ifdef HAVE_LLSEEK
#include <syscall.h>

#else	/* HAVE_LLSEEK */

#ifdef __alpha__

#define my_llseek lseek

#elif __i386__

#include <linux/unistd.h>

#ifndef __NR__llseek
#define __NR__llseek            140
#endif

static int _llseek (unsigned int, unsigned long,
		   unsigned long, ext2_loff_t *, unsigned int);

static _syscall5(int,_llseek,unsigned int,fd,unsigned long,offset_high,
		 unsigned long, offset_low, ext2_loff_t *,result,
		 unsigned int, origin)

static ext2_loff_t my_llseek (unsigned int fd, ext2_loff_t offset,
		unsigned int origin)
{
	ext2_loff_t result;
	int retval;

	retval = _llseek (fd, ((unsigned long long) offset) >> 32,
			((unsigned long long) offset) & 0xffffffff,
			&result, origin);
	return (retval == -1 ? (ext2_loff_t) retval : result);
}

#else

#error "llseek() is not available"

#endif /* __alpha__ */

#endif	/* HAVE_LLSEEK */

ext2_loff_t ext2_llseek (unsigned int fd, ext2_loff_t offset,
			 unsigned int origin)
{
	ext2_loff_t result;
	static int do_compat = 0;

	if ((sizeof(off_t) >= sizeof(ext2_loff_t)) ||
	    (offset < ((ext2_loff_t) 1 << ((sizeof(off_t)*8) -1))))
		return lseek(fd, (off_t) offset, origin);

	if (do_compat) {
		errno = EINVAL;
		return -1;
	}
	
	result = my_llseek (fd, offset, origin);
	if (result == -1 && errno == ENOSYS) {
		/*
		 * Just in case this code runs on top of an old kernel
		 * which does not support the llseek system call
		 */
		do_compat++;
		errno = EINVAL;
	}
	return result;
}

#else /* !linux */

ext2_loff_t ext2_llseek (unsigned int fd, ext2_loff_t offset,
			 unsigned int origin)
{
	if ((sizeof(off_t) < sizeof(ext2_loff_t)) &&
	    (offset >= ((ext2_loff_t) 1 << ((sizeof(off_t)*8) -1)))) {
		errno = EINVAL;
		return -1;
	}
	return lseek (fd, (off_t) offset, origin);
}

#endif 	/* linux */

//
//  low level disk sector interface routines
//

ULONG pReadDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		      ULONG sectors, ULONG readAhead)
{
    register ULONG i, bytesRead = 0;
    LONGLONG ccode;
    register ULONG bps, cbytes;
    register NWDISK *NWDisk;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;
    for (i=0; i < sectors; i++)
    {
       LONGLONG offset;

       offset = (LONGLONG) (StartingLBA + i) * (LONGLONG) bps;
       ccode = ext2_llseek((int)NWDisk->PhysicalDiskHandle,
			   offset, SEEK_SET);

       if (ccode == (LONGLONG) -1)
	  return bytesRead;

       cbytes = read((int)NWDisk->PhysicalDiskHandle,
		      &Sector[i * bps], bps);

       if (cbytes != bps)
       {
	  bytesRead += cbytes;
	  return bytesRead;
       }
       bytesRead += bps;
    }
    return bytesRead;
}

ULONG pWriteDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		       ULONG sectors, ULONG readAhead)
{
    register ULONG i, bytesWritten = 0;
    LONGLONG ccode;
    register ULONG bps, cbytes;
    register NWDISK *NWDisk;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;

    for (i=0; i < sectors; i++)
    {
       LONGLONG offset;

       offset = (LONGLONG) (StartingLBA + i) * (LONGLONG) bps;
       ccode = ext2_llseek((int)NWDisk->PhysicalDiskHandle,
			   offset, SEEK_SET);

       if (ccode == (LONGLONG) -1)
	  return bytesWritten;

       cbytes = write((int)NWDisk->PhysicalDiskHandle,
		       &Sector[i * bps], bps);

       if (cbytes != bps)
       {
	  bytesWritten += cbytes;
	  return bytesWritten;
       }
       bytesWritten += bps;
    }
    return bytesWritten;

}

ULONG pZeroFillDiskSectors(ULONG disk, ULONG StartingLBA, ULONG sectors,
			  ULONG readAhead)
{
    register ULONG i, bytesWritten = 0;
    LONGLONG ccode;
    register ULONG bps, cbytes;
    register NWDISK *NWDisk;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;

    for (i=0; i < sectors; i++)
    {
       LONGLONG offset;

       offset = (LONGLONG) (StartingLBA + i) * (LONGLONG) bps;
       ccode = ext2_llseek((int)NWDisk->PhysicalDiskHandle,
			   offset, SEEK_SET);

       if (ccode == (LONGLONG) -1)
	  return bytesWritten;

       cbytes = write((int)NWDisk->PhysicalDiskHandle,
		      &ZeroBuffer[i * bps], bps);

       if (cbytes != bps)
       {
	  bytesWritten += cbytes;
	  return bytesWritten;
       }

       bytesWritten += bps;
    }
    return bytesWritten;

}

void SyncDevice(ULONG disk)
{
    register ULONG ccode;
    
    if (SystemDisk[disk])
    {
       ccode = fdatasync((int)SystemDisk[disk]->PhysicalDiskHandle);
       if (ccode)
          NWFSPrint("fdatasync failed-%d fd-%d\n", (int)ccode,
                    (int)SystemDisk[disk]->PhysicalDiskHandle);
    }
    return;
}

ULONG ReadDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			     ULONG sectors, ULONG readAhead)
{
    return (pReadDiskSectors(disk, LBA, Sector, sectors, readAhead));
}

ULONG WriteDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			      ULONG sectors, ULONG readAhead)
{
    return (pWriteDiskSectors(disk, LBA, Sector, sectors, readAhead));
}

ULONG ZeroFillDiskSectors(ULONG disk, ULONG StartingLBA,
				 ULONG sectors, ULONG readAhead)
{
    return (pZeroFillDiskSectors(disk, StartingLBA, sectors, readAhead));
}

#endif


#if (WINDOWS_NT | WINDOWS_NT_RO)

extern ULONG ReadDisk(void *DiskPointer, ULONG SectorOffset, BYTE *Buffer,
		      ULONG SectorCount);

//
//  low level disk sector interface routines
//

ULONG pReadDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		      ULONG sectors, ULONG readAhead)
{
    register ULONG bytesRead = 0, retCode;
    register ULONG bps;
    struct buffer_head *bh;
    register NWDISK *NWDisk;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;

    retCode = ReadDisk(SystemDisk[disk]->PhysicalDiskHandle,
		       StartingLBA, Sector, sectors);

    if (NT_SUCCESS(retCode))
       bytesRead += (sectors * bps);

    return bytesRead;

}

ULONG pWriteDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		       ULONG sectors, ULONG readAhead)
{
    return 0;
}

ULONG pZeroFillDiskSectors(ULONG disk, ULONG StartingLBA, ULONG sectors,
			  ULONG readAhead)
{
    return 0;
}

void SyncDevice(ULONG disk)
{
    return;
}

ULONG ReadDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			     ULONG sectors, ULONG readAhead)
{
    return (pReadDiskSectors(disk, LBA, Sector, sectors, readAhead));
}

ULONG WriteDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			      ULONG sectors, ULONG readAhead)
{
    return (pWriteDiskSectors(disk, LBA, Sector, sectors, readAhead));
}

ULONG ZeroFillDiskSectors(ULONG disk, ULONG StartingLBA,
				 ULONG sectors, ULONG readAhead)
{
    return (pZeroFillDiskSectors(disk, StartingLBA, sectors, readAhead));
}

#endif


#if (DOS_UTIL)

#define READ_DEVICE   0x02
#define WRITE_DEVICE  0x03
#define NO_VERIFY     0x00
#define WRITE_VERIFY  0x01

// convert cylinder/head/sector addressing to LBA

ULONG CHStoLBA(ULONG disk, LONGLONG cyl, ULONG head, ULONG sector)
{
    register ULONG lba;
    ULONG heads = SystemDisk[disk]->TracksPerCylinder;
    ULONG sectors = SystemDisk[disk]->SectorsPerTrack;

    lba = cyl * (heads * sectors);
    lba += (head * sectors);
    lba += sector;
    lba -= 1;    // CHS is 1 relative so adjust LBA by subtracting 1

    return lba;

}

// convert LBA to cylinder/head/sector addressing

ULONG LBAtoCHS(ULONG disk, ULONG lba, LONGLONG *cyl, ULONG *head,
	       ULONG *sector)
{
    ULONG offset;
    ULONG heads = SystemDisk[disk]->TracksPerCylinder;
    ULONG sectors = SystemDisk[disk]->SectorsPerTrack;

    if (!cyl || !head || !sector)
       return -1;

    *cyl = (lba / (heads * sectors));
    offset = lba % (heads * sectors);
    *head = (WORD)(offset / sectors);
    *sector = (WORD)(offset % sectors) + 1;  // sector addressing is
					     // 1 relative so add 1
    return 0;
}

ULONG pReadDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		      ULONG sectors, ULONG readAhead)
{
    register ULONG bps, ccode, bytes = 0;
    register NWDISK *NWDisk;
    LONGLONG cylinder;
    ULONG head, sector;
    static _go32_dpmi_registers r;
#if (WINDOWS_98_UTIL)
    static union REGS rl;
#endif

    NWFSSet(&r, 0, sizeof(_go32_dpmi_registers));

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;

    if ((sectors * bps) > IO_BLOCK_SIZE)
       return bytes;

#if (WINDOWS_98_UTIL)
    rl.x.ax = 0x440D;    // generic IOCTL
    rl.h.bh = 3;         // locks levels 0, 1, 2 or 3
    rl.h.bl = (0x80 | (disk & 0x7F)); // dos limit is 128 drives
    rl.h.ch = 0x08;      // device category
    rl.h.cl = 0x4B;      // lock physical volume
    rl.x.dx = 0;         // permissions
    int86(0x21, &rl, &rl);

#endif

    if (NWDisk->Int13Extensions)
    {
       EXT_REQUEST request;

       NWFSSet(&request, 0, sizeof(EXT_REQUEST));

       request.Size = sizeof(EXT_REQUEST);
       request.Blocks = sectors;
       request.TransferBuffer = (ULONG)(NWDisk->DataSegment << 16) & 0xFFFF0000;
       request.LBA = StartingLBA;

       movedata(_my_ds(),
		(unsigned)&request,
		NWDisk->RequestSelector,
		0,
		sizeof(EXT_REQUEST));

       r.h.ah = 0x42;
       r.h.dl = (0x80 | (disk & 0x7F));
       r.x.ds = NWDisk->RequestSegment;
       r.x.si = 0;
       r.x.ss = r.x.sp = r.x.flags = 0;

       ccode = _go32_dpmi_simulate_int(0x13, &r);
       if (ccode)
	  goto ReadExit;

       // error if carry flag is set
       if (r.x.flags & 1)
       {
	  // if the drive reported an ECC occurred, return success
	  if (r.h.ah == 0x11)
	  {
		movedata(NWDisk->DataSelector,
			 0,
			 _my_ds(),
			 (unsigned)Sector,
			 (sectors * bps));

		bytes = (sectors * bps);
		goto ReadExit;
	  }
	  else
	     goto ReadExit;
       }

       movedata(NWDisk->DataSelector,
		0,
		_my_ds(),
		(unsigned)Sector,
		(sectors * bps));

       bytes = (sectors * bps);
       goto ReadExit;
    }
    else
    {
       ccode = LBAtoCHS(disk, StartingLBA, &cylinder, &head, &sector);
       if (ccode)
	  goto ReadExit;

       r.h.dh = (BYTE)head;
       r.h.dl = (0x80 | (disk & 0x7F));
       r.h.ch = (BYTE)(cylinder & 0xFF);
       r.h.cl = (BYTE)((cylinder & 0x0300) >> 2);
       r.h.cl |= (BYTE)(sector & 0x3F);
       r.h.ah = READ_DEVICE;
       r.h.al = (BYTE)sectors;
       r.x.es = NWDisk->DataSegment;
       r.x.bx = 0;
       r.x.ss = r.x.sp = r.x.flags = 0;

       ccode = _go32_dpmi_simulate_int(0x13, &r);
       if (ccode)
	  goto ReadExit;

       // error if carry flag is set
       if (r.x.flags & 1)
       {
	  // if the drive reported an ECC occurred, return success
	  if (r.h.ah == 0x11)
	  {
	     movedata(NWDisk->DataSelector,
			 0,
			 _my_ds(),
			 (unsigned)Sector,
			 (sectors * bps));

	     bytes = (sectors * bps);
	     goto ReadExit;
	  }
	  else
	     goto ReadExit;
       }

       movedata(NWDisk->DataSelector,
		0,
		_my_ds(),
		(unsigned)Sector,
		(sectors * bps));

       bytes = (sectors * bps);
    }

ReadExit:;
#if (WINDOWS_98_UTIL)
    rl.x.ax = 0x440D;    // generic IOCTL
    rl.h.bl = (0x80 | (disk & 0x7F)); // dos limit is 128 drives
    rl.h.ch = 0x08;      // device category
    rl.h.cl = 0x6B;      // unlock physical volume
    int86(0x21, &rl, &rl);
#endif

    return bytes;

}


ULONG pWriteDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		       ULONG sectors, ULONG readAhead)
{
    register ULONG bps, ccode, bytes = 0;
    register NWDISK *NWDisk;
    LONGLONG cylinder;
    ULONG head, sector;
    static _go32_dpmi_registers r;
#if (WINDOWS_98_UTIL)
    static union REGS rl;
#endif

    NWFSSet(&r, 0, sizeof(_go32_dpmi_registers));

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;

    if ((sectors * bps) > IO_BLOCK_SIZE)
       return bytes;

#if (WINDOWS_98_UTIL)
    rl.x.ax = 0x440D;    // generic IOCTL
    rl.h.bh = 3;         // locks levels 0, 1, 2 or 3
    rl.h.bl = (0x80 | (disk & 0x7F)); // dos limit is 128 drives
    rl.h.ch = 0x08;      // device category
    rl.h.cl = 0x4B;      // lock physical volume
    rl.x.dx = 0;         // permissions
    int86(0x21, &rl, &rl);

#endif

    if (NWDisk->Int13Extensions)
    {
       EXT_REQUEST request;

       NWFSSet(&request, 0, sizeof(EXT_REQUEST));

       request.Size = sizeof(EXT_REQUEST);
       request.Blocks = sectors;
       request.TransferBuffer = (ULONG)(NWDisk->DataSegment << 16) & 0xFFFF0000;
       request.LBA = StartingLBA;

       movedata(_my_ds(),
		(unsigned)&request,
		NWDisk->RequestSelector,
		0,
		sizeof(EXT_REQUEST));

       movedata(_my_ds(),
		(unsigned)Sector,
		NWDisk->DataSelector,
		0,
		(sectors * bps));

       r.h.ah = 0x43;
       r.h.al = NO_VERIFY;
       r.h.dl = (0x80 | (disk & 0x7F));
       r.x.ds = NWDisk->RequestSegment;
       r.x.si = 0;
       r.x.ss = r.x.sp = r.x.flags = 0;

       ccode = _go32_dpmi_simulate_int(0x13, &r);
       if (ccode)
	  goto WriteExit;

       // error if carry flag is set
       if (r.x.flags & 1)
       {
	  // if the drive reported an ECC occurred, return success
	  if (r.h.ah == 0x11)
	  {
	     bytes = (sectors * bps);
	     goto WriteExit;
	  }
	  else
	     goto WriteExit;
       }
       bytes = (sectors * bps);
       goto WriteExit;
    }
    else
    {
       ccode = LBAtoCHS(disk, StartingLBA, &cylinder, &head, &sector);
       if (ccode)
	  goto WriteExit;

       movedata(_my_ds(),
		(unsigned)Sector,
		NWDisk->DataSelector,
		0,
		(sectors * bps));

       r.h.dh = (BYTE)head;
       r.h.dl = (0x80 | (disk & 0x7F));
       r.h.ch = (BYTE)(cylinder & 0xFF);
       r.h.cl = (BYTE)((cylinder & 0x0300) >> 2);
       r.h.cl |= (BYTE)(sector & 0x3F);
       r.h.ah = WRITE_DEVICE;
       r.h.al = (BYTE)sectors;
       r.x.es = NWDisk->DataSegment;
       r.x.bx = 0;
       r.x.ds = r.x.ss = r.x.sp = r.x.flags = 0;

       ccode = _go32_dpmi_simulate_int(0x13, &r);
       if (ccode)
	  goto WriteExit;

       // error if carry flag is set
       if (r.x.flags & 1)
       {
	  // if the drive reported an ECC occurred, return success
	  if (r.h.ah == 0x11)
	  {
	     bytes = (sectors * bps);
	     goto WriteExit;
	  }
	  else
	     goto WriteExit;
       }
       bytes = (sectors * bps);
       goto WriteExit;
    }

WriteExit:;
#if (WINDOWS_98_UTIL)
    rl.x.ax = 0x440D;    // generic IOCTL
    rl.h.bl = (0x80 | (disk & 0x7F)); // dos limit is 128 drives
    rl.h.ch = 0x08;      // device category
    rl.h.cl = 0x6B;      // unlock physical volume
    int86(0x21, &rl, &rl);
#endif

    return bytes;
}

ULONG pZeroFillDiskSectors(ULONG disk, ULONG StartingLBA, ULONG sectors,
			  ULONG readAhead)
{
    register ULONG bps, ccode, bytes = 0;
    register NWDISK *NWDisk;
    LONGLONG cylinder;
    ULONG head, sector;
    static _go32_dpmi_registers r;
#if (WINDOWS_98_UTIL)
    static union REGS rl;
#endif

    NWFSSet(&r, 0, sizeof(_go32_dpmi_registers));

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;

    if ((sectors * bps) > IO_BLOCK_SIZE)
       return bytes;

    movedata(_my_ds(),
	     (unsigned)ZeroBuffer,
	     NWDisk->DataSelector,
	     0,
	     (sectors * bps));

#if (WINDOWS_98_UTIL)
    rl.x.ax = 0x440D;    // generic IOCTL
    rl.h.bh = 3;         // locks levels 0, 1, 2 or 3
    rl.h.bl = (0x80 | (disk & 0x7F)); // dos limit is 128 drives
    rl.h.ch = 0x08;      // device category
    rl.h.cl = 0x4B;      // lock physical volume
    rl.x.dx = 0;         // permissions
    int86(0x21, &rl, &rl);

#endif

    if (NWDisk->Int13Extensions)
    {
       EXT_REQUEST request;

       NWFSSet(&request, 0, sizeof(EXT_REQUEST));

       request.Size = sizeof(EXT_REQUEST);
       request.Blocks = sectors;
       request.TransferBuffer = (ULONG)(NWDisk->DataSegment << 16) & 0xFFFF0000;
       request.LBA = StartingLBA;

       movedata(_my_ds(),
		(unsigned)&request,
		NWDisk->RequestSelector,
		0,
		sizeof(EXT_REQUEST));

       r.h.ah = 0x43;
       r.h.al = NO_VERIFY;
       r.h.dl = (0x80 | (disk & 0x7F));
       r.x.ds = NWDisk->RequestSegment;
       r.x.si = 0;
       r.x.ss = r.x.sp = r.x.flags = 0;

       ccode = _go32_dpmi_simulate_int(0x13, &r);
       if (ccode)
	  goto FillExit;

       // error if carry flag is set
       if (r.x.flags & 1)
       {
	  // if the drive reported an ECC occurred, return success
	  if (r.h.ah == 0x11)
	  {
	     bytes = (sectors * bps);
	     goto FillExit;
	  }
	  else
	     goto FillExit;
       }
       bytes = (sectors * bps);
       goto FillExit;
    }
    else
    {
       ccode = LBAtoCHS(disk, StartingLBA, &cylinder, &head, &sector);
       if (ccode)
	  goto FillExit;

       r.h.dh = (BYTE)head;
       r.h.dl = (0x80 | (disk & 0x7F));
       r.h.ch = (BYTE)(cylinder & 0xFF);
       r.h.cl = (BYTE)((cylinder & 0x0300) >> 2);
       r.h.cl |= (BYTE)(sector & 0x3F);
       r.h.ah = 0x03;
       r.h.al = (BYTE)sectors;
       r.x.es = NWDisk->DataSegment;
       r.x.bx = 0;
       r.x.ds = r.x.ss = r.x.sp = r.x.flags = 0;

       ccode = _go32_dpmi_simulate_int(0x13, &r);
       if (ccode)
	  goto FillExit;

       // error if carry flag is set
       if (r.x.flags & 1)
       {
	  // if the drive reported an ECC occurred, return success
	  if (r.h.ah == 0x11)
	  {
	     bytes = (sectors * bps);
	     goto FillExit;
	  }
	  else
	     goto FillExit;
       }
       bytes = (sectors * bps);
       goto FillExit;
    }

FillExit:;
#if (WINDOWS_98_UTIL)
    rl.x.ax = 0x440D;    // generic IOCTL
    rl.h.bl = (0x80 | (disk & 0x7F)); // dos limit is 128 drives
    rl.h.ch = 0x08;      // device category
    rl.h.cl = 0x6B;      // unlock physical volume
    int86(0x21, &rl, &rl);
#endif

    return bytes;

}

void SyncDevice(ULONG disk)
{
    fsync(disk);
    return;
}

ULONG ReadDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			     ULONG sectors, ULONG readAhead)
{
    return (pReadDiskSectors(disk, LBA, Sector, sectors, readAhead));
}

ULONG WriteDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			      ULONG sectors, ULONG readAhead)
{
    return (pWriteDiskSectors(disk, LBA, Sector, sectors, readAhead));
}

ULONG ZeroFillDiskSectors(ULONG disk, ULONG StartingLBA,
				 ULONG sectors, ULONG readAhead)
{
    return (pZeroFillDiskSectors(disk, StartingLBA, sectors, readAhead));
}

#endif


#if (WINDOWS_NT_UTIL)

//
//  low level disk sector interface routines
//

ULONG pReadDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		      ULONG sectors, ULONG readAhead)
{
    register ULONG bps, cbytes, ccode;
    register NWDISK *NWDisk;
    LARGE_INTEGER offset;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;

    offset.QuadPart = (LONGLONG) StartingLBA * (LONGLONG) bps;

    ccode = SetFilePointer(NWDisk->PhysicalDiskHandle, offset.LowPart,
				&offset.HighPart, FILE_BEGIN);
    if (ccode == (LONG) -1)
       return 0;

    cbytes = FileRead(NWDisk->PhysicalDiskHandle, Sector, sectors * bps);

    return cbytes;

}


ULONG pWriteDiskSectors(ULONG disk, ULONG StartingLBA, BYTE *Sector,
		       ULONG sectors, ULONG readAhead)
{
    register ULONG bps, cbytes, ccode;
    register NWDISK *NWDisk;
    LARGE_INTEGER offset;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;

    offset.QuadPart = (LONGLONG) StartingLBA * (LONGLONG) bps;

    ccode = SetFilePointer(NWDisk->PhysicalDiskHandle, offset.LowPart,
				&offset.HighPart, FILE_BEGIN);
    if (ccode == (LONG) -1)
       return 0;

    cbytes = FileWrite(NWDisk->PhysicalDiskHandle, Sector, sectors * bps);
    return cbytes;
}

ULONG pZeroFillDiskSectors(ULONG disk, ULONG StartingLBA, ULONG sectors,
			  ULONG readAhead)
{
    LONGLONG ccode;
    register ULONG bps, cbytes;
    register NWDISK *NWDisk;
    LARGE_INTEGER offset;

    NWDisk = SystemDisk[disk];
    bps = NWDisk->BytesPerSector;

    offset.QuadPart = (LONGLONG) StartingLBA * (LONGLONG) bps;

    ccode = SetFilePointer(NWDisk->PhysicalDiskHandle, offset.LowPart,
			   &offset.HighPart, FILE_BEGIN);
    if (ccode == (LONG) -1)
       return 0;

    cbytes = FileWrite(NWDisk->PhysicalDiskHandle, ZeroBuffer, sectors * bps);

    return cbytes;

}

void SyncDevice(ULONG disk)
{
    return;
}

ULONG ReadDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			     ULONG sectors, ULONG readAhead)
{
    return (pReadDiskSectors(disk, LBA, Sector, sectors, readAhead));
}

ULONG WriteDiskSectors(ULONG disk, ULONG LBA, BYTE *Sector,
			      ULONG sectors, ULONG readAhead)
{
    return (pWriteDiskSectors(disk, LBA, Sector, sectors, readAhead));
}

ULONG ZeroFillDiskSectors(ULONG disk, ULONG StartingLBA,
				 ULONG sectors, ULONG readAhead)
{
    return (pZeroFillDiskSectors(disk, StartingLBA, sectors, readAhead));
}

#endif
#endif
