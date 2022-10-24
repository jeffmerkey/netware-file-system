
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
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWDIR.H
*   DESCRIP  :   On-Disk Directory Structures
*   DATE     :  November 1, 1998
*
*
***************************************************************************/


#ifndef _NWFS_NWDIR_
#define _NWFS_NWDIR_

/**********************************************************************
*
*	 Directory Control Flags
*
***********************************************************************/

#define FREE_NODE                   -1
#define TRUSTEE_NODE                -2
#define ROOT_NODE                   -3
#define RESTRICTION_NODE            -4
#define SUBALLOC_NODE               -5

// deleted files belong to this directory 
#define DELETED_DIRECTORY           -2

/**********************************************************************
*
*	 Extended Directory Signatures
*
*  NOTE:  These values are documented in the SDK/HDK and were
*         verified by debugging a live NetWare server File System.
*
***********************************************************************/

//  NOTE:  I haven't implemented all of the EA record types for the
//  extended directory (but I do read and cache them into the ext
//  directoty list) and I'm not certain they make much sense for
//  linux at present, however, if users had stored OS2 files on
//  a Netware server, then linux at least can provide the data, even
//  though the EA way of doing things is probably obsolete these days.

#define EXTENDED_DIR_STAMP	          0x13579ACE // "black box" EA
#define LONG_EXTENDED_DIR_STAMP           0xABA12190 // holds extra name
#define NT_EXTENDED_DIR_STAMP             0x5346544E // holds extra name
#define NFS_EXTENDED_DIR_STAMP	          0x554E4958 // holds extra name
#define MIGRATE_DIR_STAMP                 0xAFDE5713 // points migrated file
#define	TALLY_EXTENDED_DIR_STAMP          0x2E4E414A // "black box" EA
#define EXTENDED_ATTRIBUTE_DIR_STAMP      0x2E6E616A // "?????" OS2 specific

/**********************************************************************
*
*	 Name Space Flag Settings
*
***********************************************************************/

#define DOS_NAME_SPACE			0
#define MAC_NAME_SPACE			1
#define UNIX_NAME_SPACE 	        2
#define FTAM_NAME_SPACE		        3
#define LONG_NAME_SPACE 		4
#define NT_NAME_SPACE			5

/**********************************************************************
*
*	 File Attribute Flag Settings
*
*  NOTE:  Novell documented these flags in the SDK/HDK Kits for NetWare
*         4.11 and 5.x.
*
***********************************************************************/

#define READ_ONLY				0x00000001
#define	HIDDEN			        	0x00000002
#define	SYSTEM			           	0x00000004
#define	EXECUTE			        	0x00000008
#define	SUBDIRECTORY	         		0x00000010
#define	ARCHIVE			        	0x00000020

// warning!!! this attribute seems to mean something different depending
// on whether you are 3.x or 4.x.
#define	SHAREABLE				0x00000080
#define DIR_HIDDEN				0x00000080

#define	SHARE_MODES				0x00000700
#define	NO_SUBALLOC				0x00000800
#define	TRANSACTION		  		0x00001000
#define OLD_INDEXED				0x00002000
#define	READ_AUDIT				0x00004000
#define	WRITE_AUDIT		  		0x00008000
#define IMMEDIATE_PURGE	  	        	0x00010000
#define RENAME_INHIBIT		        	0x00020000
#define DELETE_INHIBIT		        	0x00040000
#define COPY_INHIBIT		        	0x00080000
#define FILE_AUDITING		        	0x00100000
#define CDS_CONTAINER		        	0x00200000
#define REMOTE_DATA_ACCESS	       		0x00400000
#define REMOTE_DATA_INHIBIT			0x00800000
#define REMOTE_DATA_SAVE_KEY	         	0x01000000
#define COMPRESS_FILE_IMMEDIATELY	        0x02000000
#define DATA_STREAM_IS_COMPRESSED	        0x04000000
#define DO_NOT_COMPRESS_FILE	         	0x08000000
#define CANT_COMPRESS_DATA_STREAM	        0x20000000
#define ARCHIVE_FILE		        	0x40000000

/**********************************************************************
*
*         Directory Entry Flag Settings
*
***********************************************************************/

//
//  Netware provides several useful bit fields for storing Unix Style
//  semantics, such as hardlinks, softlinks, and symbolic links.
//  Some entries are ambiguous, but Netware also allows special
//  "Phantom" files as well as deleted files.  Note that Netware uses
//  a different bit to distinguish between 4.x and 3.x deleted files
//  (which seems broken to me).  We make all Phantom files visible
//  in the current implementation.
//

#define NW3_DELETED_FILE	               0x001
#define PHANTOM_FILE 			       0x002
#define SUBDIRECTORY_FILE	               0x004
#define NAMED_FILE             	               0x008
#define PRIMARY_NAMESPACE		       0x010
#define NW4_DELETED_FILE         	       0x020
#define HARD_LINKED_FILE	               0x040
#define SYMBOLIC_LINKED_FILE		       0x080
#define RAW_FILE                               0x100

