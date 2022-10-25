
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
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWHASH.H
*   DESCRIP  :   Netware Partition Module
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#ifndef _NWFS_HASH_
#define _NWFS_HASH_

// file data states for read ahead and suballocation
#define NWFS_UNMODIFIED   0x00
#define NWFS_MODIFIED     0x01
#define NWFS_SEQUENTIAL   0x02


#if (LINUX | WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)

typedef struct _RAPID_FAT {
   struct _RAPID_FAT *next;
   struct _RAPID_FAT *prior;
   ULONG BlockNo;
   ULONG *IndexTable;
   ULONG TableSize;
} RAPID_FAT;

typedef struct _FILE_CONTROL_BLOCK
{
   RAPID_FAT *head;
   RAPID_FAT *tail;

#if (LINUX_SLEEP)
   struct semaphore Semaphore;
#endif

} FCB;

typedef struct _HASH
{
   struct _HASH *next;
   struct _HASH *prior;
   struct _HASH *dnext;
   struct _HASH *dprior;
   struct _HASH *pnext;
   struct _HASH *pprior;
   struct _HASH *nlnext;
   struct _HASH *nlprior;
   struct _HASH *nlroot;

   ULONG Parent;
   ULONG Root;
   ULONG DirNo;
   ULONG NameLink;

   ULONG Signature;
   ULONG NextTrustee;
   ULONG Blocks;
   ULONG Flags;

#if (HASH_FAT_CHAINS)
   ULONG FirstBlock;
   ULONG FileSize;
   ULONG FileAttributes;
#endif
   
#if (LINUX_SLEEP)
   struct semaphore Semaphore;
#endif

   BYTE State;
   ULONG TurboFATCluster;
   ULONG TurboFATIndex;
   BYTE NameSpace;
   WORD NameLength;
   BYTE *Name;
} HASH;

typedef struct _THASH
{
   struct _THASH *next;
   struct _THASH *prior;
   struct _THASH *dnext;
   struct _THASH *dprior;
   struct _THASH *pnext;
   struct _THASH *pprior;
   struct _THASH *nlnext;
   struct _THASH *nlprior;
   struct _THASH *trustee_list;

   ULONG Parent;
   ULONG DirNo;

   ULONG TrusteeCount;
   ULONG NextTrustee;
   ULONG FileEntryNumber;
   ULONG Flags;
   ULONG Attributes;
} THASH;

typedef struct _UHASH
{
   struct _UHASH *next;
   struct _UHASH *prior;
   struct _UHASH *dnext;
   struct _UHASH *dprior;
   struct _UHASH *pnext;
   struct _UHASH *pprior;
   struct _UHASH *nlnext;
   struct _UHASH *nlprior;

   ULONG Parent;
   ULONG DirNo;

   ULONG TrusteeCount;
} UHASH;

typedef struct _HASH_LIST {
   HASH *head;
   HASH *tail;
} HASH_LIST;

typedef struct _THASH_LIST {
   THASH *head;
   THASH *tail;
} THASH_LIST;

typedef struct _UHASH_LIST {
   UHASH *head;
   UHASH *tail;
} UHASH_LIST;

typedef struct _EXTENDED_DIR_HASH
{
    struct _EXTENDED_DIR_HASH *next;
    struct _EXTENDED_DIR_HASH *prior;
    ULONG Signature;
    ULONG Length;
    ULONG ExtDirNo;
    ULONG Parent;
    ULONG Root;
    BYTE NameSpace;
    BYTE Flags;
    BYTE ControlFlags;
    BYTE NameLength;
    BYTE Name[1];
} EXTENDED_DIR_HASH;

typedef struct _EXT_HASH_LIST {
   EXTENDED_DIR_HASH *head;
   EXTENDED_DIR_HASH *tail;
} EXT_HASH_LIST;

typedef union _HASH_UNION
{
   UHASH UHash;
   THASH THash;
   HASH  Hash;
} HASH_UNION;

#endif


#if (WINDOWS_NT | WINDOWS_NT_RO)

typedef struct _HASH
{
   struct _HASH *next;
   struct _HASH *prior;
   struct _HASH *dnext;
   struct _HASH *dprior;
   struct _HASH *pnext;
   struct _HASH *pprior;
   struct _HASH *nlnext;
   struct _HASH *nlprior;
   struct _HASH *nlroot;
   struct _HASH *trustee_list;

   ULONG Parent;
   ULONG DirNo;
   ULONG Root;
   ULONG NameLink;
   ULONG NextTrustee;
   ULONG Blocks;
   ULONG FileSize;
   ULONG FileAttributes;
   ULONG Flags;
   ULONG CreateDateAndTime;
   ULONG UpdatedDateAndTime;
   ULONG FirstBlock;
   ULONG Signature;
   ULONG Instance;
   PVOID FSContext;
   ULONG SlotNumber;

   WORD AccessedDate;
   BYTE State;
   BYTE NameSpace;
   WORD NameLength;
   BYTE *Name;
} HASH;

typedef struct _THASH
{
   struct _THASH *next;
   struct _THASH *prior;
   struct _THASH *dnext;
   struct _THASH *dprior;
   struct _THASH *pnext;
   struct _THASH *pprior;
   struct _THASH *nlnext;
   struct _THASH *nlprior;
   struct _THASH *trustee_list;

   ULONG Parent;
   ULONG DirNo;

   ULONG TrusteeCount;
   ULONG NextTrustee;
   ULONG FileEntryNumber;
   ULONG Flags;
   ULONG Attributes;
} THASH;

