
/***************************************************************************
*
*   Copyright (c) 1997-2022 Jeff V. Merkey
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jeffmerkey@gmail.com
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the Lesser GNU Public License as published by the
*   Free Software Foundation, version 2.1, or any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   General Public License for more details.
*
*   You are free to modify and re-distribute this program in accordance
*   with the terms specified in the GNU Public License.  The copyright
*   contained in this code is required to be present in any derivative
*   works and you are required to provide the source code for this
*   program as part of any commercial or non-commercial distribution.
*   You are required to respect the rights of the Copyright holders
*   named within this code.
*
*   jeffmerkey@gmail.com is the official maintainer of
*   this code.  You are encouraged to report any bugs, problems, fixes,
*   suggestions, and comments about this software to jeffmerkey@gmail.com
*   .  New releases, patches, bug fixes, and
*   technical documentation can be found at repo.icapsql.com.  We will
*   periodically post new releases of this software to repo.icapsql.com
*   that contain bug fixes and enhanced capabilities.
*
*   Original Authorship      :
*      source code written by Jeff V. Merkey
*
*   Original Contributors    :
*      Jeff V. Merkey
*      
*      
*
****************************************************************************
*
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWCREATE.C
*   DESCRIP  :   Directory Creation and Deletion Routines
*   DATE     :  February 12, 2022
*
*
***************************************************************************/

#include "globals.h"

//
//  GENERAL:  NetWare seems to allow just about anything in a name, even DOS
//  names, if you believe the output from the NameSpace modules.  This would
//  make sense, since NetWare clients and not the server actually provide
//  the UNICODE support for remote clients.  NetWare names, even DOS names,
//  have the ability to use just about all of the 0-256 range for special
//  code page character sets, etc. in terms of what values actually end up
//  inside of a directory record.  There is a filter through the file system
//  APIs for "behaved" and "unique" DOS names to get created during install,
//  or with NLM based programs(???) (calling an NLM an "application" is
//  technically incorrect, they are "kernel extensions").  Internally, the
//  NetWare file system appears to allow pretty broad freedom with name entry
//  values.  This means that direct Unicode/NetWare CodePage conversions
//  are possible, and in fact, the Windows NT NWFS works this way.  Netware
//  UNICODE support has trouble with UNICODE characters when converting
//  between a Windows NT UNICODE name and a NetWare name in some cases.
//
//  Netware also handles internationalization via the client, and not
//  the server, and it's interesting to note that even French and
//  German versions of Netware still host the english names on the
//  server itself.  It's the Netware client and not the server that
//  actually performs much of the magic with language enabling.  The
//  obvious disadvantage here is that on linux, Netware volumes created
//  with international versions of Netware may revert back to their
//  english names when mounted under linux.  The Novell KK (Novell
//  Japan) version of Netware for the Japanese and Far Eastern markets
//  does have some subtle differences to plain Netware, and our current
//  NWFS code base is not enabled for DBCS Novell KK Netware file
//  system support.
//
//  This implementation filters naming requests through a Linux template
//  via either the DOS, LONG, or UNIX namespaces.  The MAC namespace
//  information is maintained during create and directory namespace
//  modifications.
//
//  In order to create NetWare compatible MAC names, and to avoid
//  data loss by rendering a file's name incomprehensible or non-unique, we
//  attempt to remain compatibile with NetWare by limiting the name to 31
//  instead of 32 characters (seems to be a NetWare specific bug we should
//  reproduce ???).
//

BYTE DOSValidCharBitmap[] =
{
   0x00, 0x00, 0x00, 0x00,
   0x52, 0x63, 0xFF, 0x03,
   0xFF, 0xFF, 0xFF, 0x87,
   0xFF, 0xFF, 0xFF, 0xE8,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0x00
};

BYTE DOSSkipCharBitmap[] =
{
   0x00, 0x00, 0x00, 0x00,
   0x05, 0x40, 0x00, 0xD4,
   0x00, 0x00, 0x00, 0x10,
   0x00, 0x00, 0x00, 0x10,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00
};

BYTE DOSReplaceCharBitmap[] =
{
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x18, 0x00, 0x28,
   0x00, 0x00, 0x00, 0x28,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00
};

// These characters are allowed in a LONG file name but not a DOS File Name
// (Netware 5.x, IntraNetWare (4.x) does not seem to care)
// =(x3D) %(x25) #(x23) +(x2B)
// '(x27) [(x5B) ](x5D) ;(x3B)
// ,(x2C) "(x22) <space>(x20)

BYTE LONGValidCharBitmap[] =
{
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFB, 0x7B, 0xFF, 0x2F,
   0xFF, 0xFF, 0xFF, 0xEF,
   0xFF, 0xFF, 0xFF, 0xEF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0x00
};

BYTE NFSValidCharBitmap[] =
{
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFB, 0x7B, 0xFF, 0x2F,
   0xFF, 0xFF, 0xFF, 0xEF,
   0xFF, 0xFF, 0xFF, 0xEF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0x00
};

// this is probably ok for the MAC name table since all the bytes
// are filtered through the conversion tables below
BYTE MACValidCharBitmap[] =
{
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFB, 0x7B, 0xFF, 0x2F,
   0xFF, 0xFF, 0xFF, 0xEF,
   0xFF, 0xFF, 0xFF, 0xEF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF,
   0x00
};

BYTE DOSToMACConvertTable[] =
{
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
   0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,

   0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
   0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
   0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
   0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,

   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
   0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
   0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
   0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,

   0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
   0x82, 0x9F, 0x8E, 0x89, 0x8A, 0x88, 0x8C, 0x8D,
   0x90, 0x91, 0x8F, 0x95, 0x94, 0x93, 0x80, 0x81,
   0x83, 0xBE, 0xAE, 0x99, 0x9A, 0x98, 0x9E, 0x9D,
   0xD8, 0x85, 0x86, 0xA2, 0xA3, 0xB4, 0xCC, 0xC4,

   0x87, 0x92, 0x97, 0x9C, 0x96, 0x84, 0xBB, 0xBC,
   0xC0, 0x8B, 0xC2, 0x9B, 0xA0, 0xC1, 0xC7, 0xC8,
   0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xCD, 0xD7,
   0xCE, 0xCF, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
   0xE0, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,

   0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
   0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
   0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
   0xA4, 0xA7, 0xA6, 0xB9, 0xB7, 0xA8, 0xB5, 0xA9,
   0xAA, 0xAB, 0xBD, 0xB6, 0xB0, 0xBF, 0xCB, 0xAD,

   0xAF, 0xB1, 0xB3, 0xB2, 0xB8, 0xBA, 0xD6, 0xC5,
   0xA1, 0xA5, 0xE1, 0xC3, 0xAC, 0xC6, 0xC9, 0xCA,
   0
};

BYTE MACToDOSConvertTable[] =
{
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
   0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,

   0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
   0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
   0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
   0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,

   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
   0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
   0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
   0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,

   0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
   0x8E, 0x8F, 0x80, 0x90, 0xA5, 0x99, 0x9A, 0xA0,
   0x85, 0x83, 0x84, 0xA9, 0x86, 0x87, 0x82, 0x8A,
   0x88, 0x89, 0xA1, 0x8D, 0x8C, 0x8B, 0xA4, 0xA2,
   0x95, 0x93, 0x94, 0xAB, 0xA3, 0x97, 0x96, 0x81,

   0xAC, 0xF8, 0x9B, 0x9C, 0xE0, 0xF9, 0xE2, 0xE1,
   0xE5, 0xE7, 0xE8, 0xE9, 0xFC, 0xEF, 0x92, 0xF0,
   0xEC, 0xF1, 0xF3, 0xF2, 0x9D, 0xE6, 0xEB, 0xE4,
   0xF4, 0xE3, 0xF5, 0xA6, 0xA7, 0xEA, 0x91, 0xED,
   0xA8, 0xAD, 0xAA, 0xFB, 0x9F, 0xF7, 0xFD, 0xAE,

   0xAF, 0xFE, 0xFF, 0xEE, 0x9E, 0xB6, 0xB8, 0xB9,
   0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xF6, 0xB7,
   0x98, 0xC1, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
   0xC0, 0xFA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
   0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,

   0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
   0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
   0
};

BYTE DOSUpperCaseTable[] =
{
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
   0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,

   0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
   0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
   0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
   0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,

   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
   0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
   0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
   0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,

   0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
   0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
   0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
   0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
   0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,

   0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
   0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
   0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
   0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
   0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,

   0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
   0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
   0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
   0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
   0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,

   0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
   0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
   0
};

BYTE DOSLowerCaseTable[] =
{
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
   0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
   0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
   0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
   0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
   0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
   0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
   0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
   0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
   0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
   0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
   0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
   0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
   0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
   0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
   0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
   0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
   0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
   0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
   0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
   0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
   0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
   0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
   0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
   0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
   0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
   0
};

// MS-DOS device reserved file names NetWare appears to check for
// and disallows in any namespace implementation.  It should be noted
// that one of these names (PIPE) will actually generate a file
// system mount error under Netware if you allow folks to create
// it.  The other names may cause client software to malfunction
// if we allow them to exist.

BYTE *ReservedNames[]=
{
   "NETQ    ", "PIPE    ", "CLOCK$  ", "CON     ",
   "PRN     ", "NUL     ", "AUX     ", "LPT1    ",
   "LPT2    ", "LPT3    ", "LPT4    ", "LPT5    ",
   "LPT6    ", "LPT7    ", "LPT8    ", "LPT9    ",
   "COM1    ", "COM2    ", "COM3    ", "COM4    ",
   "COM5    ", "COM6    ", "COM7    ", "COM8    ",
   "COM9    ",  0
};

extern ULONG FreeHashDirectoryRecord(VOLUME *volume, HASH *hash);

ULONG TestCharacter(BYTE *p, BYTE c)
{
   return (ULONG) (((p[c >> 3]) >> (c & 7)) & 1);
}

//
//   these functions determine whether a given name is valid or not
//   for a given namespace.  Some of the logic in the DOS function was
//   adopted from the linux fat file system, though we were forced
//   to make some changes to correct problems in the fat file system
//   with collapsed names.
//

ULONG NWValidDOSName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		     ULONG Parent, ULONG searchFlag)
{
    register BYTE **Reserved;
    register ULONG retCode, i, RemainingLength, Space = 0;
    register HASH *hash;
    BYTE DOSName[MAX_DOS_NAME];

    NWFSSet(DOSName, ' ', MAX_DOS_NAME);

    if (NameLength > MAX_DOS_NAME)
    {
       return NwBadName;
    }

    // MSDOS file names cannot begin with a '.'
    if (DOSName[0] == '.')
    {
       return NwBadName;
    }

    // create case insensitive string
    for (i=0; (i < MAX_DOS_NAME) && (i < NameLength); i++)
       DOSName[i] = DOSUpperCaseTable[(BYTE)Name[i]];

    // check the first 8 characters for any invalid characters.
    // If any are found, reject the name.  also check the
    // character after the first 8 and see if it's a '.' entry.

    for (RemainingLength = NameLength, i = 0;
	(i < NameLength) && (i < 8);
	i++, RemainingLength--)
    {
       if (DOSName[i] == '.')
	  break;

       retCode = TestCharacter(DOSValidCharBitmap, DOSName[i]);
       if (!retCode)
       {
	  return NwBadName;
       }

       Space = (DOSName[i] == ' ');
    }

    // if there was a space in the name, reject it.
    if (Space)
    {
       return NwBadName;
    }

    // if we have additional characters, then see if the next character is
    // a dot.

    if (RemainingLength && DOSName[i] != '.')
    {
       return NwBadName;
    }

    // if we found the dot, then process the file extension
    if (RemainingLength && DOSName[i] == '.')
    {
	// if there are too many possible extension characters, error
	if (RemainingLength > 4)
	{
	   return NwBadName;
	}

	// skip past first '.' character
	i++;
	RemainingLength--;

	while ((RemainingLength > 0) && (i < MAX_DOS_NAME) &&
	       (i < NameLength))
	{
	   // only one '.' entry is allowed.  reject the name if we
	   // detect a subsequent '.' within the extension.

	   if (DOSName[i] == '.')
	   {
	      return NwBadName;
	   }

	   retCode = TestCharacter(DOSValidCharBitmap, DOSName[i]);
	   if (!retCode)
	   {
	      return NwBadName;
	   }

	   Space = (DOSName[i] == ' ');

	   i++;
	   RemainingLength--;
	}
	// if we encountered a space or we still have characters left,
	// then report error.
	if (Space)
	{
	   return NwBadName;
	}
	if (RemainingLength)
	{
	   return NwBadName;
	}
    }

    // see if the name conflicts with any special or reserved
    // DOS or Netware names.  if so, disallow the name.

    for (Reserved = ReservedNames; *Reserved; Reserved++)
       if (!strncmp(Name, *Reserved, 8))
       {
	  return NwBadName;
       }

    // see if the name exists in the specified namespace
    if (searchFlag)
    {
       hash = GetHashFromName(volume, DOSName, NameLength, DOS_NAME_SPACE, Parent);
       if (hash)
       {
           return NwFileExists;
       }
    }
    return 0;
}

ULONG NWValidMACName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		     ULONG Parent, ULONG searchFlag)
{
    register ULONG i, len, retCode;
    register HASH *hash;

    // a trailing space is not allowed
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    for (len=i=0; (i < NameLength) && (i < MAX_MAC_NAME); i++)
    {
       retCode = TestCharacter(MACValidCharBitmap, Name[i]);
       if (!retCode)
	  return NwBadName;
    }

    // see if the name exists in the specified namespace
    if (searchFlag)
    {
       hash = GetHashFromName(volume, Name, NameLength, MAC_NAME_SPACE, Parent);
       if (hash)
       {
	  return NwFileExists;
       }
    }
    return 0;
}

ULONG NWValidNFSName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		     ULONG Parent, ULONG searchFlag)
{
    register ULONG i, len, retCode;
    register HASH *hash;

    // a trailing space is not allowed
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    for (len=i=0; (i < NameLength) && (i < MAX_NFS_NAME); i++)
    {
       retCode = TestCharacter(NFSValidCharBitmap, Name[i]);
       if (!retCode)
	  return NwBadName;
    }

    // see if the name exists in the specified namespace
    if (searchFlag)
    {
       hash = GetHashFromName(volume, Name, NameLength, UNIX_NAME_SPACE, Parent);
       if (hash)
       {
	  return NwFileExists;
       }
    }
    return 0;
}

ULONG NWValidLONGName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		      ULONG Parent, ULONG searchFlag)
{
    register ULONG i, len, retCode;
    register HASH *hash;

    // a trailing space is not allowed
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    for (len=i=0; (i < NameLength) && (i < MAX_LONG_NAME); i++)
    {
       retCode = TestCharacter(LONGValidCharBitmap, Name[i]);
       if (!retCode)
	  return NwBadName;
    }

    // see if the name exists in the specified namespace
    if (searchFlag)
    {
       hash = GetHashFromName(volume, Name, NameLength, LONG_NAME_SPACE, Parent);
       if (hash)
       {
	  return NwFileExists;
       }
    }
    return 0;
}

//
//  these functions copy and/or transform names as required.  This code
//  method was adapted from the VFAT file system.

#define MSDOS_NAME  11

//   OpFlag = 0   search all name hashes for name collisions
//   OpFlag = 1   do not search any hashes for name collisions