/**********************************************************************
*
*           Volume Segment Flag Settings
*
***********************************************************************/

// Volume Table Flags Settings

// bit that tells you wether this is a SYS volume or not
#define SYSTEM_VOLUME		                0x01
#define	READ_ONLY_VOLUME                        0x80

//  Root Directory Volume Flags values (root->VolumeFlags)

#define AUDITING_ON	                     	0x01
#define SUB_ALLOCATION_ON           	        0x02
#define FILE_COMPRESSION_ON         	        0x04
#define DATA_MIGRATION_ON           	        0x08

#define NEW_TRUSTEE_COUNT	        	0x10  // 3.x/4.x Flag
// For 3.x we assume six trustees per record.  for 4.x and above the number
// is usually four.  These records are opaque to  (we do not
// need them).  At some future date, we may implement trustees for other
// operating systems than NetWare.

#define NDS_FLAG	                        0x20  // 3.x/4.x Flag
// In theory, this identifies a volume as hosting an NDS (NetWare
// Directory Service) database.  We use the Directory Services Object ID
// as the volume serial number in Microsoft-based file systems.  Unix
// has no parallel at present.

#define VFLAGS_3X_MASK (AUDITING_ON | SUB_ALLOCATION_ON | FILE_COMPRESSION_ON |                         DATA_MIGRATION_ON)

#define VFLAGS_4X_MASK (AUDITING_ON | SUB_ALLOCATION_ON | FILE_COMPRESSION_ON |	                        DATA_MIGRATION_ON | NEW_TRUSTEE_COUNT | NDS_FLAG)

/**********************************************************************
*
*          Trustee Flag Settings
*
***********************************************************************/

// Netware trustee security bits

#define	TrusteeReadFile			        0x0001
#define	TrusteeWriteFile			0x0002
#define TrusteeOpenFile			        0x0004
#define	TrusteeCreateFile			0x0008
#define	TrusteeDeleteFile			0x0010
#define	TrusteeAccessControl			0x0020
#define	TrusteeScanFile				0x0040
#define	TrusteeModifyFile			0x0080
#define TrusteeSupervisor			0x0100

#define TRUSTEE_VALID_MASK                      0x01FF

#define MAX_DOS_NAME   12
#define DOS_FILENAME    8
#define DOS_EXTENSION   3
#define MAX_MAC_NAME   31  // should be 32, bug in Netware we must reproduce

// This is the general purpose id for the NetWare Supervisor Account.
#define  SUPERVISOR    0x01000000

// allow extended directory to create ext chains for this
#define MAX_NFS_NAME   255  // limit NFS, LONG, and NT names to 255
#define MAX_LONG_NAME  255  //
#define MAX_NT_NAME    255  //

#if (LINUX | LINUX_UTIL | DOS_UTIL)

//
//  The GNU compiler won't byte pack our on-disk structures unless we
//  specifically tell it to do so.  We tell it to pack structures
//  with the [  __attribute__ ((packed))  ] modifier.
//

typedef struct _ROOT
{
    ULONG  Subdirectory       __attribute__ ((packed));
    ULONG  FileAttributes     __attribute__ ((packed));
    BYTE   UniqueID           __attribute__ ((packed));
    BYTE   Flags              __attribute__ ((packed));
    BYTE   NameSpace          __attribute__ ((packed));
    BYTE   NameSpaceCount     __attribute__ ((packed));

    // Really weird -- Looks as though at a later date someone needed
    // to squeeze extra space out of the root directory entry for an
    // NDS volume stamp.  Since namespace entries are never greater than
    // 12, a nibble turns out to be enough.  In 4.x the name space table
    // is only 6 bytes (12 nibbles/12 possible namespaces).  In 3.x, each
    // namespace takes a single byte instead of a single nibble.  Since
    // NetWare has never defined more than 7 namespaces, the last bytes
    // are still ok to use as volume flags. In 3.x, these last fields
    // always contain zero.

    ULONG  DirectoryServicesObjectID    __attribute__ ((packed));
    BYTE   SupportedNameSpacesNibble[6] __attribute__ ((packed));
    BYTE   DOSType                 __attribute__ ((packed));
    BYTE   VolumeFlags             __attribute__ ((packed));
    ULONG  CreateDateAndTime       __attribute__ ((packed));
    ULONG  OwnerID                 __attribute__ ((packed));
    ULONG  LastArchivedDateAndTime __attribute__ ((packed));
    ULONG  LastArchivedID          __attribute__ ((packed));
    ULONG  LastModifiedDateAndTime __attribute__ ((packed));
    ULONG  NextTrusteeEntry        __attribute__ ((packed));
    ULONG  Trustees[8]             __attribute__ ((packed));
    WORD   TrusteeMask[8]          __attribute__ ((packed));
    ULONG  MaximumSpace            __attribute__ ((packed));
    WORD   MaximumAccessMask       __attribute__ ((packed));
    WORD   SubdirectoryFirstBlockGone  __attribute__ ((packed));
    ULONG  ExtendedDirectoryChain0 __attribute__ ((packed));
    ULONG  ExtendedDirectoryChain1 __attribute__ ((packed));
    ULONG  ExtendedAttributes      __attribute__ ((packed));
    ULONG  ModifyTimeInSeconds     __attribute__ ((packed));
    ULONG  SubAllocationList       __attribute__ ((packed));
    ULONG  NameList                __attribute__ ((packed));
} ROOT;

