
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
*   FILE     :  ASYNC.C
*   DESCRIP  :  Asynch IO Library
*   DATE     :  July 13, 2000 (my birthday)
*
*
***************************************************************************/

#include "globals.h"

// We use eight (8) processes for the async io manager.  Each disk bin
// is computed as disk % 8 and matched into one of eight bin groups.
// On SMP versions of linux, this should allow us to keep as many
// drive spindles active as possible at one time without eating too
// many server processes to do this, provided the linux kernel
// load balances these processes across processors.

ASYNCH_IO *disk_io_head[8] = { 0, 0, 0, 0, 0, 0, 0 };
ASYNCH_IO *disk_io_tail[8] = { 0, 0, 0, 0, 0, 0, 0 };
ASYNCH_IO_HEAD asynch_io_head[MAX_DISKS];

ULONG aio_submitted = 0;
ULONG aio_completed = 0;
ULONG aio_error = 0;
ULONG disk_aio_submitted = 0;
ULONG disk_aio_completed = 0;
ULONG sync_active[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG async_active[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG cb_active = 0;
ULONG aio_sequence = 0;

#if (PROFILE_AIO)
ULONG hash_hits = 0;
ULONG hash_misses = 0;
ULONG hash_fill = 0;
ULONG hash_total = 0;
double probe_avg = 0;
ULONG probe_max = 0;

ULONG total_read_req = 0;
ULONG total_write_req = 0;
ULONG total_fill_req = 0;
ULONG total_complete = 0;
ULONG req_sec = 0;
ULONG seconds = 0;
#endif

#if (LINUX_SLEEP)

NWFSInitMutex(disk_io_sem0);
NWFSInitMutex(disk_io_sem1);
NWFSInitMutex(disk_io_sem2);
NWFSInitMutex(disk_io_sem3);
NWFSInitMutex(disk_io_sem4);
NWFSInitMutex(disk_io_sem5);
NWFSInitMutex(disk_io_sem6);
NWFSInitMutex(disk_io_sem7);

#if (LINUX_SPIN)
spinlock_t aio_spinlock = SPIN_LOCK_UNLOCKED;
long aio_flags = 0;
#else
NWFSInitMutex(asynch_head_lock);
#endif

struct semaphore *io_sem_table[8]={
    &disk_io_sem0, &disk_io_sem1, &disk_io_sem2, &disk_io_sem3,
    &disk_io_sem4, &disk_io_sem5, &disk_io_sem6, &disk_io_sem7
};

#endif

extern void RunAsynchIOQueue(ULONG disk);

#if (PROFILE_AIO)
void profile_complete(void)
{
   total_complete++;
}
#endif

void asynch_lock(void)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&aio_spinlock, aio_flags);
#else
    if (WaitOnSemaphore(&asynch_head_lock) == -EINTR)
       NWFSPrint("asynch lock was interrupted\n");
#endif
#endif
}

void asynch_unlock(void)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&aio_spinlock, aio_flags);
#else
    SignalSemaphore(&asynch_head_lock);
#endif
#endif
}

ULONG hash_disk_io(ASYNCH_IO *io)
{
    register int i = (io->disk % MAX_DISKS);
    register int j = ((io->sector_offset >> 8) & 0xF);
    ASYNCH_IO *old, *p;

    if (!asynch_io_head[i].hash_head[j])
    {
       io->hnext = io->hprior = NULL;
       asynch_io_head[i].hash_head[j] = io;
       asynch_io_head[i].hash_tail[j] = io;
       return 0;
    }

    p = asynch_io_head[i].hash_head[j];
    old = NULL;
    while (p)
    {
       if (p->disk != io->disk)
       {
          NWFSPrint("nwfs:  io request has bad disk id (%d/%d)\n",
                    (int)p->disk, (int)io->disk);
          return -1;
       }

       if (p->sector_offset < io->sector_offset)
       {
	  old = p;
	  p = p->hnext;
       }
       else
       {
	  if (p->hprior)
	  {
	     p->hprior->hnext = io;
	     io->hnext = p;
	     io->hprior = p->hprior;
	     p->hprior = io;
	     return 0;
	  }
	  io->hnext = p;
	  io->hprior = NULL;
	  p->hprior = io;
	  asynch_io_head[i].hash_head[j] = io;
	  return 0;
       }
    }
    old->hnext = io;
    io->hnext = NULL;
    io->hprior = old;
    asynch_io_head[i].hash_tail[j] = io;
    return 0;
}