ULONG NWCreateUniqueName(VOLUME *volume, ULONG Parent,
			 BYTE *Name, ULONG NameLength,
			 BYTE *MangledName, BYTE *MangledNameLength,
			 ULONG OpFlag)
{
    register HASH *hash;
    register BYTE *ip, *p;
    register BYTE *ExtensionStart, *NameEnd, *NameStart;
    register ULONG Size, ExtensionLength, BaseLength, i, value;
    BYTE FileBase[8], FileExtension[3], Buffer[8];

    if (!MangledNameLength)
       return NwInvalidParameter;

    NWFSSet(FileBase, ' ', 8);
    NWFSSet(FileExtension, ' ', 3);

    Size = 0;
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    ExtensionStart = NameEnd = &Name[NameLength];
    while (--ExtensionStart >= Name)
    {
       if (*ExtensionStart == '.')
       {
	  if (ExtensionStart == (NameEnd - 1))
	  {
	     Size = NameLength;
	     ExtensionStart = NULL;
	  }
	  break;
       }
    }

    // if we could not find a '.' entry, assume no extension
    if (ExtensionStart == (Name - 1))
    {
       Size = NameLength;
       ExtensionStart = NULL;
    }
    else
    if (ExtensionStart)
    {
       // we found a '.' entry somewhere in the name
       NameStart = &Name[0];

       // begin from start of name to location of '.' entry
       // and skip invalid characters until we find the first
       // valid character for a DOS name.
       while (NameStart < ExtensionStart)
       {
	  if (!TestCharacter(DOSSkipCharBitmap, *NameStart))
	     break;

	  if (TestCharacter(DOSValidCharBitmap, *NameStart))
	     break;

	  NameStart++;
       }

       // if we found a valid character and we have not
       // reached the '.' entry, then calculate the size
       // of the base filename, and bump the extension
       // start pointer to the first character after
       // the '.' entry.

       if (NameStart != ExtensionStart)
       {
	  Size = ExtensionStart - Name;
	  ExtensionStart++;
       }
       else
       {
	  Size = NameLength;
	  ExtensionStart = NULL;
       }
    }

    for (BaseLength = i = 0, p = FileBase, ip = Name;
	 (i < Size) && (BaseLength < 8);
	 i++)
    {
       if (*ip & 0x80)
       {
	  *p++ = '_';
	  BaseLength++;
	  continue;
       }

       if (!TestCharacter(DOSSkipCharBitmap, *ip) &&
	    TestCharacter(DOSValidCharBitmap, *ip))
       {
	  *p = *ip;

	  if (TestCharacter(DOSReplaceCharBitmap, *p))
	     *p='_';

	  p++;
	  BaseLength++;
       }
       ip++;
    }

    if (BaseLength == 0)
       return NwBadName;

    ExtensionLength = 0;
    if (ExtensionStart)
    {
       for (p = FileExtension, ip = ExtensionStart;
	   (ExtensionLength < 3) && (ip < NameEnd);
	    ip++)
       {
	  if (*ip & 0x80)
	  {
	     *p++ = '_';
	     ExtensionLength++;
	     continue;
	  }

	  if (!TestCharacter(DOSSkipCharBitmap, *ip) &&
	       TestCharacter(DOSValidCharBitmap, *ip))
	  {
	     *p = *ip;

	     if (TestCharacter(DOSReplaceCharBitmap, *p))
		*p='_';

	     ExtensionLength++;
	     p++;
	  }
       }
    }

    if (BaseLength > 8)
       BaseLength = 8;

    if (ExtensionLength > 3)
       ExtensionLength = 3;

    // attempt to randomize a DOS name even more so if it exceeded
    // the legal filename size for MSDOS.  The fat file system
    // implementation does not try to randomize file names and
    // during some copy operations, two or more files can be lost
    // and collapsed into a single file because the fat implementation
    // will convert them to identical names during create (cp isn't
    // very smart, and if a filename has been created, if a new name
    // hashes to the same mangled name, we will overwrite an existing
    // file rather than create a new one.).  We have implemented a
    // different method from the fat file system that adds the
    // bytes for a name together, and creates the name from a hash
    // value.

    // check if the name is longer than default DOS filename length.
    if (Size > 8)
    {
       // if so, we are going to calculate a hash of the source name
       // and use the hash value to create the unique name.  This will
       // attempt to guarantee uniqueness of the names and prevent
       // similar names like "trampoline.S" and "trampoline32.S from
       // mangling to an identical name (TRAMPOLI.S).  The fat file
       // system actually allows this, and will collapse several files
       // into a single file during name mangling operations if you
       // perform recursive copies of the linux tree onto a fat
       // partition.

       if (BaseLength > 5)
	  BaseLength = 5;

       NWFSSet(MangledName, ' ', 12);
       NWFSCopy(MangledName, FileBase, BaseLength);
       MangledName[BaseLength] = '~';

       // calculate horner hash value from the original name
       value = NWFSStringHash(Name, NameLength, 255);
       for (i = value; i < 255; i++)
       {
	  sprintf(Buffer, "%02X", (unsigned int) i);
	  NWFSCopy(&MangledName[BaseLength + 1], Buffer, 2);

	  if (ExtensionLength)
	  {
	     MangledName[BaseLength + 3] = '.';
	     NWFSCopy(&MangledName[BaseLength + 4], FileExtension, ExtensionLength);
	  }
	  *MangledNameLength = (BYTE)(BaseLength + ExtensionLength + 3 + (ExtensionLength > 0));

	  // if we were not asked to search the hashes, then just transform
	  // the name to it's first instanciation, then exit the function.
	  if (OpFlag)
	     return 0;

	  hash = GetHashFromName(volume, MangledName, (*MangledNameLength),
				 DOS_NAME_SPACE, Parent);

#if (VERBOSE)
	  NWFSPrint("(1a) BaseLength-%d ExtensionLength-%d [%12s]-%d hash-%X\n",
		   (int)BaseLength, (int)ExtensionLength, MangledName,
		   (int)*MangledNameLength,
		   (unsigned int)hash);
#endif

	  if (!hash)
	     return 0;
       }
    }
    else
    {
       NWFSSet(MangledName, ' ', 12);
       NWFSCopy(MangledName, FileBase, BaseLength);
       if (ExtensionLength)
       {
	  MangledName[BaseLength] = '.';
	  NWFSCopy(&MangledName[BaseLength + 1], FileExtension, ExtensionLength);
       }
       *MangledNameLength = (BYTE)(BaseLength + ExtensionLength + (ExtensionLength > 0));

       // if we were not asked to search the hashes, then just transform
       // the name to it's first instanciation, then exit the function.
       if (OpFlag)
	  return 0;

       hash = GetHashFromName(volume, MangledName, (*MangledNameLength),
			   DOS_NAME_SPACE, Parent);
#if (VERBOSE)
       NWFSPrint("(1b) BaseLength-%d ExtensionLength-%d [%12s]-%d hash-%X\n",
		 (int)BaseLength, (int)ExtensionLength, MangledName,
		 (int)*MangledNameLength,
		 (unsigned int)hash);
#endif

       if (!hash)
	  return 0;
    }

    if (BaseLength > 6)
       BaseLength = 6;

    NWFSSet(MangledName, ' ', 12);
    NWFSCopy(MangledName, FileBase, BaseLength);
    MangledName[BaseLength] = '~';

    for (i = 1; i < 10; i++)
    {
       MangledName[BaseLength + 1] = (BYTE)(i + '0');

       if (ExtensionLength)
       {
	  MangledName[BaseLength + 2] = '.';
	  NWFSCopy(&MangledName[BaseLength + 3], FileExtension, ExtensionLength);
       }
       *MangledNameLength = (BYTE)(BaseLength + ExtensionLength + 2 + (ExtensionLength > 0));

       hash = GetHashFromName(volume, MangledName, (*MangledNameLength),
			      DOS_NAME_SPACE, Parent);

#if (VERBOSE)
       NWFSPrint("(2) BaseLength-%d ExtensionLength-%d [%12s]-%d hash-%X\n",
		(int)BaseLength, (int)ExtensionLength, MangledName,
		(int)*MangledNameLength,
		(unsigned int)hash);
#endif

       if (!hash)
	  return 0;
    }

#if (LINUX_20 | LINUX_22 | LINUX_24)
    i = jiffies & 0xffff;
    Size = (jiffies >> 16) & 0x7;
#endif

#if (WINDOWS_NT | WINDOWS_NT_RO | WINDOWS_NT_UTIL | LINUX_UTIL | DOS_UTIL)
    i = NameLength & 0xffff;
    Size = (NameLength >> 16) & 0x7;
#endif

    if (BaseLength > 2)
       BaseLength = 2;

    NWFSSet(MangledName, ' ', 12);
    NWFSCopy(MangledName, FileBase, BaseLength);

    MangledName[BaseLength + 4] = '~';
    MangledName[BaseLength + 5] = (BYTE)('1' + Size);

    while (1)
    {
       //  This code was adapted from the VFAT file system for creating
       //  random names.  although the method being used in VFAT works
       //  well enough, it is not the actual method used in Windows NT
       //  and Windows 98 to do this.  In Microsoft implementations
       //  of fat, random short names are actually derived from a file's
       //  checksum value, and not the system clock, as is being done
       //  here.

       sprintf(Buffer, "%04X", (unsigned int) i);
       NWFSCopy(&MangledName[BaseLength], Buffer, 4);

       if (ExtensionLength)
       {
	  MangledName[BaseLength + 6] = '.';
	  NWFSCopy(&MangledName[BaseLength + 7], FileExtension, ExtensionLength);
       }
       *MangledNameLength = (BYTE)(BaseLength + ExtensionLength + 6 + (ExtensionLength > 0));

       hash = GetHashFromName(volume, MangledName, *MangledNameLength,
			      DOS_NAME_SPACE, Parent);

#if (VERBOSE)
       NWFSPrint("(3) BaseLength-%d ExtensionLength-%d [%12s]-%d hash-%X\n",
		 (int)BaseLength, (int)ExtensionLength, MangledName,
		 (int)*MangledNameLength,
		 (unsigned int)hash);
#endif

       if (!hash)
	  break;

       i -= 11;
    }
    return 0;
}

ULONG NWMangleDOSName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		      DOS *dos, ULONG Parent)
{
    register ULONG i, retCode;
    BYTE DOSName[256];
    BYTE OutputName[256];
    BYTE OutputLength = 0;

    if (NameLength > 255)
       return NwBadName;

    if (!NameLength)
       return NwBadName;

    // a trailing space is not allowed
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    // create case insensitive string
    for (i=0; i < NameLength; i++)
       DOSName[i] = DOSUpperCaseTable[(BYTE)Name[i]];

    // if we are passed a valid dos name that's also unique, return ok
    if (NameLength <= MAX_DOS_NAME)
    {
       retCode = NWValidDOSName(volume, DOSName, NameLength, Parent, 1);
       if (!retCode)
       {
	  if (NameLength > MAX_DOS_NAME)
	     return NwBadName;

	  dos->FileNameLength = (BYTE) NameLength;
	  NWFSCopy(dos->FileName, DOSName, NameLength);

	  return retCode;
       }
       
       if (volume->NameSpaceDefault == DOS_NAME_SPACE)
       {
          if (retCode == NwFileExists)
	     return retCode;
       } 
    }

    retCode = NWCreateUniqueName(volume, Parent, DOSName, NameLength,
				 OutputName, &OutputLength, 0);
    if (retCode)
       return retCode;

    if (OutputLength > MAX_DOS_NAME)
       return NwBadName;

    dos->FileNameLength = (BYTE) OutputLength;
    NWFSCopy(dos->FileName, OutputName, OutputLength);

    return 0;

}

ULONG NWMangleMACName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		      MACINTOSH *mac, ULONG Parent)
{
    register ULONG i, Length, retCode;
    register HASH *hash;
    BYTE CName[MAX_MAC_NAME];

    if (NameLength > 255)
       return NwBadName;

    if (!NameLength)
       return NwBadName;

    // a trailing space is not allowed
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    // save local copy just in case our name source and target
    // are the same.
    for (i=0; i < NameLength && i < MAX_MAC_NAME; i++)
       CName[i] = Name[i];

    for (Length=i=0; (i < NameLength) && (i < MAX_MAC_NAME); i++)
    {
       // copy the character into file name
       retCode = TestCharacter(MACValidCharBitmap, CName[i]);
       if (retCode)
	  mac->FileName[Length++] = DOSToMACConvertTable[(BYTE)CName[i]];
    }
    mac->FileNameLength = (BYTE) Length;

    // see if the name exists in the specified namespace
    hash = GetHashFromName(volume, mac->FileName, mac->FileNameLength,
			   MAC_NAME_SPACE, Parent);
    if (hash)
    {
       return NwFileExists;
    }

    return 0;
}

ULONG NWMangleNFSName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		      NFS *nfs, ULONG Parent)
{
    register ULONG i, Length, retCode;
    register HASH *hash;
    BYTE CName[MAX_NFS_NAME];

    if (NameLength > 255)
       return NwBadName;

    if (!NameLength)
       return NwBadName;

    // a trailing space is not allowed
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    // save local copy just in case our name source and target
    // are the same.
    for (i=0; i < NameLength && i < MAX_NFS_NAME; i++)
       CName[i] = Name[i];

    for (Length=i=0; (i < NameLength) && (i < MAX_NFS_NAME); i++)
    {
       // copy the character into file name
       retCode = TestCharacter(NFSValidCharBitmap, CName[i]);
       if (retCode)
	  nfs->FileName[Length++] = CName[i];
    }
    nfs->FileNameLength = (BYTE) Length;
    nfs->TotalFileNameLength = (BYTE) Length;

    // see if the name exists in the specified namespace
    hash = GetHashFromName(volume, nfs->FileName, nfs->TotalFileNameLength,
			   UNIX_NAME_SPACE, Parent);
    if (hash)
    {
       // try a lowercase variant if the first name already exists
       for (Length=i=0; (i < NameLength) && (i < MAX_NFS_NAME); i++)
       {
	  // copy the character into file name
	  retCode = TestCharacter(NFSValidCharBitmap, CName[i]);
	  if (retCode)
	     nfs->FileName[Length++] = DOSLowerCaseTable[(BYTE)CName[i]];
       }
       nfs->FileNameLength = (BYTE) Length;
       nfs->TotalFileNameLength = (BYTE) Length;

       // see if the name exists in the specified namespace
       hash = GetHashFromName(volume, nfs->FileName, nfs->TotalFileNameLength,
			      UNIX_NAME_SPACE, Parent);
       if (hash)
       {
	  return NwFileExists;
       }
    }
    return 0;
}

ULONG NWMangleLONGName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		       LONGNAME *longname, ULONG Parent)
{
    register ULONG i, Length, retCode;
    register HASH *hash;
    BYTE CName[MAX_LONG_NAME];

    if (NameLength > 255)
       return NwBadName;

    if (!NameLength)
       return NwBadName;

    // a trailing space is not allowed
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    // save local copy just in case our name source and target
    // are the same.
    for (i=0; i < NameLength && i < MAX_LONG_NAME; i++)
       CName[i] = Name[i];

    for (Length=i=0; (i < NameLength) && (i < MAX_LONG_NAME); i++)
    {
       // copy the character into file name
       retCode = TestCharacter(LONGValidCharBitmap, CName[i]);
       if (retCode)
	  longname->FileName[Length++] = CName[i];
    }
    longname->FileNameLength = (BYTE) Length;
    longname->LengthData = (BYTE) Length;

    // see if the name exists in the specified namespace

    hash = GetHashFromName(volume, longname->FileName,
			   longname->FileNameLength,
			   LONG_NAME_SPACE, Parent);
    if (hash)
    {
       return NwFileExists;
    }

    return 0;
}