typedef struct _ROOT3X
{
    ULONG  Subdirectory    __attribute__ ((packed));
    ULONG  FileAttributes  __attribute__ ((packed));
    BYTE   UniqueID        __attribute__ ((packed));
    BYTE   Flags           __attribute__ ((packed));
    BYTE   NameSpace       __attribute__ ((packed));
    BYTE   NameSpaceCount  __attribute__ ((packed));

    // Really weird -- Looks as though at a later date someone needed
    // to squeeze extra space out of the root directory entry for an
    // NDS volume stamp.  Since namespace entries are never greater than
    // 12, a nibble turns out to be enough.  In 4.x the name space table
    // is only 6 bytes (12 nibbles/12 possible namespaces).  In 3.x, each
    // namespace takes a single byte instead of a single nibble.  Since
    // NetWare has never defined more than 7 namespaces, the last bytes
    // are still ok to use as volume flags. In 3.x, these last fields
    // always contain zero.

    BYTE   NameSpaceTable[10]      __attribute__ ((packed));
    BYTE   DOSType                 __attribute__ ((packed));
    BYTE   VolumeFlags             __attribute__ ((packed));
    ULONG  CreateDateAndTime       __attribute__ ((packed));
    ULONG  OwnerID                 __attribute__ ((packed));
    ULONG  LastArchivedDateAndTime __attribute__ ((packed));
    ULONG  LastArchivedID          __attribute__ ((packed));
    ULONG  LastModifiedDateAndTime __attribute__ ((packed));
    ULONG  NextTrusteeEntry        __attribute__ ((packed));
    ULONG  Trustees[8]             __attribute__ ((packed));
    WORD   TrusteeMask[8]          __attribute__ ((packed));
    ULONG  MaximumSpace            __attribute__ ((packed));
    WORD   MaximumAccessMask       __attribute__ ((packed));
    WORD   SubdirectoryFirstBlockGone  __attribute__ ((packed));
    ULONG  ExtendedDirectoryChain0 __attribute__ ((packed));
    ULONG  ExtendedDirectoryChain1 __attribute__ ((packed));
    ULONG  ExtendedAttributes      __attribute__ ((packed));
    ULONG  ModifyTimeInSeconds     __attribute__ ((packed));
    ULONG  SubAllocationList       __attribute__ ((packed));
    ULONG  NameList                __attribute__ ((packed));
} ROOT3X;

typedef struct _DOS
{
    ULONG  Subdirectory       __attribute__ ((packed));
    ULONG  FileAttributes     __attribute__ ((packed));
    BYTE   UniqueID           __attribute__ ((packed));
    BYTE   Flags              __attribute__ ((packed));
    BYTE   NameSpace          __attribute__ ((packed));
    BYTE   FileNameLength     __attribute__ ((packed));
    BYTE   FileName[12]       __attribute__ ((packed));
    ULONG  CreateDateAndTime  __attribute__ ((packed));
    ULONG  OwnerID            __attribute__ ((packed));
    ULONG  LastArchivedDateAndTime __attribute__ ((packed));
    ULONG  LastArchivedID     __attribute__ ((packed));
    ULONG  LastUpdatedDateAndTime __attribute__ ((packed));
    ULONG  LastUpdatedID      __attribute__ ((packed));
    ULONG  FileSize           __attribute__ ((packed));
    ULONG  FirstBlock         __attribute__ ((packed));
    ULONG  NextTrusteeEntry   __attribute__ ((packed));
    ULONG  Trustees[4]        __attribute__ ((packed));
    ULONG  LookUpEntryNumber  __attribute__ ((packed));
    ULONG  LastUpdatedInSeconds __attribute__ ((packed));
    WORD   TrusteeMask[4]     __attribute__ ((packed));
    WORD   ChangeReferenceID  __attribute__ ((packed));
    WORD   Reserved[1]        __attribute__ ((packed));
    WORD   MaximumAccessMask  __attribute__ ((packed));
    WORD   LastAccessedDate   __attribute__ ((packed));
    ULONG  DeletedFileTime    __attribute__ ((packed));
    ULONG  DeletedDateAndTime __attribute__ ((packed));
    ULONG  DeletedID          __attribute__ ((packed));
    ULONG  ExtendedAttributes __attribute__ ((packed));
    ULONG  DeletedBlockSequenceNumber __attribute__ ((packed));
    ULONG  PrimaryEntry       __attribute__ ((packed));
    ULONG  NameList           __attribute__ ((packed));
} DOS;

