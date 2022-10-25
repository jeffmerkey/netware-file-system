
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
*   FILE     :  SUPER.C
*   DESCRIP  :   VFS Super Block Module for Linux*   DATE     :  November 16, 1998
*
*
***************************************************************************/

#define NWFSMOD_VERSION_INFO 1
#include "globals.h"


#if (LINUX_24)

int nwfs_remount(struct super_block *sb, int *flags, char *data)
{
    return 0;
}

struct super_block *nwfs_read_super(struct super_block *sb, void *data, int silent)
{
    register VOLUME *volume;
    BYTE name[64];
    register BYTE *p, *n;
    register ULONG count, AutoRepair = FALSE;
    kdev_t dev = sb->s_dev;

#if (VERBOSE)
    NWFSPrint("read super\n");
#endif
    
    if (!data)
    {
       NWFSPrint("nwfs:  missing VOLUME_NAME\n");
       NWFSPrint("usage: mount [name] [/mount_point] -t nwfs -o VOLUME_NAME(+)\n");
       NWFSPrint("       where VOLUME_NAME is the name of a NetWare file system Volume\n");
       NWFSPrint("       and '+' enables the driver to auto-repair volume errors during volume mount\n");
       NWFSPrint("       invoke NWVOL for a listing of detected NetWare Volumes in\n");
       NWFSPrint("       this system.\n\n");
       NWFSPrint("        Utilities for Linux are:\n");
       NWFSPrint("       NWVOL            - List detected NetWare volumes\n");
       NWFSPrint("       NWDISK/NWCONFIG  - File System/Partition Config Utility\n");
       sb->s_dev = 0;
       return NULL;
    }

    NWFSVolumeScan();

    count = 0; 
    p = (BYTE *) data;
    n = &name[0];
    while (*p && (*p != ' ') && (*p != '\r') && (*p != '\n') && (*p != '+') && 
	  (count < 16))
    {
       *n++ = *p++;
       count++;
    }
    *n = '\0';

    if (*p == '+')
    {
        NWFSPrint("nwfs:  volume auto-repair enabled\n");
        AutoRepair = TRUE;
    }
    
    volume = MountHandledVolume(name);
    if (!volume)
    {
       NWFSPrint("nwfs:  error mounting volume %s\n", name);
       sb->s_dev = 0;
       return NULL;
    }

    sb->s_blocksize = LogicalBlockSize;
    switch (sb->s_blocksize)
    {
       case 512:
          sb->s_blocksize_bits = 9;
	  break;
	  
       case 1024:
          sb->s_blocksize_bits = 10;
	  break;
	  
       case 2048:
          sb->s_blocksize_bits = 11;
	  break;
	  
       case 4096:
          sb->s_blocksize_bits = 12;
	  break;
	  
    }
    sb->s_magic = _LINUX_FS_ID;
    sb->s_dev = dev;
    sb->s_op = &nwfs_sops;
    sb->u.generic_sbp = (void *) volume;

    sb->s_root = d_alloc_root(iget(sb, 0));
    if (!sb->s_root)
    {
       NWFSPrint("nwfs:  get root inode failed\n");
       sb->s_dev = 0;
       return NULL;
    }
    return sb;
}

void nwfs_put_super(struct super_block *sb)
{
    register VOLUME *volume;
    register ULONG retCode;

#if (VERBOSE)
    NWFSPrint("put super\n");
#endif
    
    sb->s_dev = 0;
    volume = (VOLUME *) sb->u.generic_sbp;
    sb->u.generic_sbp = 0;

    retCode = DismountVolumeByHandle(volume);
    if (retCode)
       NWFSPrint("nwfs:  errors dismounting volume %s", volume->VolumeName);

    return;
}

int nwfs_statfs(struct super_block *sb, struct statfs *buf)
{
    register VOLUME *volume = (VOLUME *) sb->u.generic_sbp;
    register ULONG bpb = (IO_BLOCK_SIZE / LogicalBlockSize);

    buf->f_type = _LINUX_FS_ID;
    buf->f_bsize = LogicalBlockSize;
    buf->f_blocks = volume->VolumeClusters * volume->BlocksPerCluster * bpb;
    buf->f_bfree = volume->VolumeFreeClusters * volume->BlocksPerCluster * bpb;
    buf->f_bavail = volume->VolumeFreeClusters * volume->BlocksPerCluster * bpb;
    buf->f_namelen = 255;

    return 0;
}

#endif


#if (LINUX_22)

int nwfs_remount(struct super_block *sb, int *flags, char *data)
{
    return 0;
}

