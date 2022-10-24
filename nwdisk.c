
/***************************************************************************
*
*   Copyright (c) 1998, 2022 Jeff V. Merkey  All Rights
*			    Reserved.
*
*   This program is an unpublished work of TRG, Inc. and contains
*   trade secrets and other proprietary information.  Unauthorized
*   use, copying, disclosure, or distribution of this file without the
*   consent of TRG, Inc. can subject the offender to severe criminal
*   and/or civil penalities.
*
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWDISK.C
*   DESCRIP  :   NWDISK Utility
*   DATE     :  September 23, 2022
*
*
***************************************************************************/

#include "globals.h"

int main(int argc, char *argv[])
{
    InitNWFS();
    NWEditVolumes();
    ClearScreen(0);

    NWFSVolumeClose();
    RemoveDiskDevices();
    FreePartitionResources();
    return 0;

}




