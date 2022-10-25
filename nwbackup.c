
/***************************************************************************
*
*   Copyright (c) 1997-2001 Jeff V. Merkey  All Rights Reserved.
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
*   AUTHORS  :  Jeff V. Merkey
*   FILE     :  NWIMAGE.C
*   DESCRIP  :  NWFS Volume Imaging Utility
*   DATE     :  August 5, 2022
*
*
***************************************************************************/

#include "globals.h"
#include "image.h"

#if (LINUX_UTIL)
#include <dirent.h>
#endif

#if (LINUX_UTIL && LINUX_DEV)
#include <dirent.h>
extern int devfd;
#endif

extern ULONG DumpDirectoryFile(VOLUME *volume, FILE *fp, ULONG *totalDirs);
extern ULONG DumpExtendedDirectoryFile(VOLUME *volume, FILE *fp);
extern void UtilScanVolumes(void);

ULONG image_table_count = 0;
IMAGE image_table[IMAGE_TABLE_SIZE];
ULONG mark_table[IMAGE_TABLE_SIZE];
ULONG valid_table[IMAGE_TABLE_SIZE];
BYTE inputBuffer[20] = { "" };
IMAGE_SET set_header;

ULONG SkipDeletedFiles = TRUE;

void EditImageOptions(void)
{
    BYTE input_buffer[20] = { "" };

    while (TRUE)
    {
       ClearScreen(consoleHandle);
       NWFSPrint("NetWare Volume Imaging Utility\n");
       NWFSPrint("Copyright (c) 1999, 1998 JVM, Inc.  All Rights Reserved.\n");
       NWFSPrint("Portions Copyright (c) Lintell, Inc.\n\n\n");

       NWFSPrint("        Strip Deleted Files = %s\n", SkipDeletedFiles ? "TRUE " : "FALSE");

       NWFSPrint("\n");
       NWFSPrint("               1. Toggle Deleted Files Flag\n");
       NWFSPrint("               2. Return\n\n");
       NWFSPrint("        Enter a Menu Option: ");

       fgets(input_buffer, sizeof(input_buffer), stdin);
       switch (input_buffer[0])
       {
	  case	'1':
	     if (SkipDeletedFiles == TRUE)
		SkipDeletedFiles = FALSE;
	     else
		SkipDeletedFiles = TRUE;
	     break;

	  case	'2':
	     return;
       }
    }
}

void GetDateAndTimeString(BYTE *target)
{
#if (WINDOWS_NT_UTIL)
    SYSTEMTIME stime;

    GetLocalTime(&stime);
    sprintf(target, "%02d/%02d/%02d %02d:%02d %c",
	    stime.wMonth,
	    stime.wDay,
	    stime.wYear,
	    (stime.wHour > 12) ? (stime.wHour - 12) : stime.wHour,
	    stime.wMinute,
	    (stime.wHour > 12) ? 'P' : 'A');

#endif

#if (LINUX_UTIL | DOS_UTIL)
    time_t epoch_time;
    struct tm *tm;

    epoch_time = time(NULL);
    tm = localtime(&epoch_time);

    sprintf(target, "%02d/%02d/%02d %02d:%02d %c",
	    tm->tm_mon,
	    tm->tm_mday,
	    tm->tm_year,
	    (tm->tm_hour > 12) ? (tm->tm_hour - 12) : tm->tm_hour,
	    tm->tm_min,
	    (tm->tm_hour > 12) ? 'P' : 'A');

#endif
}

