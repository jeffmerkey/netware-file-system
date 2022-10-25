
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
*
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  IOCTL.C
*   DESCRIP  :   VFS IOCTL Module for Linux
*   DATE     :  December 6, 1998
*
*
***************************************************************************/

#include "globals.h"

#if (LINUX_20 | LINUX_22 | LINUX_24)

int nwfs_ioctl(struct inode *inode, struct file *filp,
		 unsigned int cmd, unsigned long arg)
{
//    DOS DOS_S;
//    register DOS *dos = &DOS_S;
    ULONG Attributes;
    
#if (VERBOSE)
    NWFSPrint("nwfs-ioctl\n");
#endif

    switch (cmd)
    {
       case NWFS_GET_ATTRIBUTES:
	  if (NWFSCopyToUserSpace((void *)arg, (void *)&Attributes, 4)) 
             return -EFAULT;
	  return 0;

       case NWFS_SET_ATTRIBUTES:
	  if (NWFSCopyFromUserSpace((void *)&Attributes, (void *)arg, 4))
	     return -EFAULT;
	  return 0;
       
       case NWFS_GET_TRUSTEES:
       case NWFS_GET_QUOTA:
       case NWFS_SET_TRUSTEES:
       case NWFS_SET_QUOTA:
	  return 0;

       default:
          return -EINVAL;
    }
}

#endif