ULONG unhash_disk_io(ASYNCH_IO *io)
{
    register int i = (io->disk % MAX_DISKS);
    register int j = ((io->sector_offset >> 8) & 0xF);

    if (asynch_io_head[i].hash_head[j] == io)
    {
       asynch_io_head[i].hash_head[j] = (void *) io->hnext;
       if (asynch_io_head[i].hash_head[j])
	  asynch_io_head[i].hash_head[j]->hprior = NULL;
       else
	  asynch_io_head[i].hash_tail[j] = NULL;
    }
    else
    {
       io->hprior->hnext = io->hnext;
       if (io != asynch_io_head[i].hash_tail[j])
	  io->hnext->hprior = io->hprior;
       else
	  asynch_io_head[i].hash_tail[j] = io->hprior;
    }
    io->hnext = io->hprior = 0;

    return 0;
}

void process_sync_io(ULONG disk)
{
   register int i = (disk % 8);
   register int r, j;
   ASYNCH_IO *list, *io;

   sync_active[i]++;
   
   while (disk_io_head[i])
   {
      asynch_lock();
      list = disk_io_head[i];
      disk_io_head[i] = disk_io_tail[i] = 0;
      for (r=0; r < MAX_DISKS; r++)
      {
         if ((r % 8) == i)
         {
            for (j=0; j < 16; j++)
            {
               asynch_io_head[r].hash_head[j] = 0;
               asynch_io_head[r].hash_tail[j] = 0;
            }
         }
      }
      asynch_unlock();

      while (list)
      {
         io = list;
         list = list->next;
         io->next = io->prior = 0;

	 if (io->signature != ASIO_SUBMIT_IO)
         {
            NWFSPrint("sync - bad io (0x%08X) request cmd-%X s-%X\n",
	              (unsigned)io, (unsigned)io->command, 
		      (unsigned)io->signature);
            io->ccode = ASIO_BAD_SIGNATURE;
            if (io->call_back_routine)
               (io->call_back_routine)(io);
            io->signature = ASIO_COMPLETED;
            aio_error++;
            aio_completed++;
            continue;
         }

         switch (io->command)
         {
            case ASYNCH_TEST:
               disk_aio_submitted++;
               io->ccode = 0;
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               disk_aio_completed++;
               aio_completed++;
	       break;
     
	    case ASYNCH_READ:
               disk_aio_submitted++;
               io->return_code = pReadDiskSectors(io->disk,
	                                          io->sector_offset,
                                                  io->buffer,
						  io->sector_count,
				                  io->sector_count);
               io->ccode = 0;
	       if (!io->return_code)
               {
                  io->ccode = ASIO_IO_ERROR;
                  if (io->call_back_routine)
                     (io->call_back_routine)(io);
                  io->signature = ASIO_COMPLETED;
	          disk_aio_completed++;
                  aio_completed++;
                  break;
               }
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               disk_aio_completed++;
               aio_completed++;
               break;

            case ASYNCH_WRITE:
               disk_aio_submitted++;
               io->return_code = pWriteDiskSectors(io->disk,
	                                           io->sector_offset,
                                                   io->buffer,
						   io->sector_count,
				                   io->sector_count);
               io->ccode = 0;
	       if (!io->return_code)
               {
                  io->ccode = ASIO_IO_ERROR;
                  if (io->call_back_routine)
                     (io->call_back_routine)(io);
                  io->signature = ASIO_COMPLETED;
	          disk_aio_completed++;
                  aio_completed++;
                  break;
               }
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               disk_aio_completed++;
               aio_completed++;
               break;

            case ASYNCH_FILL:
               disk_aio_submitted++;
               io->return_code = pZeroFillDiskSectors(io->disk,
	                                              io->sector_offset,
						      io->sector_count,
				                      io->sector_count);
               io->ccode = 0;
	       if (!io->return_code)
               {
                  io->ccode = ASIO_IO_ERROR;
                  if (io->call_back_routine)
                     (io->call_back_routine)(io);
                  io->signature = ASIO_COMPLETED;
	          disk_aio_completed++;
                  aio_completed++;
                  break;
               }
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               disk_aio_completed++;
               aio_completed++;
               break;

	    default:
               io->ccode = ASIO_BAD_COMMAND;
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               aio_error++;
               aio_completed++;
               break;
	 }
      }
   }

   sync_active[i]--;
   return;
}

