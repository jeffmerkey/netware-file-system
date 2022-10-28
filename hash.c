
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
*   FILE     :  HASH.C
*   DESCRIP  :  Directory Management
*   DATE     :  November 1, 1998
*
***************************************************************************/

#include "globals.h"

extern HASH *GetDeletedHashFromName(VOLUME *volume, BYTE *PathString, ULONG len,
		             ULONG NameSpace, ULONG Parent);

#define MAX_FREE_HASH_ELEMENTS  50

HASH *HashHead = 0;
HASH *HashTail = 0;
ULONG FreeHashCount = 0;

#if (LINUX_SLEEP)
#if (LINUX_SPIN)
spinlock_t HashFree_spinlock;
ULONG HashFree_flags = 0;
#else
NWFSInitMutex(HashFreeSemaphore);
#endif
#endif

void NWLockHashFreeList(void)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&HashFree_spinlock, HashFree_flags);
#else
    if (WaitOnSemaphore(&HashFreeSemaphore) == -EINTR)
       NWFSPrint("lock hash free list was interrupted\n");
#endif
#endif
}

void NWUnlockHashFreeList(void)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&HashFree_spinlock, HashFree_flags);
#else
    SignalSemaphore(&HashFreeSemaphore);
#endif
#endif
}


void PutHashNode(HASH *hash)
{
    NWLockHashFreeList();

    if (hash->Name)
       NWFSFree(hash->Name);
    hash->Name = 0;
    hash->TurboFATCluster = 0;
    hash->TurboFATIndex = 0;

    if (!HashHead)
    {
       HashHead = HashTail = hash;
       hash->next = hash->prior = 0;
    }
    else
    {
       HashTail->next = hash;
       hash->next = 0;
       hash->prior = HashTail;
       HashTail = hash;
    }
    FreeHashCount++;

#if (CONSERVE_HASH_MEMORY)
    // check if we have exceeded the max free list size, and discard
    // the next free element.
    if (FreeHashCount > MAX_FREE_HASH_ELEMENTS)
    {
       if (HashHead)
       {
	  hash = HashHead;
	  HashHead = hash->next;
	  if (HashHead)
	     HashHead->prior = NULL;
	  else
	     HashTail = NULL;

	  hash->Signature = 0;

	  if (FreeHashCount)
	     FreeHashCount--;

	  NWFSFree(hash);

	  NWUnlockHashFreeList();
	  return;
       }
    }
#endif

    NWUnlockHashFreeList();
    return;
}

HASH *GetHashNode(void)
{
    register HASH *hash;

    NWLockHashFreeList();
    if (HashHead)
    {
       hash = HashHead;
       HashHead = hash->next;
       if (HashHead)
	  HashHead->prior = NULL;
       else
	  HashTail = NULL;

       hash->Signature = (ULONG)"HASH";
       if (!InstanceCounter)
	  InstanceCounter = 1;

       if (FreeHashCount)
	  FreeHashCount--;
       NWUnlockHashFreeList();
       return hash;
    }
    NWUnlockHashFreeList();
    return 0;
}

BYTE *NSDescription(ULONG ns)
{
    switch (ns)
    {
       case 0:
	  return "DOS";

       case 1:
	  return "MAC";

       case 2:
	  return "NFS";

       case 3:
	  return "FTAM";

       case 4:
	  return "LONG";

       case 5:
	  return "NT/NAFS";

       default:
	  return "UNKN";
    }

}

#if (WINDOWS_NT | WINDOWS_NT_RO)

void UpperCaseConvert(POEM_STRING Dest, POEM_STRING Src)
{
    register LONG i;
    extern PUCHAR NwOemUpcaseTable;

    for (i=0; i < Src->Length; i++)
       Dest->Buffer[i] = NwOemUpcaseTable[(BYTE)Src->Buffer[i]];
    Dest->Length = Src->Length;
    return;
}

//
//  this returns the specific namespace match irregardless as to
//  whether it is root entry or not
//  if IgnoreCase is set, assume string is already uppercase
//
//  HASH *HashFindFile(IN ULONG VolumeNumber,
//		       IN ULONG Parent,
//                     IN ULONG StartingSlotNumber,
//		       IN POEM_STRING Name,
//		       IN BOOLEAN IgnoreCase)

HASH *HashFindFile(IN ULONG VolumeNumber,
		   IN ULONG Parent,
		   IN POEM_STRING Name,
		   IN BOOLEAN IgnoreCase)
{
    register VOLUME *volume = VolumeTable[VolumeNumber];
    register HASH *hash = 0;

    if ((Name->Length == 1) && (Name->Buffer[0] == '.'))
    {
       hash = GetHashFromDirectoryNumber(volume, Parent);
       return hash;
    }
    else
    if ((Name->Length == 2) && (Name->Buffer[0] == '.') &&
	(Name->Buffer[1] == '.'))
    {
       hash = GetHashFromDirectoryNumber(volume, Parent);
       if (hash)
	  return GetHashFromDirectoryNumber(volume, hash->Parent);
       return hash;
    }
    else
    {
       if (IgnoreCase)
       {
	  // case insensitive match [hash==Uppercase String==Anycase]
	  // in this case, assume during the search that all
	  // names are searched as uppercase, and we return
	  // the first uppercase match we find in the hash.
	  // ALL names are viewed as uppercase, including target.

	  hash = GetHashFromNameUppercase(volume,
					 (BYTE *)Name->Buffer,
					 Name->Length,
					 volume->NameSpaceDefault,
					 Parent);
	  if (hash)
	     return hash;

	  if ((Name->Length < 13) &&
	     (volume->NameSpaceDefault != DOS_NAME_SPACE))
	  {
	     hash = GetHashFromName(volume,
				    (BYTE *)Name->Buffer,
				    Name->Length,
				    DOS_NAME_SPACE,
				    Parent);
	  }
       }
       else
       {
	  // case sensitive match [hash==Uppercase String==Anycase]
	  // we search based on uppercase then do a mixed case
	  // compare for an exact match of the filename with each
	  // matching entry in the hash.  we return the first
	  // exact match we find

	  hash = GetHashFromName(volume,
			      (BYTE *)Name->Buffer,
			      Name->Length,
			      volume->NameSpaceDefault,
			      Parent);
	  if (hash)
	     return hash;

	  if ((Name->Length < 13) &&
	     (volume->NameSpaceDefault != DOS_NAME_SPACE))
	  {
	     hash = GetHashFromName(volume,
			      (BYTE *)Name->Buffer,
			      Name->Length,
			      DOS_NAME_SPACE,
			      Parent);
	  }
       }
    }
    return hash;
}

HASH *HashFindNext(IN     ULONG VolumeNumber,
		   IN OUT ULONG *SlotNumber,
		   IN     ULONG Parent,
		   IN OUT PVOID *PrevContext,
		   IN     BOOLEAN Current)
{
    register VOLUME *volume = VolumeTable[VolumeNumber];
    register HASH *name, *nlname, *prev;
    register HASH_LIST *HashTable;
    register ULONG Value, pos;

    if (!SlotNumber || !PrevContext)
       return 0;

    // if we are being called in a linear fashion, we store where
    // we are in the hash and the caller is required to return the
    // context during subsequent calls, otherwise we start over
    // from the beginning of the hash collision chain.

    prev = (*PrevContext);
    if (prev)
    {
       if (Current)
       {
	  (*SlotNumber)++;
	  return prev;
       }
       else
       {
	  name = prev->pnext;
	  while (name)
	  {
	     if (name->Parent == Parent)
	     {
		(*SlotNumber)++;
		*PrevContext = name;
		return (HASH *) name;
	     }
	     name = name->pnext;
	  }
	  return 0;
       }
    }
    else
    {
       // else we must perform a brute force search of the
       // hash list

       Value = (Parent & (volume->ParentHashLimit - 1));
       HashTable = (HASH_LIST *) volume->ParentHash;
       if (!HashTable)
	  return 0;

       name = (HASH *) HashTable[Value].head;
       pos = 2;   // pos always starts at 2, '.' and '..' are entries 0/1

       //pos = 0;
       while (name)
       {
	  if ((name->Parent == Parent) && (name->Root != (ULONG) -1))
	  {
	     if (*SlotNumber == pos)
	     {
		(*SlotNumber)++;

		// save current context
		*PrevContext = name;
		return (HASH *) name;
	     }
	     pos++;
	  }
	  name = name->pnext;
       }
    }
    return 0;
}

#endif

//
//  Linux directory shim functions
//

ULONG get_directory_number(VOLUME *volume, const char *name,
			   ULONG len, ULONG Parent)
{
    register HASH *hash = 0;

    if ((len == 1) && (name[0] == '.'))
       return Parent;
    else
    if ((len == 2) && (name[0] == '.') && (name[1] == '.'))
       return get_parent_directory_number(volume, Parent);
    else
    {
       if (volume->DeletedDirNo && (Parent == volume->DeletedDirNo))
          hash = GetDeletedHashFromName(volume,
			               (BYTE *)name,
			               len,
			               volume->NameSpaceDefault,
			               DELETED_DIRECTORY);
       else       
          hash = GetHashFromName(volume,
			      (BYTE *)name,
			      len,
			      volume->NameSpaceDefault,
			      Parent);
    }

    // map to the root namespace DirNo
    if (hash)
       return (ULONG) (hash->Root);
    return 0;
}

HASH *get_directory_hash(VOLUME *volume, const char *name,
			 ULONG len, ULONG Parent)
{
    register HASH *hash;

    if (volume->DeletedDirNo && (Parent == volume->DeletedDirNo))
       hash = GetDeletedHashFromName(volume,
			            (BYTE *)name,
			            len,
			            volume->NameSpaceDefault,
			            DELETED_DIRECTORY);
    else       
       hash = GetHashFromName(volume,
			      (BYTE *)name,
			      len,
			      volume->NameSpaceDefault,
			      Parent);

    // map to the root namespace hash
    if (hash)
       return (HASH *) (hash->nlroot);
    return 0;
}

ULONG get_parent_directory_number(VOLUME *volume, ULONG DirNo)
{
    register HASH *hash;

    hash = GetHashFromDirectoryNumber(volume, DirNo);
    if (hash)
    {
       if (hash->Parent == DELETED_DIRECTORY)
       {
          if (volume->DeletedDirNo)
	     return volume->DeletedDirNo;
	  else
	     return 0;
       }
       return (hash->Parent);
    }
    return 0;
}

HASH *get_directory_record(VOLUME *volume, ULONG DirNo)
{
    if (volume->DeletedDirNo && (DirNo == DELETED_DIRECTORY))
       return (GetHashFromDirectoryNumber(volume, volume->DeletedDirNo));
    else
       return (GetHashFromDirectoryNumber(volume, DirNo));
}

HASH *get_subdirectory_record(VOLUME *volume, ULONG entry, ULONG Parent,
  			      HASH **dir)
{
    if (volume->DeletedDirNo && (Parent == volume->DeletedDirNo))
       return (GetHashNameSpaceEntry(volume, entry, volume->NameSpaceDefault,
				     DELETED_DIRECTORY, dir));
    else
       return (GetHashNameSpaceEntry(volume, entry, volume->NameSpaceDefault,
				     Parent, dir));
}

ULONG get_namespace_dir_record(VOLUME *volume, DOS *dos, HASH *root, 
		               ULONG NameSpace)
{
    register ULONG ccode;
    register HASH *nlhash;

    nlhash = root;
    while (nlhash)
    {
       if (nlhash->NameSpace == NameSpace)
       {
          ccode = ReadDirectoryRecord(volume, dos, nlhash->DirNo);
	  if (ccode)
	     return (ULONG) -1;

	  return (ULONG) nlhash->DirNo;
       }
       nlhash = nlhash->nlnext;
    }
    return (ULONG) -1;
}

ULONG get_namespace_directory_number(VOLUME *volume, HASH *root, 
		                     ULONG NameSpace)
{
    register HASH *nlhash;

    nlhash = root;
    while (nlhash)
    {
       if (nlhash->NameSpace == NameSpace)
	  return (ULONG) nlhash->DirNo;
       nlhash = nlhash->nlnext;
    }
    return (ULONG) -1;
}

ULONG is_deleted(VOLUME *volume, HASH *hash)
{
    if ((hash->Flags & NW4_DELETED_FILE) || (hash->Flags & NW3_DELETED_FILE))
    {
       return 1;
    }
    return 0;
}

ULONG is_deleted_file(VOLUME *volume, HASH *hash)
{
    if ((hash->Flags & NW4_DELETED_FILE) || (hash->Flags & NW3_DELETED_FILE))
    {
       if (!(hash->Flags & SUBDIRECTORY_FILE))
	  return 1;
       else
	  return 0;
    }
    return 0;
}