ULONG NWCreateMACName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		      MACINTOSH *mac, ULONG Parent)
{
    register ULONG i, Length, retCode;
    BYTE CName[MAX_MAC_NAME];

    if (NameLength > 255)
       return NwBadName;

    if (!NameLength)
       return NwBadName;

    // a trailing space is not allowed
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    // save local copy just in case our name source and target
    // are the same.
    for (i=0; i < NameLength && i < MAX_MAC_NAME; i++)
       CName[i] = Name[i];

    for (Length=i=0; (i < NameLength) && (i < MAX_MAC_NAME); i++)
    {
       // copy the character into file name
       retCode = TestCharacter(MACValidCharBitmap, CName[i]);
       if (retCode)
	  mac->FileName[Length++] = DOSToMACConvertTable[(BYTE)CName[i]];
    }
    mac->FileNameLength = (BYTE) Length;

    return 0;
}

ULONG NWCreateNFSName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		      NFS *nfs, ULONG Parent)
{
    register ULONG i, Length, retCode;
    BYTE CName[MAX_NFS_NAME];

    if (NameLength > 255)
       return NwBadName;

    if (!NameLength)
       return NwBadName;

    // a trailing space is not allowed
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    // save local copy just in case our name source and target
    // are the same.
    for (i=0; i < NameLength && i < MAX_NFS_NAME; i++)
       CName[i] = Name[i];

    for (Length=i=0; (i < NameLength) && (i < MAX_NFS_NAME); i++)
    {
       // copy the character into file name
       retCode = TestCharacter(NFSValidCharBitmap, CName[i]);
       if (retCode)
       {
	  if ((volume->NameSpaceDefault == UNIX_NAME_SPACE) ||
	      (volume->NameSpaceDefault == LONG_NAME_SPACE))
	     nfs->FileName[Length++] = CName[i];
          else
	     nfs->FileName[Length++] = DOSLowerCaseTable[(BYTE)CName[i]];
       }
    }
    nfs->FileNameLength = (BYTE) Length;
    nfs->TotalFileNameLength = (BYTE) Length;

    return 0;
}

ULONG NWCreateLONGName(VOLUME *volume, BYTE *Name, ULONG NameLength,
		       LONGNAME *longname, ULONG Parent)
{
    register ULONG i, Length, retCode;
    BYTE CName[MAX_LONG_NAME];

    if (NameLength > 255)
       return NwBadName;

    if (!NameLength)
       return NwBadName;

    // a trailing space is not allowed
    if (NameLength && (Name[NameLength - 1] == ' '))
       return NwBadName;

    // save local copy just in case our name source and target
    // are the same.
    for (i=0; i < NameLength && i < MAX_LONG_NAME; i++)
       CName[i] = Name[i];

    for (Length=i=0; (i < NameLength) && (i < MAX_LONG_NAME); i++)
    {
       // copy the character into file name
       retCode = TestCharacter(LONGValidCharBitmap, CName[i]);
       if (retCode)
	  longname->FileName[Length++] = CName[i];
    }
    longname->FileNameLength = (BYTE) Length;
    longname->LengthData = (BYTE) Length;

    return 0;
}

ULONG FreeHashRecords(VOLUME *volume, HASH **Entries, ULONG Count)
{
    register ULONG i, retCode = 0;

    // free hash entries up to count
    for (i=0; i < Count; i++)
    {
       retCode = FreeHashDirectoryRecord(volume, Entries[i]);
       // if we fail try to free the remaining records and report the error
       if (retCode)
	  NWFSPrint("nwfs:  could not release hash record in create\n");
    }
    return retCode;
}

ULONG FreeDirectoryRecords(VOLUME *volume, ULONG *Entries, ULONG Parent,
			   ULONG Count)
{
    register ULONG i, retCode = 0;
    DOS dosst;
    register DOS *dos = &dosst;

    // free directory entries up to count
    for (i=0; i < Count; i++)
    {
       retCode = FreeDirectoryRecord(volume, dos, Entries[i], Parent);
       // if we fail try to free the remaining records and report the error
       if (retCode)
	  NWFSPrint("nwfs:  could not release dir record in free\n");
    }
    return retCode;
}

ULONG CreateDirectoryRecords(VOLUME *volume, ULONG *Entries, ULONG Parent,
			     ULONG *NumRec)
{
    register ULONG i, retCode;

    // allocate directory entries for total namespaces
    for (i=0; i < volume->NameSpaceCount; i++)
    {
       Entries[i] = AllocateDirectoryRecord(volume, Parent);
       if (Entries[i] == (ULONG) -1)
       {
	  retCode = FreeDirectoryRecords(volume, Entries, Parent, i);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  could not release dir record in create\n");
	     return retCode;
	  }
	  return NwInsufficientResources;
       }
       // count the total records created
       if (NumRec)
	  (*NumRec)++;
    }
    return 0;
}

