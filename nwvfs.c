
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
*   FILE     :  NWVFS.C
*   DESCRIP  :   VFS Module for Linux
*   DATE     :  November 16, 1998
*
*
***************************************************************************/

#include "globals.h"

extern void DisplayLRUInfo(LRU_HANDLE *lru_handle);

#if (DEBUG_LRU_AIO)
ULONG delay_counter = 0;

ULONG get_aio_count(ASYNCH_IO *io)
{
    register ULONG i = 0;

    while (io)
    {
       i++;
       io = io->next;
    }
    return i;
}

void displayNWFSState(void)
{
    extern BYTE *dumpRecord(BYTE *, ULONG);
    extern void displayMemoryInfo(void);

    NWFSPrint("\n");
//    DisplayLRUInfo(&DataLRU);
//    DisplayLRUInfo(&JournalLRU);
//    DisplayLRUInfo(&FATLRU);
    displayMemoryInfo();
}

#endif

#if (LINUX_24)

#if (PROFILE_AIO)
extern ULONG seconds;
#endif

struct task_struct *lru_task;
extern ULONG lru_active;
extern ULONG lru_state;
extern ULONG lru_signal;
NWFSInitSemaphore(lru_sem);

ULONG callback_active = 0;
ULONG exit_callback = 0;
struct task_struct *callback_task = 0;
extern struct semaphore callback_semaphore;
extern void process_callback(void);

int nwfs_callback(void *notused)
{
    sprintf(current->comm, "nwfs-callback");
    daemonize();
    callback_task = current;

    callback_active++;
    while (!exit_callback)
    {
       down(&callback_semaphore);

       if (exit_callback)
	  break;

       process_callback();
    }
    callback_active--;
    return 0;
}