ULONG is_deleted_dir(VOLUME *volume, HASH *hash)
{
    if ((hash->Flags & NW4_DELETED_FILE) || (hash->Flags & NW3_DELETED_FILE))
    {
       if ((hash->Flags & SUBDIRECTORY_FILE) || (hash->Parent == (ULONG) -1))
	  return 1;
       else
	  return 0;
    }
    return 0;
}


#if (LINUX | LINUX_UTIL)

//
//   basic string hash function, but gives very good distribution
//   model with standard ascii text.
//

ULONG HornerStringHash(BYTE *v, ULONG len, ULONG M)
{
   register ULONG h = 0, a = 127, i;

   if (!M)
   {
      NWFSPrint("nwfs:  horner hash limit error [%s] lim-%u\n",
	     v, (unsigned int)M);
      return -1;
   }

   for (i = 0; i < len && *v; v++, i++)
      h = ((a * h) + *v) % M;

   return h;

}

//
//  very good universal string hash function with a built-in random
//  number genrator, but has more computational overhead.
//

ULONG UniversalStringHash(BYTE *v, ULONG len, ULONG M)
{
   register ULONG h, a = 31415, b = 27163, i;

   if (!M)
   {
      NWFSPrint("nwfs:  universal hash limit error [%s] lim-%u\n",
	      v, (unsigned int)M);
      return -1;
   }

   for (i = 0, h = 0; i < len && *v; i++, v++, a = (a * b) % (M - 1))
      h = ((a * h) + *v) % M;

   return h;

}

#endif


#if (WINDOWS_NT_UTIL | DOS_UTIL)

//
//   basic string hash function, but gives very good distribution
//   model with standard ascii text.
//

ULONG HornerStringHash(BYTE *v, ULONG len, ULONG M)
{
   register ULONG h = 0, a = 127, i;

   if (!M)
   {
      NWFSPrint("nwfs:  horner hash limit error [%s] lim-%u\n",
	     v, (unsigned int)M);
      return -1;
   }

   for (i = 0; i < len && *v; v++, i++)
      h = ((a * h) + *v) % M;

   return h;

}

//
//  very good universal string hash function with a built-in random
//  number genrator, but has more computational overhead.
//

ULONG UniversalStringHash(BYTE *v, ULONG len, ULONG M)
{
   register ULONG h, a = 31415, b = 27163, i;

   if (!M)
   {
      NWFSPrint("nwfs:  universal hash limit error [%s] lim-%u\n",
	      v, (unsigned int)M);
      return -1;
   }

   for (i = 0, h = 0; i < len && *v; i++, v++, a = (a * b) % (M - 1))
      h = ((a * h) + *v) % M;

   return h;

}

#endif




#if (WINDOWS_NT | WINDOWS_NT_RO)

//
//   basic string hash function, but gives very good distribution
//   model with standard ascii text.
//
//   NOTE:   ALL NAMES ARE HASHED UPPERCASE
//

ULONG HornerStringHash(BYTE *v, ULONG len, ULONG M)
{
   register ULONG h = 0, a = 127, i;
   UCHAR NameBuffer[256];
   OEM_STRING Dest;
   OEM_STRING Src;
   register UCHAR *p;

   Dest.Length = 0;
   Dest.MaximumLength = 256;
   Dest.Buffer = NameBuffer;

   Src.Length = len;
   Src.MaximumLength = len;
   Src.Buffer = v;

   UpperCaseConvert(&Dest, &Src);
   p = Dest.Buffer;

   if (!M)
   {
      DbgPrint("nwfs:  horner hash limit error [%s] lim-%u\n",
	     v, (unsigned int)M);
      return -1;
   }

   for (i = 0; i < len && *p; p++, i++)
      h = ((a * h) + *p) % M;

   return h;

}

//
//  very good universal string hash function with a built-in random
//  number genrator, but has more computational overhead.
//

ULONG UniversalStringHash(BYTE *v, ULONG len, ULONG M)
{
   register ULONG h, a = 31415, b = 27163, i;
   UCHAR NameBuffer[256];
   OEM_STRING Dest;
   OEM_STRING Src;
   register UCHAR *p;

   Dest.Length = 0;
   Dest.MaximumLength = 256;
   Dest.Buffer = NameBuffer;

   Src.Length = len;
   Src.MaximumLength = len;
   Src.Buffer = v;

   UpperCaseConvert(&Dest, &Src);
   p = Dest.Buffer;

   if (!M)
   {
      DbgPrint("nwfs:  universal hash limit error [%s] lim-%u\n",
	      v, (unsigned int)M);
      return -1;
   }

   for (i = 0, h = 0; i < len && *p; i++, p++, a = (a * b) % (M - 1))
      h = ((a * h) + *p) % M;

   return h;

}

HASH *GetHashFromNameUppercase(VOLUME *volume, BYTE *PathString, ULONG len,
			       ULONG NameSpace, ULONG CurrentDirectory)
{
    register HASH *name;
    register HASH_LIST *HashTable;
    register ULONG Value;
    UCHAR NameBuffer[256];
    OEM_STRING Dest;
    OEM_STRING Src;

    Value = NWFSStringHash(PathString, len,
			   volume->VolumeNameHashLimit[NameSpace & 0xF]);
    if (Value == -1)
    {
       DbgPrint("nwfs:  hashing function error\n");
       return 0;
    }

    HashTable = (HASH_LIST *) volume->VolumeNameHash[NameSpace & 0xF];
    if (!HashTable)
       return 0;

    name = (HASH *) HashTable[Value].head;
    while (name)
    {
       if ((len == name->NameLength) &&
           (name->Parent == CurrentDirectory) &&
	   (name->Root != (ULONG) -1) &&
	   (name->nlroot->Parent != (ULONG) DELETED_DIRECTORY))
       {
	  Dest.Length = 0;
	  Dest.MaximumLength = 256;
	  Dest.Buffer = NameBuffer;

	  Src.Length = name->NameLength;
	  Src.MaximumLength = name->NameLength;
	  Src.Buffer = name->Name;

	  UpperCaseConvert(&Dest, &Src);

	  if (!NWFSCompare(Dest.Buffer, PathString, len))
	      return (HASH *) name;
       }
       name = name->next;
    }
    return 0;

}

#endif

void NWLockNameHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&volume->NameHash_spinlock, volume->NameHash_flags);
#else
    if (WaitOnSemaphore(&volume->NameHashSemaphore) == -EINTR)
       NWFSPrint("lock name hash was interrupted\n");
#endif
#endif
}
void NWUnlockNameHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&volume->NameHash_spinlock, volume->NameHash_flags);
#else
    SignalSemaphore(&volume->NameHashSemaphore);
#endif
#endif
}

void NWLockDirectoryHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&volume->DirHash_spinlock, volume->DirHash_flags);
#else
    if (WaitOnSemaphore(&volume->DirHashSemaphore) == -EINTR)
       NWFSPrint("lock dir hash was interrupted\n");
#endif
#endif
}

void NWUnlockDirectoryHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&volume->DirHash_spinlock, volume->DirHash_flags);
#else
    SignalSemaphore(&volume->DirHashSemaphore);
#endif
#endif
}

void NWLockParentHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&volume->ParentHash_spinlock, volume->ParentHash_flags);
#else
    if (WaitOnSemaphore(&volume->ParentHashSemaphore) == -EINTR)
       NWFSPrint("lock parent hash was interrupted\n");
#endif
#endif
}

void NWUnlockParentHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&volume->ParentHash_spinlock, 
		           volume->ParentHash_flags);
#else
    SignalSemaphore(&volume->ParentHashSemaphore);
#endif
#endif
}

void NWLockTrusteeHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&volume->TrusteeHash_spinlock, volume->TrusteeHash_flags);
#else
    if (WaitOnSemaphore(&volume->TrusteeHashSemaphore) == -EINTR)
       NWFSPrint("lock trustee hash was interrupted\n");
#endif
#endif
}

void NWUnlockTrusteeHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&volume->TrusteeHash_spinlock, 
		            volume->TrusteeHash_flags);
#else
    SignalSemaphore(&volume->TrusteeHashSemaphore);
#endif
#endif
}

void NWLockQuotaHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&volume->QuotaHash_spinlock, volume->QuotaHash_flags);
#else
    if (WaitOnSemaphore(&volume->QuotaHashSemaphore) == -EINTR)
       NWFSPrint("lock quota hash was interrupted\n");
#endif
#endif
}

void NWUnlockQuotaHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&volume->QuotaHash_spinlock, 
		           volume->QuotaHash_flags);
#else
    SignalSemaphore(&volume->QuotaHashSemaphore);
#endif
#endif
}

void NWLockExtHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&volume->ExtHash_spinlock, volume->ExtHash_flags);
#else
    if (WaitOnSemaphore(&volume->ExtHashSemaphore) == -EINTR)
       NWFSPrint("lock ext hash was interrupted\n");
#endif
#endif
}

void NWUnlockExtHash(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&volume->ExtHash_spinlock, volume->ExtHash_flags);
#else
    SignalSemaphore(&volume->ExtHashSemaphore);
#endif
#endif
}

void NWLockNS(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_lock_irqsave(&volume->NameSpace_spinlock, volume->NameSpace_flags);
#else
    if (WaitOnSemaphore(&volume->NameSpaceSemaphore) == -EINTR)
       NWFSPrint("lock ns was interrupted\n");
#endif
#endif
}

void NWUnlockNS(VOLUME *volume)
{
#if (LINUX_SLEEP)
#if (LINUX_SPIN)
    spin_unlock_irqrestore(&volume->NameSpace_spinlock, 
		           volume->NameSpace_flags);
#else
    SignalSemaphore(&volume->NameSpaceSemaphore);
#endif
#endif
}

// we use the horner string hashing function for now

ULONG NWFSStringHash(BYTE *v, ULONG len, ULONG M)
{
    return (HornerStringHash(v, len, M));
}

// need to add a splay tree in place of the linked list for the
// name hash at some point in the future.

HASH *GetHashFromName(VOLUME *volume, BYTE *PathString, ULONG len,
		      ULONG NameSpace, ULONG Parent)
{
    register HASH *name;
    register HASH_LIST *HashTable;
    register ULONG Value;
    register BYTE *CName;
    register ULONG CLength;
    register ULONG i, retCode;
    BYTE UpperCaseName[256];
    BYTE DOSName[13];
    ULONG DOSLength = 0;
    extern BYTE DOSUpperCaseTable[256];

    // set these two variables to PathString and len as defaults
    CName = PathString;
    CLength = len;

    // if the DOS namespace is the volume default, then uppercase
    // and convert all names prior to calculating their hash
    // values.

    if (NameSpace == DOS_NAME_SPACE)
    {
       for (i=0; i < CLength; i++)
	  UpperCaseName[i] = DOSUpperCaseTable[(BYTE)PathString[i]];

       retCode = NWCreateUniqueName(volume, Parent, UpperCaseName, CLength,
				    DOSName, (BYTE *)&DOSLength, 1);
       if (retCode)
	  return 0;

       CName = DOSName;
       CLength = DOSLength;
    }

    Value = NWFSStringHash(CName, CLength,
			   volume->VolumeNameHashLimit[NameSpace & 0xF]);

    if (Value == -1)
    {
       NWFSPrint("nwfs:  hashing function error\n");
       return 0;
    }

    NWLockNameHash(volume);
    HashTable = (HASH_LIST *) volume->VolumeNameHash[NameSpace & 0xF];
    if (!HashTable)
    {
       NWUnlockNameHash(volume);
       return 0;
    }

    name = (HASH *) HashTable[Value].head;
    while (name)
    {
       if ((CLength == name->NameLength) &&
	   (name->nlroot->Parent != (ULONG) DELETED_DIRECTORY) &&
	   (name->Parent == Parent) && (name->Root != (ULONG) -1))
       {
	  if (!NWFSCompare(name->Name, CName, CLength))
	  {
	     NWUnlockNameHash(volume);
	     return (HASH *) name;
	  }
       }
       name = name->next;
    }
    NWUnlockNameHash(volume);
    return 0;

}

