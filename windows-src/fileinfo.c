
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

    FileInfo.c

Abstract:

    This module implements the File Information routines for Nwfs called by
    the Fsd/Fsp dispatch drivers.

  Authors:

    Jeff Merkey (jmerkey@timpanogas.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

****************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_FILEINFO)

//
//  Local support routines
//

VOID
NwQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NwQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NwQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NwQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NwQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NwQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NwQueryAlternateNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NwQueryNetworkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN OUT PULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwCommonQueryInfo)
#pragma alloc_text(PAGE, NwCommonSetInfo)
#pragma alloc_text(PAGE, NwFastQueryBasicInfo)
#pragma alloc_text(PAGE, NwFastQueryStdInfo)
#pragma alloc_text(PAGE, NwFastQueryNetworkInfo)
#pragma alloc_text(PAGE, NwQueryAlternateNameInfo)
#pragma alloc_text(PAGE, NwQueryBasicInfo)
#pragma alloc_text(PAGE, NwQueryEaInfo)
#pragma alloc_text(PAGE, NwQueryInternalInfo)
#pragma alloc_text(PAGE, NwQueryNameInfo)
#pragma alloc_text(PAGE, NwQueryNetworkInfo)
#pragma alloc_text(PAGE, NwQueryPositionInfo)
#pragma alloc_text(PAGE, NwQueryStandardInfo)
#endif


NTSTATUS
NwCommonQueryInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for query file information called by both the
    fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    NTSTATUS - The return status for this operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    ULONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
    PFILE_ALL_INFORMATION Buffer;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN ReleaseFcb = FALSE;

    PAGED_CODE();

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryFile.Length;
    FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Decode the file object
    //

    TypeOfOpen = NwDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We only support query on file and directory handles.
        //

        switch (TypeOfOpen) {

        case UserDirectoryOpen :
        case UserFileOpen :

            //
            //  Acquire shared access to this file.
            //

            NwAcquireFcbShared( IrpContext, Fcb, FALSE );
            ReleaseFcb = TRUE;

            //
            //  Verify that the Vcb is in a usable condition.
            //
    
            if (Fcb->Vcb->VcbCondition != VcbMounted) {
                try_return( Status = STATUS_FILE_INVALID );
            }

            //
            //  Based on the information class we'll do different
            //  actions.  Each of the procedures that we're calling fills
            //  up the output buffer, if possible.  They will raise the
            //  status STATUS_BUFFER_OVERFLOW for an insufficient buffer.
            //  This is considered a somewhat unusual case and is handled
            //  more cleanly with the exception mechanism rather than
            //  testing a return status value for each call.
            //

            switch (FileInformationClass) {

            case FileAllInformation:

                //
                //  We don't allow this operation on a file opened by file Id.
                //

                if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                //
                //  In this case go ahead and call the individual routines to
                //  fill in the buffer.  Only the name routine will
                //  pointer to the output buffer and then call the
                //  individual routines to fill in the buffer.
                //

                Length -= (sizeof( FILE_ACCESS_INFORMATION ) +
                           sizeof( FILE_MODE_INFORMATION ) +
                           sizeof( FILE_ALIGNMENT_INFORMATION ));

                NwQueryBasicInfo( IrpContext, Fcb, &Buffer->BasicInformation, &Length );
                NwQueryStandardInfo( IrpContext, Fcb, &Buffer->StandardInformation, &Length );
                NwQueryInternalInfo( IrpContext, Fcb, &Buffer->InternalInformation, &Length );
                NwQueryEaInfo( IrpContext, Fcb, &Buffer->EaInformation, &Length );
                NwQueryPositionInfo( IrpContext, IrpSp->FileObject, &Buffer->PositionInformation, &Length );
                Status = NwQueryNameInfo( IrpContext, Ccb, &Buffer->NameInformation, &Length );

                break;

            case FileBasicInformation:

                NwQueryBasicInfo( IrpContext, Fcb, (PFILE_BASIC_INFORMATION) Buffer, &Length );
                break;

            case FileStandardInformation:

                NwQueryStandardInfo( IrpContext, Fcb, (PFILE_STANDARD_INFORMATION) Buffer, &Length );
                break;

            case FileInternalInformation:

                NwQueryInternalInfo( IrpContext, Fcb, (PFILE_INTERNAL_INFORMATION) Buffer, &Length );
                break;

            case FileEaInformation:

                NwQueryEaInfo( IrpContext, Fcb, (PFILE_EA_INFORMATION) Buffer, &Length );
                break;

            case FilePositionInformation:

                NwQueryPositionInfo( IrpContext, IrpSp->FileObject, (PFILE_POSITION_INFORMATION) Buffer, &Length );
                break;

            case FileNameInformation:

                //
                //  We don't allow this operation on a file opened by file Id.
                //

                if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    Status = NwQueryNameInfo( IrpContext, Ccb, (PFILE_NAME_INFORMATION) Buffer, &Length );

                } else {

                    Status = STATUS_INVALID_PARAMETER;
                }

                break;

            case FileAlternateNameInformation:

                if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    Status = NwQueryAlternateNameInfo( IrpContext, Fcb, (PFILE_NAME_INFORMATION) Buffer, &Length );

                } else {

                    Status = STATUS_INVALID_PARAMETER;
                }

                break;

            case FileNetworkOpenInformation:

                NwQueryNetworkInfo( IrpContext, Fcb, (PFILE_NETWORK_OPEN_INFORMATION) Buffer, &Length );
                break;

            default :

                Status = STATUS_INVALID_PARAMETER;
            }

            break;

        default :

            Status = STATUS_INVALID_PARAMETER;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //  and then complete the request
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - Length;

    try_exit: NOTHING;
    } finally {

        //
        //  Release the file.
        //

        if (ReleaseFcb) {

            NwReleaseFcb( IrpContext, Fcb );
        }
    }

    //
    //  Complete the request if we didn't raise.
    //

    NwCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


