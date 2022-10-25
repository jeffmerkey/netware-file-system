
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
*   FILE     :  RAPIDFAT.C
*   DESCRIP  :   Rapid FAT Module
*   DATE     :  July 11, 2022
*
*
***************************************************************************/

#include "globals.h"

void NWLockFCB(FCB *fcb)
{
#if (LINUX)
    if (WaitOnSemaphore(&fcb->Semaphore) == -EINTR)
       NWFSPrint("lock fcb was interrupted\n");
#endif
}

void NWUnlockFCB(FCB *fcb)
{
#if (LINUX)
    SignalSemaphore(&fcb->Semaphore);
#endif
}

