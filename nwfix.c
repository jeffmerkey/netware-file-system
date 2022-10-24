
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
*   FILE     :  NWFIX.C
*   DESCRIP  :   NWFS Volume Repair Module
*   DATE     :  July 15, 2000
*
*
***************************************************************************/

#include "globals.h"

void NWLockScan(void);
void NWUnlockScan(void);

ULONG UtilInitializeDirAssignHash(VOLUME *volume);
ULONG UtilCreateDirAssignEntry(VOLUME *volume, ULONG Parent, ULONG BlockNo,
			   DOS *dos);
ULONG UtilFreeDirAssignHash(VOLUME *volume);
ULONG UtilAddToDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock);
ULONG UtilRemoveDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock);
ULONG UtilAllocateDirectoryRecord(VOLUME *volume, ULONG Parent);
ULONG UtilFreeDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo,
			      ULONG Parent);

typedef struct _NWREPAIR_STATS 
{
   ULONG max_entries;
   ULONG bad_trustee_records;
   ULONG bad_trustee_mask;
   ULONG bad_name_records;
   ULONG bad_root_records;
   ULONG bad_root_flags;
   ULONG bad_dos_records;
   ULONG bad_mac_records;
   ULONG bad_nfs_records;
   ULONG bad_nfs_extants;
   ULONG bad_long_records;
   ULONG bad_long_extants;
   ULONG bad_nt_records;
   ULONG bad_nt_extants;
   ULONG bad_suballoc_records;
   ULONG bad_user_records;
   ULONG bad_free_records;
   ULONG cross_linked_files;
   ULONG orphaned_fat_chains;
   ULONG unowned_trustee_records;
   ULONG orphaned_name_entries;
} NWREPAIR_STATS; 

ULONG StrictValidateDirectoryRecord(VOLUME *volume, DOS *dos, 
		                    ULONG DirNo, NWREPAIR_STATS *stats);

ULONG RepairTwoFiles(VOLUME *volume, ULONG *c1, ULONG *c2);

