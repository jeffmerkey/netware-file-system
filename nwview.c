
/***************************************************************************
*
*   Copyright (c) 1998, 2022 Jeff V. Merkey  All Rights
*			    Reserved.
*
*   This program is an unpublished work of TRG, Inc. and contains
*   trade secrets and other proprietary information.  Unauthorized
*   use, copying, disclosure, or distribution of this file without the
*   consent of TRG, Inc. can subject the offender to severe criminal
*   and/or civil penalities.
*
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWVIEW.C
*   DESCRIP  :   Volume Viewer Utility
*   DATE     :  November 23, 1998
*
*
***************************************************************************/

#include "globals.h"

BYTE Sector[IO_BLOCK_SIZE];
WORD partitionSignature;
WORD stamp;

extern BYTE *NameSpaceStrings[];
extern BYTE *AttributeDescription[];
extern BYTE *AttributeShorthand[];
extern ULONG dumpRecord(BYTE *, ULONG);
extern ULONG dumpRecordBytes(BYTE *, ULONG);


ULONG DumpExtendedRecord(EXTENDED_DIR *dir, ULONG DirNo, ULONG flag)
{
    UNIX_EXTENDED_DIR *udir = (UNIX_EXTENDED_DIR *) dir;
    
    switch (dir->Signature)
    {
       case EXTENDED_DIR_STAMP:
          NWFSPrint("[%X] %08X - %s\n", (unsigned)DirNo, 
	            (unsigned)dir->Signature,
		    "EXTENDED_DIR_STAMP");
	  NWFSPrint("dir-%X len-%d ns-%2X flg-%2X contrl-%2X\n",
		    (unsigned)dir->DirectoryNumber,
		    (int)dir->Length,
		    (BYTE)dir->NameSpace,
		    (BYTE)dir->Flags,
		    (BYTE)dir->ControlFlags);
	  if (flag)
	  {
	     dumpRecord((BYTE *)dir, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dir, sizeof(ROOT));
	  }
	  break;

       case LONG_EXTENDED_DIR_STAMP:
          NWFSPrint("[%X] %08X - %s\n", (unsigned)DirNo, 
	            (unsigned)dir->Signature,
		    "LONG_EXTENDED_DIR_STAMP");
	  NWFSPrint("dir-%X len-%d ns-%2X flg-%2X contrl-%2X\n",
		    (unsigned)dir->DirectoryNumber,
		    (int)dir->Length,
		    (BYTE)dir->NameSpace,
		    (BYTE)dir->Flags,
		    (BYTE)dir->ControlFlags);
	  if (flag)
	  {
	     dumpRecord((BYTE *)dir, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dir, sizeof(ROOT));
	  }
	  break;

       case NT_EXTENDED_DIR_STAMP:
          NWFSPrint("[%X] %08X - %s\n", (unsigned)DirNo, 
	            (unsigned)dir->Signature,
		    "NT_EXTENDED_DIR_STAMP");
	  NWFSPrint("dir-%X len-%d ns-%2X flg-%2X contrl-%2X\n",
		    (unsigned)dir->DirectoryNumber,
		    (int)dir->Length,
		    (BYTE)dir->NameSpace,
		    (BYTE)dir->Flags,
		    (BYTE)dir->ControlFlags);
	  if (flag)
	  {
	     dumpRecord((BYTE *)dir, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dir, sizeof(ROOT));
	  }
	  break;

       case NFS_EXTENDED_DIR_STAMP:
          NWFSPrint("[%X] %08X - %s\n", (unsigned)DirNo, 
	            (unsigned)udir->Signature,
		    "NFS_EXTENDED_DIR_STAMP");
	  NWFSPrint("dir-%X len-%d namel-%2X-(%d) ns-%2X flg-%2X contrl-%2X\n",
		    (unsigned)udir->DirectoryNumber,
		    (int)udir->Length,
		    (BYTE)udir->NameLength,
		    (int)udir->NameLength,
		    (BYTE)udir->NameSpace,
		    (BYTE)udir->Flags,
		    (BYTE)udir->ControlFlags);
	  if (flag)
	  {
	     dumpRecord((BYTE *)udir, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)udir, sizeof(ROOT));
	  }
	  break;

       case MIGRATE_DIR_STAMP:
          NWFSPrint("[%X] %08X - %s\n", (unsigned)DirNo, 
	            (unsigned)dir->Signature,
		    "MIGRATE_DIR_STAMP");
	  NWFSPrint("dir-%X len-%d ns-%2X flg-%2X contrl-%2X\n",
		    (unsigned)dir->DirectoryNumber,
		    (int)dir->Length,
		    (BYTE)dir->NameSpace,
		    (BYTE)dir->Flags,
		    (BYTE)dir->ControlFlags);
	  if (flag)
	  {
	     dumpRecord((BYTE *)dir, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dir, sizeof(ROOT));
	  }
	  break;

       case TALLY_EXTENDED_DIR_STAMP:
          NWFSPrint("[%X] %08X - %s\n", (unsigned)DirNo, 
	            (unsigned)dir->Signature,
		    "TALLY_EXTENDED_DIR_STAMP");
	  NWFSPrint("dir-%X len-%d ns-%2X flg-%2X contrl-%2X\n",
		    (unsigned)dir->DirectoryNumber,
		    (int)dir->Length,
		    (BYTE)dir->NameSpace,
		    (BYTE)dir->Flags,
		    (BYTE)dir->ControlFlags);
	  if (flag)
	  {
	     dumpRecord((BYTE *)dir, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dir, sizeof(ROOT));
	  }
	  break;

       case EXTENDED_ATTRIBUTE_DIR_STAMP:
          NWFSPrint("[%X] %08X - %s\n", (unsigned)DirNo, 
	            (unsigned)dir->Signature,
		    "EXTENDED_ATTRIBUTE_DIR_STAMP");
	  NWFSPrint("dir-%X len-%d ns-%2X flg-%2X contrl-%2X\n",
		    (unsigned)dir->DirectoryNumber,
		    (int)dir->Length,
		    (BYTE)dir->NameSpace,
		    (BYTE)dir->Flags,
		    (BYTE)dir->ControlFlags);
	  if (flag)
	  {
	     dumpRecord((BYTE *)dir, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dir, sizeof(ROOT));
	  }
	  break;

       default:
	  return 0;
    }
    return 1;

}