HASH *GetDeletedHashFromName(VOLUME *volume, BYTE *PathString, ULONG len,
		             ULONG NameSpace, ULONG Parent)
{
    register HASH *name;
    register HASH_LIST *HashTable;
    register ULONG Value;
    register BYTE *CName;
    register ULONG CLength;
    register ULONG i, retCode;
    BYTE UpperCaseName[256];
    BYTE DOSName[13];
    ULONG DOSLength = 0;
    extern BYTE DOSUpperCaseTable[256];

    // set these two variables to PathString and len as defaults
    CName = PathString;
    CLength = len;

    // if the DOS namespace is the volume default, then uppercase
    // and convert all names prior to calculating their hash
    // values.

    if (NameSpace == DOS_NAME_SPACE)
    {
       for (i=0; i < CLength; i++)
	  UpperCaseName[i] = DOSUpperCaseTable[(BYTE)PathString[i]];

       retCode = NWCreateUniqueName(volume, Parent, UpperCaseName, CLength,
				    DOSName, (BYTE *)&DOSLength, 1);
       if (retCode)
	  return 0;

       CName = DOSName;
       CLength = DOSLength;
    }

    Value = NWFSStringHash(CName, CLength,
			   volume->VolumeNameHashLimit[NameSpace & 0xF]);

    if (Value == -1)
    {
       NWFSPrint("nwfs:  hashing function error\n");
       return 0;
    }

    NWLockNameHash(volume);
    HashTable = (HASH_LIST *) volume->VolumeNameHash[NameSpace & 0xF];
    if (!HashTable)
    {
       NWUnlockNameHash(volume);
       return 0;
    }

    name = (HASH *) HashTable[Value].head;
    while (name)
    {
       if ((CLength == name->NameLength) &&
	   (name->nlroot->Parent == (ULONG) DELETED_DIRECTORY) &&
	   (name->Parent == Parent) && (name->Root != (ULONG) -1))
       {
	  if (!NWFSCompare(name->Name, CName, CLength))
	  {
	     NWUnlockNameHash(volume);
	     return (HASH *) name;
	  }
       }
       name = name->next;
    }
    NWUnlockNameHash(volume);
    return 0;

}

HASH *GetHashFromDirectoryNumber(VOLUME *volume, ULONG Directory)
{
    register HASH *name;
    register HASH_LIST *HashTable;
    register ULONG Value;

    NWLockDirectoryHash(volume);

    Value = (Directory & (volume->DirectoryNumberHashLimit - 1));
    HashTable = (HASH_LIST *) volume->DirectoryNumberHash;
    if (!HashTable)
    {
       NWUnlockDirectoryHash(volume);
       return 0;
    }

    name = (HASH *) HashTable[Value].head;
    while (name)
    {
       if ((name->DirNo == Directory) && (name->Root != (ULONG) -1))
       {
	  NWUnlockDirectoryHash(volume);
	  return (HASH *) name;
       }
       name = name->dnext;
    }
    NWUnlockDirectoryHash(volume);
    return 0;

}

EXTENDED_DIR_HASH *GetHashFromExtendedDirectoryNumber(VOLUME *volume,
						      ULONG Parent)
{
    register EXTENDED_DIR_HASH *name;
    register EXT_HASH_LIST *HashTable;
    register ULONG Value;

    NWLockExtHash(volume);

    Value = (Parent & (volume->ExtDirHashLimit - 1));
    HashTable = (EXT_HASH_LIST *) volume->ExtDirHash;
    if (!HashTable)
    {
       NWUnlockExtHash(volume);
       return 0;
    }

    name = (EXTENDED_DIR_HASH *) HashTable[Value].head;
    while (name)
    {
       if ((name->Parent == Parent) && (name->Root != (ULONG) -1))
       {
	  NWUnlockExtHash(volume);
	  return (EXTENDED_DIR_HASH *) name;
       }
       name = name->next;
    }
    NWUnlockExtHash(volume);
    return 0;

}

HASH *GetHashNameSpaceEntry(VOLUME *volume, ULONG entry,
			    ULONG NameSpace, ULONG Directory,
			    HASH **DirectoryHash)
{
    register HASH *name, *nlname;
    register HASH_LIST *HashTable;
    register ULONG Value;

#if (VERBOSE)
    NWFSPrint("get_hash_entry %d parent %X namespace %d\n",
	      (int)entry, (unsigned)Directory, (int)NameSpace);
#endif

    if (DirectoryHash)
       *DirectoryHash = 0;

    NWLockParentHash(volume);
    Value = (Directory & (volume->ParentHashLimit - 1));
    HashTable = (HASH_LIST *) volume->ParentHash;
    if (!HashTable)
    {
       NWUnlockParentHash(volume);
       return 0;
    }

    name = (HASH *) HashTable[Value].head;
    while (name)
    {
       // skip records flagged as invalid and any root entries in the
       // parent hash
       if ((name->Parent == Directory) && (name->Root != (ULONG) -1))
       {
	  if (name->Root >= entry)
	  {
	     nlname = name->nlroot;
	     while (nlname)
	     {
		if (nlname->NameSpace == NameSpace)
		{
		   NWUnlockParentHash(volume);
		   return nlname;
		}
		nlname = nlname->nlnext;
	     }
	     NWUnlockParentHash(volume);
	     return name;
	  }
       }
       name = name->pnext;
    }
    NWUnlockParentHash(volume);
    return 0;

}

HASH *AllocHashNode(void)
{
    register HASH *hash;

    hash = GetHashNode();
    if (!hash)
    {
       hash = NWFSAlloc(sizeof(HASH_UNION), HASH_TAG);
       if (hash)
       {
	  NWFSSet(hash, 0, sizeof(HASH_UNION));

	  hash->Signature = (ULONG)"HASH";
	  if (!InstanceCounter)
	     InstanceCounter = 1;

#if (LINUX_SLEEP)
	  AllocateSemaphore(&hash->Semaphore, 1);
#endif
       }
    }
    else
    {
       hash->Signature = (ULONG)"HASH";
       if (!InstanceCounter)
	  InstanceCounter = 1;

       if (hash->Name)
	  NWFSFree(hash->Name);
       hash->Name = 0;
       hash->Blocks = 0;
       hash->TurboFATCluster = 0;
       hash->TurboFATIndex = 0;

#if (HASH_FAT_CHAINS)
       hash->FirstBlock = (ULONG)-1;
       hash->FileSize = 0;
#endif
    }
    return hash;
}

void FreeHashNode(HASH *hash)
{
    hash->Signature = 0;
    hash->pnext = 0;
    hash->pprior = 0;
    hash->Blocks = 0;
    hash->TurboFATCluster = 0;
    hash->TurboFATIndex = 0;

#if (HASH_FAT_CHAINS)
    hash->FirstBlock = (ULONG)-1;
    hash->FileSize = 0;
#endif
    PutHashNode(hash);
    return;
}

void FreeHashNodes(void)
{
    register HASH *hash;

    NWLockHashFreeList();
    while (HashHead)
    {
       hash = HashHead;
       HashHead = HashHead->next;

       if (hash->Name)
	  NWFSFree(hash->Name);
       hash->Name = 0;
       hash->Blocks = 0;
       hash->TurboFATCluster = 0;
       hash->TurboFATIndex = 0;

#if (HASH_FAT_CHAINS)
       hash->FirstBlock = (ULONG)-1;
       hash->FileSize = 0;
#endif
       NWFSFree(hash);
    }
    NWUnlockHashFreeList();
    return;
}

HASH *CreateHashNode(DOS *dos, ULONG DirNo)
{
    register ROOT *root;
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *longname;
    register NTNAME *nt;
    register TRUSTEE *trustee;
    register USER *user;
    register HASH *name;
    register THASH *tname;
    register UHASH *uname;

    switch (dos->Subdirectory)
    {
       case TRUSTEE_NODE:
	  tname = (THASH *) NWFSAlloc(sizeof(HASH_UNION), HASH_TAG);
	  if (tname)
	  {
	     trustee = (TRUSTEE *) dos;
	     tname->Parent = trustee->Subdirectory;
	     tname->DirNo = DirNo;
	     tname->NextTrustee = trustee->NextTrusteeEntry;
	     tname->FileEntryNumber = trustee->FileEntryNumber;
	     tname->Flags = trustee->Flags;
	     tname->Attributes = trustee->Attributes;
	     return (HASH *) tname;
	  }
	  return 0;

       case RESTRICTION_NODE:
	  uname = (UHASH *) NWFSAlloc(sizeof(HASH_UNION), HASH_TAG);
	  if (uname)
	  {
	     user = (USER *) dos;
	     uname->Parent = user->Subdirectory;
	     uname->DirNo = DirNo;
	     uname->TrusteeCount = user->TrusteeCount;
	     return (HASH *) uname;
	  }
	  return 0;

       case SUBALLOC_NODE:
       case FREE_NODE:
	  return 0;

       case ROOT_NODE:
	  name = (HASH *) AllocHashNode();
	  if (name)
	  {
	     root = (ROOT *) dos;
	     name->NameLength = 0;
	     name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
	     if (!name->Name)
	     {
		NWFSFree(name);
		return 0;
	     }
	     NWFSCopy(name->Name, "", name->NameLength);
	     name->Name[name->NameLength] = '\0';
	     name->Parent = (ULONG) ROOT_NODE;
	     name->DirNo = DirNo;
	     name->Root = 0; // primary entry is assumed as zero
	     name->NameSpace = root->NameSpace;
	     name->NameLink = root->NameList;
	     name->Flags = root->Flags;
	     name->NextTrustee = root->NextTrusteeEntry;

#if (HASH_FAT_CHAINS)
	     name->FileAttributes = root->FileAttributes;
	     name->FileSize = 0;
	     name->FirstBlock = 0;
#endif
	     
#if (WINDOWS_NT | WINDOWS_NT_RO)
	     name->FileAttributes = root->FileAttributes;
	     name->FileSize = 0;
	     name->FirstBlock = 0;
	     name->CreateDateAndTime = root->CreateDateAndTime;
	     name->UpdatedDateAndTime = root->LastModifiedDateAndTime;
	     name->AccessedDate = 0;
#endif

	     return name;
	  }
	  return 0;

       default:
	  switch (dos->NameSpace)
	  {
	     case DOS_NAME_SPACE:
		name = (HASH *) AllocHashNode();
		if (!name)
		   return 0;

		name->NameLength = dos->FileNameLength;
		name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
		if (!name->Name)
		{
		   NWFSFree(name);
		   return 0;
		}
		NWFSCopy(name->Name, dos->FileName, name->NameLength);

		name->Name[name->NameLength] = '\0';
		name->Parent = dos->Subdirectory;
		name->DirNo = DirNo;
		name->Root = dos->PrimaryEntry;
		name->NameSpace = dos->NameSpace;
		name->NameLink = dos->NameList;
		name->Flags = dos->Flags;
		name->NextTrustee = dos->NextTrusteeEntry;

#if (HASH_FAT_CHAINS)
		name->FileAttributes = dos->FileAttributes;
		if (dos->Flags & SUBDIRECTORY_FILE)
		{
		   name->FileSize = 0;
		   name->FirstBlock = 0;
		}
		else
		{
		   name->FileSize = dos->FileSize;
		   name->FirstBlock = dos->FirstBlock;
		}
#endif

#if (WINDOWS_NT | WINDOWS_NT_RO)
		name->FileAttributes = dos->FileAttributes;
		name->CreateDateAndTime = dos->CreateDateAndTime;
		name->UpdatedDateAndTime = dos->LastUpdatedDateAndTime;
		name->AccessedDate = dos->LastAccessedDate;

		if (dos->Flags & SUBDIRECTORY_FILE)
		{
		   name->FileSize = 0;
		   name->FirstBlock = 0;
		}
		else
		{
		   name->FileSize = dos->FileSize;
		   name->FirstBlock = dos->FirstBlock;
		}
#endif
		return name;

	     case MAC_NAME_SPACE:
		mac = (MACINTOSH *) dos;
		name = (HASH *) AllocHashNode();
		if (!name)
		   return 0;

		name->NameLength = mac->FileNameLength;
		name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
		if (!name->Name)
		{
		   NWFSFree(name);
		   return 0;
		}
		NWFSCopy(name->Name, mac->FileName, name->NameLength);

		name->Name[name->NameLength] = '\0';
		name->Parent = mac->Subdirectory;
		name->DirNo = DirNo;
		name->Root = mac->PrimaryEntry;
		name->NameSpace = mac->NameSpace;
		name->NameLink = mac->NameList;
		name->Flags = mac->Flags;

#if (HASH_FAT_CHAINS)
		name->FirstBlock = mac->ResourceFork;
		name->FileSize = mac->ResourceForkSize;
#endif

#if (WINDOWS_NT | WINDOWS_NT_RO)
		name->FirstBlock = mac->ResourceFork;
		name->FileSize = mac->ResourceForkSize;
#endif

		return name;

	     case UNIX_NAME_SPACE:
		nfs = (NFS *) dos;
		name = (HASH *) AllocHashNode();
		if (!name)
		   return 0;

		name->NameLength = nfs->TotalFileNameLength;
		name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
		if (!name->Name)
		{
		   NWFSFree(name);
		   return 0;
		}
		NWFSCopy(name->Name, nfs->FileName, nfs->FileNameLength);

		name->Name[name->NameLength] = '\0';
		name->Parent = nfs->Subdirectory;
		name->DirNo = DirNo;
		name->Root = nfs->PrimaryEntry;
		name->NameSpace = nfs->NameSpace;
		name->NameLink = nfs->NameList;
		name->Flags = nfs->Flags;
		
#if (HASH_FAT_CHAINS)
		name->FirstBlock = 0;
#endif

#if (WINDOWS_NT | WINDOWS_NT_RO)
		name->FirstBlock = 0;
#endif
		return name;

	     case LONG_NAME_SPACE:
		longname = (LONGNAME *) dos;
		name = (HASH *) AllocHashNode();
		if (!name)
		   return 0;

		name->NameLength = (WORD)(longname->FileNameLength + longname->ExtendedSpace);
		name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
		if (!name->Name)
		{
		   NWFSFree(name);
		   return 0;
		}
		NWFSCopy(name->Name, longname->FileName, longname->FileNameLength);

		name->Name[name->NameLength] = '\0';
		name->Parent = longname->Subdirectory;
		name->DirNo = DirNo;
		name->Root = longname->PrimaryEntry;
		name->NameSpace = longname->NameSpace;
		name->NameLink = longname->NameList;
		name->Flags = longname->Flags;

#if (HASH_FAT_CHAINS)
		name->FirstBlock = 0;
#endif

#if (WINDOWS_NT | WINDOWS_NT_RO)
		name->FirstBlock = 0;
#endif
		return name;

	     case NT_NAME_SPACE:
		nt = (NTNAME *) dos;
		name = (HASH *) AllocHashNode();
		if (!name)
		   return 0;

		name->NameLength = nt->FileNameLength + nt->LengthData;
		name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
		if (!name->Name)
		{
		   NWFSFree(name);
		   return 0;
		}
		NWFSCopy(name->Name, nt->FileName, nt->FileNameLength);

		name->Name[name->NameLength] = '\0';
		name->Parent = nt->Subdirectory;
		name->DirNo = DirNo;
		name->Root = nt->PrimaryEntry;
		name->NameSpace = nt->NameSpace;
		name->NameLink = nt->NameList;
		name->Flags = nt->Flags;

#if (HASH_FAT_CHAINS)
		name->FirstBlock = 0;
#endif

#if (WINDOWS_NT | WINDOWS_NT_RO)
		name->FirstBlock = 0;
#endif
		return name;

	     case FTAM_NAME_SPACE:
	     default:
		return 0;
	  }
	  return 0;
    }
    return 0;

}