ULONG NWCreateDirectoryEntry(VOLUME *volume, BYTE *Name, ULONG Len,
			     ULONG Attributes, ULONG Parent, ULONG Flag,
			     ULONG Uid, ULONG Gid, ULONG RDev, ULONG Mode,
			     ULONG *retDirNo, ULONG DataStream,
			     ULONG DataStreamSize, ULONG DataFork,
			     ULONG DataForkSize, const char *SymbolicPath,
			     ULONG SymbolicPathLength)
{
    register ULONG RootDirNo, retCode, retries;
    register BYTE *Buffer;
    register DOS *dos;
    register SUB_DIRECTORY *subdir;
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *longname;
    register NTNAME *nt;
    register ULONG i, NWFlags = 0;
    register ULONG HashNumber = 0, RootNS = (ULONG) -1;
    register VOLUME_WORKSPACE *wk;

    ULONG RecordNumber = 0;
    DOS *WorkSpace[MAX_NAMESPACES];
    HASH *HashEntries[MAX_NAMESPACES];
    ULONG Entries[MAX_NAMESPACES];

    // depending on which flag value was passed into this function,
    // OR in the correct designator for the file type.  NetWare already
    // supports most types of files, so support for Unix style gid/uid
    // designators as well as device handles are already supported.

    switch (Flag)
    {
       case NW_SUBDIRECTORY_FILE:
	  // we are not permitted to hard link to directories
	  if (Flag & NW_HARD_LINKED_FILE)
	     return NwNotPermitted;
	  NWFlags |= SUBDIRECTORY_FILE;
	  break;

       case NW_SYMBOLIC_FILE:
	  // we can only create unix special files if the nfs namespace is present
	  if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
	     return NwNotPermitted;

	  if (!SymbolicPathLength || !SymbolicPath)
	     return NwInvalidParameter;

	  NWFlags |= SYMBOLIC_LINKED_FILE;
	  break;

       case NW_HARD_LINKED_FILE:
	  // we can only create unix special files if the nfs namespace is present
	  if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
	     return NwNotPermitted;
	  NWFlags |= HARD_LINKED_FILE;
	  break;

       case NW_NAMED_FILE:
	  NWFlags |= NAMED_FILE;
	  break;

       case NW_DEVICE_FILE:
	  // we can only create unix special files if the nfs namespace is present
	  if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
	     return NwNotPermitted;
	  NWFlags |= NAMED_FILE;
	  break;

       default:
	  return NwInvalidParameter;

    }

    // see if name conforms to default namespace conventions
    switch (volume->NameSpaceDefault)
    {
       case DOS_NAME_SPACE:
       case MAC_NAME_SPACE:
	  break;

       case UNIX_NAME_SPACE:
	  retCode = NWValidNFSName(volume, Name, Len, Parent, 1);
	  if (retCode)
	     return retCode;
	  break;

       case LONG_NAME_SPACE:
       case NT_NAME_SPACE:
	  retCode = NWValidLONGName(volume, Name, Len, Parent, 1);
	  if (retCode)
	     return retCode;
	  break;

       // if we get here, then return error
       default:
	  return NwBadName;

    }

    // get enough memory for directory workspace

    wk = AllocateNSWorkspace(volume);
    if (!wk)
    {
       NWFSPrint("nwfs:  alloc failed in NWCreate\n");
       return NwInsufficientResources;
    }
    Buffer = (BYTE *)&wk->Buffer[0];

    // initialize directory workspace array
    NWFSSet(Buffer, 0, sizeof(ROOT) * volume->NameSpaceCount);
    for (i=0; i < volume->NameSpaceCount; i++)
       WorkSpace[i] = (DOS *) &Buffer[i * sizeof(ROOT)];

    // we create all of the name space directory entries in memory prior to
    // attemping to write them to disk or add them to in-memory hash.

    for (i=0; i < volume->NameSpaceCount; i++)
    {
       switch (volume->NameSpaceID[i])
       {
	  case DOS_NAME_SPACE:
	     if (NWFlags & SUBDIRECTORY_FILE)
	     {
		subdir = (SUB_DIRECTORY *) WorkSpace[i];
		retCode = NWMangleDOSName(volume, Name, Len, (DOS *)subdir, Parent);
		if (retCode)
		{
		   FreeNSWorkspace(volume, wk);
		   return retCode;
		}

		subdir->Flags |= SUBDIRECTORY_FILE;
		subdir->FileAttributes |= (SUBDIRECTORY | Attributes);

		if (volume->NameSpaceDefault == DOS_NAME_SPACE)
		   subdir->Flags |= PRIMARY_NAMESPACE;
		subdir->Subdirectory = Parent;
		subdir->NameSpace = DOS_NAME_SPACE;
		subdir->CreateDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());
		subdir->OwnerID = SUPERVISOR;
		subdir->LastModifiedDateAndTime = subdir->CreateDateAndTime;
		subdir->PrimaryEntry = 0;
		subdir->NameList = 0;

		// if we create a new subdirectory, then make certain
		// we have enough free directory space to assign a
		// free block to this directory.

		retCode = PreAllocateFreeDirectoryRecords(volume);
		if (retCode)
		{
		   FreeNSWorkspace(volume, wk);
		   return retCode;
		}
	     }
	     else
	     {
		dos = (DOS *) WorkSpace[i];
		retCode = NWMangleDOSName(volume, Name, Len, dos, Parent);
		if (retCode)
		{
		   FreeNSWorkspace(volume, wk);
		   return retCode;
		}

		dos->FileAttributes |= Attributes;
		dos->Flags |= (NAMED_FILE | NWFlags);
		if (volume->NameSpaceDefault == DOS_NAME_SPACE)
		   dos->Flags |= PRIMARY_NAMESPACE;
		dos->Subdirectory = Parent;
		dos->NameSpace = DOS_NAME_SPACE;
		dos->CreateDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());
		dos->OwnerID = SUPERVISOR;
		dos->LastUpdatedDateAndTime = dos->CreateDateAndTime;
		dos->PrimaryEntry = 0;
		dos->NameList = 0;
		dos->LastUpdatedID = SUPERVISOR;
		dos->FileSize = DataStreamSize;
		dos->FirstBlock = DataStream;
	     }

	     // record the root namespace index
	     RootNS = i;

	     break;

	  case MAC_NAME_SPACE:
	     mac = (MACINTOSH *) WorkSpace[i];
	     retCode = NWMangleMACName(volume, Name, Len, mac, Parent);
	     if (retCode)
	     {
		FreeNSWorkspace(volume, wk);
		return retCode;
	     }
	     if (NWFlags & SUBDIRECTORY_FILE)
		mac->Flags |= SUBDIRECTORY_FILE;
	     else
		mac->Flags |= (NWFlags | NAMED_FILE);
	     if (volume->NameSpaceDefault == MAC_NAME_SPACE)
		mac->Flags |= PRIMARY_NAMESPACE;
	     mac->NameSpace = MAC_NAME_SPACE;
	     mac->Subdirectory = Parent;
	     mac->NameList = 0;
	     mac->PrimaryEntry = 0;
	     mac->ResourceForkSize = DataForkSize;
	     mac->ResourceFork = DataFork;
	     break;

	  case UNIX_NAME_SPACE:
	     nfs = (NFS *) WorkSpace[i];
	     retCode = NWMangleNFSName(volume, Name, Len, nfs, Parent);
	     if (retCode)
	     {
		FreeNSWorkspace(volume, wk);
		return retCode;
	     }
	     if (NWFlags & SUBDIRECTORY_FILE)
	     {
		nfs->nlinks = 2;
		nfs->Flags |= SUBDIRECTORY_FILE;
	     }
	     else
	     {
		nfs->nlinks = 1;
		nfs->Flags |= (NWFlags | NAMED_FILE);
	     }
	     if (volume->NameSpaceDefault == UNIX_NAME_SPACE)
		nfs->Flags |= PRIMARY_NAMESPACE;

	     if (NWFlags & SYMBOLIC_LINKED_FILE)
		nfs->LinkedFlag = NFS_SYMBOLIC_LINK;
	     else
		nfs->LinkedFlag = 0;

	     nfs->NameSpace = UNIX_NAME_SPACE;
	     nfs->Subdirectory = Parent;
	     nfs->FileNameLength = (nfs->TotalFileNameLength <= MAX_NFS_NAME)
				 ? nfs->TotalFileNameLength : MAX_NFS_NAME;
	     nfs->gid = Gid;
	     nfs->uid = Uid;
	     nfs->mode = Mode;
	     nfs->rdev = RDev;
	     nfs->NameList = 0;
	     nfs->PrimaryEntry = 0;
	     break;

	  case LONG_NAME_SPACE:
	     longname = (LONGNAME *) WorkSpace[i];
	     retCode = NWMangleLONGName(volume, Name, Len, longname, Parent);
	     if (retCode)
	     {
		FreeNSWorkspace(volume, wk);
		return retCode;
	     }
	     if (NWFlags & SUBDIRECTORY_FILE)
		longname->Flags |= SUBDIRECTORY_FILE;
	     else
		longname->Flags |= (NWFlags | NAMED_FILE);
	     if (volume->NameSpaceDefault == LONG_NAME_SPACE)
		longname->Flags |= PRIMARY_NAMESPACE;
	     longname->NameSpace = LONG_NAME_SPACE;
	     longname->Subdirectory = Parent;
	     longname->NameList = 0;
	     longname->PrimaryEntry = 0;
	     break;

	  case NT_NAME_SPACE:
	     nt = (NTNAME *) WorkSpace[i];
	     retCode = NWMangleLONGName(volume, Name, Len, (LONGNAME *)nt, Parent);
	     if (retCode)
	     {
		FreeNSWorkspace(volume, wk);
		return retCode;
	     }
	     if (NWFlags & SUBDIRECTORY_FILE)
		nt->Flags |= SUBDIRECTORY_FILE;
	     else
		nt->Flags |= (NWFlags | NAMED_FILE);
	     if (volume->NameSpaceDefault == NT_NAME_SPACE)
		nt->Flags |= PRIMARY_NAMESPACE;
	     nt->NameSpace = NT_NAME_SPACE;
	     nt->Subdirectory = Parent;
	     nt->NameList = 0;
	     nt->PrimaryEntry = 0;
	     break;

	  default:
	     // this case is a fatal error.  we should never have
	     // an allocated record without a valid namespace
	     // to match up with it.

	     FreeNSWorkspace(volume, wk);
	     return NwVolumeCorrupt;
       }
    }

    if (RootNS == (ULONG) -1)
    {
       NWFSPrint("nwfs:  could not determine root namespace NWCreate\n");
       FreeNSWorkspace(volume, wk);
       return NwDirectoryCorrupt;
    }

    // allocate directory records necessary to create the file or directory
    retCode = CreateDirectoryRecords(volume, Entries, Parent, &RecordNumber);
    if (retCode)
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  dir entry alloc failed in NWCreate\n");
#endif
       FreeNSWorkspace(volume, wk);
       return NwVolumeFull;
    }

    // verify that the number of allocated directory records is equal
    // to the number of namespaces for the volume.  if not, return error.
    if (RecordNumber != volume->NameSpaceCount)
    {
       NWFSPrint("nwfs:  allocated dir records != namespaces in NWCreate\n");

       retCode = FreeDirectoryRecords(volume, Entries, Parent, RecordNumber);
       if (retCode)
	  NWFSPrint("nwfs:  could not release dir record in NWCreate\n");

       FreeNSWorkspace(volume, wk);
       return NwVolumeCorrupt;
    }

    // link allocated directory records together into a single namespace
    // chain
    for (RootDirNo = Entries[RootNS], i=0;
	 RecordNumber && (i < (RecordNumber - 1)); i++)
    {
       // store the DirNo for next record into NameList
       dos = (DOS *) WorkSpace[i];
       dos->NameList = Entries[i + 1];
       dos->PrimaryEntry = RootDirNo;
    }
    // null terminate last dir namespace record
    dos = (DOS *) WorkSpace[i];
    dos->NameList = 0;
    dos->PrimaryEntry = RootDirNo;

    // write and hash the initialized directory records
    for (i=0; i < RecordNumber; i++)
    {
       HashEntries[i] = AllocHashDirectoryRecord(volume,
						(DOS *)WorkSpace[i],
						Entries[i]);
       if (!HashEntries[i])
       {
	  retCode = FreeHashRecords(volume, HashEntries, HashNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release hash record in NWCreate)\n");

	  retCode = FreeDirectoryRecords(volume, Entries, Parent, RecordNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release dir record in NWCreate\n");

	  FreeNSWorkspace(volume, wk);
	  return NwInsufficientResources;
       }
       // count the number of name hash elements we create

       HashNumber++;

       retCode = WriteDirectoryRecord(volume,
				     (DOS *)WorkSpace[i],
				     Entries[i]);
       if (retCode)
       {
	  retCode = FreeHashRecords(volume, HashEntries, HashNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release hash record in NWCreate)\n");

	  retCode = FreeDirectoryRecords(volume, Entries, Parent, RecordNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release dir record in NWCreate\n");

	  FreeNSWorkspace(volume, wk);
	  return NwDiskIoError;
       }
    }

    if (NWFlags & SYMBOLIC_LINKED_FILE)
    {
       register ULONG bytesWritten;
       ULONG writeRetCode;

       NWLockFileExclusive(HashEntries[0]);

       bytesWritten = NWWriteFile(volume,
				  &WorkSpace[RootNS]->FirstBlock,
				  WorkSpace[RootNS]->Flags,
				  0, (BYTE *)SymbolicPath,  SymbolicPathLength,
				  0, 0, &writeRetCode, KERNEL_ADDRESS_SPACE,
				  (volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0,
				  WorkSpace[RootNS]->FileAttributes);

       if (bytesWritten != SymbolicPathLength)
       {
          NWUnlockFile(HashEntries[0]);

	  retCode = FreeHashRecords(volume, HashEntries, HashNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release hash record in NWCreate)\n");

	  retCode = FreeDirectoryRecords(volume, Entries, Parent, RecordNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release dir record in NWCreate\n");

	  FreeNSWorkspace(volume, wk);
	  return writeRetCode;
       }

       // update NetWare directory entry with modified date and time
       WorkSpace[RootNS]->FileSize = bytesWritten;
       WorkSpace[RootNS]->LastUpdatedDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());

       // force suballocation of this symbolic link file
       TruncateClusterChain(volume,
                            &WorkSpace[RootNS]->FirstBlock,
			    0,
			    0,
                            WorkSpace[RootNS]->FileSize,
			    TRUE,
			    WorkSpace[RootNS]->FileAttributes);

#if (HASH_FAT_CHAINS)
       // update the hash first block and file size fields       
       HashEntries[0]->FirstBlock = WorkSpace[RootNS]->FirstBlock,
       HashEntries[0]->FileSize = WorkSpace[RootNS]->FileSize;
       HashEntries[0]->FileAttributes = WorkSpace[RootNS]->FileAttributes;
#endif
      
       NWUnlockFile(HashEntries[0]);

       // write the dos record to the directory file.
       writeRetCode = WriteDirectoryRecord(volume, WorkSpace[RootNS], Entries[RootNS]);
       if (writeRetCode)
       {
	  retCode = FreeHashRecords(volume, HashEntries, HashNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release hash record in NWCreate)\n");

	  retCode = FreeDirectoryRecords(volume, Entries, Parent, RecordNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release dir record in NWCreate\n");

	  FreeNSWorkspace(volume, wk);
	  return writeRetCode;
       }
    }

    // free the workspace
    FreeNSWorkspace(volume, wk);

    // if the count of empty subdirectories on this volume is greater
    // than the current free directory blocks, then preallocate
    // free space in the directory file until the count of empty
    // directories is greater than or equal to the number of
    // free directory blocks in the directory file.  Netware treats
    // this as a fatal error if any empty subdirectories exist for
    // which there is no corresponding free directory block.  We
    // don't treat this as a fatal error, and will attempt to
    // pre-allocate free records until a reasonable limit
    // is reached.

    if (volume->FreeDirectoryBlockCount < volume->FreeDirectoryCount)
    {
       retries = 0;
       while (volume->FreeDirectoryBlockCount < volume->FreeDirectoryCount)
       {
	  retCode = PreAllocateEmptyDirectorySpace(volume);
	  if (retCode)
	     break;

	  if (retries++ > 5)  // if we get stuck in a loop here, then exit
	  {
	     NWFSPrint("nwfs:  exceeded maximum pre-allocated records (create)\n");
	     break;
	  }
       }
    }

    if (retDirNo)
       *retDirNo = RootDirNo;

    return 0;

}

// this function only removes the directory entries for a particular
// file or directory.  data streams are not removed.

ULONG NWDeleteDirectoryEntry(VOLUME *volume, DOS *dos, HASH *hash)
{
    register ULONG retCode, Parent, DirNo, TrusteeDirNo;
    register HASH *searchHash, *listHash;
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *longname;
    register NTNAME *nt;
    register TRUSTEE *trustee;
    register SUB_DIRECTORY *subdir;

    // the DOS namespace must always be the first record on the name list
    if (hash->NameSpace != DOS_NAME_SPACE)
       return NwHashCorrupt;

    Parent = hash->Parent;
    listHash = hash;
    while (listHash)
    {
       searchHash = listHash;
       listHash = listHash->nlnext;

       switch (searchHash->NameSpace)
       {
	  case DOS_NAME_SPACE:
	     if (searchHash->Flags & SUBDIRECTORY_FILE)
	     {
		// check if directory still has files in it
		if (searchHash->Blocks)
		{
		   // don't delete directories unless they are empty
		   return NwNotEmpty;
		}
	     }

	     // save the directory number for this hash
	     DirNo = searchHash->DirNo;

	     if (searchHash->Flags & SUBDIRECTORY_FILE)
	     {
		// free any trustee records for this entry
		subdir = (SUB_DIRECTORY *) dos;
		TrusteeDirNo = subdir->NextTrusteeEntry;
		while (TrusteeDirNo)
		{
		   retCode = ReadDirectoryRecord(volume, dos, TrusteeDirNo);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error reading trustee record\n");
		      return retCode;
		   }

		   trustee = (TRUSTEE *) dos;
		   TrusteeDirNo = trustee->NextTrusteeEntry;

		   retCode = FreeDirectoryRecord(volume, dos, TrusteeDirNo,
					      Parent);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error freeing trustee record\n");
		      return retCode;
		   }
		}
	     }
	     else
	     {
		// free any trustee records for this entry
		TrusteeDirNo = dos->NextTrusteeEntry;
		while (TrusteeDirNo)
		{
		   retCode = ReadDirectoryRecord(volume, dos, TrusteeDirNo);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error reading trustee record\n");
		      return retCode;
		   }

		   trustee = (TRUSTEE *) dos;
		   TrusteeDirNo = trustee->NextTrusteeEntry;

		   retCode = FreeDirectoryRecord(volume, dos, TrusteeDirNo,
					      Parent);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error freeing trustee record\n");
		      return retCode;
		   }
		}
	     }

	     // free the hash first so its instance handle gets
	     // invalidated before clearing the directory entry.
	     FreeHashDirectoryRecord(volume, searchHash);

	     // now clear the directory record and mark it free in
	     // the dir block free lists

	     retCode = FreeDirectoryRecord(volume, dos, DirNo, Parent);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error freeing directory record\n");
		return retCode;
	     }
	     break;

	  case MAC_NAME_SPACE:
	     retCode = ReadDirectoryRecord(volume, dos, searchHash->DirNo);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error in directory read\n");
		return retCode;
	     }
	     mac = (MACINTOSH *) dos;

	     // save the directory number for this hash
	     DirNo = searchHash->DirNo;

	     // free the hash first so its instance handle gets
	     // invalidated before clearing the directory entry.
	     FreeHashDirectoryRecord(volume, searchHash);

	     // now clear the directory record and mark it free in
	     // the dir block free lists
	     retCode = FreeDirectoryRecord(volume, dos, DirNo, Parent);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error freeing directory record\n");
		return retCode;
	     }
	     break;

	  case UNIX_NAME_SPACE:
	     retCode = ReadDirectoryRecord(volume, dos, searchHash->DirNo);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error in directory read\n");
		return retCode;
	     }
	     nfs = (NFS *) dos;

	     // save the directory number for this hash
	     DirNo = searchHash->DirNo;

	     // free the hash first so its instance handle gets
	     // invalidated before clearing the directory entry.
	     FreeHashDirectoryRecord(volume, searchHash);

	     // now clear the directory record and mark it free in
	     // the dir block free lists
	     retCode = FreeDirectoryRecord(volume, dos, DirNo, Parent);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error freeing directory record\n");
		return retCode;
	     }
	     break;

	  case LONG_NAME_SPACE:
	     retCode = ReadDirectoryRecord(volume, dos, searchHash->DirNo);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error in directory read\n");
		return retCode;
	     }
	     longname = (LONGNAME *) dos;

	     // save the directory number for this hash
	     DirNo = searchHash->DirNo;

	     // free the hash first so its instance handle gets
	     // invalidated before clearing the directory entry.
	     FreeHashDirectoryRecord(volume, searchHash);

	     // now clear the directory record and mark it free in
	     // the dir block free lists
	     retCode = FreeDirectoryRecord(volume, dos, DirNo, Parent);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error freeing directory record\n");
		return retCode;
	     }
	     break;

	  case NT_NAME_SPACE:
	     retCode = ReadDirectoryRecord(volume, dos, searchHash->DirNo);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error in directory read\n");
		return retCode;
	     }
	     nt = (NTNAME *) dos;

	     // save the directory number for this hash
	     DirNo = searchHash->DirNo;

	     // free the hash first so its instance handle gets
	     // invalidated before clearing the directory entry.
	     FreeHashDirectoryRecord(volume, searchHash);

	     // now clear the directory record and mark it free in
	     // the dir block free lists
	     retCode = FreeDirectoryRecord(volume, dos, DirNo, Parent);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error freeing directory record\n");
		return retCode;
	     }
	     break;

	  default:
	     NWFSPrint("nwfs:  unknown directory record format\n");
	     break;
       }
    }
    return 0;
}

ULONG NWDeleteHashChain(VOLUME *volume, HASH *hash)
{
    register HASH *searchHash, *listHash;

    // the DOS namespace must always be the first record on the name list
    if (hash->NameSpace != DOS_NAME_SPACE)
       return NwHashCorrupt;

    listHash = hash;
    while (listHash)
    {
       searchHash = listHash;
       listHash = listHash->nlnext;
       FreeHashDirectoryRecord(volume, searchHash);
    }
    return 0;
}

// this function removes the directory entries and all datastreams
// associated with these directory entries.