NTSTATUS
NwCommonSetInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for set file information called by both the
    fsd and fsp threads.  None of these are suported NWFSRO.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    NTSTATUS - The return status for this operation.

--*/

{
    NwCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

    return STATUS_INVALID_PARAMETER;
}

BOOLEAN
NwFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for basic file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Result = FALSE;
    TYPE_OF_OPEN TypeOfOpen;

    PFCB Fcb;

    PAGED_CODE();

    ASSERT_FILE_OBJECT( FileObject );

    FsRtlEnterFileSystem();

    //
    //  Decode the file object to find the type of open and the data
    //  structures.
    //

    TypeOfOpen = NwFastDecodeFileObject( FileObject, &Fcb );

    //
    //  We only support this request on user file or directory objects.
    //

    if ((TypeOfOpen != UserFileOpen) &&
        ((TypeOfOpen != UserDirectoryOpen) || !FlagOn( Fcb->FcbState, FCB_STATE_INITIALIZED))) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Acquire the file shared to access the Fcb.
    //

    if (!ExAcquireResourceShared( Fcb->Resource, Wait )) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Only deal with mounted volumes.
        //

        if (Fcb->Vcb->VcbCondition == VcbMounted) {

            //
            //  Fill in the input buffer from the Fcb fields.
            //

            Buffer->CreationTime.QuadPart = Fcb->CreationTime.QuadPart;
            Buffer->LastWriteTime.QuadPart =
            Buffer->ChangeTime.QuadPart = Fcb->LastWriteTime.QuadPart;
            Buffer->LastAccessTime.QuadPart = Fcb->LastAccessTime.QuadPart;

            Buffer->FileAttributes = Fcb->FileAttributes;

            //
            //  Update the IoStatus block with the size of this data.
            //

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( FILE_BASIC_INFORMATION );

            Result = TRUE;
        }

    } finally {

        ExReleaseResource( Fcb->Resource );

        FsRtlExitFileSystem();
    }

    return Result;
}


