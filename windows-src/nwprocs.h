/*++

Copyright (c) 1999-2000  Timpanogas Research Group, Inc.

Module Name:

    NwProcs.h

Abstract:

    This module defines all of the globally used procedures in the Netware
    file system.

Author:

    David Goebel (davidg@balder.com) 11-Mar-1999

--*/

#ifndef _NWPROCS_
#define _NWPROCS_

#include <ntifs.h>

#include <ntdddisk.h>

#include "nodetype.h"
#include "Nw.h"
#include "NwIoctl.h"
#include "NwStruc.h"
#include "NwData.h"

//**** x86 compiler bug ****

#if defined(_M_IX86)
#undef Int64ShraMod32
#define Int64ShraMod32(a, b) ((LONGLONG)(a) >> (b))
#endif

#ifndef ULONG_PTR
typedef ULONG ULONG_PTR;
#endif

//
//  Here are the different pool tags.
//

#define TAG_DISK_ARRAY          'aDwN'      //  Stores a copy of all the disk file objects
#define TAG_CCB                 'ccwN'      //  Ccb
#define TAG_ENUM_EXPRESSION     'eewN'      //  Search expression for enumeration
#define TAG_ENUM_CONTEXT        'cewN'      //  Search expression for enumeration context
#define TAG_FCB                 'cfwN'      //  Data Fcb
#define TAG_FCB_NONPAGED        'nfwN'      //  Nonpaged Fcb
#define TAG_FCB_RESOURCE        'eRwN'      //  Fcb Resource
#define TAG_FILE_LOCK           'lfwN'      //  Filelock
#define TAG_FILE_NAME           'nFwN'      //  Filename buffer
#define TAG_IO_CONTEXT          'oiwN'      //  Io context for async reads
#define TAG_IRP_CONTEXT         'ciwN'      //  Irp Context
#define TAG_VPB                 'pvwN'      //  Vpb allocated in filesystem
#define TAG_IO_RUN              'nRwN'      //  IoRuns array for fragmented files
#define TAG_TEMP_BUFFER         'uBwN'      //  Temporary buffer


//
//  File access check routine, implemented in AcChkSup.c
//

//
//  BOOLEAN
//  NwIllegalDesiredAccess (
//      IN PIRP_CONTEXT IrpContext,
//      IN ACCESS_MASK DesiredAccess
//      );
//

#define NwIllegalDesiredAccess(IC,DA) ( \
    FlagOn( (DA),                       \
            FILE_WRITE_DATA         |   \
            FILE_ADD_FILE           |   \
            FILE_APPEND_DATA        |   \
            FILE_ADD_SUBDIRECTORY   |   \
            FILE_WRITE_EA           |   \
            FILE_DELETE_CHILD       |   \
            FILE_WRITE_ATTRIBUTES   |   \
            DELETE                  |   \
            WRITE_DAC )                 \
)


//
//   Buffer control routines for data caching, implemented in CacheSup.c
//

NTSTATUS
NwCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwPurgeVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

//
//  Device I/O routines, implemented in DevIoSup.c
//
//  These routines perform the actual device reads.  They only affect the on
//  disk structure and do not alter any other data structures.
//

NTSTATUS
NwNonCachedNonAlignedRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN ULONG StartingOffset,
    IN ULONG ByteCount
    );

NTSTATUS
NwNonCachedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN VBN StartingVbn,
    IN ULONG ByteCount
    );

NTSTATUS
NwCreateUserMdl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BufferLength,
    IN BOOLEAN RaiseOnError
    );

NTSTATUS
NwToggleMediaEjectDisable (
    IN PDEVICE_OBJECT Partition,
    IN BOOLEAN PreventRemoval
    );

NTSTATUS
NwResyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
//  PVOID
//  NwMapUserBuffer (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  NwLockUserBuffer (
//      IN PIRP_CONTEXT IrpContext,
//      IN ULONG BufferLength
//      );
//

#define NwMapUserBuffer(IC) (                                       \
    ((PVOID) (((IC)->Irp->MdlAddress == NULL) ?                     \
              (IC)->Irp->UserBuffer :                               \
              MmGetSystemAddressForMdl( (IC)->Irp->MdlAddress )))   \
)

