/*++

Copyright (c) 1999-2022 Jeffrey V. Merkey

Module Name:

    NwStruc.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the Nwfs file system.

    In-Memory structures:

        The global data structures with the NwDataRecord.  It contains a pointer
        to a File System Device object and a queue of Vcb's.  There is a Vcb for
        every currently or previously mounted volumes.  We may be in the process
        of tearing down the Vcb's which have been dismounted.  The Vcb's are
        allocated as an extension to a volume device object.

            +--------+
            | NwData |     +--------+
            |        | --> |FilSysDo|
            |        |     |        |
            |        | <+  +--------+
            +--------+  |
                        |
                        |  +--------+     +--------+
                        |  |VolDo   |     |VolDo   |
                        |  |        |     |        |
                        |  +--------+     +--------+
                        +> |Vcb     | <-> |Vcb     | <-> ...
                           |        |     |        |
                           +--------+     +--------+


        Each Vcb contains pointers to the special Fcbs for the DASD file and
        the root directory and a list of all the Fcbs (files and directories)
        for the volume.


                            +--------+
            +---------+     |  Root  |
            |   Vcb   |---->|  Dir   |
            |         |     |  Fcb   |
            |         |     +--------+        +--------+
            |         |                       |  DASD  |
            |         |---------------------->|  File  |
            |         |                       |  Fcb   |
         +--| FcbList |--+                    +--------+
         |  +---------+  |
         |               |
         |               |  +--------+    +--------+    +--------+
         |               |  |  Fcb   |    |  Fcb   |    |  Fcb   |
         |               +->|        |<-->|        |<-->|        |--+
         |                  |        |    |        |    |        |  |
         |                  +--------+    +--------+    +--------+  |
         |                                                          |
         +----------------------------------------------------------+


        Each file object opened on a Neware volume contains two context
        pointers.  The first will point back to the Fcb for the file object.
        The second points to a Ccb (ContextControlBlock) which contains the
        per-handle information. This includes the state of any directory
        enumeration.  In addition Ccbs are linked through the Fcb


               +----------------------------------------+
               |                                        |
          +--------+      +--------+       +--------+   |
          |  Fcb   |      |  Ccb   |       |  Ccb   |   |
          |        |<---->|        |<----->|        |<->+
          |        |      |        |       |        |
          +--------+      +--------+       +--------+
               ^              ^                ^
               |              |                |
               |              | <-FsContext2-> |
               |              |                |
               |         +--------+            |
               |FsContext|  File  |            |
               +---------| Object |            |
               |         |        |            |
               |         +--------+            |
               |                          +--------+
               |                 FsContext|  File  |
               +--------------------------| Object |
                                          |        |
                                          +--------+



    Synchronization:

    1. The ERESOURCE in the NwData structure is used exclusively to synchronize
       mount operations via IOCTL_QUERY_ALL_DISKS.
       
    2. The ERESOURCE in the Vcb is acquired exclusively to syncrhonize DASD
       opens, lock, unlock, and dismount requests.
       
    3. The ERESOURCE in the Vcb is acquired shared for all other opens, change
       notify directory calls, and querying volume info.
       
    4. The ERESOURCE in the Fcb is acquired exclusively to syncrhonize access
       to the share access, file locks, and oplocks.  The DasdFcb is also used
       for Vcb dismount.  Directory Dcbs acquire it when initialzing
       enumeration contyexts.  As a result, it is acquired in create, cleanup,
       close, dirctrl, and file lock functions.
       
    5. The ERESOURCE in the Fcb is acquired shared on read and querying file
       information.  For Dcbs, it is also used when enumerating a directorey.
       
    6. The FAST_MUTEX in the NwData structure is used globally as an end
       resource to serialize access to all linked lists in all structures, and
       all counts in all structures.  It may also be used in other cases where
       simple mutual exclusion is required for a short period of time.  Since
       it is an end resource, no other forms of synchronization may be
       acquired before it is released.
       
    7. Locking Order.  For ERESOURCES it is NwData, Vcb, then Fcb.  As 
       previously stated the FAST_MUTEX in NwData is an end resource and may
       only be used as such.

Author:

    David Goebel (davidg@balder.com) 11-Mar-1999

--*/

#ifndef _NWSTRUC_
#define _NWSTRUC_

//
//  Define data types for "Logical Byte Offset" and "Virtual Byte Offset",
//  respectivly.
//

typedef LARGE_INTEGER LBO;
typedef LARGE_INTEGER VBO;

