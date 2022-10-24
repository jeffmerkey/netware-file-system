
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

    Create.c

Abstract:

    This module implements the File Create routine for Nwfs called by the
    Fsd/Fsp dispatch routines.

  Authors:

    Jeff Merkey (jmerkey@timpanogas.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

***************************************************************************/

#include "NwProcs.h"
#include "Fenris.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_CREATE)

//
//  Local support routines
//

NTSTATUS
NwNormalizeFileNames (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN OpenByFileId,
    IN BOOLEAN IgnoreCase,
    IN TYPE_OF_OPEN RelatedTypeOfOpen,
    IN OUT PUNICODE_STRING FileName
    );

NTSTATUS
NwOpenByFileId (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *FcbToRelease
    );

PFCB
NwInitializeFcbFromHash (
    IN PIRP_CONTEXT IrpContext,
    IN PHASH Hash,
    IN PHASH ParentHash
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCommonCreate)
#pragma alloc_text(PAGE, NwNormalizeFileNames)
#pragma alloc_text(PAGE, NwOpenByFileId)
#pragma alloc_text(PAGE, NwInitializeFcbFromHash)
#endif


NTSTATUS
NwCommonCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for opening a file called by both the
    Fsp and Fsd threads.  It services IRP_MJ_CREATE.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - This is the status from this open operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFILE_OBJECT FileObject;

    PHASH FoundEntry = NULL;
    PHASH PreviousFoundEntry = NULL;

    PVCB Vcb;

    ULONG DesiredAccess;

    BOOLEAN OpenByFileId;
    BOOLEAN IgnoreCase;
    ULONG CreateDisposition;

    BOOLEAN LockVolume = FALSE;
    BOOLEAN VolumeOpen = FALSE;
    BOOLEAN RootDirOpen = FALSE;

    //
    //  We will be acquiring and releasing file Fcb's as we move down the
    //  directory tree during opens.  At any time we need to know the deepest
    //  point we have traversed down in the tree in case we need to cleanup
    //  any structures created here.
    //
    //  FcbToRelease - represents this point.  If non-null it means we have
    //      acquired it and need to release it in finally clause.
    //
    //  NextFcb - represents the NextFcb to walk to but haven't acquired yet.
    //

    TYPE_OF_OPEN RelatedTypeOfOpen = UnopenedFileObject;
    PFILE_OBJECT RelatedFileObject;
    PCCB RelatedCcb = NULL;

    PCCB Ccb = NULL;
    PFCB NextFcb = NULL;
    PFCB FcbToRelease = NULL;
    ULONG CcbFlags = 0;
    ULONG CurrentDir;
    TYPE_OF_OPEN TypeOfOpen;
    BOOLEAN FileObjectCountBiased = FALSE;

    OEM_STRING RemainingName = {0, 0, NULL};
    OEM_STRING FinalName = {0, 0, NULL};
    PUCHAR RemainingNameBuffer = NULL;

    ULONG OriginalNameLength;
    PWCHAR OriginalNameBuffer = NULL;

    PAGED_CODE();

    //
    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS.
    //

    if (IrpContext->Vcb == NULL) {

        NwCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Get create parameters from the Irp.
    //

    OpenByFileId = BooleanFlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID );
    IgnoreCase = !BooleanFlagOn( IrpSp->Flags, SL_CASE_SENSITIVE );
    CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;
    DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;

    //
    //  Do some preliminary checks to make sure the operation is supported.
    //  We fail in the following cases immediately.
    //
    //      - Open a paging file.
    //      - Open a target directory.
    //      - Open a file with Eas.
    //      - Create a file.
    //
    //  Check that the desired access is legal.
    //

    if (FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE | SL_OPEN_TARGET_DIRECTORY) ||
        (IrpSp->Parameters.Create.EaLength != 0) ||
        NwIllegalDesiredAccess( IrpContext, DesiredAccess ) ||
        (CreateDisposition == FILE_CREATE) ||
        (CreateDisposition == FILE_OVERWRITE) ||
        (CreateDisposition == FILE_OVERWRITE_IF) ||
        (CreateDisposition == FILE_SUPERSEDE)) {

        NwCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Copy the Vcb to a local.  Assume the starting directory is the root.
    //

    Vcb = IrpContext->Vcb;
    NextFcb = Vcb->RootDirFcb;

    //
    //  Reference our input parameters to make things easier
    //

    FileObject = IrpSp->FileObject;
    RelatedFileObject = NULL;

    //
    //  Set up the file object's Vpb pointer in case anything happens.
    //  This will allow us to get a reasonable pop-up.
    //

    if ((FileObject->RelatedFileObject != NULL) && !OpenByFileId) {

        RelatedFileObject = FileObject->RelatedFileObject;
        FileObject->Vpb = RelatedFileObject->Vpb;

        RelatedTypeOfOpen = NwDecodeFileObject( IrpContext, RelatedFileObject, &NextFcb, &RelatedCcb );

        //
        //  Fail the request if this is not a user file object.
        //

        if (RelatedTypeOfOpen < UserVolumeOpen) {

            NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  Set the Ignore case.
    //

    if (IgnoreCase) {

        SetFlag( CcbFlags, CCB_FLAG_IGNORE_CASE );
    }

    //
    //  Check the related Ccb to see if this was an OpenByFileId and
    //  whether there was a version.
    //

    if (ARGUMENT_PRESENT( RelatedCcb )) {

        if (FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_BY_ID | CCB_FLAG_OPEN_RELATIVE_BY_ID )) {

            SetFlag( CcbFlags, CCB_FLAG_OPEN_RELATIVE_BY_ID );
        }
    }

    //
    //  If we haven't initialized the names then make sure the strings are valid.
    //  If this an OpenByFileId then verify the file id buffer.
    //
    //  After this routine returns we know that the full name is in the
    //  FileName buffer.  Any trailing backslash has been removed and the flag
    //  in the IrpContext will indicate whether we removed the backslash.
    //

    Status = NwNormalizeFileNames( IrpContext,
                                   Vcb,
                                   OpenByFileId,
                                   IgnoreCase,
                                   RelatedTypeOfOpen,
                                   &FileObject->FileName );

    //
    //  Return the error code if not successful.
    //

    if (!NT_SUCCESS( Status )) {

        NwCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  Convert the name in the file object into Oem, upcasing if we are
    //  to do a case insensitive create.
    //

    if (FileObject->FileName.Length) {

        if (IgnoreCase) {

            Status = RtlUpcaseUnicodeStringToCountedOemString( &RemainingName,
                                                               &FileObject->FileName,
                                                               TRUE );

        } else {

            Status = RtlUnicodeStringToCountedOemString( &RemainingName,
                                                         &FileObject->FileName,
                                                         TRUE );
        }

        if (!NT_SUCCESS(Status)) {
            NwCompleteRequest( IrpContext, Irp, Status );
            return Status;
        }

        RemainingNameBuffer = RemainingName.Buffer;
    }

    if ((RemainingName.Length == 0) &&
        (RelatedTypeOfOpen <= UserVolumeOpen) &&
        !OpenByFileId) {

        VolumeOpen = TRUE;
    }

    if (((RemainingName.Length == sizeof(UCHAR)) && (RemainingName.Buffer[0] == L'\\')) ||
        ((RemainingName.Length == 0) && RelatedCcb && (RelatedCcb->Fcb == Vcb->RootDirFcb))) {

        RootDirOpen = TRUE;
    }

    //
    //  We want to acquire the Nw resource shared.
    //

    if (VolumeOpen) {
        NwAcquireVcbExclusive( IrpContext, Vcb, FALSE );
    } else {
        NwAcquireVcbShared( IrpContext, Vcb, FALSE );
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify that the Vcb is in a usable condition.
        //

        if (Vcb->VcbCondition != VcbMounted) {
            try_return( Status = STATUS_FILE_INVALID );
        }

        //
        //  If the Vcb is locked then we cannot open another file
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) {
            try_return( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  If we are opening this file by FileId then process this immediately
        //  and exit.
        //

        if (OpenByFileId) {

            //
            //  The only create disposition we allow is OPEN.
            //

            if ((CreateDisposition != FILE_OPEN) &&
                (CreateDisposition != FILE_OPEN_IF)) {

                try_return( Status = STATUS_ACCESS_DENIED );
            }

            //
            //  Make sure we can wait for this request.
            //

            if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

                NwRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            Status = NwOpenByFileId( IrpContext,
                                     IrpSp,
                                     Vcb,
                                     &FcbToRelease );
        } else

        //
        //  If we are opening this volume Dasd then process this immediately
        //  and exit.
        //

        if (VolumeOpen) {

            //
            //  The only create disposition we allow is OPEN.
            //

            if ((CreateDisposition != FILE_OPEN) &&
                (CreateDisposition != FILE_OPEN_IF)) {

                try_return( Status = STATUS_ACCESS_DENIED );
            }

            //
            //  If they wanted to open a directory, surprise.
            //

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

                try_return( Status = STATUS_NOT_A_DIRECTORY );
            }

            //
            //  Acquire the Fcb first.
            //

            FcbToRelease = Vcb->VolumeDasdFcb;
            NwAcquireFcbExclusive( IrpContext, FcbToRelease, FALSE );

            //
            //  If this a volume open and the user wants to lock the volume then
            //  purge and lock the volume.
            //

            if (!FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_READ)) {

                //
                //  If there are open handles then fail this immediately.
                //

                if (Vcb->FileHandleCount != 0) {

                    try_return( Status = STATUS_SHARING_VIOLATION );
                }

                //
                //  If we can't wait then force this to be posted.
                //

                if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

                    NwRaiseStatus( IrpContext, STATUS_CANT_WAIT );
                }

                //
                //  Purge the volume and make sure all of the user references
                //  are gone.
                //

                Status = NwPurgeVolume( IrpContext, Vcb );

                if (Status != STATUS_SUCCESS) {

                    try_return( Status );
                }

                if (Vcb->FcbCount > NWFS_RESIDUAL_FCB_COUNT) {

                    try_return( Status = STATUS_SHARING_VIOLATION );
                }

                LockVolume = TRUE;
            }

            if (FcbToRelease->FileHandleCount != 0) {
                Status = IoCheckShareAccess( DesiredAccess,
                                             IrpSp->Parameters.Create.ShareAccess,
                                             IrpSp->FileObject,
                                             &FcbToRelease->ShareAccess,
                                             FALSE );

                if (!NT_SUCCESS( Status )) {
                    try_return( Status );
                }
            }

            TypeOfOpen = UserVolumeOpen;

        } else

        //
        //  Test for a root dir and handle it immediately.
        //

        if (RootDirOpen) {

            //
            //  The only create disposition we allow is OPEN.
            //

            if ((CreateDisposition != FILE_OPEN) &&
                (CreateDisposition != FILE_OPEN_IF)) {

                try_return( Status = STATUS_ACCESS_DENIED );
            }

            //
            //  This is a directory.  Verify the user didn't want to open
            //  as a file.
            //

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE )) {

                try_return( Status = STATUS_FILE_IS_A_DIRECTORY );
            }

            //
            //  Acquire the Fcb first.
            //

            FcbToRelease = Vcb->RootDirFcb;
            NwAcquireFcbExclusive( IrpContext, FcbToRelease, FALSE );

            if (FcbToRelease->FileHandleCount != 0) {
                Status = IoCheckShareAccess( DesiredAccess,
                                             IrpSp->Parameters.Create.ShareAccess,
                                             IrpSp->FileObject,
                                             &FcbToRelease->ShareAccess,
                                             FALSE );

                if (!NT_SUCCESS( Status )) {
                    try_return( Status );
                }
            }

            TypeOfOpen = UserDirectoryOpen;

        } else {

            //
            //  Compute how much memory is needed to store the entire name.
            //

            OriginalNameLength = FileObject->FileName.Length;

            if (RelatedCcb != NULL) {
                OriginalNameLength += RelatedCcb->OriginalName.Length + sizeof(WCHAR);
            }

            OriginalNameBuffer = FsRtlAllocatePoolWithTag( NwPagedPool,
                                                           OriginalNameLength,
                                                           TAG_FILE_NAME );

            //
            //  Do a search if there is more of the name to parse.
            //

            CurrentDir = NextFcb->DirNumber;
            FoundEntry = NextFcb->Hash;

            //
            //  If this is an absolute path, get rid of the leading backslash.
            //

            if (RemainingName.Length && (RemainingName.Buffer[0] == L'\\')) {
                RemainingName.Buffer++;
                RemainingName.Length -= sizeof(UCHAR);
            }

            while (RemainingName.Length != 0) {

                //
                //  Split off the next component from the name.
                //

                NwDissectName( IrpContext,
                               &RemainingName,
                               &FinalName );

                //
                //  Go ahead and look this entry up in the hash table.
                //

                PreviousFoundEntry = FoundEntry;

                FoundEntry = HashFindFile( Vcb->VolumeNumber,
                                           CurrentDir,
                                           &FinalName,
                                           IgnoreCase );

                if (FoundEntry) {
                    FoundEntry = FoundEntry->nlroot;
                }

                //
                //  If we didn't find the name, or there is more name and this is
                //  not a directory, this path could not be cracked.
                //

                if (!FoundEntry ||
                    ((RemainingName.Length != 0) &&
                     (!FlagOn(FoundEntry->FileAttributes, NETWARE_ATTR_DIRECTORY)))) {

                    Status = (RemainingName.Length == 0) ?
                             STATUS_OBJECT_NAME_NOT_FOUND :
                             STATUS_OBJECT_PATH_NOT_FOUND;

                    try_return( Status );
                }

                if (RemainingName.Length == 0) {
                    break;
                }

                CurrentDir = FoundEntry->DirNo;
            }

            //
            //  Make some checks in if this is a directory or not.
            //

            if (FlagOn(FoundEntry->FileAttributes, NETWARE_ATTR_DIRECTORY)) {

                //
                //  If this is a directory, verify the user didn't want to open
                //  as a file.
                //

                if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE )) {
                    try_return( Status = STATUS_FILE_IS_A_DIRECTORY );
                }

                TypeOfOpen = UserDirectoryOpen;

            } else {

                //
                //  This is a file. Verify the user didn't want to open a directory.
                //

                if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TRAIL_BACKSLASH ) ||
                    FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

                    try_return( Status = STATUS_NOT_A_DIRECTORY );
                }

                TypeOfOpen = UserFileOpen;
            }

            //
            //  The only create disposition we allow is OPEN.
            //

            if ((CreateDisposition != FILE_OPEN) &&
                (CreateDisposition != FILE_OPEN_IF)) {

                try_return( Status = STATUS_ACCESS_DENIED );
            }

            //
            //  OK, go ahead and open the file or directory.  This is a little
            //  complicated because we cannot hold the NwLock mutex while
            //  creating an Fcb (FoundEntry->FSContext == NULL case), so once
            //  we create it we need to check if another thread may have beat
            //  us to the gate.
            //

