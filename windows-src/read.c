
/***************************************************************************
*
*   Copyright (c) 1999-2000 Timpanogas Research Group, Inc.
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jmerkey@timpanogas.com
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the TRG Windows NT/2000(tm) Public License (WPL) as
*   published by the Timpanogas Research Group, Inc.  (Windows NT/2000
*   is a registered trademark of Microsoft Corporation).
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
*   You should have received a copy of the WPL Public License along
*   with this program; if not, a copy can be obtained from
*   www.timpanogas.com.
*
*   You are free to modify and re-distribute this program in accordance
*   with the terms specified in the TRG Windows NT/2000 Public License (WPL).
*   The copyright contained in this code is required to be present in
*   any derivative works and you are required to provide the source code
*   for this program as part of any commercial or non-commercial
*   distribution. You are required to respect the rights of the
*   Copyright holders named within this code.
*
*   jmerkey@timpanogas.com and TRG, Inc. are the official maintainers of
*   this code.  You are encouraged to report any bugs, problems, fixes,
*   suggestions, and comments about this software to jmerkey@timpanogas.com
*   or linux-kernel@vger.rutgers.edu.  New releases, patches, bug fixes, and
*   technical documentation can be found at www.timpanogas.com.  TRG will
*   periodically post new releases of this software to www.timpanogas.com
*   that contain bug fixes and enhanced capabilities.
*
****************************************************************************

Module Name:

    Read.c

Abstract:

    This module implements the File Read routine for Read called by the
    Fsd/Fsp dispatch drivers.

  Authors:

    Jeff Merkey (jmerkey@timpanogas.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

****************************************************************************/

#include "NwProcs.h"
#include "Fenris.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_READ)

//
//  VOID
//  SafeZeroMemory (
//      IN PUCHAR At,
//      IN ULONG ByteCount
//      );
//

//
//  This macro just puts a nice little try-except around RtlZeroMemory
//

#define SafeZeroMemory(IC,AT,BYTE_COUNT) {                  \
    try {                                                   \
        RtlZeroMemory( (AT), (BYTE_COUNT) );                \
    } except( EXCEPTION_EXECUTE_HANDLER ) {                 \
         NwRaiseStatus( IC, STATUS_INVALID_USER_BUFFER );   \
    }                                                       \
}

//
//  Read ahead amount used for normal data files
//

#define READ_AHEAD_GRANULARITY           (0x10000)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCommonRead)
#endif