#if (!LINUX_20 && LINUX_AIO && !DOS_UTIL && !LINUX_UTIL && !WINDOWS_NT_UTIL && !LINUX_BUFFER_CACHE)

void process_asynch_io(ULONG disk)
{
   register int i = (disk % 8);
   register int r, j;
   ASYNCH_IO *list, *io;

   // continue to cycle through this list until we have completely
   // emptied the list.

   async_active[i]++;

   while (disk_io_head[i])
   {
      // take the entire list, zero the head and tail and process the list
      // from this context.  this will simulate an alternating A and B list
      // and avoid elevator starvation.  We also need to clear all the
      // asynch hash list info for this run so folks don't attempt to
      // link new aio requests to this active aio chain.

      asynch_lock();
      list = disk_io_head[i];
      disk_io_head[i] = disk_io_tail[i] = 0;
      for (r=0; r < MAX_DISKS; r++)
      {
         if ((r % 8) == i)
         {
            for (j=0; j < 16; j++)
            {
               asynch_io_head[r].hash_head[j] = 0;
               asynch_io_head[r].hash_tail[j] = 0;
            }
         }
      }
      asynch_unlock();

      while (list)
      {
         io = list;
         list = list->next;
         io->next = io->prior = 0;
	 
	 if (io->signature != ASIO_SUBMIT_IO)
         {
            NWFSPrint("\naio process - bad io (0x%08X) cmd-%X sig-%X cb-%X d/s-%d/%X\n",
                      (unsigned)io,
	              (unsigned)io->command, 
		      (unsigned)io->signature,
		      (unsigned)io->call_back_routine,
		      (int)io->disk,
		      (unsigned)io->sector_offset);

//	    io->ccode = ASIO_BAD_SIGNATURE;
//            if (io->call_back_routine)
//               (io->call_back_routine)(io);
//            io->signature = ASIO_COMPLETED;
//            aio_error++;
//            aio_completed++;
//            continue;
         }

         switch (io->command)
         {
            case ASYNCH_TEST:
               disk_aio_submitted++;
               io->ccode = 0;
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               disk_aio_completed++;
               aio_completed++;
	       break;

	    case ASYNCH_READ:
#if (PROFILE_AIO)
               total_read_req++;
#endif
               disk_aio_submitted++;
	       io->return_code = aReadDiskSectors(io->disk,
	                                          io->sector_offset,
                                                  io->buffer,
						  io->sector_count,
				                  io->sector_count,
						  io);
	       if (!io->return_code)
               {
                  io->ccode = ASIO_IO_ERROR;
                  if (io->call_back_routine)
                     (io->call_back_routine)(io);
                  io->signature = ASIO_COMPLETED;
                  disk_aio_completed++;
                  aio_completed++;
               }
               io->signature = ASIO_AIO_POST;
	       break;

            case ASYNCH_WRITE:
#if (PROFILE_AIO)
               total_write_req++;
#endif
               disk_aio_submitted++;
	       io->return_code = aWriteDiskSectors(io->disk,
	                                           io->sector_offset,
                                                   io->buffer,
						   io->sector_count,
				                   io->sector_count,
						   io);
	       if (!io->return_code)
               {
                  io->ccode = ASIO_IO_ERROR;
                  if (io->call_back_routine)
                     (io->call_back_routine)(io);
                  io->signature = ASIO_COMPLETED;
                  disk_aio_completed++;
                  aio_completed++;
               }
               io->signature = ASIO_AIO_POST;
               break;

            case ASYNCH_FILL:
#if (PROFILE_AIO)
               total_fill_req++;
#endif
               disk_aio_submitted++;
	       io->return_code = aZeroFillDiskSectors(io->disk,
	                                              io->sector_offset,
						      io->sector_count,
				                      io->sector_count,
						      io);
	       if (!io->return_code)
               {
                  io->ccode = ASIO_IO_ERROR;
                  if (io->call_back_routine)
                     (io->call_back_routine)(io);
                  io->signature = ASIO_COMPLETED;
                  disk_aio_completed++;
                  aio_completed++;
               }
               io->signature = ASIO_AIO_POST;
               break;

            default:
               io->ccode = ASIO_BAD_COMMAND;
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               aio_error++;
               aio_completed++;
               break;
	 }
      }
#if (!POST_IMMEDIATE)
#if (LINUX_22)
      lock_kernel();
#endif
      run_task_queue(&tq_disk);
#if (LINUX_22)
      unlock_kernel();
#endif
#endif
   }

   async_active[i]--;
   return;
}

