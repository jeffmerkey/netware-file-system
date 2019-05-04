
/***************************************************************************
*
*   Copyright (c) 1997-2001 Jeff V. Merkey  All Rights
*			    Reserved.
*
*   This program is an unpublished work of Jeff V. Merkey and contains
*   trade secrets and other proprietary information.  Unauthorized
*   use, copying, disclosure, or distribution of this file without the
*   consent of Jeff V. Merkey can subject the offender to severe criminal
*   and/or civil penalities.
*
*
*   AUTHORS  :  Jeff V. Merkey, J. 
*   FILE     :  IMAGE.H
*   DESCRIP  :  NWFS Imaging Utility Includes
*   DATE     :  August 5, 1999
*
*
***************************************************************************/

#ifndef _IMAGE_INCLUDE_
#define _IMAGE_INCLUDE_

#define	IMAGE_SET_STAMP			0x44447777
#define	IMAGE_FILE_STAMP		0x77774444
#define	IMAGE_TABLE_SIZE		64
#define	IMAGE_SET_TABLE_SIZE	        256

#if (LINUX_UTIL | DOS_UTIL)

typedef struct _IMAGE_HEADER
{
   ULONG Stamp                      __attribute__ ((packed));
   ULONG CheckSum                   __attribute__ ((packed));
   ULONG Filler[2]                  __attribute__ ((packed));
   BYTE DateTimeString[32]          __attribute__ ((packed));
   BYTE VolumeName[20]              __attribute__ ((packed));
   BYTE ImageSetName[20]            __attribute__ ((packed));

   ULONG VolumeClusterSize          __attribute__ ((packed));
   ULONG LogicalVolumeClusters      __attribute__ ((packed));
   ULONG AllocatedVolumeClusters    __attribute__ ((packed));
   ULONG FilesInDataSet             __attribute__ ((packed));
   ULONG SegmentCount               __attribute__ ((packed));
   ULONG MirrorFlag                 __attribute__ ((packed));
   BYTE VolumeFile[20]              __attribute__ ((packed));
} IMAGE;

typedef struct _IMAGE_SET_HEADER
{
   ULONG Stamp                      __attribute__ ((packed));
   ULONG CheckSum                   __attribute__ ((packed));
   ULONG VolumeCount                __attribute__ ((packed));
   ULONG Filler                     __attribute__ ((packed));
   BYTE DateTimeString[32]          __attribute__ ((packed));
   BYTE VolumeFile[IMAGE_TABLE_SIZE][16] __attribute__ ((packed));
} IMAGE_SET;

typedef struct _FILE_DATA_
{
   LONGLONG FileSize                __attribute__ ((packed));
   LONGLONG FileCompressed          __attribute__ ((packed));
   LONGLONG FileOffset              __attribute__ ((packed));
   ULONG Checksum                   __attribute__ ((packed));
} FILE_DATA;

#endif

#if (WINDOWS_NT_UTIL)

typedef struct _IMAGE_HEADER
{
   ULONG Stamp;
   ULONG CheckSum;
   ULONG Filler[2];
   BYTE DateTimeString[32];
   BYTE VolumeName[20];
   BYTE ImageSetName[20];

   ULONG VolumeClusterSize;
   ULONG LogicalVolumeClusters;
   ULONG AllocatedVolumeClusters;
   ULONG FilesInDataSet;
   ULONG SegmentCount;
   ULONG MirrorFlag;
   BYTE VolumeFile[20];
} IMAGE;

typedef struct _IMAGE_SET_HEADER
{
   ULONG Stamp;
   ULONG CheckSum;
   ULONG VolumeCount;
   ULONG Filler;
   BYTE DateTimeString[32];
   BYTE VolumeFile[IMAGE_TABLE_SIZE][16];
} IMAGE_SET;

typedef struct _FILE_DATA_
{
   LONGLONG FileSize;
   LONGLONG FileCompressed;
   LONGLONG FileOffset;
   ULONG Checksum;
} FILE_DATA;

#endif

#endif


