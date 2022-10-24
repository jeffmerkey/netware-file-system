
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
*   FILE     :  NWDIR.C
*   DESCRIP  :   NetWare Directory Management
*   DATE     :  December 7, 1998
*
*
***************************************************************************/

#include "globals.h"

BYTE *NameSpaceStrings[16]=
{
    "DOS ", "MAC ", "NFS ", "FTAM", "LONG", "NT  ", "FEN ", "????",
    "????", "????", "????", "????", "????", "????", "????", "????"
};

BYTE *AttributeDescription[]=
{
    "Ro - Read Only File",              //  00000001
    "Hd - Hidden File",                 //  00000002
    "Sm - System File",                 //  00000004
    "Ex - Execute Only",                //  00000008
    "Sd - Subdirectory",                //  00000010
    "Ar - Archived",                    //  00000020
    "Reserved-00000040",                //  00000040
    "Sh - Shareable",                   //  00000080
    "Reserved-00000100",                //  00000100
    "Sm - Share Modes Supported",       //  00000200
    "Sm - Share Modes Supported",       //  00000400
    "Sd - Suballocation Disabled",      //  00000800
    "Tr - Transactional File",          //  00001000
    "Oi - Old Indexed On",              //  00002000
    "Ra - Read Auditing On",            //  00004000
    "Wa - Write Auditing On",           //  00008000
    "Ip - Immediate Purge",             //  00010000
    "Ri - Rename Inhibit",              //  00020000
    "Di - Delete Inhibit",              //  00040000
    "Ci - Copy Inhibit",                //  00080000
    "Fa - File Auditing Enabled",       //  00100000
    "Cn - Container File",              //  00200000
    "Ra - Remote Data Access",          //  00400000
    "Ri - Remote Data Save Inhibit",    //  00800000
    "Rs - Remote Data Save Key",        //  01000000
    "Ci - Compress Immediately",        //  02000000
    "Cp - File is Compressed",          //  04000000
    "Nc - Compression Disabled",        //  08000000
    "Reserved-10000000",                //  10000000
    "Cc - Cannot Compress File Data",   //  20000000
    "Af - Compressed Archive File",     //  40000000
    "Reserved-80000000",                //  80000000
};

BYTE *AttributeShorthand[]=
{
    "Ro", "Hd", "Sm", "Ex", "Sd", "Ar", "--", "Sh", "--", "Sm",
    "Sm", "Sd", "Tr", "Oi", "Ra", "Wa", "Ip", "Ri", "Di", "Ci", //  00080000
    "Fa", "Cn", "Ra", "Ri", "Rs", "Ci", "Cp", "Nc", "--", "Cc",
    "Af", "--",
};

void NWLockVolume(VOLUME *volume)
{
#if (LINUX_SLEEP)
    if (WaitOnSemaphore(&volume->VolumeSemaphore) == -EINTR)
       NWFSPrint("lock volume was interupted\n");
#endif

}
void NWUnlockVolume(VOLUME *volume)
{

#if (LINUX_SLEEP)
    SignalSemaphore(&volume->VolumeSemaphore);
#endif
}

void NWLockDirAssign(VOLUME *volume)
{
#if (LINUX_SLEEP)
    if (WaitOnSemaphore(&volume->DirAssignSemaphore) == -EINTR)
       NWFSPrint("lock dir assign was interupted\n");
#endif
}

void NWUnlockDirAssign(VOLUME *volume)
{
#if (LINUX_SLEEP)
    SignalSemaphore(&volume->DirAssignSemaphore);
#endif
}

void NWLockDirBlock(VOLUME *volume)
{
#if (LINUX_SLEEP)
    if (WaitOnSemaphore(&volume->DirBlockSemaphore) == -EINTR)
	NWFSPrint("lock dir block was interupted\n");
#endif
}

void NWUnlockDirBlock(VOLUME *volume)
{
#if (LINUX_SLEEP)
    SignalSemaphore(&volume->DirBlockSemaphore);
#endif
}

ULONG InitializeVolumeDirectory(VOLUME *volume)
{
#if (!WINDOWS_NT_RO)
    register ULONG retCode;
    ULONG DirNo = 0;

    // if the DELETED.SAV directory does not exist, then create it.

    retCode = NWCreateDirectoryEntry(volume,
		       "DELETED.SAV",
		       11,
		       (SUBDIRECTORY | HIDDEN | SYSTEM),
		       0,
		       NW_SUBDIRECTORY_FILE,
		       0, 0, 0, 0, &DirNo,
		       (ULONG) -1, 0,
		       (ULONG) -1, 0,
		       0, 0);
    if ((retCode) && (retCode != NwFileExists))
       return retCode;


    // if the volume flags indicate that this is a Netware 4.x volume
    // then create the Netware Directory Services Directory.

    if ((volume->VolumeFlags & NEW_TRUSTEE_COUNT) ||
	(volume->VolumeFlags & NDS_FLAG))
    {
       retCode = NWCreateDirectoryEntry(volume,
		       "_NETWARE",
		       8,
		       (SUBDIRECTORY | HIDDEN | SYSTEM | IMMEDIATE_PURGE),
		       0,
		       NW_SUBDIRECTORY_FILE,
		       0, 0, 0, 0, &DirNo,
		       (ULONG) -1, 0,
		       (ULONG) -1, 0,
		       0, 0);
       if ((retCode) && (retCode != NwFileExists))
	  return retCode;
    }

    // if suballocation is enabled for this volume, check if the
    // chain has been created.  if the suballoc chain heads have
    // not been initialized, then create them.

    if (volume->VolumeFlags & SUB_ALLOCATION_ON)
    {
       if (!volume->VolumeSuballocRoot)
       {
	  retCode = NWCreateSuballocRecords(volume);
	  if (retCode)
	     return retCode;
       }
    }

#if (VERBOSE)
    NWFSPrint("last dir block-%d  last valid dir block-%d\n",
              (int)volume->EndingBlockNo,
	      (int)volume->LastValidBlockNo);
#endif

#else
#endif
    return 0;
}

ULONG ValidateDirectoryRecord(VOLUME *volume, DOS *dos)
{
    switch (dos->Subdirectory)
    {
       case FREE_NODE:
	  return 0;

       case TRUSTEE_NODE:
	  {
             register ULONG i;
	     TRUSTEE *trustee = (TRUSTEE *)dos;

	     if ((trustee->Reserved1[0]) || (trustee->TrusteeCount > 16))
	     {
                NWFSPrint("Invalid trustee record res1-%d trustees-%d\n",
			  (int)trustee->Reserved1[0], 
			  (int)trustee->TrusteeCount);
     	        return 1;
	     }

	     for (i=0; i < 16; i++)
	     {
                if (trustee->TrusteeMask[i] & ~TRUSTEE_VALID_MASK)
		{
                   NWFSPrint("Invalid trustee record mask-%08X MASK-%08X\n",
			  (unsigned)trustee->TrusteeMask[i], 
			  (unsigned)TRUSTEE_VALID_MASK);
		   return 1;
		}
	     }
	  }
	  return 0;

       case ROOT_NODE:
	  {
	     ROOT *root = (ROOT *)dos;
	     ROOT3X *root3x = (ROOT3X *)dos;

	     if ((root->NameSpace > 10) || (root->NameSpaceCount > 10))
	     {
                NWFSPrint("Namespace/count (%d-%d) was > 10\n", 
			   (int)root->NameSpace, (int)root->NameSpaceCount);
     	        return 1;
	     }
	     
	     if ((volume->VolumeFlags & NDS_FLAG) ||
	         (volume->VolumeFlags & NEW_TRUSTEE_COUNT))
	     {
	        if (root->VolumeFlags & ~VFLAGS_4X_MASK)
		{
		   NWFSPrint("root 4X volume flags invalid (%08X) mask-%08X\n",
			     (unsigned)root->VolumeFlags, 
			     (unsigned)VFLAGS_4X_MASK);
	           return 1;
		}
	     }
	     else
	     {
	        if (root3x->VolumeFlags & ~VFLAGS_3X_MASK)
		{
                   NWFSPrint("root 3X volume flags invalid (%08X) mask-%08X\n",
			     (unsigned)root3x->VolumeFlags, 
			     (unsigned)VFLAGS_3X_MASK);
	           return 1;
		}
	     }
          }
	  return 0;

       case RESTRICTION_NODE:
	  {
     	     USER *user = (USER *)dos;

	     if ((user->Reserved1) || (user->TrusteeCount > 14))
	     {
                NWFSPrint("Invalid user record res1-%d trustees-%d\n",
			  (int)user->Reserved1, 
			  (int)user->TrusteeCount);
     	        return 1;
	     }
          }
	  return 0;

       case SUBALLOC_NODE:
	  {
             SUBALLOC *suballoc = (SUBALLOC *)dos;
     	     register ULONG i;
	     
	     if (suballoc->SequenceNumber > 4)
	     {
                NWFSPrint("Invalid suballocation record res-%d sequence-%d\n",
			  (int)suballoc->Reserved1, 
			  (int)suballoc->SequenceNumber);
		return 1;
	     }
             
	     for (i=0; i < 28; i++)
	     {
                // if any suballocation table entries exceed volume 
	        // clusters, zero the invalid fat heads.
     	        if (suballoc->StartingFATChain[i] > volume->VolumeClusters)
		{
                   NWFSPrint("Invalid suballocation fat head %X (%X)\n",
			  (unsigned)suballoc->StartingFATChain[i], 
			  (unsigned)volume->VolumeClusters);
		   return 1;
		}
	     }
	  }
	  return 0;

       default:
	  if (dos->NameSpace > 10)
	  {
	     NWFSPrint("Invalid namespace record %d\n", (int)dos->NameSpace);
	     return 1;
	  }

	  {
	     MACINTOSH *mac = (MACINTOSH *)dos;
             NFS *nfs = (NFS *)dos;
	     LONGNAME *longname = (LONGNAME *)dos;
	     NTNAME *nt = (NTNAME *)dos;

	     switch (dos->NameSpace)
	     {
                case DOS_NAME_SPACE:
#if (STRICT_NAMES)
     		   if (NWValidDOSName(volume, dos->FileName, 
				      dos->FileNameLength, 0, 0))
		   {
                      NWFSPrint("Invalid dos name\n");
		      return 1;
		   }
#endif
		   return 0;
			
		case MAC_NAME_SPACE:
#if (STRICT_NAMES)
                   if (NWValidMACName(volume, mac->FileName, 
				      mac->FileNameLength, 0, 0))
		   {
                      NWFSPrint("Invalid mac name\n");
     		      return 1;
		   }
#endif
		   return 0;

		case LONG_NAME_SPACE:
#if (STRICT_NAMES)
                   if (NWValidLONGName(volume, longname->FileName, 
				      longname->FileNameLength, 0, 0))
		   {
                      NWFSPrint("Invalid long name\n");
     		      return 1;
		   }
#endif
		   if (longname->ExtantsUsed > 3)
		   {
                      NWFSPrint("Invalid longname file extants-%d\n", 
				(int)longname->ExtantsUsed);
     		      return 1;
		   }

		   return 0;

		case UNIX_NAME_SPACE:
#if (STRICT_NAMES)
                   if (NWValidNFSName(volume, nfs->FileName, 
				      nfs->TotalFileNameLength, 0, 0))
		   {
                      NWFSPrint("Invalid nfs name\n");
     		      return 1;
		   }
#endif
		   
		   if (nfs->ExtantsUsed > 3)
		   {
                      NWFSPrint("Invalid nfs file extants-%d\n", 
				(int)nfs->ExtantsUsed);
     		      return 1;
		   }
		   return 0;

		case NT_NAME_SPACE:
#if (STRICT_NAMES)
                   if (NWValidLONGName(volume, nt->FileName, 
				      nt->FileNameLength, 0, 0))
		   {
                      NWFSPrint("Invalid nt name\n");
     		      return 1;
		   }
#endif
		   
		   if (nt->ExtantsUsed > 3)
		   {
                      NWFSPrint("Invalid nt file extants-%d\n", 
				(int)nt->ExtantsUsed);
     		      return 1;
		   }
		   return 0;

		default:
		   return 1;
	     }
	  }
	  
	  return 0;
    }    
}

