
/***************************************************************************
*
*   Copyright (c) 1999-2000 Timpanogas Research Group, Inc.
*   895 West Center Street
*   Orem, Utah  84057
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

    NwData.c

Abstract:

    This module declares the global data used by the Netware file system.

    This module also handles the dispath routines in the Fsd threads as well as
    handling the IrpContext and Irp through the exception paths.

  Authors:

    Jeff Merkey (jmerkey@timpanogas.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

****************************************************************************/

#include "NwProcs.h"

#ifdef NW_SANITY
BOOLEAN NwfsTestTopLevel = TRUE;
#endif

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_NWDATA)

//
//  Global data structures
//

NW_DATA NwData;
FAST_IO_DISPATCH NwFastIoDispatch;
FILE_ID FileIdZero = {0};

PUCHAR NwOemUpcaseTable = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwFastIoCheckIfPossible)
#endif


NTSTATUS
NwFsdDispatch (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the driver entry to all of the Fsd dispatch points.

    Conceptually the Io routine will call this routine on all requests
    to the file system.  We case on the type of request and invoke the
    correct handler for this type of request.  There is an exception filter
    to catch any exceptions in the NWFS code as well as the NWFS process
    exception routine.

    This routine allocates and initializes the IrpContext for this request as
    well as updating the top-level thread context as necessary.  We may loop
    in this routine if we need to retry the request for any reason.  The
    status code STATUS_CANT_WAIT is used to indicate this.

Arguments:

    VolumeDeviceObject - Supplies the volume device object for this request

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    THREAD_CONTEXT ThreadContext;
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN Wait;

#ifdef NW_SANITY
    PVOID PreviousTopLevel;
#endif

    NTSTATUS Status;
    KIRQL SaveIrql;

    ASSERT_OPTIONAL_IRP( Irp );

    //
    //  Before doing anything, fail non-volume IRPs if for an illegal function.
    //

    if ((PDEVICE_OBJECT)VolumeDeviceObject == NwData.FileSystemDeviceObject) {
        switch (IoGetCurrentIrpStackLocation( Irp )->MajorFunction) {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLEANUP:
        case IRP_MJ_CLOSE:
        case IRP_MJ_FILE_SYSTEM_CONTROL:
        case IRP_MJ_DEVICE_CONTROL:

            break;

        default:

            NwCompleteRequest(NULL, Irp, STATUS_INVALID_DEVICE_REQUEST);
            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    SaveIrql = KeGetCurrentIrql();

    FsRtlEnterFileSystem();

#ifdef NW_SANITY
    PreviousTopLevel = IoGetTopLevelIrp();
#endif

    //
    //  Loop until this request has been completed or posted.
    //

    do {

        //
        //  Use a try-except to handle the exception cases.
        //

        try {

            //
            //  If the IrpContext is NULL then this is the first pass through
            //  this loop.
            //

            if (IrpContext == NULL) {

                //
                //  Decide if this request is waitable an allocate the IrpContext.
                //  If the file object in the stack location is NULL then this
                //  is a mount which is always waitable.  Otherwise we look at
                //  the file object flags.
                //

                if (IoGetCurrentIrpStackLocation( Irp )->FileObject == NULL) {

                    Wait = TRUE;

                } else {

                    Wait = CanFsdWait( Irp );
                }

                IrpContext = NwCreateIrpContext( Irp, Wait );

                //
                //  Update the thread context information.
                //

                NwSetThreadContext( IrpContext, &ThreadContext );

#ifdef NW_SANITY
                ASSERT( !NwfsTestTopLevel ||
                        SafeNodeType( IrpContext->TopLevel ) == NWFS_NTC_IRP_CONTEXT );
#endif

            //
            //  Otherwise cleanup the IrpContext for the retry.
            //

            } else {

                //
                //  Set the MORE_PROCESSING flag to make sure the IrpContext
                //  isn't inadvertently deleted here.  Then cleanup the
                //  IrpContext to perform the retry.
                //

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING );
                NwCleanupIrpContext( IrpContext, FALSE );
            }

            //
            //  Initialize the Io status field in the Irp.
            //

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;

            //
            //  Case on the major irp code.
            //

            switch (IrpContext->MajorFunction) {

            case IRP_MJ_CREATE :

                Status = NwCommonCreate( IrpContext, Irp );
                break;

            case IRP_MJ_CLOSE :

                Status = NwCommonClose( IrpContext, Irp );
                break;

            case IRP_MJ_READ :

                //
                //  If this is an Mdl complete request, don't go through
                //  common read.
                //

                if (FlagOn( IrpContext->MinorFunction, IRP_MN_COMPLETE )) {

                    Status = NwCompleteMdl( IrpContext, Irp );

                //
                //  Always post the Dpc requests.
                //

                } else if (FlagOn( IrpContext->MinorFunction, IRP_MN_DPC )) {

                    ClearFlag( IrpContext->MinorFunction, IRP_MN_DPC );

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );

                    NwRaiseStatus( IrpContext, STATUS_CANT_WAIT );

                } else {

                    Status = NwCommonRead( IrpContext, Irp );
                }

                break;

            case IRP_MJ_QUERY_INFORMATION :

                Status = NwCommonQueryInfo( IrpContext, Irp );
                break;

            case IRP_MJ_SET_INFORMATION :

                Status = NwCommonQueryInfo( IrpContext, Irp );
                break;

            case IRP_MJ_QUERY_VOLUME_INFORMATION :

                Status = NwCommonQueryVolInfo( IrpContext, Irp );
                break;

            case IRP_MJ_DIRECTORY_CONTROL :

                Status = NwCommonDirControl( IrpContext, Irp );
                break;

            case IRP_MJ_FILE_SYSTEM_CONTROL :

                Status = NwCommonFsControl( IrpContext, Irp );
                break;

            case IRP_MJ_DEVICE_CONTROL :

                Status = NwCommonDevControl( IrpContext, Irp );
                break;

            case IRP_MJ_LOCK_CONTROL :

                Status = NwCommonLockControl( IrpContext, Irp );
                break;

            case IRP_MJ_CLEANUP :

                Status = NwCommonCleanup( IrpContext, Irp );
                break;

            default :

                Status = STATUS_INVALID_DEVICE_REQUEST;
                NwCompleteRequest( IrpContext, Irp, Status );
            }

        } except( NwExceptionFilter( IrpContext, GetExceptionInformation() )) {

            Status = NwProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT);

#ifdef NW_SANITY
    ASSERT( !NwfsTestTopLevel ||
            (PreviousTopLevel == IoGetTopLevelIrp()) );
#endif

    FsRtlExitFileSystem();

    ASSERT( SaveIrql == KeGetCurrentIrql( ));

    return Status;
}


LONG
NwExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is used to decide whether we will handle a raised exception
    status.  If NWFS explicitly raised an error then this status is already
    in the IrpContext.  We choose which is the correct status code and
    either indicate that we will handle the exception or bug-check the system.

Arguments:

    ExceptionCode - Supplies the exception code to being checked.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode;
    BOOLEAN TestStatus = TRUE;

    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );

    ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

    //
    // If the exception is STATUS_IN_PAGE_ERROR, get the I/O error code
    // from the exception record.
    //

    if ((ExceptionCode == STATUS_IN_PAGE_ERROR) &&
        (ExceptionPointer->ExceptionRecord->NumberParameters >= 3)) {

        ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
    }

    //
    //  If there is an Irp context then check which status code to use.
    //

    if (ARGUMENT_PRESENT( IrpContext )) {

        if (IrpContext->ExceptionStatus == STATUS_SUCCESS) {

            //
            //  Store the real status into the IrpContext.
            //

            IrpContext->ExceptionStatus = ExceptionCode;

        } else {

            //
            //  No need to test the status code if we raised it ourselves.
            //

            TestStatus = FALSE;
        }
    }