ULONG NWRepairVolume(VOLUME *volume, ULONG flags)
{
    register ULONG Dir1Size, Dir2Size;
    register ULONG eDir1Size, eDir2Size;
    register ULONG i, j, owner, ccode, cbytes;
    register ULONG DirTotal, DirCount, DirsPerBlock, DirBlocks;
    register DOS *dos;
    register USER *user;
    register ROOT *root;
    register TRUSTEE *trustee;
    nwvp_fat_fix_info fi;
    ULONG retCode = 0;
    NWREPAIR_STATS stats;
    
    NWFSSet(&stats, 0, sizeof(NWREPAIR_STATS));

    NWFSPrint("[ Repairing Volume FAT Tables ]\n"); 

    NWLockScan();
    if (!volume->VolumePresent)
    {
       NWFSPrint("nwfs:  volume %s is missing one or more segments\n", 
		 volume->VolumeName);
       NWUnlockScan();
       return NwFatCorrupt;
    }

    if (VolumeMountedTable[volume->VolumeNumber])
    {
       NWFSPrint("nwfs:  volume %s disk(%d) is currently mounted\n",
	         volume->VolumeName, (int)volume->VolumeDisk);
       NWUnlockScan();
       return NwFatCorrupt;
    }

    if (nwvp_vpartition_open(volume->nwvp_handle) != 0)
    {
       NWFSPrint("nwfs:  volume %s could not attach to NVMP\n",
		 volume->VolumeName);
       NWUnlockScan();
       return NwFatCorrupt;
    }

    ccode = nwvp_vpartition_fat_fix(volume->nwvp_handle, &fi, TRUE);
    if (ccode)
    {
       NWFSPrint("nwrepair:  FAT tables could not be repaired\n");
       NWUnlockScan();
       return NwFatCorrupt;
    }
    nwvp_vpartition_close(volume->nwvp_handle);

    NWUnlockScan();
    
    NWFSPrint("total_fat_blocks      : %d ",(int)fi.total_fat_blocks);
    NWFSPrint("valid_mirror_blocks   : %d\n",(int)fi.total_valid_mirror_blocks);
    NWFSPrint("valid_single_blocks   : %d ",(int)fi.total_valid_single_blocks);
    NWFSPrint("missing_blocks        : %d\n",(int)fi.total_missing_blocks);
    NWFSPrint("mismatched_blocks     : %d ",(int)fi.total_mismatch_blocks);
    NWFSPrint("table_entries         : %d\n",(int)fi.total_entries);
    NWFSPrint("index_mismatches      : %d ",(int)fi.total_index_mismatch_entries);
    NWFSPrint("end_null_entries      : %d\n",(int)fi.total_end_null_entries);
    NWFSPrint("suballoc_null_entries : %d ",(int)fi.total_SA_null_entries);
    NWFSPrint("fat_null_entries      : %d\n",(int)fi.total_FAT_null_entries);
    NWFSPrint("suballoc_mismatches   : %d ",(int)fi.total_SA_mismatch_entries);
    NWFSPrint("fat_mismatch_entries  : %d\n",(int)fi.total_FAT_mismatch_entries);
    
    NWFSPrint("[ Verifying Volume FAT Tables ]\n");
    
    ccode = MountRawVolume(volume);
    if (ccode)
    {
       NWFSPrint("nwrepair:  FAT Tables failed verify\n");
       return NwVolumeCorrupt;
    }
    
    dos = NWFSCacheAlloc(IO_BLOCK_SIZE, DIR_WORKSPACE_TAG);
    if (!dos)
    {
       DismountRawVolume(volume);
       NWFSPrint("nwrepair:  not enough memory to repair volume\n");
       return NwInsufficientResources;
    }

    NWFSPrint("[ Verifying Directory Primary/Mirror Chains ]\n");
    if (!volume->FirstDirectory || !volume->SecondDirectory)
    {
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSPrint("nwrepair:  directory primary/mirror files are mismatched\n"); 
       return NwDirectoryCorrupt;
    }
    
    Dir1Size = GetChainSize(volume, volume->FirstDirectory);
    Dir2Size = GetChainSize(volume, volume->SecondDirectory);
    if (!Dir1Size || !Dir2Size || (Dir1Size != Dir2Size))
    {
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSPrint("nwrepair:  directory primary/mirror file mismatch %d-%d\n", 
			  (int)Dir1Size, (int)Dir2Size); 
       return NwDirectoryCorrupt;
    }
    
    NWFSPrint("[ Verifying Extended Directory Primary/Mirror Chains ]\n");
    if (!volume->ExtDirectory1 && !volume->ExtDirectory2)
       NWFSPrint("nwrepair:  ext directory primary/mirror files are NULL\n"); 
    else
    if (volume->ExtDirectory1 && !volume->ExtDirectory2)
       volume->ExtDirectory2 = volume->ExtDirectory1;
    else
    if (!volume->ExtDirectory1 && volume->ExtDirectory2)
       volume->ExtDirectory1 = volume->ExtDirectory2;
   
    if (volume->ExtDirectory1 && volume->ExtDirectory2)
    {
       eDir1Size = GetChainSize(volume, volume->ExtDirectory1);
       eDir2Size = GetChainSize(volume, volume->ExtDirectory2);
       if (eDir1Size != eDir2Size)
       {
          DismountRawVolume(volume);
          NWFSFree(dos);
          NWFSPrint("nwrepair: ext dir primary/mirror file mismatch %d-%d\n",
			  (int)eDir1Size, (int)eDir2Size); 
          return NwDirectoryCorrupt;
       }
    }
    
    DirsPerBlock = volume->BlockSize / sizeof(ROOT);
    DirBlocks = (Dir1Size + (volume->BlockSize - 1)) / volume->BlockSize;
    DirTotal = (Dir1Size + (sizeof(DOS) - 1)) / sizeof(DOS);
    stats.max_entries = DirTotal;
    
    NWFSPrint("[ Directory Pass 1 -- Verifying Directory Records ]\n");
    
    for (DirCount = i = 0; i < DirBlocks; i++)
    {
       cbytes = NWReadFile(volume,
			   &volume->FirstDirectory,
			   0,
			   Dir1Size,
			   i * volume->BlockSize,
			   (BYTE *)dos,
			   volume->BlockSize,
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
                           0,
			   TRUE);

       if (cbytes != volume->BlockSize)
       {
	  NWFSPrint("nwfs:  error reading directory file\n");
	  UtilFreeDirAssignHash(volume);
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  return NwInsufficientResources;
       }

       for (owner = (ULONG) -1, j=0; j < DirsPerBlock; j++, DirCount++)
       {
          ccode = StrictValidateDirectoryRecord(volume, &dos[j], 
			                        DirCount, &stats);
       }

       cbytes = NWWriteFile(volume,
			   &volume->FirstDirectory,
			   0,
			   i * volume->BlockSize,
			   (BYTE *)dos,
			   volume->BlockSize,
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
                           0);
       
       if (cbytes != volume->BlockSize)
	  NWFSPrint("nwfs:  error writing directory primary file\n");
       
       cbytes = NWWriteFile(volume,
			   &volume->SecondDirectory,
			   0,
			   i * volume->BlockSize,
			   (BYTE *)dos,
			   volume->BlockSize,
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
                           0);

       if (cbytes != volume->BlockSize)
	  NWFSPrint("nwfs:  error writing directory mirror file\n");
    }

    NWFSPrint("[ Directory Pass 2 -- Verifying Root and Suballocation ]\n");
    
    NWFSPrint("[ Directory Pass 3 -- Verifying Name Link Consistency ]\n");
    
    NWFSPrint("[ Directory Pass 4 -- Checking User and Trustee Records ]\n");
    
    NWFSPrint("[ Directory Pass 5 -- Checking for Cross Linked Files ]\n");
        
    NWFSPrint("[ Directory Pass 6 -- Converting Lost Chains to Files\n");
    
    ccode = UtilInitializeDirAssignHash(volume);
    if (ccode)
    {
       NWFSPrint("nwfs:  error during assign hash init\n");
       DismountRawVolume(volume);
       NWFSFree(dos);
       return NwInsufficientResources;
    }

    for (DirCount = i = 0; i < DirBlocks; i++)
    {
       cbytes = NWReadFile(volume,
			   &volume->FirstDirectory,
			   0,
			   Dir1Size,
			   i * volume->BlockSize,
			   (BYTE *)dos,
			   volume->BlockSize,
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
                           0,
			   TRUE);

       if (cbytes != volume->BlockSize)
       {
	  NWFSPrint("nwfs:  error reading directory file\n");
	  UtilFreeDirAssignHash(volume);
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  return NwInsufficientResources;
       }

       for (owner = (ULONG) -1, j=0; j < DirsPerBlock; j++, DirCount++)
       {
          ccode = StrictValidateDirectoryRecord(volume, dos, DirCount, &stats);
          if (ccode)
          {
	  }

	  switch (dos[j].Subdirectory)
	  {
	     case ROOT_NODE:
		if (owner == (ULONG) -1)
		   owner = 0;
		root = (ROOT *) &dos[j];
		if (root->NameSpace == DOS_NAME_SPACE)
		   DirCount++;
		break;

	     case SUBALLOC_NODE:
		if (owner == (ULONG) -1)
		   owner = 0;
		break;

	     case FREE_NODE:
		break;

	     case RESTRICTION_NODE:
		user = (USER *) &dos[j];
		if (owner == (ULONG) -1)
		   owner = 0;
		break;

	     case TRUSTEE_NODE:
		trustee = (TRUSTEE *) &dos[j];
		if (owner == (ULONG) -1)
		   owner = 0;
		break;

	     default:
		if (owner == (ULONG) -1)
		   owner = dos[j].Subdirectory;

		if (dos[j].NameSpace == DOS_NAME_SPACE)
		   DirCount++;
		break;
	  }
       }

       ccode = UtilCreateDirAssignEntry(volume, owner, i, dos);
       if (ccode)
       {
	  NWFSPrint("nwrepair:  error creating dir assign hash\n");
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  return NwInsufficientResources;
       }
    }

    UtilFreeDirAssignHash(volume);
    DismountRawVolume(volume);
    NWFSFree(dos);

    NWFSPrint("bad_trustee_records     : %d ", 
		    (int)stats.bad_trustee_records);
    NWFSPrint("bad_trustee_mask        : %d\n", (int)stats.bad_trustee_mask);
    NWFSPrint("bad_name_records        : %d ", (int)stats.bad_name_records);
    NWFSPrint("bad_root_records        : %d\n", (int)stats.bad_root_records);
    NWFSPrint("bad_root_flags          : %d ", (int)stats.bad_root_flags);
    NWFSPrint("bad_dos_records         : %d\n", (int)stats.bad_dos_records);
    NWFSPrint("bad_mac_records         : %d ", (int)stats.bad_mac_records);
    NWFSPrint("bad_nfs_records         : %d\n", (int)stats.bad_nfs_records);
    NWFSPrint("bad_nfs_extants         : %d ", (int)stats.bad_nfs_extants);
    NWFSPrint("bad_long_records        : %d\n", (int)stats.bad_long_records);
    NWFSPrint("bad_long_extants        : %d ", (int)stats.bad_long_extants);
    NWFSPrint("bad_nt_records          : %d\n", (int)stats.bad_nt_records);
    NWFSPrint("bad_nt_extants          : %d ", (int)stats.bad_nt_extants);
    NWFSPrint("bad_suballoc_records    : %d\n",(int)stats.bad_suballoc_records);
    NWFSPrint("bad_user_records        : %d ", (int)stats.bad_user_records);
    NWFSPrint("bad_free_records        : %d\n", (int)stats.bad_free_records);
    NWFSPrint("cross_linked_files      : %d ", (int)stats.cross_linked_files);
    NWFSPrint("orphaned_fat_chains     : %d\n", (int)stats.orphaned_fat_chains);
    NWFSPrint("unowned_trustee_records : %d ",
		    (int)stats.unowned_trustee_records);
    NWFSPrint("orphaned_name_entries   : %d\n",
		    (int)stats.orphaned_name_entries);

    NWFSPrint("[ Freeing Unassigned Clusters \n");
    
    return 0;
}

