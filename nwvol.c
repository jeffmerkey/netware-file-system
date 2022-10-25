
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
*
****************************************************************************
*
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWVOL.C
*   DESCRIP  :   VOlume DIsplay Utility
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#include "globals.h"

int main(int argc, char *argv[])
{
    extern void UtilScanVolumes(void);
    
    printf("NWVOL\n");
    printf("Copyright (C) 1998-1999 Jeff V. Merkey\n");
    printf("All Rights Reserved\n\n");

    InitNWFS();
    UtilScanVolumes();
    displayNetwareVolumes();
    NWFSVolumeClose();
    RemoveDiskDevices();
    FreePartitionResources();
    printf("\n");
    return 0;

}




