
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

    NameSup.c

Abstract:

    This module implements the Nwfs Name support routines

  Authors:

    Jeff Merkey (jmerkey@timpanogas.com)
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

