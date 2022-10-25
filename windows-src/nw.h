/*++

Copyright (c) 1999-2022 Jeffrey V. Merkey

Module Name:

    nw.h

Abstract:

    This module defines the on-disk structure of the Netware file system. They
    are mainly used to interpret data from the hash structure.

Author:

    David Goebel (davidg@balder.com) 11-Mar-1999

--*/

#ifndef _NWFS_
#define _NWFS_

//
//  Sector size on Netware disks is hard-coded to 512
//

#define SECTOR_SIZE                 (0x200)
#define SECTOR_SHIFT                (9)
#define SECTOR_MASK                 (SECTOR_SIZE - 1)
#define INVERSE_SECTOR_MASK         ~(SECTOR_SIZE - 1)

//
//  Nwfs file id is a large integer.
//

typedef LARGE_INTEGER               FILE_ID;
typedef FILE_ID                     *PFILE_ID;

#define NETWARE_PARTITION_TYPE      0x65

#define NW_MAX_SEGMENTS_PER_VOLUME  0x20

//
//  This is the maximum number of bad partitions that we can remember.
//

#define MAX_BAD_PARTITIONS          0x100

//
//  Define the various dirent attributes
//

#define NETWARE_ATTR_READ_ONLY        0x01
#define NETWARE_ATTR_HIDDEN           0x02
#define NETWARE_ATTR_SYSTEM           0x04
#define NETWARE_ATTR_DIRECTORY        0x10
#define NETWARE_ATTR_ARCHIVE          0x20

#define NETWARE_ATTR_MASK  ( NETWARE_ATTR_READ_ONLY | \
                             NETWARE_ATTR_HIDDEN    | \
                             NETWARE_ATTR_SYSTEM    | \
                             NETWARE_ATTR_DIRECTORY | \
                             NETWARE_ATTR_ARCHIVE )

#endif // _NWFS_

