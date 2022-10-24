
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

    FilObSup.c

Abstract:

    This module implements the Nwfs File object support routines.

  Authors:

    Jeff Merkey (jmerkey@timpanogas.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

****************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_FILOBSUP)

//
//  Local constants.
//

#define TYPE_OF_OPEN_MASK               (0x00000007)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwDecodeFileObject)
#pragma alloc_text(PAGE, NwFastDecodeFileObject)
#pragma alloc_text(PAGE, NwSetFileObject)
#endif


VOID
NwSetFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PFCB Fcb OPTIONAL,
    IN PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This routine will initialize the FileObject context fields based on the
    input type and data structures.

Arguments:

    FileObject - Supplies the file object pointer being initialized.

    TypeOfOpen - Sets the type of open.

    Fcb - Fcb for this file object.  Ignored for UnopenedFileObject.

    Ccb - Ccb for the handle corresponding to this file object.  Will not
        be present for stream file objects.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  We only have values 0 to 7 available so make sure we didn't
    //  inadvertantly add a new type.
    //

    ASSERTMSG( "FileObject types exceed available bits\n", BeyondValidType <= 8 );

    //
    //  Setting a file object to type UnopenedFileObject means just
    //  clearing all of the context fields.  All the other input
    //

    if (TypeOfOpen == UnopenedFileObject) {

        FileObject->FsContext =
        FileObject->FsContext2 = NULL;

        return;
    }

    //
    //  Check that the 3 low-order bits of the Ccb are clear.
    //

    ASSERTMSG( "Ccb is not quad-aligned\n", !FlagOn( ((ULONG_PTR) Ccb), TYPE_OF_OPEN_MASK ));

    //
    //  We will or the type of open into the low order bits of FsContext2
    //  along with the Ccb value.
    //  The Fcb is stored into the FsContext field.
    //

    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = Ccb;

    SetFlag( ((ULONG_PTR) FileObject->FsContext2), TypeOfOpen );

    //
    //  Set the Vpb field in the file object.
    //

    FileObject->Vpb = Fcb->Vcb->Vpb;

    //
    //  Point to the section object pointer in the non-paged Fcb.
    //

    FileObject->SectionObjectPointer = &Fcb->FcbNonpaged->SegmentObject;

    return;
}



TYPE_OF_OPEN
NwDecodeFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb,
    OUT PCCB *Ccb
    )

/*++

Routine Description:

    This routine takes a file object and extracts the Fcb and Ccb (possibly NULL)
    and returns the type of open.

Arguments:

    FileObject - Supplies the file object pointer being initialized.

    Fcb - Address to store the Fcb contained in the file object.

    Ccb - Address to store the Ccb contained in the file object.

Return Value:

    TYPE_OF_OPEN - Indicates the type of file object.

--*/

{
    TYPE_OF_OPEN TypeOfOpen;

    PAGED_CODE();

    //
    //  If this is an unopened file object then return NULL for the
    //  Fcb/Ccb.  Don't trust any other values in the file object.
    //

    TypeOfOpen = FlagOn( (ULONG_PTR) FileObject->FsContext2,
                         TYPE_OF_OPEN_MASK );

    if (TypeOfOpen == UnopenedFileObject) {

        *Fcb = NULL;
        *Ccb = NULL;

    } else {

        //
        //  The Fcb is pointed to by the FsContext field.  The Ccb is in
        //  FsContext2 (after clearing the low three bits).  The low three
        //  bits are the file object type.
        //

        *Fcb = FileObject->FsContext;
        *Ccb = FileObject->FsContext2;

        ClearFlag( (ULONG_PTR) *Ccb, TYPE_OF_OPEN_MASK );
    }

    //
    //  Now return the type of open.
    //

    return TypeOfOpen;
}


TYPE_OF_OPEN
NwFastDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb
    )

/*++

Routine Description:

    This procedure takes a pointer to a file object, that has already been
    opened by Nwfs and does a quick decode operation.  It will only return
    a non null value if the file object is a user file open

Arguments:

    FileObject - Supplies the file object pointer being interrogated

    Fcb - Address to store Fcb if this is a user file object.  NULL
        otherwise.

Return Value:

    TYPE_OF_OPEN - type of open of this file object.

--*/

{
    TYPE_OF_OPEN TypeOfOpen;

    PAGED_CODE();

    ASSERT_FILE_OBJECT( FileObject );

    //
    //  The Fcb is in the FsContext field.  The type of open is in the low
    //  bits of the Ccb.
    //

    *Fcb = FileObject->FsContext;

    return (TYPE_OF_OPEN)FlagOn( (ULONG_PTR)FileObject->FsContext2, TYPE_OF_OPEN_MASK );
}