typedef struct _SUBDIR
{
    ULONG  Subdirectory       __attribute__ ((packed));
    ULONG  FileAttributes     __attribute__ ((packed));
    BYTE   UniqueID           __attribute__ ((packed));
    BYTE   Flags              __attribute__ ((packed));
    BYTE   NameSpace          __attribute__ ((packed));
    BYTE   FileNameLength     __attribute__ ((packed));
    BYTE   FileName[12]       __attribute__ ((packed));
    ULONG  CreateDateAndTime  __attribute__ ((packed));
    ULONG  OwnerID            __attribute__ ((packed));
    ULONG  LastArchivedDateAndTime __attribute__ ((packed));
    ULONG  LastArchivedID     __attribute__ ((packed));
    ULONG  LastModifiedDateAndTime __attribute__ ((packed));
    ULONG  NextTrusteeEntry   __attribute__ ((packed));
    ULONG  Trustees[8]        __attribute__ ((packed));
    WORD   TrusteeMask[8]     __attribute__ ((packed));
    ULONG  MaximumSpace       __attribute__ ((packed));
    WORD   MaximumAccessMask  __attribute__ ((packed));
    WORD   SubdirectoryFirstBlockGone __attribute__ ((packed));
    ULONG  MigrationBindID    __attribute__ ((packed));
    ULONG  Reserved           __attribute__ ((packed));
    ULONG  ExtendedAttributes __attribute__ ((packed));
    ULONG  LastModifiedInSeconds __attribute__ ((packed));
    ULONG  PrimaryEntry       __attribute__ ((packed));
    ULONG  NameList           __attribute__ ((packed));
} SUB_DIRECTORY;

typedef struct _TRUSTEE
{
    ULONG  Flag             __attribute__ ((packed));
    ULONG  Attributes       __attribute__ ((packed));
    BYTE   ID               __attribute__ ((packed));
    BYTE   TrusteeCount     __attribute__ ((packed));
    BYTE   Flags            __attribute__ ((packed));
    BYTE   Reserved1[1]     __attribute__ ((packed));
    ULONG  Subdirectory     __attribute__ ((packed));
    ULONG  NextTrusteeEntry __attribute__ ((packed));
    ULONG  FileEntryNumber  __attribute__ ((packed));
    ULONG  Trustees[16]     __attribute__ ((packed));
    WORD   TrusteeMask[16]  __attribute__ ((packed));
    BYTE   Reserved2[4]     __attribute__ ((packed));
    ULONG  DeletedBlockSequenceNumber __attribute__ ((packed));
} TRUSTEE;

typedef struct _USER
{
    ULONG  Flag            __attribute__ ((packed));
    ULONG  Reserved1       __attribute__ ((packed));
    BYTE   ID              __attribute__ ((packed));
    BYTE   TrusteeCount    __attribute__ ((packed));
    BYTE   Reserved2[2]    __attribute__ ((packed));
    ULONG  Subdirectory    __attribute__ ((packed));
    ULONG  Trustees[14]    __attribute__ ((packed));
    ULONG  Restriction[14] __attribute__ ((packed));
} USER;

typedef struct _SUBALLOC
{
    ULONG  Flag            __attribute__ ((packed));
    ULONG  Reserved1       __attribute__ ((packed));
    BYTE   ID              __attribute__ ((packed));
    BYTE   SequenceNumber  __attribute__ ((packed));
    BYTE   Reserved2[2]    __attribute__ ((packed));
    ULONG  SubAllocationList __attribute__ ((packed));
    ULONG  StartingFATChain[28]  __attribute__ ((packed));
} SUBALLOC;

typedef struct _MACINTOSH
{
    ULONG  Subdirectory      __attribute__ ((packed));
    ULONG  FileAttributes    __attribute__ ((packed));
    BYTE   UniqueID          __attribute__ ((packed));
    BYTE   Flags             __attribute__ ((packed));
    BYTE   NameSpace         __attribute__ ((packed));
    BYTE   FileNameLength    __attribute__ ((packed));
    BYTE   FileName[32]      __attribute__ ((packed));
    ULONG  ResourceFork      __attribute__ ((packed));
    ULONG  ResourceForkSize  __attribute__ ((packed));
    BYTE   FinderInfo[32]    __attribute__ ((packed));
    BYTE   ProDosInfo[6]     __attribute__ ((packed));
    BYTE   DirRightsMask[4]  __attribute__ ((packed));
    BYTE   Reserved0[2]      __attribute__ ((packed));
    ULONG  CreateTime        __attribute__ ((packed));
    ULONG  BackupTime        __attribute__ ((packed));
    BYTE   Reserved1[12]     __attribute__ ((packed));
    ULONG  DeletedBlockSequenceNumber __attribute__ ((packed));
    ULONG  PrimaryEntry      __attribute__ ((packed));
    ULONG  NameList          __attribute__ ((packed));
} MACINTOSH;