ULONG NWDeleteDirectoryEntryAndData(VOLUME *volume, DOS *dos, HASH *hash)
{
    register ULONG retCode, Parent, DirNo, TrusteeDirNo, Attributes = 0;
    register HASH *searchHash, *listHash;
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *longname;
    register NTNAME *nt;
    register TRUSTEE *trustee;
    register SUB_DIRECTORY * subdir;

    // the DOS namespace must always be the first record on the name list
    if (hash->NameSpace != DOS_NAME_SPACE)
       return NwHashCorrupt;

    Parent = hash->Parent;
    listHash = hash;
    while (listHash)
    {
       searchHash = listHash;
       listHash = listHash->nlnext;

       switch (searchHash->NameSpace)
       {
	  case DOS_NAME_SPACE:
             Attributes = dos->FileAttributes;
	     if (searchHash->Flags & SUBDIRECTORY_FILE)
	     {
		// check if directory still has files in it
		if (searchHash->Blocks)
		{
		   // don't delete directories unless they are empty
		   return NwNotEmpty;
		}
	     }
	     else
	     {
		// free the primary data stream for this file
		if (dos->FirstBlock && (dos->FirstBlock != (ULONG) -1))
		{
		   retCode = TruncateClusterChain(volume,
					&dos->FirstBlock,
					0,
					0,
					0,
					(volume->VolumeFlags & SUB_ALLOCATION_ON)
					? 1 : 0, Attributes);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error in truncate cluster chain\n");
		      return retCode;
		   }
		}
		dos->FileSize = 0;
	     }

	     // save the directory number for this hash
	     DirNo = searchHash->DirNo;

	     if (searchHash->Flags & SUBDIRECTORY_FILE)
	     {
		// free any trustee records for this entry
		subdir = (SUB_DIRECTORY *) dos;
		TrusteeDirNo = subdir->NextTrusteeEntry;
		while (TrusteeDirNo)
		{
		   retCode = ReadDirectoryRecord(volume, dos, TrusteeDirNo);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error reading trustee record\n");
		      return retCode;
		   }

		   trustee = (TRUSTEE *) dos;
		   TrusteeDirNo = trustee->NextTrusteeEntry;

		   retCode = FreeDirectoryRecord(volume, dos, TrusteeDirNo,
					      Parent);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error freeing trustee record\n");
		      return retCode;
		   }
		}
	     }
	     else
	     {
		// free any trustee records for this entry
		TrusteeDirNo = dos->NextTrusteeEntry;
		while (TrusteeDirNo)
		{
		   retCode = ReadDirectoryRecord(volume, dos, TrusteeDirNo);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error reading trustee record\n");
		      return retCode;
		   }

		   trustee = (TRUSTEE *) dos;
		   TrusteeDirNo = trustee->NextTrusteeEntry;

		   retCode = FreeDirectoryRecord(volume, dos, TrusteeDirNo,
					      Parent);
		   if (retCode)
		   {
		      NWFSPrint("nwfs:  error freeing trustee record\n");
		      return retCode;
		   }
		}
	     }

	     // free the hash first so its instance handle gets
	     // invalidated before clearing the directory entry.
	     FreeHashDirectoryRecord(volume, searchHash);

	     // now clear the directory record and mark it free in
	     // the dir block free lists
	     retCode = FreeDirectoryRecord(volume, dos, DirNo, Parent);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error freeing directory record\n");
		return retCode;
	     }
	     break;

	  case MAC_NAME_SPACE:
	     retCode = ReadDirectoryRecord(volume, dos, searchHash->DirNo);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error in directory read\n");
		return retCode;
	     }

	     mac = (MACINTOSH *) dos;
	     // free the mac resource fat chain for this file

	     if (mac->ResourceFork && (mac->ResourceFork != (ULONG) -1))
	     {
		retCode = TruncateClusterChain(volume, &mac->ResourceFork, 0, 0,
				 0, (volume->VolumeFlags & SUB_ALLOCATION_ON)
				 ? 1 : 0, Attributes);
		if (retCode)
		{
		   NWFSPrint("nwfs:  error in truncate cluster chain\n");
		   return retCode;
		}
	     }
	     mac->ResourceForkSize = 0;

	     // save the directory number for this hash
	     DirNo = searchHash->DirNo;

	     // free the hash first so its instance handle gets
	     // invalidated before clearing the directory entry.
	     FreeHashDirectoryRecord(volume, searchHash);

	     // now clear the directory record and mark it free in
	     // the dir block free lists
	     retCode = FreeDirectoryRecord(volume, dos, DirNo, Parent);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error freeing directory record\n");
		return retCode;
	     }
	     break;

	  case UNIX_NAME_SPACE:
	     retCode = ReadDirectoryRecord(volume, dos, searchHash->DirNo);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error in directory read\n");
		return retCode;
	     }
	     nfs = (NFS *) dos;

	     // save the directory number for this hash
	     DirNo = searchHash->DirNo;

	     // free the hash first so its instance handle gets
	     // invalidated before clearing the directory entry.
	     FreeHashDirectoryRecord(volume, searchHash);

	     // now clear the directory record and mark it free in
	     // the dir block free lists
	     retCode = FreeDirectoryRecord(volume, dos, DirNo, Parent);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error freeing directory record\n");
		return retCode;
	     }
	     break;

	  case LONG_NAME_SPACE:
	     retCode = ReadDirectoryRecord(volume, dos, searchHash->DirNo);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error in directory read\n");
		return retCode;
	     }
	     longname = (LONGNAME *) dos;

	     // save the directory number for this hash
	     DirNo = searchHash->DirNo;

	     // free the hash first so its instance handle gets
	     // invalidated before clearing the directory entry.
	     FreeHashDirectoryRecord(volume, searchHash);

	     // now clear the directory record and mark it free in
	     // the dir block free lists
	     retCode = FreeDirectoryRecord(volume, dos, DirNo, Parent);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error freeing directory record\n");
		return retCode;
	     }
	     break;

	  case NT_NAME_SPACE:
	     retCode = ReadDirectoryRecord(volume, dos, searchHash->DirNo);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error in directory read\n");
		return retCode;
	     }
	     nt = (NTNAME *) dos;

	     // save the directory number for this hash
	     DirNo = searchHash->DirNo;

	     // free the hash first so its instance handle gets
	     // invalidated before clearing the directory entry.
	     FreeHashDirectoryRecord(volume, searchHash);

	     // now clear the directory record and mark it free in
	     // the dir block free lists
	     retCode = FreeDirectoryRecord(volume, dos, DirNo, Parent);
	     if (retCode)
	     {
		NWFSPrint("nwfs:  error freeing directory record\n");
		return retCode;
	     }
	     break;

	  default:
	     NWFSPrint("nwfs:  unknown directory record format\n");
	     break;
       }
    }
    return 0;

}

ULONG NWShadowDirectoryEntry(VOLUME *volume, DOS *dos, HASH *hash)
{
    return 0;
}

// we create an entirely new file entry during renames, even if
// we are not moving a file to another directory.  We do it this
// way so that in the event we get interrupted during the operation
// we will not lose the file.

ULONG NWRenameEntry(VOLUME *volume, DOS *oldDir, HASH *hash,
		    BYTE *Name, ULONG Len, ULONG Parent)
{
    register ULONG retCode, i, RootNS = (ULONG) -1, oldDirNo;
    register ULONG DirCount, RootDirNo, HashNumber = 0;
    register HASH *searchHash, *listHash;
    register DOS *dos;
    register SUB_DIRECTORY *subdir;
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *longname;
    register NTNAME *nt;
    register BYTE *Buffer;
    register VOLUME_WORKSPACE *wk;

    ULONG RecordNumber = 0;
    DOS *WorkSpace[MAX_NAMESPACES];
    HASH *HashEntries[MAX_NAMESPACES];
    ULONG Entries[MAX_NAMESPACES];
    ULONG PrevDirEntries[MAX_NAMESPACES];

    // the DOS namespace must always be the first record on the name list
    if (hash->NameSpace != DOS_NAME_SPACE)
       return NwHashCorrupt;

    oldDirNo = hash->DirNo;
    
    // if we are moving a subdirectory with files, then report error.
    // if the directory is empty, or we are renaming it, then allow
    // the operation.

    if ((hash->Flags & SUBDIRECTORY_FILE) && (hash->Parent != Parent))
    {
       // check if directory still has files in it
       if (hash->Blocks)
       {
	  // don't move directories unless they are empty
	  return NwNotEmpty;
       }
    }

    // see if name conforms to default namespace conventions
    switch (volume->NameSpaceDefault)
    {
       case DOS_NAME_SPACE:
	  retCode = NWValidDOSName(volume, Name, Len, Parent, 1);
	  if (retCode)
	     return retCode;
	  break;

       case MAC_NAME_SPACE:
	  retCode = NWValidMACName(volume, Name, Len, Parent, 1);
	  if (retCode)
	     return retCode;
	  break;

       case UNIX_NAME_SPACE:
	  retCode = NWValidNFSName(volume, Name, Len, Parent, 1);
	  if (retCode)
	     return retCode;
	  break;

       case LONG_NAME_SPACE:
       case NT_NAME_SPACE:
	  retCode = NWValidLONGName(volume, Name, Len, Parent, 1);
	  if (retCode)
	     return retCode;
	  break;

       // if we get here, then return error
       default:
	  return NwBadName;

    }

    // get enough memory for directory workspace
    wk = AllocateNSWorkspace(volume);
    if (!wk)
    {
       NWFSPrint("nwfs:  alloc failed in NWRename\n");
       return NwInsufficientResources;
    }
    Buffer = (BYTE *)&wk->Buffer[0];

    // initialize directory workspace array
    NWFSSet(Buffer, 0, sizeof(ROOT) * volume->NameSpaceCount);
    for (i=0; i < volume->NameSpaceCount; i++)
       WorkSpace[i] = (DOS *) &Buffer[i * sizeof(ROOT)];

    // read the current directory entries into the workspace.
    DirCount = 0;
    listHash = hash;
    while (listHash)
    {
       searchHash = listHash;
       listHash = listHash->nlnext;

       PrevDirEntries[DirCount] = searchHash->DirNo;
       retCode = ReadDirectoryRecord(volume, WorkSpace[DirCount],
				     searchHash->DirNo);
       if (retCode)
	  return retCode;

       DirCount++;
    }

    for (i=0; i < DirCount; i++)
    {
       dos = (DOS *) WorkSpace[i];
       switch (dos->NameSpace)
       {
	  case DOS_NAME_SPACE:
	     if (dos->Flags & SUBDIRECTORY_FILE)
	     {
		subdir = (SUB_DIRECTORY *) dos;
		retCode = NWMangleDOSName(volume, Name, Len, (DOS *)subdir, Parent);
		if (retCode)
		{
		   FreeNSWorkspace(volume, wk);
		   return retCode;
		}
		subdir->Subdirectory = Parent;
		if (Parent == (ULONG) -2) // going to a deleted block
		{
		}
		subdir->CreateDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());
		subdir->OwnerID = SUPERVISOR;
		subdir->LastModifiedDateAndTime = subdir->CreateDateAndTime;
	     }
	     else
	     {
		retCode = NWMangleDOSName(volume, Name, Len, dos, Parent);
		if (retCode)
		{
		   FreeNSWorkspace(volume, wk);
		   return retCode;
		}
		dos->Subdirectory = Parent;
		if (Parent == (ULONG) -2) // going to a deleted block
		{
		}
		dos->CreateDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());
		dos->LastUpdatedDateAndTime = dos->CreateDateAndTime;
		dos->LastUpdatedID = SUPERVISOR;
	     }
	     // record the root namespace index
	     RootNS = i;
	     break;

	  case MAC_NAME_SPACE:
	     mac = (MACINTOSH *) dos;
	     retCode = NWMangleMACName(volume, Name, Len, mac, Parent);
	     if (retCode)
	     {
		FreeNSWorkspace(volume, wk);
		return retCode;
	     }
	     mac->Subdirectory = Parent;
	     break;

	  case UNIX_NAME_SPACE:
	     nfs = (NFS *) dos;
	     retCode = NWMangleNFSName(volume, Name, Len, nfs, Parent);
	     if (retCode)
	     {
		FreeNSWorkspace(volume, wk);
		return retCode;
	     }
	     nfs->Subdirectory = Parent;
	     nfs->FileNameLength = (nfs->TotalFileNameLength <= MAX_NFS_NAME)
				 ? nfs->TotalFileNameLength : MAX_NFS_NAME;
	     break;

	  case LONG_NAME_SPACE:
	     longname = (LONGNAME *) dos;
	     retCode = NWMangleLONGName(volume, Name, Len, longname, Parent);
	     if (retCode)
	     {
		FreeNSWorkspace(volume, wk);
		return retCode;
	     }
	     longname->Subdirectory = Parent;
	     break;

	  case NT_NAME_SPACE:
	     nt = (NTNAME *) dos;
	     retCode = NWMangleLONGName(volume, Name, Len, (LONGNAME *)nt, Parent);
	     if (retCode)
	     {
		FreeNSWorkspace(volume, wk);
		return retCode;
	     }
	     nt->Subdirectory = Parent;
	     break;

	  default:
	     // this case is a fatal error.  we should never have
	     // an allocated record without a valid namespace
	     // to match up with it.

	     FreeNSWorkspace(volume, wk);
	     return NwVolumeCorrupt;
       }
    }

    // now we check if we are renaming a directory rather than moving
    // one.  We allow it in this implementation so long as they are empty.
    // since Netware always maintains child links to parents, but no
    // parent links to children, if you move a directory with files
    // you will orphan all the files.  The overhead involved in
    // deleting and reassigning the children for a particular directory
    // to the correctly assigned dir blocks is considerable to support
    // this, and has a great deal of overhead (you must read and write
    // each directory record and change it's parent link.)

    if ((hash->Flags & SUBDIRECTORY_FILE) && (hash->Parent == Parent))
    {
       register HASH *searchHash;
       BYTE *Name[MAX_NAMESPACES];
       ULONG Length[MAX_NAMESPACES];

       // save the previous name links, and clear the name field
       // we also remove the name from the name hashes

       for (searchHash = hash, i = 0; (searchHash) && (i < DirCount); i++)
       {
          RemoveNameHash(volume, searchHash);
          Name[i] = searchHash->Name;
          Length[i] = searchHash->NameLength;
          searchHash->Name = 0;
          searchHash->NameLength = 0;
          searchHash = searchHash->nlnext;
       }

       for (searchHash = hash, i = 0; (searchHash) && (i < DirCount); i++)
       {
          searchHash->Name = CreateNameField((DOS *)WorkSpace[i], searchHash);
          if (!searchHash->Name)
          {
             for (searchHash = hash, i = 0; (searchHash) && (i < DirCount); i++)
             {
                if (searchHash->Name)
                {
                   RemoveNameHash(volume, searchHash);
                   NWFSFree(searchHash->Name);
                }
	        searchHash->Name = Name[i];
		searchHash->NameLength = (WORD) Length[i];
                AddToNameHash(volume, searchHash);
                searchHash = searchHash->nlnext;
             }
	     FreeNSWorkspace(volume, wk);
	     return NwInsufficientResources;
          }
          AddToNameHash(volume, searchHash);
          searchHash = searchHash->nlnext;
       }

       // write the modified directory records to disk
       for (HashNumber = i = 0; i < DirCount; i++)
       {
	  retCode = WriteDirectoryRecord(volume,
					(DOS *)WorkSpace[i],
					PrevDirEntries[i]);
	  if (retCode)
	  {
             for (searchHash = hash, i = 0; (searchHash) && (i < DirCount); i++)
             {
                if (searchHash->Name)
                {
                   RemoveNameHash(volume, searchHash);
                   NWFSFree(searchHash->Name);
                }
	        searchHash->Name = Name[i];
		searchHash->NameLength = (WORD) Length[i];
                AddToNameHash(volume, searchHash);
                searchHash = searchHash->nlnext;
             }
	     FreeNSWorkspace(volume, wk);
	     return NwDiskIoError;
	  }
       }

       // free the workspace
       FreeNSWorkspace(volume, wk);

       // free the old hash name fields
       for (i = 0; i < DirCount; i++)
       {
          if (Name[i])
             NWFSFree(Name[i]);
       }
       return 0;
    }

    if (RootNS == (ULONG) -1)
    {
       NWFSPrint("nwfs:  could not determine root namespace NWRename\n");
       FreeNSWorkspace(volume, wk);
       return NwDirectoryCorrupt;
    }

    // allocate directory records necessary to create the file or directory
    retCode = CreateDirectoryRecords(volume, Entries, Parent, &RecordNumber);
    if (retCode)
    {
       NWFSPrint("nwfs:  dir entry alloc failed in NWRename\n");
       FreeNSWorkspace(volume, wk);
       return NwInsufficientResources;
    }

    // verify that the number of allocated directory records is equal
    // to the number of namespaces for the volume.  if not, return error.
    if (RecordNumber != DirCount)
    {
       NWFSPrint("nwfs:  allocated dir records != directory count in NWRename\n");

       retCode = FreeDirectoryRecords(volume, Entries, Parent, RecordNumber);
       if (retCode)
	  NWFSPrint("nwfs:  could not release dir record in NWRename\n");

       FreeNSWorkspace(volume, wk);
       return NwVolumeCorrupt;
    }

    // link allocated directory records together into a single namespace
    // chain
    for (RootDirNo = Entries[RootNS], i=0;
	 RecordNumber && (i < (RecordNumber - 1)); i++)
    {
       // store the DirNo for next record into NameList
       dos = (DOS *) WorkSpace[i];
       dos->NameList = Entries[i + 1];
       dos->PrimaryEntry = RootDirNo;
    }
    // null terminate last dir namespace record
    dos = (DOS *) WorkSpace[i];
    dos->NameList = 0;
    dos->PrimaryEntry = RootDirNo;

    // write and hash the initialized directory records
    for (i=0; i < RecordNumber; i++)
    {
       HashEntries[i] = AllocHashDirectoryRecord(volume,
						(DOS *)WorkSpace[i],
						Entries[i]);
       if (!HashEntries[i])
       {
	  retCode = FreeHashRecords(volume, HashEntries, HashNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release hash record in NWRename)\n");

	  retCode = FreeDirectoryRecords(volume, Entries, Parent, RecordNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release dir record in NWRename\n");

	  FreeNSWorkspace(volume, wk);
	  return NwInsufficientResources;
       }
       // count the number of name hash elements we create

       HashNumber++;

       retCode = WriteDirectoryRecord(volume,
				     (DOS *)WorkSpace[i],
				     Entries[i]);
       if (retCode)
       {
	  retCode = FreeHashRecords(volume, HashEntries, HashNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release hash record in NWRename)\n");

	  retCode = FreeDirectoryRecords(volume, Entries, Parent, RecordNumber);
	  if (retCode)
	     NWFSPrint("nwfs:  could not release dir record in NWRename\n");

	  FreeNSWorkspace(volume, wk);
	  return NwDiskIoError;
       }
    }

    // free the workspace
    FreeNSWorkspace(volume, wk);

    retCode = ReadDirectoryRecord(volume, oldDir, oldDirNo);
    if (retCode)
       return retCode;

    retCode = NWDeleteDirectoryEntry(volume, oldDir, hash);

    return retCode;

}