BYTE *CreateNameField(DOS *dos, HASH *name)
{
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *longname;
    register NTNAME *nt;

    switch (dos->NameSpace)
    {
       case DOS_NAME_SPACE:
	  name->NameLength = dos->FileNameLength;
	  name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
	  if (!name->Name)
	     return name->Name;
	  NWFSCopy(name->Name, dos->FileName, name->NameLength);
	  name->Name[name->NameLength] = '\0';
	  return name->Name;

       case MAC_NAME_SPACE:
	  mac = (MACINTOSH *) dos;
	  name->NameLength = mac->FileNameLength;
	  name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
	  if (!name->Name)
	     return name->Name;
	  NWFSCopy(name->Name, mac->FileName, name->NameLength);
	  name->Name[name->NameLength] = '\0';
	  return name->Name;

       case UNIX_NAME_SPACE:
	  nfs = (NFS *) dos;
	  name->NameLength = nfs->TotalFileNameLength;
	  name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
	  if (!name->Name)
	     return name->Name;
	  NWFSCopy(name->Name, nfs->FileName, nfs->FileNameLength);
	  name->Name[name->NameLength] = '\0';
	  return name->Name;

       case LONG_NAME_SPACE:
	  longname = (LONGNAME *) dos;
	  name->NameLength = (WORD)(longname->FileNameLength + longname->ExtendedSpace);
	  name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
	  if (!name->Name)
	     return name->Name;
	  NWFSCopy(name->Name, longname->FileName, longname->FileNameLength);
	  name->Name[name->NameLength] = '\0';
	  return name->Name;

       case NT_NAME_SPACE:
	  nt = (NTNAME *) dos;
	  name->NameLength = nt->FileNameLength + nt->LengthData;
	  name->Name = NWFSAlloc(name->NameLength + 1, NAME_STORAGE_TAG);
	  if (!name->Name)
	     return name->Name;
	  NWFSCopy(name->Name, nt->FileName, nt->FileNameLength);
	  name->Name[name->NameLength] = '\0';
	  return name->Name;

       case FTAM_NAME_SPACE:
       default:
	  return 0;
    }
    return 0;

}

ULONG HashDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo)
{
    register HASH *dirHash;
    register HASH *name;
    register THASH *tname;
    register UHASH *uname;
    register ULONG retCode, i;

#if (STRICT_NAMES)
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *longname;
    register NTNAME *nt;
#endif

    //
    //  processor dependent max number of directories is addr max - 3
    //  reserved dir numbers are as follows:
    //
    //        0  =  root directory
    //       -1  =  free dir block list
    //       -2  =  deleted dir block list
    //  1 to -3  =  valid directory record numbers
    //

    if ((!DirNo) && (dos->Subdirectory != ROOT_NODE))
    {
       NWFSPrint("nwfs:  root directory record is invalid\n");
       return NwHashError;
    }

    if ((ULONG) DirNo > (ULONG) -3)
    {
       NWFSPrint("nwfs:  directory number is a reserved value\n");
       return (ULONG) NwInvalidParameter;
    }

    if (GetHashFromDirectoryNumber(volume, DirNo))
    {
       NWFSPrint("nwfs:  duplicate dir no detected (%d)\n", (int)DirNo);
       return (ULONG) 0;
    }

    switch (dos->Subdirectory)
    {
       case TRUSTEE_NODE:
	  tname = (THASH *) CreateHashNode(dos, DirNo);
	  if (!tname)
	  {
	     NWFSPrint("nwfs:  could not alloc trustee hash DirNo-%d\n", (int)DirNo);
	     return NwHashError;
	  }
	  retCode = AddToTrusteeHash(volume, tname);
	  if (retCode)
	  {
	     NWFSFree((void *)tname);
	     NWFSPrint("nwfs:  could not insert trustee hash node\n");
	     return NwHashError;
	  }
	  retCode = AddTrusteeToDirectoryHash(volume, tname);
	  if (retCode)
	  {
	     RemoveTrusteeHash(volume, tname);
	     NWFSFree((void *)tname);
	     NWFSPrint("nwfs:  could not insert trustee directory hash node\n");
	     return NwHashError;
	  }
	  break;

       case RESTRICTION_NODE:
	  uname = (UHASH *) CreateHashNode(dos, DirNo);
	  if (!uname)
	  {
	     NWFSPrint("nwfs:  could not alloc user quota hash DirNo-%d\n", (int)DirNo);
	     return NwHashError;
	  }
	  retCode = AddToUserQuotaHash(volume, uname);
	  if (retCode)
	  {
	     NWFSFree((void *)uname);
	     NWFSPrint("nwfs:  could not insert user quota hash node\n");
	     return NwHashError;
	  }
	  retCode = AddUserQuotaToDirectoryHash(volume, uname);
	  if (retCode)
	  {
	     RemoveUserQuotaHash(volume, uname);
	     NWFSFree((void *)uname);
	     NWFSPrint("nwfs:  could not insert user quota directory hash node\n");
	     return NwHashError;
	  }
	  break;

       case SUBALLOC_NODE:  // this case is handled in NWDIR.C
       case FREE_NODE:
	  break;

       case ROOT_NODE:
	  name = CreateHashNode(dos, DirNo);
	  if (!name)
	  {
	     NWFSPrint("nwfs:  could not alloc root hash DirNo-%d\n", (int)DirNo);
	     return NwHashError;
	  }
	  retCode = AddToNameHash(volume, name);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not insert root hash node\n");
	     FreeHashNode(name);
	     return NwHashError;
	  }
	  retCode = AddToDirectoryHash(volume, name);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not insert dir root hash node\n");
	     RemoveNameHash(volume, name);
	     FreeHashNode(name);
	     return NwHashError;
	  }
	  if (name->NameSpace == DOS_NAME_SPACE)
	  {
	     retCode = AddToParentHash(volume, name);
	     if (retCode)
	     {
		RemoveNameHash(volume, name);
		RemoveDirectoryHash(volume, name);
		FreeHashNode(name);
		NWFSPrint("nwfs:  could not insert parent root node\n");
		return NwHashError;
	     }
	     volume->DirectoryCount++;
	  }
	  break;

       default:
	  if (!dos->FileNameLength)
	  {
	     NWFSPrint("nwfs:  invalid namespace directory record [%X]-%X\n",
		       (int)dos->PrimaryEntry, (unsigned int)DirNo);
	     return NwHashError;
	  }

	  switch (dos->NameSpace)
	  {
	     case DOS_NAME_SPACE:
#if (STRICT_NAMES)
		retCode = NWValidDOSName(volume,
					 dos->FileName,
					 dos->FileNameLength,
					 dos->Subdirectory,
					 0);
		if (retCode)
		{
		   NWFSPrint("nwfs:  invalid DOS name dirno-%X [",
			    (unsigned int)DirNo);
		   for (i=0; i < dos->FileNameLength; i++)
		      NWFSPrint("%c", dos->FileName[i]);
		   NWFSPrint("]-(%d)\n", dos->FileNameLength);

		   retCode = NWMangleDOSName(volume, dos->FileName,
					     dos->FileNameLength, dos,
					     dos->Subdirectory);
		   if (retCode)
		      return NwHashError;
		}
#endif
		// build an assignment list for any data streams
		// detected for files other than directories in the dos
		// namespace.  skip hard links, as several directory
                // entries may be linked to a single fat chain.

		if (!(dos->Flags & SUBDIRECTORY_FILE) &&
		    !(dos->Flags & HARD_LINKED_FILE))
		{
		   retCode = BuildChainAssignment(volume, dos->FirstBlock,
		       ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0));
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  (%d) suballoc not enabled on this volume\n",
				(int) retCode);
		      return NwHashError;
		   }
		}

		name = CreateHashNode(dos, DirNo);
		if (!name)
		{
		   NWFSPrint("nwfs:  could not alloc name hash DirNo-%d\n", (int)DirNo);
		   return NwHashError;
		}

		retCode = AddToNameHash(volume, name);
		if (retCode)
		{
		   FreeHashNode(name);
		   NWFSPrint("nwfs:  could not insert name hash node [%s]\n", name->Name);
		   return NwHashError;
		}
		retCode = AddToDirectoryHash(volume, name);
		if (retCode)
		{
		   RemoveNameHash(volume, name);
		   FreeHashNode(name);
		   NWFSPrint("nwfs:  could not insert dir hash node [%s]\n", name->Name);
		   return NwHashError;
		}

		if ((dos->Flags & NW4_DELETED_FILE) ||
		    (dos->Flags & NW3_DELETED_FILE))
		{
		   retCode = AddToMountHash(volume, name);
		   if (retCode)
		   {
		      RemoveNameHash(volume, name);
		      RemoveDirectoryHash(volume, name);
		      FreeHashNode(name);
		      NWFSPrint("nwfs:  could not insert deleted hash node [%s]\n", name->Name);
		      return NwHashError;
		   }
		   break;
		}

		// It is possible that during volume mount we might
		// reference a parent entry in the directory file
		// that has a higher DirNo than a child entry.  Since
		// we don't want to perform multiple passes through
		// the diretory file during volume mount, we store this
		// hash element on a temporary list.  Following the
		// first pass through the directory file, we traverse
		// this list and attempt to resolve any orphaned
		// hash records.

		dirHash = GetHashFromDirectoryNumber(volume, name->Parent);
		if (!dirHash)
		{
		   retCode = AddToMountHash(volume, name);
		   if (retCode)
		   {
		      RemoveNameHash(volume, name);
		      RemoveDirectoryHash(volume, name);
		      FreeHashNode(name);
		      NWFSPrint("nwfs:  could not insert parent hash node [%s]\n", name->Name);
		      return NwHashError;
		   }
		}
		else
		{
		   retCode = AddToParentHash(volume, name);
		   if (retCode)
		   {
		      RemoveNameHash(volume, name);
		      RemoveDirectoryHash(volume, name);
		      FreeHashNode(name);
		      NWFSPrint("nwfs:  could not insert parent hash node [%s]\n", name->Name);
		      return NwHashError;
		   }
		   if (name->Flags & SUBDIRECTORY_FILE)
		      volume->DirectoryCount++;
		}

		if ((!volume->DeletedDirNo) &&
		    (!NWFSCompare(name->Name, "DELETED.SAV", 11)))
		   volume->DeletedDirNo = name->DirNo;

		break;

	     case MAC_NAME_SPACE:
#if (STRICT_NAMES)
		mac = (MACINTOSH *) dos;
		retCode = NWValidMACName(volume,
					 mac->FileName,
					 mac->FileNameLength,
					 mac->Subdirectory,
					 0);
		if (retCode)
		{
		   NWFSPrint("nwfs:  invalid MAC name dirno-%X [",
			    (unsigned int)DirNo);
		   for (i=0; i < mac->FileNameLength; i++)
		      NWFSPrint("%c", mac->FileName[i]);
		   NWFSPrint("]\n");

		   retCode = NWMangleMACName(volume, mac->FileName,
					     mac->FileNameLength, mac,
					     mac->Subdirectory);
		   if (retCode)
		      return NwHashError;
		}
#endif
		// build an assignment list for any mac resource forks
		// detected for files other than directories in the mac
		// namespace.  skip hard linked files since several
                // directory entries may point to the same fat chain.

		if (!(dos->Flags & SUBDIRECTORY_FILE) &&
		    !(dos->Flags & HARD_LINKED_FILE))
		{
		   retCode = BuildChainAssignment(volume, mac->ResourceFork,
		       ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0));
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  suballoc not enabled on this volume\n");
		      return NwHashError;
		   }
		}
		goto CreateHash;

	     case UNIX_NAME_SPACE:
#if (STRICT_NAMES)
		nfs = (NFS *) dos;
		retCode = NWValidNFSName(volume,
					 nfs->FileName,
					 nfs->FileNameLength,
					 nfs->Subdirectory,
					 0);
		if (retCode)
		{
		   NWFSPrint("nwfs:  invalid UNIX name dirno-%X [",
			    (unsigned int)DirNo);
		   for (i=0; i < nfs->FileNameLength; i++)
		      NWFSPrint("%c", nfs->FileName[i]);
		   NWFSPrint("]\n");

		   retCode = NWMangleNFSName(volume, nfs->FileName,
					     nfs->FileNameLength, nfs,
					     nfs->Subdirectory);
		   if (retCode)
		      return NwHashError;
		}
		goto CreateHash;
#endif

	     case LONG_NAME_SPACE:
#if (STRICT_NAMES)
		longname = (LONGNAME *) dos;
		retCode = NWValidLONGName(volume,
					  longname->FileName,
					  longname->FileNameLength,
					  longname->Subdirectory,
					  0);
		if (retCode)
		{
		   NWFSPrint("nwfs:  invalid LONG name dirno-%X [",
			    (unsigned int)DirNo);
		   for (i=0; i < longname->FileNameLength; i++)
		      NWFSPrint("%c", longname->FileName[i]);
		   NWFSPrint("]\n");

		   retCode = NWMangleLONGName(volume, longname->FileName,
					      longname->FileNameLength,
					      longname,
					      longname->Subdirectory);
		   if (retCode)
		      return NwHashError;
		}
		goto CreateHash;
#endif

	     case NT_NAME_SPACE:
#if (STRICT_NAMES)
		nt = (NTNAME *) dos;
		retCode = NWValidLONGName(volume,
					 nt->FileName,
					 nt->FileNameLength,
					 nt->Subdirectory,
					 0);
		if (retCode)
		{
		   NWFSPrint("nwfs:  invalid NT name dirno-%X [",
			    (unsigned int)DirNo);
		   for (i=0; i < nt->FileNameLength; i++)
		      NWFSPrint("%c", nt->FileName[i]);
		   NWFSPrint("]\n");

		   retCode = NWMangleLONGName(volume, nt->FileName,
					      nt->FileNameLength,
					      (LONGNAME *)nt,
					      nt->Subdirectory);
		   if (retCode)
		      return NwHashError;
		}
CreateHash:;

#endif
		name = CreateHashNode(dos, DirNo);
		if (!name)
		{
		   NWFSPrint("nwfs:  could not alloc name hash DirNo-%d\n", (int)DirNo);
		   return NwHashError;
		}
		retCode = AddToNameHash(volume, name);
		if (retCode)
		{
		   NWFSPrint("nwfs:  could not insert name hash node [%s]\n", name->Name);
		   FreeHashNode(name);
		   return NwHashError;
		}
		retCode = AddToDirectoryHash(volume, name);
		if (retCode)
		{
		   NWFSPrint("nwfs:  could not insert dir hash node [%s]\n", name->Name);
		   RemoveNameHash(volume, name);
		   FreeHashNode(name);
		   return NwHashError;
		}
		break;

	     case FTAM_NAME_SPACE: // We have not seen this namespace.
	     default:
		break;
	  }
	  break;
    }
    return 0;

}

HASH *AllocHashDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo)
{
    register HASH *name = 0;
    register ULONG retCode;

    //
    //  processor dependent max number of directories is machine
    //  dependent address maximum - 4
    //  reserved dir numbers are as follows:
    //
    //        0  =  root directory
    //       -1  =  free dir block list
    //       -2  =  deleted dir block list
    //  1 to -3  =  valid dir block list values
    //
    //  Netware places limitations on the percentage
    //  of a volume that a directory file is allowed
    //  to occupy.

    // check for valid directory number range
    if ((ULONG) DirNo > (ULONG) -4)
       return (ULONG) 0;

    if (GetHashFromDirectoryNumber(volume, DirNo))
    {
       NWFSPrint("nwfs:  duplicate dir no detected (%d)\n", (int)DirNo);
       return (ULONG) 0;
    }

    switch (dos->Subdirectory)
    {
       case TRUSTEE_NODE:
       case RESTRICTION_NODE:
       case SUBALLOC_NODE:
       case FREE_NODE:
	  return 0;

       case ROOT_NODE:
	  name = CreateHashNode(dos, DirNo);
	  if (!name)
	  {
	     NWFSPrint("nwfs:  could not alloc name hash DirNo-%d\n", (int)DirNo);
	     return 0;
	  }
	  retCode = AddToNameHash(volume, name);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not insert name hash node [%s]\n", name->Name);
	     FreeHashNode(name);
	     return 0;
	  }
	  retCode = AddToDirectoryHash(volume, name);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not insert dir hash node [%s]\n", name->Name);
	     RemoveNameHash(volume, name);
	     FreeHashNode(name);
	     return 0;
	  }

	  if (name->NameSpace == DOS_NAME_SPACE)
	  {
	     retCode = AddToParentHash(volume, name);
	     if (retCode)
	     {
		RemoveNameHash(volume, name);
		RemoveDirectoryHash(volume, name);
		FreeHashNode(name);
		NWFSPrint("nwfs:  could not insert parent root node\n");
		return 0;
	     }

	     // link into namespace chain
	     retCode = InsertNameSpaceElement(volume, name);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  could not insert root namespace link node\n");
		RemoveParentHash(volume, name);
		RemoveDirectoryHash(volume, name);
		RemoveNameHash(volume, name);
		FreeHashNode(name);
		return 0;
	     }
	     volume->DirectoryCount++;
	  }
	  break;

       default:
	  if (!dos->FileNameLength)
	  {
	     NWFSPrint("nwfs:  invalid namespace directory record [%X]-%X\n",
		       (int)dos->PrimaryEntry, (unsigned int)DirNo);
	     return 0;
	  }

	  switch (dos->NameSpace)
	  {
	     case DOS_NAME_SPACE:
		name = CreateHashNode(dos, DirNo);
		if (!name)
		{
		   NWFSPrint("nwfs:  could not alloc name hash DirNo-%d\n", (int)DirNo);
		   return 0;
		}
		retCode = AddToNameHash(volume, name);
		if (retCode)
		{
		   FreeHashNode(name);
		   NWFSPrint("nwfs:  could not insert name hash node [%s]\n", name->Name);
		   return 0;
		}
		retCode = AddToDirectoryHash(volume, name);
		if (retCode)
		{
		   RemoveNameHash(volume, name);
		   FreeHashNode(name);
		   NWFSPrint("nwfs:  could not insert dir hash node [%s]\n", name->Name);
		   return 0;
		}

		if ((dos->Flags & NW4_DELETED_FILE) ||
		    (dos->Flags & NW3_DELETED_FILE))
	           name->Parent = DELETED_DIRECTORY;

		retCode = AddToParentHash(volume, name);
		if (retCode)
		{
		   RemoveDirectoryHash(volume, name);
		   RemoveNameHash(volume, name);
		   FreeHashNode(name);
		   NWFSPrint("nwfs:  could not insert parent hash node [%s]\n", name->Name);
		   return 0;
		}

		// link into namespace chain
		retCode = InsertNameSpaceElement(volume, name);
		if (retCode)
		{
		   NWFSPrint("nwfs:  could not insert dir name link node [%s]\n", name->Name);
		   RemoveParentHash(volume, name);
		   RemoveDirectoryHash(volume, name);
		   RemoveNameHash(volume, name);
		   FreeHashNode(name);
		   return 0;
		}

		if (name->Flags & SUBDIRECTORY_FILE)
		   volume->DirectoryCount++;
		break;

	     case MAC_NAME_SPACE:
	     case UNIX_NAME_SPACE:
	     case LONG_NAME_SPACE:
	     case NT_NAME_SPACE:
		name = CreateHashNode(dos, DirNo);
		if (!name)
		{
		   NWFSPrint("nwfs:  could not alloc name hash DirNo-%d\n", (int)DirNo);
		   return 0;
		}
		retCode = AddToNameHash(volume, name);
		if (retCode)
		{
		   NWFSPrint("nwfs:  could not insert name hash node [%s]\n", name->Name);
		   FreeHashNode(name);
		   return 0;
		}
		retCode = AddToDirectoryHash(volume, name);
		if (retCode)
		{
		   NWFSPrint("nwfs:  could not insert dir hash node [%s]\n", name->Name);
		   RemoveNameHash(volume, name);
		   FreeHashNode(name);
		   return 0;
		}

		// link into namespace chain
		retCode = InsertNameSpaceElement(volume, name);
		if (retCode)
		{
		   NWFSPrint("nwfs:  could not insert dir name link node [%s]\n", name->Name);
		   RemoveDirectoryHash(volume, name);
		   RemoveNameHash(volume, name);
		   FreeHashNode(name);
		   return 0;
		}
		break;

	     case FTAM_NAME_SPACE:
	     default:
		break;
	  }
	  break;
    }
    return name;

}

ULONG FreeHashDirectoryRecord(VOLUME *volume, HASH *hash)
{
    // set the hash element signature to 0 to prevent
    // dir lookups during hash deletions.
    hash->Signature = 0;

    if (hash->NameSpace == DOS_NAME_SPACE)
    {
       if (hash->Flags & SUBDIRECTORY_FILE)
       {
	  if (volume->DirectoryCount)
	     volume->DirectoryCount--;
       }
       RemoveParentHash(volume, hash);
    }
    RemoveDirectoryHash(volume, hash);
    RemoveNameHash(volume, hash);
    FreeHashNode(hash);

    return 0;
}

ULONG HashExtendedDirectoryRecord(VOLUME *volume, EXTENDED_DIR *dir,
				  ULONG Cluster, ULONG DirNo)
{
    register ULONG retCode;
    register EXTENDED_DIR_HASH *ext;

    switch (dir->Signature)
    {
       case EXTENDED_DIR_STAMP:
       case LONG_EXTENDED_DIR_STAMP:
       case NT_EXTENDED_DIR_STAMP:
       case NFS_EXTENDED_DIR_STAMP:
       case MIGRATE_DIR_STAMP:
       case TALLY_EXTENDED_DIR_STAMP:
       case EXTENDED_ATTRIBUTE_DIR_STAMP:
	  ext = NWFSAlloc(sizeof(EXTENDED_DIR_HASH) + 1, EXT_HASH_TAG);
	  if (!ext)
	  {
	     NWFSPrint("nwfs:  could not alloc extended hash DirNo-%d\n", (int)DirNo);
	     return -1;
	  }
	  NWFSSet(ext, 0, sizeof(EXTENDED_DIR_HASH));

	  ext->ExtDirNo = DirNo;
	  ext->Parent = dir->DirectoryNumber;
	  ext->Signature = dir->Signature;
	  ext->Length = dir->Length;
	  ext->NameSpace = dir->NameSpace;
	  ext->Flags = dir->Flags;
	  ext->ControlFlags = dir->ControlFlags;

	  retCode = AddToExtHash(volume, ext);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not add extended dir node\n");
	     NWFSFree(ext);
	     return -1;
	  }
	  break;

       default:
	  break;
    }
    return 0;

}