ULONG remirror_active;
ULONG flush_active;
ULONG asynch_io_active[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

ULONG exit_remirror;
ULONG exit_flush;
ULONG exit_asynch_io[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG asynch_signal[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG asynch_active[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG asynch_threads = 0;

NWFSInitSemaphore(mirror_sem);
NWFSInitSemaphore(asynch_io_sem0);
NWFSInitSemaphore(asynch_io_sem1);
NWFSInitSemaphore(asynch_io_sem2);
NWFSInitSemaphore(asynch_io_sem3);
NWFSInitSemaphore(asynch_io_sem4);
NWFSInitSemaphore(asynch_io_sem5);
NWFSInitSemaphore(asynch_io_sem6);
NWFSInitSemaphore(asynch_io_sem7);

struct semaphore *sem_table[8]={
   &asynch_io_sem0, &asynch_io_sem1, &asynch_io_sem2, &asynch_io_sem3,
   &asynch_io_sem4, &asynch_io_sem5, &asynch_io_sem6, &asynch_io_sem7
};

struct task_struct *remirror_task = 0;
struct task_struct *asynch_io_task[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
struct timer_list flush_timer = { { NULL, NULL }, 0, };

extern ULONG nwvp_remirror_poll(void);
extern void nwvp_quiese_remirroring(void);
extern void process_asynch_io(ULONG disk);
extern void process_sync_io(ULONG disk);
extern ULONG asynch_io_pending(ULONG disk);

int count = 0;

void NWFSStartRemirror(void)
{
   if (remirror_task != current)
      SignalSemaphore(&mirror_sem);
}

void NWFSFlush(void)
{
   if (lru_task != current)
      SignalSemaphore(&lru_sem);
}

void RunAsynchIOQueue(ULONG disk)
{
    register int i = (disk % 8);

    if (!asynch_threads)
    {
       asynch_active[i] = TRUE;
       process_sync_io(i);
       asynch_active[i] = 0;
       asynch_signal[i] = 0;
    }

#if (DO_ASYNCH_IO)
    if (asynch_io_task[i] != current)
    {
       if (!asynch_signal[i])
       {
	  if (!asynch_active[i])
	  {
	     asynch_signal[i] = TRUE;
             SignalSemaphore(sem_table[i]);
	  }
       }
    }
#endif
}

int nwfs_mirror(void *notused)
{
    register ULONG ccode;
#if (DO_ASYNCH_IO)
    register int i;
#endif
    
    sprintf(current->comm, "nwfs-remirror");
    daemonize();
    remirror_task = current;

    remirror_active++;
    while (!exit_remirror)
    {
       ccode = down_interruptible(&mirror_sem);
       if (ccode == -EINTR)
       {
  	  NWFSPrint("Shutting Down REMIRRORING Process ... ");
	  exit_remirror = TRUE;

	  nwvp_quiese_remirroring();
	  while (nwvp_remirror_poll() == 0);

	  NWFSPrint("done\n");

	  NWFSPrint("Shutting Down LRU Timer ... ");
	  del_timer(&flush_timer);
	  NWFSPrint("done\n");

	  NWFSPrint("Shutting Down LRU Process ... ");
	  exit_flush = TRUE;
	  while (flush_active)
	  {
	     NWFSFlush();
#if (LINUX_SLEEP)
	     schedule();
#endif
	  }
	  NWFSPrint("done\n");

#if (DO_ASYNCH_IO)
	  NWFSPrint("Shutting Down ASYNCH_CALLBACK Process ... ");
	  exit_callback = TRUE;
	  while (callback_active)
	  {
             SignalSemaphore(&callback_semaphore);
#if (LINUX_SLEEP)
	     schedule();
#endif
	  }
	  NWFSPrint("done\n");


	  NWFSPrint("Shutting Down ASYNCH_IO Processes [ ");
          for (i=0; i < 8; i++)
          {
	     exit_asynch_io[i] = TRUE;
	     while (asynch_io_active[i])
	     {
	        RunAsynchIOQueue(i);
#if (LINUX_SLEEP)
	        schedule();
#endif
	     }
             NWFSPrint("%d ", (int)i);
          }
          NWFSPrint("]\n");
#endif
	  break;
       }

       if (exit_remirror)
       {
	   nwvp_quiese_remirroring();
           while (nwvp_remirror_poll() == 0);
	   break;
       }

       while (nwvp_remirror_poll() == 0)
       {
	  if (exit_remirror)
	  {
	     nwvp_quiese_remirroring();
	     while (nwvp_remirror_poll() == 0);
	     break;
	  }
       }
    }
    remirror_active--;
    return 0;

}

int nwfs_flush(void *notused)
{
    sprintf(current->comm, "nwfs-flush");
    daemonize();
    lru_task = current;

    flush_active++;
    while (!exit_flush)
    {
       down(&lru_sem);

       if (exit_flush)
	  break;

       if (lru_signal)
       {
	  if (!lru_active)
	     FlushEligibleLRU();
	  lru_signal = 0;
       }
    }
    flush_active--;
    return 0;

}

int nwfs_asynch_io_process(void *id)
{
    register int i = (int)id;

    sprintf(current->comm, "nwfs-async%d", (int)i);
    daemonize();
    asynch_io_task[i] = current;

    asynch_io_active[i]++;
    asynch_threads++;
    while (!exit_asynch_io[i])
    {
       if (!asynch_io_pending(i))
          down(sem_table[i]);

       if (exit_asynch_io[i])
	  break;

#if (DO_ASYNCH_IO)
       if (asynch_signal[i])
       {
          if (!asynch_active[i])
          {
             asynch_active[i] = TRUE;
             process_asynch_io(i);
             asynch_active[i] = 0;
          }
          asynch_signal[i] = 0;
       }
#endif
    }
    asynch_threads--;
    asynch_io_active[i]--;
    return 0;

}

void LRUTimer(void)
{
    flush_timer.data = 0;
    flush_timer.expires = jiffies + HZ; // second delay
    flush_timer.function = (void (*)(ULONG))LRUTimer;
    add_timer(&flush_timer);

#if (PROFILE_AIO)
    seconds++;
#endif

    if (lru_task != current)
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

#if (DEBUG_LRU_AIO)
    if (delay_counter++ > 60)
    {
       delay_counter = 0;
       displayNWFSState();
    }
#endif

    return;
}

struct super_operations nwfs_sops =
{
    read_inode:     nwfs_read_inode,    // read inode
    write_inode:    nwfs_write_inode,   // write inode
    put_inode:      nwfs_put_inode,     // put inode
    delete_inode:   nwfs_delete_inode,  // delete inode
    put_super:      nwfs_put_super,     // put superblock
    statfs:         nwfs_statfs,	// stat filesystem
    remount_fs:     nwfs_remount,       // remount filesystem
};

DECLARE_FSTYPE(nwfs_fs_type, "nwfs", nwfs_read_super, 0);

int init_module(void)
{
    register int status;
#if (DO_ASYNCH_IO)
    register int i;
#endif
    
    NWFSPrint("NetWare File System NWFS v%d.%02d.%02d Copyright(c) %d Jeff V. Merkey\n",
	      (int)MAJOR_VERSION, (int)MINOR_VERSION, (int)BUILD_VERSION,
              (int)BUILD_YEAR);

    if ((status = register_filesystem(&nwfs_fs_type)) == 0)
       NWFSPrint("nwfs:  initialized successfully\n");

    InitNWFS();

    remirror_active = 0;
    flush_active = 0;

    exit_remirror = 0;
    exit_flush = 0;

    NWFSPrint("starting REMIRROR process ...\n");
    kernel_thread(nwfs_mirror, NULL, 0);

    NWFSPrint("starting LRU process ...\n");
    kernel_thread(nwfs_flush, NULL, 0);

#if (DO_ASYNCH_IO)
    NWFSPrint("starting ASYNCH_CALLBACK process ...\n");
    kernel_thread(nwfs_callback, NULL, 0);

    NWFSPrint("starting ASYNCH_IO processes [ ");
    for (i=0; i < 8; i++)
    {
       kernel_thread(nwfs_asynch_io_process, (void *)i, 0);
       NWFSPrint("%d ", (int)i);
    }
    NWFSPrint("]\n");
#endif
    
    NWFSPrint("starting LRU Timer ...\n");
    init_timer(&flush_timer);
    flush_timer.data = 0;
    flush_timer.expires = jiffies + HZ; // second delay
    flush_timer.function = (void (*)(ULONG))LRUTimer;
    add_timer(&flush_timer);

    NWFSVolumeScan();
    ReportVolumes();

    return status;
}

void cleanup_module(void)
{
    register int err;
#if (DO_ASYNCH_IO)
    register int i;
#endif
    extern void displayMemoryList(void);

    err = unregister_filesystem(&nwfs_fs_type);
    if (err)
    {
       NWFSPrint("nwfs:  filesystem in use.  unload failed\n");
       return;
    }

    DismountAllVolumes();

    NWFSPrint("Shutting Down LRU Timer ... ");
    del_timer(&flush_timer);
    NWFSPrint("done\n");

    NWFSPrint("Shutting Down REMIRRORING Process ... ");
    exit_remirror = TRUE;
    while (remirror_active)
    {
       NWFSStartRemirror();
#if (LINUX_SLEEP)
       schedule();
#endif
    }
    NWFSPrint("done\n");

    NWFSPrint("Shutting Down LRU Process ... ");
    exit_flush = TRUE;
    while (flush_active)
    {
       NWFSFlush();
#if (LINUX_SLEEP)
       schedule();
#endif
    }
    NWFSPrint("done\n");

#if (DO_ASYNCH_IO)
    NWFSPrint("Shutting Down ASYNCH_CALLBACK Process ... ");
    exit_callback = TRUE;
    while (callback_active)
    {
       SignalSemaphore(&callback_semaphore);
#if (LINUX_SLEEP)
       schedule();
#endif
    }
    NWFSPrint("done\n");

    NWFSPrint("Shutting Down ASYNCH_IO Processes [ ");
    for (i=0; i < 8; i++)
    {
       exit_asynch_io[i] = TRUE;
       while (asynch_io_active[i])
       {
	  RunAsynchIOQueue(i);
#if (LINUX_SLEEP)
	  schedule();
#endif
       }
       NWFSPrint("%d ", (int)i);
    }
    NWFSPrint("]\n");
#endif
    

    NWFSVolumeClose();
    RemoveDiskDevices();
    FreePartitionResources();

}

#endif


#if (LINUX_22)

#if (PROFILE_AIO)
extern ULONG seconds;
#endif

struct task_struct *lru_task;
extern ULONG lru_active;
extern ULONG lru_state;
extern ULONG lru_signal;
NWFSInitSemaphore(lru_sem);

ULONG callback_active = 0;
ULONG exit_callback = 0;
struct task_struct *callback_task = 0;
extern struct semaphore callback_semaphore;
extern void process_callback(void);

int nwfs_callback(void *notused)
{
    current->session = 1;
    current->pgrp = 1;

    sprintf(current->comm, "nwfs-callback");
    callback_task = current;

    callback_active++;
    while (!exit_callback)
    {
       down(&callback_semaphore);

       if (exit_callback)
	  break;

       process_callback();
    }
    callback_active--;
    return 0;
}

ULONG remirror_active;
ULONG flush_active;
ULONG asynch_io_active[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

ULONG exit_remirror;
ULONG exit_flush;
ULONG exit_asynch_io[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG asynch_signal[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG asynch_active[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG asynch_threads = 0;

NWFSInitSemaphore(mirror_sem);
NWFSInitSemaphore(asynch_io_sem0);
NWFSInitSemaphore(asynch_io_sem1);
NWFSInitSemaphore(asynch_io_sem2);
NWFSInitSemaphore(asynch_io_sem3);
NWFSInitSemaphore(asynch_io_sem4);
NWFSInitSemaphore(asynch_io_sem5);
NWFSInitSemaphore(asynch_io_sem6);
NWFSInitSemaphore(asynch_io_sem7);

struct semaphore *sem_table[8]={
   &asynch_io_sem0, &asynch_io_sem1, &asynch_io_sem2, &asynch_io_sem3,
   &asynch_io_sem4, &asynch_io_sem5, &asynch_io_sem6, &asynch_io_sem7
};

struct task_struct *remirror_task = 0;
struct task_struct *asynch_io_task[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
struct timer_list flush_timer = { NULL, NULL, 0, 0, 0 };

extern ULONG nwvp_remirror_poll(void);
extern void nwvp_quiese_remirroring(void);
extern void process_asynch_io(ULONG disk);
extern void process_sync_io(ULONG disk);
extern ULONG asynch_io_pending(ULONG disk);

int count = 0;

void NWFSStartRemirror(void)
{
   if (remirror_task != current)
      SignalSemaphore(&mirror_sem);
}

void NWFSFlush(void)
{
   if (lru_task != current)
      SignalSemaphore(&lru_sem);
}

void RunAsynchIOQueue(ULONG disk)
{
    register int i = (disk % 8);

    if (!asynch_threads)
    {
       asynch_active[i] = TRUE;
       process_sync_io(i);
       asynch_active[i] = 0;
       asynch_signal[i] = 0;
    }

#if (DO_ASYNCH_IO)
    if (asynch_io_task[i] != current)
    {
       if (!asynch_signal[i])
       {
	  if (!asynch_active[i])
	  {
	     asynch_signal[i] = TRUE;
             SignalSemaphore(sem_table[i]);
	  }
       }
    }
#endif

}

int nwfs_mirror(void *notused)
{
    register ULONG ccode;
#if (DO_ASYNCH_IO)
    register int i;
#endif

    current->session = 1;
    current->pgrp = 1;

    sprintf(current->comm, "nwfs-remirror");
    remirror_task = current;

    remirror_active++;
    while (!exit_remirror)
    {
       ccode = down_interruptible(&mirror_sem);
       if (ccode == -EINTR)
       {
	  NWFSPrint("Shutting Down REMIRRORING Process ... ");
	  exit_remirror = TRUE;
	  nwvp_quiese_remirroring();
	  while (nwvp_remirror_poll() == 0);
	  NWFSPrint("done\n");

	  NWFSPrint("Shutting Down LRU Timer ... ");
	  del_timer(&flush_timer);
	  NWFSPrint("done\n");

	  NWFSPrint("Shutting Down LRU Process ... ");
	  exit_flush = TRUE;
	  while (flush_active)
	  {
	     NWFSFlush();
#if (LINUX_SLEEP)
	     schedule();
#endif
	  }
	  NWFSPrint("done\n");

#if (DO_ASYNCH_IO)
	  NWFSPrint("Shutting Down ASYNCH_CALLBACK Process ... ");
	  exit_callback = TRUE;
	  while (callback_active)
	  {
             SignalSemaphore(&callback_semaphore);
#if (LINUX_SLEEP)
	     schedule();
#endif
	  }
	  NWFSPrint("done\n");

	  NWFSPrint("Shutting Down ASYNCH_IO Processes [ ");
          for (i=0; i < 8; i++)
          {
	     exit_asynch_io[i] = TRUE;
	     while (asynch_io_active[i])
	     {
	        RunAsynchIOQueue(i);
#if (LINUX_SLEEP)
	        schedule();
#endif
	     }
             NWFSPrint("%d ", (int)i);
          }
          NWFSPrint("]\n");
#endif
	  break;
       }

       if (exit_remirror)
       {
	   nwvp_quiese_remirroring();
	   while (nwvp_remirror_poll() == 0);
	   break;
       }

       while (nwvp_remirror_poll() == 0)
       {
	  if (exit_remirror)
	  {
	     nwvp_quiese_remirroring();
	     while (nwvp_remirror_poll() == 0);
	     break;
	  }
       }
    }
    remirror_active--;
    return 0;

}

int nwfs_flush(void *notused)
{
    current->session = 1;
    current->pgrp = 1;

    sprintf(current->comm, "nwfs-flush");
    lru_task = current;

    flush_active++;
    while (!exit_flush)
    {
       down(&lru_sem);

       if (exit_flush)
	  break;

       if (lru_signal)
       {
	  if (!lru_active)
	     FlushEligibleLRU();
	  lru_signal = 0;
       }
    }
    flush_active--;
    return 0;

}

int nwfs_asynch_io_process(void *id)
{
    register int i = (int)id;

    current->session = 1;
    current->pgrp = 1;

    sprintf(current->comm, "nwfs-async%d", (int)i);
    asynch_io_task[i] = current;

    asynch_io_active[i]++;
    asynch_threads++;
    while (!exit_asynch_io[i])
    {
       if (!asynch_io_pending(i))
          down(sem_table[i]);

       if (exit_asynch_io[i])
	  break;

#if (DO_ASYNCH_IO)
       if (asynch_signal[i])
       {
          if (!asynch_active[i])
          {
             asynch_active[i] = TRUE;
             process_asynch_io(i);
             asynch_active[i] = 0;
          }
          asynch_signal[i] = 0;
       }
#endif

    }
    asynch_threads--;
    asynch_io_active[i]--;
    return 0;

}


void LRUTimer(void)
{
    flush_timer.data = 0;
    flush_timer.expires = jiffies + HZ; // second delay
    flush_timer.function = (void (*)(ULONG))LRUTimer;
    add_timer(&flush_timer);

#if (PROFILE_AIO)
    seconds++;
#endif

    if (lru_task != current)
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

#if (DEBUG_LRU_AIO)
    if (delay_counter++ > 60)
    {
       delay_counter = 0;
       displayNWFSState();
    }
#endif
    
    return;
}

struct super_operations nwfs_sops =
{
    nwfs_read_inode,        // read inode
    nwfs_write_inode,	    // write inode
    nwfs_put_inode,	    // put inode
    nwfs_delete_inode,      // delete inode
    nwfs_notify_change,     // notify change
    nwfs_put_super,	    // put superblock
    NULL,		    // write superblock
    nwfs_statfs,	    // stat filesystem
    nwfs_remount,           // remount_fs
};

struct file_system_type nwfs_fs_type =
{
    "nwfs",
    0,
    nwfs_read_super,
    NULL
};

int init_module(void)
{
    register int status;
#if (DO_ASYNCH_IO)
    register int i;
#endif

    NWFSPrint("NetWare File System NWFS v%d.%02d.%02d Copyright(c) %d TRG, Inc.\n",
	      (int)MAJOR_VERSION, (int)MINOR_VERSION, (int)BUILD_VERSION,
              (int)BUILD_YEAR);

    if ((status = register_filesystem(&nwfs_fs_type)) == 0)
       NWFSPrint("nwfs:  initialized successfully\n");

    InitNWFS();

    remirror_active = 0;
    flush_active = 0;

    exit_remirror = 0;
    exit_flush = 0;

    NWFSPrint("starting REMIRROR process ...\n");
    kernel_thread(nwfs_mirror, NULL, 0);

    NWFSPrint("starting LRU process ...\n");
    kernel_thread(nwfs_flush, NULL, 0);

#if (DO_ASYNCH_IO)
    NWFSPrint("starting ASYNCH_CALLBACK process ...\n");
    kernel_thread(nwfs_callback, NULL, 0);

    NWFSPrint("starting ASYNCH_IO processes [ ");
    for (i=0; i < 8; i++)
    {
       kernel_thread(nwfs_asynch_io_process, (void *)i, 0);
       NWFSPrint("%d ", (int)i);
    }
    NWFSPrint("]\n");
#endif
    
    NWFSPrint("starting LRU Timer ...\n");
    init_timer(&flush_timer);
    flush_timer.data = 0;
    flush_timer.expires = jiffies + HZ; // second delay
    flush_timer.function = (void (*)(ULONG))LRUTimer;
    add_timer(&flush_timer);

    NWFSVolumeScan();
    ReportVolumes();

    return status;
}

void cleanup_module(void)
{
    register int err;
#if (DO_ASYNCH_IO)
    register int i;
#endif
    extern void displayMemoryList(void);

    err = unregister_filesystem(&nwfs_fs_type);
    if (err)
    {
       NWFSPrint("nwfs:  filesystem in use.  unload failed\n");
       return;
    }

    DismountAllVolumes();

    NWFSPrint("Shutting Down LRU Timer ... ");
    del_timer(&flush_timer);
    NWFSPrint("done\n");

    NWFSPrint("Shutting Down REMIRRORING Process ... ");
    exit_remirror = TRUE;
    while (remirror_active)
    {
       NWFSStartRemirror();
#if (LINUX_SLEEP)
       schedule();
#endif
    }
    NWFSPrint("done\n");

    NWFSPrint("Shutting Down LRU Process ... ");
    exit_flush = TRUE;
    while (flush_active)
    {
       NWFSFlush();
#if (LINUX_SLEEP)
       schedule();
#endif
    }
    NWFSPrint("done\n");

#if (DO_ASYNCH_IO)
    NWFSPrint("Shutting Down ASYNCH_CALLBACK Process ... ");
    exit_callback = TRUE;
    while (callback_active)
    {
       SignalSemaphore(&callback_semaphore);
#if (LINUX_SLEEP)
       schedule();
#endif
    }
    NWFSPrint("done\n");

    NWFSPrint("Shutting Down ASYNCH_IO Processes [ ");
    for (i=0; i < 8; i++)
    {
       exit_asynch_io[i] = TRUE;
       while (asynch_io_active[i])
       {
	  RunAsynchIOQueue(i);
#if (LINUX_SLEEP)
	  schedule();
#endif
       }
       NWFSPrint("%d ", (int)i);
    }
    NWFSPrint("]\n");
#endif
    
    NWFSVolumeClose();
    RemoveDiskDevices();
    FreePartitionResources();

}

#endif


#if (LINUX_20)

#if (PROFILE_AIO)
extern ULONG seconds;
#endif

struct task_struct *lru_task;
extern ULONG lru_active;
extern ULONG lru_state;
extern ULONG lru_signal;
NWFSInitSemaphore(lru_sem);

ULONG callback_active = 0;
ULONG exit_callback = 0;
struct task_struct *callback_task = 0;
extern struct semaphore callback_semaphore;
extern void process_callback(void);

int nwfs_callback(void *notused)
{
    current->session = 1;
    current->pgrp = 1;

    sprintf(current->comm, "nwfs-callback");
    callback_task = current;

    callback_active++;
    while (!exit_callback)
    {
       down(&callback_semaphore);

       if (exit_callback)
	  break;

       process_callback();
    }
    callback_active--;
    return 0;
}

ULONG remirror_active;
ULONG flush_active;
ULONG asynch_io_active[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

ULONG exit_remirror;
ULONG exit_flush;
ULONG exit_asynch_io[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG asynch_signal[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG asynch_active[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG asynch_threads = 0;

NWFSInitSemaphore(mirror_sem);
NWFSInitSemaphore(asynch_io_sem0);
NWFSInitSemaphore(asynch_io_sem1);
NWFSInitSemaphore(asynch_io_sem2);
NWFSInitSemaphore(asynch_io_sem3);
NWFSInitSemaphore(asynch_io_sem4);
NWFSInitSemaphore(asynch_io_sem5);
NWFSInitSemaphore(asynch_io_sem6);
NWFSInitSemaphore(asynch_io_sem7);

struct semaphore *sem_table[8]={
   &asynch_io_sem0, &asynch_io_sem1, &asynch_io_sem2, &asynch_io_sem3,
   &asynch_io_sem4, &asynch_io_sem5, &asynch_io_sem6, &asynch_io_sem7
};

struct task_struct *remirror_task = 0;
struct task_struct *asynch_io_task[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
struct timer_list flush_timer = { NULL, NULL, 0, 0, 0 };

extern ULONG nwvp_remirror_poll(void);
extern void nwvp_quiese_remirroring(void);
extern void process_asynch_io(ULONG disk);
extern void process_sync_io(ULONG disk);
extern ULONG asynch_io_pending(ULONG disk);

int count = 0;


void NWFSStartRemirror(void)
{
   if (remirror_task != current)
      SignalSemaphore(&mirror_sem);
}

void NWFSFlush(void)
{
   if (lru_task != current)
      SignalSemaphore(&lru_sem);
}

void RunAsynchIOQueue(ULONG disk)
{
    register int i = (disk % 8);

    if (!asynch_threads)
    {
       asynch_active[i] = TRUE;
       process_sync_io(i);
       asynch_active[i] = 0;
       asynch_signal[i] = 0;
    }

#if (DO_ASYNCH_IO)
    if (asynch_io_task[i] != current)
    {
       if (!asynch_signal[i])
       {
	  if (!asynch_active[i])
	  {
	     asynch_signal[i] = TRUE;
             SignalSemaphore(sem_table[i]);
	  }
       }
    }
#endif

}

int nwfs_mirror(void *notused)
{
    register ULONG ccode;
#if (DO_ASYNCH_IO)
    register int i;
#endif
    
    current->session = 1;
    current->pgrp = 1;

    sprintf(current->comm, "nwfs-remirror");
    remirror_task = current;

    remirror_active++;
    while (!exit_remirror)
    {
       ccode = down_interruptible(&mirror_sem);
       if (ccode == -EINTR)
       {
	  NWFSPrint("Shutting Down REMIRRORING Process ... ");
	  exit_remirror = TRUE;
	  nwvp_quiese_remirroring();
	  while (nwvp_remirror_poll() == 0);
	  NWFSPrint("done\n");

	  NWFSPrint("Shutting Down LRU Timer ... ");
	  del_timer(&flush_timer);
	  NWFSPrint("done\n");

	  NWFSPrint("Shutting Down LRU Process ... ");
	  exit_flush = TRUE;
	  while (flush_active)
	  {
	     NWFSFlush();
#if (LINUX_SLEEP)
	     schedule();
#endif
	  }
	  NWFSPrint("done\n");

#if (DO_ASYNCH_IO)
	  NWFSPrint("Shutting Down ASYNCH_CALLBACK Process ... ");
	  exit_callback = TRUE;
	  while (callback_active)
	  {
             SignalSemaphore(&callback_semaphore);
#if (LINUX_SLEEP)
	     schedule();
#endif
	  }
	  NWFSPrint("done\n");

	  NWFSPrint("Shutting Down ASYNCH_IO Processes [ ");
          for (i=0; i < 8; i++)
          {
	     exit_asynch_io[i] = TRUE;
	     while (asynch_io_active[i])
	     {
	        RunAsynchIOQueue(i);
#if (LINUX_SLEEP)
	        schedule();
#endif
	     }
             NWFSPrint("%d ", (int)i);
          }
          NWFSPrint("]\n");
#endif
	  break;
       }

       if (exit_remirror)
       {
	   nwvp_quiese_remirroring();
	   while (nwvp_remirror_poll() == 0);
	   break;
       }

       while (nwvp_remirror_poll() == 0)
       {
	  if (exit_remirror)
	  {
	     nwvp_quiese_remirroring();
	     while (nwvp_remirror_poll() == 0);
	     break;
	  }
       }
    }
    remirror_active--;
    return 0;

}

int nwfs_flush(void *notused)
{
    current->session = 1;
    current->pgrp = 1;

    sprintf(current->comm, "nwfs-flush");
    lru_task = current;

    flush_active++;
    while (!exit_flush)
    {
       down(&lru_sem);

       if (exit_flush)
	  break;

       if (lru_signal)
       {
	  if (!lru_active)
	     FlushEligibleLRU();
	  lru_signal = 0;
       }
    }
    flush_active--;
    return 0;

}

int nwfs_asynch_io_process(void *id)
{
    register int i = (int)id;

    current->session = 1;
    current->pgrp = 1;

    sprintf(current->comm, "nwfs-async%d", (int)i);
    asynch_io_task[i] = current;

    asynch_io_active[i]++;
    asynch_threads++;
    while (!exit_asynch_io[i])
    {
       if (!asynch_io_pending(i))
          down(sem_table[i]);

       if (exit_asynch_io[i])
	  break;

#if (DO_ASYNCH_IO)
       if (asynch_signal[i])
       {
          if (!asynch_active[i])
          {
             asynch_active[i] = TRUE;
             process_asynch_io(i);
             asynch_active[i] = 0;
          }
          asynch_signal[i] = 0;
       }
#endif

    }
    asynch_threads--;
    asynch_io_active[i]--;
    return 0;

}

void LRUTimer(void)
{
    flush_timer.data = 0;
    flush_timer.expires = jiffies + HZ; // second delay
    flush_timer.function = (void (*)(ULONG))LRUTimer;
    add_timer(&flush_timer);

#if (PROFILE_AIO)
    seconds++;
#endif

    if (lru_task != current)
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

#if (DEBUG_LRU_AIO)
    if (delay_counter++ > 60)
    {
       delay_counter = 0;
       displayNWFSState();
    }
#endif

    return;
}

struct file_system_type nwfs_type =
{
    nwfs_read_super, "nwfs", 0, NULL
};

struct super_operations nwfs_sops =
{
    nwfs_read_inode,        // read inode
    NULL,                   // notify change
    nwfs_write_inode,	    // write inode
    nwfs_put_inode,	    // put inode
    nwfs_put_super,	    // put superblock
    NULL,		    // write superblock
    nwfs_statfs,	    // stat filesystem
    nwfs_remount,           // remount filesystem
};

int init_module(void)
{
    register int status;
#if (DO_ASYNCH_IO)
    register int i;
#endif
    extern int nwfs_register_symbols(void);

    NWFSPrint("NetWare File System NWFS v%d.%02d.%02d Copyright(c) %d TRG, Inc.\n",
	      (int)MAJOR_VERSION, (int)MINOR_VERSION, (int)BUILD_VERSION,
              (int)BUILD_YEAR);

    if ((status = register_filesystem(&nwfs_type)) == 0)
    {
       NWFSPrint("nwfs:  initialized successfully\n");
       nwfs_register_symbols();
    }

    InitNWFS();

    remirror_active = 0;
    flush_active = 0;

    exit_remirror = 0;
    exit_flush = 0;

    NWFSPrint("starting REMIRROR process ...\n");
    kernel_thread(nwfs_mirror, NULL, 0);

    NWFSPrint("starting LRU process ...\n");
    kernel_thread(nwfs_flush, NULL, 0);

#if (DO_ASYNCH_IO)
    NWFSPrint("starting ASYNCH_CALLBACK process ...\n");
    kernel_thread(nwfs_callback, NULL, 0);

    NWFSPrint("starting ASYNCH_IO processes [ ");
    for (i=0; i < 8; i++)
    {
       kernel_thread(nwfs_asynch_io_process, (void *)i, 0);
       NWFSPrint("%d ", (int)i);
    }
    NWFSPrint("]\n");
#endif
    
    NWFSPrint("starting LRU Timer ...\n");
    init_timer(&flush_timer);
    flush_timer.data = 0;
    flush_timer.expires = jiffies + HZ; // second delay
    flush_timer.function = (void (*)(ULONG))LRUTimer;
    add_timer(&flush_timer);

    NWFSVolumeScan();
    ReportVolumes();

    return status;
}

void cleanup_module(void)
{
    register int err;
#if (DO_ASYNCH_IO)
    register int i;
#endif
    extern void displayMemoryList(void);

    err = unregister_filesystem(&nwfs_type);
    if (err)
    {
       NWFSPrint("nwfs:  filesystem in use.  unload failed\n");
       return;
    }

    DismountAllVolumes();

    NWFSPrint("Shutting Down LRU Timer ... ");
    del_timer(&flush_timer);
    NWFSPrint("done\n");

    NWFSPrint("Shutting Down REMIRRORING Process ... ");
    exit_remirror = TRUE;
    while (remirror_active)
    {
       NWFSStartRemirror();
#if (LINUX_SLEEP)
       schedule();
#endif
    }
    NWFSPrint("done\n");

    NWFSPrint("Shutting Down LRU Process ... ");
    exit_flush = TRUE;
    while (flush_active)
    {
       NWFSFlush();
#if (LINUX_SLEEP)
       schedule();
#endif
    }
    NWFSPrint("done\n");

#if (DO_ASYNCH_IO)
    NWFSPrint("Shutting Down ASYNCH_CALLBACK Process ... ");
    exit_callback = TRUE;
    while (callback_active)
    {
       SignalSemaphore(&callback_semaphore);
#if (LINUX_SLEEP)
       schedule();
#endif
    }
    NWFSPrint("done\n");

    NWFSPrint("Shutting Down ASYNCH_IO Processes [ ");
    for (i=0; i < 8; i++)
    {
       exit_asynch_io[i] = TRUE;
       while (asynch_io_active[i])
       {
	  RunAsynchIOQueue(i);
#if (LINUX_SLEEP)
	  schedule();
#endif
       }
       NWFSPrint("%d ", (int)i);
    }
    NWFSPrint("]\n");
#endif
    
    NWFSVolumeClose();
    RemoveDiskDevices();
    FreePartitionResources();

}

#endif