//
//
//

ULONG UtilInitializeDirAssignHash(VOLUME *volume);
ULONG UtilCreateDirAssignEntry(VOLUME *volume, ULONG Parent, ULONG BlockNo,
			   DOS *dos);
ULONG UtilFreeDirAssignHash(VOLUME *volume);
ULONG UtilAddToDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock);
ULONG UtilRemoveDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock);
ULONG UtilAllocateDirectoryRecord(VOLUME *volume, ULONG Parent);
ULONG UtilFreeDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo,
			      ULONG Parent);

ULONG UtilInitializeDirAssignHash(VOLUME *volume)
{
    volume->DirAssignBlocks = 0;
    volume->DirAssignHash = NWFSCacheAlloc(ASSIGN_BLOCK_HASH_SIZE,
					   ASSN_BLOCKHASH_TAG);

    if (!volume->DirAssignHash)
       return -1;

    volume->DirAssignHashLimit = NUMBER_OF_ASSIGN_BLOCK_ENTRIES;
    NWFSSet(volume->DirAssignHash, 0, ASSIGN_BLOCK_HASH_SIZE);

    return 0;
}

ULONG UtilCreateDirAssignEntry(VOLUME *volume, ULONG Parent, ULONG BlockNo,
			       DOS *dos)
{
    register ULONG DirsPerBlock;
    register DIR_ASSIGN_HASH *hash;
    register ULONG retCode, i;

    DirsPerBlock = volume->BlockSize / sizeof(ROOT);

    hash = NWFSAlloc(sizeof(DIR_ASSIGN_HASH) +
		     (DirsPerBlock / 8), ASSN_BLOCK_TAG);
    if (hash)
    {
       NWFSSet(hash, 0, sizeof(DIR_ASSIGN_HASH) + (DirsPerBlock / 8));

       hash->DirOwner = Parent;
       hash->BlockNo = BlockNo;

       // create directory record free list for this block

       for (i=0; i < DirsPerBlock; i++)
       {
	  if (dos[i].Subdirectory == (ULONG) -1)
	     hash->FreeList[i >> 3] &= ~(1 << (i & 7));
	  else
	     hash->FreeList[i >> 3] |= (1 << (i & 7));
       }

       retCode = UtilAddToDirAssignHash(volume, hash);
       if (retCode)
       {
	  NWFSFree(hash);
	  return -1;
       }
       return 0;
    }
    return -1;

}

ULONG UtilFreeDirAssignHash(VOLUME *volume)
{
    register DIR_ASSIGN_HASH_LIST *HashTable;
    register DIR_ASSIGN_HASH *list, *hash;
    register ULONG count;

    if (volume->DirAssignHash)
    {
       HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
       for (count=0; count < volume->DirAssignHashLimit; count++)
       {
	  list = (DIR_ASSIGN_HASH *) HashTable[count].head;
	  HashTable[count].head = HashTable[count].head = 0;
	  while (list)
	  {
	     hash = list;
	     list = list->next;
	     NWFSFree(hash);
	  }
       }
    }

    if (volume->DirAssignHash)
       NWFSFree(volume->DirAssignHash);
    volume->DirAssignHashLimit = 0;
    volume->DirAssignHash = 0;

    return 0;

}