ULONG ReadDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo)
{
    register ULONG DirsPerCluster, hash, cbytes;
    register ULONG Block, Offset;
    register DIR_BLOCK_HASH_LIST *HashTable;
    register DIR_BLOCK_HASH *list;
    ULONG retCode = 0;

    if (!dos)
       return NwInvalidParameter;

    DirsPerCluster = volume->ClusterSize / sizeof(DOS);
    Block = DirNo / DirsPerCluster;
    Offset = DirNo % DirsPerCluster;

    hash = (Block & (volume->DirBlockHashLimit - 1));
    HashTable = (DIR_BLOCK_HASH_LIST *) volume->DirBlockHash;

// if the block hash is uninitialized and we are in utility mode,
// and we treat the directory as a file if the faster block
// hash is not present.

#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
    if (!HashTable)
    {
       cbytes = NWReadFile(volume,
			   &volume->FirstDirectory,
			   0,
			   -1,
			   DirNo * sizeof(DOS),
			   (BYTE *)dos,
			   sizeof(DOS),
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   0,
			   TRUE);

       if ((cbytes == sizeof(DOS)) && !retCode)
       {
          if (ValidateDirectoryRecord(volume, dos))
          {
             NWFSPrint("directory # %X is invalid\n", (unsigned)DirNo);
             return NwDirectoryCorrupt;
          }
	  return 0;
       }

       cbytes = NWReadFile(volume,
			   &volume->SecondDirectory,
			   0,
			   -1,
			   DirNo * sizeof(DOS),
			   (BYTE *)dos,
			   sizeof(DOS),
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   0,
			   TRUE);

       if ((cbytes == sizeof(DOS)) && !retCode)
       {
          if (ValidateDirectoryRecord(volume, dos))
          {
             NWFSPrint("directory # %X is invalid\n", (unsigned)DirNo);
             return NwDirectoryCorrupt;
          }
	  return 0;
       }
    }
#else
    if (!HashTable)
       return NwInvalidParameter;
#endif

    NWLockDirBlock(volume);
    list = (DIR_BLOCK_HASH *) HashTable[hash].head;
    while (list)
    {
       if (list->BlockNo == Block)
       {
	  cbytes = ReadClusterWithOffset(volume, list->Cluster1,
					 (Offset * sizeof(DOS)),
					 (BYTE *)dos, sizeof(DOS),
					 KERNEL_ADDRESS_SPACE,
					 &retCode,
					 DIRECTORY_PRIORITY);
	  if ((cbytes == sizeof(DOS)) && !retCode)
	  {
	     NWUnlockDirBlock(volume);
             if (ValidateDirectoryRecord(volume, dos))
             {
                NWFSPrint("directory # %X is invalid\n", (unsigned)DirNo);
                return NwDirectoryCorrupt;
             }
	     return 0;
	  }

	  // if the first dir read failed, try the second
	  cbytes = ReadClusterWithOffset(volume, list->Cluster2,
					 (Offset * sizeof(DOS)),
					 (BYTE *)dos, sizeof(DOS),
					 KERNEL_ADDRESS_SPACE,
					 &retCode,
					 DIRECTORY_PRIORITY);
	  if ((cbytes == sizeof(DOS)) && !retCode)
	  {
	     NWUnlockDirBlock(volume);
             if (ValidateDirectoryRecord(volume, dos))
             {
                NWFSPrint("directory # %X is invalid\n", (unsigned)DirNo);
                return NwDirectoryCorrupt;
             }
	     return 0;
	  }
	  NWUnlockDirBlock(volume);
	  return NwDiskIoError;
       }
       list = list->next;
    }
    NWUnlockDirBlock(volume);
    return NwNoEntry;

}

ULONG WriteDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo)
{
    register ULONG DirsPerCluster, hash, cbytes;
    register ULONG Block, Offset;
    register DIR_BLOCK_HASH_LIST *HashTable;
    register DIR_BLOCK_HASH *list;
    ULONG retCode = 0;

    if (!dos)
       return NwInvalidParameter;

    if (ValidateDirectoryRecord(volume, dos))
    {
       NWFSPrint("directory # %X is invalid\n", (unsigned)DirNo);
       return NwDirectoryCorrupt;
    }
    
    DirsPerCluster = volume->ClusterSize / sizeof(DOS);
    Block = DirNo / DirsPerCluster;
    Offset = DirNo % DirsPerCluster;

    hash = (Block & (volume->DirBlockHashLimit - 1));
    HashTable = (DIR_BLOCK_HASH_LIST *) volume->DirBlockHash;

// if the block hash is uninitialized and we are in utility mode,
// and we treat the directory as a file if the faster block
// hash is not present.

#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
    if (!HashTable)
    {
       // write primary directory
       cbytes = NWWriteFile(volume,
			   &volume->FirstDirectory,
			   0,
			   DirNo * sizeof(DOS),
			   (BYTE *)dos,
			   sizeof(DOS),
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   0);

       if ((cbytes == sizeof(DOS)) && !retCode)
       {
#if (!CREATE_DIR_MISMATCH)
	  // write mirror directory
	  cbytes = NWWriteFile(volume,
			   &volume->SecondDirectory,
			   0,
			   DirNo * sizeof(DOS),
			   (BYTE *)dos,
			   sizeof(DOS),
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   0);

          if ((cbytes == sizeof(DOS)) && !retCode)
#endif
     	     return 0;
       }
       return NwDiskIoError;
    }
#else
    if (!HashTable)
       return NwInvalidParameter;
#endif

    NWLockDirBlock(volume);
    list = (DIR_BLOCK_HASH *) HashTable[hash].head;
    while (list)
    {
       if (list->BlockNo == Block)
       {
	  cbytes = WriteClusterWithOffset(volume, list->Cluster1,
					  (Offset * sizeof(DOS)),
					  (BYTE *)dos, sizeof(DOS),
					  KERNEL_ADDRESS_SPACE,
					  &retCode,
					  DIRECTORY_PRIORITY);

	  if ((cbytes == sizeof(DOS)) && !retCode)
	  {
#if (!CREATE_DIR_MISMATCH)
     	     // if first write succeeded, write mirror directory entry
	     cbytes = WriteClusterWithOffset(volume, list->Cluster2,
					  (Offset * sizeof(DOS)),
					  (BYTE *)dos, sizeof(DOS),
					  KERNEL_ADDRESS_SPACE,
					  &retCode,
					  DIRECTORY_PRIORITY);

	     if ((cbytes == sizeof(DOS)) && !retCode)
	     {
	        NWUnlockDirBlock(volume);
	        return 0;

	     }
	     NWUnlockDirBlock(volume);
	     NWFSPrint("mirror directory write failed dirno-%X\n",
		       (unsigned)DirNo);
	     return NwDiskIoError;
#else
	     return 0;
#endif
	  }
	  NWUnlockDirBlock(volume);
	  NWFSPrint("primary directory write failed dirno-%X\n",
		    (unsigned)DirNo);
	  return NwDiskIoError;
       }
       list = list->next;
    }
    NWUnlockDirBlock(volume);
    return NwNoEntry;

}

