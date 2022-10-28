

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
*   FILE     :  RWLOCK.C
*   DESCRIP  :  Reader Writer Locks
*   DATE     :  April 2, 2000
*
*
***************************************************************************/

#include "globals.h"


//
//  SMP Reader Writer Locks for the File FAT List Heads.
//

void NWLockRW(HASH *hash)
{
#if (LINUX_SLEEP)
   if (WaitOnSemaphore((struct semaphore *)&hash->RWSemaphore) == -EINTR)
       NWFSPrint("LockRW (%s) was interrupted\n", hash->Name);
#endif
}

void NWUnlockRW(HASH *hash)
{
#if (LINUX_SLEEP)
    SignalSemaphore(&hash->RWSemaphore);
#endif
}

ULONG SleepOnReadersQueue(HASH *hash)
{
    hash->readers_pending++;
    NWUnlockRW(hash);

#if (LINUX_SLEEP)
    WaitOnSemaphore(&hash->ReadersSemaphore);
#endif

    NWLockRW(hash);
    // handle is invalid
    if (hash->Signature != (ULONG)"HASH")
       return -1;
    return 0;
}

ULONG SleepOnWritersQueue(HASH *hash)
{
    hash->writers_pending++;
    NWUnlockRW(hash);

#if (LINUX_SLEEP)
    WaitOnSemaphore(&hash->WritersSemaphore);
#endif

    NWLockRW(hash);
    // handle is invalid
    if (hash->Signature != (ULONG)"HASH")
       return -1;
    return 0;
}

void ReleaseNextReader(HASH *hash)
{
   if (hash->readers_pending)
   {
      hash->readers_pending--;
#if (LINUX_SLEEP)
      SignalSemaphore(&hash->ReadersSemaphore);
#endif
   }
}

void ReleaseNextWriter(HASH *hash)
{
   if (hash->writers_pending)
   {
      hash->writers_pending--;
#if (LINUX_SLEEP)
      SignalSemaphore(&hash->WritersSemaphore);
#endif
   }
}

void ReleaseReaders(HASH *hash)
{
   while (hash->readers_pending)
   {
      hash->readers_pending--;
#if (LINUX_SLEEP)
      SignalSemaphore(&hash->ReadersSemaphore);
#endif
   }
}

void ReleaseWriters(HASH *hash)
{
   while (hash->writers_pending)
   {
      hash->writers_pending--;
#if (LINUX_SLEEP)
      SignalSemaphore(&hash->WritersSemaphore);
#endif
   }
}

ULONG NWReaderWriterLock(HASH *hash, ULONG which)
{
    register ULONG ccode;

    which = which % WRITER;
    NWLockRW(hash);
    switch (hash->rw_state)
    {
       case READER_OWNER:
          switch (which)
          {
             case READER:
                if (hash->writers_pending)
                {
                   ccode = SleepOnReadersQueue(hash);
                   hash->readers++;
                   NWUnlockRW(hash);
                   return ccode;
                }
                hash->readers++;
                NWUnlockRW(hash);
                return 0;

             case WRITER:
                if (hash->readers)
                {
                   ccode = SleepOnWritersQueue(hash);
                   hash->writers++;
                   hash->writers_count++;
                   NWUnlockRW(hash);
                   return ccode;
                }
                hash->rw_state = WRITER_OWNER;
                hash->writers++;
                hash->writers_count++;
                NWUnlockRW(hash);
                return 0;

             default:
                return -2;
          }
          break;

       case WRITER_OWNER:
          switch (which)
          {
             case READER:
                ccode = SleepOnReadersQueue(hash);
                hash->readers++;
                NWUnlockRW(hash);
                return ccode;

             case WRITER:
                ccode = SleepOnWritersQueue(hash);
                hash->writers++;
                hash->writers_count++;
                NWUnlockRW(hash);
                return ccode;

             default:
                return -2;
          }
          break;

       default:
          NWUnlockRW(hash);
          return -3;   // bad hash structure
    }
    NWUnlockRW(hash);
    return 0;
}

ULONG NWReaderWriterUnlock(HASH *hash, ULONG which)
{
    which = which % WRITER;
    NWLockRW(hash);
    switch (hash->rw_state)
    {
       case READER_OWNER:
          if (which == WRITER)
          {
             NWFSPrint("nwfs:  fatal -- rwlock state mismatch (r-%d/c-%d)\n",
	               (int)which, (int)hash->rw_state);
             return -1;
          }

          if (hash->readers)
             hash->readers--;

          if (!hash->readers)
	  {
	     if (hash->writers_pending)
                ReleaseNextWriter(hash);
          }
          NWUnlockRW(hash);
          return 0;

       case WRITER_OWNER:
          if (which == READER)
          {
             NWFSPrint("nwfs:  fatal -- rwlock state mismatch (r-%d/c-%d)\n",
	               (int)which, (int)hash->rw_state);
             return -1;
          }

          if (hash->writers_pending)
          {
             ReleaseNextWriter(hash);
             NWUnlockRW(hash);
             return 0;
          }

          hash->rw_state = READER_OWNER;
          if (hash->readers_pending)
          {
             ReleaseReaders(hash); // release all the readers.
             NWUnlockRW(hash);
             return 0;
          }
          break;

       default:
          NWUnlockRW(hash);
          return -2;   // bad hash structure
    }
    NWUnlockRW(hash);
    return 0;
}