ULONG UtilAddToDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock)
{
    register ULONG hash;
    register DIR_ASSIGN_HASH_LIST *HashTable;

    hash = (dblock->DirOwner & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (HashTable)
    {
       if (!HashTable[hash].head)
       {
	  HashTable[hash].head = dblock;
	  HashTable[hash].tail = dblock;
	  dblock->next = dblock->prior = 0;
       }
       else
       {
	  HashTable[hash].tail->next = dblock;
	  dblock->next = 0;
	  dblock->prior = HashTable[hash].tail;
	  HashTable[hash].tail = dblock;
       }
       return 0;
    }
    return -1;
}

ULONG UtilRemoveDirAssignHash(VOLUME *volume, DIR_ASSIGN_HASH *dblock)
{
    register ULONG hash;
    register DIR_ASSIGN_HASH_LIST *HashTable;

    hash = (dblock->DirOwner & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (HashTable)
    {
       if (HashTable[hash].head == dblock)
       {
	  HashTable[hash].head = dblock->next;
	  if (HashTable[hash].head)
	     HashTable[hash].head->prior = NULL;
	  else
	     HashTable[hash].tail = NULL;
       }
       else
       {
	  dblock->prior->next = dblock->next;
	  if (dblock != HashTable[hash].tail)
	     dblock->next->prior = dblock->prior;
	  else
	     HashTable[hash].tail = dblock->prior;
       }
       return 0;
    }
    return -1;
}

ULONG UtilAllocateDirectoryRecord(VOLUME *volume, ULONG Parent)
{
    register ULONG hash, i, DirsPerBlock, DirNo, DirsPerCluster;
    register DIR_ASSIGN_HASH_LIST *HashTable;
    register DIR_ASSIGN_HASH *list;
    register VOLUME_WORKSPACE *WorkSpace;
    register BYTE *cBuffer;
    register ULONG cbytes, retCode, block, BlockNo;
    register DOS *dos;
    register ULONG dirFileSize, dirBlocks;
    ULONG retRCode;

    DirsPerBlock = volume->BlockSize / sizeof(ROOT);

    hash = (Parent & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (!HashTable)
       return -1;

    list = (DIR_ASSIGN_HASH *) HashTable[hash].head;
    while (list)
    {
       if (list->DirOwner == Parent)
       {
	  for (i=0; i < DirsPerBlock; i++)
	  {
	     // if we locate a free entry in the bit list, set it
	     // then return the allocated DirNo
	     if (!(((list->FreeList[i >> 3]) >> (i & 7)) & 1))
	     {
		list->FreeList[i >> 3] |= (1 << (i & 7));
		DirNo = ((DirsPerBlock * list->BlockNo) + i);
		return DirNo;
	     }
	  }
       }
       list = list->next;
    }

    // if we got here, then we could not locate an assigned directory
    // block with available entries.  at this point, we scan the free
    // list hash (-1) and search free directory blocks for available
    // entries.  if we find a free block, then we assign this
    // dir block as owned by the specified parent.

    hash = ((ULONG)FREE_NODE & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (!HashTable)
       return -1;

    list = (DIR_ASSIGN_HASH *) HashTable[hash].head;
    while (list)
    {
       if (list->DirOwner == FREE_NODE)
       {
	  for (i=0; i < DirsPerBlock; i++)
	  {
	     // if we locate a free entry in the bit list, set it
	     // then return the allocated DirNo
	     if (!(((list->FreeList[i >> 3]) >> (i & 7)) & 1))
	     {
		// set directory entry as allocated
		list->FreeList[i >> 3] |= (1 << (i & 7));

		// re-hash block assign record to reflect
		// new parent
		UtilRemoveDirAssignHash(volume, list);
		list->DirOwner = Parent;
		UtilAddToDirAssignHash(volume, list);

		// calculate and return directory number
		DirNo = ((DirsPerBlock * list->BlockNo) + i);
		return DirNo;
	     }
	  }
       }
       list = list->next;
    }

    // if we got here, then the directory is completely full, and we need
    // to extend the directory by allocating another cluster for both the
    // primary and mirror copies of the directory.

    WorkSpace = AllocateWorkspace(volume);
    if (!WorkSpace)
       return -1;
    cBuffer = &WorkSpace->Buffer[0];

    // initialize new cluster and fill with free directory
    // entries
    NWFSSet(cBuffer, 0, volume->ClusterSize);

    DirsPerCluster = volume->ClusterSize / sizeof(ROOT);
    for (i=0; i < DirsPerCluster; i++)
    {
       dos = (DOS *) &cBuffer[i * sizeof(ROOT)];
       dos->Subdirectory = FREE_NODE;
    }

    // get directory file size
    dirFileSize = GetChainSize(volume, volume->FirstDirectory);
    dirBlocks = (dirFileSize + (volume->BlockSize - 1)) / volume->BlockSize;

    // write free cluster to the end of the primary directory chain
    cbytes = NWWriteFile(volume,
			    &volume->FirstDirectory,
			    0,
			    dirBlocks * volume->BlockSize,
			    cBuffer,
			    volume->ClusterSize,
			    0,
			    0,
			    &retRCode,
			    KERNEL_ADDRESS_SPACE,
			    0,
			    0);
    if (cbytes != volume->ClusterSize)
    {
       NWFSPrint("nwfs:  error extending primary directory file\n");
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

    // write free cluster to the end of the mirror directory chain
    cbytes = NWWriteFile(volume,
			    &volume->SecondDirectory,
			    0,
			    dirBlocks * volume->BlockSize,
			    cBuffer,
			    volume->ClusterSize,
			    0,
			    0,
			    &retRCode,
			    KERNEL_ADDRESS_SPACE,
			    0,
			    0);
    if (cbytes != volume->ClusterSize)
    {
       NWFSPrint("nwfs:  error extending mirror directory file\n");
       FreeWorkspace(volume, WorkSpace);
       return -1;
    }

    BlockNo = dirBlocks;
    for (block=0; block < volume->BlocksPerCluster; block++, BlockNo++)
    {
       dos = (DOS *) &cBuffer[block * volume->BlockSize];
       retCode = UtilCreateDirAssignEntry(volume, FREE_NODE, BlockNo, dos);
       if (retCode)
       {
	  NWFSPrint("nwfs:  could not allocate dir block Assign element during extend\n");
	  FreeWorkspace(volume, WorkSpace);
	  return -1;
       }
    }

    // free the workspace
    FreeWorkspace(volume, WorkSpace);

    // now try to scan the free list hash (-1) and search free
    // directory blocks for available entries.  if we find a free
    // block (we'd better, we just created one above), then we mark
    // this dir block as owned by the specified parent.  we should
    // never get here unless we successfully extended the directory file.

    hash = ((ULONG)FREE_NODE & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (!HashTable)
       return -1;

    list = (DIR_ASSIGN_HASH *) HashTable[hash].head;
    while (list)
    {
       if (list->DirOwner == FREE_NODE)
       {
	  for (i=0; i < DirsPerBlock; i++)
	  {
	     // if we locate a free entry in the bit list, set it
	     // then return the allocated DirNo
	     if (!(((list->FreeList[i >> 3]) >> (i & 7)) & 1))
	     {
		// set directory entry as allocated
		list->FreeList[i >> 3] |= (1 << (i & 7));

		// re-hash block assign record to reflect
		// new parent
		UtilRemoveDirAssignHash(volume, list);
		list->DirOwner = Parent;
		UtilAddToDirAssignHash(volume, list);

		// calculate and return directory number
		DirNo = ((DirsPerBlock * list->BlockNo) + i);
		return DirNo;
	     }
	  }
       }
       list = list->next;
    }
    return -1;

}

ULONG UtilFreeDirectoryRecord(VOLUME *volume, DOS *dos, ULONG DirNo,
			      ULONG Parent)
{
    register ULONG hash, i, count, SubdirNo;
    register ULONG DirsPerBlock, DirsPerCluster;
    register ULONG DirIndex, DirBlockNo;
    register ULONG retCode;
    register DIR_ASSIGN_HASH_LIST *HashTable;
    register DIR_ASSIGN_HASH *list;

    DirsPerCluster = volume->ClusterSize / sizeof(ROOT);
    DirsPerBlock = volume->BlockSize / sizeof(ROOT);
    DirBlockNo = DirNo / DirsPerBlock;
    DirIndex = DirNo % DirsPerBlock;

    hash = ((ULONG)Parent & (volume->DirAssignHashLimit - 1));
    HashTable = (DIR_ASSIGN_HASH_LIST *) volume->DirAssignHash;
    if (!HashTable)
       return NwHashCorrupt;

    SubdirNo = dos->Subdirectory;
    NWFSSet(dos, 0, sizeof(ROOT));
    dos->Subdirectory = FREE_NODE;      // mark node free

    retCode = WriteDirectoryRecord(volume, dos, DirNo);
    if (retCode)
    {
       NWFSPrint("nwfs:  error in directory write (%d)\n", (int)retCode);
       return retCode;
    }

    list = (DIR_ASSIGN_HASH *) HashTable[hash].head;
    while (list)
    {
       if ((list->BlockNo == DirBlockNo) &&
	   (list->DirOwner == Parent))
       {
	  // free directory record
	  list->FreeList[DirIndex >> 3] &= ~(1 << (DirIndex & 7));

	  // see if this block has any allocated dir entries, if so
	  // then exit, otherwise, re-hash the block into the
	  // free list

	  for (i=0; i < DirsPerBlock; i++)
	  {
	     // check for allocated entries
	     if (((list->FreeList[i >> 3]) >> (i & 7)) & 1)
		return 0;
	  }

	  // if we get here the we assume that the entire directory
	  // block is now free.  we remove this block from the
	  // previous assigned parent hash, then re-hash it into the
	  // directory free list (-1)

	  UtilRemoveDirAssignHash(volume, list);
	  list->DirOwner = FREE_NODE;
	  UtilAddToDirAssignHash(volume, list);
	  return 0;
       }
       list = list->next;
    }

    // if we could not locate the directory entry, then perform
    // a brute force search of the record in all of the parent
    // assign hash records.  Netware does allow parent records
    // to be mixed within a single block in rare circumstances.
    // this should rarely happen, however, and if it ever does
    // it indicates possible directory corruption.  In any case,
    // even if the directory is corrupt, we should allow folks
    // the ability to purge these records.

    for (count=0; count < volume->DirAssignHashLimit; count++)
    {
       list = (DIR_ASSIGN_HASH *) HashTable[count].head;
       while (list)
       {
	  if (list->BlockNo == DirBlockNo)
	  {
	     NWFSPrint("nwfs:  record freed in non-assigned parent block [%X/%X]\n",
		       (unsigned int)SubdirNo,
		       (unsigned int)list->DirOwner);

	     // free directory record
	     list->FreeList[DirIndex >> 3] &= ~(1 << (DirIndex & 7));

	     // see if this block has any allocated dir entries, if so
	     // then exit, otherwise, re-hash the block into the
	     // free list

	     for (i=0; i < DirsPerBlock; i++)
	     {
		// check for allocated entries
		if (((list->FreeList[i >> 3]) >> (i & 7)) & 1)
		   return 0;
	     }

	     // if we get here the we assume that the entire directory
	     // block is now free.  we remove this block from the
	     // previous assigned parent hash, then re-hash it into the
	     // directory free list (-1)

	     UtilRemoveDirAssignHash(volume, list);
	     list->DirOwner = FREE_NODE;
	     UtilAddToDirAssignHash(volume, list);
	     return 0;
	  }
	  list = list->next;
       }
    }
    return 0;
}

ULONG FormatNamespaceRecord(VOLUME *volume, BYTE *Name, ULONG Len,
			    ULONG Parent, ULONG NWFlags, ULONG Namespace,
			    DOS *dos)
{
    register ULONG retCode;
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *longname;
    register NTNAME *nt;

    NWFSSet(dos, 0, sizeof(DOS));

    switch (Parent)
    {
       case FREE_NODE:
       case SUBALLOC_NODE:
       case RESTRICTION_NODE:
       case TRUSTEE_NODE:
	  return -1;

       case ROOT_NODE:
	  switch (Namespace)
	  {
	     case LONG_NAME_SPACE:
		// create LONG_NAME_SPACE root directory record
		longname = (LONGNAME *) dos;
		longname->Flags = SUBDIRECTORY_FILE;
		longname->NameSpace = LONG_NAME_SPACE;
		longname->Subdirectory = ROOT_NODE;
		return 0;

	     case UNIX_NAME_SPACE:
		// create UNIX_NAME_SPACE root directory record
		nfs = (NFS *) dos;
		nfs->nlinks = 2;
		nfs->Flags = SUBDIRECTORY_FILE;
		nfs->NameSpace = UNIX_NAME_SPACE;
		nfs->Subdirectory = ROOT_NODE;
		return 0;

	     case MAC_NAME_SPACE:
		// create MAC_NAME_SPACE root directory record
		mac = (MACINTOSH *) dos;
		mac->Flags = SUBDIRECTORY_FILE;
		mac->NameSpace = MAC_NAME_SPACE;
		mac->Subdirectory = ROOT_NODE;
		mac->ResourceFork = (ULONG) -1;
		return 0;

	     case NT_NAME_SPACE:
		// create NT_NAME_SPACE root directory record
		nt = (NTNAME *) dos;
		nt->Flags = SUBDIRECTORY_FILE;
		nt->NameSpace = NT_NAME_SPACE;
		nt->Subdirectory = ROOT_NODE;
		return 0;

	     default:
		return -1;
	  }
	  break;

       default:
	  switch (Namespace)
	  {
	     case MAC_NAME_SPACE:
		mac = (MACINTOSH *) dos;
		retCode = NWCreateMACName(volume, Name, Len, mac, Parent);
		if (retCode)
		   return retCode;

		if (NWFlags & SUBDIRECTORY_FILE)
		   mac->Flags |= SUBDIRECTORY_FILE;
		else
		   mac->Flags |= (NWFlags | NAMED_FILE);
		mac->NameSpace = MAC_NAME_SPACE;
		mac->Subdirectory = Parent;
		mac->NameList = 0;
		mac->PrimaryEntry = 0;
		mac->ResourceForkSize = 0;
		mac->ResourceFork = (ULONG) -1;
		return 0;

	     case UNIX_NAME_SPACE:
		nfs = (NFS *) dos;
		retCode = NWCreateNFSName(volume, Name, Len, nfs, Parent);
		if (retCode)
		   return retCode;

		if (NWFlags & SUBDIRECTORY_FILE)
		{
		   nfs->nlinks = 2;
		   nfs->Flags |= SUBDIRECTORY_FILE;
		}
		else
		{
		   nfs->nlinks = 1;
		   nfs->Flags |= (NWFlags | NAMED_FILE);
		}
		if (NWFlags & SYMBOLIC_LINKED_FILE)
		   nfs->LinkedFlag = NFS_SYMBOLIC_LINK;
		else
		   nfs->LinkedFlag = 0;

		nfs->NameSpace = UNIX_NAME_SPACE;
		nfs->Subdirectory = Parent;
		nfs->FileNameLength = (nfs->TotalFileNameLength <= MAX_NFS_NAME)
				 ? nfs->TotalFileNameLength : MAX_NFS_NAME;
		nfs->gid = 0;
		nfs->uid = 0;
		nfs->mode = 0;
		nfs->rdev = 0;
		nfs->NameList = 0;
		nfs->PrimaryEntry = 0;
		return 0;

	     case LONG_NAME_SPACE:
		longname = (LONGNAME *) dos;
		retCode = NWCreateLONGName(volume, Name, Len, longname, Parent);
		if (retCode)
		   return retCode;

		if (NWFlags & SUBDIRECTORY_FILE)
		   longname->Flags |= SUBDIRECTORY_FILE;
		else
		   longname->Flags |= (NWFlags | NAMED_FILE);
		longname->NameSpace = LONG_NAME_SPACE;
		longname->Subdirectory = Parent;
		longname->NameList = 0;
		longname->PrimaryEntry = 0;
		return 0;

	     case NT_NAME_SPACE:
		nt = (NTNAME *) dos;
		retCode = NWCreateLONGName(volume, Name, Len, (LONGNAME *)nt, Parent);
		if (retCode)
		   return retCode;

		if (NWFlags & SUBDIRECTORY_FILE)
		   nt->Flags |= SUBDIRECTORY_FILE;
		else
		   nt->Flags |= (NWFlags | NAMED_FILE);
		nt->NameSpace = NT_NAME_SPACE;
		nt->Subdirectory = Parent;
		nt->NameList = 0;
		nt->PrimaryEntry = 0;
		return 0;

	     default:
		return -1;
       }
    }
}

extern BYTE *NSDescription(ULONG ns);

ULONG AddSpecificVolumeNamespace(VOLUME *volume, ULONG namespace)
{
    register ULONG Dir1Size, Dir2Size, i, j, k, ccode, owner;
    register ULONG DirCount, DirBlocks, cbytes, DirsPerBlock, DirTotal;
    register ULONG PrevLink, NameLink, NextLink, LinkCount;
    LONGLONG VolumeSpace, NameSpaceSize;
    register DOS *dos, *new, *prev, *name, *base;
    register MACINTOSH *mac;
    register NFS *nfs;
    register LONGNAME *longname;
    register NTNAME *nt;
    register TRUSTEE *trustee;
    register USER *user;
    register ROOT *root;
    register ROOT3X *root3x;
    register ULONG MacID, LongID, NfsID, DosID, NtID, NewDirNo;
    ULONG retRCode;
    BYTE FoundNameSpace[MAX_NAMESPACES];

    if (namespace == DOS_NAME_SPACE)
       return -1;

    ccode = MountRawVolume(volume);
    if (ccode)
       return NwVolumeCorrupt;

    dos = NWFSCacheAlloc(IO_BLOCK_SIZE, DIR_WORKSPACE_TAG);
    if (!dos)
    {
       DismountRawVolume(volume);
       return NwInsufficientResources;
    }

    new = NWFSCacheAlloc(IO_BLOCK_SIZE, DIR_WORKSPACE_TAG);
    if (!new)
    {
       NWFSFree(dos);
       DismountRawVolume(volume);
       return NwInsufficientResources;
    }
    prev     = (DOS *)&new[1];  // create temporary directory workspaces
    name     = (DOS *)&new[2];
    base     = (DOS *)&new[3];
    mac      = (MACINTOSH *)&new[4];
    nfs      = (NFS *)&new[5];
    longname = (LONGNAME *)&new[6];
    nt       = (NTNAME *)&new[7];

    Dir1Size = GetChainSize(volume, volume->FirstDirectory);
    Dir2Size = GetChainSize(volume, volume->SecondDirectory);

    if (!Dir1Size || !Dir2Size || (Dir1Size != Dir2Size))
    {
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return NwDirectoryCorrupt;
    }

    DirsPerBlock = volume->BlockSize / sizeof(ROOT);
    DirBlocks = (Dir1Size + (volume->BlockSize - 1)) / volume->BlockSize;
    DirTotal = (Dir1Size + (sizeof(DOS) - 1)) / sizeof(DOS);

    NWFSPrint("\nAnalyzing space requirements for the %s namespace ...\n",
	      NSDescription(namespace));

    ccode = UtilInitializeDirAssignHash(volume);
    if (ccode)
    {
       NWFSPrint("nwfs:  error during assign hash init\n");
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return -1;
    }

    for (DirCount = i = 0; i < DirBlocks; i++)
    {
       cbytes = NWReadFile(volume,
			      &volume->FirstDirectory,
			      0,
			      Dir1Size,
			      i * volume->BlockSize,
			      (BYTE *)dos,
			      volume->BlockSize,
			      0,
			      0,
			      &retRCode,
			      KERNEL_ADDRESS_SPACE,
			      0,
                              0,
			      TRUE);
       if (cbytes != volume->BlockSize)
       {
	  NWFSPrint("nwfs:  error reading directory file\n");
	  UtilFreeDirAssignHash(volume);
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  NWFSFree(new);
	  return -1;
       }

       for (owner = (ULONG) -1, j=0; j < DirsPerBlock; j++)
       {
	  switch (dos[j].Subdirectory)
	  {
	     case ROOT_NODE:
		if (owner == (ULONG) -1)
		   owner = 0;
		root = (ROOT *) &dos[j];
		if (root->NameSpace == DOS_NAME_SPACE)
		   DirCount++;
		break;

	     case SUBALLOC_NODE:
		if (owner == (ULONG) -1)
		   owner = 0;
		break;

	     case FREE_NODE:
		break;

	     case RESTRICTION_NODE:
		user = (USER *) &dos[j];
		if (owner == (ULONG) -1)
		   owner = 0;
		break;

	     case TRUSTEE_NODE:
		trustee = (TRUSTEE *) &dos[j];
		if (owner == (ULONG) -1)
		   owner = 0;
		break;

	     default:
		if (owner == (ULONG) -1)
		   owner = dos[j].Subdirectory;

		if (dos[j].NameSpace == DOS_NAME_SPACE)
		   DirCount++;
		break;
	  }
       }

       ccode = UtilCreateDirAssignEntry(volume, owner, i, dos);
       if (ccode)
       {
	  NWFSPrint("nwfs:  error creating dir assign hash\n");
	  UtilFreeDirAssignHash(volume);
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  NWFSFree(new);
	  return -1;
       }
    }

    VolumeSpace = (LONGLONG)((LONGLONG)volume->VolumeAllocatedClusters *
			     (LONGLONG)volume->ClusterSize);
    NameSpaceSize = (LONGLONG)((LONGLONG)DirCount * (LONGLONG)sizeof(ROOT));

    if (VolumeSpace >= 1024)
       NWFSPrint("%u K of free space available on volume %s\n",
	     (unsigned int)(VolumeSpace / 1024), volume->VolumeName);
    else
       NWFSPrint("%u bytes of free space available on volume %s\n",
	     (unsigned int)VolumeSpace, volume->VolumeName);

    if (NameSpaceSize >= 1024)
       NWFSPrint("%u K directory space required to add the %s namespace\n",
	     (unsigned int)(NameSpaceSize / 1024), NSDescription(namespace));
    else
       NWFSPrint("%u bytes directory space required to add the %s namespace\n",
	     (unsigned int)NameSpaceSize, NSDescription(namespace));

    if (NameSpaceSize >= (VolumeSpace + sizeof(ROOT)))
    {
       NWFSPrint("Insufficient space available to add namespace %s\n",
		 NSDescription(namespace));
       UtilFreeDirAssignHash(volume);
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return -1;
    }

    NWFSPrint("\nAdding %s namespace to volume [%-15s]\n",
	      NSDescription(namespace), volume->VolumeName);

    // read the root directory record
    root = (ROOT *) dos;
    root3x = (ROOT3X *) dos;
    ccode = ReadDirectoryRecord(volume, (DOS *)root, 0);
    if (ccode)
    {
       NWFSPrint("nwfs:  error reading volume root\n");
       UtilFreeDirAssignHash(volume);
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return -1;
    }

    // see if this is a Netware 4.x/5.x volume
    if ((volume->VolumeFlags & NDS_FLAG) ||
	(volume->VolumeFlags & NEW_TRUSTEE_COUNT))
    {
       // check for duplicate namespace entries
       NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (volume->NameSpaceID[j] == namespace)
	  {
	     NWFSPrint("nwfs:  duplicate namespace error\n");
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  if (!j && volume->NameSpaceID[j])
	  {
	     NWFSPrint("nwfs:  dos namespace not present\n");
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  if (j && (FoundNameSpace[volume->NameSpaceID[j] & 0xF]))
	  {
	     NWFSPrint("nwfs:  multiple namespace error\n");
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  FoundNameSpace[volume->NameSpaceID[j] & 0xF] = TRUE;
       }

       // add the namespace to the list in the volume table
       volume->NameSpaceID[volume->NameSpaceCount] = namespace;
       volume->NameSpaceCount++;
       root->NameSpaceCount = (BYTE) volume->NameSpaceCount;

       NWFSSet(&root->SupportedNameSpacesNibble[0], 0, 6);
       for (j=0, k=0; j < volume->NameSpaceCount; j++)
       {
	  if (j & 1)
	  {
	     root->SupportedNameSpacesNibble[k] |=
		((volume->NameSpaceID[j] << 4) & 0xF0);
	     k++;
	  }
	  else
	  {
	     root->SupportedNameSpacesNibble[k] |=
		(volume->NameSpaceID[j] & 0xF);
	  }
       }
    }
    else
    {
       // check for duplicate namespace entries
       NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (volume->NameSpaceID[j] == namespace)
	  {
	     NWFSPrint("nwfs:  duplicate namespace error\n");
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  if (!j && volume->NameSpaceID[j])
	  {
	     NWFSPrint("nwfs:  dos namespace not present\n");
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  if (j && (FoundNameSpace[volume->NameSpaceID[j] & 0xF]))
	  {
	     NWFSPrint("nwfs:  multiple namespace error\n");
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
	  FoundNameSpace[volume->NameSpaceID[j] & 0xF] = TRUE;
       }

       // add the namespace to the list in the volume table
       volume->NameSpaceID[volume->NameSpaceCount] = namespace;
       volume->NameSpaceCount++;
       root3x->NameSpaceCount = (BYTE) volume->NameSpaceCount;

       // clear the current namespace entries in the root
       // entry and reset.

       NWFSSet(&root3x->NameSpaceTable[0], 0, 10);
       for (j=0; j < volume->NameSpaceCount; j++)
	  root3x->NameSpaceTable[j] = (BYTE)volume->NameSpaceID[j];
    }

    // write the modified root entry
    ccode = WriteDirectoryRecord(volume, (DOS *)root, 0);
    if (ccode)
    {
       NWFSPrint("nwfs:  error writing volume root\n");
       UtilFreeDirAssignHash(volume);
       DismountRawVolume(volume);
       NWFSFree(dos);
       NWFSFree(new);
       return -1;
    }

    // now add the new namespace records.  DirTotal should be the upward
    // limit of detected DOS namespace records.  While we are allocating
    // new records to add the namespace, the directory file will expand
    // beyond this point, but this should be ok.

    for (i=0; i < DirTotal; i++)
    {
       ccode = ReadDirectoryRecord(volume, dos, i);
       if (ccode)
       {
	  NWFSPrint("nwfs:  error reading volume directory\n");
	  UtilFreeDirAssignHash(volume);
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  NWFSFree(new);
	  return -1;
       }

       NtID = MacID = DosID = LongID = NfsID = (ULONG) -1;
       if ((dos->Subdirectory != FREE_NODE)        &&
	   (dos->Subdirectory != SUBALLOC_NODE)    &&
	   (dos->Subdirectory != RESTRICTION_NODE) &&
	   (dos->Subdirectory != TRUSTEE_NODE)     &&
	   (dos->NameSpace == DOS_NAME_SPACE))
       {
	  // copy the DOS root entry and store in temporary workspace
	  NWFSCopy(base, dos, sizeof(DOS));

	  NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
	  PrevLink = i;
	  LinkCount = 1;
	  NameLink = dos->NameList;
	  DosID = i;

	  // run the entire name list chain and look for duplicate
	  // namespace records.  Also check for invalid name list
	  // linkage.

	  while (NameLink)
	  {
	     if (LinkCount++ > volume->NameSpaceCount)
	     {
		NWFSPrint("nwfs:  invalid name list detected (add)\n");
		UtilFreeDirAssignHash(volume);
		DismountRawVolume(volume);
		NWFSFree(dos);
		NWFSFree(new);
		return -1;
	     }

	     if (NameLink > DirTotal)
	     {
		NWFSPrint("nwfs:  directory number out of range (add namespace)\n");
		UtilFreeDirAssignHash(volume);
		DismountRawVolume(volume);
		NWFSFree(dos);
		NWFSFree(new);
		return -1;
	     }

	     if (FoundNameSpace[dos->NameSpace & 0xF])
	     {
		NWFSPrint("nwfs:  multiple name space error\n");
		UtilFreeDirAssignHash(volume);
		DismountRawVolume(volume);
		NWFSFree(dos);
		NWFSFree(new);
		return -1;
	     }
	     FoundNameSpace[dos->NameSpace & 0xF] = TRUE;

	     ccode = ReadDirectoryRecord(volume, dos, NameLink);
	     if (ccode)
	     {
		NWFSPrint("nwfs:  error reading volume directory\n");
		UtilFreeDirAssignHash(volume);
		DismountRawVolume(volume);
		NWFSFree(dos);
		NWFSFree(new);
		return -1;
	     }

	     // record directory numbers for each namespace we encounter
	     // and make a copy of each of the diretory records
	     switch (dos->NameSpace)
	     {
		case MAC_NAME_SPACE:
		   if (MacID == (ULONG) -1)
		      MacID = NameLink;
		   NWFSCopy(mac, dos, sizeof(DOS));
		   break;

		case LONG_NAME_SPACE:
		   if (LongID == (ULONG) -1)
		      LongID = NameLink;
		   NWFSCopy(longname, dos, sizeof(DOS));
		   break;

		case NT_NAME_SPACE:
		   if (NtID == (ULONG) -1)
		      NtID = NameLink;
		   NWFSCopy(nt, dos, sizeof(DOS));
		   break;

		case UNIX_NAME_SPACE:
		   if (NfsID == (ULONG) -1)
		      NfsID = NameLink;
		   NWFSCopy(nfs, dos, sizeof(DOS));
		   break;

		default:
		   break;
	     }

	     // save the link to the next namespace record
	     NextLink = dos->NameList;
	     PrevLink = NameLink;
	     NameLink = NextLink;
	  }

	  // select a name record to use to create the name for the new
	  // record.  If we have a LONG or NFS namespace, use this
	  // namespace to provide the name for the new record, otherwise
	  // default to NT and MAC namespaces.  If none of the cases
	  // are detected, then by default, we use the DOS namespace.

	  if ((namespace != LONG_NAME_SPACE) && (LongID != (ULONG) -1))
	  {
	     // format the new namespace record
	     ccode = FormatNamespaceRecord(volume,
					   longname->FileName,
					   longname->FileNameLength,
					   base->Subdirectory,
					   base->Flags,
					   namespace,
					   new);
	  }
	  else
	  if ((namespace != UNIX_NAME_SPACE) && (NfsID != (ULONG) -1))
	  {
	     // format the new namespace record
	     ccode = FormatNamespaceRecord(volume,
					   nfs->FileName,
					   nfs->TotalFileNameLength,
					   base->Subdirectory,
					   base->Flags,
					   namespace,
					   new);
	  }
	  else
	  if ((namespace != NT_NAME_SPACE) && (NtID != (ULONG) -1))
	  {
	     // format the new namespace record
	     ccode = FormatNamespaceRecord(volume,
					   nt->FileName,
					   nt->FileNameLength,
					   base->Subdirectory,
					   base->Flags,
					   namespace,
					   new);
	  }
	  else
	  {
	     // format the new namespace record
	     ccode = FormatNamespaceRecord(volume,
					   base->FileName,
					   base->FileNameLength,
					   base->Subdirectory,
					   base->Flags,
					   namespace,
					   new);
	  }

	  // save a copy of the last namespace record in this chain.
	  NWFSCopy(prev, dos, sizeof(DOS));

	  if (ccode)
	  {
	     NWFSPrint("nwfs:  error formatting namespace record\n");
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }

	  // now try to allocate a new directory record for this namespace
	  // entry.

	  if (base->Subdirectory == ROOT_NODE)
	     NewDirNo = UtilAllocateDirectoryRecord(volume, 0);
	  else
	     NewDirNo = UtilAllocateDirectoryRecord(volume, base->Subdirectory);

	  if (NewDirNo == (ULONG) -1)
	  {
	     NWFSPrint("nwfs:  error allocating directory record\n");
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }

	  // set the last record in the namespace chain to point to this
	  // record and set the new record to point to the primary entry.

	  prev->NameList = NewDirNo;
	  new->NameList = 0;
	  new->PrimaryEntry = prev->PrimaryEntry;

	  ccode = WriteDirectoryRecord(volume, prev, PrevLink);
	  if (ccode)
	  {
	     NWFSPrint("nwfs:  error writing volume directory\n");
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }

	  ccode = WriteDirectoryRecord(volume, new, NewDirNo);
	  if (ccode)
	  {
	     NWFSPrint("nwfs:  error writing volume directory\n");
	     UtilFreeDirAssignHash(volume);
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     NWFSFree(new);
	     return -1;
	  }
       }
    }
    UtilFreeDirAssignHash(volume);
    DismountRawVolume(volume);
    NWFSFree(dos);
    NWFSFree(new);

    NWFSPrint("\n%s namespace added successfully to volume %s\n",
	      NSDescription(namespace), volume->VolumeName);

    return 0;
}

ULONG RemoveSpecificVolumeNamespace(VOLUME *volume, ULONG namespace)
{
    register ULONG Dir1Size, Dir2Size, i, j, k, ccode, pFlag;
    register ULONG DirTotal, PrevLink, NameLink, NextLink, LinkCount;
    register DOS *dos;
    register ROOT *root;
    register ROOT3X *root3x;
    BYTE FoundNameSpace[MAX_NAMESPACES];

    if (namespace == DOS_NAME_SPACE)
       return -1;

    ccode = MountRawVolume(volume);
    if (ccode)
       return NwVolumeCorrupt;

    dos = NWFSCacheAlloc(sizeof(ROOT), DIR_WORKSPACE_TAG);
    if (!dos)
    {
       DismountRawVolume(volume);
       return NwInsufficientResources;
    }

    Dir1Size = GetChainSize(volume, volume->FirstDirectory);
    Dir2Size = GetChainSize(volume, volume->SecondDirectory);

    if (!Dir1Size || !Dir2Size || (Dir1Size != Dir2Size))
    {
       DismountRawVolume(volume);
       NWFSFree(dos);
       return NwDirectoryCorrupt;
    }

    DirTotal = Dir1Size / sizeof(ROOT);

    NWFSPrint("\nRemoving %s namespace from volume %s\n",
	      NSDescription(namespace), volume->VolumeName);

    // read the root directory record
    root = (ROOT *) dos;
    root3x = (ROOT3X *) dos;
    ccode = ReadDirectoryRecord(volume, (DOS *)root, 0);
    if (ccode)
    {
       NWFSPrint("nwfs:  error reading volume root\n");
       DismountRawVolume(volume);
       NWFSFree(dos);
       return -1;
    }

    // see if this is a Netware 4.x/5.x volume
    if ((volume->VolumeFlags & NDS_FLAG) ||
	(volume->VolumeFlags & NEW_TRUSTEE_COUNT))
    {
       // check for duplicate namespace entries
       NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (!j && volume->NameSpaceID[j])
	  {
	     NWFSPrint("nwfs:  dos namespace not present\n");
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     return -1;
	  }
	  if (j && (FoundNameSpace[volume->NameSpaceID[j] & 0xF]))
	  {
	     NWFSPrint("nwfs:  multiple namespace error\n");
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     return -1;
	  }
	  FoundNameSpace[volume->NameSpaceID[j] & 0xF] = TRUE;
       }

       // remove the namespace from the list in the volume table
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (volume->NameSpaceID[j] == namespace)
	  {
	     for (; j < volume->NameSpaceCount; j++)
	     {
		if ((j + 1) >= volume->NameSpaceCount)
		   volume->NameSpaceID[j] = 0;
		else
		   volume->NameSpaceID[j] = volume->NameSpaceID[j + 1];
	     }
	  }
       }

       // don't go negative if our count is zero
       if (volume->NameSpaceCount)
	  volume->NameSpaceCount--;

       // if only the DOS namespace is present, then we set the
       // namespace count to zero (Netware specific behavior).
       if (volume->NameSpaceCount == 1)
	  root->NameSpaceCount = 0;
       else
	  root->NameSpaceCount = (BYTE) volume->NameSpaceCount;


       // clear the current namespace entries in the root
       // entry and reset.

       NWFSSet(&root->SupportedNameSpacesNibble[0], 0, 6);
       for (j=0, k=0; j < volume->NameSpaceCount; j++)
       {
	  if (j & 1)
	  {
	     root->SupportedNameSpacesNibble[k] |=
		((volume->NameSpaceID[j] << 4) & 0xF0);
	     k++;
	  }
	  else
	  {
	     root->SupportedNameSpacesNibble[k] |=
		(volume->NameSpaceID[j] & 0xF);
	  }
       }
    }
    else
    {
       // check for duplicate namespace entries
       NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (!j && volume->NameSpaceID[j])
	  {
	     NWFSPrint("nwfs:  dos namespace not present\n");
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     return -1;
	  }
	  if (j && (FoundNameSpace[volume->NameSpaceID[j] & 0xF]))
	  {
	     NWFSPrint("nwfs:  multiple namespace error\n");
	     DismountRawVolume(volume);
	     NWFSFree(dos);
	     return -1;
	  }
	  FoundNameSpace[volume->NameSpaceID[j] & 0xF] = TRUE;
       }

       // remove the namespace from the list in the volume table
       for (j=0; j < volume->NameSpaceCount; j++)
       {
	  if (volume->NameSpaceID[j] == namespace)
	  {
	     for (; j < volume->NameSpaceCount; j++)
	     {
		if ((j + 1) >= volume->NameSpaceCount)
		   volume->NameSpaceID[j] = 0;
		else
		   volume->NameSpaceID[j] = volume->NameSpaceID[j + 1];
	     }
	  }
       }

       // don't go negative if our count is zero
       if (volume->NameSpaceCount)
	  volume->NameSpaceCount--;

       // if only the DOS namespace is present, then we set the
       // namespace count to zero (Netware specific behavior).
       if (volume->NameSpaceCount == 1)
	  root3x->NameSpaceCount = 0;
       else
	  root3x->NameSpaceCount = (BYTE) volume->NameSpaceCount;

       // clear the current namespace entries in the root
       // entry and reset.

       NWFSSet(&root3x->NameSpaceTable[0], 0, 10);
       for (j=0; j < volume->NameSpaceCount; j++)
	  root3x->NameSpaceTable[j] = (BYTE)volume->NameSpaceID[j];
    }

    // write the modified root entry
    ccode = WriteDirectoryRecord(volume, (DOS *)root, 0);
    if (ccode)
    {
       NWFSPrint("nwfs:  error writing volume root\n");
       DismountRawVolume(volume);
       NWFSFree(dos);
       return -1;
    }

    // now remove the affected namespace records
    for (i=0; i < DirTotal; i++)
    {
       ccode = ReadDirectoryRecord(volume, dos, i);
       if (ccode)
       {
	  NWFSPrint("nwfs:  error reading volume directory\n");
	  DismountRawVolume(volume);
	  NWFSFree(dos);
	  return -1;
       }

       if ((dos->Subdirectory != FREE_NODE)        &&
	   (dos->Subdirectory != SUBALLOC_NODE)    &&
	   (dos->Subdirectory != RESTRICTION_NODE) &&
	   (dos->Subdirectory != TRUSTEE_NODE)     &&
	   (dos->NameSpace == DOS_NAME_SPACE))
       {
	  NWFSSet(&FoundNameSpace[0], 0, MAX_NAMESPACES);
	  pFlag = 0;
	  PrevLink = i;
	  LinkCount = 1;
	  NameLink = dos->NameList;

	  while (NameLink)
	  {
	     if (LinkCount++ > volume->NameSpaceCount)
	     {
		NWFSPrint("nwfs:  invalid name list detected (add)\n");
		UtilFreeDirAssignHash(volume);
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }

	     if (NameLink > DirTotal)
	     {
		NWFSPrint("nwfs:  directory number out of range (remove)\n");
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }

	     if (FoundNameSpace[dos->NameSpace & 0xF])
	     {
		NWFSPrint("nwfs:  multiple name space error\n");
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }
	     FoundNameSpace[dos->NameSpace & 0xF] = TRUE;

	     ccode = ReadDirectoryRecord(volume, dos, NameLink);
	     if (ccode)
	     {
		NWFSPrint("nwfs:  error reading volume directory\n");
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }

	     // save the link to the next namespace record
	     NextLink = dos->NameList;

	     if (dos->NameSpace == namespace)
	     {
		// if this record is designated as primary namespace,
		// then we must set the DOS_NAME_SPACE record as the
		// new primary.
		if (dos->Flags & PRIMARY_NAMESPACE)
		   pFlag = TRUE;

		// free this record
		NWFSSet(dos, 0, sizeof(DOS));
		dos->Subdirectory = (ULONG) FREE_NODE;

		// mark this record as free
		ccode = WriteDirectoryRecord(volume, dos, NameLink);
		if (ccode)
		{
		   NWFSPrint("nwfs:  error writing volume directory\n");
		   DismountRawVolume(volume);
		   NWFSFree(dos);
		   return -1;
		}

		// read the previous record
		ccode = ReadDirectoryRecord(volume, dos, PrevLink);
		if (ccode)
		{
		   NWFSPrint("nwfs:  error reading volume directory\n");
		   DismountRawVolume(volume);
		   NWFSFree(dos);
		   return -1;
		}

		// update previous namespace forward link to the next link
		// for this namespace record
		dos->NameList = NextLink;

		// write the previous record
		ccode = WriteDirectoryRecord(volume, dos, PrevLink);
		if (ccode)
		{
		   NWFSPrint("nwfs:  error writing volume directory\n");
		   DismountRawVolume(volume);
		   NWFSFree(dos);
		   return -1;
		}
		break;
	     }
	     PrevLink = NameLink;
	     NameLink = NextLink;
	  }

	  // if we are removing a namespace record that was designated
	  // as PRIMARY_NAMESPACE, then set the DOS_NAME_SPACE entry
	  // as the new primary namespace record.

	  if (pFlag)
	  {
	     ccode = ReadDirectoryRecord(volume, dos, i);
	     if (ccode)
	     {
		NWFSPrint("nwfs:  error reading volume directory\n");
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }

	     dos->Flags |= PRIMARY_NAMESPACE;
	     ccode = WriteDirectoryRecord(volume, dos, i);
	     if (ccode)
	     {
		NWFSPrint("nwfs:  error writing volume directory\n");
		DismountRawVolume(volume);
		NWFSFree(dos);
		return -1;
	     }
	  }
       }
    }
    DismountRawVolume(volume);
    NWFSFree(dos);

    NWFSPrint("\n%s namespace successfully removed from volume %s\n",
	      NSDescription(namespace), volume->VolumeName);
    return 0;
}