BOOLEAN
NwFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for standard file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Result = FALSE;
    TYPE_OF_OPEN TypeOfOpen;

    PFCB Fcb;

    PAGED_CODE();

    ASSERT_FILE_OBJECT( FileObject );

    FsRtlEnterFileSystem();

    //
    //  Decode the file object to find the type of open and the data
    //  structures.
    //

    TypeOfOpen = NwFastDecodeFileObject( FileObject, &Fcb );

    //
    //  We only support this request on initialized user file or directory objects.
    //

    if ((TypeOfOpen != UserFileOpen) &&
        ((TypeOfOpen != UserDirectoryOpen) || !FlagOn( Fcb->FcbState, FCB_STATE_INITIALIZED ))) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Acquire the file shared to access the Fcb.
    //

    if (!ExAcquireResourceShared( Fcb->Resource, Wait )) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Only deal with mounted volumes.
        //

        if (Fcb->Vcb->VcbCondition == VcbMounted) {

            //
            //  Check whether this is a directory.
            //

            if (FlagOn( Fcb->FileAttributes, NETWARE_ATTR_DIRECTORY )) {

                Buffer->AllocationSize.QuadPart =
                Buffer->EndOfFile.QuadPart = 0;

                Buffer->Directory = TRUE;

            } else {

                Buffer->AllocationSize.QuadPart = Fcb->AllocationSize.QuadPart;
                Buffer->EndOfFile.QuadPart = Fcb->FileSize.QuadPart;

                Buffer->Directory = FALSE;
            }

            Buffer->NumberOfLinks = Fcb->NumberOfLinks;
            Buffer->DeletePending = FALSE;

            //
            //  Update the IoStatus block with the size of this data.
            //

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( FILE_STANDARD_INFORMATION );

            Result = TRUE;
        }

    } finally {

        ExReleaseResource( Fcb->Resource );

        FsRtlExitFileSystem();
    }

    return Result;
}


BOOLEAN
NwFastQueryNetworkInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for network file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Result = FALSE;
    TYPE_OF_OPEN TypeOfOpen;

    PFCB Fcb;

    PAGED_CODE();

    ASSERT_FILE_OBJECT( FileObject );

    FsRtlEnterFileSystem();

    //
    //  Decode the file object to find the type of open and the data
    //  structures.
    //

    TypeOfOpen = NwFastDecodeFileObject( FileObject, &Fcb );

    //
    //  We only support this request on user file or directory objects.
    //

    if ((TypeOfOpen != UserFileOpen) &&
        ((TypeOfOpen != UserDirectoryOpen) || !FlagOn( Fcb->FcbState, FCB_STATE_INITIALIZED))) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Acquire the file shared to access the Fcb.
    //

    if (!ExAcquireResourceShared( Fcb->Resource, Wait )) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Only deal with mounted volumes.
        //

        if (Fcb->Vcb->VcbCondition == VcbMounted) {

            //
            //  Fill in the input buffer from the Fcb fields.
            //

            Buffer->CreationTime.QuadPart = Fcb->CreationTime.QuadPart;
            Buffer->LastWriteTime.QuadPart =
            Buffer->ChangeTime.QuadPart = Fcb->LastWriteTime.QuadPart;
            Buffer->LastAccessTime.QuadPart = Fcb->LastAccessTime.QuadPart;

            Buffer->FileAttributes = Fcb->FileAttributes;

            //
            //  Check whether this is a directory.
            //

            if (FlagOn( Fcb->FileAttributes, NETWARE_ATTR_DIRECTORY )) {

                Buffer->AllocationSize.QuadPart =
                Buffer->EndOfFile.QuadPart = 0;

            } else {

                Buffer->AllocationSize.QuadPart = Fcb->AllocationSize.QuadPart;
                Buffer->EndOfFile.QuadPart = Fcb->FileSize.QuadPart;
            }

            //
            //  Update the IoStatus block with the size of this data.
            //

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( FILE_NETWORK_OPEN_INFORMATION );

            Result = TRUE;
        }

    } finally {

        ExReleaseResource( Fcb->Resource );

        FsRtlExitFileSystem();
    }

    return Result;
}


//
//  Local support routine
//

VOID
NwQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

 Description:

    This routine performs the query basic information function for Nwfs

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Fill in the input buffer from the Fcb fields.
    //

    Buffer->CreationTime.QuadPart = Fcb->CreationTime.QuadPart;
    Buffer->LastWriteTime.QuadPart =
    Buffer->ChangeTime.QuadPart = Fcb->LastWriteTime.QuadPart;
    Buffer->LastAccessTime.QuadPart = Fcb->LastAccessTime.QuadPart;

    Buffer->FileAttributes = Fcb->FileAttributes;

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_BASIC_INFORMATION );

    return;
}


//
//  Local support routine
//

VOID
NwQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length
    )