#define NFS_HASSTREAM_HARD_LINK      0x01
#define NFS_SYMBOLIC_LINK            0x02
#define NFS_HARD_LINK                0x04

typedef struct _NFS
{
    ULONG  Subdirectory                __attribute__ ((packed));
    ULONG  FileAttributes              __attribute__ ((packed));
    BYTE   UniqueID                    __attribute__ ((packed));
    BYTE   Flags                       __attribute__ ((packed));
    BYTE   NameSpace                   __attribute__ ((packed));
    BYTE   FileNameLength              __attribute__ ((packed));

    BYTE   FileName[40]                __attribute__ ((packed));
    BYTE   TotalFileNameLength         __attribute__ ((packed));
    BYTE   ExtantsUsed                 __attribute__ ((packed));
    ULONG  StartExtantNumber           __attribute__ ((packed));
    ULONG  mode                        __attribute__ ((packed));
    ULONG  flags                       __attribute__ ((packed));
    ULONG  gid                         __attribute__ ((packed));
    ULONG  rdev                        __attribute__ ((packed));
    ULONG  nlinks                      __attribute__ ((packed));
    BYTE   LinkedFlag                  __attribute__ ((packed));
    BYTE   FirstCreated                __attribute__ ((packed));
    ULONG  LinkNextDirNo               __attribute__ ((packed));
    ULONG  LinkEndDirNo                __attribute__ ((packed));
    ULONG  LinkPrevDirNo               __attribute__ ((packed));
    ULONG  uid                         __attribute__ ((packed));
    BYTE   ACSFlags                    __attribute__ ((packed));
    WORD   LastAccessedTime            __attribute__ ((packed));
    BYTE   Reserved[17]                __attribute__ ((packed));
    ULONG  DeletedBlockSequenceNumber  __attribute__ ((packed));
    ULONG  PrimaryEntry                __attribute__ ((packed));
    ULONG  NameList                    __attribute__ ((packed));
} NFS;

typedef struct _LONGNAME
{
    ULONG  Subdirectory       __attribute__ ((packed));
    ULONG  FileAttributes     __attribute__ ((packed));
    BYTE   UniqueID           __attribute__ ((packed));
    BYTE   Flags              __attribute__ ((packed));
    BYTE   NameSpace          __attribute__ ((packed));
    BYTE   FileNameLength     __attribute__ ((packed));
    BYTE   FileName[80]       __attribute__ ((packed));
    ULONG  ExtendedSpace      __attribute__ ((packed));
    BYTE   ExtantsUsed        __attribute__ ((packed));
    BYTE   LengthData         __attribute__ ((packed));
    BYTE   Reserved1[18]      __attribute__ ((packed));
    ULONG  Reserved2          __attribute__ ((packed));
    ULONG  PrimaryEntry       __attribute__ ((packed));
    ULONG  NameList           __attribute__ ((packed));
} LONGNAME;

typedef struct _NTNAME
{
    ULONG  Subdirectory       __attribute__ ((packed));
    ULONG  FileAttributes     __attribute__ ((packed));
    BYTE   UniqueID           __attribute__ ((packed));
    BYTE   Flags              __attribute__ ((packed));
    BYTE   NameSpace          __attribute__ ((packed));
    BYTE   FileNameLength     __attribute__ ((packed));
    BYTE   FileName[80]       __attribute__ ((packed));
    ULONG  ExtendedSpace      __attribute__ ((packed));
    BYTE   ExtantsUsed        __attribute__ ((packed));
    BYTE   LengthData         __attribute__ ((packed));
    WORD   ExtendedFlags      __attribute__ ((packed));
    ULONG  FileIdentifierStamp[2] __attribute__ ((packed));
    ULONG  Reserved1[2]       __attribute__ ((packed));
    ULONG  Reserved2          __attribute__ ((packed));
    ULONG  PrimaryEntry       __attribute__ ((packed));
    ULONG  NameList           __attribute__ ((packed));
} NTNAME;

typedef struct __ROOT
{
    ULONG  Subdirectory       __attribute__ ((packed));
    ULONG  FileAttributes     __attribute__ ((packed));
    BYTE   UniqueID           __attribute__ ((packed));
    BYTE   Flags              __attribute__ ((packed));
    BYTE   NameSpace          __attribute__ ((packed));
    BYTE   FileNameLength     __attribute__ ((packed));
    BYTE   FileName[80]       __attribute__ ((packed));
    ULONG  BlockSize          __attribute__ ((packed));
    ULONG  SplitSize          __attribute__ ((packed));
    ULONG  FenrisNext         __attribute__ ((packed));
    ULONG  FenrisPrior        __attribute__ ((packed));
    ULONG  FenrisParent       __attribute__ ((packed));
    ULONG  FenrisChild        __attribute__ ((packed));
    ULONG  FenrisFlags        __attribute__ ((packed));
    ULONG  PrimaryEntry       __attribute__ ((packed));
    ULONG  NameList           __attribute__ ((packed));
} _ROOT;