#define NwLockUserBuffer(IC,BL) {                   \
    if ((IC)->Irp->MdlAddress == NULL) {            \
        (VOID) NwCreateUserMdl( (IC), (BL), TRUE ); \
    }                                               \
}


//
//  The following routines are used to manipulate the fscontext fields
//  of the file object, implemented in FilObSup.c
//

//
//  Type of opens.  FilObSup.c depends on this order.
//

typedef enum _TYPE_OF_OPEN {

    UnopenedFileObject = 0,
    UserVolumeOpen,
    UserDirectoryOpen,
    UserFileOpen,
    BeyondValidType

} TYPE_OF_OPEN;
typedef TYPE_OF_OPEN *PTYPE_OF_OPEN;

VOID
NwSetFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PFCB Fcb OPTIONAL,
    IN PCCB Ccb OPTIONAL
    );

TYPE_OF_OPEN
NwDecodeFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb,
    OUT PCCB *Ccb
    );

TYPE_OF_OPEN
NwFastDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb
    );


//
//  Name support routines, implemented in NameSup.c
//

VOID
NwDissectName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT POEM_STRING RemainingName,
    OUT POEM_STRING FinalName
    );

BOOLEAN
NwIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING CurrentName,
    IN PUNICODE_STRING SearchExpression,
    IN ULONG  WildcardFlags
    );


//
//  Netware specific support routines, implemented in NwSup.c
//

NTSTATUS
NwProcessVolumeLayout (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
NwSetEnumContext(
    IN PIRP_CONTEXT IrpContext,
    IN struct _HASH *Hash,
    IN PENUM_CONTEXT Entry,
    IN ULONG SlotIndex
    );

//
//  Synchronization routines.  Implemented in Resrcsup.c
//
//  The following routines/macros are used to synchronize the in-memory structures.
//
//      Routine/Macro               Synchronizes                            Subsequent
//
//      NwAcquireNwData             Volume Mounts/Dismounts,Vcb Queue       NwReleaseNwData
//      NwAcquireVcbExclusive       Vcb for open/close                      NwReleaseVcb
//      NwAcquireVcbShared          Vcb for open/close                      NwReleaseVcb
//      NwAcquireFcbExclusive       Fcb for open/close                      NwReleaseFcb
//      NwAcquireFcbShared          Fcb for open/close                      NwReleaseFcb
//      NwLock                      Fields in NwData, Vcb, Fcb              NwUnlock
//

typedef enum _TYPE_OF_ACQUIRE {

    AcquireExclusive,
    AcquireShared,
    AcquireSharedStarveExclusive

} TYPE_OF_ACQUIRE, *PTYPE_OF_ACQUIRE;

BOOLEAN
NwAcquireResource (
    IN PIRP_CONTEXT IrpContext,
    IN PERESOURCE Resource,
    IN BOOLEAN IgnoreWait,
    IN TYPE_OF_ACQUIRE Type
    );

//
//  BOOLEAN
//  NwAcquireNwData (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  NwReleaseNwData (
//      IN PIRP_CONTEXT IrpContext
//    );
//
//  BOOLEAN
//  NwAcquireVcbExclusive (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  NwAcquireVcbShared (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  VOID
//  NwReleaseVcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  BOOLEAN
//  NwAcquireFcbExclusive (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  NwAcquireFcbShared (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  NwReleaseFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  NwLock (
//      );
//
//  VOID
//  NwUnlock (
//      );
//

#define NwAcquireNwData(IC)                                         \
    ExAcquireResourceExclusive( &NwData.DataResource, TRUE )

#define NwReleaseNwData(IC)                                         \
    ExReleaseResource( &NwData.DataResource )

#define NwAcquireVcbExclusive(IC,V,I)                               \
    NwAcquireResource( (IC), &(V)->VcbResource, (I), AcquireExclusive )

#define NwAcquireVcbShared(IC,V,I)                                  \
    NwAcquireResource( (IC), &(V)->VcbResource, (I), AcquireShared )

#define NwReleaseVcb(IC,V)                                          \
    ExReleaseResource( &(V)->VcbResource )

#define NwAcquireFcbExclusive(IC,F,I)                               \
    NwAcquireResource( (IC), (F)->Resource, (I), AcquireExclusive )

#define NwAcquireFcbShared(IC,F,I)                                  \
    NwAcquireResource( (IC), (F)->Resource, (I), AcquireShared )

#define NwReleaseFcb(IC,F)                                          \
    ExReleaseResource( (F)->Resource )

#define NwLock()                                                    \
    ExAcquireFastMutex( &NwData.NwDataMutex );                      \
    NwData.NwDataLockThread = PsGetCurrentThread()

#define NwUnlock()                                                  \
    NwData.NwDataLockThread = NULL;                                 \
    ExReleaseFastMutex( &NwData.NwDataMutex )

BOOLEAN
NwNoopAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    );

