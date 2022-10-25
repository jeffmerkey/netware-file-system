
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

Module Name:

    NameSup.c

Abstract:

    This module implements the Nwfs Name support routines

  Authors:

    Jeff Merkey (jeffmerkey@gmail.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

****************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_NAMESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwDissectName)
#pragma alloc_text(PAGE, NwIsNameInExpression)
#endif


VOID
NwDissectName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT POEM_STRING RemainingName,
    OUT POEM_STRING FinalName
    )

/*++

Routine Description:

    This routine is called to strip off leading components of the name strings.
    We search for either the end of the string or separating characters.  The
    input remaining name strings should have neither a trailing or leading
    backslash.

Arguments:

    RemainingName - Remaining name.

    FinalName - Location to store next component of name.

Return Value:

    None.

--*/

{
    ULONG NameLength;
    PUCHAR NextChar;

    PAGED_CODE();

    //
    //  Find the offset of the next component separators.
    //

    for (NameLength = 0, NextChar = RemainingName->Buffer;
         (NameLength < RemainingName->Length) && (*NextChar != '\\');
         NameLength += 1, NextChar += 1);

    //
    //  Adjust all the strings by this amount.
    //

    FinalName->Buffer = RemainingName->Buffer;

    FinalName->MaximumLength = FinalName->Length = (USHORT) NameLength;

    //
    //  If this is the last component then set the RemainingName lengths to zero.
    //

    if (NameLength == RemainingName->Length) {

        RemainingName->Length = 0;

    //
    //  Otherwise we adjust the string by this amount plus the separating character.
    //

    } else {

        RemainingName->MaximumLength -= (USHORT) (NameLength + 1);
        RemainingName->Length -= (USHORT) (NameLength + 1);
        RemainingName->Buffer = Add2Ptr( RemainingName->Buffer,
                                         NameLength + 1,
                                         PUCHAR );
    }

    return;
}


BOOLEAN
NwIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING CurrentName,
    IN PUNICODE_STRING SearchExpression,
    IN ULONG  WildcardFlags
    )

/*++

Routine Description:

    This routine will compare two NwName strings.  We assume that if this
    is to be a case-insensitive search then they are already upcased.

    We compare the filename portions of the name and if they match we
    compare the version strings if requested.

Arguments:

    CurrentName - Filename from the disk.

    SearchExpression - Filename expression to use for match.

    WildcardFlags - Flags field which indicates which parts of the
        search expression might have wildcards.  These flags are the
        same as in the Ccb flags field.

Return Value:

    BOOLEAN - TRUE if the expressions match, FALSE otherwise.

--*/

{
    PAGED_CODE();

    //
    //  If there are wildcards in the expression then we call the
    //  appropriate FsRtlRoutine.
    //

    if (!FlagOn( WildcardFlags, CCB_FLAG_ENUM_CONSTANT )) {

        return FsRtlIsNameInExpression( SearchExpression,
                                        CurrentName,
                                        FALSE,
                                        NULL );

    //
    //  Otherwise do a direct memory comparison for the name string.
    //

    } else {

        if ((CurrentName->Length == SearchExpression->Length) &&
            RtlEqualMemory( CurrentName->Buffer,
                            SearchExpression->Buffer,
                            CurrentName->Length )) {
            return TRUE;

        } else {

            return FALSE;
        }
    }
}

