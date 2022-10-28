
/***************************************************************************
*
*   Copyright (c) 1997-2022 Jeff V. Merkey  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWDISK.C
*   DESCRIP  :  NWDISK Utility
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