//
//   During initial reading of the directory file, it may have been
//   possible that vrepair could have previoulsy had to reshuffle
//   records due to block dir assignment errors.  These means that some
//   namespace records may not have been written in monatomically
//   ascending order in the dir file.  Since we have not completely
//   cached all of the dir no assignments during initial mount, then
//   it is possible that a record being linked into a namespace chain
//   may point down rather than up with regard to the root DOS entry
//   for a filename that has mulitple namespace entries.  because of this,
//   we are forced to link the namespaces after all of the directory
//   records have been scanned into the dir number hash.  If we attempt
//   to force the namespace link before we have hashed all of the dir
//   records, then we may not be able to link in all the records.
//
//   during normal creates (after the dir file has been hashed) its
//   a simple matter to link namespace records together, but during
//   mount we are forced to provide a separate function that walks
//   the entire directory file hash and links the records together.
//   this is not a particularly elegant way to do accomplish this,
//   however, it allows us to avoid making multiple passes through
//   the directory file during volume mount.
//
//   we always assume that when we break down and free namespace lists,
//   we consume the list linkage and free the list as we traverse it.
//

HASH *RemoveNameSpaceElement(VOLUME *volume, HASH *hash)
{
    register HASH *searchHash, *root;

    root = hash->nlroot;
    if (!root)
    {
       NWFSPrint("nwfs:  nlroot invalid in RemoveNameSpaceElement\n");
       return 0;
    }

    // if this element is the root, then exit
    if (hash == root)
       return 0;

    NWLockNS(volume);

    // check and make certain this element is really linked
    searchHash = root->nlnext;
    while (searchHash)
    {
       if (searchHash == hash)
       {
	  if (root->nlnext == hash)
	  {
	     root->nlnext = (void *) hash->nlnext;
	     if (root->nlnext)
		root->nlnext->nlprior = NULL;
	     else
		root->nlprior = NULL;
	  }
	  else
	  {
	     hash->nlprior->nlnext = hash->nlnext;
	     if (hash != root->nlprior)
		hash->nlnext->nlprior = hash->nlprior;
	     else
		root->nlprior = hash->nlprior;
	  }
	  NWUnlockNS(volume);
	  return hash;
       }
       searchHash = searchHash->nlnext;
    }
    NWUnlockNS(volume);
    return 0;

}

ULONG InsertNameSpaceElement(VOLUME *volume, HASH *hash)
{
    register HASH *searchHash, *root;

    // check if this is a root entry, if so then exit
    if (hash->DirNo == hash->Root)
    {
       // this element is root
       hash->nlroot = hash;
       hash->nlnext = hash->nlprior = 0;
       return 0;
    }

    root = GetHashFromDirectoryNumber(volume, hash->Root);
    if (!root)
    {
       NWFSPrint("nwfs:  root dirno error InsertNameSpaceElement [%X->%X]\n",
		 (unsigned int) hash->DirNo,
		 (unsigned int) hash->Root);
       return NwHashError;
    }

    // if we found ourself, then exit (this shouldn't ever happen)
    if (hash == root)
    {
       NWFSPrint("nwfs:  root hash was cyclic in InsertNameSpaceElement\n");

       // this element is root
       hash->nlroot = hash;
       hash->nlnext = hash->nlprior = 0;
       return NwHashError;
    }

    // save the root hash entry for this element
    hash->nlroot = root;
    hash->Parent = root->Parent;
    
    // set the namespace record's parent to the root parent.  deleted
    // files may not have them set equal.
    hash->Parent = root->Parent;

    NWLockNS(volume);

    // check and make certain we have not already linked this element
    searchHash = root->nlnext;
    while (searchHash)
    {
       if (searchHash == hash)
       {
	  NWUnlockNS(volume);
	  return NwHashError;
       }
       searchHash = searchHash->nlnext;
    }

    if (!root->nlnext)
    {
       root->nlnext = hash;
       root->nlprior = hash;
       hash->nlnext = hash->nlprior = 0;
    }
    else
    {
       root->nlprior->nlnext = hash;
       hash->nlnext = 0;
       hash->nlprior = root->nlprior;
       root->nlprior = hash;
    }

    NWUnlockNS(volume);

    return 0;

}

ULONG LinkVolumeNameSpaces(VOLUME *volume)
{
    register HASH_LIST *HashTable;
    HASH *name, *nlname;
    register ULONG i, r, NameSpace, count, retCode, NameList;
    ULONG HashTraceCount;
    HASH *HashTrace[MAX_NAMESPACES];
    BYTE FoundNameSpace[MAX_NAMESPACES];
    ULONG FoundNameSpaceDirNo[MAX_NAMESPACES];
    ULONG NameLinkTrace[MAX_NAMESPACES];
    ULONG NameLinkCount, j;

#if (MOUNT_VERBOSE)
    NWFSPrint("*** Linking Volume Namespaces ***\n");
#endif

    for (r=0; r < MAX_NAMESPACES; r++)
    {
       FoundNameSpace[r] = 0;
       FoundNameSpaceDirNo[r] = 0;
    }

    // first we validate the NameLink field for the root namespace (DOS)
    // and verify the primary entry number of all linked records.  if
    // we detect a corrupted name link field, then we return an error
    // and refuse to mount the volume.  The user must run vrepair.
    // This error indicates that the volume directory file hash been
    // corrupted.

    NWLockNameHash(volume);
    for (i=0; i < volume->NameSpaceCount; i++)
    {
       NameSpace = volume->NameSpaceID[i];
       if (NameSpace == DOS_NAME_SPACE)
       {
	  if (volume->VolumeNameHash[NameSpace & 0xF])
	  {
	     HashTable = (HASH_LIST *) volume->VolumeNameHash[NameSpace & 0xF];

	     for (count=0;
		  count < volume->VolumeNameHashLimit[NameSpace & 0xF];
		  count++)
	     {
		name = (HASH *) HashTable[count].head;
		while (name)
		{
		   HashTraceCount = NameLinkCount = 0;
		   NWFSSet(&HashTrace[0], 0, (sizeof(HASH *) * MAX_NAMESPACES));
		   NWFSSet(&NameLinkTrace[0], 0, (sizeof(ULONG) * MAX_NAMESPACES));
		   NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
		   NWFSSet(&FoundNameSpaceDirNo[0], 0, (sizeof(ULONG) * MAX_NAMESPACES));

		   FoundNameSpace[name->NameSpace & 0xF] = TRUE;
		   FoundNameSpaceDirNo[name->NameSpace & 0xF] = name->DirNo;
		   HashTrace[HashTraceCount++] = name;

		   NameList = name->NameLink;
		   while (NameList)
		   {
		      NameLinkTrace[NameLinkCount++] =  NameList;

		      // see if this name list field points to a valid
		      // directory record
		      nlname = GetHashFromDirectoryNumber(volume, NameList);
		      if (!nlname)
		      {
			 NWFSPrint("nwfs:  record [%X] has invalid name list\n",
				   (unsigned int)NameList);
			 NWUnlockNameHash(volume);
			 return NwDirectoryCorrupt;
		      }
		      HashTrace[HashTraceCount++] = nlname;

		      // see if this dir record primary entry points back to
		      // the root DOS entry.  Netware tolerates this error
		      // in the directory, so we ignore the inconsistency
		      // and correct the hash in memory and in the directory.

		      if (nlname->Root != name->DirNo)
		      {
			 NWFSPrint("nwfs:  record [%X]-[%s] invalid primary entry [R-%X/D-%X]\n",
				   (unsigned int)NameList,
				   nlname->Name,
				   (unsigned int)nlname->Root,
				   (unsigned int)name->DirNo);

			 for (j=0; j < NameLinkCount; j++)
			 {
			    if ((j + 1) >= NameLinkCount)
			       NWFSPrint("0x%08X\n",
				      (unsigned int)NameLinkTrace[j]);
			    else
			       NWFSPrint("0x%08X -> ",
				      (unsigned int)NameLinkTrace[j]);
			 }

			 for (j=0; j < HashTraceCount; j++)
			 {
			    extern ULONG dumpRecord(BYTE *, ULONG);
			    extern ULONG dumpRecordBytes(BYTE *, ULONG);

			    if (HashTrace[j])
			    {
			       NWFSPrint("hash-%08X\n",
					(unsigned int)HashTrace[j]);
			       dumpRecord((BYTE *)HashTrace[j], sizeof(HASH));
			       dumpRecordBytes((BYTE *)HashTrace[j], sizeof(HASH));
			       if (HashTrace[j]->Name)
				  dumpRecordBytes(HashTrace[j]->Name,
						  HashTrace[j]->NameLength);
			    }
			 }
			 nlname->Root = name->DirNo;
		      }

		      // see if we have multiple entries in the name list
		      // that are the same name space
		      if (FoundNameSpace[nlname->NameSpace & 0xF])
		      {
			 NWFSPrint("nwfs:  multiple name space error [%X/%X] NS-(%d)\n",
				   (unsigned int)FoundNameSpaceDirNo[name->NameSpace & 0xF],
				   (unsigned int)nlname->DirNo,
				   (int)nlname->NameSpace);

			 for (j=0; j < NameLinkCount; j++)
			 {
			    if ((j + 1) >= NameLinkCount)
			       NWFSPrint("0x%08X\n",
				      (unsigned int)NameLinkTrace[j]);
			    else
			       NWFSPrint("0x%08X ->",
				      (unsigned int)NameLinkTrace[j]);
			 }

			 for (j=0; j < HashTraceCount; j++)
			 {
			    extern ULONG dumpRecord(BYTE *, ULONG);
			    extern ULONG dumpRecordBytes(BYTE *, ULONG);

			    if (HashTrace[j])
			    {
			       dumpRecord((BYTE *)HashTrace[j], sizeof(HASH));
			       dumpRecordBytes((BYTE *)HashTrace[j], sizeof(HASH));
			       if (HashTrace[j]->Name)
				  dumpRecordBytes(HashTrace[j]->Name,
						  HashTrace[j]->NameLength);
			    }
			 }

			 NWUnlockNameHash(volume);
			 return NwDirectoryCorrupt;
		      }

		      FoundNameSpace[nlname->NameSpace & 0xF] = TRUE;
		      FoundNameSpaceDirNo[nlname->NameSpace & 0xF] = nlname->DirNo;

		      NameList = nlname->NameLink;
		   }
		   name = name->next;
		}
	     }
	     break;
	  }
       }
    }

    // now we make a second pass through the namespaces and
    // link the namespace records onto the root entry in the name
    // hash.

    for (i=0; i < volume->NameSpaceCount; i++)
    {
       NameSpace = volume->NameSpaceID[i];
       if (volume->VolumeNameHash[NameSpace & 0xF])
       {
	  HashTable = (HASH_LIST *) volume->VolumeNameHash[NameSpace & 0xF];
	  for (count=0;
	       count < volume->VolumeNameHashLimit[NameSpace & 0xF];
	       count++)
	  {
	     name = (HASH *) HashTable[count].head;
	     while (name)
	     {
		// if we fail to locate the root entry for this record,
		// then flag the hash record as invalid.  this will tell
		// the search routines to ignore the record.  We will remove
		// the record later.

		retCode = InsertNameSpaceElement(volume, name);
		if (retCode)
		   name->Root = (ULONG) -1;

		name = name->next;
	     }
	  }
       }
    }
    NWUnlockNameHash(volume);
    return 0;

}

