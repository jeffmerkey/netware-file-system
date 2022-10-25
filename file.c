
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
*   FILE     :  FILE.C
*   DESCRIP  :   VFS File Module for Linux
*   DATE     :  December 6, 1998
*
*
***************************************************************************/

#include "globals.h"

#if (LINUX_24)

extern ULONG nwfs_to_linux_error(ULONG NwfsError);

int nwfs_open(struct inode *inode, struct file *file)
{
    // clear read ahead info
    file->private_data = NULL;
    return 0;
}

int nwfs_close(struct inode *inode, struct file *file)
{
    register ULONG retCode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    DOS dos;

#if (VFS_VERBOSE)
    NWFSPrint("nwfs:  close called\n");
#endif

    // clear read ahead info
    file->private_data = NULL;

    // We perform a truncate operation on the file during close to force
    // allocation of a suballoc element.  We do not allocate suballoc
    // elements during write because this results in excessive copying
    // and impacts performance very significantly .  We actualy perform the
    // suballocation only when we A) truncate a file, or B) close a file.
    // In the case of close, we simply truncate the cluster chain to
    // the reported file size.

    // if suballocation is disabled for this volume, then exit.
    if (!(volume->VolumeFlags & SUB_ALLOCATION_ON))
       return 0;

    if (!hash)
       return 0;

    NWLockFileExclusive(hash);

    // see if the modified flag was set for this file.  if not, then
    // the file is unchanged so we will exit.
    if (!(hash->State & NWFS_MODIFIED))
    {
       NWUnlockFile(hash);
       return 0;
    }
    // clear modified flag
    hash->State &= ~NWFS_MODIFIED;

    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) read dir error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }

    // if this file does not allow suballocation, then exit.
    if ((dos.FileAttributes & NO_SUBALLOC) || (dos.FileAttributes & TRANSACTION))
    {
       NWUnlockFile(hash);
       return 0;
    }

    // if the fat chain is already empty, then don't try to truncate
    // the file.  Truncating the file to it's current filesize will
    // force a suballocation.

    hash->TurboFATCluster = 0;
    hash->TurboFATIndex = 0;
    
    retCode = TruncateClusterChain(volume,
				   &dos.FirstBlock,
				   0,
				   0,
				   inode->i_size,
				   TRUE,
				   dos.FileAttributes);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  error in truncate cluster chain (%d)\n",
		(int)retCode);
       return 0;
    }

    // set file to new size.
    dos.FileSize = inode->i_size;

#if (HASH_FAT_CHAINS)
    hash->FirstBlock = dos.FirstBlock;
    hash->FileSize = dos.FileSize;
#endif
    
    retCode = WriteDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		      (int)inode->i_ino, (int)retCode);
       return 0;
    }

    NWUnlockFile(hash);
    return 0;
}

int nwfs_fsync(struct file *file, struct dentry *dentry)
{
    register struct inode *inode = dentry->d_inode;
    register VOLUME *volume = inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG ccode = 0;
    extern ULONG NWSyncFile(VOLUME *, HASH *);
    
    if (!hash)
       return 0;

    NWLockFileExclusive(hash);
    ccode = NWSyncFile(volume, hash);
    NWUnlockFile(hash);

#if (!DO_ASYNCH_IO && LINUX_BUFFER_CACHE)
    // if we are double buffering in the Linux buffer cache, then flush 
    // any disks currently owned by NWFS
    SyncDisks();
#endif
    
    if (ccode)
       ccode = -EPERM;
    return ccode;
}

void nwfs_truncate(struct inode *inode)
{
    register ULONG retCode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    DOS dos;

    if (!hash)
       return;

    NWLockFileExclusive(hash);

    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) read dir error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return;
    }

    // if the fat chain is already empty, then don't try to truncate
    // the file

    hash->TurboFATCluster = 0;
    hash->TurboFATIndex = 0;
    
    retCode = TruncateClusterChain(volume,
				   &dos.FirstBlock,
				   0,
				   0,
				   inode->i_size,
				   (volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0,
				   dos.FileAttributes);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  error in truncate cluster chain (%d)\n",
		(int)retCode);
       return;
    }

#if (VFS_VERBOSE)
    NWFSPrint("truncate:  FileSize(%d) -> NewSize(%d)\n",
	      (int)dos.FileSize, (int)inode->i_size);
#endif

    // set file to new size.
    dos.FileSize = inode->i_size;

#if (HASH_FAT_CHAINS)
    hash->FirstBlock = dos.FirstBlock;
    hash->FileSize = dos.FileSize;
#endif

    // update NetWare directory entry with modified date and time
    dos.LastUpdatedDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());

    retCode = WriteDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return;
    }

    inode->i_mtime = NWFSGetSystemTime();
    mark_inode_dirty(inode);

    NWUnlockFile(hash);
    return;
}

int nwfs_file_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    struct dentry *dentry = file->f_dentry;
    struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG len, offset, next;
    ULONG retCode;
#if (!HASH_FAT_CHAINS)
    DOS dos;
#endif
    off_t pos;

    pos = *ppos;
    if ((pos + count) > inode->i_size)
       count = inode->i_size - pos;

    if (count <= 0)
       return 0;

    if (!hash)
       return 0;

    NWLockFile(hash);

#if (!HASH_FAT_CHAINS)
    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }
#endif
    
    offset = (ULONG) pos;
    next = (ULONG)file->private_data;
    if (next == offset)
       hash->State |= NWFS_SEQUENTIAL;
    else
       hash->State &= ~NWFS_SEQUENTIAL;
    file->private_data = (void *)(offset + count);

    len = NWReadFile(volume,
#if (HASH_FAT_CHAINS)
		     &hash->FirstBlock,
		     hash->Flags,
		     hash->FileSize,
#else
		     &dos.FirstBlock,
		     dos.Flags,
		     dos.FileSize,
#endif
		     offset,
		     buf,
		     count,
#if (TURBO_FAT_ON)
		     &hash->TurboFATCluster,
		     &hash->TurboFATIndex,
#else
                     0, 0,
#endif
		     &retCode,
		     USER_ADDRESS_SPACE,
		     ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
#if (HASH_FAT_CHAINS)
                     hash->FileAttributes,
#else
		     dos.FileAttributes,
#endif
		     (hash->State & NWFS_SEQUENTIAL) ? TRUE : 0);
    
#if (VFS_VERBOSE)
    NWFSPrint("read_file:  context-%08X index-%08X offset-%d size-%d\n",
	     (unsigned int)hash->TurboFATCluster,
	     (unsigned int)hash->TurboFATIndex,
	     (int)offset, (int)count);
#endif

    if (retCode)
    {
       NWUnlockFile(hash);
       return (nwfs_to_linux_error(retCode));
    }

    *ppos = (pos + len);
    if (!IS_RDONLY(inode))
    {
       inode->i_atime = NWFSGetSystemTime();
       mark_inode_dirty(inode);
    }

    NWUnlockFile(hash);
    return len;

}

int nwfs_read_file_link(struct inode *inode, char *buf, size_t count, 
                        loff_t *ppos)
{
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG len, offset;
    ULONG retCode;
#if (!HASH_FAT_CHAINS)
    DOS dos;
#endif
    off_t pos;

    pos = *ppos;
    if ((pos + count) > inode->i_size)
       count = inode->i_size - pos;

    if (count <= 0)
       return 0;

    if (!hash)
       return 0;

    NWLockFile(hash);

#if (!HASH_FAT_CHAINS)
    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }
#endif
    
    offset = (ULONG) pos;
    len = NWReadFile(volume,
#if (HASH_FAT_CHAINS)
		     &hash->FirstBlock,
		     hash->Flags,
		     hash->FileSize,
#else
		     &dos.FirstBlock,
		     dos.Flags,
		     dos.FileSize,
#endif
		     offset,
		     buf,
		     count,
#if (TURBO_FAT_ON)
		     &hash->TurboFATCluster,
		     &hash->TurboFATIndex,
#else
                     0, 0,
#endif
		     &retCode,
		     KERNEL_ADDRESS_SPACE,
		     ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
#if (HASH_FAT_CHAINS)
                     hash->FileAttributes,
#else
		     dos.FileAttributes,
#endif
		     (hash->State & NWFS_SEQUENTIAL) ? TRUE : 0);
    
#if (VFS_VERBOSE)
    NWFSPrint("read_kernel_file:  ctx-%08X ndx-%08X off-%d sz-%d rc-%d len-%d\n",
	     (unsigned int)hash->TurboFATCluster,
	     (unsigned int)hash->TurboFATIndex,
	     (int)offset, (int)count, (int)retCode, (int)len);
#endif

    if (retCode)
    {
       NWUnlockFile(hash);
       return (nwfs_to_linux_error(retCode));
    }

    *ppos = (pos + len);
    if (!IS_RDONLY(inode))
    {
       inode->i_atime = NWFSGetSystemTime();
       mark_inode_dirty(inode);
    }

    NWUnlockFile(hash);
    return len;

}

int nwfs_file_read_kernel(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    struct dentry *dentry = file->f_dentry;
    struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG len, offset, next;
    ULONG retCode;
#if (!HASH_FAT_CHAINS)
    DOS dos;
#endif
    off_t pos;

    pos = *ppos;
    if ((pos + count) > inode->i_size)
       count = inode->i_size - pos;

    if (count <= 0)
       return 0;

    if (!hash)
       return 0;

    NWLockFile(hash);

#if (!HASH_FAT_CHAINS)
    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }
#endif
    
    offset = (ULONG) pos;
    next = (ULONG)file->private_data;
    if (next == offset)
       hash->State |= NWFS_SEQUENTIAL;
    else
       hash->State &= ~NWFS_SEQUENTIAL;
    file->private_data = (void *)(offset + count);

    len = NWReadFile(volume,
#if (HASH_FAT_CHAINS)
		     &hash->FirstBlock,
		     hash->Flags,
		     hash->FileSize,
#else
		     &dos.FirstBlock,
		     dos.Flags,
		     dos.FileSize,
#endif
		     offset,
		     buf,
		     count,
#if (TURBO_FAT_ON)
		     &hash->TurboFATCluster,
		     &hash->TurboFATIndex,
#else
                     0, 0,
#endif
		     &retCode,
		     KERNEL_ADDRESS_SPACE,
		     ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
#if (HASH_FAT_CHAINS)
                     hash->FileAttributes,
#else
		     dos.FileAttributes,
#endif
		     (hash->State & NWFS_SEQUENTIAL) ? TRUE : 0);
    
#if (VFS_VERBOSE)
    NWFSPrint("read_kernel_file:  context-%08X index-%08X offset-%d size-%d\n",
	     (unsigned int)hash->TurboFATCluster,
	     (unsigned int)hash->TurboFATIndex,
	     (int)offset, (int)count);