VOID
NwNoopRelease (
    IN PVOID Fcb
    );

BOOLEAN
NwAcquireForCache (
    IN PFCB Fcb,
    IN BOOLEAN Wait
    );

VOID
NwReleaseFromCache (
    IN PFCB Fcb
    );

VOID
NwAcquireForCreateSection (
    IN PFILE_OBJECT FileObject
    );

VOID
NwReleaseForCreateSection (
    IN PFILE_OBJECT FileObject
    );


//
//  In-memory structure support routines.  Implemented in StrucSup.c
//

VOID
NwInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN ULONG VolumeNumber,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PDEVICE_OBJECT PhysicalPartition,
    IN PVPB Vpb,
    IN ULONG TotalClusters,
    IN ULONG FreeClusters,
    IN ULONG SectorsPerCluster
    );

VOID
NwDeleteVcb (
    IN OUT PVCB Vcb
    );

PFCB
NwCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN FILE_ID FileId
    );

VOID
NwDeleteFcb (
    IN PFCB Fcb
    );

PCCB
NwCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Flags
    );

VOID
NwDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    );

BOOLEAN
NwCreateFileLock (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb,
    IN BOOLEAN RaiseOnError
    );

VOID
NwDeleteFileLock (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_LOCK FileLock
    );

PIRP_CONTEXT
NwCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    );

VOID
NwCleanupIrpContext (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN Post
    );

//
//  VOID
//  NwIncrementHandleCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  NwDecrementHandleCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  NwIncrementObjectCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  NwDecrementObjectCounts (
//      IN PFCB Fcb
//      );
//
//  VOID
//  NwIncrementFcbCount (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  NwDecrementFcbCount (
//      IN PVCB Vcb
//      );
//

#define NwIncrementHandleCounts(IC,F) { \
    ASSERT_LOCKED_NW();                 \
    (F)->FileHandleCount += 1;          \
    (F)->Vcb->FileHandleCount += 1;     \
}

#define NwDecrementHandleCounts(IC,F) { \
    ASSERT_LOCKED_NW();                 \
    (F)->FileHandleCount -= 1;          \
    (F)->Vcb->FileHandleCount -= 1;     \
}

#define NwIncrementObjectCounts(IC,F) { \
    ASSERT_LOCKED_NW();                 \
    (F)->FileObjectCount += 1;          \
    (F)->Vcb->FileObjectCount += 1;     \
}

#define NwDecrementObjectCounts(F) {    \
    ASSERT_LOCKED_NW();                 \
    (F)->FileObjectCount -= 1;          \
    (F)->Vcb->FileObjectCount -= 1;     \
}

#define NwIncrementFcbCount(IC,V) {     \
    ASSERT_LOCKED_NW();                 \
    (V)->FcbCount += 1;                 \
}

#define NwDecrementFcbCount(V) {        \
    ASSERT_LOCKED_NW();                 \
    (V)->FcbCount -= 1;                 \
}

VOID
NwUnlinkFcb (
    IN PFCB Fcb
    );

VOID
NwDetachDevice (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
    );

VOID
NwUnload (
    IN PDRIVER_OBJECT DriverObject
    );