ULONG RepairTwoFiles(VOLUME *volume, ULONG *c1, ULONG *c2)
{
    return 0;
}

ULONG StrictValidateDirectoryRecord(VOLUME *volume, DOS *dos, 
		                    ULONG DirNo, NWREPAIR_STATS *stats)
{
    switch (dos->Subdirectory)
    {
       case FREE_NODE:
          if (NWFSCompare(&dos->FileAttributes, ZeroBuffer, 
	     (sizeof(DOS) - sizeof(ULONG))))
	  {
             // repair free record
     	     NWFSSet(dos, 0, sizeof(DOS));
	     dos->Subdirectory = (ULONG)-1;

	     stats->bad_free_records++;
	     return 1;
	  }
          return 0;

       case TRUSTEE_NODE:
	  {
             register ULONG i;
     	     TRUSTEE *trustee = (TRUSTEE *)dos;

	     if ((trustee->Reserved1[0]) || (trustee->TrusteeCount > 16))
	     {
#if (VERBOSE)
     	        NWFSPrint("Invalid trustee record res1-%d trustees-%d\n",
			  (int)trustee->Reserved1[0], 
			  (int)trustee->TrusteeCount);
#endif
		stats->bad_trustee_records++;
     	        return 1;
	     }

	     for (i=0; i < 16; i++)
	     {
                if (trustee->TrusteeMask[i] & ~TRUSTEE_VALID_MASK)
		{
#if (VERBOSE)
                   NWFSPrint("Invalid trustee record mask-%08X MASK-%08X\n",
			  (unsigned)trustee->TrusteeMask[i], 
			  (unsigned)TRUSTEE_VALID_MASK);
#endif
		   stats->bad_trustee_mask++;
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
#if (VERBOSE)
                NWFSPrint("Namespace/count (%d-%d) was > 10\n", 
			   (int)root->NameSpace, (int)root->NameSpaceCount);
#endif
		stats->bad_root_records++;
     	        return 1;
	     }
             
	     if (root->NameSpace != DOS_NAME_SPACE)
	        break;
	     
	     if ((volume->VolumeFlags & NDS_FLAG) ||
	         (volume->VolumeFlags & NEW_TRUSTEE_COUNT))
	     {
	        if (root->VolumeFlags & ~VFLAGS_4X_MASK)
		{
#if (VERBOSE)
                   NWFSPrint("root 4X volume flags invalid (%08X) mask-%08X\n",
			     (unsigned)root->VolumeFlags, 
			     (unsigned)VFLAGS_4X_MASK);
#endif
		   stats->bad_root_flags++;
	           return 1;
		}
	     }
	     else
	     {
	        if (root3x->VolumeFlags & ~VFLAGS_3X_MASK)
		{
#if (VERBOSE)
                   NWFSPrint("root 3X volume flags invalid (%08X) mask-%08X\n",
			     (unsigned)root3x->VolumeFlags, 
			     (unsigned)VFLAGS_3X_MASK);
#endif
		   stats->bad_root_flags++;
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
#if (VERBOSE)
                NWFSPrint("Invalid user record res1-%d trustees-%d\n",
			  (int)user->Reserved1, 
			  (int)user->TrusteeCount);
#endif
   	        stats->bad_user_records++;
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
#if (VERBOSE)
                NWFSPrint("Invalid suballocation record res-%d sequence-%d\n",
			  (int)suballoc->Reserved1, 
			  (int)suballoc->SequenceNumber);
#endif
		stats->bad_suballoc_records++;
		return 1;
	     }

             for (i=0; i < 28; i++)
	     {
                // if any suballocation table entries exceed volume 
	        // clusters, zero the invalid fat heads.
     	        if (suballoc->StartingFATChain[i] > volume->VolumeClusters)
                   suballoc->StartingFATChain[i] = 0;
	     }
	  }
	  return 0;

       default:
	  if (dos->NameSpace > 10)
	  {
#if (VERBOSE)
	     NWFSPrint("Invalid namespace record %d\n", (int)dos->NameSpace);
#endif
             // set record as free
     	     NWFSSet(dos, 0, sizeof(DOS));
	     dos->Subdirectory = (ULONG)-1;

   	     stats->bad_name_records++;
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
		   if (NWValidDOSName(volume, dos->FileName,
				      dos->FileNameLength, 0, 0))
		   {
#if (VERBOSE)
                      NWFSPrint("Invalid dos name\n");
#endif
                      // set record as free
     	              NWFSSet(dos, 0, sizeof(DOS));
	              dos->Subdirectory = (ULONG)-1;

		      stats->bad_dos_records++;
		      return 1;
		   }
		   return 0;
			
		case MAC_NAME_SPACE:
                   if (NWValidMACName(volume, mac->FileName, 
				      mac->FileNameLength, 0, 0))
		   {
#if (VERBOSE)
                      NWFSPrint("Invalid mac name\n");
#endif
                      // set record as free
     	              NWFSSet(dos, 0, sizeof(DOS));
	              dos->Subdirectory = (ULONG)-1;
                      
		      stats->bad_mac_records++;
     		      return 1;
		   }
		   return 0;

		case LONG_NAME_SPACE:
                   if (NWValidLONGName(volume, longname->FileName, 
				      longname->FileNameLength, 0, 0))
		   {
#if (VERBOSE)
                      NWFSPrint("Invalid long name\n");
#endif
                      // set record as free
     	              NWFSSet(dos, 0, sizeof(DOS));
	              dos->Subdirectory = (ULONG)-1;
                      
		      stats->bad_long_records++;
     		      return 1;
		   }
	
		   if (longname->ExtantsUsed > 3)
		   {
#if (VERBOSE)
                      NWFSPrint("Invalid longname file extants-%d\n", 
				(int)longname->ExtantsUsed);
#endif
                      // set record as free
     	              NWFSSet(dos, 0, sizeof(DOS));
	              dos->Subdirectory = (ULONG)-1;
                      
		      stats->bad_long_extants++;
     		      return 1;
		   }

		   return 0;

		case UNIX_NAME_SPACE:
                   if (NWValidNFSName(volume, nfs->FileName, 
				      nfs->TotalFileNameLength, 0, 0))
		   {
#if (VERBOSE)
                      NWFSPrint("Invalid nfs name\n");
#endif
                      // set record as free
     	              NWFSSet(dos, 0, sizeof(DOS));
	              dos->Subdirectory = (ULONG)-1;
                      
		      stats->bad_nfs_records++;
     		      return 1;
		   }
		   
		   if (nfs->ExtantsUsed > 3)
		   {
#if (VERBOSE)
                      NWFSPrint("Invalid nfs file extants-%d\n", 
				(int)nfs->ExtantsUsed);
#endif
                      // set record as free
     	              NWFSSet(dos, 0, sizeof(DOS));
	              dos->Subdirectory = (ULONG)-1;
                      
		      stats->bad_nfs_extants++;
     		      return 1;
		   }
		   return 0;

		case NT_NAME_SPACE:
                   if (NWValidLONGName(volume, nt->FileName, 
				      nt->FileNameLength, 0, 0))
		   {
#if (VERBOSE)
                      NWFSPrint("Invalid nt name\n");
#endif
                      // set record as free
     	              NWFSSet(dos, 0, sizeof(DOS));
	              dos->Subdirectory = (ULONG)-1;
                      
		      stats->bad_nt_records++;
     		      return 1;
		   }
		   
		   if (nt->ExtantsUsed > 3)
		   {
#if (VERBOSE)
		      NWFSPrint("Invalid nt file extants-%d\n",
				(int)nt->ExtantsUsed);
#endif
                      // set record as free
     	              NWFSSet(dos, 0, sizeof(DOS));
	              dos->Subdirectory = (ULONG)-1;
                      
		      stats->bad_nt_extants++;
     		      return 1;
		   }
		   return 0;

		default:
                   // set record as free
     	           NWFSSet(dos, 0, sizeof(DOS));
	           dos->Subdirectory = (ULONG)-1;
		   return 1;
	     }
	  }
	  return 0;
    }
    return 0;    
}