struct super_block *nwfs_read_super(struct super_block *sb, void *data, int silent)
{
    register VOLUME *volume;
    BYTE name[32];
    register BYTE *p, *n;
    register ULONG count, AutoRepair = FALSE;
    kdev_t dev = sb->s_dev;
    extern ULONG NWRepairVolume(VOLUME *volume, ULONG flags);

    if (!data)
    {
       NWFSPrint("nwfs:  missing VOLUME_NAME\n");
       NWFSPrint("USAGE: mount [name] [/mount_point] -t nwfs -o VOLUME_NAME(+)\n");
       NWFSPrint("       where VOLUME_NAME is the name of a NetWare file system Volume\n");
       NWFSPrint("       and '+' enables the driver to auto-repair volume errors during volume mount\n");
       NWFSPrint("       invoke NWVOL for a listing of detected NetWare Volumes in\n");
       NWFSPrint("       this system.\n\n");
       NWFSPrint("        Utilities for Linux are:\n");
       NWFSPrint("       NWVOL            - List detected NetWare volumes\n");
       NWFSPrint("       NWDISK/NWCONFIG  - File System/Partition Config Utility\n");
       sb->s_dev = 0;
       return NULL;
    }

    MOD_INC_USE_COUNT;
    lock_super(sb);

    NWFSVolumeScan();
    
    count = 0; 
    p = (BYTE *) data;
    n = &name[0];
    while (*p && (*p != ' ') && (*p != '\r') && (*p != '\n') && (*p != '+') && 
	  (count < 16))
    {
       *n++ = *p++;
       count++;
    }
    *n = '\0';

    if (*p == '+')
    {
        NWFSPrint("nwfs:  volume auto-repair enabled\n");
        AutoRepair = TRUE;
    }
    
    volume = MountHandledVolume(name);
    if (!volume)
    {
       NWFSPrint("nwfs:  error mounting volume %s\n", name);
       sb->s_dev = 0;
       unlock_super(sb);
       MOD_DEC_USE_COUNT;
       return NULL;
    }

    sb->s_blocksize = LogicalBlockSize;
    switch (sb->s_blocksize)
    {
       case 512:
          sb->s_blocksize_bits = 9;
	  break;
	  
       case 1024:
          sb->s_blocksize_bits = 10;
	  break;
	  
       case 2048:
          sb->s_blocksize_bits = 11;
	  break;
	  
       case 4096:
          sb->s_blocksize_bits = 12;
	  break;
	  
    }
    sb->s_magic = _LINUX_FS_ID;
    sb->s_dev = dev;
    sb->s_op = &nwfs_sops;
    sb->u.generic_sbp = (void *) volume;

    sb->s_root = d_alloc_root(iget(sb, 0), NULL);
    if (!sb->s_root)
    {
       NWFSPrint("nwfs:  get root inode failed\n");
       sb->s_dev = 0;
       unlock_super(sb);
       MOD_DEC_USE_COUNT;
       return NULL;
    }
    unlock_super(sb);
    return sb;
}

void nwfs_put_super(struct super_block *sb)
{
    register VOLUME *volume;
    register ULONG retCode;

    sb->s_dev = 0;
    volume = (VOLUME *) sb->u.generic_sbp;
    sb->u.generic_sbp = 0;

    retCode = DismountVolumeByHandle(volume);
    if (retCode)
       NWFSPrint("nwfs:  errors dismounting volume %s", volume->VolumeName);

    MOD_DEC_USE_COUNT;
    return;
}

int nwfs_statfs(struct super_block *sb, struct statfs *buf, int bufsiz)
{
    register VOLUME *volume = (VOLUME *) sb->u.generic_sbp;
    register ULONG bpb = (IO_BLOCK_SIZE / LogicalBlockSize);
    struct statfs tmp;

    tmp.f_type = _LINUX_FS_ID;
    tmp.f_bsize = LogicalBlockSize;
    tmp.f_blocks = volume->VolumeClusters * volume->BlocksPerCluster * bpb;
    tmp.f_bfree = volume->VolumeFreeClusters * volume->BlocksPerCluster * bpb;
    tmp.f_bavail = volume->VolumeFreeClusters * volume->BlocksPerCluster * bpb;
    tmp.f_files = 0;
    tmp.f_ffree = 0;
    tmp.f_namelen = 255;
    return copy_to_user(buf, &tmp, bufsiz) ? -EFAULT : 0;
}

#endif



#if (LINUX_20)

int nwfs_remount(struct super_block *sb, int *flags, char *data)
{
    return 0;
}