#endif

    if (retCode)
    {
       NWUnlockFile(hash);
       return (nwfs_to_linux_error(retCode));
    }

    *ppos = (pos + len);
    if (!IS_RDONLY(inode))
    {
       inode->i_atime = NWFSGetSystemTime();
       mark_inode_dirty(inode);
    }

    NWUnlockFile(hash);
    return len;

}

int nwfs_file_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    struct dentry *dentry = file->f_dentry;
    struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    ULONG retCode;
    register ULONG len = 0;
    DOS dos;
    off_t pos;

    if (inode == NULL)
    {
       NWFSPrint("nwfs: inode = NULL\n");
       return -EINVAL;
    }

    if (count <= 0)
       return 0;

    pos = *ppos;
    if (file->f_flags & O_APPEND)
    {
       pos = inode->i_size;
    }

    if (!hash)
       return -ENOENT;

    NWLockFileExclusive(hash);

    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) file write error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -ENOENT;
    }

    len = NWWriteFile(volume,
		      &dos.FirstBlock,
		      dos.Flags,
		      pos,
		      (void *)buf,
		      count,
#if (TURBO_FAT_ON)
		     &hash->TurboFATCluster,
		     &hash->TurboFATIndex,
#else
                     0, 0,
#endif
		      &retCode,
		      USER_ADDRESS_SPACE,
		      ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
		      dos.FileAttributes);
#if (VFS_VERBOSE)
    NWFSPrint("write_file:  context-%08X index-%08X offset-%d size-%d\n",
	     (unsigned int)hash->TurboFATCluster,
	     (unsigned int)hash->TurboFATIndex,
	     (int)pos, (int)count);
#endif

    if (retCode)
    {
       NWUnlockFile(hash);
       return (nwfs_to_linux_error(retCode));
    }

    pos += len;
    *ppos = pos;
    if (pos > inode->i_size)
    {
       inode->i_size = pos;
       // file size changed, update directory entry
       dos.FileSize = pos;
    }

#if (HASH_FAT_CHAINS)
    hash->FirstBlock = dos.FirstBlock;
    hash->FileSize = dos.FileSize;
#endif

    // update NetWare directory entry with modified date and time

    dos.LastUpdatedDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());

    retCode = WriteDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }

    // set the modified flag so close will know to perform suballocation
    // of this file stream.
    hash->State |= NWFS_MODIFIED;

    // update the modified time for this file.
    inode->i_mtime = NWFSGetSystemTime();
    mark_inode_dirty(inode);

    NWUnlockFile(hash);
    return len;

}

int nwfs_permission(struct inode *inode, int mask)
{
    unsigned short mode = inode->i_mode;

    // Nobody gets write access to a file on a readonly-fs
    if ((mask & S_IWOTH) && (S_ISREG(mode) || S_ISDIR(mode) ||
	 S_ISLNK(mode)) && IS_RDONLY(inode))
       return -EROFS;

    // Nobody gets write access to an immutable file
    if ((mask & S_IWOTH) && IS_IMMUTABLE(inode))
       return -EACCES;
    // If no ACL, checks using the file mode
    else
    if (current->fsuid == inode->i_uid)
       mode >>= 6;
    else
    if (in_group_p (inode->i_gid))
       mode >>= 3;

    // Access is always granted for root. We now check last,
    // though, for BSD process accounting correctness

    if (((mode & mask & S_IRWXO) == mask) || capable(CAP_DAC_OVERRIDE))
	return 0;

    if ((mask == S_IROTH) || (S_ISDIR(mode) && !(mask & ~(S_IROTH | S_IXOTH))))
       if (capable(CAP_DAC_READ_SEARCH))
	  return 0;

    return -EACCES;
}

struct file_operations nwfs_file_operations =
{
#if (PAGE_CACHE_ON)
    read:      generic_file_read,
    write:     generic_file_write,
#else
    read:      nwfs_file_read,
    write:     nwfs_file_write,
#endif
    ioctl:     nwfs_ioctl,
    mmap:      generic_file_mmap,
    open:      nwfs_open,
    release:   nwfs_close,
//    fsync:     nwfs_fsync
};

struct inode_operations nwfs_file_inode_operations =
{
    truncate:     nwfs_truncate,
    permission:   nwfs_permission,
    setattr:      nwfs_notify_change,
};

#endif


#if (LINUX_22)

extern ULONG nwfs_to_linux_error(ULONG NwfsError);

int nwfs_open(struct inode *inode, struct file *file)
{
#if (VFS_VERBOSE)
    NWFSPrint("nwfs_open inode no %d size-%d %s\n", (int)inode->i_ino,
		     (int)inode->i_size, 
		     file->f_dentry->d_name.name);
#endif

    // clear read ahead info
    file->private_data = NULL;
    return 0;
}