ULONG ProcessOrphans(VOLUME *volume)
{
    register ULONG retCode;
    register HASH *hash, *listHash, *searchHash;

    while (volume->HashMountHead)
    {
       hash = volume->HashMountHead;
       volume->HashMountHead = hash->pnext;

       if (!volume->HashMountHead)
	  volume->HashMountTail = 0;

       if ((hash->Flags & NW4_DELETED_FILE) ||
	   (hash->Flags & NW3_DELETED_FILE))
	  hash->Parent = DELETED_DIRECTORY;

       retCode = AddToParentHash(volume, hash);
       if (retCode)
       {
	  // see if this hash record's parent is further down
	  // on the hash mount list.  If so, insert this record at
	  // the end of the list, and process again after the parent
	  // has been added.

	  searchHash = volume->HashMountHead;
	  while (searchHash)
	  {
	     if (searchHash->DirNo == hash->Parent)
	     {
		retCode = AddToMountHash(volume, hash);
		if (retCode)
		   break;
		else
		   goto SkipRecord;
	     }
	     searchHash = searchHash->pnext;
	  }

	  // if we detect an orphaned file, remove the hash and
	  // all associated namespace records for this entry.

	  NWFSPrint("nwfs:  orphaned file [%s] detected [%X/%X]\n",
		    hash->Name, (unsigned int)hash->DirNo,
		    (unsigned int)hash->Parent);

	  listHash = hash;
	  while (listHash)
	  {
	     hash = listHash;
	     listHash = listHash->nlnext;
	     if (hash->NameSpace == DOS_NAME_SPACE)
	     {
		// set the hash element signature to 0 to prevent
		// dir lookups during hash deletions.
		hash->Signature = 0;

		RemoveDirectoryHash(volume, hash);
		RemoveNameHash(volume, hash);
		FreeHashNode(hash);
	     }
	     else
		FreeHashDirectoryRecord(volume, hash);
	  }
       }
       if (hash->Flags & SUBDIRECTORY_FILE)
	  volume->DirectoryCount++;

SkipRecord:;
    }
    return 0;

}

void FreeVolumeNamespaces(VOLUME *volume)
{
    register HASH_LIST *HashTable;
    register HASH *name, *list;
    register THASH *tname, *tlist;
    register UHASH *uname, *ulist;
    register EXTENDED_DIR_HASH *ext, *extlist;
    register ULONG i, NameSpace, count;

    //  the directory and parent hash records cross reference to the
    //  name space, trustee, and user quota hashes

    NWLockDirectoryHash(volume);
    if (volume->DirectoryNumberHash)
       NWFSFree(volume->DirectoryNumberHash);
    volume->DirectoryNumberHashLimit = 0;
    volume->DirectoryNumberHash = 0;
    NWUnlockDirectoryHash(volume);

    NWLockParentHash(volume);
    if (volume->ParentHash)
       NWFSFree(volume->ParentHash);
    volume->ParentHashLimit = 0;
    volume->ParentHash = 0;
    NWUnlockParentHash(volume);

    NWLockExtHash(volume);
    if (volume->ExtDirHash)
    {
       HashTable = (HASH_LIST *) volume->ExtDirHash;
       for (count=0; count < volume->ExtDirHashLimit; count++)
       {
	  extlist = (EXTENDED_DIR_HASH *) HashTable[count].head;
	  HashTable[count].head = HashTable[count].tail = 0;
	  while (extlist)
	  {
	     ext = extlist;
	     extlist = extlist->next;
	     NWFSFree((void *)ext);
	  }
       }
       NWFSFree(volume->ExtDirHash);
    }
    volume->ExtDirHashLimit = 0;
    volume->ExtDirHash = 0;
    NWUnlockExtHash(volume);

    NWLockTrusteeHash(volume);
    if (volume->TrusteeHash)
    {
       HashTable = (HASH_LIST *) volume->TrusteeHash;
       for (count=0; count < volume->TrusteeHashLimit; count++)
       {
	  tlist = (THASH *) HashTable[count].head;
	  HashTable[count].head = HashTable[count].tail = 0;
	  while (tlist)
	  {
	     tname = tlist;
	     tlist = tlist->next;
	     NWFSFree((void *)tname);
	  }
       }
       NWFSFree(volume->TrusteeHash);
    }
    volume->TrusteeHashLimit = 0;
    volume->TrusteeHash = 0;
    NWUnlockTrusteeHash(volume);

    NWLockQuotaHash(volume);
    if (volume->UserQuotaHash)
    {
       HashTable = (HASH_LIST *) volume->UserQuotaHash;
       for (count=0; count < volume->UserQuotaHashLimit; count++)
       {
	  ulist = (UHASH *) HashTable[count].head;
	  HashTable[count].head = HashTable[count].tail = 0;
	  while (ulist)
	  {
	     uname = ulist;
	     ulist = ulist->next;
	     NWFSFree((void *)uname);
	  }
       }
       NWFSFree(volume->UserQuotaHash);
    }
    volume->UserQuotaHashLimit = 0;
    volume->UserQuotaHash = 0;
    NWUnlockQuotaHash(volume);

    NWLockNameHash(volume);
    for (i=0; i < volume->NameSpaceCount; i++)
    {
       NameSpace = volume->NameSpaceID[i];

       if (volume->VolumeNameHash[NameSpace & 0xF])
       {
	  HashTable = (HASH_LIST *) volume->VolumeNameHash[NameSpace & 0xF];
	  for (count=0;
	       count < volume->VolumeNameHashLimit[NameSpace & 0xF];
	       count++)
	  {
	     list = (HASH *) HashTable[count].head;
	     HashTable[count].head = HashTable[count].tail = 0;
	     while (list)
	     {
		name = list;
		list = list->next;
		FreeHashNode(name);
	     }
	  }
	  NWFSFree(volume->VolumeNameHash[NameSpace & 0xF]);
       }
       volume->VolumeNameHashLimit[NameSpace & 0xF] = 0;
       volume->VolumeNameHash[NameSpace & 0xF] = 0;
    }
    NWUnlockNameHash(volume);

}

//
//
//

ULONG AddToNameHash(VOLUME *volume, HASH *name)
{
    register ULONG Value;
    register HASH_LIST *HashTable;

    Value = NWFSStringHash(name->Name, name->NameLength,
			   volume->VolumeNameHashLimit[name->NameSpace]);
    if (Value == (ULONG) -1)
       return -1;

    NWLockNameHash(volume);

    HashTable = (HASH_LIST *) volume->VolumeNameHash[name->NameSpace & 0xF];
    if (HashTable)
    {
       if (!HashTable[Value].head)
       {
	  HashTable[Value].head = name;
	  HashTable[Value].tail = name;
	  name->next = name->prior = 0;
       }
       else
       {
	  HashTable[Value].tail->next = name;
	  name->next = 0;
	  name->prior = HashTable[Value].tail;
	  HashTable[Value].tail = name;
       }
       NWUnlockNameHash(volume);
       return 0;
    }
    NWUnlockNameHash(volume);
    return -1;
}

ULONG RemoveNameHash(VOLUME *volume, HASH *name)
{
    register ULONG Value;
    register HASH_LIST *HashTable;

    Value = NWFSStringHash(name->Name, name->NameLength,
			   volume->VolumeNameHashLimit[name->NameSpace]);
    if (Value == (ULONG) -1)
       return -1;

    NWLockNameHash(volume);
    HashTable = (HASH_LIST *) volume->VolumeNameHash[name->NameSpace & 0xF];
    if (HashTable)
    {
       if (HashTable[Value].head == name)
       {
	  HashTable[Value].head = name->next;
	  if (HashTable[Value].head)
	     HashTable[Value].head->prior = NULL;
	  else
	     HashTable[Value].tail = NULL;
       }
       else
       {
	  name->prior->next = name->next;
	  if (name != HashTable[Value].tail)
	     name->next->prior = name->prior;
	  else
	     HashTable[Value].tail = name->prior;
       }
       NWUnlockNameHash(volume);
       return 0;
    }
    NWUnlockNameHash(volume);
    return -1;
}

//
//
//


ULONG AddToDirectoryHash(VOLUME *volume, HASH *name)
{
    register ULONG Value;
    register HASH_LIST *HashTable;

    NWLockDirectoryHash(volume);
    Value = (name->DirNo & (volume->DirectoryNumberHashLimit - 1));
    HashTable = (HASH_LIST *) volume->DirectoryNumberHash;
    if (HashTable)
    {
       if (!HashTable[Value].head)
       {
	  HashTable[Value].head = name;
	  HashTable[Value].tail = name;
	  name->dnext = name->dprior = 0;
       }
       else
       {
	  HashTable[Value].tail->dnext = name;
	  name->dnext = 0;
	  name->dprior = HashTable[Value].tail;
	  HashTable[Value].tail = name;
       }
       NWUnlockDirectoryHash(volume);
       return 0;
    }
    NWUnlockDirectoryHash(volume);
    return -1;
}

ULONG RemoveDirectoryHash(VOLUME *volume, HASH *name)
{
    register ULONG Value;
    register HASH_LIST *HashTable;

    NWLockDirectoryHash(volume);
    Value = (name->DirNo & (volume->DirectoryNumberHashLimit - 1));
    HashTable = (HASH_LIST *) volume->DirectoryNumberHash;
    if (HashTable)
    {
       if (HashTable[Value].head == name)
       {
	  HashTable[Value].head = name->dnext;
	  if (HashTable[Value].head)
	     HashTable[Value].head->dprior = NULL;
	  else
	     HashTable[Value].tail = NULL;
       }
       else
       {
	  name->dprior->dnext = name->dnext;
	  if (name != HashTable[Value].tail)
	     name->dnext->dprior = name->dprior;
	  else
	     HashTable[Value].tail = name->dprior;
       }
       NWUnlockDirectoryHash(volume);
       return 0;
    }
    NWUnlockDirectoryHash(volume);
    return -1;
}

ULONG AddTrusteeToDirectoryHash(VOLUME *volume, THASH *name)
{
    register ULONG Value;
    register THASH_LIST *HashTable;

    NWLockDirectoryHash(volume);
    Value = (name->DirNo & (volume->DirectoryNumberHashLimit - 1));
    HashTable = (THASH_LIST *) volume->DirectoryNumberHash;
    if (HashTable)
    {
       if (!HashTable[Value].head)
       {
	  HashTable[Value].head = name;
	  HashTable[Value].tail = name;
	  name->dnext = name->dprior = 0;
       }
       else
       {
	  HashTable[Value].tail->dnext = name;
	  name->dnext = 0;
	  name->dprior = HashTable[Value].tail;
	  HashTable[Value].tail = name;
       }
       NWUnlockDirectoryHash(volume);
       return 0;
    }
    NWUnlockDirectoryHash(volume);
    return -1;
}

ULONG RemoveTrusteeFromDirectoryHash(VOLUME *volume, THASH *name)
{
    register ULONG Value;
    register THASH_LIST *HashTable;

    NWLockDirectoryHash(volume);
    Value = (name->DirNo & (volume->DirectoryNumberHashLimit - 1));
    HashTable = (THASH_LIST *) volume->DirectoryNumberHash;
    if (HashTable)
    {
       if (HashTable[Value].head == name)
       {
	  HashTable[Value].head = name->dnext;
	  if (HashTable[Value].head)
	     HashTable[Value].head->dprior = NULL;
	  else
	     HashTable[Value].tail = NULL;
       }
       else
       {
	  name->dprior->dnext = name->dnext;
	  if (name != HashTable[Value].tail)
	     name->dnext->dprior = name->dprior;
	  else
	     HashTable[Value].tail = name->dprior;
       }
       NWUnlockDirectoryHash(volume);
       return 0;
    }
    NWUnlockDirectoryHash(volume);
    return -1;
}

ULONG AddUserQuotaToDirectoryHash(VOLUME *volume, UHASH *name)
{
    register ULONG Value;
    register UHASH_LIST *HashTable;

    NWLockDirectoryHash(volume);
    Value = (name->DirNo & (volume->DirectoryNumberHashLimit - 1));
    HashTable = (UHASH_LIST *) volume->DirectoryNumberHash;
    if (HashTable)
    {
       if (!HashTable[Value].head)
       {
	  HashTable[Value].head = name;
	  HashTable[Value].tail = name;
	  name->dnext = name->dprior = 0;
       }
       else
       {
	  HashTable[Value].tail->dnext = name;
	  name->dnext = 0;
	  name->dprior = HashTable[Value].tail;
	  HashTable[Value].tail = name;
       }
       NWUnlockDirectoryHash(volume);
       return 0;
    }
    NWUnlockDirectoryHash(volume);
    return -1;
}

ULONG RemoveUserQuotaFromDirectoryHash(VOLUME *volume, UHASH *name)
{
    register ULONG Value;
    register UHASH_LIST *HashTable;

    NWLockDirectoryHash(volume);
    Value = (name->DirNo & (volume->DirectoryNumberHashLimit - 1));
    HashTable = (UHASH_LIST *) volume->DirectoryNumberHash;
    if (HashTable)
    {
       if (HashTable[Value].head == name)
       {
	  HashTable[Value].head = name->dnext;
	  if (HashTable[Value].head)
	     HashTable[Value].head->dprior = NULL;
	  else
	     HashTable[Value].tail = NULL;
       }
       else
       {
	  name->dprior->dnext = name->dnext;
	  if (name != HashTable[Value].tail)
	     name->dnext->dprior = name->dprior;
	  else
	     HashTable[Value].tail = name->dprior;
       }
       NWUnlockDirectoryHash(volume);
       return 0;
    }
    NWUnlockDirectoryHash(volume);
    return -1;
}