VOID
NwMarkPartitionBad (
    IN PDEVICE_OBJECT BadPartition
    );

BOOLEAN
NwIsPartitionBad (
    IN PFILE_OBJECT BadPartition
    );

//
//  For debugging purposes we sometimes want to allocate our structures from nonpaged
//  pool so that in the kernel debugger we can walk all the structures.
//

#define NwPagedPool                 PagedPool
#define NwNonPagedPool              NonPagedPool
#define NwNonPagedPoolCacheAligned  NonPagedPoolCacheAligned


//
//  Work queue routines for posting and retrieving an Irp, implemented in
//  workque.c
//

NTSTATUS
NwFsdPostRequest(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
NwPrePostIrp (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
NwOplockComplete (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

//
//  Miscellaneous support routines
//

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#define BooleanFlagOn(F,SF) (    \
    (BOOLEAN)(((F) & (SF)) != 0) \
)

#define SetFlag(Flags,SingleFlag) { \
    (Flags) |= (SingleFlag);        \
}

#define ClearFlag(Flags,SingleFlag) { \
    (Flags) &= ~(SingleFlag);         \
}

//
//      CAST
//      Add2Ptr (
//          IN PVOID Pointer,
//          IN ULONG Increment
//          IN (CAST)
//          );
//
//      ULONG
//      PtrOffset (
//          IN PVOID BasePtr,
//          IN PVOID OffsetPtr
//          );
//

#define Add2Ptr(PTR,INC,CAST) ((CAST)((PUCHAR)(PTR) + (INC)))

#define PtrOffset(BASE,OFFSET) ((ULONG)((ULONG_PTR)(OFFSET) - (ULONG_PTR)(BASE)))

//
//  This macro takes a pointer (or ulong) and returns its rounded up word
//  value
//

#define WordAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 1) & 0xfffffffe) \
    )

//
//  This macro takes a pointer (or ulong) and returns its rounded up longword
//  value
//

#define LongAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 3) & 0xfffffffc) \
    )

//
//  This macro takes a pointer (or ulong) and returns its rounded up quadword
//  value
//

#define QuadAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )

//
//  The following macros round up and down to sector boundaries.
//

#define SectorAlign(L) (                                                \
    ((((ULONG)(L)) + (SECTOR_SIZE - 1)) & ~(SECTOR_SIZE - 1))           \
)

#define LlSectorAlign(L) (                                              \
    ((((LONGLONG)(L)) + (SECTOR_SIZE - 1)) & ~(SECTOR_SIZE - 1))        \
)

#define SectorTruncate(L) (                                             \
    ((ULONG)(L)) & ~(SECTOR_SIZE - 1)                                   \
)

#define LlSectorTruncate(L) (                                           \
    ((LONGLONG)(L)) & ~(SECTOR_SIZE - 1)                                \
)

#define BytesFromSectors(L) (                                           \
    ((ULONG) (L)) << SECTOR_SHIFT                                       \
)

#define SectorsFromBytes(L) (                                           \
    ((ULONG) (L)) >> SECTOR_SHIFT                                       \
)

#define LlBytesFromSectors(L) (                                         \
    Int64ShllMod32( (LONGLONG)(L), SECTOR_SHIFT )                       \
)

#define LlSectorsFromBytes(L) (                                         \
    Int64ShraMod32( (LONGLONG)(L), SECTOR_SHIFT )                       \
)

#define SectorOffset(L) (                                               \
    ((ULONG) (L)) & SECTOR_MASK                                         \
)

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
//

typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

typedef union _USHORT2 {
    USHORT Ushort[2];
    ULONG  ForceAlignment;
} USHORT2, *PUSHORT2;

//
//  This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) {                           \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src));  \
    }

//
//  This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) {                           \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src));  \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) {                           \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src));  \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//  accessing the source on a word boundary.
//

#define CopyUshort2(Dst,Src) {                          \
    *((USHORT2 *)(Dst)) = *((UNALIGNED USHORT2 *)(Src));\
    }


//
//  Following routines handle entry in and out of the filesystem.  They are
//  contained in NwData.c
//