int nwfs_close(struct inode *inode, struct file *file)
{
    register ULONG retCode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    DOS dos;

#if (VFS_VERBOSE)
    NWFSPrint("nwfs_close inode no %d size-%d %s\n", (int)inode->i_ino,
		     (int)inode->i_size,
		     file->f_dentry->d_name.name);
#endif

    // clear read ahead info
    file->private_data = NULL;

    // We perform a truncate operation on the file during close to force
    // allocation of a suballoc element.  We do not allocate suballoc
    // elements during write because this results in excessive copying
    // and impacts performance very significantly .  We actualy perform the
    // suballocation only when we A) truncate a file, or B) close a file.
    // In the case of close, we simply truncate the cluster chain to
    // the reported file size.


    // if suballocation is disabled for this volume, then exit.
    if (!(volume->VolumeFlags & SUB_ALLOCATION_ON))
       return 0;

    if (!hash)
       return 0;

    NWLockFileExclusive(hash);

    // see if the modified flag was set for this file.  if not, then
    // the file is unchanged so we will exit.
    if (!(hash->State & NWFS_MODIFIED))
    {
       NWUnlockFile(hash);
       return 0;
    }
    // clear modified flag
    hash->State &= ~NWFS_MODIFIED;

    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) read dir error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }

    // if this file does not allow suballocation, then exit.
    if ((dos.FileAttributes & NO_SUBALLOC) || (dos.FileAttributes & TRANSACTION))
    {
       NWUnlockFile(hash);
       return 0;
    }

    // if the fat chain is already empty, then don't try to truncate
    // the file.  Truncating the file to it's current filesize will
    // force a suballocation.

    hash->TurboFATCluster = 0;
    hash->TurboFATIndex = 0;
    
    retCode = TruncateClusterChain(volume,
				   &dos.FirstBlock,
				   0,
				   0,
				   inode->i_size,
				   TRUE,
				   dos.FileAttributes);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  error in truncate cluster chain (%d)\n",
		(int)retCode);
       return 0;
    }

    // set file to new size.
    dos.FileSize = inode->i_size;

#if (HASH_FAT_CHAINS)
    hash->FirstBlock = dos.FirstBlock;
    hash->FileSize = dos.FileSize;
#endif

    retCode = WriteDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		      (int)inode->i_ino, (int)retCode);
       return 0;
    }

    NWUnlockFile(hash);
    return 0;
}

int nwfs_fsync(struct file *file, struct dentry *dentry)
{
    register struct inode *inode = dentry->d_inode;
    register VOLUME *volume = inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG ccode = 0;
    extern ULONG NWSyncFile(VOLUME *, HASH *);

#if (VFS_VERBOSE)
    NWFSPrint("nwfs_sync inode no %d %s\n", (int)inode->i_ino,
		     dentry->d_name.name);
#endif

    if (!hash)
       return 0;

    NWLockFileExclusive(hash);
    ccode = NWSyncFile(volume, hash);
    NWUnlockFile(hash);

#if (!DO_ASYNCH_IO && LINUX_BUFFER_CACHE)
    // if we are double buffering in the Linux buffer cache, then flush 
    // any disks currently owned by NWFS
    SyncDisks();
#endif
    
    if (ccode)
       ccode = -EPERM;
    return ccode;
}

void nwfs_truncate(struct inode *inode)
{
    register ULONG retCode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    DOS dos;

#if (VFS_VERBOSE)
    NWFSPrint("nwfs_truncate inode no %d size %d %s\n", 
 	     (int)inode->i_ino, (int)inode->i_size,
	     hash->Name);
#endif

    if (!hash)
       return;

    NWLockFileExclusive(hash);

    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) read dir error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return;
    }

    // if the fat chain is already empty, then don't try to truncate
    // the file

    hash->TurboFATCluster = 0;
    hash->TurboFATIndex = 0;
    
    retCode = TruncateClusterChain(volume,
				   &dos.FirstBlock,
				   0,
				   0,
				   inode->i_size,
				   (volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0,
				   dos.FileAttributes);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  error in truncate cluster chain (%d)\n",
		(int)retCode);
       return;
    }

#if (VFS_VERBOSE)
    NWFSPrint("truncate:  FileSize(%d) -> NewSize(%d)\n",
	      (int)dos.FileSize, (int)inode->i_size);
#endif

    // set file to new size.
    dos.FileSize = inode->i_size;

#if (HASH_FAT_CHAINS)
    hash->FirstBlock = dos.FirstBlock;
    hash->FileSize = dos.FileSize;
#endif

    // update NetWare directory entry with modified date and time
    dos.LastUpdatedDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());

    retCode = WriteDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return;
    }

    inode->i_mtime = NWFSGetSystemTime();
    mark_inode_dirty(inode);

    NWUnlockFile(hash);
    return;
}

int nwfs_file_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    struct dentry *dentry = file->f_dentry;
    struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG len, offset, next;
    ULONG retCode;
#if (!HASH_FAT_CHAINS)
    DOS dos;
#endif
    off_t pos;

    pos = *ppos;
    if ((pos + count) > inode->i_size)
       count = inode->i_size - pos;

    if (count <= 0)
       return 0;

    if (!hash)
       return 0;

    NWLockFile(hash);

#if (!HASH_FAT_CHAINS)
    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }
#endif
    
    offset = (ULONG) pos;
    next = (ULONG)file->private_data;
    if (next == offset)
       hash->State |= NWFS_SEQUENTIAL;
    else
       hash->State &= ~NWFS_SEQUENTIAL;
    file->private_data = (void *)(offset + count);

    len = NWReadFile(volume,
#if (HASH_FAT_CHAINS)
		     &hash->FirstBlock,
		     hash->Flags,
		     hash->FileSize,
#else
		     &dos.FirstBlock,
		     dos.Flags,
		     dos.FileSize,
#endif
		     offset,
		     buf,
		     count,
#if (TURBO_FAT_ON)
		     &hash->TurboFATCluster,
		     &hash->TurboFATIndex,
#else
                     0, 0,
#endif
		     &retCode,
		     USER_ADDRESS_SPACE,
		     ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
#if (HASH_FAT_CHAINS)
                     hash->FileAttributes,
#else
		     dos.FileAttributes,
#endif
		     (hash->State & NWFS_SEQUENTIAL) ? TRUE : 0);