ULONG AddToMountHash(VOLUME *volume, HASH *name)
{
    if (name->Parent == (ULONG) -1)
       return -1;

    if (name->NameSpace == DOS_NAME_SPACE)
    {
       NWLockParentHash(volume);
       if (!volume->HashMountHead)
       {
	  volume->HashMountHead = name;
	  volume->HashMountTail = name;
	  name->pnext = name->pprior = 0;
       }
       else
       {
	  volume->HashMountTail->pnext = name;
	  name->pnext = 0;
	  name->pprior = volume->HashMountTail;
	  volume->HashMountTail = name;
       }
       NWUnlockParentHash(volume);
       return 0;
    }
    return -1;
}

ULONG RemoveMountHash(VOLUME *volume, HASH *name)
{
    if (name->Parent == (ULONG) -1)
       return -1;

    if (name->NameSpace == DOS_NAME_SPACE)
    {
       NWLockParentHash(volume);
       if (volume->HashMountHead == name)
       {
	  volume->HashMountHead = name->pnext;
	  if (volume->HashMountHead)
	     volume->HashMountHead->pprior = NULL;
	  else
	     volume->HashMountTail = NULL;
       }
       else
       {
	  name->pprior->pnext = name->pnext;
	  if (name != volume->HashMountTail)
	     name->pnext->pprior = name->pprior;
	  else
	     volume->HashMountTail = name->pprior;
       }
       NWUnlockParentHash(volume);
       return 0;
    }
    return -1;
}

ULONG AddToDelMountHash(VOLUME *volume, HASH *name)
{
    if (name->Parent == (ULONG) -1)
       return -1;

    NWLockParentHash(volume);
    if (!volume->DelHashMountHead)
    {
       volume->DelHashMountHead = name;
       volume->DelHashMountTail = name;
       name->pnext = name->pprior = 0;
    }
    else
    {
       volume->DelHashMountTail->pnext = name;
       name->pnext = 0;
       name->pprior = volume->DelHashMountTail;
       volume->DelHashMountTail = name;
    }
    NWUnlockParentHash(volume);
    return 0;
}

ULONG RemoveDelMountHash(VOLUME *volume, HASH *name)
{
    if (name->Parent == (ULONG) -1)
       return -1;

    NWLockParentHash(volume);
    if (volume->DelHashMountHead == name)
    {
       volume->DelHashMountHead = name->pnext;
       if (volume->DelHashMountHead)
	  volume->DelHashMountHead->pprior = NULL;
       else
	  volume->DelHashMountTail = NULL;
    }
    else
    {
       name->pprior->pnext = name->pnext;
       if (name != volume->DelHashMountTail)
	  name->pnext->pprior = name->pprior;
       else
	  volume->DelHashMountTail = name->pprior;
    }
    NWUnlockParentHash(volume);
    return 0;
}

//
//
//

ULONG AddToTrusteeHash(VOLUME *volume, THASH *name)
{
    register ULONG Value;
    register THASH_LIST *HashTable;

    NWLockTrusteeHash(volume);
    Value = (name->Parent & (volume->TrusteeHashLimit - 1));
    HashTable = (THASH_LIST *) volume->TrusteeHash;
    if (HashTable)
    {
       if (!HashTable[Value].head)
       {
	  HashTable[Value].head = name;
	  HashTable[Value].tail = name;
	  name->next = name->prior = 0;
       }
       else
       {
	  HashTable[Value].tail->next = name;
	  name->next = 0;
	  name->prior = HashTable[Value].tail;
	  HashTable[Value].tail = name;
       }
       NWUnlockTrusteeHash(volume);
       return 0;
    }
    NWUnlockTrusteeHash(volume);
    return -1;
}

ULONG RemoveTrusteeHash(VOLUME *volume, THASH *name)
{
    register ULONG Value;
    register THASH_LIST *HashTable;

    NWLockTrusteeHash(volume);
    Value = (name->Parent & (volume->TrusteeHashLimit - 1));
    HashTable = (THASH_LIST *) volume->TrusteeHash;
    if (HashTable)
    {
       if (HashTable[Value].head == name)
       {
	  HashTable[Value].head = name->next;
	  if (HashTable[Value].head)
	     HashTable[Value].head->prior = NULL;
	  else
	     HashTable[Value].tail = NULL;
       }
       else
       {
	  name->prior->next = name->next;
	  if (name != HashTable[Value].tail)
	     name->next->prior = name->prior;
	  else
	     HashTable[Value].tail = name->prior;
       }
       NWUnlockTrusteeHash(volume);
       return 0;
    }
    NWUnlockTrusteeHash(volume);
    return -1;
}

//
//
//

ULONG AddToUserQuotaHash(VOLUME *volume, UHASH *name)
{
    register ULONG Value;
    register UHASH_LIST *HashTable;

    NWLockQuotaHash(volume);
    Value = (name->Parent & (volume->UserQuotaHashLimit - 1));
    HashTable = (UHASH_LIST *) volume->UserQuotaHash;
    if (HashTable)
    {
       if (!HashTable[Value].head)
       {
	  HashTable[Value].head = name;
	  HashTable[Value].tail = name;
	  name->next = name->prior = 0;
       }
       else
       {
	  HashTable[Value].tail->next = name;
	  name->next = 0;
	  name->prior = HashTable[Value].tail;
	  HashTable[Value].tail = name;
       }
       NWUnlockQuotaHash(volume);
       return 0;
    }
    NWUnlockQuotaHash(volume);
    return -1;
}

ULONG RemoveUserQuotaHash(VOLUME *volume, UHASH *name)
{
    register ULONG Value;
    register UHASH_LIST *HashTable;

    NWLockQuotaHash(volume);
    Value = (name->Parent & (volume->UserQuotaHashLimit - 1));
    HashTable = (UHASH_LIST *) volume->UserQuotaHash;
    if (HashTable)
    {
       if (HashTable[Value].head == name)
       {
	  HashTable[Value].head = name->next;
	  if (HashTable[Value].head)
	     HashTable[Value].head->prior = NULL;
	  else
	     HashTable[Value].tail = NULL;
       }
       else
       {
	  name->prior->next = name->next;
	  if (name != HashTable[Value].tail)
	     name->next->prior = name->prior;
	  else
	     HashTable[Value].tail = name->prior;
       }
       NWUnlockQuotaHash(volume);
       return 0;
    }
    NWUnlockQuotaHash(volume);
    return -1;
}

//
//
//

ULONG AddToExtHash(VOLUME *volume, EXTENDED_DIR_HASH *name)
{
    register ULONG Value;
    register EXT_HASH_LIST *HashTable;

    NWLockExtHash(volume);

    Value = (name->Parent & (volume->ExtDirHashLimit - 1));
    HashTable = (EXT_HASH_LIST *) volume->ExtDirHash;
    if (HashTable)
    {
       if (!HashTable[Value].head)
       {
	  HashTable[Value].head = name;
	  HashTable[Value].tail = name;
	  name->next = name->prior = 0;
       }
       else
       {
	  HashTable[Value].tail->next = name;
	  name->next = 0;
	  name->prior = HashTable[Value].tail;
	  HashTable[Value].tail = name;
       }
       NWUnlockExtHash(volume);
       return 0;
    }
    NWUnlockExtHash(volume);
    return -1;
}

ULONG RemoveExtHash(VOLUME *volume, EXTENDED_DIR_HASH *name)
{
    register ULONG Value;
    register EXT_HASH_LIST *HashTable;

    NWLockExtHash(volume);
    Value = (name->Parent & (volume->ExtDirHashLimit - 1));
    HashTable = (EXT_HASH_LIST *) volume->ExtDirHash;
    if (HashTable)
    {
       if (HashTable[Value].head == name)
       {
	  HashTable[Value].head = name->next;
	  if (HashTable[Value].head)
	     HashTable[Value].head->prior = NULL;
	  else
	     HashTable[Value].tail = NULL;
       }
       else
       {
	  name->prior->next = name->next;
	  if (name != HashTable[Value].tail)
	     name->next->prior = name->prior;
	  else
	     HashTable[Value].tail = name->prior;
       }
       NWUnlockExtHash(volume);
       return 0;
    }
    NWUnlockExtHash(volume);
    return -1;
}

//
//
//

ULONG AddToParentHash(VOLUME *volume, HASH *name)
{
    register ULONG Value;
    register HASH *dirHash;
    register HASH_LIST *HashTable;

#if (WINDOWS_NT_RO)
    register ULONG slot;
    register HASH *search;
#endif

    if (name->Parent == (ULONG) -1)
       return -1;

    if (name->NameSpace == DOS_NAME_SPACE)
    {
       NWLockParentHash(volume);
       switch (name->Parent)
       {
	  case DELETED_DIRECTORY:
             if (volume->DeletedDirNo)
	        dirHash = GetHashFromDirectoryNumber(volume, 
				        volume->DeletedDirNo);
	     else
     	        dirHash = 0;
	     break;

	  case ROOT_NODE:
	     dirHash = GetHashFromDirectoryNumber(volume, 0);
	     break;

	  default:
	     dirHash = GetHashFromDirectoryNumber(volume, name->Parent);
	     if (!dirHash)
	     {
		NWFSPrint("nwfs:  orphaned file (%X) detected parent-%X (PH)\n",
		    (unsigned int)name->DirNo,
		    (unsigned int)name->Parent);
	     }
	     break;
       }

       Value = (name->Parent & (volume->ParentHashLimit - 1));
       HashTable = (HASH_LIST *) volume->ParentHash;
       if (HashTable)
       {
          HASH *old, *p;

          if (!HashTable[Value].tail)
	  {
	     HashTable[Value].head = name;
	     HashTable[Value].tail = name;
	     name->pnext = name->pprior = 0;
	  }
          else
	  {
             p = HashTable[Value].head;
             old = NULL;
             while (p)
             {
                if ((p->Parent != name->Parent) || (p->DirNo < name->DirNo))
                {
	           old = p;
	           p = p->pnext;
                }
                else
                {
	           if (p->pprior)
	           {
	              p->pprior->pnext = name;
	              name->pnext = p;
	              name->pprior = p->pprior;
	              p->pprior = name;
                      goto parent_inserted;
	           }
	           name->pnext = p;
	           name->pprior = NULL;
	           p->pprior = name;
	           HashTable[Value].head = name;
                   goto parent_inserted;
                }
             }
             old->pnext = name;
             name->pnext = NULL;
             name->pprior = old;
             HashTable[Value].tail = name;
	  }

parent_inserted:;
	  

#if (WINDOWS_NT_RO)
	  // enumerations are 2 relative
	  slot = 2;
	  search = name;
	  while (search)
	  {
	     search = search->pprior;
	     if (search && search->Parent == name->Parent)
		slot++;
	  }
	  name->SlotNumber = slot;
#endif
	  if (dirHash)
	     dirHash->Blocks++;

	  NWUnlockParentHash(volume);
	  return 0;
       }
       NWUnlockParentHash(volume);
    }
    return -1;
}

ULONG RemoveParentHash(VOLUME *volume, HASH *name)
{
    register ULONG Value;
    register HASH *dirHash;
    register HASH_LIST *HashTable;

    if (name->Parent == (ULONG) -1)
       return -1;

    if (name->NameSpace == DOS_NAME_SPACE)
    {
       NWLockParentHash(volume);
       Value = (name->Parent & (volume->ParentHashLimit - 1));
       HashTable = (HASH_LIST *) volume->ParentHash;
       if (HashTable)
       {
	  if (HashTable[Value].head == name)
	  {
	     HashTable[Value].head = name->pnext;
	     if (HashTable[Value].head)
		HashTable[Value].head->pprior = NULL;
	     else
		HashTable[Value].tail = NULL;
	  }
	  else
	  {
	     name->pprior->pnext = name->pnext;
	     if (name != HashTable[Value].tail)
		name->pnext->pprior = name->pprior;
	     else
		HashTable[Value].tail = name->pprior;
	  }

	  if (name->Parent == (ULONG)ROOT_NODE)
	     dirHash = GetHashFromDirectoryNumber(volume, 0);
	  else
	  if ((name->Parent == DELETED_DIRECTORY) && (volume->DeletedDirNo))
	     dirHash = GetHashFromDirectoryNumber(volume, 
			                          volume->DeletedDirNo);
	  else
	     dirHash = GetHashFromDirectoryNumber(volume, name->Parent);

	  if (dirHash)
	     dirHash->Blocks--;
	  
	  NWUnlockParentHash(volume);
	  return 0;
       }
       NWUnlockParentHash(volume);
    }
    return -1;
}