ULONG MangleVolumeNames(BYTE *VolumeName,
			BYTE *VolumeFile,
			BYTE *DirectoryFile,
			BYTE *ExtDirectoryFile,
			BYTE *DataFile)
{
    register ULONG BaseLength, i, collision, val, Size;
    register BYTE *p, *v;
    register FILE *fd;
    register VOLUME *volume;
    BYTE Buffer[20];
    BYTE BaseName[20] = { "" };

    volume = GetVolumeHandle(VolumeName);
    if (!volume)
       return -1;

    // skip whitespace
    p = volume->VolumeName;
    while (*p && *p == ' ')
       p++;

    // strip the first eight characters of the volume name
    for (v = BaseName, BaseLength=0;
	(BaseLength < 8) && (*p) && (*p != ' ');
	BaseLength++)
       *v++ = *p++;
	*v = '\0';

    // here we attempt to create a unique name for this data set.
    // we do check the current directory to see if there already
    // is an existing data set that collides with our name.

    if (BaseLength > 5)
       BaseLength = 5;

    if (VolumeFile)
       NWFSSet(VolumeFile, 0, 13);
    if (DirectoryFile)
       NWFSSet(DirectoryFile, 0, 13);
    if (ExtDirectoryFile)
       NWFSSet(ExtDirectoryFile, 0, 13);
    if (DataFile)
       NWFSSet(DataFile, 0, 13);

    if (VolumeFile)
       NWFSCopy(VolumeFile, BaseName, BaseLength);
    if (DirectoryFile)
       NWFSCopy(DirectoryFile, BaseName, BaseLength);
    if (ExtDirectoryFile)
       NWFSCopy(ExtDirectoryFile, BaseName, BaseLength);
    if (DataFile)
       NWFSCopy(DataFile, BaseName, BaseLength);

    if (VolumeFile)
       VolumeFile[BaseLength] = '~';
    if (DirectoryFile)
       DirectoryFile[BaseLength] = '~';
    if (ExtDirectoryFile)
       ExtDirectoryFile[BaseLength] = '~';
    if (DataFile)
       DataFile[BaseLength] = '~';

    // calculate horner hash value from the original name
    val = NWFSStringHash(volume->VolumeName, 15, 255);
    for (collision=i=0; i < 255; i++)
    {
       sprintf(Buffer, "%02X", (BYTE)(val + i));

       if (VolumeFile)
	  NWFSCopy(&VolumeFile[BaseLength + 1], Buffer, 2);
       if (DirectoryFile)
	  NWFSCopy(&DirectoryFile[BaseLength + 1], Buffer, 2);
       if (ExtDirectoryFile)
	  NWFSCopy(&ExtDirectoryFile[BaseLength + 1], Buffer, 2);
       if (DataFile)
	  NWFSCopy(&DataFile[BaseLength + 1], Buffer, 2);

       if (VolumeFile)
	  VolumeFile[BaseLength + 3] = '.';
       if (DirectoryFile)
	  DirectoryFile[BaseLength + 3] = '.';
       if (ExtDirectoryFile)
	  ExtDirectoryFile[BaseLength + 3] = '.';
       if (DataFile)
	  DataFile[BaseLength + 3] = '.';

       if (VolumeFile)
	  NWFSCopy(&VolumeFile[BaseLength + 4], "VOL", 3);
       if (DirectoryFile)
	  NWFSCopy(&DirectoryFile[BaseLength + 4], "DIR", 3);
       if (ExtDirectoryFile)
	  NWFSCopy(&ExtDirectoryFile[BaseLength + 4], "EXT", 3);
       if (DataFile)
	  NWFSCopy(&DataFile[BaseLength + 4], "DAT", 3);

       if (VolumeFile)
       {
	  fd = fopen(VolumeFile, "rb");
	  if (!fd)
	     break;
	  else
	     fclose(fd);
       }
    }

    // if we could not create a unique name, then increase the
    // counter portion of the name and try until we generate
    // a unique name.

    if (collision)
    {
       i = BaseLength & 0xffff;
       Size = (BaseLength >> 16) & 0x7;

       if (BaseLength > 2)
	  BaseLength = 2;

       if (VolumeFile)
	  NWFSSet(VolumeFile, ' ', 13);
       if (DirectoryFile)
	  NWFSSet(DirectoryFile, ' ', 13);
       if (ExtDirectoryFile)
	  NWFSSet(ExtDirectoryFile, ' ', 13);
       if (DataFile)
	  NWFSSet(DataFile, ' ', 13);

       if (VolumeFile)
	  NWFSCopy(VolumeFile, BaseName, BaseLength);
       if (DirectoryFile)
	  NWFSCopy(DirectoryFile, BaseName, BaseLength);
       if (ExtDirectoryFile)
	  NWFSCopy(ExtDirectoryFile, BaseName, BaseLength);
       if (DataFile)
	  NWFSCopy(DataFile, BaseName, BaseLength);

       if (VolumeFile)
	  VolumeFile[BaseLength + 4] = '~';
       if (DirectoryFile)
	  DirectoryFile[BaseLength + 4] = '~';
       if (ExtDirectoryFile)
	  ExtDirectoryFile[BaseLength + 4] = '~';
       if (DataFile)
	  DataFile[BaseLength + 4] = '~';

       if (VolumeFile)
	  VolumeFile[BaseLength + 5] = (BYTE)('1' + Size);
       if (DirectoryFile)
	  DirectoryFile[BaseLength + 5] = (BYTE)('1' + Size);
       if (ExtDirectoryFile)
	  ExtDirectoryFile[BaseLength + 5] = (BYTE)('1' + Size);
       if (DataFile)
	  DataFile[BaseLength + 5] = (BYTE)('1' + Size);

       i -= 11;

       while (TRUE)
       {
	  sprintf(Buffer, "%04X", (unsigned int) i);

	  if (VolumeFile)
	     NWFSCopy(&VolumeFile[BaseLength], Buffer, 4);
	  if (DirectoryFile)
	     NWFSCopy(&DirectoryFile[BaseLength], Buffer, 4);
	  if (ExtDirectoryFile)
	     NWFSCopy(&ExtDirectoryFile[BaseLength], Buffer, 4);
	  if (DataFile)
	     NWFSCopy(&DataFile[BaseLength], Buffer, 4);

	  if (VolumeFile)
	     VolumeFile[BaseLength + 6] = '.';
	  if (DirectoryFile)
	     DirectoryFile[BaseLength + 6] = '.';
	  if (ExtDirectoryFile)
	     ExtDirectoryFile[BaseLength + 6] = '.';
	  if (DataFile)
	     DataFile[BaseLength + 6] = '.';

	  if (VolumeFile)
	     NWFSCopy(&VolumeFile[BaseLength + 7], "VOL", 3);
	  if (DirectoryFile)
	     NWFSCopy(&DirectoryFile[BaseLength + 7], "DIR", 3);
	  if (ExtDirectoryFile)
	     NWFSCopy(&ExtDirectoryFile[BaseLength + 7], "EXT", 3);
	  if (DataFile)
	     NWFSCopy(&DataFile[BaseLength + 7], "DAT", 3);

	  if (VolumeFile)
	  {
	     fd = fopen(VolumeFile, "rb");
	     if (!fd)
		break;
	     else
		fclose(fd);
	  }

	  i -= 11;
       }
    }
    return 0;

}