NTSTATUS
NwFsdDispatch (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

LONG
NwExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
NwProcessException (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    );

VOID
NwCompleteRequest (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    );

//
//  VOID
//  NwRaiseStatus (
//      IN PRIP_CONTEXT IrpContext,
//      IN NT_STATUS Status
//      );
//
//  VOID
//  NwNormalAndRaiseStatus (
//      IN PRIP_CONTEXT IrpContext,
//      IN NT_STATUS Status
//      );
//

#define NwRaiseStatus(IC,S) {                               \
    (IC)->ExceptionStatus = (S);                            \
    ExRaiseStatus( (S) );                                   \
}

#define NwNormalizeAndRaiseStatus(IC,S) {                                           \
    (IC)->ExceptionStatus = FsRtlNormalizeNtstatus((S),STATUS_UNEXPECTED_IO_ERROR); \
    ExRaiseStatus( (IC)->ExceptionStatus );                                         \
}

VOID
NwPopUpFileCorrupt (
    IN PIRP_CONTEXT IrpContext
    );

//
//  Following are the fast entry points.
//

BOOLEAN
NwFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NwFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NwFastLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NwFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NwFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NwFastUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NwFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NwFastQueryNetworkInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

//
//  Following are the routines to handle the top level thread logic.
//

VOID
NwSetThreadContext (
    IN PIRP_CONTEXT IrpContext,
    IN PTHREAD_CONTEXT ThreadContext
    );

//
//  VOID
//  NwRestoreThreadContext (
//      IN PIRP_CONTEXT IrpContext
//      );
//

#define NwRestoreThreadContext(IC)                              \
    (IC)->ThreadContext->Nwfs = 0;                              \
    IoSetTopLevelIrp( (IC)->ThreadContext->SavedTopLevelIrp );  \
    (IC)->ThreadContext = NULL

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//

#define CanFsdWait(I)   IoIsOperationSynchronous(I)

//
//  The following macro is used to set the fast i/o possible bits in the
//  FsRtl header.
//
//      FastIoIsNotPossible - If the Fcb is bad or there are oplocks on the file.
//
//      FastIoIsQuestionable - If there are file locks.
//
//      FastIoIsPossible - In all other cases.
//
//

#define NwIsFastIoPossible(F) ((BOOLEAN)                                            \
    ((((F)->Vcb->VcbCondition != VcbMounted ) ||                                    \
      !FsRtlOplockIsFastIoPossible( &(F)->Oplock )) ?                               \
                                                                                    \
     FastIoIsNotPossible :                                                          \
                                                                                    \
     ((((F)->FileLock != NULL) && FsRtlAreThereCurrentFileLocks( (F)->FileLock )) ? \
                                                                                    \
        FastIoIsQuestionable :                                                      \
                                                                                    \
        FastIoIsPossible))                                                          \
)


//
//  The FSP level dispatch/main routine.  This is the routine that takes
//  IRP's off of the work queue and calls the appropriate FSP level
//  work routine.
//

VOID
NwFspDispatch (                             //  implemented in FspDisp.c
    IN PIRP_CONTEXT IrpContext
    );

VOID
NwFspClose (                                //  implemented in Close.c
    IN PVCB Vcb OPTIONAL
    );

//
//  The following routines are the entry points for the different operations
//  based on the IrpSp major functions.
//

NTSTATUS
NwCommonCreate (                            //  Implemented in Create.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwCommonClose (                             //  Implemented in Close.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwCommonRead (                              //  Implemented in Read.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwCommonQueryInfo (                         //  Implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwCommonSetInfo (                           //  Implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwCommonQueryVolInfo (                      //  Implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwCommonDirControl (                        //  Implemented in DirCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwCommonFsControl (                         //  Implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwCommonDevControl (                        //  Implemented in DevCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwCommonLockControl (                       //  Implemented in LockCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NwCommonCleanup (                           //  Implemented in Cleanup.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//      #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//      #define try_return(S)  { S; goto try_exit; }
//

#define try_return(S) { S; goto try_exit; }

#endif // _NWPROCS_
