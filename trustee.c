
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
*   FILE     :  TRUSTEE.C
*   DESCRIP  :  Trustee Hashing Functions
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
