
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
*   FILE     :  NWEXT.C
*   DESCRIP  :   NetWare Extended Directory Management
*   DATE     :  January 4, 1999
*
*
***************************************************************************/

#include "globals.h"

extern void NWLockVolume(VOLUME *volume);
extern void NWUnlockVolume(VOLUME *volume);

ULONG ValidateEXTRecord(EXTENDED_DIR *dir, ULONG DirNo)
{
    register UNIX_EXTENDED_DIR *udir = (UNIX_EXTENDED_DIR *)dir;
    
    if (!DirNo)
    {
       if (NWFSCompare(dir, ZeroBuffer, sizeof(ROOT)))
          return 1;
       return 0;
    }

    switch (dir->Signature)
    {
       case NFS_EXTENDED_DIR_STAMP:
       case EXTENDED_DIR_STAMP:
       case LONG_EXTENDED_DIR_STAMP:
       case NT_EXTENDED_DIR_STAMP:
       case MIGRATE_DIR_STAMP:
       case TALLY_EXTENDED_DIR_STAMP:
       case EXTENDED_ATTRIBUTE_DIR_STAMP:
	  if (dir->DirectoryNumber > 0x00FFFFFF) // 16 million file limit
	     return 1;

	  if (dir->NameSpace > 10)              
	     return 1;

	  if ((dir->Flags > 2) || (dir->ControlFlags > 2))
	     return 1;

          if (NFS_EXTENDED_DIR_STAMP == dir->Signature)
	  {
	     if (udir->NameLength > 255)
	        return 1;
	  }
	  else
	  {
	     if (dir->Length > 255)
                return 1;
	  }
	  return 0;

       default:
	  return 1;
    }

}

ULONG ReadExtDirectoryRecords(VOLUME *volume, EXTENDED_DIR *dir, BYTE *data,
			      ULONG DirNo, ULONG count)
{
    register ULONG cbytes;
    ULONG retCode = 0;
 
    cbytes = NWReadFile(volume,
			   &volume->ExtDirectory1,
			   0,
			   -1,
			   DirNo * sizeof(DOS),
			   (BYTE *)dir,
			   sizeof(DOS) * count,
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   0,
			   TRUE);

    if ((cbytes == (sizeof(DOS) * count)) && !retCode)
    {
       if (ValidateEXTRecord(dir, DirNo))
       {
          NWFSPrint("ext directory # %X is invalid\n", (unsigned)DirNo);
          return NwDirectoryCorrupt;
       }
       return 0;
    }

    cbytes = NWReadFile(volume,
			   &volume->ExtDirectory2,
			   0,
			   -1,
			   DirNo * sizeof(DOS),
			   (BYTE *)dir,
			   sizeof(DOS) * count,
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   0,
			   TRUE);

    if ((cbytes == (sizeof(DOS) * count)) && !retCode)
    {
       if (ValidateEXTRecord(dir, DirNo))
       {
          NWFSPrint("ext directory # %X is invalid\n", (unsigned)DirNo);
          return NwDirectoryCorrupt;
       }
       return 0;
    }
    return 0;
}

ULONG WriteExtDirectoryRecords(VOLUME *volume, EXTENDED_DIR *dir, BYTE *data,
			       ULONG DirNo, ULONG count)
{
    register ULONG cbytes;
    ULONG retCode = 0;
    
    if (ValidateEXTRecord(dir, DirNo))
    {
       NWFSPrint("ext directory # %X is invalid\n", (unsigned)DirNo);
       return NwDirectoryCorrupt;
    }

    cbytes = NWWriteFile(volume,
			   &volume->ExtDirectory1,
			   0,
			   DirNo * sizeof(DOS),
			   (BYTE *)dir,
			   sizeof(DOS) * count,
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   0);

    if ((cbytes == (sizeof(DOS) * count)) && !retCode)
    {
#if (!CREATE_EDIR_MISMATCH)
	  // write mirror ext directory
       cbytes = NWWriteFile(volume,
			   &volume->ExtDirectory2,
			   0,
			   DirNo * sizeof(DOS),
			   (BYTE *)dir,
			   sizeof(DOS) * count,
			   0,
			   0,
			   &retCode,
			   KERNEL_ADDRESS_SPACE,
			   0,
			   0);

       if ((cbytes == (sizeof(DOS) * count)) && !retCode)
#endif
     	  return 0;
    }
    return NwDiskIoError;
}