FcbAppeared:
            NwLock();

            if (FoundEntry->FSContext) {

                FcbToRelease = FoundEntry->FSContext;

                if (FlagOn(FcbToRelease->FcbState, FCB_STATE_CLOSE_DELAYED)) {

                    ASSERT(FcbToRelease->FileObjectCount == 0);

                    ClearFlag(FcbToRelease->FcbState, FCB_STATE_CLOSE_DELAYED);
                    RemoveEntryList( &FcbToRelease->DelayedCloseLinks );
                    NwData.DelayedCloseCount -= 1;
                }

                NwIncrementObjectCounts( IrpContext, FcbToRelease );
                FileObjectCountBiased = TRUE;

                NwUnlock();

                //
                //  Acquire the Fcb first.
                //

                NwAcquireFcbExclusive( IrpContext, FcbToRelease, FALSE );

                if (FcbToRelease->FileHandleCount != 0) {

                    if ((TypeOfOpen == UserFileOpen) &&
                        FsRtlCurrentBatchOplock( &FcbToRelease->Oplock )) {

                        //
                        //  We remember if a batch oplock break is underway for the
                        //  case where the sharing check fails.
                        //

                        Irp->IoStatus.Information = FILE_OPBATCH_BREAK_UNDERWAY;

                        Status = FsRtlCheckOplock( &FcbToRelease->Oplock,
                                                   IrpContext->Irp,
                                                   IrpContext,
                                                   NwOplockComplete,
                                                   NwPrePostIrp );

                        if ((Status != STATUS_SUCCESS) &&
                            (Status != STATUS_OPLOCK_BREAK_IN_PROGRESS)) {

                            try_return( Status = STATUS_PENDING );
                        }
                    }

                    //
                    //  Check the share access before breaking any exclusive oplocks.
                    //

                    Status = IoCheckShareAccess( DesiredAccess,
                                                 IrpSp->Parameters.Create.ShareAccess,
                                                 IrpSp->FileObject,
                                                 &FcbToRelease->ShareAccess,
                                                 FALSE );

                    if (!NT_SUCCESS( Status )) {
                        try_return( Status );
                    }

                    //
                    //  Now check that we can continue based on the oplock state of the
                    //  file.
                    //

                    if (TypeOfOpen == UserFileOpen) {

                        Status = FsRtlCheckOplock( &FcbToRelease->Oplock,
                                                   IrpContext->Irp,
                                                   IrpContext,
                                                   NwOplockComplete,
                                                   NwPrePostIrp );

                        if ((Status != STATUS_SUCCESS) &&
                            (Status != STATUS_OPLOCK_BREAK_IN_PROGRESS)) {
                            try_return( Status = STATUS_PENDING );
                        }
                    }
                }

            } else {

                NwUnlock();

                FcbToRelease = NwInitializeFcbFromHash( IrpContext, FoundEntry, PreviousFoundEntry );

                //
                //  Acquire the Fcb first.
                //

                NwAcquireFcbExclusive( IrpContext, FcbToRelease, FALSE );

                NwLock();

                if (FoundEntry->FSContext == NULL) {
                    FoundEntry->FSContext = FcbToRelease;
                } else {
                    NwUnlinkFcb( FcbToRelease );
                    NwUnlock();
                    NwReleaseFcb( IrpContext, FcbToRelease );
                    NwDeleteFcb( FcbToRelease );
                    FcbToRelease = NULL;
                    goto FcbAppeared;
                }

                NwUnlock();
            }
        }

        ASSERT((Status == STATUS_SUCCESS) ||
               (Status == STATUS_OPLOCK_BREAK_IN_PROGRESS));

        //
        //  Create the Ccb now.  After this point, nothing can fail.
        //

        Ccb = NwCreateCcb( IrpContext, FcbToRelease, CcbFlags );

        //
        //  Update or set the share access.
        //

        if (FcbToRelease->FileHandleCount == 0) {

            IoSetShareAccess( DesiredAccess,
                              IrpSp->Parameters.Create.ShareAccess,
                              IrpSp->FileObject,
                              &FcbToRelease->ShareAccess );

        } else {

            IoUpdateShareAccess( IrpSp->FileObject, &FcbToRelease->ShareAccess );
        }

        //
        //  Set the file object type.
        //

        NwSetFileObject( IrpContext, IrpSp->FileObject, TypeOfOpen, FcbToRelease, Ccb );

        //
        //  Set the appropriate cache flags for a user file object.
        //

        if (TypeOfOpen == UserFileOpen) {

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_INTERMEDIATE_BUFFERING )) {

                SetFlag( IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING );

            } else {

                SetFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );
            }
        }

        //
        //  If this worked, store the full original path name in the CCB.
        //

        if (OriginalNameBuffer) {

            PWCHAR DestNamePointer;

            Ccb->OriginalName.Length =
            Ccb->OriginalName.MaximumLength = (USHORT)OriginalNameLength;
            Ccb->OriginalName.Buffer = OriginalNameBuffer;

            DestNamePointer = OriginalNameBuffer;

            if (RelatedCcb != NULL) {
                RtlCopyMemory(DestNamePointer,
                              RelatedCcb->OriginalName.Buffer,
                              RelatedCcb->OriginalName.Length);

                DestNamePointer += RelatedCcb->OriginalName.Length/sizeof(WCHAR);
                *DestNamePointer++ = L'\\';
            }

            RtlCopyMemory(DestNamePointer,
                          FileObject->FileName.Buffer,
                          FileObject->FileName.Length);
        }

        //
        //  Update the open and cleanup counts.  Check the fast io state here.
        //

        NwLock();

        if (!FileObjectCountBiased) {
            NwIncrementObjectCounts( IrpContext, FcbToRelease );
        }
        NwIncrementHandleCounts( IrpContext, FcbToRelease );

        NwUnlock();

        FcbToRelease->IsFastIoPossible = (TypeOfOpen == UserFileOpen) ?
                                         NwIsFastIoPossible( FcbToRelease ) :
                                         FastIoIsNotPossible;

        if (LockVolume) {
            Vcb->VolumeLockFileObject = IrpSp->FileObject;
            SetFlag( Vcb->Vpb->Flags, VPB_LOCKED );
            SetFlag( Vcb->VcbState, VCB_STATE_LOCKED );
        }

        //
        //  Show that we opened the file.
        //

        Irp->IoStatus.Information = FILE_OPENED;

    try_exit:  NOTHING;

    } finally {

        //
        //  Free the remaining name is it was allocated.
        //

        if (RemainingNameBuffer) {
            ExFreePool( RemainingNameBuffer );
        }

        //
        //  The result of this open could be success, pending or some error
        //  condition.
        //
        //  There are two specifc peculiar values we are concerned about, both
        //  NT_SUCCESS (i.e., > 0), but we take opposite paths.  If we receive
        //  STATUS_OPLOCK_BREAK_IN_PROGRESS (0x108), this open actually succeded,
        //  and we should proceed as such.  If we received STATUS_PENDING (0x103),
        //  we are going to restart this request in our FSP thread or oplock
        //  handler, and as such should treat it as an error and tear down all
        //  our structures.
        //

        if (AbnormalTermination() || !NT_SUCCESS(Status) || (Status == STATUS_PENDING)) {

            if (OriginalNameBuffer) {
                ASSERT( Ccb == NULL ); // Ccb and OriginalNameBuffer should be mutually exclusive
                ExFreePool( OriginalNameBuffer );
            }

            if (FcbToRelease) {

                if (Ccb) {
                    ASSERT( OriginalNameBuffer == NULL );
                    NwDeleteCcb( IrpContext, Ccb );
                }

                NwLock();

                if (FileObjectCountBiased) {
                    NwDecrementObjectCounts( FcbToRelease );
                }

                if (FcbToRelease->FileObjectCount == 0) {
                    NwUnlinkFcb( FcbToRelease );
                    NwUnlock();
                    NwReleaseFcb( IrpContext, FcbToRelease );
                    NwDeleteFcb( FcbToRelease );
                } else {
                    NwUnlock();
                    NwReleaseFcb( IrpContext, FcbToRelease );
                }
            }

            //
            //  If we posted this request through the oplock package we need
            //  to show that there is no reason to complete the request.  If
            //  exception was raised, the packet will be completed in
            //  NwProcessException() above us.
            //

            if (AbnormalTermination() || (Status == STATUS_PENDING)) {

                IrpContext = NULL;
                Irp = NULL;
            }

        } else {

            //
            //  Release the Current Fcb if still acquired.
            //

            if (FcbToRelease != NULL) {
                NwReleaseFcb( IrpContext, FcbToRelease );
            }
        }

        //
        //  Release the Vcb.
        //

        NwReleaseVcb( IrpContext, Vcb );

        //
        //  Call our completion routine.  It will handle the case where either
        //  the Irp and/or IrpContext are gone.
        //

        NwCompleteRequest( IrpContext, Irp, Status );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NwNormalizeFileNames (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN OpenByFileId,
    IN BOOLEAN IgnoreCase,
    IN TYPE_OF_OPEN RelatedTypeOfOpen,
    IN OUT PUNICODE_STRING FileName
    )

/*++

Routine Description:

    This routine is called to store the full name into the filename buffer.
    We also check for a trailing backslash and lead-in double backslashes.
    This routine also verifies the mode of the related open against the name
    currently in the

Arguments:

    Vcb - Vcb for this volume.

    OpenByFileId - Indicates if the filename should be a 64 bit FileId.

    IgnoreCase - Indicates if this open is a case-insensitive operation.

    RelatedTypeOfOpen - Indicates the type of the related file object.

    FileName - FileName to update in this routine.  The name should
        either be a 64-bit FileId or a Unicode string.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the names are OK, appropriate error code
        otherwise.

--*/

{
    ULONG RemainingNameLength;
    ULONG RelatedNameLength = 0;
    ULONG SeparatorLength = 0;

    ULONG BufferLength;

    UNICODE_STRING NewFileName;

    PAGED_CODE();

    //
    //  Deal with the regular file name case first.
    //

    if (!OpenByFileId) {

        //
        //  Hack: Win32 sends us double backslash.
        //

        if ((FileName->Length > sizeof( WCHAR )) &&
            (FileName->Buffer[1] == L'\\') &&
            (FileName->Buffer[0] == L'\\')) {

            //
            //  If there are still two beginning backslashes, the name is bogus.
            //

            if ((FileName->Length > 2 * sizeof( WCHAR )) &&
                (FileName->Buffer[2] == L'\\')) {

                return STATUS_OBJECT_NAME_INVALID;
            }

            //
            //  Slide the name down in the buffer.
            //

            RtlMoveMemory( FileName->Buffer,
                           FileName->Buffer + 1,
                           FileName->Length );

            FileName->Length -= sizeof( WCHAR );
        }

        //
        //  Check for a trailing backslash.  Don't strip off if only character
        //  in the full name or for relative opens where this is illegal.
        //

        if (((FileName->Length > sizeof( WCHAR)) ||
             ((FileName->Length == sizeof( WCHAR )) && (RelatedTypeOfOpen == UserDirectoryOpen))) &&
            (FileName->Buffer[ (FileName->Length/2) - 1 ] == L'\\')) {

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_TRAIL_BACKSLASH );
            FileName->Length -= sizeof( WCHAR );
        }

        //
        //  If this is a related file object then we verify the compatibility
        //  of the name in the file object with the relative file object.
        //

        if (RelatedTypeOfOpen != UnopenedFileObject) {

            //
            //  If the filename length was zero then it must be legal.
            //  If there are characters then check with the related
            //  type of open.
            //

            if (FileName->Length != 0) {

                //
                //  The name length must always be zero for a volume open.
                //

                if (RelatedTypeOfOpen == UserVolumeOpen) {

                    return STATUS_INVALID_PARAMETER;

                //
                //  The remaining name cannot begin with a backslash.
                //

                } else if (FileName->Buffer[0] == L'\\' ) {

                    return STATUS_INVALID_PARAMETER;

                //
                //  If the related file is a user file then there
                //  is no file with this path.
                //

                } else if (RelatedTypeOfOpen == UserFileOpen) {

                    return STATUS_OBJECT_PATH_NOT_FOUND;
                }
            }


        //
        //  The full name is already in the FileObject.  It must either
        //  be length 0 or begin with a backslash.
        //

        } else if (FileName->Length != 0) {

            if (FileName->Buffer[0] != L'\\') {

                return STATUS_INVALID_PARAMETER;
            }
        }

        //
        //  Do a quick check to make sure there are no wildcards.
        //

        if (FsRtlDoesNameContainWildCards( FileName )) {

            return STATUS_OBJECT_NAME_INVALID;
        }

    //
    //  For the open by file Id case we verify the name really contains
    //  a 64 bit value.
    //

    } else {

        //
        //  Check for validity of the buffer.
        //

        if (FileName->Length != sizeof( FILE_ID )) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
NwOpenByFileId (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *FcbToRelease
    )

/*++

Routine Description:

    This routine is called to open a file by the FileId.  The file Id is in
    the FileObject name buffer and has been verified to be 64 bits.

    The Netware FS doesn't curently support open by ID.
    
Arguments:

    IrpSp - Stack location within the create Irp.

    Vcb - Vcb for this volume.

    FcbToRelease - Address to store the Fcb for this open.  We only store the
        FcbToRelease here when we have acquired it so our caller knows to
        free or deallocate it.

Return Value:

    NTSTATUS - Status indicating the result of the operation.

--*/

{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    return Status;
}

//
//  Local support routine
//

PFCB
NwInitializeFcbFromHash (
    IN PIRP_CONTEXT IrpContext,
    IN PHASH Hash,
    IN PHASH ParentHash
    )

/*++

Routine Description:

    This routine is basically responsible for creating a new Fcb and filling
    it in from the values in the Hash entry for the file or directory.

Arguments:

    Hash - The hash entry for a file or directory.

    ParentHash - It's parent's hash entry.

Return Value:

    PFCB - A pointer to the newly created and initialized Fcb.

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    LARGE_INTEGER GmtTime;
    PFCB NewFcb;
    FILE_ID FileId;
    ENUM_CONTEXT Entry = {0};

    FileId.QuadPart = Hash->DirNo;

    //
    //  First, create the actual Fcb.
    //

    NewFcb = NwCreateFcb ( IrpContext, FileId );

    NewFcb->Hash = Hash;
    NewFcb->ParentHash = ParentHash;

    //
    //  Call NwSetEnumContext() to get the data we need from the
    //  Hash.  Note that we don't allocate a long name buffer
    //  since we don't need to know what it is.
    //

    Entry.AlternateFileName.Buffer = Entry.AlternateFileNameBuffer;
    Entry.AlternateFileName.MaximumLength = 12 * sizeof(WCHAR);

    NwSetEnumContext( IrpContext, Hash, &Entry, Hash->SlotNumber );

    //
    //  Fill in the time stamps.  If the conversion fails, the entries
    //  are not filled in (i.e. contain zero).
    //

    if (RtlTimeFieldsToTime(&Entry.CreationTime, &GmtTime)) {
        ExLocalTimeToSystemTime( &GmtTime, &NewFcb->CreationTime );
    }

    if (RtlTimeFieldsToTime(&Entry.LastWriteTime, &GmtTime)) {
        ExLocalTimeToSystemTime( &GmtTime, &NewFcb->LastWriteTime );
    }

    if (RtlTimeFieldsToTime(&Entry.LastAccessTime, &GmtTime)) {
        ExLocalTimeToSystemTime( &GmtTime, &NewFcb->LastAccessTime );
    }

    //
    //  Set the sizes separately for directories and files.
    //

    if (FlagOn( Entry.FileAttributes, NETWARE_ATTR_DIRECTORY )) {

        NewFcb->FileSize.QuadPart =
        NewFcb->ValidDataLength.QuadPart =
        NewFcb->AllocationSize.QuadPart = 0;

    } else {

        NewFcb->FileSize.LowPart = Entry.FileSize;
        NewFcb->ValidDataLength.LowPart = Entry.FileSize;
        NewFcb->AllocationSize.LowPart = SectorAlign( Entry.FileSize );
    }

    //
    //  All Netware files are readonly.  We also copy the existence
    //  bit to the hidden attribute.
    //

    NewFcb->FileAttributes = Entry.FileAttributes;

    if (Entry.AlternateFileName.Length) {

        NewFcb->ShortName.MaximumLength = 12*sizeof(WCHAR);
        NewFcb->ShortName.Length = Entry.AlternateFileName.Length;
        NewFcb->ShortName.Buffer = NewFcb->ShortNameBuffer;

        RtlCopyMemory( NewFcb->ShortName.Buffer,
                       Entry.AlternateFileName.Buffer,
                       Entry.AlternateFileName.Length );
    }

    return NewFcb;
}