ULONG CompareDirectoryTable(VOLUME *volume)
{
    register FAT_ENTRY *FAT1, *FAT2;
    FAT_ENTRY FAT1_S, FAT2_S;
    register ULONG retCode;
    register long index1, index2;
    register ULONG DirNo, DirBlock, block, entry;
    register ULONG DirIndex = 0, owner;
    register ULONG cluster1, cluster2;
    register BYTE *Buffer1, *Buffer2;
    register BYTE *PBuffer1, *PBuffer2;
    register DOS *dos;

    NWFSPrint("*** Comparing Primary and Mirror Directory Files ***\n");

    Buffer1 = NWFSCacheAlloc(volume->ClusterSize, DIR_WORKSPACE_TAG);
    if (!Buffer1)
       return -3;

    Buffer2 = NWFSCacheAlloc(volume->ClusterSize, DIR_WORKSPACE_TAG);
    if (!Buffer2)
    {
       NWFSFree(Buffer1);
       return -3;
    }
    
    PBuffer1 = NWFSIOAlloc(volume->ClusterSize, DIR_WORKSPACE_TAG);
    if (!PBuffer1)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -3;
    }

    PBuffer2 = NWFSIOAlloc(volume->ClusterSize, DIR_WORKSPACE_TAG);
    if (!PBuffer2)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       return -3;
    }

    DirBlock = DirNo = 0;
    cluster1 = volume->FirstDirectory;
    cluster2 = volume->SecondDirectory;

    if ((cluster1 && !cluster2) || (!cluster1 && cluster2))
    {
       NWFSPrint("nwfs:  directory fat head chain mismatch (comp)\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       NWFSFree(PBuffer2);
       return -1;
    }
    
    if ((!cluster1) || (!cluster2))
    {
       NWFSPrint("nwfs:  no root directory located for volume (comp)\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       NWFSFree(PBuffer2);
       return -3;
    }
    
    if (((cluster1 == (ULONG)-1) && (cluster2 != (ULONG)-1)) ||
       ((cluster1 != (ULONG)-1) && (cluster2 == (ULONG)-1))) 
    {
       NWFSPrint("nwfs:  directory fat head chain mismatch (comp)\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       NWFSFree(PBuffer2);
       return -1;
    }
    
    if ((cluster1 == (ULONG)-1) && (cluster2 == (ULONG)-1))
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       NWFSFree(PBuffer2);
       return 0;
    }

    // read directory root 1

    retCode = ReadPhysicalVolumeCluster(volume, cluster1, Buffer1, volume->ClusterSize,
				        DIRECTORY_PRIORITY);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       NWFSFree(PBuffer2);
       return retCode;
    }

    // read directory root 2

    retCode = ReadPhysicalVolumeCluster(volume, cluster2, Buffer2, volume->ClusterSize,
    					DIRECTORY_PRIORITY);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       NWFSFree(PBuffer2);
       return retCode;
    }

    // read directory root 1

    retCode = ReadAbsoluteVolumeCluster(volume, cluster1, PBuffer1);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       NWFSFree(PBuffer2);
       return retCode;
    }

    // read directory root 2

    retCode = ReadAbsoluteVolumeCluster(volume, cluster2, PBuffer2);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       NWFSFree(PBuffer2);
       return retCode;
    }

    FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
    FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);

    if (!FAT1 || !FAT2)
    {
       NWFSPrint("nwfs:  error reading fat entry ReadFATTables (comp)\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       NWFSFree(PBuffer2);
       return -1;
    }
    index1 = FAT1->FATIndex;
    index2 = FAT2->FATIndex;

    while (TRUE)
    {
       if (NWFSCompare(Buffer1, Buffer2, volume->ClusterSize))
       {
          register ULONG i;
	  extern void dumpRecordBytes(BYTE *, ULONG);

	  for (i=0; i < (volume->ClusterSize / 64); i++)
	  {
	     if (NWFSCompare(&Buffer1[i * 64], &Buffer2[i * 64], 64))
	     {
		NWFSPrint("nwfs:  directory data mirror mismatch (comp)\n");
		NWFSPrint("offset into cluster is 0x%X (%u bytes)\n",
			  (unsigned)(i * 64), (unsigned)(i * 64));

		NWFSPrint("index1-%d cluster1-[%08X] next1-[%08X]\n", (int)index1,
		    (unsigned int)cluster1,
		    (unsigned int)FAT1->FATCluster);
		dumpRecordBytes(&Buffer1[i * 64], 64);

		NWFSPrint("index2-%d cluster2-[%08X] next2-[%08X]\n", (int)index2,
		    (unsigned int)cluster2,
		    (unsigned int)FAT2->FATCluster);
		dumpRecordBytes(&Buffer2[i * 64], 64);
		break;
	     }
          }
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
	  return -3;
       }

       for (block=0; block < volume->BlocksPerCluster; block++, DirBlock++)
       {
	  dos = (DOS *) &Buffer1[block * volume->BlockSize];
	  for (entry = 0, owner = (ULONG) -1;
	       entry < (volume->BlockSize / sizeof(ROOT));
	       entry++, DirNo++)
	  {
             if (ValidateDirectoryRecord(volume, &dos[entry]))
	     {
                NWFSPrint("Invalid directory record %X detected (comp).\n",
			 (unsigned)DirNo);
     	        NWFSFree(Buffer1);
	        NWFSFree(Buffer2);
                NWFSFree(PBuffer1);
                NWFSFree(PBuffer2);
	        return -3;
	     }
	  }
       }
        
       if (NWFSCompare(Buffer1, PBuffer1, volume->ClusterSize))
       {
          NWFSPrint("lru/disk mismatch primary directory cluster-%X blocks %X-%X\n", 
	            (unsigned)cluster1,
		    (unsigned)(cluster1 * volume->BlocksPerCluster),
		    (unsigned)((cluster1 * volume->BlocksPerCluster) +
			       volume->BlocksPerCluster));
       }
       
       if (NWFSCompare(Buffer1, PBuffer2, volume->ClusterSize))
       {
          NWFSPrint("lru/disk mismatch mirror directory cluster-%X block %X-%X\n", 
	            (unsigned)cluster2,
		    (unsigned)(cluster2 * volume->BlocksPerCluster),
		    (unsigned)((cluster2 * volume->BlocksPerCluster) +
			       volume->BlocksPerCluster));
       }

       retCode = WritePhysicalVolumeCluster(volume, cluster1, Buffer1, 
		                          volume->ClusterSize,
				          DIRECTORY_PRIORITY);
       if (retCode)
       {
          NWFSFree(Buffer1);
          NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
          return retCode;
       }

       retCode = WritePhysicalVolumeCluster(volume, cluster2, Buffer2, 
		                          volume->ClusterSize,
    					  DIRECTORY_PRIORITY);
       if (retCode)
       {
          NWFSFree(Buffer1);
          NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
          return retCode;
       }
       
       if (FAT1->FATCluster == (ULONG) -1)
	  break;

       // if the other FAT chain terminated abruptly, exit the loop

       if (FAT2->FATCluster == (ULONG) -1)
	  break;

       if (!FAT1->FATCluster)
       {
	  NWFSPrint("nwfs:  Free Cluster detected in DIR1 Chain (comp)\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
	  return -3;
       }

       if (!FAT2->FATCluster)
       {
	  NWFSPrint("nwfs:  Free Cluster detected in DIR2 Chain (comp)\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
	  return -3;
       }

       if (FAT1->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  SubAlloc Node Detected in DIR1 (comp)\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
	  return -3;
       }

       if (FAT2->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  SubAlloc Node Detected in DIR2 (comp)\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
	  return -3;
       }

       DirIndex++;
       cluster1 = FAT1->FATCluster;
       cluster2 = FAT2->FATCluster;

       retCode = ReadPhysicalVolumeCluster(volume, cluster1, Buffer1, volume->ClusterSize,
       					   DIRECTORY_PRIORITY);
       if (retCode)
       {
	  NWFSPrint("nwfs:  Read Error Directory1 returned %d (comp)\n", (int)retCode);
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
	  return retCode;
       }

       retCode = ReadPhysicalVolumeCluster(volume, cluster2, Buffer2, volume->ClusterSize,
       				           DIRECTORY_PRIORITY);
       if (retCode)
       {
	  NWFSPrint("nwfs:  Read Error Directory2 returned %d (comp)\n", (int)retCode);
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
	  return retCode;
       }

       // read directory root 1

       retCode = ReadAbsoluteVolumeCluster(volume, cluster1, PBuffer1);
       if (retCode)
       {
          NWFSFree(Buffer1);
          NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
          return retCode;
       }

       // read directory root 2

       retCode = ReadAbsoluteVolumeCluster(volume, cluster2, PBuffer2);
       if (retCode)
       {
          NWFSFree(Buffer1);
          NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
          return retCode;
       }

       FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
       FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);
       if (!FAT1 || !FAT2)
       {
	  NWFSPrint("nwfs:  error reading fat entry ReadDirectoryTables (comp)\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
	  return -1;
       }

       // index numbers should always be ascending.  if we detect a
       // non-ascending sequence, this indicates a corrupt FAT chain

       if (index1 > FAT1->FATIndex)
       {
	  NWFSPrint("nwfs:  DIR1 fat index overlaps on itself (comp)\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
	  return -1;
       }

       if (index2 > FAT2->FATIndex)
       {
	  NWFSPrint("nwfs:  DIR2 fat index overlaps on itself (comp)\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
          NWFSFree(PBuffer1);
          NWFSFree(PBuffer2);
	  return -1;
       }

       //  update FAT index number for chains

       index1 = FAT1->FATIndex;
       index2 = FAT2->FATIndex;
    }

    if (FAT1->FATCluster != FAT2->FATCluster)
    {
       NWFSPrint("nwfs:  Directory FAT chain mirror mismatch (comp)\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSFree(PBuffer1);
       NWFSFree(PBuffer2);
       return -3;
    }

    NWFSFree(Buffer1);
    NWFSFree(Buffer2);
    NWFSFree(PBuffer1);
    NWFSFree(PBuffer2);
    return 0;

}

ULONG ReadDirectoryTable(VOLUME *volume)
{
    register FAT_ENTRY *FAT1, *FAT2;
    FAT_ENTRY FAT1_S, FAT2_S;
    register ULONG retCode;
    register long index1, index2;
    register ULONG DirNo, DirBlock, block, entry;
    register ULONG DirIndex = 0, owner;
    register ULONG cluster1, cluster2;
    register BYTE *Buffer1, *Buffer2;
    register DOS *dos;
    register ROOT *root;
    register TRUSTEE *trustee;
    register USER *user;
    
#if (MOUNT_VERBOSE)
    NWFSPrint("*** Scanning Directory File ***\n");
#endif

    Buffer1 = NWFSCacheAlloc(volume->ClusterSize, DIR_WORKSPACE_TAG);
    if (!Buffer1)
       return -3;

    Buffer2 = NWFSCacheAlloc(volume->ClusterSize, DIR_WORKSPACE_TAG);
    if (!Buffer2)
    {
       NWFSFree(Buffer1);
       return -3;
    }

    volume->EndingDirCluster1 = 0;
    volume->EndingDirCluster2 = 0;
    volume->EndingDirIndex = 0;
    volume->EndingBlockNo = 0;
    volume->LastValidBlockNo = (ULONG)-1;

    DirBlock = DirNo = 0;
    cluster1 = volume->FirstDirectory;
    cluster2 = volume->SecondDirectory;

    if ((cluster1 && !cluster2) || (!cluster1 && cluster2))
    {
       NWFSPrint("nwfs:  directory fat head chain mismatch\n");
       if (!cluster1)
       {
          volume->FirstDirectory = (ULONG) -1;
          volume->EndingDirCluster1 = (ULONG)-1;
       }
       if (!cluster2)
       {
          volume->SecondDirectory = (ULONG) -1;
          volume->EndingDirCluster2 = (ULONG)-1;
       }
       volume->EndingDirIndex = (ULONG)-1;
       volume->EndingBlockNo = (ULONG)-1;
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -1;
    }
    
    if ((!cluster1) || (!cluster2))
    {
       NWFSPrint("nwfs:  no root directory located for volume\n");
       volume->FirstDirectory = (ULONG) -1;
       volume->SecondDirectory = (ULONG) -1;
       volume->EndingDirCluster1 = (ULONG)-1;
       volume->EndingDirCluster2 = (ULONG)-1;
       volume->EndingDirIndex = (ULONG)-1;
       volume->EndingBlockNo = (ULONG)-1;
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -3;
    }
    
    if (((cluster1 == (ULONG)-1) && (cluster2 != (ULONG)-1)) ||
       ((cluster1 != (ULONG)-1) && (cluster2 == (ULONG)-1))) 
    {
       NWFSPrint("nwfs:  directory fat head chain mismatch\n");
       if (!cluster1)
       {
          volume->EndingDirCluster1 = (ULONG)-1;
          volume->EndingDirIndex = (ULONG)-1;
          volume->EndingBlockNo = -1;
       }
       if (!cluster2)
       {
          volume->EndingDirCluster2 = (ULONG)-1;
          volume->EndingDirIndex = (ULONG)-1;
          volume->EndingBlockNo = -1;
       }
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -1;
    }
    
    if ((cluster1 == (ULONG)-1) && (cluster2 == (ULONG)-1))
    {
       volume->EndingDirCluster1 = (ULONG)-1;
       volume->EndingDirCluster2 = (ULONG)-1;
       volume->EndingDirIndex = (ULONG)-1;
       volume->EndingBlockNo = -1;
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return 0;
    }

    retCode = CreateDirBlockEntry(volume, cluster1, cluster2, DirIndex);
    if (retCode)
    {
       NWFSPrint("nwfs:  could not allocate Dir Block Hash element in ReadDirectory\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -1;
    }

    // read directory root 1

    retCode = ReadPhysicalVolumeCluster(volume, cluster1, Buffer1, volume->ClusterSize,
				        DIRECTORY_PRIORITY);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return retCode;
    }

    // read directory root 2

    retCode = ReadPhysicalVolumeCluster(volume, cluster2, Buffer2, volume->ClusterSize,
    					DIRECTORY_PRIORITY);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return retCode;
    }

    FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
    FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);

    if (!FAT1 || !FAT2)
    {
       NWFSPrint("nwfs:  error reading fat entry ReadFATTables\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -1;
    }
    index1 = FAT1->FATIndex;
    index2 = FAT2->FATIndex;

    while (TRUE)
    {
       if (NWFSCompare(Buffer1, Buffer2, volume->ClusterSize))
       {
	  extern void dumpRecordBytes(BYTE *, ULONG);

	  NWFSPrint("nwfs:  directory data mirror mismatch\n");

#if (VERBOSE)
	  for (i=0; i < (volume->ClusterSize / 64); i++)
	  {
	     if (NWFSCompare(&Buffer1[i * 64], &Buffer2[i * 64], 64))
	     {
		NWFSPrint("offset into cluster is 0x%X (%u bytes)\n",
			  (unsigned)(i * 64), (unsigned)(i * 64));

		NWFSPrint("index1-%d cluster1-[%08X] next1-[%08X]\n", (int)index1,
		    (unsigned int)cluster1,
		    (unsigned int)FAT1->FATCluster);
		dumpRecordBytes(&Buffer1[i * 64], 64);

		NWFSPrint("index2-%d cluster2-[%08X] next2-[%08X]\n", (int)index2,
		    (unsigned int)cluster2,
		    (unsigned int)FAT2->FATCluster);
		dumpRecordBytes(&Buffer2[i * 64], 64);
		break;
	     }
          }
#endif
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -3;
       }

       // each 4K block in a Netware directory is assumed to
       // be owned by a single parent.  we parse the directory
       // file in 4K blocks and each block is hashed based on
       // parent.  this speeds up creates since we can quickly
       // find blocks by parent and search them for unused entries.

       for (block=0; block < volume->BlocksPerCluster; block++, DirBlock++)
       {
	  dos = (DOS *) &Buffer1[block * volume->BlockSize];
	  for (entry = 0, owner = (ULONG) -1;
	       entry < (volume->BlockSize / sizeof(ROOT));
	       entry++, DirNo++)
	  {
             if (ValidateDirectoryRecord(volume, &dos[entry]))
	     {
                NWFSPrint("Invalid directory record %X detected.\n",
			 (unsigned)DirNo);
     	        NWFSFree(Buffer1);
	        NWFSFree(Buffer2);
	        return -3;
	     }

	     switch (dos[entry].Subdirectory)
	     {
		// we skip free nodes
		case FREE_NODE:
		   break;

		case SUBALLOC_NODE:
		   retCode = HashDirectoryRecord(volume, &dos[entry], DirNo);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error hashing directory block (suballoc)\n");
		      NWFSFree(Buffer1);
		      NWFSFree(Buffer2);
		      return retCode;
		   }
		   break;

		case TRUSTEE_NODE:
		   trustee = (TRUSTEE *) &dos[entry];
		   if (owner == (ULONG) -1)
		   {
		      owner = trustee->Subdirectory;
		   }
		   else
		   if (owner != trustee->Subdirectory)
		   {
		      NWFSPrint("nwfs:  quota record not in assigned directory block\n");
		      NWFSFree(Buffer1);
		      NWFSFree(Buffer2);
		      return -1;
		   }
		   retCode = HashDirectoryRecord(volume, &dos[entry], DirNo);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error hashing directory block (trustee)\n");
		      NWFSFree(Buffer1);
		      NWFSFree(Buffer2);
		      return retCode;
		   }
		   break;

		case RESTRICTION_NODE:
		   user = (USER *) &dos[entry];
		   if (owner == (ULONG) -1)
		   {
		      owner = user->Subdirectory;
		   }
		   else
		   if (owner != user->Subdirectory)
		   {
		      NWFSPrint("nwfs:  quota record not in assigned directory block\n");
		      NWFSFree(Buffer1);
		      NWFSFree(Buffer2);
		      return -1;
		   }
		   retCode = HashDirectoryRecord(volume, &dos[entry], DirNo);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error hashing directory block (quota)\n");
		      NWFSFree(Buffer1);
		      NWFSFree(Buffer2);
		      return retCode;
		   }
		   break;

		case ROOT_NODE:
		   root = (ROOT *) &dos[entry];
		   if (owner == (ULONG) -1)
		      owner = 0;

		   retCode = HashDirectoryRecord(volume, &dos[entry], DirNo);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error hashing directory block (root)\n");
		      NWFSFree(Buffer1);
		      NWFSFree(Buffer2);
		      return retCode;
		   }
		   break;

		default:
		   if ((dos[entry].Flags & NW4_DELETED_FILE) ||
		       (dos[entry].Flags & NW3_DELETED_FILE))
		   {
		      if (owner == (ULONG) -1)
			 owner = (ULONG) -2;
		      else
		      if (owner != (ULONG) -2)
		      {
			 NWFSPrint("nwfs:  deleted record not in assigned directory block\n");
			 NWFSFree(Buffer1);
			 NWFSFree(Buffer2);
			 return -1;
		      }
		   }
		   else
		   {
		      if (owner == (ULONG) -1)
			 owner = dos[entry].Subdirectory;
		      else
		      if (owner != dos[entry].Subdirectory)
		      {
			 NWFSPrint("nwfs(non-fatal):  record not in assigned dir block DirNo-%X [%X/%X]\n",
				   (unsigned int) DirNo,
				   (unsigned int) owner,
				   (unsigned int) dos[entry].Subdirectory);
		      }
		   }

		   retCode = HashDirectoryRecord(volume, &dos[entry], DirNo);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error hashing directory block (record)\n");
		      NWFSFree(Buffer1);
		      NWFSFree(Buffer2);
		      return retCode;
		   }
		   break;
	     }
	  }

	  // if we get here and owner is -1, then the entire block is
	  // filled with free entries.  since we store it this way,
	  // the directory block free list can be accessed with a
	  // search value of -1;  if owner is -2, then this block
	  // contains deleted files.

	  // if the block is not completely free, record it as the
	  // last valid block number.
	  if (owner != (ULONG) -1)
	     volume->LastValidBlockNo = DirBlock;

	  retCode = CreateDirAssignEntry(volume, owner, DirBlock, dos);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not allocate Dir Block Assign element in ReadDirectory\n");
	     NWFSFree(Buffer1);
	     NWFSFree(Buffer2);
	     return retCode;
	  }
	  volume->EndingBlockNo = DirBlock;
       }

       if (FAT1->FATCluster == (ULONG) -1)
	  break;

       // if the other FAT chain terminated abruptly, exit the loop

       if (FAT2->FATCluster == (ULONG) -1)
	  break;

       if (!FAT1->FATCluster)
       {
	  NWFSPrint("nwfs:  Free Cluster detected in DIR1 Chain\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -3;
       }

       if (!FAT2->FATCluster)
       {
	  NWFSPrint("nwfs:  Free Cluster detected in DIR2 Chain\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -3;
       }

       if (FAT1->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  SubAlloc Node Detected in DIR1\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -3;
       }

       if (FAT2->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  SubAlloc Node Detected in DIR2\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -3;
       }

       DirIndex++;
       cluster1 = FAT1->FATCluster;
       cluster2 = FAT2->FATCluster;

       retCode = CreateDirBlockEntry(volume, cluster1, cluster2, DirIndex);
       if (retCode)
       {
	  NWFSPrint("nwfs:  could not allocate Dir Block Hash element in ReadDirectory\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return retCode;
       }

       retCode = ReadPhysicalVolumeCluster(volume, cluster1, Buffer1, volume->ClusterSize,
       					   DIRECTORY_PRIORITY);
       if (retCode)
       {
	  NWFSPrint("nwfs:  Read Error Directory1 returned %d\n", (int)retCode);
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return retCode;
       }

       retCode = ReadPhysicalVolumeCluster(volume, cluster2, Buffer2, volume->ClusterSize,
       				           DIRECTORY_PRIORITY);
       if (retCode)
       {
	  NWFSPrint("nwfs:  Read Error Directory2 returned %d\n", (int)retCode);
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return retCode;
       }

       FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
       FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);
       if (!FAT1 || !FAT2)
       {
	  NWFSPrint("nwfs:  error reading fat entry ReadDirectoryTables\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -1;
       }

       // index numbers should always be ascending.  if we detect a
       // non-ascending sequence, this indicates a corrupt FAT chain

       if (index1 > FAT1->FATIndex)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  DIR1 fat index overlaps on itself\n");
	  return -1;
       }

       if (index2 > FAT2->FATIndex)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  DIR2 fat index overlaps on itself\n");
	  return -1;
       }

       //  update FAT index number for chains

       index1 = FAT1->FATIndex;
       index2 = FAT2->FATIndex;
    }

    if (FAT1->FATCluster != FAT2->FATCluster)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSPrint("nwfs:  Directory FAT chain mirror mismatch\n");
       return -3;
    }

    // record ending cluster and index values for directory
    volume->EndingDirCluster1 = cluster1;
    volume->EndingDirCluster2 = cluster2;
    volume->EndingDirIndex = FAT1->FATIndex;

    NWFSFree(Buffer1);
    NWFSFree(Buffer2);

    return 0;

}