typedef ULONG LBN;
typedef ULONG VBN;



//
//  The NW_DATA record is the top record in the Netware file system in-memory
//  data structure.  This structure must be allocated from non-paged pool.
//

typedef struct _NW_DATA {

    //
    //  The type and size of this record (must be NWFS_NTC_DATA_HEADER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A pointer to the Driver object we were initialized with
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  Vcb queue.
    //

    LIST_ENTRY VcbQueue;

    //
    //  Nt version information.  We only mount on NT 4 & 5
    //

    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;
    BOOLEAN Checked;
    UNICODE_STRING CSDVersion;

    //
    //  The following fields are used to allocate IRP context structures
    //  using a lookaside list, and other fixed sized structures from a
    //  small cache.  We use the NwData mutex to protext these structures.
    //

    ULONG IrpContextDepth;
    ULONG IrpContextMaxDepth;
    SINGLE_LIST_ENTRY IrpContextList;

    //
    //  Filesystem device object for NWFS.
    //

    PDEVICE_OBJECT FileSystemDeviceObject;

    //
    //  The following are used to manage the delayed close queue.
    //
    //  FspCloseActive - Indicates whether there is a thread processing the
    //      close queue.
    //  ReduceDelayedClose - Indicates that we have hit the upper threshold
    //      for the delayed close queue and need to reduce it to lower threshold.
    //
    //  DelayedCloseQueue - Queue of IrpContextLite waiting for delayed close
    //      operation.
    //  DelayedCloseCount - Number of entries on the delayted close queue.
    //  MaxDelayedCloseCount - Trigger delay close work at this threshold.
    //  MinDelayedCloseCount - Turn off delay close work at this threshold.
    //

    BOOLEAN FspCloseActive;
    BOOLEAN ReduceDelayedClose;

    LIST_ENTRY DelayedCloseQueue;
    ULONG DelayedCloseCount;
    ULONG MaxDelayedCloseCount;
    ULONG MinDelayedCloseCount;

    //
    //  Fast mutex used to lock the fields of this structure and others.
    //

    PVOID NwDataLockThread;
    FAST_MUTEX NwDataMutex;

    //
    //  A resource variable to control access to the global NWFS data record
    //

    ERESOURCE DataResource;

    //
    //  Cache manager call back structure, which must be passed on each call
    //  to CcInitializeCacheMap.
    //

    CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;

    //
    //  This is the ExWorkerItem that does both kinds of deferred closes.
    //

    WORK_QUEUE_ITEM CloseItem;

    //
    //  This is the most stack locations we saw beneath us.
    //

    CCHAR MaxDeviceStackSize;

    //
    //  Stash away a copy of all the file objects we have referenced.
    //

    PDISK_ARRAY SavedDiskArray;

    //
    //  The following fields manage a table of known bad partitions.
    //

    KSPIN_LOCK BadPartitionSpinLock;

    PDEVICE_OBJECT BadPartitions[MAX_BAD_PARTITIONS];
    
    //
    //  This is a table for quick upcasing of Oem strings
    //

    UCHAR OemUpcaseTable[256];

} NW_DATA;
typedef NW_DATA *PNW_DATA;


//
//  The Vcb (Volume control block) record corresponds to every
//  volume mounted by the file system.  They are ordered in a queue off
//  of NwData.VcbQueue.
//
//  The Vcb will be in several conditions during its lifespan.
//
//      NotMounted - Disk is not currently mounted (i.e. removed
//          from system) but cleanup and close operations are
//          supported.
//
//      MountInProgress - State of the Vcb from the time it is
//          created until it is successfully mounted or the mount
//          fails.
//
//      Mounted - Volume is currently in the mounted state.
//
//      Invalid - User has invalidated the volume.  Only legal operations
//          are cleanup and close.
//
//      DismountInProgress - We have begun the process of tearing down the
//          Vcb.  It can be deleted when all the references to it
//          have gone away.
//

typedef enum _VCB_CONDITION {

    VcbNotMounted = 0,
    VcbMountInProgress,
    VcbMounted,
    VcbInvalid,
    VcbDismountInProgress

} VCB_CONDITION;