#if (VFS_VERBOSE)
    NWFSPrint("read_file %s:  context-%08X index-%08X offset-%d size-%d\n",
	     file->f_dentry->d_name.name,
	     (unsigned int)hash->TurboFATCluster,
	     (unsigned int)hash->TurboFATIndex,
	     (int)offset, (int)count);
#endif

    if (retCode)
    {
       NWUnlockFile(hash);
       return (nwfs_to_linux_error(retCode));
    }

    *ppos = (pos + len);
    if (!IS_RDONLY(inode))
    {
       inode->i_atime = NWFSGetSystemTime();
       mark_inode_dirty(inode);
    }

    NWUnlockFile(hash);
    return len;

}

int nwfs_file_read_kernel(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    struct dentry *dentry = file->f_dentry;
    struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG len, offset, next;
    ULONG retCode;
#if (!HASH_FAT_CHAINS)
    DOS dos;
#endif
    off_t pos;

    pos = *ppos;
    if ((pos + count) > inode->i_size)
       count = inode->i_size - pos;

    if (count <= 0)
       return 0;

    if (!hash)
       return 0;

    NWLockFile(hash);

#if (!HASH_FAT_CHAINS)
    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }
#endif
    
    offset = (ULONG) pos;
    next = (ULONG)file->private_data;
    if (next == offset)
       hash->State |= NWFS_SEQUENTIAL;
    else
       hash->State &= ~NWFS_SEQUENTIAL;
    file->private_data = (void *)(offset + count);

    len = NWReadFile(volume,
#if (HASH_FAT_CHAINS)
		     &hash->FirstBlock,
		     hash->Flags,
		     hash->FileSize,
#else
		     &dos.FirstBlock,
		     dos.Flags,
		     dos.FileSize,
#endif
		     offset,
		     buf,
		     count,
#if (TURBO_FAT_ON)
		     &hash->TurboFATCluster,
		     &hash->TurboFATIndex,
#else
                     0, 0,
#endif
		     &retCode,
		     KERNEL_ADDRESS_SPACE,
		     ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
#if (HASH_FAT_CHAINS)
                     hash->FileAttributes,
#else
		     dos.FileAttributes,
#endif
		     (hash->State & NWFS_SEQUENTIAL) ? TRUE : 0);

#if (VFS_VERBOSE)
    NWFSPrint("read_kernel_file %s:  context-%08X index-%08X offset-%d size-%d rc-%d len-%d\n[fb-%08X]-(%d bytes)\n",
	     file->f_dentry->d_name.name,
	     (unsigned int)hash->TurboFATCluster,
	     (unsigned int)hash->TurboFATIndex,
	     (int)offset, (int)count, (int)retCode,
	     (int)len, (unsigned)hash->FirstBlock,
	     (int)hash->FileSize);
#endif

    if (retCode)
    {
       NWUnlockFile(hash);
       return (nwfs_to_linux_error(retCode));
    }

    *ppos = (pos + len);
    if (!IS_RDONLY(inode))
    {
       inode->i_atime = NWFSGetSystemTime();
       mark_inode_dirty(inode);
    }

    NWUnlockFile(hash);
    return len;

}

int nwfs_file_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    struct dentry *dentry = file->f_dentry;
    struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    ULONG retCode;
    register ULONG len = 0;
    DOS dos;
    off_t pos;

    if (inode == NULL)
    {
       NWFSPrint("nwfs: inode = NULL\n");
       return -EINVAL;
    }

    if (count <= 0)
       return 0;

    pos = *ppos;
    if (file->f_flags & O_APPEND)
    {
       pos = inode->i_size;
    }

    if (!hash)
       return -ENOENT;

    NWLockFileExclusive(hash);

    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) file write error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -ENOENT;
    }
    
    len = NWWriteFile(volume,
		      &dos.FirstBlock,
		      dos.Flags,
		      pos,
		      (void *)buf,
		      count,
#if (TURBO_FAT_ON)
		      &hash->TurboFATCluster,
		      &hash->TurboFATIndex,
#else
                      0, 0,
#endif
		      &retCode,
		      USER_ADDRESS_SPACE,
		      ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
		     dos.FileAttributes);

#if (VFS_VERBOSE)
    NWFSPrint("write_file %s:  context-%08X index-%08X offset-%d size-%d\n",
	     file->f_dentry->d_name.name,
	     (unsigned int)hash->TurboFATCluster,
	     (unsigned int)hash->TurboFATIndex,
	     (int)pos, (int)count);
#endif

    if (retCode)
    {
       NWUnlockFile(hash);
       return (nwfs_to_linux_error(retCode));
    }

    pos += len;
    *ppos = pos;
    if (pos > inode->i_size)
    {
       inode->i_size = pos;
       // file size changed, update directory entry
       dos.FileSize = pos;
    }

#if (HASH_FAT_CHAINS)
    hash->FirstBlock = dos.FirstBlock;
    hash->FileSize = dos.FileSize;