typedef struct _
{
    ULONG  Subdirectory       __attribute__ ((packed));
    ULONG  FileAttributes     __attribute__ ((packed));
    BYTE   UniqueID           __attribute__ ((packed));
    BYTE   Flags              __attribute__ ((packed));
    BYTE   NameSpace          __attribute__ ((packed));
    BYTE   FileNameLength     __attribute__ ((packed));
    BYTE   FileName[80]       __attribute__ ((packed));
    ULONG  ExtendedSpace      __attribute__ ((packed));
    BYTE   ExtantsUsed        __attribute__ ((packed));
    BYTE   LengthData         __attribute__ ((packed));
    ULONG  frNext             __attribute__ ((packed));
    ULONG  frPrior            __attribute__ ((packed));
    ULONG  frParent           __attribute__ ((packed));
    ULONG  frChild            __attribute__ ((packed));
    ULONG  frFlags            __attribute__ ((packed));
    BYTE   Reserved1[2]       __attribute__ ((packed));
    ULONG  PrimaryEntry       __attribute__ ((packed));
    ULONG  NameList           __attribute__ ((packed));
} ;

typedef struct _UNIX_EXTENDED_DIR
{
    ULONG  Signature          __attribute__ ((packed));
    ULONG  Length             __attribute__ ((packed));
    ULONG  DirectoryNumber    __attribute__ ((packed));
    BYTE   NameSpace          __attribute__ ((packed));
    BYTE   Flags              __attribute__ ((packed));
    BYTE   ControlFlags       __attribute__ ((packed));
    BYTE   Reserved           __attribute__ ((packed));
    BYTE   NameLength         __attribute__ ((packed));
    BYTE   Name[1]            __attribute__ ((packed));
} UNIX_EXTENDED_DIR;

typedef struct _EXTENDED_DIR
{
    ULONG  Signature          __attribute__ ((packed));
    ULONG  Length             __attribute__ ((packed));
    ULONG  DirectoryNumber    __attribute__ ((packed));
    BYTE   NameSpace          __attribute__ ((packed));
    BYTE   Flags              __attribute__ ((packed));
    BYTE   ControlFlags       __attribute__ ((packed));
    BYTE   Reserved           __attribute__ ((packed));
} EXTENDED_DIR;

#else

typedef struct _ROOT
{
    ULONG  Subdirectory;
    ULONG  FileAttributes;
    BYTE   UniqueID;
    BYTE   Flags;
    BYTE   NameSpace;
    BYTE   NameSpaceCount;

    // Really weird -- Looks as though at a later date someone needed
    // to squeeze extra space out of the root directory entry for an
    // NDS volume stamp.  Since namespace entries are never greater than
    // 12, a nibble turns out to be enough.  In 4.x the name space table
    // is only 6 bytes (12 nibbles/12 possible namespaces).  In 3.x, each
    // namespace takes a single byte instead of a single nibble.  Since
    // NetWare has never defined more than 7 namespaces, the last bytes
    // are still ok to use as volume flags. In 3.x, these last fields
    // always contain zero.

    ULONG  DirectoryServicesObjectID;
    BYTE   SupportedNameSpacesNibble[6];
    BYTE   DOSType;
    BYTE   VolumeFlags;
    ULONG  CreateDateAndTime;
    ULONG  OwnerID;
    ULONG  LastArchivedDateAndTime;
    ULONG  LastArchivedID;
    ULONG  LastModifiedDateAndTime;
    ULONG  NextTrusteeEntry;
    ULONG  Trustees[8];
    WORD   TrusteeMask[8];
    ULONG  MaximumSpace;
    WORD   MaximumAccessMask;
    WORD   SubdirectoryFirstBlockGone;
    ULONG  ExtendedDirectoryChain0;
    ULONG  ExtendedDirectoryChain1;
    ULONG  ExtendedAttributes;
    ULONG  ModifyTimeInSeconds;
    ULONG  SubAllocationList;
    ULONG  NameList;
} ROOT;