ULONG DisplayRecord(DOS *dos, ULONG DirNo, ULONG flag)
{
    register SUB_DIRECTORY *subdir;
    register ROOT *root;
    register USER *user;
    register TRUSTEE *trustee;
    register SUBALLOC *suballoc;
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *win95;
    register NTNAME *nt;

    register ULONG j, k, nameS;
    BYTE PathString[256]={ "" };

    switch (dos->Subdirectory)
    {
       case FREE_NODE:
	  if (flag)
	  {
	     dumpRecord((BYTE *)dos, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
	  }
	  return 0;

       case TRUSTEE_NODE:
	  trustee = (TRUSTEE *) dos;
	  printf("[%X]  Attributes-%08X  TrusteeCount-%02X  Flags-%02X ID-%X\n"
		 "  Subdir-%08X  NextTrustee-%08X FileEntry-%08X\n"
		 "  DeletedBlockNo-%08X\n",
		 (unsigned int)DirNo,
		 (unsigned int)trustee->Attributes,
		 (unsigned int)trustee->TrusteeCount,
		 (unsigned int)trustee->Flags,
		 (unsigned int)trustee->ID,
		 (unsigned int)trustee->Subdirectory,
		 (unsigned int)trustee->NextTrusteeEntry,
		 (unsigned int)trustee->FileEntryNumber,
		 (unsigned int)trustee->DeletedBlockSequenceNumber);

	  for (j=0; j < trustee->TrusteeCount && j < 16; j++)
	  {
	     printf("Trustee: %08X  Mask: %08X\n",
		    (unsigned int)trustee->Trustees[j],
		    (unsigned int)trustee->TrusteeMask[j]);
	  }
	  if (flag)
	  {
	     dumpRecord((BYTE *)dos, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
	  }
	  return 1;

       case ROOT_NODE:
	  root = (ROOT *) dos;
	  printf("[%X] [.] %s  ExtD(0)-%X ExtD(1)-%X Flg-%X SA-%X NL-%X NS-%X NSC-%02X (",
		 (unsigned int)DirNo,
		 NameSpaceStrings[root->NameSpace & 0xF],
		 (unsigned int)root->ExtendedDirectoryChain0,
		 (unsigned int)root->ExtendedDirectoryChain1,
		 (unsigned int)root->Flags,
		 (unsigned int)root->SubAllocationList,
		 (unsigned int)root->NameList,
		 (unsigned int)root->NameSpace,
		 (unsigned int)root->NameSpaceCount);
	  for (j=0, k=0; j < root->NameSpaceCount; j++)
	  {
	     if (j & 1)
	     {
		nameS = (root->SupportedNameSpacesNibble[k++] >> 4) & 0xF;
		printf("%s ", NameSpaceStrings[nameS & 0xF]);
	     }
	     else
	     {
		nameS = root->SupportedNameSpacesNibble[k] & 0xF;
		printf("%s ", NameSpaceStrings[nameS & 0xF]);
	     }
	  }
	  printf(")\n");
	  if (flag)
	  {
	     dumpRecord((BYTE *)dos, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
	  }
	  return 1;

       case RESTRICTION_NODE:
	  user = (USER *) dos;
	  printf("%X TrusteeCount-%08X  Subdir-%08X Flag-%02X\n",
		  (unsigned int)DirNo,
		  (unsigned int)user->TrusteeCount,
		  (unsigned int)user->Subdirectory,
		  (unsigned int)user->Flag);

	  for (j=0; (j < user->TrusteeCount) && (j < 14); j++)
	  {
	     printf("  Trustee: %08X  Restrictions: %08X\n",
		     (unsigned int)user->Trustees[j],
		     (unsigned int)user->Restriction[j]);
	  }
	  if (flag)
	  {
	     dumpRecord((BYTE *)dos, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
	  }
	  return 1;

       case SUBALLOC_NODE:
	  suballoc = (SUBALLOC *) dos;
	  printf("%X  Sequence-%02X  SuballocList-%08X ID-%02X\n",
		  (unsigned int)DirNo,
		  (unsigned int)suballoc->SequenceNumber,
		  (unsigned int)suballoc->SubAllocationList,
		  (unsigned int)suballoc->ID);
	  if (flag)
	  {
	     dumpRecord((BYTE *)dos, sizeof(ROOT));
	     dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
	  }
	  return 1;

       default:
	  switch (dos->NameSpace)
	  {
	     case DOS_NAME_SPACE:
		strncpy(PathString, dos->FileName, dos->FileNameLength);
		PathString[dos->FileNameLength] = '\0';

		if (dos->Flags & SUBDIRECTORY_FILE)
		{
		   subdir = (SUB_DIRECTORY *) dos;
		   printf("[%X] %-12s(%d)   <DIR>      %s NL-%X ID-%X Flg-%X p-%X\n",
			  (unsigned int)DirNo,
			  PathString,
			  (int)subdir->FileNameLength,
			  NameSpaceStrings[subdir->NameSpace & 0xF],
			  (unsigned int)subdir->NameList,
			  (unsigned int)subdir->PrimaryEntry,
			  (unsigned int)subdir->Flags,
			  (unsigned int)subdir->Subdirectory);
		}
		else
		{
		   printf("[%X] %-12s(%d)%s %12u %s NL-%X ID-%X Flg-%X p-%X [%8X]\n",
			  (unsigned int)DirNo,
			  PathString,
			  (int)dos->FileNameLength,
			  (dos->FileAttributes & DATA_STREAM_IS_COMPRESSED)
			  ? "[COMP]" : "",
			  (unsigned int)dos->FileSize,
			  NameSpaceStrings[dos->NameSpace & 0xF],
			  (unsigned int)dos->NameList,
			  (unsigned int)dos->PrimaryEntry,
			  (unsigned int)dos->Flags,
			  (unsigned int)dos->Subdirectory,
			  (unsigned int)dos->FirstBlock);

		   printf("   DFT-%08X DDT-%08X DID-%08X DBSN-%08X\n",
			  (unsigned int)dos->DeletedFileTime,
			  (unsigned int)dos->DeletedDateAndTime,
			  (unsigned int)dos->DeletedID,
			  (unsigned int)dos->DeletedBlockSequenceNumber);
		}
		if (flag)
		{
		   dumpRecord((BYTE *)dos, sizeof(ROOT));
		   dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
		}
		return 1;

	     case MAC_NAME_SPACE:
		mac = (MACINTOSH *) dos;
		strncpy(PathString, mac->FileName, mac->FileNameLength);
		PathString[mac->FileNameLength] = '\0';

		(mac->Flags & SUBDIRECTORY_FILE)
		? printf("[%X] [%-12s](%d) %s NL-%X ID-%X FLG-%02X p-%X FORK[%X]\n",
			  (unsigned int)DirNo,
			  PathString,
			  (int)mac->FileNameLength,
			  NameSpaceStrings[mac->NameSpace & 0xF],
			  (unsigned int)mac->NameList,
			  (unsigned int)mac->PrimaryEntry,
			  (unsigned int)mac->Flags,
			  (unsigned int)mac->Subdirectory,
			  (unsigned int)mac->ResourceFork)
		: printf("[%X] %-12s(%d) %12u %s NL-%X ID-%X FLG-%02X p-%X FORK[%X]\n",
			  (unsigned int)DirNo,
			  PathString,
			  (int)mac->FileNameLength,
			  (unsigned int)mac->ResourceForkSize,
			  NameSpaceStrings[mac->NameSpace & 0xF],
			  (unsigned int)mac->NameList,
			  (unsigned int)mac->PrimaryEntry,
			  (unsigned int)mac->Flags,
			  (unsigned int)mac->Subdirectory,
			  (unsigned int)mac->ResourceFork);
		if (flag)
		{
		   dumpRecord((BYTE *)dos, sizeof(ROOT));
		   dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
		}
		return 1;

	     case UNIX_NAME_SPACE:
		nfs = (NFS *) dos;
		strncpy(PathString, nfs->FileName, nfs->FileNameLength);
		PathString[nfs->FileNameLength] = '\0';

		(nfs->Flags & SUBDIRECTORY_FILE)
		? printf("[%X] [%-12s](%d) [%u:%u:%u] %s NL-%X ID-%X FLG-%02X p-%X [%8X]\n",
			  (unsigned int)DirNo,
			  PathString,
			  (int)nfs->FileNameLength,
			  (unsigned int)nfs->TotalFileNameLength,
			  (unsigned int)nfs->ExtantsUsed,
			  (unsigned int)nfs->StartExtantNumber,
			  NameSpaceStrings[nfs->NameSpace & 0xF],
			  (unsigned int)nfs->NameList,
			  (unsigned int)nfs->PrimaryEntry,
			  (unsigned int)nfs->Flags,
			  (unsigned int)nfs->Subdirectory,
			  (unsigned int)nfs->StartExtantNumber)
		: printf("[%X] %-12s(%d) [%u:%u:%u] %s NL-%X ID-%X FLG-%02X p-%X [%8X]\n",
			  (unsigned int)DirNo,
			  PathString,
			  (int)nfs->FileNameLength,
			  (unsigned int)nfs->TotalFileNameLength,
			  (unsigned int)nfs->ExtantsUsed,
			  (unsigned int)nfs->StartExtantNumber,
			  NameSpaceStrings[nfs->NameSpace & 0xF],
			  (unsigned int)nfs->NameList,
			  (unsigned int)nfs->PrimaryEntry,
			  (unsigned int)nfs->Flags,
			  (unsigned int)nfs->Subdirectory,
			  (unsigned int)nfs->StartExtantNumber);
		if (flag)
		{
		   dumpRecord((BYTE *)dos, sizeof(ROOT));
		   dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
		}
		return 1;


	     case LONG_NAME_SPACE:
		win95 = (LONGNAME *) dos;
		strncpy(PathString, win95->FileName, win95->FileNameLength);
		PathString[win95->FileNameLength] = '\0';

		(win95->Flags & SUBDIRECTORY_FILE)
		? printf("[%X] [%-12s](%d) [%u:%u:%u] %s NL-%X ID-%X FLG-%02X p-%X [%8X]\n",
			  (unsigned int)DirNo,
			  PathString,
			  (int)win95->FileNameLength,
			  (unsigned int)win95->ExtendedSpace,
			  (unsigned int)win95->ExtantsUsed,
			  (unsigned int)win95->LengthData,
			  NameSpaceStrings[win95->NameSpace & 0xF],
			  (unsigned int)win95->NameList,
			  (unsigned int)win95->PrimaryEntry,
			  (unsigned int)win95->Flags,
			  (unsigned int)win95->Subdirectory,
			  (unsigned int)win95->ExtendedSpace)
		: printf("[%X] %-12s(%d) [%u:%u:%u] %s NL-%X ID-%X FLG-%02X p-%X [%8X]\n",
			  (unsigned int)DirNo,
			  PathString,
			  (int)win95->FileNameLength,
			  (unsigned int)win95->ExtendedSpace,
			  (unsigned int)win95->ExtantsUsed,
			  (unsigned int)win95->LengthData,
			  NameSpaceStrings[win95->NameSpace & 0xF],
			  (unsigned int)win95->NameList,
			  (unsigned int)win95->PrimaryEntry,
			  (unsigned int)win95->Flags,
			  (unsigned int)win95->Subdirectory,
			  (unsigned int)win95->ExtendedSpace);
		if (flag)
		{
		   dumpRecord((BYTE *)dos, sizeof(ROOT));
		   dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
		}
		return 1;

	     case NT_NAME_SPACE:
		nt = (NTNAME *) dos;
		strncpy(PathString, nt->FileName, nt->FileNameLength);
		PathString[nt->FileNameLength] = '\0';
		(dos->Flags & SUBDIRECTORY_FILE)
		? printf("[%X] [%-23s](%d) [%u:%u:%u] %s NL-%X ID-%X FLG-%02X p-%X\n",
			  (unsigned int)DirNo,
			  PathString,
			  (int)nt->FileNameLength,
			  (unsigned int)nt->ExtendedSpace,
			  (unsigned int)nt->ExtantsUsed,
			  (unsigned int)nt->LengthData,
			  NameSpaceStrings[nt->NameSpace & 0xF],
			  (unsigned int)nt->NameList,
			  (unsigned int)nt->PrimaryEntry,
			  (unsigned int)nt->Flags,
			  (unsigned int)nt->Subdirectory)
		: printf("[%X] %-23s(%d) [%u:%u:%u] %s NL-%X ID-%X FLG-%02X p-%X\n",
			  (unsigned int)DirNo,
			  PathString,
			  (int)nt->FileNameLength,
			  (unsigned int)nt->ExtendedSpace,
			  (unsigned int)nt->ExtantsUsed,
			  (unsigned int)nt->LengthData,
			  NameSpaceStrings[nt->NameSpace & 0xF],
			  (unsigned int)nt->NameList,
			  (unsigned int)nt->PrimaryEntry,
			  (unsigned int)nt->Flags,
			  (unsigned int)nt->Subdirectory);
		if (flag)
		{
		   dumpRecord((BYTE *)dos, sizeof(ROOT));
		   dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
		}
		return 1;

	     case FTAM_NAME_SPACE:
	     default:
		if (flag)
		{
		   dumpRecord((BYTE *)dos, sizeof(ROOT));
		   dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
		}
		return 0;
	  }
	  break;
    }


}

ULONG atox(BYTE *p)
{
    register ULONG c = 0;

    if (!p)
       return 0;

    while (*p)
    {
       if (*p >= '0' && *p <= '9')
	  c = (c << 4) | (*p - '0');
       else
       if (*p >= 'A' && *p <= 'F')
	  c = (c << 4) | (*p - 'A' + 10);
       else
       if (*p >= 'a' && *p <= 'f')
	  c = (c << 4) | (*p - 'a' + 10);
       else
	  break;
       p++;
    }
    return (c);
}


unsigned long ipause(void)
{
   char key;

   NWFSPrint(" --- (Enter) Continue or (R) Return --- ");
   key = getc(stdin);

   NWFSPrint("%c                                    %c", '\r', '\r');

   switch (toupper(key))
   {
      case 'R':
      case 'X':
      case 'Q':
	 return 1;

      case 'C':
      default:
	 return 0;
   }
}

void DisplayASCIITable(void)
{
    register ULONG i, lines = 0;
    union bhex
    {
       unsigned int i;
       struct btemp {
	     unsigned one : 1;
	     unsigned two : 1;
	     unsigned three : 1;
	     unsigned four : 1;
	     unsigned five : 1;
	     unsigned six : 1;
	     unsigned seven : 1;
	     unsigned eight : 1;
       } b;
    } val;

    NWFSPrint("ASCII Table\n");
    for (i=0; i < 256; i++)
    {
       val.i = i;
       switch (i)
       {

	  case 0:
	     NWFSPrint("|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | NULL  |",
		       (int)i,
		       (unsigned int)i,
		       (int)val.b.eight,
		       (int)val.b.seven,
		       (int)val.b.six,
		       (int)val.b.five,
		       (int)val.b.four,
		       (int)val.b.three,
		       (int)val.b.two,
		       (int)val.b.one);
	     break;

	  case 7:
	     NWFSPrint("|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | BELL  |",
		       (int)i,
		       (unsigned int)i,
		       (int)val.b.eight,
		       (int)val.b.seven,
		       (int)val.b.six,
		       (int)val.b.five,
		       (int)val.b.four,
		       (int)val.b.three,
		       (int)val.b.two,
		       (int)val.b.one);
	     break;

	  case 8:
	     NWFSPrint("|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | BKSP  |",
		       (int)i,
		       (unsigned int)i,
		       (int)val.b.eight,
		       (int)val.b.seven,
		       (int)val.b.six,
		       (int)val.b.five,
		       (int)val.b.four,
		       (int)val.b.three,
		       (int)val.b.two,
		       (int)val.b.one);
	     break;

	  case 9:
	     NWFSPrint("|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | TAB   |",
		       (int)i,
		       (unsigned int)i,
		       (int)val.b.eight,
		       (int)val.b.seven,
		       (int)val.b.six,
		       (int)val.b.five,
		       (int)val.b.four,
		       (int)val.b.three,
		       (int)val.b.two,
		       (int)val.b.one);
	     break;

	  case 10:
	     NWFSPrint("|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | <CR>  |",
		       (int)i,
		       (unsigned int)i,
		       (int)val.b.eight,
		       (int)val.b.seven,
		       (int)val.b.six,
		       (int)val.b.five,
		       (int)val.b.four,
		       (int)val.b.three,
		       (int)val.b.two,
		       (int)val.b.one);
	     break;

	  case 13:
	     NWFSPrint("|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | <LF>  |",
		       (int)i,
		       (unsigned int)i,
		       (int)val.b.eight,
		       (int)val.b.seven,
		       (int)val.b.six,
		       (int)val.b.five,
		       (int)val.b.four,
		       (int)val.b.three,
		       (int)val.b.two,
		       (int)val.b.one);
	     break;

	  case 32:
	     NWFSPrint("|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | SPACE |",
		       (int)i,
		       (unsigned int)i,
		       (int)val.b.eight,
		       (int)val.b.seven,
		       (int)val.b.six,
		       (int)val.b.five,
		       (int)val.b.four,
		       (int)val.b.three,
		       (int)val.b.two,
		       (int)val.b.one);
	     break;

	  default:
	     NWFSPrint("|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  |  %c    |",
		       (int)i,
		       (unsigned int)i,
		       (int)val.b.eight,
		       (int)val.b.seven,
		       (int)val.b.six,
		       (int)val.b.five,
		       (int)val.b.four,
		       (int)val.b.three,
		       (int)val.b.two,
		       (int)val.b.one,
		       (BYTE) i);
	     break;

       }
       NWFSPrint("\n");
       if (lines++ > 16)
       {
	 if (ipause())
	    break;
	  lines = 0;
       }
    }

}

void UpperCaseString(BYTE *p)
{
    while (*p)
    {
       *p = toupper(*p);
       p++;
    }
    return;
}

DOS dosst;
DOS *dos = (DOS *) &dosst;
BYTE buf[65536];

int main(int argc, char *argv[])
{
    register ULONG ccode, dirno, count, records, i, rm = 0;
    ULONG retCode, value, dirFileSize, dirBlocks, dirTotal;
    register VOLUME *volume;
    register BYTE *p;
    BYTE input_buffer[255] = {""};
    extern void UtilScanVolumes(void);
    
    InitNWFS();
    UtilScanVolumes();

    NWFSPrint("NetWare Volume(s)\n\n");
    for (i = 0; i < MAX_VOLUMES; i++)
    {
       if (VolumeTable[i])
       {
	  volume = VolumeTable[i];
	  if (volume && volume->VolumePresent)
	  {
	     NWFSPrint("%d.  %-15s\n", (int)(i + 1), volume->VolumeName);
	  }
       }
    }
    NWFSPrint("\n");

    NWFSPrint("Enter Volume Selection: ");
    fgets(input_buffer, sizeof(input_buffer), stdin);

    UpperCaseString(input_buffer);
    if (!input_buffer[0])
       return 0;

    value = atoi(input_buffer);
    if (!value || (value >= MAX_VOLUMES))
       return 0;

    if (!VolumeTable[value - 1])
    {
       NWFSPrint("Invalid Volume Number (%d)\n", (int)value);
       return 0;
    }

    volume = GetVolumeHandle(VolumeTable[value - 1]->VolumeName);
    if (!volume)
    {
       NWFSPrint("Could Find Volume [%s]\n", argv[1]);
       return 0;
    }

    ccode = MountRawVolume(volume);
    if (ccode)
    {
       NWFSPrint("error mounting volume %s\n", volume->VolumeName);
       return 0;
    }
    rm = TRUE;

    dirFileSize = GetChainSize(volume, volume->FirstDirectory);
    dirBlocks = (dirFileSize + (volume->BlockSize - 1)) / volume->BlockSize;
    dirTotal = (dirFileSize + (sizeof(DOS) - 1)) / sizeof(DOS);

    while (TRUE)
    {
       NWFSPrint("%s> ", volume->VolumeName);
       fgets(input_buffer, sizeof(input_buffer), stdin);
       UpperCaseString(input_buffer);

       if (!NWFSCompare(input_buffer, "?", (ULONG)strlen("?")))
       {
	  NWFSPrint("?                - this help panel\n");
	  NWFSPrint(".E               - evaluate numeric expression\n");
	  NWFSPrint("Q                - quit\n");
	  NWFSPrint("CLS              - clear screen\n");
	  NWFSPrint("ASCII            - display ASCII table\n");
	  NWFSPrint("DIR <rec_number> - display dir starting from #\n");
	  NWFSPrint("DN  <name>       - display dir starting from name\n");
	  NWFSPrint("DP  <parent>     - display dir for parent\n");
	  NWFSPrint("EDIR <rec>       - display edir starting from #\n");
	  NWFSPrint("EDP  <parent>    - display edir for parent\n");
	  NWFSPrint("REC <rec_number> - dump a dir record\n");
	  NWFSPrint("FAT <index>      - dump a FAT table\n");
	  NWFSPrint("FS  <cluster>    - search the FAT table for a cluster\n");
	  NWFSPrint("INFO             - display FAT/DIR Locations\n");
          NWFSPrint("CLUSTER <number> - dump a cluster of data\n");
	  continue;
       }
       else
       if (!NWFSCompare(input_buffer, "MOUNT", (ULONG)strlen("MOUNT")))
       {
	  if (rm)
	  {
	     DismountRawVolume(volume);
	     ccode = MountVolumeByHandle(volume);
	     if (ccode)
	     {
		NWFSPrint("error mounting volume %s\n", volume->VolumeName);
	     }
	     rm = 0;
	  }
	  continue;
       }
       else
       if (!NWFSCompare(input_buffer, "UMOUNT", (ULONG)strlen("UMOUNT")))
       {
	  if (!rm)
	  {
	     DismountVolumeByHandle(volume);
	     ccode = MountRawVolume(volume);
	     if (ccode)
	     {
		NWFSPrint("error mounting volume %s\n", volume->VolumeName);
	     }
	     rm = TRUE;
	  }
	  continue;
       }
       else
       if (!NWFSCompare(input_buffer, "CLUSTER", (ULONG)strlen("CLUSTER")))
       {
	  register ULONG cluster = 0, block = 0, ccode, j, r, s;
          nwvp_asynch_map map[8];
	  ULONG count = 0;
	  
	  p = (BYTE *) &input_buffer[7];
	  count = 7;
	  while (*p && (*p == ' ') && (count < 255))
	  {
	     count++;
	     p++;
	  }

	  cluster = atox(p);

	  ccode = ReadPhysicalVolumeCluster(volume, cluster, buf,
			                 volume->ClusterSize, DATA_PRIORITY);
          if (ccode)
	  {
             NWFSPrint("error reading custer-%X\n", (unsigned)cluster);
             continue;
	  }
	  
          block = cluster * volume->BlocksPerCluster;
	  for (r=0, i=0; i < volume->BlocksPerCluster; i++)
	  {
             nwvp_vpartition_map_asynch_read(volume->nwvp_handle,
                                             block + i, 
					     &count,
					     &map[0]);
     
             for (s=0, j=0; j < volume->BlockSize / 256; j++, r++)
	     {
		NWFSPrint("cluster-%X offset-%d bytes block-%d lba-%X\n", 
		          (unsigned)cluster, (int)(r * 256),
		          (int)block, 
		          (unsigned)map[0].sector_offset + s);
     	        dumpRecordBytes(&buf[r * 256], 256);
                if (ipause())
	           goto cluster_end;

		if (j & 1)
		   s++;
	     }
	  }

cluster_end:;
	  continue;
       }
       else
       if (!NWFSCompare(input_buffer, "Q", (ULONG)strlen("Q")))
       {
	  break;
       }
       else
       if (!NWFSCompare(input_buffer, "INFO", (ULONG)strlen("INFO")))
       {
	  printf("Volume [%-15s]\n", volume->VolumeName);
	  printf("FirstFAT-%08X  SecondFAT-%08X\n",
		 (unsigned int)volume->FirstFAT,
		 (unsigned int)volume->SecondFAT);
	  printf("FirstDir-%08X  SecondDir-%08X\n",
		 (unsigned int)volume->FirstDirectory,
		 (unsigned int)volume->SecondDirectory);
	  continue;
       }
       else
       if (!NWFSCompare(input_buffer, ".E", (ULONG)strlen(".E")))
       {
	  break;
       }
       else
       if (!NWFSCompare(input_buffer, "CLS", (ULONG)strlen("CLS")))
       {
	  ClearScreen(0);
	  continue;
       }
       else
       if (!NWFSCompare(input_buffer, "ASCII", (ULONG)strlen("ASCII")))
       {
	  DisplayASCIITable();
	  continue;
       }
       else
       if (!NWFSCompare(input_buffer, "FAT", (ULONG)strlen("FAT")))
       {
	  register FAT_ENTRY *fat;
          FAT_ENTRY FAT_S;
	  register ULONG cluster = 0;

	  p = (BYTE *) &input_buffer[3];
	  count = 3;
	  while (*p && (*p == ' ') && (count < 255))
	  {
	     count++;
	     p++;
	  }

	  records = 0;
	  cluster = atox(p);
	  for (i=cluster; i < volume->VolumeClusters; i++)
	  {
	     fat = GetFatEntry(volume, i, &FAT_S);
	     printf("0x%08X: i-%08X c-%08X  ",
		    (unsigned int)i,
		    (unsigned int)fat->FATIndex,
		    (unsigned int)fat->FATCluster);

	     if (i & 1)
	     {
		printf("\n");
		records++;
		if (records > 10)
		{
		  if (ipause())
		     break;
		  records = 0;
		}
	     }
	  }
	  continue;
       }
       else
       if (!NWFSCompare(input_buffer, "FS", (ULONG)strlen("FS")))
       {
	  register FAT_ENTRY *fat;
          FAT_ENTRY FAT_S;
	  register ULONG cluster = 0;
	  register MACINTOSH *mac;

	  p = (BYTE *) &input_buffer[3];
	  count = 3;
	  while (*p && (*p == ' ') && (count < 255))
	  {
	     count++;
	     p++;
	  }

	  records = 0;
	  cluster = atox(p);
	  for (i=0; i < volume->VolumeClusters; i++)
	  {
	     fat = GetFatEntry(volume, i, &FAT_S);
	     if (fat->FATCluster == (long)cluster)
	     {
		printf("0x%08X: i-%08X c-%08X  ",
		       (unsigned int)i,
		       (unsigned int)fat->FATIndex,
		       (unsigned int)fat->FATCluster);

		if (ipause())
		   break;
	     }
	  }

	  for (i=0; i < volume->SuballocListCount; i++)
	  {
	     if (cluster == volume->SuballocChain[i / 28]->StartingFATChain[i % 28])
	     {
		printf("sa %d/%d-[%d] ", (int)(i / 28), (int)(i % 28), (int)i);
		if (ipause())
		   break;
	     }
	  }

	  for (dirno = 0; dirno < dirTotal; dirno++)
	  {
	     ccode = NWReadFile(volume,
				&volume->FirstDirectory,
				0,
				-1,
				dirno * sizeof(DOS),
				(BYTE *)dos,
				sizeof(DOS),
				0,
				0,
				&retCode,
				KERNEL_ADDRESS_SPACE,
				0,
				0,
				TRUE);
	     if (ccode != sizeof(DOS))
	     {
		NWFSPrint("\nerror reading volume directory file\n");
		break;
	     }

	     if (dos->NameSpace == DOS_NAME_SPACE)
	     {
		if (dos->FirstBlock == cluster)
		{
		   DisplayRecord(dos, dirno, 0);
		   if (ipause())
		      break;
		}
	     }
	     else
	     if (dos->NameSpace == MAC_NAME_SPACE)
	     {
		mac = (MACINTOSH *)dos;
		if (mac->ResourceFork == cluster)
		{
		   DisplayRecord(dos, dirno, 0);
		   if (ipause())
		      break;
		}
	     }
	  }
	  continue;
       }
       else
       if (!NWFSCompare(input_buffer, "DIR", (ULONG)strlen("DIR")))
       {
          register BYTE *tmp;
     
          p = (BYTE *) &input_buffer[3];
	  count = 3;
	  while (*p && (*p == ' ') && (count < 80))
	  {
	     count++;
	     p++;
	  }

          tmp = p;
	  while (*tmp)
	  {
             if ((*tmp == '\n') || (*tmp == '\r')) 
	        *tmp = '\0';
	     tmp++;
	  }
	  
	  dirno = atox(p);
	  records = 0;
	  for (i = dirno; dirno < dirTotal; dirno++)
	  {
	     ccode = NWReadFile(volume,
				&volume->FirstDirectory,
				0,
				-1,
				dirno * sizeof(DOS),
				(BYTE *)dos,
				sizeof(DOS),
				0,
				0,
				&retCode,
				KERNEL_ADDRESS_SPACE,
				0,
				0,
				TRUE);
	     if (ccode != sizeof(DOS))
	     {
		NWFSPrint("\nerror reading volume directory file\n");
		break;
	     }

	     if (DisplayRecord(dos, dirno, 0))
		records++;
	     if (records > 10)
	     {
		if (ipause())
		   break;
		records = 0;
	     }
	  }
       }
       else
       if (!NWFSCompare(input_buffer, "EDIR", (ULONG)strlen("EDIR")))
       {
          register BYTE *tmp;
     
          p = (BYTE *) &input_buffer[4];
	  count = 4;
	  while (*p && (*p == ' ') && (count < 80))
	  {
	     count++;
	     p++;
	  }

          tmp = p;
	  while (*tmp)
	  {
             if ((*tmp == '\n') || (*tmp == '\r')) 
	        *tmp = '\0';
	     tmp++;
	  }
	  
	  dirno = atox(p);
	  records = 0;
	  for (i = dirno; dirno < dirTotal; dirno++)
	  {
	     ccode = NWReadFile(volume,
				&volume->ExtDirectory1,
				0,
				-1,
				dirno * sizeof(DOS),
				(BYTE *)dos,
				sizeof(DOS),
				0,
				0,
				&retCode,
				KERNEL_ADDRESS_SPACE,
				0,
				0,
				TRUE);
	     if (ccode != sizeof(DOS))
	     {
		NWFSPrint("\nerror reading ext directory file\n");
		break;
	     }

	     if (DumpExtendedRecord((EXTENDED_DIR *)dos, dirno, 0))
		records++;
	     if (records > 10)
	     {
		if (ipause())
		   break;
		records = 0;
	     }
	  }
       }
       else
       if (!NWFSCompare(input_buffer, "DN", (ULONG)strlen("DN")))
       {
          register MACINTOSH *mac = (MACINTOSH *)dos;
          register LONGNAME *ln = (LONGNAME *)dos;
          register NFS *nfs = (NFS *)dos;
          register NTNAME *nt = (NTNAME *)dos;
          register BYTE *tmp;
	  
	  p = (BYTE *) &input_buffer[2];
	  count = 2;
	  while (*p && (*p == ' ') && (count < 80))
	  {
	     count++;
	     p++;
	  }

          tmp = p;
	  while (*tmp)
	  {
             if ((*tmp == '\n') || (*tmp == '\r')) 
	        *tmp = '\0';
	     tmp++;
	  }
	  
	  if (!*p)
	     break;

	  records = 0;
	  for (dirno = 0; dirno < dirTotal; dirno++)
	  {
	     ccode = NWReadFile(volume,
				&volume->FirstDirectory,
				0,
				-1,
				dirno * sizeof(DOS),
				(BYTE *)dos,
				sizeof(DOS),
				0,
				0,
				&retCode,
				KERNEL_ADDRESS_SPACE,
				0,
				0,
				TRUE);
	     if (ccode != sizeof(DOS))
	     {
		NWFSPrint("\nerror reading volume directory file\n");
		break;
	     }

             switch (dos->Subdirectory)
	     {
                case FREE_NODE:
		case TRUSTEE_NODE:
		case ROOT_NODE:
		case RESTRICTION_NODE:
		case SUBALLOC_NODE:
		   break;
			
     	        default:
                   switch (dos->NameSpace)
		   {
     		      case DOS_NAME_SPACE:
     		         if (!NWFSCompare(dos->FileName, p, strlen(p)))
	                 {
		            if (DisplayRecord(dos, dirno, 0))
		               records++;
		            if (records > 10)
		            {
		               if (ipause())
		                  goto dn_done;
		               records = 0;
			    }
			 }
			 break;

     		      case LONG_NAME_SPACE:
     		         if (!NWFSCompare(ln->FileName, p, strlen(p)))
	                 {
		            if (DisplayRecord(dos, dirno, 0))
		               records++;
		            if (records > 10)
		            {
		               if (ipause())
		                  goto dn_done;
		               records = 0;
			    }
			 }
			 break;

     		      case NT_NAME_SPACE:
     		         if (!NWFSCompare(nt->FileName, p, strlen(p)))
	                 {
		            if (DisplayRecord(dos, dirno, 0))
		               records++;
		            if (records > 10)
		            {
		               if (ipause())
		                  goto dn_done;
		               records = 0;
			    }
			 }
			 break;

     		      case UNIX_NAME_SPACE:
     		         if (!NWFSCompare(nfs->FileName, p, strlen(p)))
	                 {
		            if (DisplayRecord(dos, dirno, 0))
		               records++;
		            if (records > 10)
		            {
		               if (ipause())
		                  goto dn_done;
		               records = 0;
			    }
			 }
			 break;

     		      case MAC_NAME_SPACE:
     		         if (!NWFSCompare(mac->FileName, p, strlen(p)))
	                 {
		            if (DisplayRecord(dos, dirno, 0))
		               records++;
		            if (records > 10)
		            {
		               if (ipause())
		                  goto dn_done;
		               records = 0;
			    }
			 }
			 break;

		      default:
			 break;
		   
                }
   	     }
	  }
dn_done:;
       }
       else
       if (!NWFSCompare(input_buffer, "DP", (ULONG)strlen("DP")))
       {
          register BYTE *tmp;
	  register ULONG parent;
	  
	  p = (BYTE *) &input_buffer[2];
	  count = 2;
	  while (*p && (*p == ' ') && (count < 80))
	  {
	     count++;
	     p++;
	  }

          tmp = p;
	  while (*tmp)
	  {
             if ((*tmp == '\n') || (*tmp == '\r')) 
	        *tmp = '\0';
	     tmp++;
	  }
	  
	  if (!*p)
	     break;

	  parent = atox(p);
	  records = 0;
	  for (dirno = 0; dirno < dirTotal; dirno++)
	  {
	     ccode = NWReadFile(volume,
				&volume->FirstDirectory,
				0,
				-1,
				dirno * sizeof(DOS),
				(BYTE *)dos,
				sizeof(DOS),
				0,
				0,
				&retCode,
				KERNEL_ADDRESS_SPACE,
				0,
				0,
				TRUE);
	     if (ccode != sizeof(DOS))
	     {
		NWFSPrint("\nerror reading volume directory file\n");
		break;
	     }

             switch (dos->Subdirectory)
	     {
                case FREE_NODE:
		case TRUSTEE_NODE:
		case ROOT_NODE:
		case RESTRICTION_NODE:
		case SUBALLOC_NODE:
		   break;
			
     	        default:
                   if (dos->Subdirectory == parent)
                   {
                      if (DisplayRecord(dos, dirno, 0))
                         records++;
                      if (records > 10)
                      {
                         if (ipause())
                            goto dp_done;
                         records = 0;
	              }
	           }
	           break;
   	     }
	  }
dp_done:;
       }
       else
       if (!NWFSCompare(input_buffer, "REC", (ULONG)strlen("REC")))
       {
	  p = (BYTE *) &input_buffer[3];
	  count = 3;
	  while (*p && (*p == ' ') && (count < 255))
	  {
	     count++;
	     p++;
	  }

	  dirno = atox(p);
	  if (dirno < dirTotal)
	  {
	     ccode = NWReadFile(volume,
				&volume->FirstDirectory,
				0,
				-1,
				dirno * sizeof(DOS),
				(BYTE *)dos,
				sizeof(DOS),
				0,
				0,
				&retCode,
				KERNEL_ADDRESS_SPACE,
				0,
				0,
				TRUE);
	     if (ccode != sizeof(DOS))
	     {
		NWFSPrint("\nerror reading volume directory file\n");
	     }
	     else
		DisplayRecord(dos, dirno, TRUE);
	  }
       }
       else
       if (!NWFSCompare(input_buffer, "EREC", (ULONG)strlen("EREC")))
       {
	  p = (BYTE *) &input_buffer[4];
	  count = 4;
	  while (*p && (*p == ' ') && (count < 255))
	  {
	     count++;
	     p++;
	  }

	  dirno = atox(p);
	  if (dirno < dirTotal)
	  {
	     ccode = NWReadFile(volume,
				&volume->ExtDirectory1,
				0,
				-1,
				dirno * sizeof(DOS),
				(BYTE *)dos,
				sizeof(DOS),
				0,
				0,
				&retCode,
				KERNEL_ADDRESS_SPACE,
				0,
				0,
				TRUE);
	     if (ccode != sizeof(DOS))
	     {
		NWFSPrint("\nerror reading volume directory file\n");
	     }
	     else
	     {
		if (!DumpExtendedRecord((EXTENDED_DIR *)dos, dirno, TRUE))
		{
	           dumpRecord((BYTE *)dos, sizeof(ROOT));
	           dumpRecordBytes((BYTE *)dos, sizeof(ROOT));
		}
	     }
	  }
       }
    }
    if (rm)
       DismountRawVolume(volume);
    else
       DismountVolumeByHandle(volume);
    NWFSVolumeClose();
    RemoveDiskDevices();
    FreePartitionResources();

    return 0;

}