typedef struct _VCB {

    //
    //  The type and size of this record (must be NWFS_NTC_VCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Vpb for this volume.
    //

    PVPB Vpb;

    //
    //  Pseudo device object for the driver below us (that we created).
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    //  This is the partition that sector 0 of the volume resolves to.  It is
    //  used when we must deal with a physical partition.
    //

    PDEVICE_OBJECT PhysicalPartition;

    //
    //  File object used to lock the volume.
    //

    PFILE_OBJECT VolumeLockFileObject;

    //
    //  We always have FCBs for the DASD file and root directory.
    //

    struct _FCB *VolumeDasdFcb;

    struct _FCB *RootDirFcb;

    //
    //  Link into queue of Vcb's in the NwData structure.  We will create a union with
    //  a LONGLONG to force the Vcb to be quad-aligned.
    //

    union {

        LIST_ENTRY VcbLinks;
        LONGLONG Alignment;
    };

    //
    //  State flags and condition for the Vcb.
    //

    ULONG VcbState;
    VCB_CONDITION VcbCondition;

    //
    //  Various counts for this Vcb.
    //
    //      FcbCount - The number of Fcbs still linked in the FcbList.
    //      FileHandleCount - The number of user handles open on the volume,
    //          i.e. number of cleanups we have yet to receive.
    //      FileObjectCount - The number of FileObjects referencing this
    //          volume, i.e. the number of closes we have yet to receive.
    //

    ULONG FcbCount;
    ULONG FileHandleCount;
    ULONG FileObjectCount;

    //
    //  Vcb resource.  This is used to synchronize open/cleanup/close operations.
    //

    ERESOURCE VcbResource;

    //
    //  The following is used to synchronize the dir notify package.
    //

    PNOTIFY_SYNC NotifySync;

    //
    //  The following is the head of a list of notify Irps.
    //

    LIST_ENTRY DirNotifyList;

    //
    //  List of all open Fcbs.  Synchronized with the Vcb fast mutex.
    //

    LIST_ENTRY FcbList;

    //
    //  This number is used to reference this volume with the Netware
    //  specific code.
    //

    ULONG VolumeNumber;

    //
    //  These number are geometry supplied from the Netware specific code.
    //

    ULONG SectorsPerCluster;
    ULONG TotalClusters;
    ULONG FreeClusters;

} VCB;
typedef VCB *PVCB;

#define VCB_STATE_LOCKED                            (0x00000001)
#define VCB_STATE_REMOVABLE_MEDIA                   (0x00000002)


//
//  The Volume Device Object is an I/O system device object with a
//  workqueue and an VCB record appended to the end.  There are multiple
//  of these records, one for every mounted volume, and are created during
//  a volume mount operation.  The work queue is for handling an overload
//  of work requests to the volume.
//

typedef struct _VOLUME_DEVICE_OBJECT {

    DEVICE_OBJECT DeviceObject;
    DEVOBJ_EXTENSION DeviceExtention;

    //
    //  The following field tells how many requests for this volume have
    //  either been enqueued to ExWorker threads or are currently being
    //  serviced by ExWorker threads.  If the number goes above
    //  a certain threshold, put the request on the overflow queue to be
    //  executed later.
    //

    ULONG PostedRequestCount;

    //
    //  The following field indicates the number of IRP's waiting
    //  to be serviced in the overflow queue.
    //

    ULONG OverflowQueueCount;

    //
    //  The following field contains the queue header of the overflow queue.
    //  The Overflow queue is a list of IRP's linked via the IRP's ListEntry
    //  field.
    //

    LIST_ENTRY OverflowQueue;

    //
    //  This is the file system specific volume control block.
    //

    VCB Vcb;

} VOLUME_DEVICE_OBJECT;
typedef VOLUME_DEVICE_OBJECT *PVOLUME_DEVICE_OBJECT;


typedef enum _FCB_CONDITION {
    FcbGood = 1,
    FcbBad,
    FcbNeedsToBeVerified
} FCB_CONDITION;

typedef struct _FCB_NONPAGED {

    //
    //  Type and size of this record must be NWFS_NTC_FCB_NONPAGED
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The following field contains a record of special pointers used by
    //  MM and Cache to manipluate section objects.  Note that the values
    //  are set outside of the file system.  However the file system on an
    //  open/create will set the file object's SectionObject field to
    //  point to this field
    //

    SECTION_OBJECT_POINTERS SegmentObject;

    //
    //  This is the resource for this Fcb.
    //

    ERESOURCE Resource;

} FCB_NONPAGED;
typedef FCB_NONPAGED *PFCB_NONPAGED;

//
//  The Fcb/Dcb record corresponds to every open file and directory, and to
//  every directory on an opened path.
//