#endif

    // update NetWare directory entry with modified date and time
    dos.LastUpdatedDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());

    retCode = WriteDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }
    
    // set the modified flag so close will know to perform suballocation
    // of this file stream.
    hash->State |= NWFS_MODIFIED;

    // update the modified time for this file.
    inode->i_mtime = NWFSGetSystemTime();
    mark_inode_dirty(inode);

    NWUnlockFile(hash);
    return len;

}

int nwfs_permission(struct inode *inode, int mask)
{
    unsigned short mode = inode->i_mode;

#if (VFS_VERBOSE)
    NWFSPrint("nwfs_permission inode no %d mask-%d\n",
	      (int)inode->i_ino, mask);
#endif
    
    // Nobody gets write access to a file on a readonly-fs
    if ((mask & S_IWOTH) && (S_ISREG(mode) || S_ISDIR(mode) ||
	 S_ISLNK(mode)) && IS_RDONLY(inode))
       return -EROFS;

    // Nobody gets write access to an immutable file
    if ((mask & S_IWOTH) && IS_IMMUTABLE(inode))
       return -EACCES;
    // If no ACL, checks using the file mode
    else
    if (current->fsuid == inode->i_uid)
       mode >>= 6;
    else
    if (in_group_p (inode->i_gid))
       mode >>= 3;

    // Access is always granted for root. We now check last,
    // though, for BSD process accounting correctness

    if (((mode & mask & S_IRWXO) == mask) || capable(CAP_DAC_OVERRIDE))
	return 0;

    if ((mask == S_IROTH) || (S_ISDIR(mode) && !(mask & ~(S_IROTH | S_IXOTH))))
       if (capable(CAP_DAC_READ_SEARCH))
	  return 0;

    return -EACCES;
}

static struct file_operations nwfs_file_operations =
{
//    nwfs_lseek,			// lseek
    NULL,
#if (PAGE_CACHE_ON)
    generic_file_read,          // read
#else
    nwfs_file_read,	        // read
#endif
    nwfs_file_write,	        // write
    NULL,			// readdir
    NULL,			// select
    nwfs_ioctl,		        // ioctl
    generic_file_mmap,	        // mmap
    nwfs_open,			// open
    NULL,                       // flush
    nwfs_close,			// release

    //    nwfs_fsync,		// fsync
    NULL,                       // fsync
    
    NULL,                       // fasync
    NULL,                       // check_media_change
    NULL,                       // revalidate
};

struct inode_operations nwfs_file_inode_operations =
{
    &nwfs_file_operations,    // default file operations
    NULL,			// create
    NULL,			// lookup
    NULL,			// link
    NULL,			// unlink
    NULL,			// symlink
    NULL,			// mkdir
    NULL,			// rmdir
    NULL,			// mknod
    NULL,			// rename
    NULL,			// readlink
    NULL,			// follow_link
    generic_readpage,		// readpage
    NULL,			// writepage
    nwfs_bmap,			// bmap
    nwfs_truncate,		// truncate
    nwfs_permission,		// permission
    NULL,		        // smap
};

#endif


#if (LINUX_20)

extern ULONG nwfs_to_linux_error(ULONG NwfsError);

int nwfs_open(struct inode *inode, struct file *file)
{
    // clear read ahead info
    file->private_data = NULL;
    return 0;
}

void nwfs_close(struct inode *inode, struct file *file)
{
    register ULONG retCode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    DOS dos;

    // clear read ahead info
    file->private_data = NULL;

    // We perform a truncate operation on the file during close to force
    // allocation of a suballoc element.  We do not allocate suballoc
    // elements during write because this results in excessive copying
    // and impacts performance very significantly .  We actualy perform the
    // suballocation only when we A) truncate a file, or B) close a file.
    // In the case of close, we simply truncate the cluster chain to
    // the reported file size.

    // if suballocation is disabled for this volume, then exit.
    if (!(volume->VolumeFlags & SUB_ALLOCATION_ON))
       return;

    if (!hash)
       return;

    NWLockFileExclusive(hash);

    // see if the modified flag was set for this file.  if not, then
    // the file is unchanged so we will exit.
    if (!(hash->State & NWFS_MODIFIED))
    {
       NWUnlockFile(hash);
       return;
    }
    // clear modified flag
    hash->State &= ~NWFS_MODIFIED;

    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) read dir error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return;
    }

    // if this file does not allow suballocation, then exit.
    if ((dos.FileAttributes & NO_SUBALLOC) || (dos.FileAttributes & TRANSACTION))
    {
       NWUnlockFile(hash);
       return;
    }

    // if the fat chain is already empty, then don't try to truncate
    // the file.

    hash->TurboFATCluster = 0;
    hash->TurboFATIndex = 0;
    
    retCode = TruncateClusterChain(volume,
				   &dos.FirstBlock,
				   0,
				   0,
				   inode->i_size,
				   TRUE,
				   dos.FileAttributes);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  error in truncate cluster chain (%d)\n",
		(int)retCode);
       return;
    }

    // set file to new size.
    dos.FileSize = inode->i_size;

#if (HASH_FAT_CHAINS)
    hash->FirstBlock = dos.FirstBlock;
    hash->FileSize = dos.FileSize;
#endif

    retCode = WriteDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		      (int)inode->i_ino, (int)retCode);
       return;
    }

    NWUnlockFile(hash);
    return;

}