ULONG CreateUniqueSet(BYTE *Name, BYTE *MangledName, ULONG MangledLength,
		      ULONG *NewLength)
{
    register ULONG BaseLength, i, collision, val, Size;
    register BYTE *p, *v;
    register FILE *fd;
    BYTE BaseName[20] = { "" };
    BYTE Buffer[20] = { "" };

    // strip the first eight characters of the name
    for (p = Name, v = BaseName, BaseLength=0;
	(BaseLength < 8) && (*p) && (*p != ' ');
	BaseLength++)
       *v++ = *p++;
    *v = '\0';

    // here we attempt to create a unique name for this data set.
    // we do check the current directory to see if there already
    // is an existing data set that collides with our name.

    if (BaseLength > 5)
       BaseLength = 5;

    NWFSSet(MangledName, 0, MangledLength);
    NWFSCopy(MangledName, BaseName, BaseLength);
    MangledName[BaseLength] = '~';

    // calculate horner hash value from the original name
    val = NWFSStringHash(Name, strlen(Name), 255);
    for (collision=i=0; i < 255; i++)
    {
       sprintf(Buffer, "%02X", (BYTE)(val + i));

       NWFSCopy(&MangledName[BaseLength + 1], Buffer, 2);
       MangledName[BaseLength + 3] = '.';
       NWFSCopy(&MangledName[BaseLength + 4], "SET", 3);

       fd = fopen(MangledName, "rb");
       if (!fd)
	  break;
       else
	  fclose(fd);
    }

    // if we could not create a unique name, then increase the
    // counter portion of the name and try until we generate
    // a unique name.

    if (collision)
    {
       i = BaseLength & 0xffff;
       Size = (BaseLength >> 16) & 0x7;

       if (BaseLength > 2)
	  BaseLength = 2;

       NWFSSet(MangledName, ' ', 13);
       NWFSCopy(MangledName, BaseName, BaseLength);
       MangledName[BaseLength + 4] = '~';
       MangledName[BaseLength + 5] = (BYTE)('1' + Size);

       i -= 11;

       while (TRUE)
       {
	  sprintf(Buffer, "%04X", (unsigned int) i);

	  NWFSCopy(&MangledName[BaseLength], Buffer, 4);
	  MangledName[BaseLength + 6] = '.';
	  NWFSCopy(&MangledName[BaseLength + 7], "SET", 3);

	  fd = fopen(MangledName, "rb");
	  if (!fd)
	     break;
	  else
	    fclose(fd);

	  i -= 11;
       }
    }

    if (NewLength)
       *NewLength = strlen(MangledName);

    return 0;

}