#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)

ULONG DumpDirectoryFile(VOLUME *volume, FILE *fp, ULONG *totalDirs)
{
    register DOS *dos;
    register FAT_ENTRY *FAT1, *FAT2;
    FAT_ENTRY FAT1_S, FAT2_S;
    register ULONG retCode;
    register long index1, index2;
    register ULONG i;
    register ULONG DirIndex = 0;
    register ULONG cluster1, cluster2;
    register BYTE *Buffer1, *Buffer2;
    register ULONG widget = 0;
#if (LINUX_UTIL)
    struct termios init_settings, new_settings;
#endif
    BYTE wa[4] = "\\|/-";

    if (totalDirs)
       *totalDirs = 0;

    NWFSPrint("Analyzing Volume Directory ... ");
    NWFSPrint("%c\b", wa[widget++ & 3]);

    Buffer1 = NWFSCacheAlloc(volume->ClusterSize, DIR_WORKSPACE_TAG);
    if (!Buffer1)
       return -3;

    Buffer2 = NWFSCacheAlloc(volume->ClusterSize, DIR_WORKSPACE_TAG);
    if (!Buffer2)
    {
       NWFSFree(Buffer1);
       return -3;
    }

    cluster1 = volume->FirstDirectory;
    cluster2 = volume->SecondDirectory;

    if ((cluster1 && !cluster2) || (!cluster1 && cluster2))
    {
       NWFSPrint("nwfs:  directory fat chain mismatch\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -1;
    }
    
    if (!cluster1 || (cluster1 == (ULONG)-1))
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return 0;
    }

    
    // read directory root 1

    retCode = ReadPhysicalVolumeCluster(volume, cluster1, Buffer1, volume->ClusterSize,
    					DIRECTORY_PRIORITY);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return retCode;
    }

    // read directory root 2

    retCode = ReadPhysicalVolumeCluster(volume, cluster2, Buffer2, volume->ClusterSize,
    					DIRECTORY_PRIORITY);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return retCode;
    }

    FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
    FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);

    if (!FAT1 || !FAT2)
    {
       NWFSPrint("nwfs:  error reading fat entry ReadFATTables\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -1;
    }
    index1 = FAT1->FATIndex;
    index2 = FAT2->FATIndex;

#if (LINUX_UTIL)
    // if we are Linux, we need kbhit(), so we implement a terminal
    // based version of kbhit() so we can detect keystrokes during
    // the imaging operation, and abort the operation if asked to
    // do so by the user.  the code below puts a terminal into raw mode
    // so that is does not block when trying to read from the
    // keyboard device.

    tcgetattr(0, &init_settings);
    new_settings = init_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
#endif

    while (TRUE)
    {
#if (LINUX_UTIL)
       BYTE nread, ch;

       // put the terminal into raw mode, and see if we have a valid
       // keystroke pending.  if we have a keystroke pending, then
       // see if the user requested that we abort the imaging operation.

       new_settings.c_cc[VMIN] = 0;
       tcsetattr(0, TCSANOW, &new_settings);
       nread = read(0, &ch, 1);
       new_settings.c_cc[VMIN] = 1;
       tcsetattr(0, TCSANOW, &new_settings);

       if (nread)
       {
	  if ((toupper(ch) == 'X') || (toupper(ch) == 'Q') ||
	      (toupper(ch) == ESC) || (toupper(ch) == 'A'))
	  {
	     tcsetattr(0, TCSANOW, &init_settings);
	     return -13;
	  }
       }
#else
       BYTE ch;

       // DOS and Windows NT/2000 both support the kbhit() function.

       if (kbhit())
       {
	  ch = getch();
	  if ((toupper(ch) == 'X') || (toupper(ch) == 'Q') ||
	      (toupper(ch) == ESC) || (toupper(ch) == 'A'))
	  {
	     return -13;
	  }
       }
#endif

       if (NWFSCompare(Buffer1, Buffer2, volume->ClusterSize))
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  Directory data mirror mismatch [%X/%X]\n",
		    (unsigned int)cluster1,
		    (unsigned int)cluster2);
	  return -3;
       }


       if (totalDirs)
       {
	  for (i=0; i < (volume->ClusterSize / sizeof(DOS)); i++)
	  {
	     dos = (DOS *) &Buffer1[i * sizeof(DOS)];
	     switch (dos->Subdirectory)
	     {
		case FREE_NODE:
		case TRUSTEE_NODE:
		case ROOT_NODE:
		case RESTRICTION_NODE:
		case SUBALLOC_NODE:
		   break;

		default:
		   if (dos->NameSpace == DOS_NAME_SPACE)
		      (*totalDirs)++;
		   if (dos->NameSpace == MAC_NAME_SPACE)
		      (*totalDirs)++;
		   break;
	     }
	  }
       }

       NWFSPrint("%c\b", wa[widget++ & 3]);
       for (i=0; i < (volume->ClusterSize / IO_BLOCK_SIZE); i++)
       {
	  fwrite(&Buffer1[i * IO_BLOCK_SIZE], IO_BLOCK_SIZE, 1, fp);
       }

       if (FAT1->FATCluster == (ULONG) -1)
	  break;

       // if the other FAT chain terminated abruptly, exit the loop

       if (FAT2->FATCluster == (ULONG) -1)
	  break;

       if (!FAT1->FATCluster)
       {
	  NWFSPrint("nwfs:  Free Cluster detected in DIR1 Chain\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -3;
       }

       if (!FAT2->FATCluster)
       {
	  NWFSPrint("nwfs:  Free Cluster detected in DIR2 Chain\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -3;
       }

       if (FAT1->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  SubAlloc Node Detected in DIR1\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -3;
       }

       if (FAT2->FATCluster & 0x80000000)
       {
	  NWFSPrint("nwfs:  SubAlloc Node Detected in DIR2\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -3;
       }

       DirIndex++;
       cluster1 = FAT1->FATCluster;
       cluster2 = FAT2->FATCluster;


       retCode = ReadPhysicalVolumeCluster(volume, cluster1, Buffer1, volume->ClusterSize, DIRECTORY_PRIORITY);
       if (retCode)
       {
	  NWFSPrint("nwfs:  Read Error Directory1 returned %d\n", (int)retCode);
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return retCode;
       }

       retCode = ReadPhysicalVolumeCluster(volume, cluster2, Buffer2, volume->ClusterSize, DIRECTORY_PRIORITY);
       if (retCode)
       {
	  NWFSPrint("nwfs:  Read Error Directory2 returned %d\n", (int)retCode);
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return retCode;
       }

       FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
       FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);
       if (!FAT1 || !FAT2)
       {
	  NWFSPrint("nwfs:  error reading fat entry ReadDirectoryTables\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -1;
       }

       // index numbers should always be ascending.  if we detect a
       // non-ascending sequence, this indicates a corrupt FAT chain

       if (index1 > FAT1->FATIndex)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  DIR1 fat index overlaps on itself\n");
	  return -1;
       }

       if (index2 > FAT2->FATIndex)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  DIR2 fat index overlaps on itself\n");
	  return -1;
       }

       //  update FAT index number for chains

       index1 = FAT1->FATIndex;
       index2 = FAT2->FATIndex;
    }