#ifdef NW_SANITY
    ASSERT( (ExceptionCode != STATUS_DISK_CORRUPT_ERROR ) &&
            (ExceptionCode != STATUS_FILE_CORRUPT_ERROR ));
#endif

    //
    //  Bug check if this status is not supported.
    //

    if (TestStatus && !FsRtlIsNtstatusExpected( ExceptionCode )) {

        NwBugCheck( (ULONG) ExceptionPointer->ExceptionRecord,
                    (ULONG) ExceptionPointer->ContextRecord,
                    (ULONG) ExceptionPointer->ExceptionRecord->ExceptionAddress );

    }

    return EXCEPTION_EXECUTE_HANDLER;
}


NTSTATUS
NwProcessException (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This routine processes an exception.  It either completes the request
    with the exception status in the IrpContext or sends this off to the Fsp
    workque.

Arguments:

    Irp - Supplies the Irp being processed

    ExceptionCode - Supplies the normalized exception status being handled

Return Value:

    NTSTATUS - Returns the results of either posting the Irp or the
        saved completion status.

--*/

{
    PDEVICE_OBJECT Device;
    PVPB Vpb;
    PETHREAD Thread;

    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  If there is not an irp context, then complete the request with the
    //  current status code.
    //

    if (!ARGUMENT_PRESENT( IrpContext )) {

        NwCompleteRequest( NULL, Irp, ExceptionCode );
        return ExceptionCode;
    }

    //
    //  Get the real exception status from the IrpContext.
    //

    ExceptionCode = IrpContext->ExceptionStatus;

    //
    //  If we are not a top level request then we just complete the request
    //  with the current status code.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL )) {

        NwCompleteRequest( IrpContext, Irp, ExceptionCode );

        return ExceptionCode;
    }

    //
    //  Check if we are posting this request.  The following must be true if
    //  we are to post a request.
    //
    //      - Status code is STATUS_CANT_WAIT and the request is asynchronous
    //          or we are forcing this to be posted.
    //
    //  Set the MORE_PROCESSING flag in the IrpContext to keep it from being
    //  deleted if this is a retryable condition.
    //
    //  Note that (children of) NwFsdPostRequest can raise (Mdl allocation).
    //

    try {

        if ((ExceptionCode == STATUS_CANT_WAIT) &&
            FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST )) {
    
            ExceptionCode = NwFsdPostRequest( IrpContext, Irp );

        } else if (ExceptionCode == STATUS_VERIFY_REQUIRED) {

            //
            //  For right now we won't support dynamic removable media.
            //  If we got a verify, just clear the bit and post the
            //  request.
            //

            Device = IoGetDeviceToVerify( Irp->Tail.Overlay.Thread );
            IoSetDeviceToVerify( Irp->Tail.Overlay.Thread, NULL );

            //
            //  If there is no device in that location then check in the
            //  current thread.
            //

            if (Device == NULL) {
                Device = IoGetDeviceToVerify( PsGetCurrentThread() );
                IoSetDeviceToVerify( PsGetCurrentThread(), NULL );
            }

            if (Device) {
                ClearFlag( Device->Flags, DO_VERIFY_VOLUME );
            }

            ExceptionCode = NwFsdPostRequest( IrpContext, Irp );
        }

    } except( NwExceptionFilter( IrpContext, GetExceptionInformation() ))  {

        ExceptionCode = GetExceptionCode();
    }

    //
    //  If we posted the request or our caller will retry then just return here.
    //

    if ((ExceptionCode == STATUS_PENDING) ||
        (ExceptionCode == STATUS_CANT_WAIT)) {

        return ExceptionCode;
    }

    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING );

    //
    //  Store this error into the Irp for posting back to the Io system.
    //

    Irp->IoStatus.Status = ExceptionCode;

    if (IoIsErrorUserInduced( ExceptionCode )) {

        //
        //  The other user induced conditions generate an error unless
        //  they have been disabled for this request.
        //

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS )) {

            NwCompleteRequest( IrpContext, Irp, ExceptionCode );

            return ExceptionCode;

        } else {

            //
            //  Generate a pop-up
            //

            if (IoGetCurrentIrpStackLocation( Irp )->FileObject != NULL) {

                Vpb = IoGetCurrentIrpStackLocation( Irp )->FileObject->Vpb;

            } else {

                Vpb = NULL;
            }

            //
            //  The device to verify is either in my thread local storage
            //  or that of the thread that owns the Irp.
            //

            Thread = Irp->Tail.Overlay.Thread;
            Device = IoGetDeviceToVerify( Thread );

            if (Device == NULL) {

                Thread = PsGetCurrentThread();
                Device = IoGetDeviceToVerify( Thread );

                ASSERT( Device != NULL );

                //
                //  Let's not BugCheck just because the driver screwed up.
                //

                if (Device == NULL) {

                    NwCompleteRequest( IrpContext, Irp, ExceptionCode );

                    return ExceptionCode;
                }
            }

            //
            //  This routine actually causes the pop-up.  It usually
            //  does this by queuing an APC to the callers thread,
            //  but in some cases it will complete the request immediately,
            //  so it is very important to IoMarkIrpPending() first.
            //

            IoMarkIrpPending( Irp );
            IoRaiseHardError( Irp, Vpb, Device );

            //
            //  We will be handing control back to the caller here, so
            //  reset the saved device object.
            //

            IoSetDeviceToVerify( Thread, NULL );

            //
            //  The Irp will be completed by Io or resubmitted.  In either
            //  case we must clean up the IrpContext here.
            //

            NwCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );
            return STATUS_PENDING;
        }
    }

    //
    //  This is just a run of the mill error.
    //

    NwCompleteRequest( IrpContext, Irp, ExceptionCode );

    return ExceptionCode;
}