typedef struct _FCB {

    //
    //  Common Fsrtl Header.  The named header is for the fieldoff.c output.  We
    //  use the unnamed header internally.
    //

    union{

        FSRTL_COMMON_FCB_HEADER Header;
        FSRTL_COMMON_FCB_HEADER;
    };

    //
    //  State flags for this Fcb.
    //

    ULONG FcbState;

    //
    //  NT style attributes for the Fcb.
    //

    ULONG FileAttributes;

    //
    //  All the Ccbs for this Fcb are linked here.
    //

    LIST_ENTRY CcbList;

    //
    //  Pointer to the Fcb non-paged structures.
    //

    PFCB_NONPAGED FcbNonpaged;

    //
    //  Vcb for this Fcb.
    //

    PVCB Vcb;

    //
    //  Links to the queue of Fcb's in the volume.
    //

    LIST_ENTRY FcbLinks;


    //
    //  List entry to attach to delayed close queue.
    //

    LIST_ENTRY DelayedCloseLinks;

    //
    //  FileId for this file.
    //

    union {

        FILE_ID FileId;
        ULONG DirNumber;
    };

    //
    //  Pointer to the DOS hash record for this file.
    //

    struct _HASH *Hash;

    //
    //  Pointer to our parent's DOS hash record.
    //

    struct _HASH *ParentHash;

    //
    //  Two if the file has two names.
    //

    ULONG NumberOfLinks;

    //
    //  This is the short name from the DOS name space if it exists.
    //

    UNICODE_STRING ShortName;
    WCHAR ShortNameBuffer[12];

    //
    //  Various counts for this Fcb.
    //
    //      FileHandleCount - The number of user handles open on this file,
    //          i.e., number of cleanups we have yet to receive.
    //      FileObjectCount - The number of FileObjects referencing this
    //          Fcb, i.e. the number of closes we have yet to receive.
    //

    ULONG FileHandleCount;
    ULONG FileObjectCount;

    //
    //  Share access structure.
    //

    SHARE_ACCESS ShareAccess;

    //
    //  Time stamps for this file.
    //

    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;

    //
    //  The following field is used by the oplock module
    //  to maintain current oplock information.
    //

    OPLOCK Oplock;

    //
    //  The following field is used by the filelock module
    //  to maintain current byte range locking information.
    //  A file lock is allocated as needed.
    //

    PFILE_LOCK FileLock;

    //
    //  Mcb for the on disk mapping, and its synchronization.
    //

    MCB Mcb;

    FAST_MUTEX McbMutex;

    //
    //  The number of sectors containing valid data.  Reads between this number
    //  and FileSize return zero.
    //

    ULONG ValidDataSectors;

} FCB;
typedef FCB *PFCB;

#define FCB_STATE_INITIALIZED                   (0x00000001)
#define FCB_STATE_DELAY_CLOSE                   (0x00000002)
#define FCB_STATE_CLOSE_DELAYED                 (0x00000004)
#define FCB_STATE_MCB_LOADED                    (0x00000008)

#define MAX_FCB_ASYNC_ACQUIRE                   (0xf000)


//
//  The Ccb record is allocated for every file object
//

typedef struct _CCB {

    //
    //  Type and size of this record (must be NWFS_NTC_CCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Flags.  Indicates flags to apply for the current open.
    //

    ULONG Flags;

    //
    //  Links to the queue of Ccbs in the Fcb.
    //

    LIST_ENTRY CcbLinks;

    //
    //  Fcb for the file being opened.
    //

    PFCB Fcb;

    //
    //  The "owner" of this Ccb
    //

    PFILE_OBJECT FileObject;

    //
    //  This is the full name that opened this file.
    //

    UNICODE_STRING OriginalName;

    //
    //  This is the thread that opened this file.
    //

    PETHREAD OpeningThread;

    //
    //  We store state information in the Ccb for a directory
    //  enumeration on this handle.
    //
    //  Offset in the directory stream to base the next enumeration.
    //

    ULONG SlotIndex;

    //
    //  This is context used by the Trg components.  It contains NULL on
    //  first invocation.
    //

    PVOID Context;

    //
    //  Finally, the search expression of the current enumeration.
    //

    OEM_STRING OemSearchExpression;
    UNICODE_STRING UnicodeSearchExpression;

} CCB;
typedef CCB *PCCB;

#define CCB_FLAG_OPEN_BY_ID                     (0x00000001)
#define CCB_FLAG_OPEN_RELATIVE_BY_ID            (0x00000002)
#define CCB_FLAG_IGNORE_CASE                    (0x00000004)

//
//  Following flags refer to index enumeration.
//