#if (LINUX_UTIL)
    tcsetattr(0, TCSANOW, &init_settings);
#endif

    if (FAT1->FATCluster != FAT2->FATCluster)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       NWFSPrint("nwfs:  Directory FAT chain mirror mismatch\n");
       return -3;
    }

    NWFSFree(Buffer1);
    NWFSFree(Buffer2);

    return 0;

}

#endif

//
//
//

ULONG InitializeDirBlockHash(VOLUME *volume)
{
    volume->DirListBlocks = 0;
    volume->DirBlockHash = NWFSCacheAlloc(DIR_BLOCK_HASH_SIZE,
					  DIR_BLOCKHASH_TAG);

    if (!volume->DirBlockHash)
       return -1;

    volume->DirBlockHashLimit = NUMBER_OF_DIR_BLOCK_ENTRIES;
    NWFSSet(volume->DirBlockHash, 0, DIR_BLOCK_HASH_SIZE);

    return 0;
}

ULONG CreateDirBlockEntry(VOLUME *volume, ULONG cluster,
			 ULONG mirror, ULONG index)
{
    register DIR_BLOCK_HASH *hash;
    register ULONG retCode;

    hash = (DIR_BLOCK_HASH *) NWFSAlloc(sizeof(DIR_BLOCK_HASH), DIR_BLOCK_TAG);
    if (hash)
    {
       NWFSSet(hash, 0, sizeof(DIR_BLOCK_HASH));
       hash->Cluster1 = cluster;
       hash->Cluster2 = mirror;
       hash->BlockNo = index;

       retCode = AddToDirBlockHash(volume, hash);
       if (retCode)
       {
	  NWFSFree(hash);
	  return -1;
       }

       retCode = SetAssignedClusterValue(volume, cluster, 1);
       if (retCode)
       {
	  NWFSPrint("nwfs:  assigned bit block value not set cluster-%X\n",
		    (unsigned int)cluster);
       }

       retCode = SetAssignedClusterValue(volume, mirror, 1);
       if (retCode)
       {
	  NWFSPrint("nwfs:  assigned bit block value not set mirror-%X\n",
		    (unsigned int)mirror);
       }

       return 0;
    }
    return -1;

}

ULONG FreeDirBlockHash(VOLUME *volume)
{
    register DIR_BLOCK_HASH_LIST *HashTable;
    register DIR_BLOCK_HASH *list, *hash;
    register ULONG count;

    if (volume->DirBlockHash)
    {
       NWLockDirBlock(volume);
       HashTable = (DIR_BLOCK_HASH_LIST *) volume->DirBlockHash;
       for (count=0; count < volume->DirBlockHashLimit; count++)
       {

	  list = (DIR_BLOCK_HASH *) HashTable[count].head;
	  HashTable[count].head = HashTable[count].head = 0;
	  while (list)
	  {
	     hash = list;
	     list = list->next;
	     NWFSFree(hash);
	  }

       }
       NWUnlockDirBlock(volume);
    }

    if (volume->DirBlockHash)
       NWFSFree(volume->DirBlockHash);
    volume->DirBlockHashLimit = 0;
    volume->DirBlockHash = 0;

    return 0;

}

ULONG AddToDirBlockHash(VOLUME *volume, DIR_BLOCK_HASH *dblock)
{
    register ULONG hash;
    register DIR_BLOCK_HASH_LIST *HashTable;

    hash = (dblock->BlockNo & (volume->DirBlockHashLimit - 1));
    HashTable = (DIR_BLOCK_HASH_LIST *) volume->DirBlockHash;
    if (HashTable)
    {
       NWLockDirBlock(volume);
       if (!HashTable[hash].head)
       {
	  HashTable[hash].head = dblock;
	  HashTable[hash].tail = dblock;
	  dblock->next = dblock->prior = 0;
       }
       else
       {
	  HashTable[hash].tail->next = dblock;
	  dblock->next = 0;
	  dblock->prior = HashTable[hash].tail;
	  HashTable[hash].tail = dblock;
       }
       NWUnlockDirBlock(volume);
       return 0;
    }
    return -1;
}

ULONG RemoveDirBlockHash(VOLUME *volume, DIR_BLOCK_HASH *dblock)
{
    register ULONG hash;
    register DIR_BLOCK_HASH_LIST *HashTable;

    hash = (dblock->BlockNo & (volume->DirBlockHashLimit - 1));
    HashTable = (DIR_BLOCK_HASH_LIST *) volume->DirBlockHash;
    if (HashTable)
    {
       NWLockDirBlock(volume);
       if (HashTable[hash].head == dblock)
       {
	  HashTable[hash].head = dblock->next;
	  if (HashTable[hash].head)
	     HashTable[hash].head->prior = NULL;
	  else
	     HashTable[hash].tail = NULL;
       }
       else
       {
	  dblock->prior->next = dblock->next;
	  if (dblock != HashTable[hash].tail)
	     dblock->next->prior = dblock->prior;
	  else
	     HashTable[hash].tail = dblock->prior;
       }
       NWUnlockDirBlock(volume);
       return 0;
    }
    return -1;
}

ULONG IndexDeletedBlock(VOLUME *volume, DIR_BLOCK_HASH *dblock)
{
    DIR_BLOCK_HASH *old, *p;

    if (!volume->dblock_tail)
    {
       volume->dblock_head = dblock;
       volume->dblock_tail = dblock;
       dblock->dnext = dblock->dprior = 0;
    }
    else
    {
       p = volume->dblock_head;
       old = NULL;
       while (p)
       {
          if (p->DelBlockNo == dblock->DelBlockNo)
	  {
             NWFSPrint("duplicate deleted block sequence number detected\n");
             return -1;
	  }
          else 
          if (p->DelBlockNo < dblock->DelBlockNo)
          {
             old = p;
             p = p->dnext;
          }
          else
          {
	     if (p->dprior)
	     {
                p->dprior->dnext = dblock;
                dblock->dnext = p;
                dblock->dprior = p->dprior;
                p->dprior = dblock;
                goto dblock_inserted;
	     }
             dblock->dnext = p;
             dblock->dprior = NULL;
             p->dprior = dblock;
             volume->dblock_head = dblock;
             goto dblock_inserted;
          }
       }
       old->dnext = dblock;
       dblock->dnext = NULL;
       dblock->dprior = old;
       volume->dblock_tail = dblock;
    }

dblock_inserted:;
	  
    return 0;
}

ULONG AddDeletedBlock(VOLUME *volume, DIR_BLOCK_HASH *dblock)
{
    NWLockDirBlock(volume);
    if (!volume->dblock_head)
    {
       volume->dblock_head = dblock;
       volume->dblock_tail = dblock;
       dblock->dnext = dblock->dprior = 0;
    }
    else
    {
       volume->dblock_tail->dnext = dblock;
       dblock->dnext = 0;
       dblock->dprior = volume->dblock_tail;
       volume->dblock_tail = dblock;
    }
    NWUnlockDirBlock(volume);
    return 0;
}

ULONG RemoveDeletedBlock(VOLUME *volume, DIR_BLOCK_HASH *dblock)
{
    NWLockDirBlock(volume);
    if (volume->dblock_head == dblock)
    {
       volume->dblock_head = dblock->dnext;
       if (volume->dblock_head)
          volume->dblock_head->dprior = NULL;
       else
	  volume->dblock_tail = NULL;
    }
    else
    {
       dblock->dprior->dnext = dblock->dnext;
       if (dblock != volume->dblock_tail)
	  dblock->dnext->dprior = dblock->dprior;
       else
	  volume->dblock_tail = dblock->dprior;
    }
    NWUnlockDirBlock(volume);
    return 0;
}

//
//
//

ULONG InitializeDirAssignHash(VOLUME *volume)
{
    volume->DirAssignBlocks = 0;
    volume->DirAssignHash = NWFSCacheAlloc(ASSIGN_BLOCK_HASH_SIZE,
					   ASSN_BLOCKHASH_TAG);

    if (!volume->DirAssignHash)
       return -1;

    volume->DirAssignHashLimit = NUMBER_OF_ASSIGN_BLOCK_ENTRIES;
    NWFSSet(volume->DirAssignHash, 0, ASSIGN_BLOCK_HASH_SIZE);

    return 0;
}

ULONG CreateDirAssignEntry(VOLUME *volume, ULONG Parent, ULONG BlockNo,
			   DOS *dos)
{
    register ULONG DirsPerBlock;
    register DIR_ASSIGN_HASH *hash;
    register ULONG retCode, i;

    DirsPerBlock = volume->BlockSize / sizeof(ROOT);

    hash = NWFSAlloc(sizeof(DIR_ASSIGN_HASH), ASSN_BLOCK_TAG);
    if (hash)
    {
       NWFSSet(hash, 0, sizeof(DIR_ASSIGN_HASH));

       hash->DirOwner = Parent;
       hash->BlockNo = BlockNo;

       for (i=0; i < DirsPerBlock; i++)
       {
          switch (dos[i].Subdirectory)
	  {
	     case FREE_NODE:
	        hash->FreeList[i >> 3] &= ~(1 << (i & 7));
	        break;

	     case SUBALLOC_NODE:
 	     case TRUSTEE_NODE:
	     case RESTRICTION_NODE:
	     case ROOT_NODE:
		hash->FreeList[i >> 3] |= (1 << (i & 7));
		break;
		
	     default:
		hash->FreeList[i >> 3] |= (1 << (i & 7));
		break;
	  }
       }

       retCode = AddToDirAssignHash(volume, hash);
       if (retCode)
       {
	  NWFSFree(hash);
	  return -1;
       }
       return 0;
    }
    return -1;

}

ULONG FreeDirAssignHash(VOLUME *volume)
{
    register DIR_ASSIGN_HASH_LIST *HashTable;
    register DIR_ASSIGN_HASH *list, *hash;
    register ULONG count;

    if (volume->DirAssignHash)
    {
       NWLockDirAssign(volume);
       HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
       for (count=0; count < volume->DirAssignHashLimit; count++)
       {
	  list = (DIR_ASSIGN_HASH *) HashTable[count].head;
	  HashTable[count].head = HashTable[count].head = 0;
	  while (list)
	  {
	     hash = list;
	     list = list->next;

	     if (hash->DirOwner == (ULONG) -1)
		if (volume->FreeDirectoryBlockCount)
		   volume->FreeDirectoryBlockCount--;

	     NWFSFree(hash);
	  }
       }
       NWUnlockDirAssign(volume);
    }

    if (volume->DirAssignHash)
       NWFSFree(volume->DirAssignHash);
    volume->DirAssignHashLimit = 0;
    volume->DirAssignHash = 0;

    return 0;

}