#else

void process_asynch_io(ULONG disk)
{
   register int i = (disk % 8);
   register int r, j;
   ASYNCH_IO *list, *io;

   // continue to cycle through this list until we have completely
   // emptied the list.

   async_active[i]++;

   while (disk_io_head[i])
   {
      // take the entire list, zero the head and tail and process the list
      // from this context.  this will simulate an alternating A and B list
      // and avoid elevator starvation.  We also need to clear all the
      // asynch hash list info for this run so folks don't attempt to
      // link new aio requests to this active aio chain.

      asynch_lock();
      list = disk_io_head[i];
      disk_io_head[i] = disk_io_tail[i] = 0;
      for (r=0; r < MAX_DISKS; r++)
      {
         if ((r % 8) == i)
         {
            for (j=0; j < 16; j++)
            {
               asynch_io_head[r].hash_head[j] = 0;
               asynch_io_head[r].hash_tail[j] = 0;
            }
         }
      }

#if (PROFILE_AIO)
      if (hash_hits || hash_misses)
      {
         if (hash_total)
            probe_avg = (double)((double)probe_avg / (double)hash_total);
         else
            probe_avg = 0;

         // we seem to average 96% hit efficiency for locating an insertion
	 // point in the aio list with an average of 1 probe per insert.  The
         // other 4% involve cases where a single element is on the list,
         // and we only probe 1 time to find our insert point.

#if (VERBOSE)
         NWFSPrint("rd_aio-%d wr_aio-%d f_aio-%d compl-%d req/sec-%d\n",
                   (int)total_read_req, (int)total_write_req,
                   (int)total_fill_req, (int)total_complete,
                   (int)(seconds ? (total_complete / seconds) : total_complete));

         NWFSPrint("hits-%d misses-%d fill-%d total-%d probe_avg-%d probe_max-%d\n",
	           (int)hash_hits, (int)hash_misses, (int)hash_fill,
		   (int)hash_total, (int)probe_avg, (int)probe_max);
#endif

      }
      total_read_req = total_write_req = total_fill_req = total_complete = 0;
      req_sec = seconds = 0;
      hash_hits = hash_misses = hash_fill = hash_total = probe_avg = probe_max = 0;
#endif
      asynch_unlock();

      while (list)
      {
         io = list;
         list = list->next;
         io->next = io->prior = 0;
 
	 if (io->signature != ASIO_SUBMIT_IO)
         {
            NWFSPrint("\nnwfs aio - bad io (0x%08X) cmd-%X s-%X cb-%X \nd/s-%d/%X\n",
                      (unsigned)io,
      		      (unsigned)io->command, 
		      (unsigned)io->signature,
		      (unsigned)io->call_back_routine,
		      (int)io->disk,
		      (unsigned)io->sector_offset);

//	    io->ccode = ASIO_BAD_SIGNATURE;
//            if (io->call_back_routine)
//               (io->call_back_routine)(io);
//            io->signature = ASIO_COMPLETED;
//            aio_error++;
//            aio_completed++;
//            continue;
         }

         switch (io->command)
         {
            case ASYNCH_TEST:
               disk_aio_submitted++;
               io->ccode = 0;
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               disk_aio_completed++;
               aio_completed++;
	       break;

	    case ASYNCH_READ:
#if (PROFILE_AIO)
               total_read_req++;
#endif
               disk_aio_submitted++;
               io->return_code = pReadDiskSectors(io->disk,
	                                          io->sector_offset,
                                                  io->buffer,
						  io->sector_count,
				                  io->sector_count);
               io->ccode = 0;
	       if (!io->return_code)
               {
                  io->ccode = ASIO_IO_ERROR;
                  if (io->call_back_routine)
                     (io->call_back_routine)(io);
                  io->signature = ASIO_COMPLETED;
                  disk_aio_completed++;
                  aio_completed++;
                  break;
               }
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               disk_aio_completed++;
               aio_completed++;
               break;

            case ASYNCH_WRITE:
#if (PROFILE_AIO)
               total_write_req++;
#endif
               disk_aio_submitted++;
               io->return_code = pWriteDiskSectors(io->disk,
	                                           io->sector_offset,
                                                   io->buffer,
						   io->sector_count,
				                   io->sector_count);
               io->ccode = 0;
	       if (!io->return_code)
               {
                  io->ccode = ASIO_IO_ERROR;
                  if (io->call_back_routine)
                     (io->call_back_routine)(io);
                  io->signature = ASIO_COMPLETED;
                  disk_aio_completed++;
                  aio_completed++;
                  break;
               }
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               disk_aio_completed++;
               aio_completed++;
               break;

            case ASYNCH_FILL:
#if (PROFILE_AIO)
               total_fill_req++;
#endif
               disk_aio_submitted++;
               io->return_code = pZeroFillDiskSectors(io->disk,
	                                              io->sector_offset,
						      io->sector_count,
				                      io->sector_count);
               io->ccode = 0;
	       if (!io->return_code)
               {
                  io->ccode = ASIO_IO_ERROR;
                  if (io->call_back_routine)
                     (io->call_back_routine)(io);
                  io->signature = ASIO_COMPLETED;
                  disk_aio_completed++;
                  aio_completed++;
                  break;
               }
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               disk_aio_completed++;
               aio_completed++;
               break;

	    default:
               io->ccode = ASIO_BAD_COMMAND;
               if (io->call_back_routine)
                  (io->call_back_routine)(io);
               io->signature = ASIO_COMPLETED;
               aio_error++;
               aio_completed++;
               break;
	 }
      }
   }

   async_active[i]--;
   return;
}