VOID
NwCompleteRequest (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine completes a Irp and cleans up the IrpContext.  Either or
    both of these may not be specified.

Arguments:

    Irp - Supplies the Irp being processed.

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );
    ASSERT_OPTIONAL_IRP( Irp );

    //
    //  Cleanup the IrpContext if passed in here.
    //

    if (ARGUMENT_PRESENT( IrpContext )) {

        NwCleanupIrpContext( IrpContext, FALSE );
    }

    //
    //  If we have an Irp then complete the irp.
    //

    if (ARGUMENT_PRESENT( Irp )) {

        //
        //  Clear the information field in case we have used this Irp
        //  internally.
        //

        if (NT_ERROR( Status ) &&
            FlagOn( Irp->Flags, IRP_INPUT_OPERATION )) {

            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    }

    return;
}


VOID
NwSetThreadContext (
    IN PIRP_CONTEXT IrpContext,
    IN PTHREAD_CONTEXT ThreadContext
    )

/*++

Routine Description:

    This routine is called at each Fsd/Fsp entry point set up the IrpContext
    and thread local storage to track top level requests.  If there is
    not a Nwfs context in the thread local storage then we use the input one.
    Otherwise we use the one already there.  This routine also updates the
    IrpContext based on the state of the top-level context.

    If the TOP_LEVEL flag in the IrpContext is already set when we are called
    then we force this request to appear top level.

Arguments:

    ThreadContext - Address on stack for local storage if not already present.

    ForceTopLevel - We force this request to appear top level regardless of
        any previous stack value.

Return Value:

    None

--*/