#define CCB_FLAG_ENUM_CONSTANT                  (0x00010000)
#define CCB_FLAG_ENUM_MATCH_ALL                 (0x00020000)
#define CCB_FLAG_ENUM_INITIALIZED               (0x00040000)
#define CCB_FLAG_ENUM_RE_READ_ENTRY             (0x00080000)

//
//  This structure holds info extracted from the hash entry.
//

typedef struct _ENUM_CONTEXT {

    //
    //  This is the file name to use, along with an upcased version.
    //

    UNICODE_STRING FileName;
    UNICODE_STRING UpcasedFileName;
    
    //
    //  If a name other than the DOS name is available (and thus returned
    //  above), then the DOS name is returned as an Alternate Name.  This
    //  will the common case.
    //

    UNICODE_STRING AlternateFileName;
    WCHAR  AlternateFileNameBuffer[12];

    //
    //  If the FileName is known to be upcased (i.e. it is the DOS name), then
    //  UpcasedFileName is not used.
    //

    BOOLEAN FileNameUpcase;

    //
    //  Just the dirent attributes and the file size.
    //

    ULONG FileAttributes;
    ULONG FileSize;

    //
    //  The various times for the file.
    //

    TIME_FIELDS CreationTime;
    TIME_FIELDS LastWriteTime;
    TIME_FIELDS LastAccessTime;

} ENUM_CONTEXT, *PENUM_CONTEXT;


//
//  The Irp Context record is allocated for every orginating Irp.  It is
//  created by the Fsd dispatch routines, and deallocated by the NwComplete
//  request routine.
//

typedef struct _IRP_CONTEXT {

    //
    //  Type and size of this record (must be NWFS_NTC_IRP_CONTEXT)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Originating Irp for the request.
    //

    PIRP Irp;

    //
    //  Vcb for this operation.  When this is NULL it means we were called
    //  with our filesystem device object instead of a volume device object.
    //  (Mount will fill this in once the Vcb is created)
    //

    PVCB Vcb;

    //
    //  Exception encountered during the request.  Any error raised explicitly by
    //  the file system will be stored here.  Any other error raised by the system
    //  is stored here after normalizing it.
    //

    NTSTATUS ExceptionStatus;

    //
    //  Flags for this request.
    //

    ULONG Flags;

    //
    //  Real device object.  This is out pseudo device object.
    //

    PDEVICE_OBJECT RealDevice;

    //
    //  Top level irp context for this thread.
    //

    struct _IRP_CONTEXT *TopLevel;

    //
    //  Major and minor function codes.
    //

    UCHAR MajorFunction;
    UCHAR MinorFunction;

    //
    //  This is the context used for managing a read request.
    //
    
    struct _NWFS_IO_CONTEXT *NwIoContext;

    //
    //  Pointer to the top-level context if this IrpContext is responsible
    //  for cleaning it up.
    //

    struct _THREAD_CONTEXT *ThreadContext;

    //
    //  This structure is used for posting to the Ex worker threads.
    //

    WORK_QUEUE_ITEM WorkQueueItem;

} IRP_CONTEXT;
typedef IRP_CONTEXT *PIRP_CONTEXT;

#define IRP_CONTEXT_FLAG_ON_STACK               (0x00000001)
#define IRP_CONTEXT_FLAG_MORE_PROCESSING        (0x00000002)
#define IRP_CONTEXT_FLAG_WAIT                   (0x00000004)
#define IRP_CONTEXT_FLAG_FORCE_POST             (0x00000008)
#define IRP_CONTEXT_FLAG_TOP_LEVEL              (0x00000010)
#define IRP_CONTEXT_FLAG_TOP_LEVEL_NWFS         (0x00000020)
#define IRP_CONTEXT_FLAG_IN_FSP                 (0x00000040)
#define IRP_CONTEXT_FLAG_IN_TEARDOWN            (0x00000080)
#define IRP_CONTEXT_FLAG_DISABLE_POPUPS         (0x00000100)

//
//  Flags used for create.
//

#define IRP_CONTEXT_FLAG_TRAIL_BACKSLASH        (0x10000000)

//
//  The following flags need to be cleared when a request is posted.
//

#define IRP_CONTEXT_FLAGS_CLEAR_ON_POST (   \
    IRP_CONTEXT_FLAG_MORE_PROCESSING    |   \
    IRP_CONTEXT_FLAG_WAIT               |   \
    IRP_CONTEXT_FLAG_FORCE_POST         |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL          |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL_NWFS     |   \
    IRP_CONTEXT_FLAG_IN_FSP             |   \
    IRP_CONTEXT_FLAG_IN_TEARDOWN        |   \
    IRP_CONTEXT_FLAG_DISABLE_POPUPS         \
)

