/*++

Copyright (c) 1999-2000  Timpanogas Research Group, Inc.

Module Name:

    NodeType.h

Abstract:

    This module defines all of the node type codes used in this development
    shell.  Every major data structure in the file system is assigned a node
    type code.  This code is the first CSHORT in the structure and is followed
    by a CSHORT containing the size, in bytes, of the structure.

Author:

    David Goebel (davidg@balder.com) 11-Mar-1999

--*/

#ifndef _NWNODETYPE_
#define _NWNODETYPE_

typedef CSHORT NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;

#define NTC_UNDEFINED                   ((NODE_TYPE_CODE)0x0000)

#define NWFS_NTC_DATA_HEADER            ((NODE_TYPE_CODE)0x0e01)
#define NWFS_NTC_VCB                    ((NODE_TYPE_CODE)0x0e02)
#define NWFS_NTC_DCB                    ((NODE_TYPE_CODE)0x0e03)
#define NWFS_NTC_FCB                    ((NODE_TYPE_CODE)0x0e04)
#define NWFS_NTC_FCB_NONPAGED           ((NODE_TYPE_CODE)0x0e05)
#define NWFS_NTC_CCB                    ((NODE_TYPE_CODE)0x0e06)
#define NWFS_NTC_IRP_CONTEXT            ((NODE_TYPE_CODE)0x0e07)

typedef CSHORT NODE_BYTE_SIZE;

//
//  So all records start with
//
//  typedef struct _RECORD_NAME {
//      NODE_TYPE_CODE NodeTypeCode;
//      NODE_BYTE_SIZE NodeByteSize;
//          :
//  } RECORD_NAME;
//  typedef RECORD_NAME *PRECORD_NAME;
//

#define NodeType(P) ((P) != NULL ? (*((PNODE_TYPE_CODE)(P))) : NTC_UNDEFINED)
#define SafeNodeType(Ptr) (*((PNODE_TYPE_CODE)(Ptr)))


//
//  The following definitions are used to generate meaningful blue bugcheck
//  screens.  On a bugcheck the file system can output 4 ulongs of useful
//  information.  The first ulong will have encoded in it a source file id
//  (in the high word) and the line number of the bugcheck (in the low word).
//  The other values can be whatever the caller of the bugcheck routine deems
//  necessary.
//
//  Each individual file that calls bugcheck needs to have defined at the
//  start of the file a constant called BugCheckFileId with one of the
//  NWFS_BUG_CHECK_ values defined below and then use NwBugCheck to bugcheck
//  the system.
//

#define NWFS_BUG_CHECK_CACHESUP          (0x00010000)
#define NWFS_BUG_CHECK_NWDATA            (0x00020000)
#define NWFS_BUG_CHECK_NWINIT            (0x00030000)
#define NWFS_BUG_CHECK_CLEANUP           (0x00040000)
#define NWFS_BUG_CHECK_CLOSE             (0x00050000)
#define NWFS_BUG_CHECK_CREATE            (0x00060000)
#define NWFS_BUG_CHECK_DEVCTRL           (0x00070000)
#define NWFS_BUG_CHECK_DEVIOSUP          (0x00080000)
#define NWFS_BUG_CHECK_DIRCTRL           (0x00090000)
#define NWFS_BUG_CHECK_DIRSUP            (0x000a0000)
#define NWFS_BUG_CHECK_FILEINFO          (0x000b0000)
#define NWFS_BUG_CHECK_FILOBSUP          (0x000c0000)
#define NWFS_BUG_CHECK_FSCTRL            (0x000d0000)
#define NWFS_BUG_CHECK_FSPDISP           (0x000e0000)
#define NWFS_BUG_CHECK_LOCKCTRL          (0x000f0000)
#define NWFS_BUG_CHECK_NAMESUP           (0x00100000)
#define NWFS_BUG_CHECK_READ              (0x00110000)
#define NWFS_BUG_CHECK_RESRCSUP          (0x00120000)
#define NWFS_BUG_CHECK_STRUCSUP          (0x00130000)
#define NWFS_BUG_CHECK_VERFYSUP          (0x00140000)
#define NWFS_BUG_CHECK_VOLINFO           (0x00150000)
#define NWFS_BUG_CHECK_WORKQUE           (0x00160000)
#define NWFS_BUG_CHECK_NWSUP             (0x00170000)

#define NwBugCheck(A,B,C) { KeBugCheckEx(FILE_SYSTEM, BugCheckFileId | __LINE__, A, B, C ); }

#endif // _NWNODETYPE_