ULONG ValidateExtendedDirectory(VOLUME *volume)
{
    register FAT_ENTRY *FAT1, *FAT2;
    FAT_ENTRY FAT1_S, FAT2_S;
    register ULONG retCode;
    register long index1, index2;
    register ULONG DirNo;
    register ULONG cluster1, cluster2, DirCount;
    register BYTE *Buffer1, *Buffer2;

#if (MOUNT_VERBOSE)
    NWFSPrint("*** Validating Extended Directory ***\n");
#endif

    Buffer1 = NWFSCacheAlloc(volume->ClusterSize, EXT_WORKSPACE_TAG);
    if (!Buffer1)
    {
       NWFSPrint("nwfs:  could not alloc extended directory buffer\n");
       return -3;
    }

    Buffer2 = NWFSCacheAlloc(volume->ClusterSize, EXT_WORKSPACE_TAG);
    if (!Buffer2)
    {
       NWFSPrint("nwfs:  could not alloc extended directory buffer\n");
       NWFSFree(Buffer1);
       return -3;
    }

    DirNo = 0;
    cluster1 = volume->ExtDirectory1;
    cluster2 = volume->ExtDirectory2;

    if ((cluster1 && !cluster2) || (!cluster1 && cluster2))
    {
       NWFSPrint("nwfs:  extended directory fat head chain mismatch\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -1;
    }

    // Extended Directory File is empty
    if ((!cluster1) || (!cluster2))
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return 0;
    }

    // read extended directory root 1

    retCode = ReadPhysicalVolumeCluster(volume, cluster1, Buffer1, volume->ClusterSize,
    					DIRECTORY_PRIORITY);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return retCode;
    }

    // read extended directory root 2

    retCode = ReadPhysicalVolumeCluster(volume, cluster2, Buffer2, volume->ClusterSize,
    					DIRECTORY_PRIORITY);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return retCode;
    }

    retCode = SetAssignedClusterValue(volume, cluster1, 1);
    if (retCode)
    {
       NWFSPrint("nwfs:  assigned bit block value not set edir cluster-%X\n",
		    (unsigned int)cluster1);
    }

    retCode = SetAssignedClusterValue(volume, cluster2, 1);
    if (retCode)
    {
       NWFSPrint("nwfs:  assigned bit block value not set edir mirror-%X\n",
		    (unsigned int)cluster2);
    }

    FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
    FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);
    if (!FAT1 || !FAT2)
    {
       NWFSPrint("nwfs:  error reading fat entry ReadExtDirectory\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -1;
    }

    index1 = FAT1->FATIndex;
    index2 = FAT2->FATIndex;
    DirCount = volume->ClusterSize / sizeof(DOS);

    while (TRUE)
    {
       if (NWFSCompare(Buffer1, Buffer2, volume->ClusterSize))
       {
	  extern void dumpRecordBytes(BYTE *, ULONG);
	  
	  NWFSPrint("nwfs:  Extended Directory data mirror mismatch\n");

#if (VERBOSE)
	  NWFSPrint("index1-%d cluster1-[%08X] next1-[%08X]\n", (int)index1,
		    (unsigned int)cluster1,
		    (unsigned int)FAT1->FATCluster);
	  dumpRecordBytes(Buffer1, 64);

	  NWFSPrint("index2-%d cluster2-[%08X] next2-[%08X]\n", (int)index2,
		    (unsigned int)cluster2,
		    (unsigned int)FAT2->FATCluster);
	  dumpRecordBytes(Buffer2, 64);
#endif
	  
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -3;
       }

       if (FAT1->FATCluster == (ULONG) -1)
	  break;

       // if the other FAT chain terminated abruptly, exit the loop

       if (FAT2->FATCluster == (ULONG) -1)
	  break;

       if (!FAT1->FATCluster)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  Free Cluster detected in Ext DIR1 Chain\n");
	  return -3;
       }

       if (!FAT2->FATCluster)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  Free Cluster detected in Ext DIR2 Chain\n");
	  return -3;
       }

       if (FAT1->FATCluster & 0x80000000)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  SubAlloc Node Detected in Ext DIR1\n");
	  return -3;
       }

       if (FAT2->FATCluster & 0x80000000)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  SubAlloc Node Detected in Ext DIR2\n");
	  return -3;
       }

       cluster1 = FAT1->FATCluster;
       cluster2 = FAT2->FATCluster;

       retCode = ReadPhysicalVolumeCluster(volume, cluster1, Buffer1, volume->ClusterSize,
       					   DIRECTORY_PRIORITY);
       if (retCode)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  Read Error Ext Directory1 returned %d\n", (int)retCode);
	  return retCode;
       }

       retCode = ReadPhysicalVolumeCluster(volume, cluster2, Buffer2, volume->ClusterSize,
       				           DIRECTORY_PRIORITY);
       if (retCode)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  Read Error Ext Directory2 returned %d\n", (int)retCode);
	  return retCode;
       }

       retCode = SetAssignedClusterValue(volume, cluster1, 1);
       if (retCode)
       {
	  NWFSPrint("nwfs:  assigned bit block value not set edir cluster-%X\n",
		    (unsigned int)cluster1);
       }

       retCode = SetAssignedClusterValue(volume, cluster2, 1);
       if (retCode)
       {
	  NWFSPrint("nwfs:  assigned bit block value not set edir mirror-%X\n",
		    (unsigned int)cluster2);
       }

       FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
       FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);
       if (!FAT1 || !FAT2)
       {
	  NWFSPrint("nwfs:  error reading fat entry ReadExtDirectory\n");
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  return -1;
       }

       // index numbers should always be ascending.  if we detect a
       // non-ascending sequence, this indicates a corrupt FAT chain

       if (index1 > (long)FAT1->FATIndex)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  EXTDIR1 fat index overlaps on itself\n");
	  return -1;
       }

       if (index2 > (long)FAT2->FATIndex)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  EXTDIR2 fat index overlaps on itself\n");
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
       NWFSPrint("nwfs:  Extended Directory FAT chain mirror mismatch\n");
       return -3;
    }

    NWFSFree(Buffer1);
    NWFSFree(Buffer2);

    return 0;

}