typedef struct _ROOT3X
{
    ULONG  Subdirectory;
    ULONG  FileAttributes;
    BYTE   UniqueID;
    BYTE   Flags;
    BYTE   NameSpace;
    BYTE   NameSpaceCount;

    // Really weird -- Looks as though at a later date someone needed
    // to squeeze extra space out of the root directory entry for an
    // NDS volume stamp.  Since namespace entries are never greater than
    // 12, a nibble turns out to be enough.  In 4.x the name space table
    // is only 6 bytes (12 nibbles/12 possible namespaces).  In 3.x, each
    // namespace takes a single byte instead of a single nibble.  Since
    // NetWare has never defined more than 7 namespaces, the last bytes
    // are still ok to use as volume flags. In 3.x, these last fields
    // always contain zero.

    BYTE   NameSpaceTable[10];
    BYTE   DOSType;
    BYTE   VolumeFlags;
    ULONG  CreateDateAndTime;
    ULONG  OwnerID;
    ULONG  LastArchivedDateAndTime;
    ULONG  LastArchivedID;
    ULONG  LastModifiedDateAndTime;
    ULONG  NextTrusteeEntry;
    ULONG  Trustees[8];
    WORD   TrusteeMask[8];
    ULONG  MaximumSpace;
    WORD   MaximumAccessMask;
    WORD   SubdirectoryFirstBlockGone;
    ULONG  ExtendedDirectoryChain0;
    ULONG  ExtendedDirectoryChain1;
    ULONG  ExtendedAttributes;
    ULONG  ModifyTimeInSeconds;
    ULONG  SubAllocationList;
    ULONG  NameList;
} ROOT3X;

typedef struct _DOS
{
    ULONG  Subdirectory;
    ULONG  FileAttributes;
    BYTE   UniqueID;
    BYTE   Flags;
    BYTE   NameSpace;
    BYTE   FileNameLength;
    BYTE   FileName[12];
    ULONG  CreateDateAndTime;
    ULONG  OwnerID;
    ULONG  LastArchivedDateAndTime;
    ULONG  LastArchivedID;
    ULONG  LastUpdatedDateAndTime;
    ULONG  LastUpdatedID;
    ULONG  FileSize;
    ULONG  FirstBlock;
    ULONG  NextTrusteeEntry;
    ULONG  Trustees[4];
    ULONG  LookUpEntryNumber;
    ULONG  LastUpdatedInSeconds;
    WORD   TrusteeMask[4];
    WORD   ChangeReferenceID;
    WORD   Reserved[1];
    WORD   MaximumAccessMask;
    WORD   LastAccessedDate;
    ULONG  DeletedFileTime;
    ULONG  DeletedDateAndTime;
    ULONG  DeletedID;
    ULONG  ExtendedAttributes;
    ULONG  DeletedBlockSequenceNumber;
    ULONG  PrimaryEntry;
    ULONG  NameList;
} DOS;

typedef struct _SUBDIR
{
    ULONG  Subdirectory;
    ULONG  FileAttributes;
    BYTE   UniqueID;
    BYTE   Flags;
    BYTE   NameSpace;
    BYTE   FileNameLength;
    BYTE   FileName[12];
    ULONG  CreateDateAndTime;
    ULONG  OwnerID;
    ULONG  LastArchivedDateAndTime;
    ULONG  LastArchivedID;
    ULONG  LastModifiedDateAndTime;
    ULONG  NextTrusteeEntry;
    ULONG  Trustees[8];
    WORD   TrusteeMask[8];
    ULONG  MaximumSpace;
    WORD   MaximumAccessMask;
    WORD   SubdirectoryFirstBlockGone;
    ULONG  MigrationBindID;
    ULONG  Reserved;
    ULONG  ExtendedAttributes;
    ULONG  LastModifiedInSeconds;
    ULONG  PrimaryEntry;
    ULONG  NameList;
} SUB_DIRECTORY;

typedef struct _TRUSTEE
{
    ULONG  Flag;
    ULONG  Attributes;
    BYTE   ID;
    BYTE   TrusteeCount;
    BYTE   Flags;
    BYTE   Reserved1[1];
    ULONG  Subdirectory;
    ULONG  NextTrusteeEntry;
    ULONG  FileEntryNumber;
    ULONG  Trustees[16];
    WORD   TrusteeMask[16];
    BYTE   Reserved2[4];
    ULONG  DeletedBlockSequenceNumber;
} TRUSTEE;

typedef struct _USER
{
    ULONG  Flag;
    ULONG  Reserved1;
    BYTE   ID;
    BYTE   TrusteeCount;
    BYTE   Reserved2[2];
    ULONG  Subdirectory;
    ULONG  Trustees[14];
    ULONG  Restriction[14];
} USER;

typedef struct _SUBALLOC
{
    ULONG  Flag;
    ULONG  Reserved1;
    BYTE   ID;
    BYTE   SequenceNumber;
    BYTE   Reserved2[2];
    ULONG  SubAllocationList;
    ULONG  StartingFATChain[28];
} SUBALLOC;

typedef struct _MACINTOSH
{
    ULONG  Subdirectory;
    ULONG  FileAttributes;
    BYTE   UniqueID;
    BYTE   Flags;
    BYTE   NameSpace;
    BYTE   FileNameLength;
    BYTE   FileName[32];
    ULONG  ResourceFork;
    ULONG  ResourceForkSize;
    BYTE   FinderInfo[32];
    BYTE   ProDosInfo[6];
    BYTE   DirRightsMask[4];
    BYTE   Reserved0[2];
    ULONG  CreateTime;
    ULONG  BackupTime;
    BYTE   Reserved1[12];
    ULONG  DeletedBlockSequenceNumber;
    ULONG  PrimaryEntry;
    ULONG  NameList;
} MACINTOSH;

