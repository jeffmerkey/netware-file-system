
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
*   FILE     :  LOCK.C
*   DESCRIP  :   Atomic Locking Functions
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#include "globals.h"

#if (WINDOWS_NT | WINDOWS_NT_RO)

//
//   returns 0 - lock acquired (was unlocked) or -1 - deadlock
//

ULONG spin_lock(ULONG *value)
{
   register ULONG retCode, counter = 0;

   retCode = InterlockedExchange(value, 1);
   if (!retCode)
      return 0;

   while (1)
   {
      if (!*value)
      {
	 retCode = InterlockedExchange(value, 1);
	 if (!retCode)
	    return 0;
      }
      if (counter++ > 0x4FFFFFF)
      {
	 NWFSPrint("error: deadlock detected in spin_lock\n");
	 return -1;
      }
   }
}

//
// returns 0 on success or -1 on failure
//

ULONG spin_unlock(ULONG *value)
{
   if (*value)
   {
      *value = 0;
      return 0;
   }
   else
   {
      NWFSPrint("error: spin_unlock called with lock open (???)\n");
      return -1;
   }
}

//
//   returns 0 - lock acquired (was unlocked) or 1 - failure
//

ULONG spin_try_lock(ULONG *value)
{
   register ULONG retCode;

   retCode = InterlockedExchange(value, 1);
   return retCode;

}

#endif


#if (LINUX)
#endif