/*++

Routine Description:

    This routine performs the query standard information function for nwfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  There is only one link and delete is never pending on an NWFSRO file.
    //

    Buffer->NumberOfLinks = 1;
    Buffer->DeletePending = FALSE;

    //
    //  We get the sizes from the header.  Return a size of zero
    //  for all directories.
    //

    if (FlagOn( Fcb->FileAttributes, NETWARE_ATTR_DIRECTORY )) {

        Buffer->AllocationSize.QuadPart =
        Buffer->EndOfFile.QuadPart = 0;

        Buffer->Directory = TRUE;

    } else {

        Buffer->AllocationSize.QuadPart = Fcb->AllocationSize.QuadPart;
        Buffer->EndOfFile.QuadPart = Fcb->FileSize.QuadPart;

        Buffer->Directory = FALSE;
    }

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_STANDARD_INFORMATION );

    return;
}


//
//  Local support routine
//

VOID
NwQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query internal information function for nwfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Index number is the file Id number in the Fcb.
    //

    Buffer->IndexNumber = Fcb->FileId;
    *Length -= sizeof( FILE_INTERNAL_INFORMATION );

    return;
}


//
//  Local support routine
//

VOID
NwQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query Ea information function for nwfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  No Ea's on Nwfs volumes.
    //

    Buffer->EaSize = 0;
    *Length -= sizeof( FILE_EA_INFORMATION );

    return;
}


//
//  Local support routine
//

VOID
NwQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query position information function for nwfs.

Arguments:

    FileObject - Supplies the File object being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Get the current position found in the file object.
    //

    Buffer->CurrentByteOffset = FileObject->CurrentByteOffset;

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_POSITION_INFORMATION );

    return;
}


//
//  Local support routine
//

NTSTATUS
NwQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query name information function for nwfs.

Arguments:

    FileObject - Supplies the file object containing the name.

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    NTSTATUS - STATUS_BUFFER_OVERFLOW if the entire name can't be copied.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthToCopy;
    PAGED_CODE();

    //
    //  If this a Unicode disk then simply copy the name in the file object to the
    //  user's buffer.
    //

    LengthToCopy =
    Buffer->FileNameLength = Ccb->OriginalName.Length;

    *Length -= sizeof(ULONG);

    if (LengthToCopy > *Length) {

        LengthToCopy = *Length;
        Status = STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory( Buffer->FileName, Ccb->OriginalName.Buffer, LengthToCopy );

    //
    //  Reduce the available bytes by the amount stored into this buffer.
    //

    *Length -= LengthToCopy;

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NwQueryAlternateNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query alternate name information function.
    We lookup the dirent for this file and then check if there is a
    short name.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified.

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned.

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the whole name would fit into the user buffer,
               STATUS_OBJECT_NAME_NOT_FOUND if we can't return the name,
               STATUS_BUFFER_OVERFLOW otherwise.

--*/

{
    NTSTATUS Status;
    ULONG BytesToCopy;

    *Length -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

    if (Fcb->ShortName.Length == 0) {
        Buffer->FileNameLength = 0;
        return STATUS_SUCCESS;
    }

    //
    //  If we overflow, return STATUS_BUFFER_OVERFLOW.
    //

    if (*Length < Fcb->ShortName.Length) {

        BytesToCopy = *Length;
        Status = STATUS_BUFFER_OVERFLOW;

    } else {

        BytesToCopy = Fcb->ShortName.Length;
        Status = STATUS_SUCCESS;
    }

    RtlCopyMemory( &Buffer->FileName[0],
                   &Fcb->ShortName.Buffer[0],
                   BytesToCopy );

    Buffer->FileNameLength = Fcb->ShortName.Length;

    //
    //  Reduce the available bytes by the amount stored into this buffer.
    //

    *Length -= BytesToCopy;

    return Status;
}

//
//  Local support routine
//

VOID
NwQueryNetworkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

 Description:

    This routine performs the query network open information function for Nwfs

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Fill in the input buffer from the Fcb fields.
    //

    Buffer->CreationTime.QuadPart = Fcb->CreationTime.QuadPart;
    Buffer->LastWriteTime.QuadPart =
    Buffer->ChangeTime.QuadPart = Fcb->LastWriteTime.QuadPart;
    Buffer->LastAccessTime.QuadPart = Fcb->LastAccessTime.QuadPart;

    Buffer->FileAttributes = Fcb->FileAttributes;

    //
    //  We get the sizes from the header.  Return a size of zero
    //  for all directories.
    //

    if (FlagOn( Fcb->FileAttributes, NETWARE_ATTR_DIRECTORY )) {

        Buffer->AllocationSize.QuadPart =
        Buffer->EndOfFile.QuadPart = 0;

    } else {

        Buffer->AllocationSize.QuadPart = Fcb->AllocationSize.QuadPart;
        Buffer->EndOfFile.QuadPart = Fcb->FileSize.QuadPart;
    }

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_NETWORK_OPEN_INFORMATION );

    return;
}