#define NFS_HASSTREAM_HARD_LINK      0x01
#define NFS_SYMBOLIC_LINK            0x02
#define NFS_HARD_LINK                0x04

#pragma pack(1)

typedef struct _NFS
{
    ULONG  Subdirectory;
    ULONG  FileAttributes;
    BYTE   UniqueID;
    BYTE   Flags;
    BYTE   NameSpace;
    BYTE   FileNameLength;
    BYTE   FileName[40];
    BYTE   TotalFileNameLength;
    BYTE   ExtantsUsed;
    ULONG  StartExtantNumber;
    ULONG  mode;
    ULONG  flags; // ???? look like Netware specific flags
    ULONG  gid;
    ULONG  rdev;
    ULONG  nlinks;
    BYTE   LinkedFlag;
    BYTE   FirstCreated;
    ULONG  LinkNextDirNo;
    ULONG  LinkEndDirNo;
    ULONG  LinkPrevDirNo;
    ULONG  uid;
    BYTE   ACSFlags; // 1-lastunix, 2-lastnetware
    WORD   LastAccessedTime;
    BYTE   Reserved[17];
    ULONG  DeletedBlockSequenceNumber;
    ULONG  PrimaryEntry;
    ULONG  NameList;
} NFS;

#pragma pack()

typedef struct _LONGNAME
{
    ULONG  Subdirectory;
    ULONG  FileAttributes;
    BYTE   UniqueID;
    BYTE   Flags;
    BYTE   NameSpace;
    BYTE   FileNameLength;
    BYTE   FileName[80];
    ULONG  ExtendedSpace;
    BYTE   ExtantsUsed;
    BYTE   LengthData;
    BYTE   Reserved1[18];
    ULONG  Reserved2;
    ULONG  PrimaryEntry;
    ULONG  NameList;
} LONGNAME;

typedef struct _NTNAME
{
    ULONG  Subdirectory;
    ULONG  FileAttributes;
    BYTE   UniqueID;
    BYTE   Flags;
    BYTE   NameSpace;
    BYTE   FileNameLength;
    BYTE   FileName[80];
    ULONG  ExtendedSpace;
    BYTE   ExtantsUsed;
    BYTE   LengthData;
    WORD   ExtendedFlags;
    ULONG  FileIdentifierStamp[2];
    ULONG  Reserved1[2];
    ULONG  Reserved2;
    ULONG  PrimaryEntry;
    ULONG  NameList;
} NTNAME;

typedef struct __ROOT
{
    ULONG  Subdirectory;
    ULONG  FileAttributes;
    BYTE   UniqueID;
    BYTE   Flags;
    BYTE   NameSpace;
    BYTE   FileNameLength;
    BYTE   FileName[80];
    ULONG  BlockSize;
    ULONG  SplitSize;
    ULONG  FenrisNext;
    ULONG  FenrisPrior;
    ULONG  FenrisParent;
    ULONG  FenrisChild;
    ULONG  FenrisFlags;
    ULONG  PrimaryEntry;
    ULONG  NameList;
} _ROOT;

typedef struct _
{
    ULONG  Subdirectory;
    ULONG  FileAttributes;
    BYTE   UniqueID;
    BYTE   Flags;
    BYTE   NameSpace;
    BYTE   FileNameLength;
    BYTE   FileName[80];
    ULONG  ExtendedSpace;
    BYTE   ExtantsUsed;
    BYTE   LengthData;
    ULONG  frNext;
    ULONG  frPrior;
    ULONG  frParent;
    ULONG  frChild;
    ULONG  frFlags;
    BYTE   Reserved1[2];
    ULONG  PrimaryEntry;
    ULONG  NameList;
} ;

typedef struct _UNIX_EXTENDED_DIR
{
    ULONG  Signature;
    ULONG  Length;
    ULONG  DirectoryNumber;
    BYTE   NameSpace;
    BYTE   Flags;
    BYTE   ControlFlags;
    BYTE   Reserved;
    BYTE   NameLength;
    BYTE   Name[1];
} UNIX_EXTENDED_DIR;

typedef struct _EXTENDED_DIR
{
    ULONG  Signature;
    ULONG  Length;
    ULONG  DirectoryNumber;
    BYTE   NameSpace;
    BYTE   Flags;
    BYTE   ControlFlags;
    BYTE   Reserved;
} EXTENDED_DIR;

#endif

typedef struct _SUBALLOC_MAP {
    ULONG  Count;
    long   Size;
    ULONG  clusterIndex[2];
    ULONG  clusterOffset[2];
    ULONG  clusterNumber[2];
    ULONG  clusterSize[2];
} SUBALLOC_MAP;

#endif