ULONG ImageNetWareVolume(IMAGE *image)
{
    ULONG ClusterChain;
    LONGLONG FileSize;
    register DOS *dos;
    register MACINTOSH *mac;
    register VOLUME *volume;
    register ULONG DirNo, len;
    ULONG totalDirs, ccode;
    register ULONG total, index;
    register long bytesLeft, bytesToRead, vsize;
    register BYTE *cBuffer, *buf;
    ULONG retCode = 0;
    ULONG current, recordOffset;
    FILE *file, *ptr, *fp;
    BYTE Buffer[20] = { "" };
    BYTE DirectoryName[20] = { "" };
    BYTE ExtDirectoryName[20] = { "" };
    BYTE DataName[20] = { "" };
    LONGLONG DataSize;
    LONGLONG TotalSize = 0;

    len = strlen(image->VolumeFile);
    if (len <= 4)
       return -1;

    NWFSCopy(DirectoryName, image->VolumeFile, len);
    NWFSCopy(ExtDirectoryName, image->VolumeFile, len);
    NWFSCopy(DataName, image->VolumeFile, len);

    DirectoryName[len - 3] = '\0';
    strcat(DirectoryName, "DIR");
    ExtDirectoryName[len - 3] = '\0';
    strcat(ExtDirectoryName, "EXT");
    DataName[len - 3] = '\0';
    strcat(DataName, "DAT");

    volume = GetVolumeHandle(&image->VolumeName[1]);
    if (!volume)
       return -1;

    NWFSPrint("Imaging Volume [%15s] \n", &image->VolumeName[1]);

    ccode = MountVolumeByHandle(volume);
    if (ccode)
    {
       NWFSPrint("nwimage:  error mounting volume %s\n", volume->VolumeName);
       return -1;
    }

    DataSize = (LONGLONG)volume->VolumeAllocatedClusters *
	       (LONGLONG)volume->ClusterSize;

    // save the volume directory file

    NWFSPrint("Creating Directory Table Archive       :  %s\n", DirectoryName);
    fp = fopen(DirectoryName, "wb");
    if (!fp)
    {
       NWFSPrint("nwimage:  could not create file %s\n", DirectoryName);
       return -1;
    }

    ccode = DumpDirectoryFile(volume, fp, &totalDirs);
    if (ccode)
    {
       NWFSPrint("\nnwimage:  could not dump directory tables\n");
       fclose(fp);
       return -1;
    }
    NWFSPrint(" \b\n");
    fclose(fp);

    // save the volume extended directory file

    NWFSPrint("Creating Extended Directory Archive    :  %s\n", ExtDirectoryName);
    fp = fopen(ExtDirectoryName, "wb");
    if (!fp)
    {
       NWFSPrint("nwimage:  could not create file %s\n", ExtDirectoryName);
       return -1;
    }

    ccode = DumpExtendedDirectoryFile(volume, fp);
    if (ccode)
    {
       NWFSPrint("\nnwimage:  could not dump extended directory tables\n");
       fclose(fp);
       return -1;
    }
    NWFSPrint(" \b\n");
    fclose(fp);

    NWFSPrint("Creating Compressed Data Archive       :  %s\n", DataName);
    file = fopen(DataName, "wb");
    if (!file)
    {
       NWFSPrint("nwimage:  could not create file %s\n", DataName);
       return -1;
    }

    cBuffer = malloc(volume->ClusterSize);
    if (!cBuffer)
    {
       NWFSPrint("nwimage:  could not allocate workspace 1 memory\n");
       fclose(file);
       return -1;
    }

    dos = malloc(sizeof(DOS));
    if (!dos)
    {
       NWFSPrint("nwimage:  could not allocate workspace 2 memory\n");
       free(Buffer);
       fclose(file);
       return -1;
    }

    ptr = fopen(DirectoryName, "rb+wb");
    if (!ptr)
    {
       NWFSPrint("nwimage:  could not reopen directory file\n");
       free(cBuffer);
       free(dos);
       fclose(file);
       return -1;
    }
    NWFSPrint("\n");

    image->FilesInDataSet = 0;
    FileSize = DirNo = total = 0;
    fseek(ptr, 0L, SEEK_SET);
    while (!feof(ptr))
    {
       current = ftell(ptr);
       fread(dos, sizeof(DOS), 1, ptr);
       DirNo++;

       switch (dos->Subdirectory)
       {
	  case FREE_NODE:
	  case TRUSTEE_NODE:
	  case ROOT_NODE:
	  case RESTRICTION_NODE:
	     break;

	  case SUBALLOC_NODE:
	     // remove this record from the directory archive
	     NWFSSet(dos, 0, sizeof(DOS));
	     dos->Subdirectory = (ULONG) -1;

	     fseek(ptr, current, SEEK_SET);
	     ccode = fwrite(dos, sizeof(DOS), 1, ptr);
	     if (!ccode)
	     {
		NWFSPrint("\nnwimage:  error writing suballoc records\n");
		retCode = (ULONG) -1;
		goto shutdown;
	     }
	     fseek(ptr, current + sizeof(DOS), SEEK_SET);
	     break;

	  default:
	     if (dos->Flags & SUBDIRECTORY_FILE)
		break;

	     // check for deleted Netware 3.x and Netware 4.x files
	     if ((dos->Flags & NW3_DELETED_FILE) ||
		 (dos->Flags & NW4_DELETED_FILE))
	     {
		if (SkipDeletedFiles)
		{
		   NWFSPrint("Skipping Deleted File Entry at [%08X]\n",
			     (unsigned int)DirNo);

		   if (dos->NameSpace == DOS_NAME_SPACE)
		      TotalSize += (LONGLONG) dos->FileSize;

		   // remove this record from the directory archive
		   NWFSSet(dos, 0, sizeof(DOS));
		   dos->Subdirectory = (ULONG) -1;

		   fseek(ptr, current, SEEK_SET);
		   ccode = fwrite(dos, sizeof(DOS), 1, ptr);
		   if (!ccode)
		   {
		      NWFSPrint("\nnwimage:  error writing deleted directory record\n");
		      retCode = (ULONG) -1;
		      goto shutdown;
		   }
		   fseek(ptr, current + sizeof(DOS), SEEK_SET);
		   break;
		}
	     }

	     switch (dos->NameSpace)
	     {
		case DOS_NAME_SPACE:
		   NWFSCopy(Buffer, dos->FileName,
			    (dos->FileNameLength <= 12)
			    ? dos->FileNameLength : 12);
		   Buffer[(dos->FileNameLength <= 12)
			    ? dos->FileNameLength : 12] = '\0';

		   ClusterChain = dos->FirstBlock;
		   if ((ClusterChain) && (ClusterChain != (ULONG) -1))
		   {
		      // round to next 16 byte boundry
		      recordOffset = (((ftell(file) + 15L) / 16L) * 16L);
		      fseek(file, recordOffset, SEEK_SET);

		      dos->FirstBlock = (recordOffset >> 4);
		      FileSize = dos->FileSize;
		   }
		   TotalSize += (LONGLONG) dos->FileSize;
		   goto UpdateDirRecord;

		case MAC_NAME_SPACE:
		   mac = (MACINTOSH *) dos;
		   NWFSCopy(Buffer, mac->FileName,
			    (mac->FileNameLength <= 12)
			    ? mac->FileNameLength : 12);
		   Buffer[(mac->FileNameLength <= 12)
			       ? mac->FileNameLength : 12] = '\0';

		   ClusterChain = mac->ResourceFork;
		   if ((ClusterChain) && (ClusterChain != (ULONG) -1))
		   {
		      // round to next 16 byte boundry
		      recordOffset = (((ftell(file) + 15L) / 16L) * 16L);
		      fseek(file, recordOffset, SEEK_SET);

		      mac->ResourceFork = (recordOffset >> 4);
		      FileSize = mac->ResourceForkSize;
		   }
		   TotalSize += (LONGLONG) mac->ResourceForkSize;

UpdateDirRecord:;
		   fseek(ptr, current, SEEK_SET);
		   ccode = fwrite(dos, sizeof(DOS), 1, ptr);
		   if (!ccode)
		   {
		      NWFSPrint("\nnwimage:  error writing updated directory record\n");
		      retCode = (ULONG) -1;
		      goto shutdown;
		   }
		   fseek(ptr, current + sizeof(DOS), SEEK_SET);

		   NWFSPrint("Processing [%-12s] %10d bytes  Files Processed-%d  %d%% Complete \n",
			     Buffer,
			     (int)FileSize,
			     (int)image->FilesInDataSet,
			     (ULONG)((((LONGLONG)TotalSize *
				       (LONGLONG)100) /
				       (LONGLONG)DataSize) > 100)
			     ? (int)100
			     : (int)(((LONGLONG)TotalSize *
				      (LONGLONG)100) /
				      (LONGLONG)DataSize));

		   if ((ClusterChain) && (ClusterChain != (ULONG) -1))
		   {
		      index = 0;
		      bytesToRead = FileSize;
		      while (bytesToRead > 0)
		      {
			 ccode = NWReadFile(volume,
					 &ClusterChain,
					 0,
					 FileSize,
					 index * IO_BLOCK_SIZE,
					 cBuffer,
					 IO_BLOCK_SIZE,
					 0,
					 0,
					 &retCode,
					 KERNEL_ADDRESS_SPACE,
					 TRUE,
					 0,
					 TRUE);
			 if (retCode)
			 {
			    NWFSPrint("\nnwimage:  error reading volume ccode-%d\n",
				     (int)retCode);
			    goto shutdown;
			 }

			 bytesToRead -= ccode;
			 bytesLeft = ccode;
			 buf = cBuffer;
			 while (bytesLeft > 0)
			 {
			    vsize = ((bytesLeft > IO_BLOCK_SIZE)
				    ? IO_BLOCK_SIZE : bytesLeft);

			    ccode = fwrite(buf, vsize, 1, file);
			    if (!ccode)
			    {
			       NWFSPrint("\nnwimage:  error writing to image file\n");
			       retCode = (ULONG) -1;
			       goto shutdown;
			    }
			    total += vsize;
			    bytesLeft -= vsize;
			    buf += vsize;
			 }
			 index++;
		      }
		   }
		   image->FilesInDataSet++;
		   break;
	     }
	     break;
       }

       if (feof(ptr))
       {
	  NWFSPrint("total written to archive [%12d] bytes\n", (int)total);
       }
    }

shutdown:;
    free(cBuffer);
    free(dos);

    fclose(ptr);
    fclose(file);

    image->AllocatedVolumeClusters = volume->VolumeAllocatedClusters;
    image->VolumeClusterSize = volume->ClusterSize;
    image->LogicalVolumeClusters = volume->VolumeClusters;

    ccode = DismountVolumeByHandle(volume);
    if (ccode)
       NWFSPrint("error dismounting volume [%15s]\n", volume->VolumeName);

    return retCode;

}

