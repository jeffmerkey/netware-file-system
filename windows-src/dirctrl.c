
/***************************************************************************
*
*   Copyright (c) 1999-2022 Jeffrey V. Merkey
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jeffmerkey@gmail.com
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the Lesser GNU Public License 2.1 as
*   published by the Free Software Foundation.  (Windows NT/2000
*   is a registered trademark of Microsoft Corporation).
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
****************************************************************************
/*++

Copyright (c) 1999-2000  Free Software Foundation.

Module Name:

    DirCtrl.c

Abstract:

    This module implements the File Directory Control routines for Nwfs called
    by the Fsd/Fsp dispatch drivers.

  Authors:

    Jeff Merkey (jeffmerkey@gmail.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

--*/

#include "NwProcs.h"
#include "Fenris.h"

//
//  This code conditionalized by the variable provides exhaustive debugging
//  information of the entire FindFirst/FindNext operation.
//

BOOLEAN NwDebugDir = FALSE;

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_DIRCTRL)

//
//  Local support routines
//

NTSTATUS
NwQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN PCCB Ccb
    );

NTSTATUS
NwNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PCCB Ccb
    );

VOID
NwInitializeEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN OUT PCCB Ccb,
    IN OUT PENUM_CONTEXT Entry,
    IN OUT PBOOLEAN ReturnSingleEntry,
    OUT PBOOLEAN InitialQuery
    );

BOOLEAN
NwEnumerateIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN OUT PENUM_CONTEXT Entry
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCommonDirControl)
#pragma alloc_text(PAGE, NwInitializeEnumeration)
#pragma alloc_text(PAGE, NwEnumerateIndex)
#pragma alloc_text(PAGE, NwNotifyChangeDirectory)
#pragma alloc_text(PAGE, NwQueryDirectory)
#endif


NTSTATUS
NwCommonDirControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the entry point for the directory control operations.  These
    are directory enumerations and directory notify calls.  We verify the
    user's handle is for a directory and then call the appropriate routine.

Arguments:

    Irp - Irp for this request.

Return Value:

    NTSTATUS - Status returned from the lower level routines.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Decode the user file object and fail this request if it is not
    //  a user directory.
    //

    if (NwDecodeFileObject( IrpContext,
                            IrpSp->FileObject,
                            &Fcb,
                            &Ccb ) != UserDirectoryOpen) {

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We know this is a directory control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_QUERY_DIRECTORY:

        Status = NwQueryDirectory( IrpContext, Irp, IrpSp, Fcb, Ccb );
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

        Status = NwNotifyChangeDirectory( IrpContext, Irp, IrpSp, Ccb );
        break;

    default:

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}


//
//  Local support routines
//

NTSTATUS
NwQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the query directory operation.  It is responsible
    for either completing of enqueuing the input Irp.  We store the state of the
    search in the Ccb.