int nwfs_fsync(struct inode *inode, struct file *file)
{
    register VOLUME *volume = inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG ccode = 0;
    extern ULONG NWSyncFile(VOLUME *, HASH *);
    
    if (!hash)
       return 0;

    NWLockFileExclusive(hash);
    ccode = NWSyncFile(volume, hash);
    NWUnlockFile(hash);

#if (!DO_ASYNCH_IO && LINUX_BUFFER_CACHE)
    // if we are double buffering in the Linux buffer cache, then flush 
    // any disks currently owned by NWFS
    SyncDisks();
#endif
    
    if (ccode)
       ccode = -EPERM;
    return ccode;
}

void nwfs_truncate(struct inode *inode)
{
    register ULONG retCode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    DOS dos;

    if (!hash)
       return;

    NWLockFileExclusive(hash);

    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) read dir error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return;
    }

    // if the fat chain is already empty, then don't try to truncate
    // the file

    hash->TurboFATCluster = 0;
    hash->TurboFATIndex = 0;
    
    retCode = TruncateClusterChain(volume,
				   &dos.FirstBlock,
				   0,
				   0,
				   inode->i_size,
				   (volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0,
				   dos.FileAttributes);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  error in truncate cluster chain (%d)\n",
		(int)retCode);
       return;
    }

#if (VFS_VERBOSE)
    NWFSPrint("truncate:  FileSize(%d) -> NewSize(%d)\n",
	      (int)dos.FileSize, (int)inode->i_size);
#endif

    // set file to new size.
    dos.FileSize = inode->i_size;

#if (HASH_FAT_CHAINS)
    hash->FirstBlock = dos.FirstBlock;
    hash->FileSize = dos.FileSize;
#endif

    // update NetWare directory entry with modified date and time
    dos.LastUpdatedDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());

    retCode = WriteDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return;
    }

    inode->i_mtime = NWFSGetSystemTime();
    inode->i_dirt = 1;

    NWUnlockFile(hash);
    return;
}

int nwfs_file_read(struct inode *inode, struct file *file,
		     char *buf, int count)
{
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG len, offset, next;
    ULONG retCode;
#if (!HASH_FAT_CHAINS)
    DOS dos;
#endif
    off_t pos;

    pos = file->f_pos;
    if ((pos + count) > inode->i_size)
       count = inode->i_size - pos;

    if (count <= 0)
       return 0;

    if (!hash)
       return 0;

    NWLockFile(hash);

#if (!HASH_FAT_CHAINS)
    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }
#endif
    
    offset = (ULONG) pos;
    next = (ULONG)file->private_data;
    if (next == offset)
       hash->State |= NWFS_SEQUENTIAL;
    else
       hash->State &= ~NWFS_SEQUENTIAL;
    file->private_data = (void *)(offset + count);

    len = NWReadFile(volume,
#if (HASH_FAT_CHAINS)
		     &hash->FirstBlock,
		     hash->Flags,
		     hash->FileSize,
#else
		     &dos.FirstBlock,
		     dos.Flags,
		     dos.FileSize,
#endif
		     offset,
		     buf,
		     count,
#if (TURBO_FAT_ON)
		     &hash->TurboFATCluster,
		     &hash->TurboFATIndex,
#else
                     0, 0,
#endif
		     &retCode,
		     USER_ADDRESS_SPACE,
		     ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
#if (HASH_FAT_CHAINS)
                     hash->FileAttributes,
#else
		     dos.FileAttributes,
#endif
		     (hash->State & NWFS_SEQUENTIAL) ? TRUE : 0);

#if (VFS_VERBOSE)
    NWFSPrint("readf: ctx/ndx-%X/%X offset-%d size-%d\n",
	     (unsigned int)hash->TurboFATCluster,
	     (unsigned int)hash->TurboFATIndex,
	     (int)offset, (int)count);
#endif

    if (retCode)
    {
       NWUnlockFile(hash);
       return (nwfs_to_linux_error(retCode));
    }

    file->f_pos = (pos + len);
    if (!IS_RDONLY(inode))
    {
       inode->i_atime = NWFSGetSystemTime();
       inode->i_dirt = 1;
    }

    NWUnlockFile(hash);
    return len;

}

int nwfs_file_read_kernel(struct inode *inode, struct file *file,
			  char *buf, int count)
{
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG len, offset, next;
    ULONG retCode;
#if (!HASH_FAT_CHAINS)
    DOS dos;
#endif
    off_t pos;

    pos = file->f_pos;
    if ((pos + count) > inode->i_size)
       count = inode->i_size - pos;

    if (count <= 0)
       return 0;

    if (!hash)
       return 0;

    NWLockFile(hash);

#if (!HASH_FAT_CHAINS)
    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }
#endif
    
    offset = (ULONG) pos;
    next = (ULONG)file->private_data;
    if (next == offset)
       hash->State |= NWFS_SEQUENTIAL;
    else
       hash->State &= ~NWFS_SEQUENTIAL;
    file->private_data = (void *)(offset + count);

    len = NWReadFile(volume,
#if (HASH_FAT_CHAINS)
		     &hash->FirstBlock,
		     hash->Flags,
		     hash->FileSize,
#else
		     &dos.FirstBlock,
		     dos.Flags,
		     dos.FileSize,
#endif
		     offset,
		     buf,
		     count,
#if (TURBO_FAT_ON)
		     &hash->TurboFATCluster,
		     &hash->TurboFATIndex,
#else
                     0, 0,
#endif
		     &retCode,
		     KERNEL_ADDRESS_SPACE,
		     ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
#if (HASH_FAT_CHAINS)
                     hash->FileAttributes,
#else
		     dos.FileAttributes,
#endif
		     (hash->State & NWFS_SEQUENTIAL) ? TRUE : 0);