ULONG AddToDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock)
{
    register ULONG hash;
    register DIR_ASSIGN_HASH_LIST *HashTable;

    hash = (dblock->DirOwner & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (HashTable)
    {
       NWLockDirAssign(volume);
       if (!HashTable[hash].head)
       {
	  HashTable[hash].head = dblock;
	  HashTable[hash].tail = dblock;
	  dblock->next = dblock->prior = 0;
       }
       else
       {
	  HashTable[hash].tail->next = dblock;
	  dblock->next = 0;
	  dblock->prior = HashTable[hash].tail;
	  HashTable[hash].tail = dblock;
       }

       if (dblock->DirOwner == (ULONG) -1)
	  volume->FreeDirectoryBlockCount++;

       NWUnlockDirAssign(volume);
       return 0;
    }
    return -1;
}

ULONG RemoveDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock)
{
    register ULONG hash;
    register DIR_ASSIGN_HASH_LIST *HashTable;

    hash = (dblock->DirOwner & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (HashTable)
    {
       NWLockDirAssign(volume);
       if (HashTable[hash].head == dblock)
       {
	  HashTable[hash].head = dblock->next;
	  if (HashTable[hash].head)
	     HashTable[hash].head->prior = NULL;
	  else
	     HashTable[hash].tail = NULL;
       }
       else
       {
	  dblock->prior->next = dblock->next;
	  if (dblock != HashTable[hash].tail)
	     dblock->next->prior = dblock->prior;
	  else
	     HashTable[hash].tail = dblock->prior;
       }

       if (dblock->DirOwner == (ULONG) -1)
	  if (volume->FreeDirectoryBlockCount)
	     volume->FreeDirectoryBlockCount--;

       NWUnlockDirAssign(volume);
       return 0;
    }
    return -1;
}

ULONG AddToDirAssignHashNoLock(VOLUME *volume, DIR_ASSIGN_HASH *dblock)
{
    register ULONG hash;
    register DIR_ASSIGN_HASH_LIST *HashTable;

    hash = (dblock->DirOwner & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (HashTable)
    {
       if (!HashTable[hash].head)
       {
	  HashTable[hash].head = dblock;
	  HashTable[hash].tail = dblock;
	  dblock->next = dblock->prior = 0;
       }
       else
       {
	  HashTable[hash].tail->next = dblock;
	  dblock->next = 0;
	  dblock->prior = HashTable[hash].tail;
	  HashTable[hash].tail = dblock;
       }

       if (dblock->DirOwner == (ULONG) -1)
	  volume->FreeDirectoryBlockCount++;

       return 0;
    }
    return -1;
}

ULONG RemoveDirAssignHashNoLock(VOLUME *volume, DIR_ASSIGN_HASH *dblock)
{
    register ULONG hash;
    register DIR_ASSIGN_HASH_LIST *HashTable;

    hash = (dblock->DirOwner & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (HashTable)
    {
       if (HashTable[hash].head == dblock)
       {
	  HashTable[hash].head = dblock->next;
	  if (HashTable[hash].head)
	     HashTable[hash].head->prior = NULL;
	  else
	     HashTable[hash].tail = NULL;
       }
       else
       {
	  dblock->prior->next = dblock->next;
	  if (dblock != HashTable[hash].tail)
	     dblock->next->prior = dblock->prior;
	  else
	     HashTable[hash].tail = dblock->prior;
       }

       if (dblock->DirOwner == (ULONG) -1)
	  if (volume->FreeDirectoryBlockCount)
	     volume->FreeDirectoryBlockCount--;

       return 0;
    }
    return -1;
}

ULONG AllocateDirectoryRecord(VOLUME *volume, ULONG Parent)
{
    register ULONG hash, i, DirsPerBlock, DirNo, DirsPerCluster;
    register DIR_ASSIGN_HASH_LIST *HashTable;
    register DIR_ASSIGN_HASH *list;
    register VOLUME_WORKSPACE *WorkSpace;
    register BYTE *cBuffer;
    register ULONG cbytes, vindex, retCode, block, BlockNo;
    register long cluster1, cluster2;
    register DOS *dos;
    ULONG retRCode;

    DirsPerBlock = volume->BlockSize / sizeof(ROOT);

    hash = (Parent & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (!HashTable)
    {
       register ULONG dirOwner, dirFileSize, dirBlocks, dirTotal, j;
       register ULONG FoundFreeRecord;
       register TRUSTEE *trustee;
       register USER *user;
       register ROOT *root;

       NWFSPrint("accessing raw directory mode\n");

       // we go ahead and allocate a full cluster of storage just in
       // case we need to extend the directory file;

       WorkSpace = AllocateWorkspace(volume);
       if (!WorkSpace)
	  return -1;
       cBuffer = &WorkSpace->Buffer[0];

       // get directory file size
       dirFileSize = GetChainSize(volume, volume->FirstDirectory);
       dirBlocks = (dirFileSize + (volume->BlockSize - 1)) / volume->BlockSize;
       dirTotal = (dirFileSize + (sizeof(DOS) - 1)) / sizeof(DOS);

       // scan each block until we find either the proper parent
       // directory block or we encounter a free block in the
       // dir file.

       for (j=0; j < dirBlocks; j++)
       {
	  cbytes = NWReadFile(volume,
			      &volume->FirstDirectory,
			      0,
			      dirFileSize,
			      j * volume->BlockSize,
			      cBuffer,
			      volume->BlockSize,
			      0,
			      0,
			      &retRCode,
			      KERNEL_ADDRESS_SPACE,
			      0,
			      0,
			      TRUE);
	  if (cbytes != volume->BlockSize)
	  {
	     NWFSPrint("nwfs:  error reading directory file\n");
	     FreeWorkspace(volume, WorkSpace);
	     return -1;
	  }

	  // scan through this directory block
	  for (FoundFreeRecord = dirOwner = (ULONG) -1, i=0;
	       i < DirsPerBlock; i++)
	  {
	     dos = (DOS *) &cBuffer[i * sizeof(DOS)];
	     switch (dos->Subdirectory)
	     {
		// root is owned by directory (0)
		case ROOT_NODE:
		   root = (ROOT *) dos;
		   if (dirOwner == (ULONG) -1)
		      dirOwner = 0;
		   break;

		// suballoc nodes are owned by the root (0)
		case SUBALLOC_NODE:
		   if (dirOwner == (ULONG) -1)
		      dirOwner = 0;
		   break;

		case FREE_NODE:
		   // remember the first free record found on this
		   // block.
		   if (FoundFreeRecord == (ULONG) -1)
		      FoundFreeRecord = i;
		   break;

		case RESTRICTION_NODE:
		   user = (USER *) dos;
		   if (dirOwner == (ULONG) -1)
		      dirOwner = user->Subdirectory;
		   break;

		case TRUSTEE_NODE:
		   trustee = (TRUSTEE *) dos;
		   if (dirOwner == (ULONG) -1)
		      dirOwner = trustee->Subdirectory;
		   break;

		default:
		   if (dirOwner == (ULONG) -1)
		      dirOwner = dos->Subdirectory;
		   break;
	     }
	  }

	  // if there was a free record found, then return its directory
	  // number
	  if (FoundFreeRecord != (ULONG) -1)
	  {
	     // The entire block is free.  return first element.
	     if (dirOwner == (ULONG) -1)
	     {
		dos = (DOS *) cBuffer;
		dos->Subdirectory = Parent;

		cbytes = NWWriteFile(volume,
				  &volume->FirstDirectory,
				  0,
				  j * volume->BlockSize,
				  cBuffer,
				  volume->BlockSize,
				  0,
				  0,
				  &retRCode,
				  KERNEL_ADDRESS_SPACE,
				  0,
				  0);
		if (cbytes != volume->BlockSize)
		{
		   NWFSPrint("nwfs:  error writing directory file\n");
		   FreeWorkspace(volume, WorkSpace);
		   return -1;
		}

		cbytes = NWWriteFile(volume,
				  &volume->SecondDirectory,
				  0,
				  j * volume->BlockSize,
				  cBuffer,
				  volume->BlockSize,
				  0,
				  0,
				  &retRCode,
				  KERNEL_ADDRESS_SPACE,
				  0,
				  0);
		if (cbytes != volume->BlockSize)
		{
		   NWFSPrint("nwfs:  error writing directory file\n");
		   FreeWorkspace(volume, WorkSpace);
		   return -1;
		}
		FreeWorkspace(volume, WorkSpace);
		return (j * DirsPerBlock);
	     }
	     else
	     if (dirOwner == Parent)
	     {
		dos = (DOS *) &cBuffer[FoundFreeRecord * sizeof(DOS)];
		dos->Subdirectory = Parent;

		cbytes = NWWriteFile(volume,
				  &volume->FirstDirectory,
				  0,
				  j * volume->BlockSize,
				  cBuffer,
				  volume->BlockSize,
				  0,
				  0,
				  &retRCode,
				  KERNEL_ADDRESS_SPACE,
				  0,
				  0);
		if (cbytes != volume->BlockSize)
		{
		   NWFSPrint("nwfs:  error writing directory file\n");
		   FreeWorkspace(volume, WorkSpace);
		   return -1;
		}

		cbytes = NWWriteFile(volume,
				  &volume->SecondDirectory,
				  0,
				  j * volume->BlockSize,
				  cBuffer,
				  volume->BlockSize,
				  0,
				  0,
				  &retRCode,
				  KERNEL_ADDRESS_SPACE,
				  0,
				  0);
		if (cbytes != volume->BlockSize)
		{
		   NWFSPrint("nwfs:  error writing directory file\n");
		   FreeWorkspace(volume, WorkSpace);
		   return -1;
		}
		FreeWorkspace(volume, WorkSpace);
		return ((j * DirsPerBlock) + FoundFreeRecord);
	     }
	  }
       }

       // if we get here, then we need to extend the directory file
       // by one cluster and fill this cluster with free entries because
       // the entire directory file is full.

       // fill cluster buffer with free directory record entries.
       NWFSSet(cBuffer, 0, volume->ClusterSize);
       for (i=0; i < (volume->ClusterSize / sizeof(DOS)); i++)
       {
	  dos = (DOS *) &cBuffer[i * sizeof(DOS)];
	  if (!i)
	     dos->Subdirectory = Parent; // assign first record
	  else
	     dos->Subdirectory = (ULONG) -1;
       }

       // write free cluster to the end of the primary directory chain
       cbytes = NWWriteFile(volume,
			    &volume->FirstDirectory,
			    0,
			    dirBlocks * volume->BlockSize,
			    cBuffer,
			    volume->ClusterSize,
			    0,
			    0,
			    &retRCode,
			    KERNEL_ADDRESS_SPACE,
			    0,
			    0);
       if (cbytes != volume->ClusterSize)
       {
	  NWFSPrint("nwfs:  error extending primary directory file\n");
	  FreeWorkspace(volume, WorkSpace);
	  return -1;
       }

       // write free cluster to the end of the mirror directory chain
       cbytes = NWWriteFile(volume,
			    &volume->SecondDirectory,
			    0,
			    dirBlocks * volume->BlockSize,
			    cBuffer,
			    volume->ClusterSize,
			    0,
			    0,
			    &retRCode,
			    KERNEL_ADDRESS_SPACE,
			    0,
			    0);
       if (cbytes != volume->ClusterSize)
       {
	  NWFSPrint("nwfs:  error extending mirror directory file\n");
	  FreeWorkspace(volume, WorkSpace);
	  return -1;
       }
       FreeWorkspace(volume, WorkSpace);
       return (dirBlocks * DirsPerBlock);
    }


    
    NWLockDirAssign(volume);
    list = (DIR_ASSIGN_HASH *) HashTable[hash].head;
    while (list)
    {
       if (list->DirOwner == Parent)
       {
	  for (i=0; i < DirsPerBlock; i++)
	  {
	     // if we locate a free entry in the bit list, set it
	     // then return the allocated DirNo
	     if (!(((list->FreeList[i >> 3]) >> (i & 7)) & 1))
	     {
		list->FreeList[i >> 3] |= (1 << (i & 7));
		DirNo = ((DirsPerBlock * list->BlockNo) + i);

		NWUnlockDirAssign(volume);
		return DirNo;
	     }
	  }
       }
       list = list->next;
    }

    // if we got here, then we could not locate an assigned directory
    // block with available entries.  at this point, we scan the free
    // list hash (-1) and search free directory blocks for available
    // entries.  if we find a free block, then we assign this
    // dir block as owned by the specified parent.

    hash = ((ULONG)FREE_NODE & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (!HashTable)
    {
       NWUnlockDirAssign(volume);
       return -1;
    }

    list = (DIR_ASSIGN_HASH *) HashTable[hash].head;
    while (list)
    {
       if (list->DirOwner == FREE_NODE)
       {
	  for (i=0; i < DirsPerBlock; i++)
	  {
	     // if we locate a free entry in the bit list, set it
	     // then return the allocated DirNo
	     if (!(((list->FreeList[i >> 3]) >> (i & 7)) & 1))
	     {
		// set directory entry as allocated
		list->FreeList[i >> 3] |= (1 << (i & 7));

		// re-hash block assign record to reflect
		// new parent
		RemoveDirAssignHashNoLock(volume, list);
		list->DirOwner = Parent;
		AddToDirAssignHashNoLock(volume, list);

		// calculate and return directory number
		DirNo = ((DirsPerBlock * list->BlockNo) + i);

		NWUnlockDirAssign(volume);
		return DirNo;
	     }
	  }
       }
       list = list->next;
    }
    NWUnlockDirAssign(volume);

    // if we got here, then the directory is completely full, and we need
    // to extend the directory by allocating another cluster for both the
    // primary and mirror copies of the directory.

    WorkSpace = AllocateWorkspace(volume);
    if (!WorkSpace)
       return -1;
    cBuffer = &WorkSpace->Buffer[0];

    // initialize new cluster and fill with free directory
    // entries
    NWFSSet(cBuffer, 0, volume->ClusterSize);

    DirsPerCluster = volume->ClusterSize / sizeof(ROOT);
    for (i=0; i < DirsPerCluster; i++)
    {
       dos = (DOS *) &cBuffer[i * sizeof(ROOT)];
       dos->Subdirectory = FREE_NODE;
    }

    // modifying global data, lock the volume structure
    NWLockVolume(volume);

    // set vindex to next cluster index
    vindex = volume->EndingDirIndex + 1;

    // allocate cluster for primary directory. we try to get both clusters
    // first before we start modifying the dir fat chain just in case we
    // run out of disk space. this makes error handling and unwinding of
    // a failed create much easier.

    cluster1 = AllocateClusterSetIndexSetChain(volume, vindex, -1);
    if (cluster1 == -1)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

    //  allocate cluster for mirrored directory
    cluster2 = AllocateClusterSetIndexSetChain(volume, vindex, -1);
    if (cluster2 == -1)
    {
       NWUnlockVolume(volume);
       FreeCluster(volume, cluster1);  // free the cluster
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

#if (ZERO_FILL_SECTORS)
    ZeroPhysicalVolumeCluster(volume, cluster1, DIRECTORY_PRIORITY);
    ZeroPhysicalVolumeCluster(volume, cluster2, DIRECTORY_PRIORITY);
#endif

    
    // now write both newly allocated clusters with a pre-initialized
    // cluster of free directory records.  if either write fails, then
    // free both clusters.  we have not spliced them into the dir fat
    // chain as of yet.

    cbytes = WriteClusterWithOffset(volume, cluster1, 0, cBuffer,
				    volume->ClusterSize, KERNEL_ADDRESS_SPACE,
				    &retRCode, DIRECTORY_PRIORITY);
    if (cbytes != volume->ClusterSize)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       FreeCluster(volume, cluster1);
       FreeCluster(volume, cluster2);
       return -1;
    }

    cbytes = WriteClusterWithOffset(volume, cluster2, 0, cBuffer,
				    volume->ClusterSize, KERNEL_ADDRESS_SPACE,
				    &retRCode, DIRECTORY_PRIORITY);
    if (cbytes != volume->ClusterSize)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       FreeCluster(volume, cluster1);
       FreeCluster(volume, cluster2);
       return -1;
    }

    // success, now we can link both the primary and mirrored
    // directory fat chains to point to these new clusters.

    if (volume->EndingDirCluster1 == (ULONG)-1)
       volume->FirstDirectory = cluster1;
    else
       SetClusterValue(volume, volume->EndingDirCluster1, cluster1);

    if (volume->EndingDirCluster2 == (ULONG)-1)
       volume->SecondDirectory = cluster2;
    else
       SetClusterValue(volume, volume->EndingDirCluster2, cluster2);

    // update ending cluster and ending index values
    volume->EndingDirCluster1 = cluster1;
    volume->EndingDirCluster2 = cluster2;
    volume->EndingDirIndex = vindex;

    // at this point, we have successfully extended both the primary and
    // mirrored directories.  now we update in memory meta-data and hash
    // structures to reflect the changes to the directory file.

    retCode = CreateDirBlockEntry(volume, cluster1, cluster2, vindex);
    if (retCode)
    {
       NWUnlockVolume(volume);
       NWFSPrint("nwfs:  could not allocate dir block during dir extend\n");
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

    // set BlockNo to the next logical block number
    BlockNo = volume->EndingBlockNo + 1;
    for (block=0; block < volume->BlocksPerCluster; block++, BlockNo++)
    {
       dos = (DOS *) &cBuffer[block * volume->BlockSize];
       retCode = CreateDirAssignEntry(volume, FREE_NODE, BlockNo, dos);
       if (retCode)
       {
	  NWUnlockVolume(volume);
	  NWFSPrint("nwfs:  could not allocate dir block Assign element during extend\n");
	  FreeWorkspace(volume, WorkSpace);
	  return -1;
       }
       volume->EndingBlockNo = BlockNo;  // save the new ending block number
    }

    NWUnlockVolume(volume);

    // free the workspace
    FreeWorkspace(volume, WorkSpace);

    // now try to scan the free list hash (-1) and search free
    // directory blocks for available entries.  if we find a free
    // block (we'd better, we just created one above), then we mark
    // this dir block as owned by the specified parent.  we should
    // never get here unless we successfully extended the directory file.

    hash = ((ULONG)FREE_NODE & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (!HashTable)
       return -1;

    NWLockDirAssign(volume);
    list = (DIR_ASSIGN_HASH *) HashTable[hash].head;
    while (list)
    {
       if (list->DirOwner == FREE_NODE)
       {
	  for (i=0; i < DirsPerBlock; i++)
	  {
	     // if we locate a free entry in the bit list, set it
	     // then return the allocated DirNo
	     if (!(((list->FreeList[i >> 3]) >> (i & 7)) & 1))
	     {
		// set directory entry as allocated
		list->FreeList[i >> 3] |= (1 << (i & 7));

		// re-hash block assign record to reflect
		// new parent
		RemoveDirAssignHashNoLock(volume, list);
		list->DirOwner = Parent;
		AddToDirAssignHashNoLock(volume, list);

		// calculate and return directory number
		DirNo = ((DirsPerBlock * list->BlockNo) + i);

		NWUnlockDirAssign(volume);
		return DirNo;
	     }
	  }
       }
       list = list->next;
    }
    NWUnlockDirAssign(volume);
    return -1;

}

ULONG FreeDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo,
			  ULONG Parent)
{
    register ULONG hash, i, count, SubdirNo;
    register ULONG DirsPerBlock, DirsPerCluster;
    register ULONG DirIndex, DirBlockNo;
    register ULONG retCode;
    register DIR_ASSIGN_HASH_LIST *HashTable;
    register DIR_ASSIGN_HASH *list;

    DirsPerCluster = volume->ClusterSize / sizeof(ROOT);
    DirsPerBlock = volume->BlockSize / sizeof(ROOT);
    DirBlockNo = DirNo / DirsPerBlock;
    DirIndex = DirNo % DirsPerBlock;

    hash = ((ULONG)Parent & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;

#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
    {
       ULONG retRCode, cbytes;
       ULONG dirFileSize, dirTotal;

       // get directory file size
       dirFileSize = GetChainSize(volume, volume->FirstDirectory);
       dirTotal = (dirFileSize + (sizeof(DOS) - 1)) / sizeof(DOS);

       if (DirNo >= dirTotal)
       {
	  NWFSPrint("nwfs:  directory number out of range (free)\n");
	  return -1;
       }

       NWFSSet(dos, 0, sizeof(ROOT));
       dos->Subdirectory = FREE_NODE;      // mark node free

       // write free cluster to the end of the primary directory chain
       cbytes = NWWriteFile(volume,
			    &volume->FirstDirectory,
			    0,
			    DirNo * sizeof(ROOT),
			    (BYTE *)dos,
			    sizeof(ROOT),
			    0,
			    0,
			    &retRCode,
			    KERNEL_ADDRESS_SPACE,
			    0,
			    0);
       if (cbytes != sizeof(ROOT))
       {
	  NWFSPrint("nwfs:  error extending primary directory file\n");
	  return -1;
       }

       // write free cluster to the end of the mirror directory chain
       cbytes = NWWriteFile(volume,
			    &volume->SecondDirectory,
			    0,
			    DirNo * sizeof(ROOT),
			    (BYTE *)dos,
			    sizeof(ROOT),
			    0,
			    0,
			    &retRCode,
			    KERNEL_ADDRESS_SPACE,
			    0,
			    0);
       if (cbytes != sizeof(ROOT))
       {
	  NWFSPrint("nwfs:  error extending mirror directory file\n");
	  return -1;
       }
       return 0;
    }
#else
    if (!HashTable)
       return NwHashCorrupt;
#endif

    NWLockDirAssign(volume);

    SubdirNo = Parent;
    NWFSSet(dos, 0, sizeof(ROOT));
    dos->Subdirectory = FREE_NODE;      // mark node free

    retCode = WriteDirectoryRecord(volume, dos, DirNo);
    if (retCode)
    {
       NWUnlockDirAssign(volume);
       NWFSPrint("nwfs:  error in directory write (%d)\n", (int)retCode);
       return retCode;
    }

    list = (DIR_ASSIGN_HASH *) HashTable[hash].head;
    while (list)
    {
       if ((list->BlockNo == DirBlockNo) &&
	   (list->DirOwner == Parent))
       {
	  // free directory record
	  list->FreeList[DirIndex >> 3] &= ~(1 << (DirIndex & 7));

	  // see if this block has any allocated dir entries, if so
	  // then exit, otherwise, re-hash the block into the
	  // free list

	  for (i=0; i < DirsPerBlock; i++)
	  {
	     // check for allocated entries
	     if (((list->FreeList[i >> 3]) >> (i & 7)) & 1)
	     {
		NWUnlockDirAssign(volume);
		return 0;
	     }
	  }

	  // if we get here the we assume that the entire directory
	  // block is now free.  we remove this block from the
	  // previous assigned parent hash, then re-hash it into the
	  // directory free list (-1)

	  RemoveDirAssignHashNoLock(volume, list);
	  list->DirOwner = FREE_NODE;
	  AddToDirAssignHashNoLock(volume, list);

	  NWUnlockDirAssign(volume);
	  return 0;
       }
       list = list->next;
    }

    // if we could not locate the directory entry, then perform
    // a brute force search of the record in all of the parent
    // assign hash records.  Netware does allow parent records
    // to be mixed within a single block in rare circumstances.
    // this should rarely happen, however, and if it ever does
    // it indicates possible directory corruption.  In any case,
    // even if the directory is corrupt, we should allow folks
    // the ability to purge these records.

    for (count=0; count < volume->DirAssignHashLimit; count++)
    {
       list = (DIR_ASSIGN_HASH *) HashTable[count].head;
       while (list)
       {
	  if (list->BlockNo == DirBlockNo)
	  {
	     NWFSPrint("nwfs:  record freed in non-assigned parent block [%X/%X]\n",
		       (unsigned int)SubdirNo,
		       (unsigned int)list->DirOwner);

	     // free directory record
	     list->FreeList[DirIndex >> 3] &= ~(1 << (DirIndex & 7));

	     // see if this block has any allocated dir entries, if so
	     // then exit, otherwise, re-hash the block into the
	     // free list

	     for (i=0; i < DirsPerBlock; i++)
	     {
		// check for allocated entries
		if (((list->FreeList[i >> 3]) >> (i & 7)) & 1)
		{
		   NWUnlockDirAssign(volume);
		   return 0;
		}
	     }

	     // if we get here the we assume that the entire directory
	     // block is now free.  we remove this block from the
	     // previous assigned parent hash, then re-hash it into the
	     // directory free list (-1)

	     RemoveDirAssignHashNoLock(volume, list);
	     list->DirOwner = FREE_NODE;
	     AddToDirAssignHashNoLock(volume, list);

	     NWUnlockDirAssign(volume);
	     return 0;
	  }
	  list = list->next;
       }
    }

    NWUnlockDirAssign(volume);
    return 0;

}

ULONG ReleaseDirectoryRecord(VOLUME *volume, ULONG DirNo, ULONG Parent)
{
    register ULONG hash, i, count;
    register ULONG DirsPerBlock, DirsPerCluster;
    register ULONG DirIndex, DirBlockNo;
    register DIR_ASSIGN_HASH_LIST *HashTable;
    register DIR_ASSIGN_HASH *list;

    DirsPerCluster = volume->ClusterSize / sizeof(ROOT);
    DirsPerBlock = volume->BlockSize / sizeof(ROOT);
    DirBlockNo = DirNo / DirsPerBlock;
    DirIndex = DirNo % DirsPerBlock;

    hash = ((ULONG)Parent & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;

#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
    if (!HashTable)
       return 0;
#else
    if (!HashTable)
       return NwHashCorrupt;
#endif

    NWLockDirAssign(volume);
    list = (DIR_ASSIGN_HASH *) HashTable[hash].head;
    while (list)
    {
       if ((list->BlockNo == DirBlockNo) &&
	   (list->DirOwner == Parent))
       {
	  // free directory record
	  list->FreeList[DirIndex >> 3] &= ~(1 << (DirIndex & 7));

	  // see if this block has any allocated dir entries, if so
	  // then exit, otherwise, re-hash the block into the
	  // free list

	  for (i=0; i < DirsPerBlock; i++)
	  {
	     // check for allocated entries
	     // if we find an allocated record, then exit
	     if (((list->FreeList[i >> 3]) >> (i & 7)) & 1)
	     {
		NWUnlockDirAssign(volume);
		return 0;
	     }
	  }

	  // if we get here the we assume that the entire directory
	  // block is now free.  we remove this block from the
	  // previous assigned parent hash, then re-hash it into the
	  // directory free list (-1)

	  RemoveDirAssignHashNoLock(volume, list);
	  list->DirOwner = FREE_NODE;
	  AddToDirAssignHashNoLock(volume, list);

	  NWUnlockDirAssign(volume);
	  return 0;
       }
       list = list->next;
    }

    // if we could not locate the directory entry, then perform
    // a brute force search of the record in all of the parent
    // assign hash records.  Netware does allow parent records
    // to be mixed within a single block in rare circumstances.
    // this should rarely happen, however, and if it ever does
    // it indicates possible directory corruption.  In any case,
    // even if the directory is corrupt, we should allow folks
    // the ability to purge these records.

    for (count=0; count < volume->DirAssignHashLimit; count++)
    {
       list = (DIR_ASSIGN_HASH *) HashTable[count].head;
       while (list)
       {
	  if (list->BlockNo == DirBlockNo)
	  {
	     NWFSPrint("nwfs:  record freed in non-assigned parent block [%X/%X]\n",
		       (unsigned int)Parent,
		       (unsigned int)list->DirOwner);

	     // free directory record
	     list->FreeList[DirIndex >> 3] &= ~(1 << (DirIndex & 7));

	     // see if this block has any allocated dir entries, if so
	     // then exit, otherwise, re-hash the block into the
	     // free list

	     for (i=0; i < DirsPerBlock; i++)
	     {
		// check for allocated entries
		// if we find an allocated record, then exit
		if (((list->FreeList[i >> 3]) >> (i & 7)) & 1)
		{
		   NWUnlockDirAssign(volume);
		   return 0;
		}
	     }

	     // if we get here the we assume that the entire directory
	     // block is now free.  we remove this block from the
	     // previous assigned parent hash, then re-hash it into the
	     // directory free list (-1)

	     RemoveDirAssignHashNoLock(volume, list);
	     list->DirOwner = FREE_NODE;
	     AddToDirAssignHashNoLock(volume, list);

	     NWUnlockDirAssign(volume);
	     return 0;
	  }
	  list = list->next;
       }
    }
    NWUnlockDirAssign(volume);
    return NwNoEntry;
}

//  This function pre-allocates a cluster of free directory records
//  then creates an assignment hash and a block hash for the blocks
//  that make up this cluster.

ULONG PreAllocateFreeDirectoryRecords(VOLUME *volume)
{
    register ULONG hash, i, DirsPerBlock, DirsPerCluster;
    register DIR_ASSIGN_HASH_LIST *HashTable;
    register DIR_ASSIGN_HASH *list;
    register VOLUME_WORKSPACE *WorkSpace;
    register BYTE *cBuffer;
    register ULONG cbytes, vindex, retCode, block, BlockNo;
    register long cluster1, cluster2;
    register DOS *dos;
    ULONG retRCode;

    DirsPerBlock = volume->BlockSize / sizeof(ROOT);
    hash = ((ULONG)FREE_NODE & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (!HashTable)
       return -1;

    NWLockDirAssign(volume);
    list = (DIR_ASSIGN_HASH *) HashTable[hash].head;
    while (list)
    {
       if (list->DirOwner == FREE_NODE)
       {
	  for (i=0; i < DirsPerBlock; i++)
	  {
	     // if we locate a free entry in the bit list
	     // then return
	     if (!(((list->FreeList[i >> 3]) >> (i & 7)) & 1))
	     {
		NWUnlockDirAssign(volume);
		return 0;
	     }
	  }
       }
       list = list->next;
    }
    NWUnlockDirAssign(volume);

    // no free directory space left, allocate a new cluster for the free list
    WorkSpace = AllocateWorkspace(volume);
    if (!WorkSpace)
       return -1;
    cBuffer = &WorkSpace->Buffer[0];

    // initialize new cluster and fill with free directory entries
    NWFSSet(cBuffer, 0, volume->ClusterSize);

    DirsPerCluster = volume->ClusterSize / sizeof(ROOT);
    for (i=0; i < DirsPerCluster; i++)
    {
       dos = (DOS *) &cBuffer[i * sizeof(ROOT)];
       dos->Subdirectory = FREE_NODE;
    }

    // modifying global data, lock the volume structure
    NWLockVolume(volume);

    // set vindex to next cluster index
    vindex = volume->EndingDirIndex + 1;

    // allocate cluster for primary directory. we try to get both clusters
    // first before we start modifying the dir fat chain just in case we
    // run out of disk space. this makes error handling and unwinding of
    // a failed create much easier.

    cluster1 = AllocateClusterSetIndexSetChain(volume, vindex, -1);
    if (cluster1 == -1)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

    //  allocate cluster for mirrored directory
    cluster2 = AllocateClusterSetIndexSetChain(volume, vindex, -1);
    if (cluster2 == -1)
    {
       NWUnlockVolume(volume);
       FreeCluster(volume, cluster1);  // free the cluster
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

#if (ZERO_FILL_SECTORS)
    ZeroPhysicalVolumeCluster(volume, cluster1, DIRECTORY_PRIORITY);
    ZeroPhysicalVolumeCluster(volume, cluster2, DIRECTORY_PRIORITY);
#endif

    // now write both newly allocated clusters with a pre-initialized
    // cluster of free directory records.  if either write fails, then
    // free both clusters.  we have not spliced them into the dir fat
    // chain as of yet.

    cbytes = WriteClusterWithOffset(volume, cluster1, 0, cBuffer,
				    volume->ClusterSize, KERNEL_ADDRESS_SPACE,
				    &retRCode, DIRECTORY_PRIORITY);
    if (cbytes != volume->ClusterSize)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       FreeCluster(volume, cluster1);
       FreeCluster(volume, cluster2);
       return -1;
    }

    cbytes = WriteClusterWithOffset(volume, cluster2, 0, cBuffer,
				    volume->ClusterSize, KERNEL_ADDRESS_SPACE,
				    &retRCode, DIRECTORY_PRIORITY);
    if (cbytes != volume->ClusterSize)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       FreeCluster(volume, cluster1);
       FreeCluster(volume, cluster2);
       return -1;
    }

    // success, now we can link both the primary and mirrored
    // directory fat chains to point to these new clusters.

    if (volume->EndingDirCluster1 == (ULONG)-1)
       volume->FirstDirectory = cluster1;
    else
       SetClusterValue(volume, volume->EndingDirCluster1, cluster1);

    if (volume->EndingDirCluster2 == (ULONG)-1)
       volume->SecondDirectory = cluster2;
    else
       SetClusterValue(volume, volume->EndingDirCluster2, cluster2);

    // update ending cluster and ending index values
    volume->EndingDirCluster1 = cluster1;
    volume->EndingDirCluster2 = cluster2;
    volume->EndingDirIndex = vindex;

    // at this point, we have successfully extended both the primary and
    // mirrored directories.  now we update in memory meta-data and hash
    // structures to reflect the changes to the directory file.

    retCode = CreateDirBlockEntry(volume, cluster1, cluster2, vindex);
    if (retCode)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

    // set BlockNo to the next logical block number
    BlockNo = volume->EndingBlockNo + 1;
    for (block=0; block < volume->BlocksPerCluster; block++, BlockNo++)
    {
       dos = (DOS *) &cBuffer[block * volume->BlockSize];
       retCode = CreateDirAssignEntry(volume, FREE_NODE, BlockNo, dos);
       if (retCode)
       {
	  NWUnlockVolume(volume);
	  FreeWorkspace(volume, WorkSpace);
	  return -1;
       }
       volume->EndingBlockNo = BlockNo;  // save the new ending block number
    }

    NWUnlockVolume(volume);

    // free the workspace
    FreeWorkspace(volume, WorkSpace);

    return 0;
}

//  This function pre-allocates a cluster of free directory records
//  but does not create any assignment or block hash records.

ULONG PreAllocateEmptyDirectorySpace(VOLUME *volume)
{
    register ULONG i, DirsPerCluster;
    register VOLUME_WORKSPACE *WorkSpace;
    register BYTE *cBuffer;
    register ULONG cbytes, vindex, retCode, block, BlockNo;
    register long cluster1, cluster2;
    register DOS *dos;
    ULONG retRCode;

    // allocate a new cluster for the free list
    WorkSpace = AllocateWorkspace(volume);
    if (!WorkSpace)
       return -1;
    cBuffer = &WorkSpace->Buffer[0];

    // initialize new cluster and fill with free directory entries
    NWFSSet(cBuffer, 0, volume->ClusterSize);

    DirsPerCluster = volume->ClusterSize / sizeof(ROOT);
    for (i=0; i < DirsPerCluster; i++)
    {
       dos = (DOS *) &cBuffer[i * sizeof(ROOT)];
       dos->Subdirectory = FREE_NODE;
    }

    // modifying global data, lock the volume structure
    NWLockVolume(volume);

    // set vindex to next cluster index
    vindex = volume->EndingDirIndex + 1;

    // allocate cluster for primary directory. we try to get both clusters
    // first before we start modifying the dir fat chain just in case we
    // run out of disk space. this makes error handling and unwinding of
    // a failed create much easier.

    cluster1 = AllocateClusterSetIndexSetChain(volume, vindex, -1);
    if (cluster1 == -1)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

    //  allocate cluster for mirrored directory
    cluster2 = AllocateClusterSetIndexSetChain(volume, vindex, -1);
    if (cluster2 == -1)
    {
       NWUnlockVolume(volume);
       FreeCluster(volume, cluster1);  // free the cluster
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

#if (ZERO_FILL_SECTORS)
    ZeroPhysicalVolumeCluster(volume, cluster1, DIRECTORY_PRIORITY);
    ZeroPhysicalVolumeCluster(volume, cluster2, DIRECTORY_PRIORITY);
#endif

    // now write both newly allocated clusters with a pre-initialized
    // cluster of free directory records.  if either write fails, then
    // free both clusters.  we have not spliced them into the dir fat
    // chain as of yet.

    cbytes = WriteClusterWithOffset(volume, cluster1, 0, cBuffer,
				    volume->ClusterSize, KERNEL_ADDRESS_SPACE,
				    &retRCode, DIRECTORY_PRIORITY);
    if (cbytes != volume->ClusterSize)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       FreeCluster(volume, cluster1);
       FreeCluster(volume, cluster2);
       return -1;
    }

    cbytes = WriteClusterWithOffset(volume, cluster2, 0, cBuffer,
				    volume->ClusterSize, KERNEL_ADDRESS_SPACE,
				    &retRCode, DIRECTORY_PRIORITY);
    if (cbytes != volume->ClusterSize)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       FreeCluster(volume, cluster1);
       FreeCluster(volume, cluster2);
       return -1;
    }

    // success, now we can link both the primary and mirrored
    // directory fat chains to point to these new clusters.

    if (volume->EndingDirCluster1 == (ULONG)-1)
       volume->FirstDirectory = cluster1;
    else
       SetClusterValue(volume, volume->EndingDirCluster1, cluster1);

    if (volume->EndingDirCluster2 == (ULONG)-1)
       volume->SecondDirectory = cluster2;
    else
       SetClusterValue(volume, volume->EndingDirCluster2, cluster2);

    // update ending cluster and ending index values
    volume->EndingDirCluster1 = cluster1;
    volume->EndingDirCluster2 = cluster2;
    volume->EndingDirIndex = vindex;

    // at this point, we have successfully extended both the primary and
    // mirrored directories.  now we update in memory meta-data and hash
    // structures to reflect the changes to the directory file.

    retCode = CreateDirBlockEntry(volume, cluster1, cluster2, vindex);
    if (retCode)
    {
       NWUnlockVolume(volume);
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

    // set BlockNo to the next logical block number
    BlockNo = volume->EndingBlockNo + 1;
    for (block=0; block < volume->BlocksPerCluster; block++, BlockNo++)
    {
       dos = (DOS *) &cBuffer[block * volume->BlockSize];
       retCode = CreateDirAssignEntry(volume, FREE_NODE, BlockNo, dos);
       if (retCode)
       {
	  NWUnlockVolume(volume);
	  FreeWorkspace(volume, WorkSpace);
	  return -1;
       }
       volume->EndingBlockNo = BlockNo;  // save the new ending block number
    }

    NWUnlockVolume(volume);

    // free the workspace
    FreeWorkspace(volume, WorkSpace);

    return 0;
}