int main(int argc, char *argv[])
{
    register ULONG i, k, j, retCode;
    register ULONG index;
    register ULONG display_base = 0;
    register ULONG checksum;
    register ULONG *checksum_ptr;
    register FILE *sfd, *fp;

    BYTE input_buffer[20] = { "" };
    ULONG handles[7];
    nwvp_payload payload;
    nwvp_vpartition_info vpart_info;
    BYTE ImageSetName[20] = "IMAGE000.SET";
    BYTE WorkSpace[20];

    NWFSSet(&set_header, 0, sizeof(IMAGE_SET));

    if (argc > 1)
    {
       for (i=0; i < 8; i++)
       {
	  if ((argv[1][i] >= 'a') && (argv[1][i] <= 'z'))
	     ImageSetName[i] = (argv[1][i] - 'a') + 'A';
	  else
	  if ((argv[1][i] >= 'a') && (argv[1][i] <= 'z'))
	     ImageSetName[i] = argv[1][i];
	  else
	  if ((argv[1][i] >= '0') && (argv[1][i] <= '9'))
	     ImageSetName[i] = argv[1][i];
	  else
	     break;
       }

       ImageSetName[i] = '.';
       ImageSetName[i+1] = 'S';
       ImageSetName[i+2] = 'E';
       ImageSetName[i+3] = 'T';
       ImageSetName[i+4] = 0;
    }

    // make the volume set name unique if a file with this name
    // already exists
    fp = fopen(ImageSetName, "rb");
    if (fp)
    {
       fclose(fp);
       retCode = CreateUniqueSet(ImageSetName, WorkSpace, 20, 0);
       if (!retCode)
	  NWFSCopy(ImageSetName, WorkSpace, strlen(WorkSpace));
    }

    InitNWFS();
    UtilScanVolumes();
    for (i=0; i < TotalDisks; i++)
       SyncDevice(i);

    payload.payload_object_count = 0;
    payload.payload_index = 0;
    payload.payload_object_size_buffer_size = (7 << 20) + sizeof(handles);
    payload.payload_buffer = (BYTE *) &handles[0];

    do
    {
       nwvp_vpartition_scan(&payload);
       for (j=0; j < payload.payload_object_count; j++)
       {
	  if (nwvp_vpartition_return_info(handles[j], &vpart_info) == 0)
	  {
	     memmove(&image_table[image_table_count].VolumeName[0], &vpart_info.volume_name[0], 16);

	     image_table[image_table_count].VolumeClusterSize = vpart_info.blocks_per_cluster * 4096;
	     image_table[image_table_count].LogicalVolumeClusters = vpart_info.cluster_count;
	     image_table[image_table_count].SegmentCount = vpart_info.segment_count;
	     image_table[image_table_count].MirrorFlag = vpart_info.mirror_flag;

	     if ((valid_table[image_table_count] = ((vpart_info.volume_valid_flag == 0) ? 0xFFFFFFFF : 0)) != 0)
		mark_table[image_table_count] = 1;

	     image_table_count ++;
	  }
       }
    } while ((payload.payload_index != 0) && (payload.payload_object_count != 0));

#if (LINUX_UTIL && LINUX_DEV)
    devfd = open("/dev/nwfs", O_RDWR);
    if (devfd < 0)
    {
       printf("Error opening /dev/nwfs device - some operations disabled\n");
       printf("press a key to continue ...\n");
       WaitForKey();
    }
#endif

    while (TRUE)
    {
       ClearScreen(consoleHandle);

       NWFSPrint("NetWare Volume Imaging Utility\n");
       NWFSPrint("Copyright (c) 1999, 1998 JVM, Inc.  All Rights Reserved.\n");
       NWFSPrint("Portions Copyright (c) Lintell, Inc.\n");
       NWFSPrint(" Image Set: %s \n", &ImageSetName[0]);
       NWFSPrint("\n Marked #           Name     Total Size  Cluster  Segs Mirror Status\n\n");

       for (i=0; i < 8; i++)
       {
	  if ((index = display_base + i) < image_table_count)
	  {
	     NWFSPrint(" %s %2d [%16s]  %6u MB     %2d K    %2d  %s   %s\n",
			(mark_table[index] == 0) ? "   " : "***",
			(int)index,
			&image_table[index].VolumeName[1],
			(unsigned int)
                           (((LONGLONG)image_table[index].LogicalVolumeClusters *
			     (LONGLONG)image_table[index].VolumeClusterSize) /
			     (LONGLONG)0x100000),
			(int)(image_table[index].VolumeClusterSize / 1024),
			(int)image_table[index].SegmentCount,
			(image_table[index].MirrorFlag == 0) ?
			"SINGLE" : "MIRROR",
		       (valid_table[index] == 0) ? "INVALID" : "OK");
	  }
	  else
	     NWFSPrint("\n");
       }

       NWFSPrint("\n\n");
       NWFSPrint("               1. Image Marked Volumes\n");
       NWFSPrint("               2. Change Image Set Name \n");
       NWFSPrint("               3. Mark/Unmark Volume\n");
       NWFSPrint("               4. Scroll Volumes\n");
       NWFSPrint("               5. Set Image Options\n");
       NWFSPrint("               6. Exit NWImage\n\n");
       NWFSPrint("        Enter a Menu Option: ");

       fgets(input_buffer, sizeof(input_buffer), stdin);

       switch(input_buffer[0])
       {
	  case '1':
	     if ((sfd = fopen(&ImageSetName[0], "rb")) != 0)
	     {
		NWFSPrint("  **** Data Set %s already exists ****\n",
			  &ImageSetName[0]);
		fclose(sfd);
	     }
	     else
	     if ((sfd = fopen(&ImageSetName[0], "wb")) == 0)
	     {
		NWFSPrint("Error Opening %s \n", &ImageSetName[0]);
	     }
	     else
	     {
		NWFSSet((BYTE *) &set_header, 0, sizeof(set_header));
		checksum = 0;
		set_header.Stamp = IMAGE_SET_STAMP;
		set_header.CheckSum = 0;
		set_header.VolumeCount = 0;

		GetDateAndTimeString(&set_header.DateTimeString[0]);

		for (i=0; i < image_table_count; i++)
		{
		   if (mark_table[i] != 0)
		   {
		      if (MangleVolumeNames(&image_table[i].VolumeName[1],
					    &image_table[i].VolumeFile[0],
					    0,
					    0,
					    0) == 0)
		      {
			 NWFSCopy(&set_header.VolumeFile[i][0],
				  &image_table[i].VolumeFile[0], 16);

			 set_header.VolumeCount++;
		      }
		      else
		      {
			 NWFSPrint("************* error creating unique names *************\n");
			 WaitForKey();
			 goto ImageError1;
		      }
		   }
		}

		checksum_ptr = (ULONG *) &set_header;
		for (i=0; i < (sizeof(IMAGE_SET) / 4); i++)
		{
		   checksum += checksum_ptr[i];
		}
		set_header.CheckSum = ~checksum;

		fwrite(&set_header, sizeof(IMAGE_SET), 1, sfd);
		fclose(sfd);

		//
		//  volume backup goes here
		//

		ClearScreen(consoleHandle);
		for (i=0; i < image_table_count; i++)
		{
		   if (mark_table[i] != 0)
		   {
		      ImageNetWareVolume(&image_table[i]);
		   }
		}
		fclose(sfd);

		for (i=0; i < image_table_count; i++)
		{
		   if (mark_table[i] != 0)
		   {
		      memmove(&image_table[i].ImageSetName[0], &ImageSetName[0], 12);
		      if ((sfd = fopen(&set_header.VolumeFile[i][0], "wb")) != 0)
		      {
			 image_table[i].Stamp = IMAGE_FILE_STAMP;

			 GetDateAndTimeString(&image_table[i].DateTimeString[0]);

			 image_table[i].CheckSum = 0;
			 image_table[i].Filler[0] = 0;
			 image_table[i].Filler[1] = 0;
			 checksum = 0;
			 checksum_ptr = (ULONG *) &image_table[i];

			 for (k=0; k < (sizeof(IMAGE) / 4); k++)
			    checksum += checksum_ptr[k];
			 image_table[i].CheckSum = ~checksum;
			 fwrite(&image_table[i], sizeof(IMAGE), 1, sfd);
			 fclose(sfd);
		      }
		      else
			 NWFSPrint(" error opening %s \n", &set_header.VolumeFile[i][0]);
		   }
		}
	     }
	     WaitForKey();
ImageError1:;
	     break;

	  case '2':
	     NWFSPrint("       Enter Image Set Name: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     for (i=0; i < 8; i++)
	     {
		if ((input_buffer[i] >= 'a') && (input_buffer[i] <= 'z'))
		   ImageSetName[i] = (input_buffer[i] - 'a') + 'A';
		else
		if ((input_buffer[i] >= 'A') && (input_buffer[i] <= 'Z'))
		   ImageSetName[i] = input_buffer[i];
		else
		if ((input_buffer[i] >= '0') && (input_buffer[i] <= '9'))
		   ImageSetName[i] = input_buffer[i];
		else
		   break;
	     }
	     ImageSetName[i] = '.';
	     ImageSetName[i+1] = 'S';
	     ImageSetName[i+2] = 'E';
	     ImageSetName[i+3] = 'T';
	     ImageSetName[i+4] = 0;
	     break;

	  case '3':
	     NWFSPrint("        Enter Volume Number: ");
	     fgets(input_buffer, sizeof(input_buffer), stdin);
	     index = 0xFFFFFFFF;
	     for (i=0; i < 4; i++)
	     {
		if ((input_buffer[i] >= '0') && (input_buffer[i] <='8'))
		{
		   if (index == 0xFFFFFFFF)
		      index = 0;
		   index = (index * 10) + (input_buffer[i] - '0');
		}
		else
		   break;
	     }

	     if (index < image_table_count)
	     {
		if (valid_table[index] != 0)
		   mark_table[index] ++;
		mark_table[index] &= 0x00000001;
	     }
	     else
	     {
		if (index != 0xFFFFFFFF)
		{
		   NWFSPrint("       *********  Invalid Volume ID **********\n");
		   WaitForKey();
		}
	     }
	     break;

	  case '4':
	     display_base += 8;
	     if (display_base >= image_table_count)
		display_base = 0;
	     break;

	  case '5':
	     ClearScreen(consoleHandle);
	     EditImageOptions();
	     break;

	  case '6':
	     ClearScreen(consoleHandle);
	     goto Shutdown;
       }
    }

Shutdown:
#if (LINUX_UTIL && LINUX_DEV)
    if (devfd)
       close(devfd);
#endif

    DismountAllVolumes();
    NWFSVolumeClose();
    RemoveDiskDevices();
    FreePartitionResources();
    CloseNWFS();
    NWFSPrint("\n");

    return 0;

}