NTSTATUS
NwCommonRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common entry point for NtReadFile calls.  For synchronous requests,
    CommonRead will complete the request in the current thread.  If not
    synchronous the request will be passed to the Fsp if there is a need to
    block.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The result of this operation.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN Wait;
    ULONG PagingIo;
    ULONG SynchronousIo;
    ULONG NonCachedIo;

    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER ByteRange;
    ULONG ByteCount;
    ULONG OriginalByteCount;

    BOOLEAN ReleaseFile = FALSE;

    PUCHAR SystemBuffer = NULL;

    NWFS_IO_CONTEXT StackNwIoContext;

    PAGED_CODE();

    //
    //  If this is a zero length read then return SUCCESS immediately.
    //

    if (IrpSp->Parameters.Read.Length == 0) {

        NwCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Decode the file object and verify we support read on this.  It
    //  must be a user file, stream file or volume file (for a data disk).
    //

    TypeOfOpen = NwDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb );

    if ((TypeOfOpen != UserVolumeOpen) &&
        (TypeOfOpen != UserFileOpen)) {

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  Examine our input parameters to determine if this is noncached and/or
    //  a paging io operation.
    //

    Wait = BooleanFlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
    PagingIo = FlagOn( Irp->Flags, IRP_PAGING_IO );
    NonCachedIo = FlagOn( Irp->Flags, IRP_NOCACHE );
    SynchronousIo = FlagOn( IrpSp->FileObject->Flags, FO_SYNCHRONOUS_IO );

    //
    //  Extract the range of the Io.
    //

    StartingOffset = IrpSp->Parameters.Read.ByteOffset;
    OriginalByteCount = ByteCount = IrpSp->Parameters.Read.Length;

    ByteRange.QuadPart = StartingOffset.QuadPart + ByteCount;

    //
    //  Files on Netware volumes are limited to 4Gig.
    //

    if ((TypeOfOpen == UserFileOpen) &&
        (StartingOffset.HighPart || ByteRange.HighPart)) {

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Force DASD I/O to be NonCached
    //

    if (TypeOfOpen == UserVolumeOpen) {
        NonCachedIo = TRUE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Allocate if necessary and initialize a NWFS_IO_CONTEXT block for
        //  all non cached Io.  We handle sync and async I/O identically.
        //

        if (NonCachedIo) {

            if (IrpContext->NwIoContext == NULL) {

                IrpContext->NwIoContext =
                    FsRtlAllocatePoolWithTag( NonPagedPool,
                                              sizeof(NWFS_IO_CONTEXT),
                                              TAG_IO_CONTEXT );
            }

            RtlZeroMemory( IrpContext->NwIoContext, sizeof(NWFS_IO_CONTEXT) );

            KeInitializeSemaphore( &IrpContext->NwIoContext->Semaphore,
                                   0,
                                   MAXLONG );

            IrpContext->NwIoContext->MasterIrp = Irp;
            IrpContext->NwIoContext->IrpCount = MAXULONG;
            IrpContext->NwIoContext->Resource = Fcb->Resource;
            IrpContext->NwIoContext->ResourceThreadId = ExGetCurrentResourceThread();
            IrpContext->NwIoContext->RequestedByteCount = ByteCount;
            IrpContext->NwIoContext->ByteRange = ByteRange;
            IrpContext->NwIoContext->FileObject = IrpSp->FileObject;
        }

        //
        //  Acquire the file shared to perform the read.
        //

        if (!NwAcquireFcbShared( IrpContext, Fcb, FALSE )) {
            try_return( Status = STATUS_CANT_WAIT );
        }

        ReleaseFile = TRUE;

        //
        //  Verify that the Vcb is in a usable condition.
        //

        if (Fcb->Vcb->VcbCondition != VcbMounted) {
            try_return( Status = STATUS_FILE_INVALID );
        }

        //
        //  If this is a non-cached then check whether we need to post this
        //  request if this thread can't block.
        //

        if (!Wait && NonCachedIo) {

            //
            //  Limit the number of times this thread can acquire the resource.
            //

            if (ExIsResourceAcquiredShared( Fcb->Resource ) > MAX_FCB_ASYNC_ACQUIRE) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );
                try_return( Status = STATUS_CANT_WAIT );
            }
        }

        //
        //  Before going any further, get DASD I/O out of the way.
        //

        if (TypeOfOpen == UserVolumeOpen) {

            //
            //  Make sure everything is sector aligned and the user specified non-cached.
            //

            if (SectorOffset(StartingOffset.LowPart) ||
                SectorOffset(ByteCount) ||
                (ByteRange.QuadPart < StartingOffset.QuadPart) ||
                (ByteRange.QuadPart > Fcb->AllocationSize.QuadPart)) {

                try_return( Status = STATUS_INVALID_PARAMETER );
            }

            Status = NwNonCachedIo( IrpContext,
                                    Irp,
                                    Fcb,
                                    (ULONG)LlSectorsFromBytes(StartingOffset.QuadPart),
                                    ByteCount );

            //
            //  Don't complete this request now if STATUS_PENDING was returned.
            //

            if (Status == STATUS_PENDING) {

                IrpContext->NwIoContext = NULL;
                ReleaseFile = FALSE;
            }

            try_return(Status);
        }

        //
        //  We check whether we can proceed based on the state of the file
        //  oplocks and byte range locks.
        //

        Status = FsRtlCheckOplock( &Fcb->Oplock,
                                   Irp,
                                   IrpContext,
                                   NwOplockComplete,
                                   NwPrePostIrp );

        //
        //  If the result is not STATUS_SUCCESS then the Irp was completed
        //  elsewhere.
        //

        if (Status != STATUS_SUCCESS) {

            Irp = NULL;
            IrpContext = NULL;

            try_return( NOTHING );
        }

        if (!PagingIo &&
            (Fcb->FileLock != NULL) &&
            !FsRtlCheckLockForReadAccess( Fcb->FileLock, Irp )) {

            try_return( Status = STATUS_FILE_LOCK_CONFLICT );
        }

        //
        //  Complete the request if it begins beyond the end of file.
        //

        if (StartingOffset.LowPart >= Fcb->FileSize.LowPart) {

            try_return( Status = STATUS_END_OF_FILE );
        }

        //
        //  Truncate the read if it extends beyond the end of the file.
        //

        if (ByteRange.QuadPart > Fcb->FileSize.QuadPart) {

            ByteCount = (ULONG)(Fcb->FileSize.LowPart - StartingOffset.LowPart);
            ByteRange = Fcb->FileSize;

            if (NonCachedIo) {
                IrpContext->NwIoContext->RequestedByteCount = ByteCount;
                IrpContext->NwIoContext->ByteRange = ByteRange;
            }
        }

        if ( NonCachedIo ) {

            //
            //  If the request is non-sector aligned, we need to take a
            //  special path.
            //
            //  If we truncated the read down to file size, but the
            //  buffer is actually big enough to hold this value rounded
            //  up to sector size, and this is paging I/O, then we don't
            //  need to call the non-aligned routine.
            //
            //  NOTE: If this is not paging I/O, then a user could potentially
            //  see uninitialized disk data from another thread, even if ever
            //  so shortly.  In this case we take the real non-aligned path.
            //

            if ((SectorOffset(StartingOffset.LowPart)) ||
                (SectorAlign(ByteCount) > OriginalByteCount) ||
                (SectorOffset(ByteCount) && !PagingIo)) {

                //
                //  Do the physical read.
                //
                //  This routine correctly copies to the byte, so no
                //  zeroing is required.
                //

                Status = NwNonCachedNonAlignedRead( IrpContext,
                                                    Irp,
                                                    Fcb,
                                                    StartingOffset.LowPart,
                                                    ByteCount );

            } else {

                ULONG BytesToRead;

                //
                //  Now, if ByteCount is not sector aligned, then round it
                //  up to a sector and make sure we are synchronous.  We know
                //  the buffer is big enough from the previous 'if' clause.
                //

                if (SectorOffset(ByteCount)) {

                    BytesToRead = SectorAlign(ByteCount);

                    //
                    //  Indicate that we must zero out the garbage we read
                    //  from the disk at the end of the last sector.
                    //

                    if (ByteCount != BytesToRead) {

                        IrpContext->NwIoContext->SectorTailToZero =
                            (PUCHAR)NwMapUserBuffer( IrpContext ) + ByteCount;

                        IrpContext->NwIoContext->BytesToZero =
                            BytesToRead - ByteCount;
                    }

                } else {

                    BytesToRead = ByteCount;
                }

                //
                //  Perform the actual IO
                //

                Status = NwNonCachedIo( IrpContext,
                                        Irp,
                                        Fcb,
                                        SectorsFromBytes(StartingOffset.LowPart),
                                        BytesToRead );
            }

            //
            //  Don't complete this request now if STATUS_PENDING was returned.
            //

            if (Status == STATUS_PENDING) {

                IrpContext->NwIoContext = NULL;
                ReleaseFile = FALSE;
                try_return(Status);
            }

            try_return( NOTHING );
        }

        //
        //  Handle the cached case.  Start by initializing the private
        //  cache map.
        //

        if (IrpSp->FileObject->PrivateCacheMap == NULL) {

            //
            //  Now initialize the cache map.
            //

            CcInitializeCacheMap( IrpSp->FileObject,
                                  (PCC_FILE_SIZES) &Fcb->AllocationSize,
                                  FALSE,
                                  &NwData.CacheManagerCallbacks,
                                  Fcb );

            CcSetReadAheadGranularity( IrpSp->FileObject, READ_AHEAD_GRANULARITY );
        }

        //
        //  Read from the cache if this is not an Mdl read.
        //

        if (!FlagOn( IrpContext->MinorFunction, IRP_MN_MDL )) {

            //
            // If we are in the Fsp now because we had to wait earlier,
            // we must map the user buffer, otherwise we can use the
            // user's buffer directly.
            //

            SystemBuffer = NwMapUserBuffer( IrpContext );

            //
            // Now try to do the copy.
            //

            if (!CcCopyRead( IrpSp->FileObject,
                             &StartingOffset,
                             ByteCount,
                             Wait,
                             SystemBuffer,
                             &Irp->IoStatus )) {

                try_return( Status = STATUS_CANT_WAIT );
            }

            //
            //  If the call didn't succeed, raise the error status
            //

            if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                NwNormalizeAndRaiseStatus( IrpContext, Irp->IoStatus.Status );
            }

        //
        //  Otherwise perform the MdlRead operation.
        //

        } else {

            CcMdlRead( IrpSp->FileObject,
                       &StartingOffset,
                       ByteCount,
                       &Irp->MdlAddress,
                       &Irp->IoStatus );

            Status = Irp->IoStatus.Status;
        }

    try_exit:  NOTHING;

    } finally {

        //
        //  Release the Fcb.
        //

        if (ReleaseFile) {

            NwReleaseFcb( IrpContext, Fcb );
        }
    }

    //
    //  Post the request if we got CANT_WAIT.
    //

    if (Status == STATUS_CANT_WAIT) {

        return NwFsdPostRequest( IrpContext, Irp );
    }

    //
    //  If we received STATUS_PENDING, we need to clean up the IrpContext.
    //

    if (Status == STATUS_PENDING) {

        NwCompleteRequest( IrpContext, NULL, Status );

        return Status;
    }

    //
    //  We do some post processing based on the final status of the operation.
    //

    if (NT_SUCCESS(Status)) {

        //
        //  Update the current file position in the user file object.
        //

        if (SynchronousIo && !PagingIo) {

            IrpSp->FileObject->CurrentByteOffset = ByteRange;
        }

        //
        //  Fill in the number of bytes read.
        //

        Irp->IoStatus.Information = ByteCount;

    } else {

        //
        //  Set the information field to zero.
        //

        Irp->IoStatus.Information = 0;

        //
        //  Raise if this is a user induced error.
        //

        if (IoIsErrorUserInduced( Status )) {

            NwRaiseStatus( IrpContext, Status );
        }

        //
        //  Convert any unknown error code to IO_ERROR.
        //

        Status = FsRtlNormalizeNtstatus( Status, STATUS_UNEXPECTED_IO_ERROR );
    }

    NwCompleteRequest( IrpContext, Irp, Status );

    return Status;
}

