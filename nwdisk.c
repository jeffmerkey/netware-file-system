
/***************************************************************************
*
*   Copyright (c) 1998, 1999 Jeff V. Merkey  All Rights
*			    Reserved.
*
*   This program is an unpublished work of TRG, Inc. and contains
*   trade secrets and other proprietary information.  Unauthorized
*   use, copying, disclosure, or distribution of this file without the
*   consent of TRG, Inc. can subject the offender to severe criminal
*   and/or civil penalities.
*
*
*   AUTHOR   :  Jeff V. Merkey (jmerkey@utah-nac.org)
*   FILE     :  NWDISK.C
*   DESCRIP  :  FENRIS NWDISK Utility
*   DATE     :  September 23, 1999
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