ULONG ReadExtendedDirectory(VOLUME *volume)
{
    return 0;
}





#if (WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)

ULONG DumpExtendedDirectoryFile(VOLUME *volume, FILE *fp)
{
    register FAT_ENTRY *FAT1, *FAT2;
    FAT_ENTRY FAT1_S, FAT2_S;
    register ULONG retCode;
    register long index1, index2;
    register ULONG i, Block = 0;
    register ULONG cluster1, cluster2;
    register BYTE *Buffer1, *Buffer2;
    register ULONG widget = 0;
#if (LINUX_UTIL)
    struct termios init_settings, new_settings;
#endif
    BYTE wa[4] = "\\|/-";

    NWFSPrint("Analyzing Extended Directory ... ");
    NWFSPrint("%c\b", wa[widget++ & 3]);

    Buffer1 = NWFSCacheAlloc(volume->ClusterSize, EXT_WORKSPACE_TAG);
    if (!Buffer1)
       return -3;

    Buffer2 = NWFSCacheAlloc(volume->ClusterSize, EXT_WORKSPACE_TAG);
    if (!Buffer2)
    {
       NWFSFree(Buffer1);
       return -3;
    }

    cluster1 = volume->ExtDirectory1;
    cluster2 = volume->ExtDirectory2;

    // check that both copies of the extended directory are present
    if ((cluster1 && !cluster2) || (!cluster1 && cluster2))
    {
       NWFSPrint("nwfs:  extended directory fat chain mismatch\n");
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return -1;
    }

    // extended directory is null, just return
    if ((!cluster1) || (cluster1 == (ULONG)-1))
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return 0;
    }

    // read extended directory root 1

    retCode = ReadPhysicalVolumeCluster(volume, cluster1, Buffer1, volume->ClusterSize, DIRECTORY_PRIORITY);
    if (retCode)
    {
       NWFSFree(Buffer1);
       NWFSFree(Buffer2);
       return retCode;
    }

    // read extended directory root 2

    retCode = ReadPhysicalVolumeCluster(volume, cluster2, Buffer2, volume->ClusterSize, DIRECTORY_PRIORITY);
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
       NWFSPrint("nwfs:  error reading fat entry ReadExtDirectory\n");
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
	  NWFSPrint("nwfs:  extended Directory data mirror mismatch\n");
	  return -3;
       }

       NWFSPrint("%c\b", wa[widget++ & 3]);
       for (i=0; i < (volume->ClusterSize / IO_BLOCK_SIZE); i++)
	  fwrite(&Buffer1[i * IO_BLOCK_SIZE], IO_BLOCK_SIZE, 1, fp);

       if (FAT1->FATCluster == (ULONG) -1)
       {
	  break;
       }

       // if the other FAT chain terminated abruptly, exit the loop

       if (FAT2->FATCluster == (ULONG) -1)
       {
	  break;
       }

       if (!FAT1->FATCluster)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  Free Cluster detected in Ext DIR1 Chain\n");
	  return -3;
       }

       if (!FAT2->FATCluster)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  Free Cluster detected in Ext DIR2 Chain\n");
	  return -3;
       }

       if (FAT1->FATCluster & 0x80000000)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  SubAlloc Node Detected in Ext DIR1\n");
	  return -3;
       }

       if (FAT2->FATCluster & 0x80000000)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  SubAlloc Node Detected in Ext DIR2\n");
	  return -3;
       }

       Block++;
       cluster1 = FAT1->FATCluster;
       cluster2 = FAT2->FATCluster;


       retCode = ReadPhysicalVolumeCluster(volume, cluster1, Buffer1, volume->ClusterSize, DIRECTORY_PRIORITY);
       if (retCode)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  Read Error Ext Directory1 returned %d\n", (int)retCode);
	  return retCode;
       }

       retCode = ReadPhysicalVolumeCluster(volume, cluster2, Buffer2, volume->ClusterSize, DIRECTORY_PRIORITY);
       if (retCode)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  Read Error Ext Directory2 returned %d\n", (int)retCode);
	  return retCode;
       }

       FAT1 = GetFatEntry(volume, cluster1, &FAT1_S);
       FAT2 = GetFatEntry(volume, cluster2, &FAT2_S);
       if (!FAT1 || !FAT2)
       {
	  NWFSPrint("nwfs:  error reading fat entry ReadExtDirectory\n");
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
	  NWFSPrint("nwfs:  EXTDIR1 fat index overlaps on itself\n");
	  return -1;
       }

       if (index2 > FAT2->FATIndex)
       {
	  NWFSFree(Buffer1);
	  NWFSFree(Buffer2);
	  NWFSPrint("nwfs:  EXTDIR2 fat index overlaps on itself\n");
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
       NWFSPrint("nwfs:  Extended Directory FAT chain mirror mismatch\n");
       return -3;
    }

    NWFSFree(Buffer1);
    NWFSFree(Buffer2);

    return 0;

}

#endif

