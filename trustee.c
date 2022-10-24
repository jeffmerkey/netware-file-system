
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
*   FILE     :  TRUSTEE.C
*   DESCRIP  :   Trustee Hashing Functions
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#include "globals.h"

THASH *GetTrusteeEntry(VOLUME *volume, ULONG entry, ULONG Directory)
{
    register THASH *tname;
    register THASH_LIST *HashTable;
    register ULONG Value, pos;

    Value = (Directory & (volume->TrusteeHashLimit - 1));
    HashTable = (THASH_LIST *) volume->TrusteeHash;
    if (!HashTable)
       return 0;

    tname = (THASH *) HashTable[Value].head;
    pos = 0;
    while (tname)
    {
       if (tname->Parent == Directory)
       {
	  if (entry == pos)
	     return tname;
	  pos++;
       }
       tname = tname->next;
    }
    return 0;

}

UHASH *GetUserQuotaEntry(VOLUME *volume, ULONG entry, ULONG Directory)
{
    register UHASH *uname;
    register UHASH_LIST *HashTable;
    register ULONG Value, pos;

    Value = (Directory & (volume->UserQuotaHashLimit - 1));
    HashTable = (UHASH_LIST *) volume->UserQuotaHash;
    if (!HashTable)
       return 0;

    uname = (UHASH *) HashTable[Value].head;
    pos = 0;
    while (uname)
    {
       if (uname->Parent == Directory)
       {
	  if (entry == pos)
	     return uname;
	  pos++;
       }
       uname = uname->next;
    }
    return 0;

}