Arguments:

    Irp - Supplies the Irp to process

    IrpSp - Stack location for this Irp.

    Fcb - Fcb for this directory.

    Ccb - Ccb for this directory open.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Information = 0;

    ULONG LastEntry = 0;
    ULONG NextEntry = 0;

    BOOLEAN InitialQuery;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN Found;

    ENUM_CONTEXT Entry;

    ULONG SlotIndex;

    PCHAR UserBuffer;
    ULONG BytesRemainingInBuffer;
    ULONG BytesToCopy;
    ULONG BaseLength;

    PFILE_BOTH_DIR_INFORMATION DirInfo;
    PFILE_NAMES_INFORMATION NamesInfo;

    PAGED_CODE();

    RtlZeroMemory( &Entry, sizeof(ENUM_CONTEXT) );

    //
    //  Check if we support this search mode.  Also remember the size of the base part of
    //  each of these structures.
    //

    switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {

    case FileDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_DIRECTORY_INFORMATION,
                                   FileName[0] );
        break;

    case FileFullDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_FULL_DIR_INFORMATION,
                                   FileName[0] );
        break;

    case FileNamesInformation:

        BaseLength = FIELD_OFFSET( FILE_NAMES_INFORMATION,
                                   FileName[0] );
        break;

    case FileBothDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION,
                                   FileName[0] );
        break;

    default:

        NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_INFO_CLASS );
        return STATUS_INVALID_INFO_CLASS;
    }

    //
    //  Get the user buffer.
    //

    UserBuffer = NwMapUserBuffer( IrpContext );

    //
    //  Acquire the directory.
    //

    if (FlagOn( Ccb->Flags, CCB_FLAG_ENUM_INITIALIZED )) {

        NwAcquireFcbShared( IrpContext, Fcb, FALSE );

    } else {

        NwAcquireFcbExclusive( IrpContext, Fcb, FALSE );
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        LARGE_INTEGER GmtTime;

        //
        //  Verify that the Vcb is in a usable condition.
        //

        if (Fcb->Vcb->VcbCondition != VcbMounted) {
            try_return( Status = STATUS_FILE_INVALID );
        }

        //
        //  Start by getting the initial state for the enumeration.  This will
        //  set up the Ccb with the initial search parameters and let us know
        //  the starting offset in the directory to search.
        //

        NwInitializeEnumeration( IrpContext,
                                 IrpSp,
                                 Fcb,
                                 Ccb,
                                 &Entry,
                                 &ReturnSingleEntry,
                                 &InitialQuery );

        //
        //  At this point we are about to enter our query loop.  We have
        //  determined the index into the directory file to begin the
        //  search.  LastEntry and NextEntry are used to index into the user
        //  buffer.  LastEntry is the last entry we've added, NextEntry is
        //  current one we're working on.  If NextEntry is non-zero, then
        //  at least one entry was added.
        //

        while (TRUE) {

            //
            //  If the user had requested only a single match and we have
            //  returned that, then we stop at this point.  We update the Ccb with
            //  the status based on the last entry returned.
            //

            if ((NextEntry != 0) && ReturnSingleEntry) {

                try_return( Status );
            }

            //
            //  If we were looking for a constant expression and this is
            //  not the initial query, or not the first time through the
            //  loop, we have to special case the failure.
            //

            if (FlagOn(Ccb->Flags, CCB_FLAG_ENUM_CONSTANT) && !InitialQuery) {

                if (NextEntry == 0) {
                    Status = STATUS_NO_MORE_FILES;
                }

                try_return( Status );
            }

            //
            //  We try to locate the next matching dirent.  Our search is based
            //  on a starting dirent offset, whether we should return the
            //  current or next entry, and whether we should be doing a short
            //  name search.
            //

            Found = NwEnumerateIndex( IrpContext, Ccb, &Entry );

            //
            //  If we didn't receive a dirent, then we are at the end of the
            //  directory.  If we have returned any files, we exit with
            //  success, otherwise we return STATUS_NO_MORE_FILES.
            //

            if (!Found) {

                if (NextEntry == 0) {

                    Status = STATUS_NO_MORE_FILES;

                    if (InitialQuery) {

                        Status = STATUS_NO_SUCH_FILE;
                    }
                }

                try_return( Status );
            }

            //
            //  Here are the rules concerning filling up the buffer:
            //
            //  1.  The Io system garentees that there will always be
            //      enough room for at least one base record.
            //
            //  2.  If the full first record (including file name) cannot
            //      fit, as much of the name as possible is copied and
            //      STATUS_BUFFER_OVERFLOW is returned.
            //
            //  3.  If a subsequent record cannot completely fit into the
            //      buffer, none of it (as in 0 bytes) is copied, and
            //      STATUS_SUCCESS is returned.  A subsequent query will
            //      pick up with this record.
            //

            //
            //  If the slot for the next entry would be beyond the length of the
            //  user's buffer just exit (we know we've returned at least one entry
            //  already). This will happen when we align the pointer past the end.
            //

            if (NextEntry > IrpSp->Parameters.QueryDirectory.Length) {

                //
                //  Since we already read the directory entry, decrement
                //  the SlotIndex so that we read it again the next time.
                //

                Ccb->SlotIndex -= 1;

                SetFlag(Ccb->Flags, CCB_FLAG_ENUM_RE_READ_ENTRY);

                try_return( Status = STATUS_SUCCESS );
            }

            //
            //  Compute the number of bytes remaining in the buffer.  Round this
            //  down to a WCHAR boundary so we can copy full characters.
            //

            BytesRemainingInBuffer = IrpSp->Parameters.QueryDirectory.Length - NextEntry;
            ClearFlag( BytesRemainingInBuffer, 1 );

            //
            //  If this won't fit and we have returned a previous entry then just
            //  return STATUS_SUCCESS.
            //

            if (BaseLength + Entry.FileName.Length > BytesRemainingInBuffer) {

                //
                //  Since we already read the directory entry, decrement
                //  the SlotIndex so that we read it again the next time.
                //

                Ccb->SlotIndex -= 1;

                SetFlag(Ccb->Flags, CCB_FLAG_ENUM_RE_READ_ENTRY);

                //
                //  If we already found an entry then just exit.
                //

                if (NextEntry != 0) {

                    try_return( Status = STATUS_SUCCESS );
                }

                //
                //  Use a status code of STATUS_BUFFER_OVERFLOW.  Also set
                //  ReturnSingleEntry so that we will exit the loop at the top.
                //

                BytesToCopy =  BytesRemainingInBuffer - BaseLength;
                Status = STATUS_BUFFER_OVERFLOW;
                ReturnSingleEntry = TRUE;

            } else {
                BytesToCopy = Entry.FileName.Length;
            }

            //
            //  Protect access to the user buffer with an exception handler.
            //  Since (at our request) IO doesn't buffer these requests, we have
            //  to guard against a user messing with the page protection and other
            //  such trickery.
            //

            try {

                //
                //  Zero and initialize the base part of the current entry.
                //

                RtlZeroMemory( Add2Ptr( UserBuffer, NextEntry, PVOID ),
                               BaseLength );

                //
                //  Now we have an entry to return to our caller.
                //  We'll case on the type of information requested and fill up
                //  the user buffer if everything fits.
                //

                switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {

                case FileBothDirectoryInformation:
                case FileFullDirectoryInformation:
                case FileDirectoryInformation:

                    DirInfo = Add2Ptr( UserBuffer, NextEntry, PFILE_BOTH_DIR_INFORMATION );

                    //
                    //  Fill in the time stamps.  If the conversion fails, the entries
                    //  are not filled in (i.e. contain zero).
                    //

                    if (RtlTimeFieldsToTime(&Entry.CreationTime, &GmtTime)) {
                        ExLocalTimeToSystemTime( &GmtTime, &DirInfo->CreationTime );
                    }

                    if (RtlTimeFieldsToTime(&Entry.LastWriteTime, &GmtTime)) {
                        ExLocalTimeToSystemTime( &GmtTime, &DirInfo->LastWriteTime );
                    }

                    if (RtlTimeFieldsToTime(&Entry.LastAccessTime, &GmtTime)) {
                        ExLocalTimeToSystemTime( &GmtTime, &DirInfo->LastAccessTime );
                    }

                    //
                    //  Set the sizes separately for directories and files.
                    //

                    if (FlagOn( Entry.FileAttributes, NETWARE_ATTR_DIRECTORY )) {

                        DirInfo->EndOfFile.QuadPart = DirInfo->AllocationSize.QuadPart = 0;

                    } else {

                        DirInfo->EndOfFile.QuadPart = Entry.FileSize;
                        DirInfo->AllocationSize.QuadPart = LlSectorAlign( Entry.FileSize );
                    }

                    //
                    //  All Nwfs files are readonly.  We also copy the existence
                    //  bit to the hidden attribute.
                    //

                    DirInfo->FileAttributes = Entry.FileAttributes;

                    DirInfo->FileIndex = Ccb->SlotIndex;

                    DirInfo->FileNameLength = Entry.FileName.Length;

                    break;

                case FileNamesInformation:

                    NamesInfo = Add2Ptr( UserBuffer, NextEntry, PFILE_NAMES_INFORMATION );

                    NamesInfo->FileIndex = Ccb->SlotIndex;

                    NamesInfo->FileNameLength = Entry.FileName.Length;

                    break;
                }

                //begin debug
                if (NwDebugDir) {
                    WCHAR Name[256]= {0};
                    RtlCopyMemory( Name,
                                   Entry.FileName.Buffer,
                                   Entry.FileName.Length );
                    DbgPrint("  %x: %S\n", Ccb->SlotIndex, Name);
                }

                //
                //  Now copy as much of the name as possible.
                //

                if (BytesToCopy != 0) {

                    RtlCopyMemory( Add2Ptr( UserBuffer, NextEntry + BaseLength, PVOID ),
                                   Entry.FileName.Buffer,
                                   BytesToCopy );
                }

                //
                //  Fill in the short name if we got STATUS_SUCCESS.  The short name
                //  may already be in the file context.  Otherwise we will check
                //  whether the long name is 8.3.  Special case the self and parent
                //  directory names.
                //

                if ((IrpSp->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation) &&
                    (Entry.AlternateFileName.Length)) {

                    RtlCopyMemory( DirInfo->ShortName,
                                   Entry.AlternateFileName.Buffer,
                                   Entry.AlternateFileName.Length );

                    DirInfo->ShortNameLength = (CCHAR) Entry.AlternateFileName.Length;
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {

                //
                //  We had a problem filling in the user's buffer, so stop and
                //  fail this request.  This is the only reason any exception
                //  would have occured at this level.
                //

                Information = 0;
                try_return(Status = GetExceptionCode());
            }

            //
            //  Update the information with the number of bytes stored in the
            //  buffer.  We quad-align the existing buffer to add any necessary
            //  pad bytes.
            //

            Information = NextEntry + BaseLength + BytesToCopy;

            //
            //  Go back to the previous entry and fill in the update to this entry.
            //

            *(Add2Ptr( UserBuffer, LastEntry, PULONG )) = NextEntry - LastEntry;

            //
            //  Set up our variables for the next dirent.
            //

            InitialQuery = FALSE;

            LastEntry = NextEntry;
            NextEntry = QuadAlign( Information );
        }

    try_exit:  NOTHING;

    } finally {

        //
        //  free the Entry string pool
        //

        if (Entry.FileName.Buffer) {
            ExFreePool( Entry.FileName.Buffer );
        }

        //
        //  Release the Fcb.
        //

        NwReleaseFcb( IrpContext, Fcb );
    }

    //begin debug
    if (NwDebugDir) {
        WCHAR Name[32]= {0};
        PUCHAR StatusName = NULL;

        switch (Status) {
        case STATUS_INVALID_PARAMETER:
            StatusName = "STATUS_INVALID_PARAMETER";
            break;
        case STATUS_INVALID_DEVICE_REQUEST:
            StatusName = "STATUS_INVALID_DEVICE_REQUEST";
            break;
        case STATUS_SUCCESS:
            StatusName = "STATUS_SUCCESS";
            break;
        case STATUS_INVALID_INFO_CLASS:
            StatusName = "STATUS_INVALID_INFO_CLASS";
            break;
        case STATUS_NO_MORE_FILES:
            StatusName = "STATUS_NO_MORE_FILES";
            break;
        case STATUS_NO_SUCH_FILE:
            StatusName = "STATUS_NO_SUCH_FILE";
            break;
        case STATUS_BUFFER_OVERFLOW:
            StatusName = "STATUS_BUFFER_OVERFLOW";
            break;
        default :
            break;
        }

        if (StatusName) {
            DbgPrint("  Return Iosb = {%s, 0x%x}\n\n", StatusName, Information);
        } else {
            DbgPrint("  Return Iosb = {%08lx, 0x%x}\n\n", Status, Information);
        }
    }

    //
    //  Complete the request here.
    //

    Irp->IoStatus.Information = Information;

    NwCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routines
//

NTSTATUS
NwNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the notify change directory operation.  It is
    responsible for either completing of enqueuing the input Irp.  Although there
    will never be a notify signalled on a NWFSRO disk we still support this call.

    We have already checked that this is not an OpenById handle.

Arguments:

    Irp - Supplies the Irp to process

    IrpSp - Io stack location for this request.

    Ccb - Handle to the directory being watched.

Return Value:

    NTSTATUS - STATUS_PENDING, any other error will raise.

--*/

{
    PAGED_CODE();

    //
    //  Always set the wait bit in the IrpContext so the initial wait can't fail.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    //
    //  Acquire the Vcb shared.
    //

    NwAcquireVcbShared( IrpContext, IrpContext->Vcb, FALSE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify that the Vcb is a usable condition.
        //

        if (Ccb->Fcb->Vcb->VcbCondition != VcbMounted) {
            NwRaiseStatus( IrpContext, STATUS_FILE_INVALID );
        }

        //
        //  Call the Fsrtl package to process the request.  We cast the
        //  unicode strings to ansi strings as the dir notify package
        //  only deals with memory matching.
        //

        FsRtlNotifyFullChangeDirectory( IrpContext->Vcb->NotifySync,
                                        &IrpContext->Vcb->DirNotifyList,
                                        Ccb,
                                        (PSTRING) &IrpSp->FileObject->FileName,
                                        BooleanFlagOn( IrpSp->Flags, SL_WATCH_TREE ),
                                        FALSE,
                                        IrpSp->Parameters.NotifyDirectory.CompletionFilter,
                                        Irp,
                                        NULL,
                                        NULL );

    } finally {

        //
        //  Release the Vcb.
        //

        NwReleaseVcb( IrpContext, IrpContext->Vcb );
    }

    //
    //  Cleanup the IrpContext.
    //

    NwCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

    return STATUS_PENDING;
}


//
//  Local support routine
//

VOID
NwInitializeEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN OUT PCCB Ccb,
    IN OUT PENUM_CONTEXT Entry,
    OUT PBOOLEAN ReturnSingleEntry,
    OUT PBOOLEAN InitialQuery
    )

/*++

Routine Description:

    This routine is called to initialize the enumeration variables and
    structures.  We look at the state of a previous enumeration from the Ccb
    as well as any input values from the user.  On exit we will position the
    Entry at a file in the directory.

Arguments:

    IrpSp - Irp stack location for this request.

    Fcb - Fcb for this directory.

    Ccb - Ccb for the directory handle.

    Entry - FileContext to use for this enumeration.

    ReturnSingleEntry - Address to store whether we should only return
        a single entry.

    InitialQuery - Address to store whether this is the first enumeration
        query on this handle.

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    PUNICODE_STRING Expression;

    ULONG CcbFlags;

    ULONG DirentOffset;
    ULONG LastDirentOffset;
    BOOLEAN KnownOffset;

    BOOLEAN Found;

    PAGED_CODE();

    //
    //  If this is the initial query then build a search expression from the input
    //  file name.
    //

    if (!FlagOn( Ccb->Flags, CCB_FLAG_ENUM_INITIALIZED )) {

        Expression = (PUNICODE_STRING)IrpSp->Parameters.QueryDirectory.FileName;

        CcbFlags = 0;

        //
        //  If the Expression is not specified or is a single '*' then we will
        //  match all names.
        //

        if ((Expression == NULL) ||
            (Expression->Buffer == NULL) ||
            (Expression->Length == 0) ||
            ((Expression->Length == sizeof( WCHAR )) &&
             (Expression->Buffer[0] == L'*'))) {

            SetFlag( CcbFlags, CCB_FLAG_ENUM_MATCH_ALL );

        //
        //  Otherwise build the NwName from the name in the stack location.
        //  This involves checking for wild card characters and also upcasing
        //  the string if this is a case-insensitive search.
        //

        } else {

            //
            //  Check for wildcards in the search expression.  If there
            //  are none, we take a different path.  In this case, we
            //  will be calling HashFindFile to find the exact file.
            //  In the other case, we will be enumerating via HashFindNext
            //  looking for a match.
            //

            if (FsRtlDoesNameContainWildCards( Expression)) {

                UNICODE_STRING SearchExpression = {0};

                //
                //  Now create the search expression to store in the Ccb.
                //

                SearchExpression.Buffer = FsRtlAllocatePoolWithTag( NwPagedPool,
                                                                    Expression->Length,
                                                                    TAG_ENUM_EXPRESSION );

                SearchExpression.MaximumLength = Expression->Length;

                //
                //  Either copy the name directly or perform the upcase.
                //

                if (FlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE )) {

                    Status = RtlUpcaseUnicodeString( &SearchExpression,
                                                     Expression,
                                                     FALSE );

                    //
                    //  This should never fail.
                    //

                    ASSERT( Status == STATUS_SUCCESS );

                    if (!NT_SUCCESS(Status)) {
                        ExFreePool( SearchExpression.Buffer );
                        NwRaiseStatus( IrpContext, Status );
                    }

                } else {

                    SearchExpression.Length = Expression->Length;

                    RtlCopyMemory( SearchExpression.Buffer,
                                   Expression->Buffer,
                                   Expression->Length );
                }

                Ccb->UnicodeSearchExpression = SearchExpression;

            } else {

                //
                //  Convert the search expression into Oem, upcasing if we are
                //  to do a case insensitive search.
                //

                SetFlag(Ccb->Flags, CCB_FLAG_ENUM_CONSTANT);

                if (FlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE )) {

                    Status = RtlUpcaseUnicodeStringToCountedOemString( &Ccb->OemSearchExpression,
                                                                       Expression,
                                                                       TRUE );

                } else {

                    Status = RtlUnicodeStringToCountedOemString( &Ccb->OemSearchExpression,
                                                                 Expression,
                                                                 TRUE );
                }
            }
        }

        //
        //  Update the values in the Ccb.
        //

        Ccb->SlotIndex = 0;
        Ccb->Context = NULL;

        //
        //  Set the appropriate flags in the Ccb.
        //

        SetFlag( Ccb->Flags, CcbFlags | CCB_FLAG_ENUM_INITIALIZED );
    }

    //
    //  Capture the current state of the enumeration.
    //
    //  If the user specified an index then use his offset.  We always
    //  return the next entry in this case.
    //

    if (FlagOn( IrpSp->Flags, SL_INDEX_SPECIFIED )) {

        Ccb->SlotIndex = IrpSp->Parameters.QueryDirectory.FileIndex;
        Ccb->Context = NULL;
        ClearFlag(Ccb->Flags, CCB_FLAG_ENUM_RE_READ_ENTRY);

    //
    //  If we are restarting the scan then go from the self entry.
    //

    } else if (FlagOn( IrpSp->Flags, SL_RESTART_SCAN )) {

        Ccb->SlotIndex = 0;
        Ccb->Context = NULL;
        ClearFlag(Ccb->Flags, CCB_FLAG_ENUM_RE_READ_ENTRY);
    }

    //
    //  Otherwise use the values from the Ccb.
    //
    //  We have the starting offset in the directory and whether to return
    //  that entry or the next.  If we are at the beginning of the directory
    //  and are returning that entry, then tell our caller this is the
    //  initial query.
    //

    *InitialQuery = FALSE;

    if (Ccb->SlotIndex == 0) {

        *InitialQuery = TRUE;
    }

    //
    //  Look at the flag in the IrpSp indicating whether to return just
    //  one entry.
    //

    *ReturnSingleEntry = FALSE;

    if (FlagOn( IrpSp->Flags, SL_RETURN_SINGLE_ENTRY )) {

        *ReturnSingleEntry = TRUE;
    }

    //
    //  Allocate the memory for the enum context
    //

    Entry->FileName.Buffer = FsRtlAllocatePoolWithTag( NwPagedPool,
                                                       256 * sizeof(WCHAR) * 2,
                                                       TAG_ENUM_CONTEXT );
    Entry->FileName.MaximumLength = 256 * sizeof(WCHAR);

    Entry->UpcasedFileName.Buffer = &Entry->FileName.Buffer[256];
    Entry->UpcasedFileName.MaximumLength = 256 * sizeof(WCHAR);

    Entry->AlternateFileName.Buffer = Entry->AlternateFileNameBuffer;
    Entry->AlternateFileName.MaximumLength = 12 * sizeof(WCHAR);

    // begin debug
    if (NwDebugDir) {

        WCHAR Expr[256]= {0};

        PUCHAR Class, Flag;
        PUCHAR Flags[8] = {"None",
                           "RS",
                           "RSE",
                           "RS|RSE",
                           "IS",
                           "RS|IS",
                           "RSE|IS",
                           "RS|RSE|IS"};

        switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {
        case FileDirectoryInformation:
            Class = "DirInfo";
            break;

        case FileFullDirectoryInformation:
            Class = "FullDirInfo";
            break;

        case FileNamesInformation:
            Class = "NamesInfo";
            break;

        case FileBothDirectoryInformation:
            Class = "BothDirInfo";
        }

        Flag = Flags[IrpSp->Flags & 7];

        DbgPrint("NWFSRO DirCtrl - Class: %s, Flags: %s", Class, Flag);

        if (FlagOn( IrpSp->Flags, SL_INDEX_SPECIFIED )) {
            DbgPrint(", Index: 0x%x", IrpSp->Parameters.QueryDirectory.FileIndex);
        }

        if (Ccb->UnicodeSearchExpression.Buffer) {

            RtlCopyMemory( Expr,
                           Ccb->UnicodeSearchExpression.Buffer,
                           Ccb->UnicodeSearchExpression.Length );

            DbgPrint(", Slot: 0x%x, Buffer Size: 0x%x, Expr: %S\n",
                     Ccb->SlotIndex,
                     IrpSp->Parameters.QueryDirectory.Length,
                     FlagOn( Ccb->Flags, CCB_FLAG_ENUM_MATCH_ALL ) ?
                     L"MATCH ALL" : Expr);

        } else {

            ASSERT( Ccb->OemSearchExpression.Buffer );
            RtlCopyMemory( Expr,
                           Ccb->OemSearchExpression.Buffer,
                           Ccb->OemSearchExpression.Length );

            DbgPrint(", Slot: 0x%x, Buffer Size: 0x%x, Expr: %s\n",
                     Ccb->SlotIndex,
                     IrpSp->Parameters.QueryDirectory.Length,
                     FlagOn( Ccb->Flags, CCB_FLAG_ENUM_MATCH_ALL ) ?
                     "MATCH ALL" : (PUCHAR)Expr);
        }
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
NwEnumerateIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN OUT PENUM_CONTEXT Entry
    )

/*++

Routine Description:

    This routine is the worker routine for index enumeration.  We are positioned
    at some dirent in the directory and will either return the first match
    at that point or look to the next entry.  The Ccb contains details about
    the type of matching to do. 

Arguments:

    Ccb - Ccb for this directory handle.

    Entry - File context already positioned at some entry in the directory.

Return Value:

    BOOLEAN - TRUE if next entry is found, FALSE otherwise.

--*/

{
    BOOLEAN Found = FALSE;
    PUNICODE_STRING FileName;
    PHASH Hash;
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  We want to special case constant names and use the hash built into
    //  the NWFS code.
    //

    if (FlagOn(Ccb->Flags, CCB_FLAG_ENUM_CONSTANT)) {

        Hash = HashFindFile( Ccb->Fcb->Vcb->VolumeNumber,
                             Ccb->Fcb->DirNumber,
                             &Ccb->OemSearchExpression,
                             BooleanFlagOn(Ccb->Flags, CCB_FLAG_IGNORE_CASE) );

        if (!Hash) {
            return FALSE;
        }

        Hash = Hash->nlroot;
        Ccb->SlotIndex = Hash->SlotNumber;

        //
        //  Extract all the info from the Hash into NT form.
        //

        NwSetEnumContext( IrpContext, Hash, Entry, Ccb->SlotIndex );

        return TRUE;
    }

    //
    //  Loop until we find a match or exaust the directory.
    //

    while (TRUE) {

        PFCB Fcb;

        Fcb = Ccb->Fcb;

        //
        //  We need to special case . and .. (SlotIndex 0 and 1), but not in
        //  the root dir.
        //

        if ((Fcb == Fcb->Vcb->RootDirFcb) && (Ccb->SlotIndex == 0)) {
            Ccb->SlotIndex = 2;
        }

        if (Ccb->SlotIndex < 2) {

            Hash = Fcb->Hash;

            Ccb->SlotIndex++;

        }  else {

            //
            //  Get the next file in this directory.  On return, Ccb->SlotIndex
            //  will point to the next entry if a hash was found.
            //

            Hash = HashFindNext( Ccb->Fcb->Vcb->VolumeNumber,
                                 &Ccb->SlotIndex,
                                 Ccb->Fcb->DirNumber,
                                 &Ccb->Context,
                                 BooleanFlagOn(Ccb->Flags, CCB_FLAG_ENUM_RE_READ_ENTRY) );

            ClearFlag(Ccb->Flags, CCB_FLAG_ENUM_RE_READ_ENTRY);

            if (!Hash) {
                return FALSE;
            }

            ASSERT( Hash->nlroot->SlotNumber == Ccb->SlotIndex - 1);
        }

        //
        //  Extract all the info from the Hash into NT form.
        //

        NwSetEnumContext( IrpContext, Hash, Entry, Ccb->SlotIndex - 1);


        //
        //  Now check if we should return it.
        //

        if (FlagOn(Ccb->Flags, CCB_FLAG_ENUM_MATCH_ALL)) {
            return TRUE;
        }

        //
        //  If we ignore case, upcase the name now.
        //

        if (FlagOn(Ccb->Flags, CCB_FLAG_IGNORE_CASE) &&
            !Entry->FileNameUpcase) {

            Status = RtlUpcaseUnicodeString( &Entry->UpcasedFileName, &Entry->FileName, FALSE );
            if (!NT_SUCCESS(Status)) {
                NwRaiseStatus( IrpContext, Status );
            }

            FileName = &Entry->UpcasedFileName;

        } else {

            FileName = &Entry->FileName;
        }

        //
        //  Now see if we have a match on the main name.
        //

        if (NwIsNameInExpression( IrpContext,
                                  FileName,
                                  &Ccb->UnicodeSearchExpression,
                                  Ccb->Flags )) {

            return TRUE;
        }

        //
        //  and if not, on the alternative name.
        //

        if (Entry->AlternateFileName.Length &&
            NwIsNameInExpression( IrpContext,
                                  &Entry->AlternateFileName,
                                  &Ccb->UnicodeSearchExpression,
                                  Ccb->Flags )) {

            return TRUE;
        }
    }
}

VOID
NwSetEnumContext(
    IN PIRP_CONTEXT IrpContext,
    IN PHASH Hash,
    IN PENUM_CONTEXT Entry,
    IN ULONG SlotIndex
    )

/*++

Routine Description:

    This routine fills in an 'entry' structure from an a hash structure.  The
    basic idea here is to convert data from an Netware form to an NT form. 

Arguments:

    Hash - The source for the data we are converting.

    Entry - The target that will receive the data.
    
    SlotIndex - This is the location of the file in its parent directory.  It
        is currently only used to special case . and .. (i.e. Index 0 and 1).

Return Value:

    None

--*/

{
    PHASH DosHash;
    UCHAR NameSpace;
    ULONG NameSpaceMask = 0;
    OEM_STRING FileName;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    //  We take most values out of the DOS name space hash entry.
    //

    DosHash = Hash->nlroot;

    Entry->FileAttributes = (DosHash->FileAttributes & NETWARE_ATTR_MASK) |
                            NETWARE_ATTR_READ_ONLY;

    Entry->FileSize = DosHash->FileSize;

    (VOID)NWFSConvertFromNetwareTime( DosHash->CreateDateAndTime,
                                      &Entry->CreationTime );

    (VOID)NWFSConvertFromNetwareTime( DosHash->UpdatedDateAndTime,
                                      &Entry->LastWriteTime );

    (VOID)NWFSConvertFromNetwareTime( (DosHash->AccessedDate) << 16,
                                      &Entry->LastAccessTime );

    Hash = DosHash->nlnext;

    //
    //  Now, we special case Slots 0 and 1, which are . and ..
    //

    if (SlotIndex < 2) {

        Entry->FileName.Length = sizeof(WCHAR) * (SlotIndex + 1);
        Entry->FileName.Buffer[0] = L'.';
        Entry->FileName.Buffer[1] = L'.';

        Entry->FileNameUpcase = TRUE;

        Entry->AlternateFileName.Length = 0; // no alternate name
        return;
    }

    //
    //  Find all the name spaces.
    //

    while (Hash) {

        NameSpaceMask |= 1<< Hash->NameSpace;
        Hash = Hash->nlnext;
    }

    NameSpaceMask &= ((1 << MAC_NAME_SPACE) |
                      (1 << UNIX_NAME_SPACE) |
                      (1 << LONG_NAME_SPACE) |
                      (1 << NT_NAME_SPACE));

    //
    //  If there were no other usable spaces, just use the DOS space.
    //

    if (NameSpaceMask == 0) {

        //
        //  If the caller didn't supply a FileName buffer, just return here.
        //

        if (Entry->FileName.Buffer == NULL) {
            return;
        }

        //
        //  otherwise, convert the name to Unicode.
        //

        FileName.Length =
        FileName.MaximumLength = DosHash->NameLength;
        FileName.Buffer = DosHash->Name;

        Entry->FileNameUpcase = TRUE;

        Status = RtlOemStringToCountedUnicodeString( &Entry->FileName,
                                                     &FileName,
                                                     FALSE );

        if (!NT_SUCCESS(Status)) {
            NwRaiseStatus( IrpContext, Status );
        }

        Entry->AlternateFileName.Length = 0; // no alternate name
        return;
    }

    //
    //  Now figure out which name space we'll use for the long name.
    //

    if (NameSpaceMask & (1 << NT_NAME_SPACE)) {
        NameSpace = NT_NAME_SPACE;
    } else if (NameSpaceMask & (1 << LONG_NAME_SPACE)) {
        NameSpace = LONG_NAME_SPACE;
    } else if (NameSpaceMask & (1 << UNIX_NAME_SPACE)) {
        NameSpace = UNIX_NAME_SPACE;
    } else {
        NameSpace = MAC_NAME_SPACE;
    }

    //
    //  Now get the long name.
    //

    Hash = DosHash->nlnext;

    while (Hash) {

        //
        //  When we find our name space, get the name and also the DOS
        //  name as an alternate unless they are exactly identical.
        //

        if (Hash->NameSpace == NameSpace) {

            //
            //  If the caller didn't supply a FileName buffer, don't copy
            //  the long name from the hash.
            //

            if (Entry->FileName.Buffer != NULL) {

                FileName.Length =
                FileName.MaximumLength = Hash->NameLength;
                FileName.Buffer = Hash->Name;

                Status = RtlOemStringToCountedUnicodeString( &Entry->FileName,
                                                             &FileName,
                                                             FALSE );

                if (!NT_SUCCESS(Status)) {
                    NwRaiseStatus( IrpContext, Status );
                }

            } else {

                //
                //  ...but we still need to continue to see if we need an
                //  alternate name or not.
                //

                NOTHING;
            }

            //
            //  If the file name in the long space is identical to the
            //  DOS space, then there is no alternate name.
            //

            if (Hash->NameLength == DosHash->NameLength) {

                ULONG i;

                for (i=0; i<DosHash->NameLength; i++) {

                    if (NwOemUpcaseTable[Hash->Name[i]] != DosHash->Name[i]) {
                        break;
                    }
                }

                if (i == DosHash->NameLength) {
                    Entry->AlternateFileName.Length = 0; // no alternate name
                    return;
                }
            }

            //
            //  otherwise we have an alternate name to return.
            //

            FileName.Length =
            FileName.MaximumLength = DosHash->NameLength;
            FileName.Buffer = DosHash->Name;

            Status = RtlOemStringToCountedUnicodeString( &Entry->AlternateFileName,
                                                         &FileName,
                                                         FALSE );

            if (!NT_SUCCESS(Status)) {
                NwRaiseStatus( IrpContext, Status );
            }
        }

        Hash = Hash->nlnext;
    }
}