#endif

ULONG asynch_io_pending(ULONG disk)
{
   return ((disk_io_head[(disk % 8)]) ? 1 : 0);
}

ASYNCH_IO *remove_io(ULONG disk, ASYNCH_IO *io)
{
    register int i = (disk % 8);

    if (disk_io_head[i] == io)
    {
       disk_io_head[i] = (void *) io->next;
       if (disk_io_head[i])
	  disk_io_head[i]->prior = NULL;
       else
	  disk_io_tail[i] = NULL;
    }
    else
    {
       io->prior->next = io->next;
       if (io != disk_io_tail[i])
	  io->next->prior = io->prior;
       else
	  disk_io_tail[i] = io->prior;
    }
    io->next = io->prior = 0;

    return io;

}

ULONG index_io(ULONG disk, ASYNCH_IO *io)
{
    register int i = (disk % 8);
    register int r = (io->disk % MAX_DISKS);
    register int j = ((io->sector_offset >> 8) & 0xF);
#if (PROFILE_AIO)
    register int count;
#endif
    ASYNCH_IO *old, *p;

#if (PROFILE_AIO)
    count = 1;
#endif
    if (!disk_io_tail[i])
    {
       io->next = io->prior = NULL;
       disk_io_head[i] = io;
       disk_io_tail[i] = io;
#if (PROFILE_AIO)
       hash_fill++;
       hash_total++;

       if (count > probe_max)
          probe_max = count;
       probe_avg += count;
#endif
       return 0;
    }

    if (asynch_io_head[r].hash_head[j])
    {
#if (PROFILE_AIO)
       hash_hits++;
#endif
       p = asynch_io_head[r].hash_head[j];
    }
    else
    {
#if (PROFILE_AIO)
       hash_misses++;
#endif
       p = disk_io_head[i];
    }
#if (PROFILE_AIO)
    hash_total++;
#endif

    old = NULL;
    while (p)
    {
       if ((p->disk < io->disk) || 
	   (p->sector_offset < io->sector_offset))
       {
	  old = p;
	  p = p->next;
#if (PROFILE_AIO)
          count++;
#endif
       }
       else
       {
	  if (p->prior)
	  {
	     p->prior->next = io;
	     io->next = p;
	     io->prior = p->prior;
	     p->prior = io;

#if (PROFILE_AIO)
             if (count > probe_max)
                probe_max = count;
             probe_avg += count;
#endif
	     return 0;
	  }
	  io->next = p;
	  io->prior = NULL;
	  p->prior = io;
	  disk_io_head[i] = io;

#if (PROFILE_AIO)
          if (count > probe_max)
             probe_max = count;
          probe_avg += count;
#endif
	  return 0;
       }
    }
    old->next = io;
    io->next = NULL;
    io->prior = old;
    disk_io_tail[i] = io;

#if (PROFILE_AIO)
    if (count > probe_max)
       probe_max = count;
    probe_avg += count;
#endif
    return 0;
}