typedef struct _UHASH
{
   struct _UHASH *next;
   struct _UHASH *prior;
   struct _UHASH *dnext;
   struct _UHASH *dprior;
   struct _UHASH *pnext;
   struct _UHASH *pprior;
   struct _UHASH *nlnext;
   struct _UHASH *nlprior;
   ULONG Parent;
   ULONG DirNo;

   ULONG TrusteeCount;
} UHASH;

typedef struct _HASH_LIST {
   HASH *head;
   HASH *tail;
} HASH_LIST;

typedef struct _THASH_LIST {
   THASH *head;
   THASH *tail;
} THASH_LIST;

typedef struct _UHASH_LIST {
   UHASH *head;
   UHASH *tail;
} UHASH_LIST;

typedef struct _EXTENDED_DIR_HASH
{
    struct _EXTENDED_DIR_HASH *next;
    struct _EXTENDED_DIR_HASH *prior;
    ULONG Signature;
    ULONG Length;
    ULONG ExtDirNo;
    ULONG Parent;
    ULONG Root;
    BYTE NameSpace;
    BYTE Flags;
    BYTE ControlFlags;
    BYTE NameLength;
    BYTE Name[1];
} EXTENDED_DIR_HASH;

typedef struct _EXT_HASH_LIST {
   EXTENDED_DIR_HASH *head;
   EXTENDED_DIR_HASH *tail;
} EXT_HASH_LIST;

typedef union _HASH_UNION
{
   UHASH UHash;
   THASH THash;
   HASH  Hash;
} HASH_UNION;

#endif

typedef struct _DIR_BLOCK_HASH
{
   struct _DIR_BLOCK_HASH *next;
   struct _DIR_BLOCK_HASH *prior;
   struct _DIR_BLOCK_HASH *dnext;
   struct _DIR_BLOCK_HASH *dprior;
   ULONG Cluster1;
   ULONG Cluster2;
   ULONG BlockNo;
   ULONG DelBlockNo;
} DIR_BLOCK_HASH;

typedef struct _DIR_BLOCK_HASH_LIST
{
   DIR_BLOCK_HASH *head;
   DIR_BLOCK_HASH *tail;
} DIR_BLOCK_HASH_LIST;

typedef struct _DIR_ASSIGN_HASH
{
   struct _DIR_ASSIGN_HASH *next;
   struct _DIR_ASSIGN_HASH *prior;
   struct _DIR_ASSIGN_HASH *dnext;
   struct _DIR_ASSIGN_HASH *dprior;
   struct _DIR_ASSIGN_HASH *hnext;
   struct _DIR_ASSIGN_HASH *hprior;
   ULONG BlockNo;
   ULONG DirOwner;
   ULONG DirBlockNo;
   BYTE FreeList[((IO_BLOCK_SIZE / 128) / 8)];
} DIR_ASSIGN_HASH;

typedef struct _DIR_ASSIGN_HASH_LIST
{
   DIR_ASSIGN_HASH *head;
   DIR_ASSIGN_HASH *tail;
} DIR_ASSIGN_HASH_LIST;

#define DEFAULT_HASH_SIZE        1024

#define NUMBER_OF_NAME_HASH_ENTRIES     DEFAULT_HASH_SIZE
#define VOLUME_NAME_HASH_SIZE    NUMBER_OF_NAME_HASH_ENTRIES * sizeof(HASH_LIST)

#define NUMBER_OF_DIR_NUMBER_ENTRIES    DEFAULT_HASH_SIZE
#define DIR_NUMBER_HASH_SIZE     NUMBER_OF_DIR_NUMBER_ENTRIES * sizeof(HASH_LIST)

#define NUMBER_OF_BLOCK_HASH_ENTRIES    DEFAULT_HASH_SIZE
#define BLOCK_NUMBER_HASH_SIZE   NUMBER_OF_BLOCK_HASH_ENTRIES * sizeof(HASH_LIST)

#define NUMBER_OF_LRU_HASH_ENTRIES    DEFAULT_HASH_SIZE
#define BLOCK_LRU_HASH_SIZE   NUMBER_OF_LRU_HASH_ENTRIES * sizeof(HASH_LIST)

#define NUMBER_OF_PARENT_ENTRIES        DEFAULT_HASH_SIZE
#define PARENT_HASH_SIZE         NUMBER_OF_PARENT_ENTRIES * sizeof(HASH_LIST)

#define NUMBER_OF_TRUSTEE_ENTRIES       DEFAULT_HASH_SIZE
#define TRUSTEE_HASH_SIZE        NUMBER_OF_TRUSTEE_ENTRIES * sizeof(HASH_LIST)

#define NUMBER_OF_QUOTA_ENTRIES         DEFAULT_HASH_SIZE
#define USER_QUOTA_HASH_SIZE     NUMBER_OF_QUOTA_ENTRIES * sizeof(HASH_LIST)

#define NUMBER_OF_EXT_HASH_ENTRIES      DEFAULT_HASH_SIZE
#define EXT_HASH_SIZE            NUMBER_OF_EXT_HASH_ENTRIES * sizeof(EXT_HASH_LIST)

#define NUMBER_OF_ASSIGN_BLOCK_ENTRIES  DEFAULT_HASH_SIZE
#define ASSIGN_BLOCK_HASH_SIZE   NUMBER_OF_ASSIGN_BLOCK_ENTRIES * sizeof(DIR_ASSIGN_HASH_LIST)

#define NUMBER_OF_DIR_BLOCK_ENTRIES     DEFAULT_HASH_SIZE
#define DIR_BLOCK_HASH_SIZE      NUMBER_OF_DIR_BLOCK_ENTRIES * sizeof(DIR_BLOCK_HASH_LIST)

#endif