{
    PTHREAD_CONTEXT CurrentThreadContext;
    ULONG_PTR StackTop;
    ULONG_PTR StackBottom;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Get the current top-level irp out of the thread storage.
    //  If NULL then this is the top-level request.
    //

    CurrentThreadContext = (PTHREAD_CONTEXT) IoGetTopLevelIrp();

    if (CurrentThreadContext == NULL) {

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL );
    }

    //
    //  Initialize the input context unless we are using the current
    //  thread context block.  We use the new block if our caller
    //  specified this or the existing block is invalid.
    //
    //  The following must be true for the current to be a valid Nwfs context.
    //
    //      Structure must lie within current stack.
    //      Address must be ULONG aligned.
    //      Nwfs signature must be present.
    //
    //  If this is not a valid Nwfs context then use the input thread
    //  context and store it in the top level context.
    //

    IoGetStackLimits( &StackTop, &StackBottom);

    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL ) ||
        (((ULONG_PTR) CurrentThreadContext > StackBottom - sizeof( THREAD_CONTEXT )) ||
         ((ULONG_PTR) CurrentThreadContext <= StackTop) ||
         FlagOn( (ULONG_PTR) CurrentThreadContext, 0x3 ) ||
         (CurrentThreadContext->Nwfs != 0x54373534))) {

        ThreadContext->Nwfs = 0x54373534;
        ThreadContext->SavedTopLevelIrp = (PIRP) CurrentThreadContext;
        ThreadContext->TopLevelIrpContext = IrpContext;
        IoSetTopLevelIrp( (PIRP) ThreadContext );

        IrpContext->TopLevel = IrpContext;
        IrpContext->ThreadContext = ThreadContext;

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL_NWFS );

    //
    //  Otherwise use the IrpContext in the thread context.
    //

    } else {

        IrpContext->TopLevel = CurrentThreadContext->TopLevelIrpContext;
    }

    return;
}



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
    )

