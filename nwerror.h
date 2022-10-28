
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
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWERROR.H
*   DESCRIP  :  Error Codes
*   DATE     :  April 3, 2022
*
*
***************************************************************************/

#ifndef _NWFS_ERROR_
#define _NWFS_ERROR_

/**********************************************************************
*
*   NWFS Error Codes
*
***********************************************************************/

typedef enum _NW_STATUS
{
   NwSuccess,                     // 0
   NwFileCorrupt,                 // 1
   NwDirectoryCorrupt,            // 2
   NwVolumeCorrupt,               // 3
   NwHashCorrupt,                 // 4
   NwHashError,                   // 5
   NwDiskIoError,                 // 6
   NwInsufficientResources,       // 7
   NwInvalidParameter,            // 8
   NwNoMoreMirrors,               // 9
   NwOtherError,                  // 10
   NwEndOfFile,                   // 11
   NwFileExists,                  // 12
   NwBadName,                     // 13
   NwVolumeFull,                  // 14
   NwFatCorrupt,                  // 15
   NwAccessError,                 // 16
   NwMissingSegment,              // 17
   NwMirrorGroupFailure,          // 18
   NwNotEmpty,                    // 19
   NwNoEntry,                     // 20
   NwNotPermitted,                // 21
   NwSuballocMissing,             // 22
   NwSuballocExceeded,            // 23
   NwMemoryFault,                 // 24
   NwChainBad                     // 25	   
} NW_STATUS;


#endif