#if (VFS_VERBOSE)
    NWFSPrint("readkf:  ctx/ndx-%X/%X offset-%d size-%d\n",
	     (unsigned int)hash->TurboFATCluster,
	     (unsigned int)hash->TurboFATIndex,
	     (int)offset, (int)count);
#endif

    if (retCode)
    {
       NWUnlockFile(hash);
       return (nwfs_to_linux_error(retCode));
    }

    file->f_pos = (pos + len);
    if (!IS_RDONLY(inode))
    {
       inode->i_atime = NWFSGetSystemTime();
       inode->i_dirt = 1;
    }

    NWUnlockFile(hash);
    return len;

}

int nwfs_file_write(struct inode *inode, struct file *file,
		      const char *buf, int count)
{
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    ULONG retCode;
    register ULONG len = 0;
    DOS dos;
    off_t pos;

    if (inode == NULL)
    {
       NWFSPrint("nwfs: inode = NULL\n");
       return -EINVAL;
    }

    if (count <= 0)
       return 0;

    pos = file->f_pos;
    if (file->f_flags & O_APPEND)
    {
       pos = inode->i_size;
    }

    if (!hash)
       return -ENOENT;

    NWLockFileExclusive(hash);

    retCode = ReadDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) file write error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -ENOENT;
    }

    len = NWWriteFile(volume,
		      &dos.FirstBlock,
		      dos.Flags,
		      pos,
		      (void *)buf,
		      count,
#if (TURBO_FAT_ON)
		     &hash->TurboFATCluster,
		     &hash->TurboFATIndex,
#else
                     0, 0,
#endif
		      &retCode,
		      USER_ADDRESS_SPACE,
		      ((volume->VolumeFlags & SUB_ALLOCATION_ON) ? 1 : 0),
		      dos.FileAttributes);
#if (VFS_VERBOSE)
    NWFSPrint("writef: ctx/ndx-%X/%X offset-%d size-%d\n",
	     (unsigned int)hash->TurboFATCluster,
	     (unsigned int)hash->TurboFATIndex,
	     (int)pos, (int)count);
#endif

    if (retCode)
    {
       NWUnlockFile(hash);
       return (nwfs_to_linux_error(retCode));
    }

    pos += len;
    file->f_pos = pos;
    if (pos > inode->i_size)
    {
       inode->i_size = pos;
       // file size changed, update directory entry
       dos.FileSize = pos;
    }

#if (HASH_FAT_CHAINS)
    hash->FirstBlock = dos.FirstBlock;
    hash->FileSize = dos.FileSize;
#endif

    // update NetWare directory entry with modified date and time

    dos.LastUpdatedDateAndTime = NWFSSystemToNetwareTime(NWFSGetSystemTime());
    retCode = WriteDirectoryRecord(volume, &dos, inode->i_ino);
    if (retCode)
    {
       NWUnlockFile(hash);
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       return -EIO;
    }

    // set the modified flag so close will know to perform suballocation
    // of this file stream.
    hash->State |= NWFS_MODIFIED;

    // update the modified time for this file.

    inode->i_mtime = NWFSGetSystemTime();
    inode->i_dirt = 1;

    NWUnlockFile(hash);
    return len;

}

int nwfs_permission(struct inode *inode, int mask)
{
    unsigned short mode = inode->i_mode;

    // Nobody gets write access to a file on a readonly-fs
    if ((mask & S_IWOTH) && (S_ISREG(mode) || S_ISDIR(mode) || S_ISLNK(mode)) &&
	IS_RDONLY(inode))
       return -EROFS;

    // Nobody gets write access to an immutable file
    if ((mask & S_IWOTH) && IS_IMMUTABLE(inode))
       return -EACCES;

    // Special case, access is always granted for root
    if (fsuser())
       return 0;
    else   // If no ACL, checks using the file mode
    if (current->fsuid == inode->i_uid)
       mode >>= 6;
    else
    if (in_group_p(inode->i_gid))
       mode >>= 3;

    if (((mode & mask & S_IRWXO) == mask))
       return 0;
    else
       return -EACCES;
}

static struct file_operations nwfs_file_operations =
{
//    nwfs_lseek,			// lseek
    NULL,
#if (PAGE_CACHE_ON)
    generic_file_read,          // read
#else
    nwfs_file_read,	        // read
#endif
    nwfs_file_write,	        // write
    NULL,			// readdir
    NULL,			// select
    nwfs_ioctl,		        // ioctl
    generic_file_mmap,	        // mmap
    nwfs_open,			// open
    nwfs_close,			// release

    //    nwfs_fsync,		// fsync
    NULL,                       // fsync
    
    NULL,                       // fasync
    NULL,                       // check_media_change
    NULL,                       // revalidate
};

struct inode_operations nwfs_file_inode_operations =
{
    &nwfs_file_operations,    // default file operations
    NULL,			// create
    NULL,			// lookup
    NULL,			// link
    NULL,			// unlink
    NULL,			// symlink
    NULL,			// mkdir
    NULL,			// rmdir
    NULL,			// mknod
    NULL,			// rename
    NULL,			// readlink
    NULL,			// follow_link
    generic_readpage,		// readpage
    NULL,			// writepage
    nwfs_bmap,			// bmap
    nwfs_truncate,		// truncate
    nwfs_permission,		// permission
    NULL,			// smap
};

#endif