/*++

Routine Description:

    This routine checks if fast i/o is possible for a read/write operation

Arguments:

    FileObject - Supplies the file object used in the query

    FileOffset - Supplies the starting byte offset for the read/write operation

    Length - Supplies the length, in bytes, of the read/write operation

    Wait - Indicates if we can wait

    LockKey - Supplies the lock key

    CheckForReadOperation - Indicates if this is a check for a read or write
        operation

    IoStatus - Receives the status of the operation if our return value is
        FastIoReturnError

Return Value:

    BOOLEAN - TRUE if fast I/O is possible and FALSE if the caller needs
        to take the long route.

--*/

{
    PFCB Fcb;
    TYPE_OF_OPEN TypeOfOpen;
    LARGE_INTEGER LargeLength;

    PAGED_CODE();

    //
    //  Decode the type of file object we're being asked to process and
    //  make sure that is is only a user file open.
    //

    TypeOfOpen = NwFastDecodeFileObject( FileObject, &Fcb );

    if ((TypeOfOpen != UserFileOpen) || !CheckForReadOperation) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return TRUE;
    }

    LargeLength.QuadPart = Length;

    //
    //  Check whether the file locks will allow for fast io.
    //

    if ((Fcb->FileLock == NULL) ||
        FsRtlFastCheckLockForRead( Fcb->FileLock,
                                   FileOffset,
                                   &LargeLength,
                                   LockKey,
                                   FileObject,
                                   PsGetCurrentProcess() )) {

        return TRUE;
    }

    return FALSE;
}

VOID
NwPopUpFileCorrupt (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    The Following routine makes an informational popup that the file is corrupt.

Arguments:

    IrpContext - Supplies the file that is corrupt, indirectly.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PKTHREAD Thread;
    PCCB Ccb = NULL;
    PFCB DontCare;

    IrpSp = IoGetCurrentIrpStackLocation( IrpContext->Irp );

    if (IrpSp->FileObject == NULL) {
        return;
    }

    (VOID)NwDecodeFileObject( IrpContext, IrpSp->FileObject, &DontCare, &Ccb );

    //
    //  If this is an open by ID, or there is not a name filled in for
    //  some reason, we will just forget about the pop-up.
    //

    if ((Ccb == NULL) ||
        FlagOn(Ccb->Flags, CCB_FLAG_OPEN_RELATIVE_BY_ID | CCB_FLAG_OPEN_BY_ID) ||
        (Ccb->OriginalName.Length == 0) ||
        (Ccb->OriginalName.Buffer == NULL)) {

        return;
    }

    //
    //  We never want to block a system thread waiting for the user to
    //  press OK.
    //

    if (IoIsSystemThread(IrpContext->Irp->Tail.Overlay.Thread)) {

       Thread = NULL;

    } else {

       Thread = IrpContext->Irp->Tail.Overlay.Thread;
    }

    IoRaiseInformationalHardError( STATUS_FILE_CORRUPT_ERROR,
                                   &Ccb->OriginalName,
                                   Thread);
}