void insert_io(ULONG disk, ASYNCH_IO *io)
{
    asynch_lock();
    if (io->signature == ASIO_SUBMIT_IO)
    {
       NWFSPrint("nwfs:  asynch io request already active\n");
       asynch_unlock();
       return;
    }
    io->signature = ASIO_SUBMIT_IO;

    index_io(disk, io);
    hash_disk_io(io);

    aio_submitted++;

    if (io->signature != ASIO_SUBMIT_IO)
       NWFSPrint("nwfs:  insert_io request not active (%X)\n",
		 (unsigned)io->signature);

    asynch_unlock();
    return;

}

//
//
//

ASYNCH_IO *io_callback_head = 0;
ASYNCH_IO *io_callback_tail = 0;

#if (LINUX_SLEEP)
NWFSInitSemaphore(callback_semaphore);
#endif

#if (LINUX_SLEEP)
#if (LINUX_SPIN)
spinlock_t cb_spinlock = SPIN_LOCK_UNLOCKED;
long cb_flags = 0;
#else
NWFSInitMutex(cb_sem);
#endif
#endif

void cb_lock(void)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&cb_spinlock, cb_flags);
#else
    if (WaitOnSemaphore(&cb_sem) == -EINTR)
       NWFSPrint("cb lock was interrupted\n");
#endif
#endif
}

void cb_unlock(void)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&cb_spinlock, cb_flags);
#else
    SignalSemaphore(&cb_sem);
#endif
#endif
}

ASYNCH_IO *get_callback(void)
{
    register ASYNCH_IO *io;

    cb_lock();
    io = io_callback_head;
    if (io)
    {
       io_callback_head = io->next;
       if (io_callback_head)
	  io_callback_head->prior = NULL;
       else
	  io_callback_tail = NULL;

       io->next = io->prior = 0;
       cb_unlock();
       return io;
    }
    cb_unlock();
    return 0;
}

void insert_callback(ASYNCH_IO *io)
{
#if (LINUX_SPIN)
    if (io->flags & ASIO_INTR_CALLBACK)
    {
       register ULONG ccode = 0;
       
       if (io->call_back_routine)
          ccode = (io->call_back_routine)(io);

       if (!ccode)
       {
          disk_aio_completed++;
          io->signature = ASIO_CALLBACK_POST;
          io->signature = ASIO_COMPLETED;
          aio_completed++;
          return;
       }
    }
#endif
    
    NWFSPrint("sleep callback\n");

    cb_lock();

    if (io->signature != ASIO_AIO_POST)
    {
          NWFSPrint("\nins callback - bad io (0x%08X) cmd-%X sig-%X cb-%X \nd/s-%d/%X\n",
              (unsigned)io, (unsigned)io->command, (unsigned)io->signature,
	      (unsigned)io->call_back_routine, (int)io->disk,
	      (unsigned)io->sector_offset);
    }

    disk_aio_completed++;
    
    if (!io_callback_head)
    {
       io_callback_head = io;
       io_callback_tail = io;
       io->next = io->prior = 0;
    }
    else
    {
       io_callback_tail->next = io;
       io->next = 0;
       io->prior = io_callback_tail;
       io_callback_tail = io;
    }

    io->signature = ASIO_CALLBACK_POST;
    
#if (LINUX_SLEEP)
    SignalSemaphore(&callback_semaphore);
#endif

    cb_unlock();
    return;
}

void process_callback(void)
{
    register ASYNCH_IO *io;

    cb_active++;
    io = get_callback();
    while (io)
    {
       if (io->signature != ASIO_CALLBACK_POST)
       {
          NWFSPrint("\nproc callback - bad io (0x%08X) cmd-%X sig-%X cb-%X \nd/s-%d/%X\n",
              (unsigned)io, (unsigned)io->command, (unsigned)io->signature,
              (unsigned)io->call_back_routine, (int)io->disk, 
	      (unsigned)io->sector_offset);

	  io->ccode = ASIO_BAD_SIGNATURE;
	  if (io->call_back_routine)
             (io->call_back_routine)(io);
	  io->signature = ASIO_COMPLETED;
	  aio_error++;
          aio_completed++;
	  io = get_callback();
	  continue;
       }

       if (io->call_back_routine)
          (io->call_back_routine)(io);
       io->signature = ASIO_COMPLETED;
       aio_completed++;

       io = get_callback();
    }
    cb_active--;
    return;
}