//
//  The following flags need to be cleared when a request is retried.
//

#define IRP_CONTEXT_FLAGS_CLEAR_ON_RETRY (  \
    IRP_CONTEXT_FLAG_MORE_PROCESSING    |   \
    IRP_CONTEXT_FLAG_IN_TEARDOWN        |   \
    IRP_CONTEXT_FLAG_DISABLE_POPUPS         \
)

//
//  The following flags are set each time through the Fsp loop.
//

#define IRP_CONTEXT_FSP_FLAGS (             \
    IRP_CONTEXT_FLAG_WAIT               |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL          |   \
    IRP_CONTEXT_FLAG_TOP_LEVEL_NWFS     |   \
    IRP_CONTEXT_FLAG_IN_FSP                 \
)

//
//  Context structure for non-cached I/O calls.  We effectively treat all
//  non-cached I/O as asynchronous since we do not know how many physical
//  extents a particular I/O will span until the last minute.  This avoids
//  extremely complicated race conditions with I/O completion.
//

typedef struct _NWFS_IO_CONTEXT {

    //
    //  This semaphore is released each time a completion routine is invoked.
    //  It is also thereby returns the count of associated Irps that have been
    //  completed and thus allows a single associated Irp to know that it is
    //  the final Irp for the read and perform cleanup.  Note that on MP
    //  machines multiple completion routines can be executing simultaneously,
    //  so this IoContext structure cannot be touched after the sempahore is
    //  released.
    //
    //  The main line path will also wait on the semaphore in the case that it
    //  encounters an error trying to prepare Irps.  Since it knows how many
    //  Irps have been previously queued, it knows how many times it must wait
    //  on the semaphore before it can free the IoContext structure and perform
    //  other cleanup, providing another reason to not touch the IoContext
    //  structure after releasing the semaphore.
    //

    KSEMAPHORE Semaphore;

    //
    //  This is the Irp received in the Fsd path of the file system.  It is not
    //  used to do any I/O.  Associated Irps do all the atcual I/O.
    //

    PIRP MasterIrp;

    //
    //  This is the count of how many associated Irps are required to actually
    //  perform the I/O.  Since we do not know this number until we have built
    //  the final I/O, MAX_ULONG is stored here until this final I/O.  This 
    //  prevents the Master Irp from being completed until the entire read is
    //  complete.
    //

    ULONG IrpCount;

    //
    //  The below two fields allow the final completion routine to release the
    //  ERESOURCE protecting the I/O.
    //

    PERESOURCE Resource;                
    ERESOURCE_THREAD ResourceThreadId;

    //
    //  The following fields are somewhat redundant (they are also in the Irp)
    //  but stashing them here minimizes the amount of processing we have to
    //  perform at DPC level which is a good thing.
    //

    ULONG RequestedByteCount;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER ByteRange;

    //
    //  If this read was rounded up to a sector due to file size truncation
    //  then the following fields what we must zero.
    //

    PUCHAR SectorTailToZero;

    ULONG BytesToZero;

    //
    //  Finally, in the unfortunate case that this I/O fails, we post this
    //  structure to an ExWorker thread so it can be retried.
    //

    WORK_QUEUE_ITEM RetryReadItem;

} NWFS_IO_CONTEXT;

typedef NWFS_IO_CONTEXT *PNWFS_IO_CONTEXT;

//
//  Following structure is used to track the top level request.  Each Nwfs
//  Fsd and Fsp entry point will examine the top level irp location in the
//  thread local storage to determine if this request is top level and/or
//  top level Nwfs.  The top level Nwfs request will remember the previous
//  value and update that location with a stack location.  This location
//  can be accessed by recursive Nwfs entry points.
//

typedef struct _THREAD_CONTEXT {

    //
    //  NWFS signature.  Used to confirm structure on stack is valid.
    //

    ULONG Nwfs;

    //
    //  Previous value in top-level thread location.  We restore this
    //  when done.
    //

    PIRP SavedTopLevelIrp;

    //
    //  Top level NWFS IrpContext.  Initial Nwfs entry point on stack
    //  will store the IrpContext for the request in this stack location.
    //

    PIRP_CONTEXT TopLevelIrpContext;

} THREAD_CONTEXT;
typedef THREAD_CONTEXT *PTHREAD_CONTEXT;

#endif // _NWSTRUC_