struct super_block *nwfs_read_super(struct super_block *sb, void *data, int silent)
{
    register VOLUME *volume;
    BYTE name[32];
    register BYTE *p, *n;
    register ULONG count, AutoRepair = FALSE;
    kdev_t dev = sb->s_dev;

    if (!data)
    {
       NWFSPrint("nwfs:  missing VOLUME_NAME\n");
       NWFSPrint("USAGE: mount [name] [/mount_point] -t nwfs -o VOLUME_NAME(+)\n");
       NWFSPrint("       where VOLUME_NAME is the name of a NetWare file system Volume\n");
       NWFSPrint("       and '+' enables the driver to auto-repair volume errors during volume mount\n");
       NWFSPrint("       invoke NWVOL for a listing of detected NetWare Volumes in\n");
       NWFSPrint("       this system.\n\n");
       NWFSPrint("        Utilities for Linux are:\n");
       NWFSPrint("       NWVOL            - List detected NetWare volumes\n");
       NWFSPrint("       NWDISK/NWCONFIG  - File System/Partition Config Utility\n");
       sb->s_dev = 0;
       return NULL;
    }

    NWFSVolumeScan();

    count = 0; 
    p = (BYTE *) data;
    n = &name[0];
    while (*p && (*p != ' ') && (*p != '\r') && (*p != '\n') && (*p != '+') && 
	  (count < 16))
    {
       *n++ = *p++;
       count++;
    }
    *n = '\0';

    if (*p == '+')
    {
        NWFSPrint("nwfs:  volume auto-repair enabled\n");
        AutoRepair = TRUE;
    }
    
    volume = MountHandledVolume(name);
    if (!volume)
    {
       NWFSPrint("nwfs:  error mounting volume %s\n", name);
       sb->s_dev = 0;
       return NULL;
    }

    MOD_INC_USE_COUNT;

    lock_super(sb);

    sb->s_blocksize = LogicalBlockSize;
    switch (sb->s_blocksize)
    {
       case 512:
          sb->s_blocksize_bits = 9;
	  break;
	  
       case 1024:
          sb->s_blocksize_bits = 10;
	  break;
	  
       case 2048:
          sb->s_blocksize_bits = 11;
	  break;
	  
       case 4096:
          sb->s_blocksize_bits = 12;
	  break;
	  
    }
    sb->s_magic = _LINUX_FS_ID;
    sb->s_dev = dev;
    sb->s_op = &nwfs_sops;
    sb->u.generic_sbp = (void *) volume;

    unlock_super(sb);

    sb->s_mounted = iget(sb, 0);
    if (!sb->s_mounted)
    {
       NWFSPrint("nwfs:  get root inode failed\n");
       sb->s_dev = 0;
       MOD_DEC_USE_COUNT;
       return NULL;
    }

    sb->s_mounted->i_atime = sb->s_mounted->i_mtime = NWFSGetSystemTime();
    sb->s_mounted->i_ctime = NWFSGetSystemTime();

    return sb;

}

void nwfs_put_super(struct super_block *sb)
{
    register VOLUME *volume;
    register ULONG retCode;

    lock_super(sb);

    sb->s_dev = 0;
    volume = (VOLUME *) sb->u.generic_sbp;
    sb->u.generic_sbp = 0;

    unlock_super(sb);

    retCode = DismountVolumeByHandle(volume);
    if (retCode)
       NWFSPrint("nwfs:  errors dismounting volume %s", volume->VolumeName);

    MOD_DEC_USE_COUNT;

    return;
}

void nwfs_statfs(struct super_block *sb, struct statfs *buf, int bufsiz)
{
    register VOLUME *volume = (VOLUME *) sb->u.generic_sbp;
    register ULONG bpb = (IO_BLOCK_SIZE / LogicalBlockSize);
    struct statfs tmp;

    tmp.f_type = _LINUX_FS_ID;
    tmp.f_bsize = LogicalBlockSize;
    tmp.f_blocks = volume->VolumeClusters * volume->BlocksPerCluster * bpb;
    tmp.f_bfree = volume->VolumeFreeClusters * volume->BlocksPerCluster * bpb;
    tmp.f_bavail = volume->VolumeFreeClusters * volume->BlocksPerCluster * bpb;
    tmp.f_files = 0;
    tmp.f_ffree = 0;
    tmp.f_namelen = 255;
    NWFSCopyToUserSpace(buf, &tmp, bufsiz);

}

int nwfs_register_symbols(void)
{
    return 0;
}

#endif

