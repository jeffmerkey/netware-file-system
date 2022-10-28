
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
*   FILE     :  CREATE.C
*   DESCRIP  :  VFS Directory Create Module for Linux
*   DATE     :  January 26, 2022
*
*
***************************************************************************/

#include "globals.h"


#if (LINUX_24)

ULONG nwfs_to_linux_error(ULONG NwfsError)
{
    switch (NwfsError)
    {
       case NwSuccess:
	   return 0;

       case NwFatCorrupt:
       case NwFileCorrupt:
       case NwDirectoryCorrupt:
       case NwVolumeCorrupt:
       case NwHashCorrupt:
       case NwMissingSegment:
	  return -ENFILE;

       case NwHashError:
       case NwNoEntry:
	  return -ENOENT;

       case NwDiskIoError:
       case NwMirrorGroupFailure:
       case NwNoMoreMirrors:
	  return -EIO;

       case NwInsufficientResources:
	  return -ENOMEM;

       case NwFileExists:
	  return -EEXIST;

       case NwInvalidParameter:
       case NwBadName:
	  return -EINVAL;

       case NwVolumeFull:
	  return -ENOSPC;

       case NwNotEmpty:
	  return -ENOTEMPTY;

       case NwAccessError:
	  return -EACCES;

       case NwNotPermitted:
	  return -EPERM;

       case NwMemoryFault:
          return -EFAULT;

       case NwOtherError:
       case NwEndOfFile:
       default:
	  return -EACCES;
    }
}

int nwfs_create(struct inode *dir, struct dentry *dentry, int mode)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

#if (VFS_VERBOSE)
    NWFSPrint("create %s\n", dentry->d_name.name);
#endif

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in create\n");
       return -ENOENT;
    }

    if (!hash)
       return -ENOENT;

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
       return -EEXIST;

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				   0, dir->i_ino,
				   NW_NAMED_FILE,
				   current->fsuid,
				   ((dir->i_mode & S_ISGID)
				   ? dir->i_gid
				   : current->fsgid),
				   0, mode,
				   &DirNo,
				   (ULONG) -1, 0,
				   (ULONG) -1, 0,
				   0, 0);
    if (retCode)
       return (nwfs_to_linux_error(retCode));

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in create\n");
       return -EBADF;
    }

    inode->i_uid = current->fsuid;
    inode->i_gid = (dir->i_mode & S_ISGID) ? dir->i_gid : current->fsgid;
    mark_inode_dirty(inode);

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;

}

int nwfs_unlink(struct inode *dir, struct dentry *dentry)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash;
    register ULONG retCode;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;

#if (VFS_VERBOSE)
    NWFSPrint("unlink %s\n", name);
#endif

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory\n");
       return -EBADF;
    }

    hash = get_directory_hash(volume, name, namelen, dir->i_ino);
    if (!hash)
       return -ENOENT;

    ino = hash->DirNo;

    retCode = ReadDirectoryRecord(volume, dos, ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) read dir error [%d] (unlink)\n",
	      (int)ino, (int)retCode);
       return -EIO;
    }

#if (SALVAGEABLE_FS)

    retCode = NWShadowDirectoryEntry(volume, dos, hash);
    if (retCode)
       return nwfs_to_linux_error(retCode);

#else

    if (hash->Flags & HARD_LINKED_FILE)
    {
       retCode = NWDeleteDirectoryEntry(volume, dos, hash);
       if (retCode)
          return nwfs_to_linux_error(retCode);
    }
    else
    {
       retCode = NWDeleteDirectoryEntryAndData(volume, dos, hash);
       if (retCode)
          return nwfs_to_linux_error(retCode);
    }

#endif

    // adjust directory link count and size
    dir->i_ctime = dir->i_mtime = NWFSGetSystemTime();
    if (dir->i_nlink)
       dir->i_nlink--;
    if (dir->i_size)
       dir->i_size -= sizeof(ROOT);
    mark_inode_dirty(dir);

    inode->i_nlink--;
    inode->i_ctime = dir->i_ctime;
    mark_inode_dirty(inode);

    d_delete(dentry);

    return 0;

}

int nwfs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

#if (VFS_VERBOSE)
    NWFSPrint("mkdir %s\n", dentry->d_name.name);
#endif

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in mkdir\n");
       return -ENOTDIR;
    }

    if (!hash)
       return -ENOENT;

    // if there is a name collision, then report that the directory exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
       return -EEXIST;

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				     SUBDIRECTORY, dir->i_ino,
		 		     NW_SUBDIRECTORY_FILE,
				     current->fsuid,
				     ((dir->i_mode & S_ISGID)
				     ? dir->i_gid
				     : current->fsgid),
				     0, mode, &DirNo,
				     (ULONG) -1, 0,
				     (ULONG) -1, 0,
				     0, 0);
    if (retCode)
       return (nwfs_to_linux_error(retCode));

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in mkdir\n");
       return -EINVAL;
    }

    inode->i_uid = current->fsuid;
    inode->i_gid = (dir->i_mode & S_ISGID) ? dir->i_gid : current->fsgid;
    mark_inode_dirty(inode);

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;

}

int nwfs_rmdir(struct inode *dir, struct dentry *dentry)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *dirhash = (HASH *) dir->u.generic_ip;
    register HASH *hash;
    register ULONG retCode;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;

#if (VFS_VERBOSE)
    NWFSPrint("rmdir %s\n", name);
#endif

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory\n");
       return -ENOTDIR;
    }

    if (!dirhash)
       return -ENOENT;

    hash = get_directory_hash(volume, name, namelen, dir->i_ino);
    if (!hash)
       return -ENOENT;
    ino = hash->DirNo;

    // don't allow DELETED.SAV to be removed from the root directory
    if ((!hash->Parent) && (!NWFSCompare((BYTE *)name, "DELETED.SAV", 11)))
       return -EPERM;

    retCode = ReadDirectoryRecord(volume, dos, ino);
    if (retCode)
       return -EIO;

#if (SALVAGEABLE_FS)
    retCode = NWShadowDirectoryEntry(volume, dos, hash);
    if (retCode)
       return nwfs_to_linux_error(retCode);

#else

    // really delete the file from the volume
    retCode = NWDeleteDirectoryEntryAndData(volume, dos, hash);

    if (retCode)
       return nwfs_to_linux_error(retCode);
#endif

    // adjust directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    if (dir->i_nlink)
       dir->i_nlink--;
    if (dir->i_size)
       dir->i_size -= sizeof(ROOT);
    mark_inode_dirty(dir);

    // if we are deleting the last instance of a busy directory
    // then make directory empty

    inode->i_nlink = 0;  // zap the link count
    inode->i_size = 0;
    inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
    mark_inode_dirty(inode);

    d_delete(dentry);

    return 0;
}

int nwfs_rename(struct inode *oldDir, struct dentry *old_dentry,
		struct inode *newDir, struct dentry *new_dentry)
{
    const char *newName = new_dentry->d_name.name;
    size_t newLen = new_dentry->d_name.len;
    const char *oldName = old_dentry->d_name.name;
    size_t oldLen = old_dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) oldDir->i_sb->u.generic_sbp;
    register HASH *odirhash = (HASH *) oldDir->u.generic_ip;
    register HASH *ndirhash = (HASH *) newDir->u.generic_ip;
    register HASH *hash;
    register ULONG retCode, oldDirNo, newDirNo;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;

#if (VFS_VERBOSE)
    NWFSPrint("rename %s -> %s\n", old_dentry->d_name.name,
              new_dentry->d_name.name);
#endif

    if (!oldDir || !S_ISDIR(oldDir->i_mode))
    {
       NWFSPrint("nwfs: old inode is NULL or not a directory\n");
       return -EBADF;
    }

    if (!newDir || !S_ISDIR(newDir->i_mode))
    {
       NWFSPrint("nwfs_rename: new inode is NULL or not a directory\n");
       return -EBADF;
    }

    if (!odirhash)
       return -ENOENT;

    if (!ndirhash)
       return -ENOENT;

    oldDirNo = odirhash->DirNo;
    newDirNo = ndirhash->DirNo;

    // if there is a name collision, then remove the target file
    ino = get_directory_number(volume, newName, newLen, newDir->i_ino);
    if (ino)
    {
       retCode = nwfs_unlink(newDir, new_dentry);
       if (retCode)
          return retCode;
    }

    hash = get_directory_hash(volume, oldName, oldLen, oldDir->i_ino);
    if (!hash)
       return -ENOENT;

    // read source file record
    retCode = ReadDirectoryRecord(volume, dos, oldDir->i_ino);
    if (retCode)
       return -EIO;

    retCode = NWRenameEntry(volume, dos, hash, (BYTE *)newName, newLen,
			    newDir->i_ino);

    if (retCode)
       return (nwfs_to_linux_error(retCode));

    // adjust old directory link count and size
    oldDir->i_mtime = NWFSGetSystemTime();
    if (oldDir->i_nlink)
       oldDir->i_nlink--;
    if (oldDir->i_size)
       oldDir->i_size -= sizeof(ROOT);
    mark_inode_dirty(oldDir);

    // bump new directory link count and size
    newDir->i_mtime = NWFSGetSystemTime();
    newDir->i_nlink++;
    newDir->i_size += sizeof(ROOT);
    mark_inode_dirty(newDir);

    return 0;
}

int nwfs_symlink(struct inode *dir, struct dentry *dentry, const char *path)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

#if (VFS_VERBOSE)
    NWFSPrint("symlink %s\n", dentry->d_name.name);
#endif

    // don't allow symlinks unless the NFS namespace is present
    if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
       return -EPERM;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in symlink\n");
       return -EBADF;
    }

    if (!hash)
       return -ENOENT;

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
       return -EEXIST;

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				     0, dir->i_ino,
				     NW_SYMBOLIC_FILE,
				     current->fsuid,
				     ((dir->i_mode & S_ISGID)
				     ? dir->i_gid
				     : current->fsgid),
				     0,
				     (S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO),
				     &DirNo,
				     (ULONG) -1, 0,
				     (ULONG) -1, 0,
				     path, strlen(path));

    if (retCode)
       return (nwfs_to_linux_error(retCode));

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in symlink\n");
       return -EBADF;
    }

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;
}

int nwfs_link(struct dentry *olddentry, struct inode *dir, struct dentry *dentry)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *dirhash = (HASH *) dir->u.generic_ip;
    struct inode *oldinode = olddentry->d_inode;
    register HASH *hash = (HASH *) oldinode->u.generic_ip;
    register HASH *newHash;
    register ULONG retCode;
    ULONG DirNo, NFSDirNo, NewNFSDirNo;
    register struct inode *inode;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;
    NFS nfsst;
    register NFS *nfs = &nfsst;
    NFS nfsNewst;
    register NFS *nfsNew = &nfsNewst;

#if (VFS_VERBOSE)
    NWFSPrint("link %s\n", dentry->d_name.name);
#endif

    // don't allow hard links unless the NFS namespace is present
    if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
       return -EPERM;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in hardlink\n");
       return -EBADF;
    }

    // get the target file hash
    if (!dirhash)
       return -ENOENT;

    // get the target file hash
    if (!hash)
       return -ENOENT;

    if (hash->Flags & SUBDIRECTORY_FILE)
       return -EPERM;

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
       return -EEXIST;

    // get the nfs name space directory number for this file
    NFSDirNo = get_namespace_directory_number(volume, hash, UNIX_NAME_SPACE);
    if (NFSDirNo == (ULONG) -1)
       return -ENOENT;

    retCode = ReadDirectoryRecord(volume, (DOS *)nfs, NFSDirNo);
    if (retCode)
       return -EIO;

    // read the root directory record
    retCode = ReadDirectoryRecord(volume, dos, hash->DirNo);
    if (retCode)
       return -EIO;

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				     0, dir->i_ino,
				     NW_HARD_LINKED_FILE,
				     current->fsuid,
				     ((dir->i_mode & S_ISGID)
				     ? dir->i_gid
				     : current->fsgid),
				     nfs->rdev, nfs->mode,
				     &DirNo,
				     dos->FirstBlock,
				     dos->FileSize,
				     (ULONG) -1, 0,
				     0, 0);

    if (retCode)
       return (nwfs_to_linux_error(retCode));

    // get the new file hash
    newHash = get_directory_record(volume, DirNo);
    if (!newHash)
       return -ENOENT;

    // get the nfs name space directory number for this file
    NewNFSDirNo = get_namespace_directory_number(volume, newHash, UNIX_NAME_SPACE);
    if (NewNFSDirNo == (ULONG) -1)
       return -ENOENT;

    // read new NFS record
    retCode = ReadDirectoryRecord(volume, (DOS *)nfsNew, NewNFSDirNo);
    if (retCode)
       return -EIO;

    nfs->LinkedFlag = NFS_HASSTREAM_HARD_LINK;
    nfs->FirstCreated = 0;
//    nfs->nlinks++;

    nfsNew->LinkedFlag = NFS_HARD_LINK;
    nfsNew->FirstCreated = 1;
    nfsNew->mode = 0;
    nfsNew->LinkEndDirNo = NFSDirNo;

    // write the new NFS record
    retCode = WriteDirectoryRecord(volume, (DOS *)nfsNew, NewNFSDirNo);
    if (retCode)
       return -EIO;

    // write the root NFS record
    retCode = WriteDirectoryRecord(volume, (DOS *)nfs, NFSDirNo);
    if (retCode)
       return -EIO;

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in link\n");
       return -EBADF;
    }

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;

}

int nwfs_mknod(struct inode *dir, struct dentry *dentry, int mode, int rdev)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

#if (VFS_VERBOSE)
    NWFSPrint("mknod %s\n", dentry->d_name.name);
#endif

    // don't allow device files unless the NFS namespace is present
    if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
       return -EPERM;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in mknod\n");
       return -EBADF;
    }

    if (!hash)
       return -ENOENT;

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
       return -EEXIST;

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				   0, dir->i_ino,
				   NW_DEVICE_FILE,
				   current->fsuid,
				   ((dir->i_mode & S_ISGID)
				   ? dir->i_gid
				   : current->fsgid),
				   rdev, mode,
				   &DirNo,
				   (ULONG) -1, 0,
				   (ULONG) -1, 0,
				   0, 0);

    if (retCode)
       return (nwfs_to_linux_error(retCode));

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in mknod\n");
       return -EBADF;
    }

#if (VFS_VERBOSE)
    NWFSPrint("nwfs:  mknod mode-%X rdev-%X inode-mode-%X\n",
	      (unsigned int)mode, (unsigned int)rdev,
	      (unsigned int)inode->i_mode);

    NWFSPrint("nwfs:  IFBLK-%X IFCHR-%X IFLNK-%X\n",
	      (unsigned int)S_IFBLK,(unsigned int)S_IFCHR,
	      (unsigned int)S_IFLNK);
#endif

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;
}

#endif


#if (LINUX_22)

ULONG nwfs_to_linux_error(ULONG NwfsError)
{
    switch (NwfsError)
    {
       case NwSuccess:
	   return 0;

       case NwFatCorrupt:
       case NwFileCorrupt:
       case NwDirectoryCorrupt:
       case NwVolumeCorrupt:
       case NwHashCorrupt:
       case NwMissingSegment:
	  return -ENFILE;

       case NwHashError:
       case NwNoEntry:
	  return -ENOENT;

       case NwDiskIoError:
       case NwMirrorGroupFailure:
       case NwNoMoreMirrors:
	  return -EIO;

       case NwInsufficientResources:
	  return -ENOMEM;

       case NwFileExists:
	  return -EEXIST;

       case NwInvalidParameter:
       case NwBadName:
	  return -EINVAL;

       case NwVolumeFull:
	  return -ENOSPC;

       case NwNotEmpty:
	  return -ENOTEMPTY;

       case NwAccessError:
	  return -EACCES;

       case NwNotPermitted:
	  return -EPERM;

       case NwMemoryFault:
          return -EFAULT;

       case NwOtherError:
       case NwEndOfFile:
       default:
	  return -EACCES;
    }
}

int nwfs_create(struct inode *dir, struct dentry *dentry, int mode)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

#if (VFS_VERBOSE)
    NWFSPrint("create %s\n", dentry->d_name.name);
#endif

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in create\n");
       return -ENOENT;
    }

    if (!hash)
       return -ENOENT;

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
       return -EEXIST;

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				   0, dir->i_ino,
				   NW_NAMED_FILE,
				   current->fsuid,
				   ((dir->i_mode & S_ISGID)
				   ? dir->i_gid
				   : current->fsgid),
				   0, mode,
				   &DirNo,
				   (ULONG) -1, 0,
				   (ULONG) -1, 0,
				   0, 0);
    if (retCode)
       return (nwfs_to_linux_error(retCode));

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in create\n");
       return -EBADF;
    }

    inode->i_uid = current->fsuid;
    inode->i_gid = (dir->i_mode & S_ISGID) ? dir->i_gid : current->fsgid;
    mark_inode_dirty(inode);

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;

}

int nwfs_unlink(struct inode *dir, struct dentry *dentry)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *dirhash = (HASH *) dir->u.generic_ip;
    register HASH *hash;
    register ULONG retCode;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;

#if (VFS_VERBOSE)
    NWFSPrint("unlink %s\n", name);
#endif

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory\n");
       return -EBADF;
    }

    if (!dirhash)
       return -ENOENT;

    hash = get_directory_hash(volume, name, namelen, dir->i_ino);
    if (!hash)
       return -ENOENT;
    ino = hash->DirNo;

    retCode = ReadDirectoryRecord(volume, dos, ino);
    if (retCode)
       return -EIO;

#if (SALVAGEABLE_FS)

    retCode = NWShadowDirectoryEntry(volume, dos, hash);
    if (retCode)
       return nwfs_to_linux_error(retCode);
#else

    if (hash->Flags & HARD_LINKED_FILE)
    {
       retCode = NWDeleteDirectoryEntry(volume, dos, hash);
       if (retCode)
          return nwfs_to_linux_error(retCode);
    }
    else
    {
       retCode = NWDeleteDirectoryEntryAndData(volume, dos, hash);
       if (retCode)
          return nwfs_to_linux_error(retCode);
    }

#endif

    // adjust directory link count and size
    dir->i_ctime = dir->i_mtime = NWFSGetSystemTime();
    if (dir->i_nlink)
       dir->i_nlink--;
    if (dir->i_size)
       dir->i_size -= sizeof(ROOT);
    mark_inode_dirty(dir);

    inode->i_nlink--;
    inode->i_ctime = dir->i_ctime;
    mark_inode_dirty(inode);

    d_delete(dentry);

    return 0;

}

int nwfs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

#if (VFS_VERBOSE)
    NWFSPrint("mkdir %s\n", dentry->d_name.name);
#endif

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in mkdir\n");
       return -ENOTDIR;
    }

    if (!hash)
       return -ENOENT;

    // if there is a name collision, then report that the directory exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
       return -EEXIST;

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				     SUBDIRECTORY, dir->i_ino,
				     NW_SUBDIRECTORY_FILE,
				     current->fsuid,
				     ((dir->i_mode & S_ISGID)
				     ? dir->i_gid
				     : current->fsgid),
				     0, mode, &DirNo,
				     (ULONG) -1, 0,
				     (ULONG) -1, 0,
				     0, 0);
    if (retCode)
       return (nwfs_to_linux_error(retCode));

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
       return -EINVAL;

    inode->i_uid = current->fsuid;
    inode->i_gid = (dir->i_mode & S_ISGID) ? dir->i_gid : current->fsgid;
    mark_inode_dirty(inode);

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;

}

int nwfs_rmdir(struct inode *dir, struct dentry *dentry)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *dirhash = (HASH *) dir->u.generic_ip;
    register HASH *hash;
    register ULONG retCode;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;

#if (VFS_VERBOSE)
    NWFSPrint("rmdir %s\n", name);
#endif

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory\n");
       return -ENOTDIR;
    }

    if (!dirhash)
       return -ENOENT;

    hash = get_directory_hash(volume, name, namelen, dir->i_ino);
    if (!hash)
       return -ENOENT;
    ino = hash->DirNo;

    // don't allow DELETED.SAV to be removed from the root directory
    if ((!hash->Parent) && (!NWFSCompare((BYTE *)name, "DELETED.SAV", 11)))
       return -EPERM;

    retCode = ReadDirectoryRecord(volume, dos, ino);
    if (retCode)
       return -EIO;

#if (SALVAGEABLE_FS)
    retCode = NWShadowDirectoryEntry(volume, dos, hash);
    if (retCode)
       return nwfs_to_linux_error(retCode);

#else

    // really delete the file from the volume
    retCode = NWDeleteDirectoryEntryAndData(volume, dos, hash);

    if (retCode)
       return nwfs_to_linux_error(retCode);
#endif

    // adjust directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    if (dir->i_nlink)
       dir->i_nlink--;
    if (dir->i_size)
       dir->i_size -= sizeof(ROOT);
    mark_inode_dirty(dir);

    // if we are deleting the last instance of a busy directory
    // then make directory empty

    inode->i_nlink = 0;  // zap the link count
    inode->i_size = 0;
    inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
    mark_inode_dirty(inode);

    d_delete(dentry);

    return 0;
}

int nwfs_rename(struct inode *oldDir, struct dentry *old_dentry,
		struct inode *newDir, struct dentry *new_dentry)
{
    const char *newName = new_dentry->d_name.name;
    size_t newLen = new_dentry->d_name.len;
    const char *oldName = old_dentry->d_name.name;
    size_t oldLen = old_dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) oldDir->i_sb->u.generic_sbp;
    register HASH *odirhash = (HASH *) oldDir->u.generic_ip;
    register HASH *ndirhash = (HASH *) newDir->u.generic_ip;
    register HASH *hash;
    register ULONG retCode, oldDirNo, newDirNo;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;

#if (VFS_VERBOSE)
    NWFSPrint("rename %s -> %s\n", old_dentry->d_name.name,
              new_dentry->d_name.name);
#endif

    if (!oldDir || !S_ISDIR(oldDir->i_mode))
    {
       NWFSPrint("nwfs: old inode is NULL or not a directory\n");
       return -EBADF;
    }

    if (!newDir || !S_ISDIR(newDir->i_mode))
    {
       NWFSPrint("nwfs_rename: new inode is NULL or not a directory\n");
       return -EBADF;
    }

    if (!odirhash)
       return -ENOENT;

    if (!ndirhash)
       return -ENOENT;

    oldDirNo = odirhash->DirNo;
    newDirNo = ndirhash->DirNo;

    // if there is a name collision, then remove the target file
    ino = get_directory_number(volume, newName, newLen, newDir->i_ino);
    if (ino)
    {
       retCode = nwfs_unlink(newDir, new_dentry);
       if (retCode)
          return retCode;
    }

    hash = get_directory_hash(volume, oldName, oldLen, oldDir->i_ino);
    if (!hash)
       return -ENOENT;

    // read source file record
    retCode = ReadDirectoryRecord(volume, dos, oldDir->i_ino);
    if (retCode)
       return -EIO;

    retCode = NWRenameEntry(volume, dos, hash, (BYTE *)newName, newLen,
			    newDir->i_ino);

    if (retCode)
       return (nwfs_to_linux_error(retCode));

    // adjust old directory link count and size
    oldDir->i_mtime = NWFSGetSystemTime();
    if (oldDir->i_nlink)
       oldDir->i_nlink--;
    if (oldDir->i_size)
       oldDir->i_size -= sizeof(ROOT);
    mark_inode_dirty(oldDir);

    // bump new directory link count and size
    newDir->i_mtime = NWFSGetSystemTime();
    newDir->i_nlink++;
    newDir->i_size += sizeof(ROOT);
    mark_inode_dirty(newDir);

    return 0;
}

int nwfs_symlink(struct inode *dir, struct dentry *dentry, const char *path)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

#if (VFS_VERBOSE)
    NWFSPrint("symlink %s -> %s\n", dentry->d_name.name, path);
#endif

    // don't allow symlinks unless the NFS namespace is present
    if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
       return -EPERM;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in symlink\n");
       return -EBADF;
    }

    if (!hash)
       return -ENOENT;

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
       return -EEXIST;

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				     0, dir->i_ino,
				     NW_SYMBOLIC_FILE,
				     current->fsuid,
				     ((dir->i_mode & S_ISGID)
				     ? dir->i_gid
				     : current->fsgid),
				     0,
				     (S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO),
				     &DirNo,
				     (ULONG) -1, 0,
				     (ULONG) -1, 0,
				     path, strlen(path));

    if (retCode)
       return (nwfs_to_linux_error(retCode));

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in symlink\n");
       return -EBADF;
    }

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;
}

int nwfs_link(struct dentry *olddentry, struct inode *dir, struct dentry *dentry)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *dirhash = (HASH *) dir->u.generic_ip;
    struct inode *oldinode = olddentry->d_inode;
    register HASH *hash = (HASH *) oldinode->u.generic_ip;
    register HASH *newHash;
    register ULONG retCode;
    ULONG DirNo, NFSDirNo, NewNFSDirNo;
    register struct inode *inode;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;
    NFS nfsst;
    register NFS *nfs = &nfsst;
    NFS nfsNewst;
    register NFS *nfsNew = &nfsNewst;

#if (VFS_VERBOSE)
    NWFSPrint("link %s - > %s\n", dentry->d_name.name, olddentry->d_name.name);
#endif

    // don't allow hard links unless the NFS namespace is present
    if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
       return -EPERM;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in hardlink\n");
       return -EBADF;
    }

    // get the target file hash
    if (!dirhash)
       return -ENOENT;

    // get the target file hash
    if (!hash)
       return -ENOENT;

    if (hash->Flags & SUBDIRECTORY_FILE)
       return -EPERM;

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
       return -EEXIST;

    // get the nfs name space directory number for this file
    NFSDirNo = get_namespace_directory_number(volume, hash, UNIX_NAME_SPACE);
    if (NFSDirNo == (ULONG) -1)
       return -ENOENT;

    retCode = ReadDirectoryRecord(volume, (DOS *)nfs, NFSDirNo);
    if (retCode)
       return -EIO;

    // read the root directory record
    retCode = ReadDirectoryRecord(volume, dos, hash->DirNo);
    if (retCode)
       return -EIO;

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				     0, dir->i_ino,
				     NW_HARD_LINKED_FILE,
				     current->fsuid,
				     ((dir->i_mode & S_ISGID)
				     ? dir->i_gid
				     : current->fsgid),
				     nfs->rdev, nfs->mode,
				     &DirNo,
				     dos->FirstBlock,
				     dos->FileSize,
				     (ULONG) -1, 0,
				     0, 0);

    if (retCode)
       return (nwfs_to_linux_error(retCode));

    // get the new file hash
    newHash = get_directory_record(volume, DirNo);
    if (!newHash)
       return -ENOENT;

    // get the nfs name space directory number for this file
    NewNFSDirNo = get_namespace_directory_number(volume, newHash, 
		                                 UNIX_NAME_SPACE);
    if (NewNFSDirNo == (ULONG) -1)
       return -ENOENT;

    // read new NFS record
    retCode = ReadDirectoryRecord(volume, (DOS *)nfsNew, NewNFSDirNo);
    if (retCode)
       return -EIO;

    nfs->LinkedFlag = NFS_HASSTREAM_HARD_LINK;
    nfs->FirstCreated = 0;
//    nfs->nlinks++;

    nfsNew->LinkedFlag = NFS_HARD_LINK;
    nfsNew->FirstCreated = 1;
    nfsNew->mode = 0;
    nfsNew->LinkEndDirNo = NFSDirNo;

    // write the new NFS record
    retCode = WriteDirectoryRecord(volume, (DOS *)nfsNew, NewNFSDirNo);
    if (retCode)
       return -EIO;

    // write the root NFS record
    retCode = WriteDirectoryRecord(volume, (DOS *)nfs, NFSDirNo);
    if (retCode)
       return -EIO;

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in link\n");
       return -EBADF;
    }

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;

}

int nwfs_mknod(struct inode *dir, struct dentry *dentry, int mode, int rdev)
{
    const char *name = dentry->d_name.name;
    size_t namelen = dentry->d_name.len;
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

#if (VFS_VERBOSE)
    NWFSPrint("mknod %s\n", dentry->d_name.name);
#endif

    // don't allow device files unless the NFS namespace is present
    if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
       return -EPERM;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in mknod\n");
       return -EBADF;
    }

    if (!hash)
       return -ENOENT;

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
       return -EEXIST;

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				   0, dir->i_ino,
				   NW_DEVICE_FILE,
				   current->fsuid,
				   ((dir->i_mode & S_ISGID)
				   ? dir->i_gid
				   : current->fsgid),
				   rdev, mode,
				   &DirNo,
				   (ULONG) -1, 0,
				   (ULONG) -1, 0,
				   0, 0);

    if (retCode)
       return (nwfs_to_linux_error(retCode));

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in mknod\n");
       return -EBADF;
    }

#if (VFS_VERBOSE)
    NWFSPrint("nwfs:  mknod mode-%X (%s%s%s%s) rdev-%X inode-mode-%X\n",
	      (unsigned int)mode, 
	      (S_ISLNK(mode) ? " LNK  "  : ""),
	      (S_ISBLK(mode) ? " BLK  "  : ""),
	      (S_ISCHR(mode) ? " CHAR "  : ""),
	      (S_ISDIR(mode) ? " DIR  "  : ""),
	      (unsigned int)rdev,
	      (unsigned int)inode->i_mode);

    NWFSPrint("nwfs:  IFBLK-%X IFCHR-%X IFLNK-%X\n",
	      (unsigned int)S_IFBLK,(unsigned int)S_IFCHR,
	      (unsigned int)S_IFLNK);
#endif

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    mark_inode_dirty(dir);

    d_instantiate(dentry, inode);

    return 0;
}

#endif


#if (LINUX_20)

ULONG nwfs_to_linux_error(ULONG NwfsError)
{
    switch (NwfsError)
    {
       case NwSuccess:
	   return 0;

       case NwFatCorrupt:
       case NwFileCorrupt:
       case NwDirectoryCorrupt:
       case NwVolumeCorrupt:
       case NwHashCorrupt:
       case NwMissingSegment:
	  return -ENFILE;

       case NwHashError:
       case NwNoEntry:
	  return -ENOENT;

       case NwDiskIoError:
       case NwMirrorGroupFailure:
       case NwNoMoreMirrors:
	  return -EIO;

       case NwInsufficientResources:
	  return -ENOMEM;

       case NwFileExists:
	  return -EEXIST;

       case NwInvalidParameter:
       case NwBadName:
	  return -EINVAL;

       case NwVolumeFull:
	  return -ENOSPC;

       case NwNotEmpty:
	  return -ENOTEMPTY;

       case NwAccessError:
	  return -EACCES;

       case NwNotPermitted:
	  return -EPERM;

       case NwMemoryFault:
          return -EFAULT;

       case NwOtherError:
       case NwEndOfFile:
       default:
	  return -EACCES;
    }
}

int nwfs_create(struct inode *dir, const char *name, int namelen,
		  int mode, struct inode **result)
{
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

    if (result)
       *result = NULL;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in create\n");
       iput(dir);
       return -ENOENT;
    }

    if (!hash)
    {
       iput(dir);
       return -ENOENT;
    }

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
    {
       // we found a matching entry
       iput(dir);
       return -EEXIST;
    }

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				   0, dir->i_ino,
				   NW_NAMED_FILE,
				   current->fsuid,
				   ((dir->i_mode & S_ISGID)
				   ? dir->i_gid
				   : current->fsgid),
				   0, mode,
				   &DirNo,
				   (ULONG) -1, 0,
				   (ULONG) -1, 0,
				   0, 0);
    if (retCode)
    {
       iput(dir);
       return (nwfs_to_linux_error(retCode));
    }

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in create\n");
       iput(dir);
       return -EBADF;
    }

    // the caller is assumed to be the one doing an iput
    // for created inodes

    inode->i_uid = current->fsuid;
    inode->i_gid = (dir->i_mode & S_ISGID) ? dir->i_gid : current->fsgid;
    inode->i_dirt = 1;

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    dir->i_dirt = 1;

    if (result)
       *result = inode;

    iput(dir);
    return 0;

}

int nwfs_unlink(struct inode *dir, const char *name, int namelen)
{
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *dirhash = (HASH *) dir->u.generic_ip;
    register HASH *hash;
    register ULONG retCode;
    register ino_t ino;
    register struct inode *inode;
    DOS dosst;
    register DOS *dos = &dosst;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory\n");
       iput(dir);
       return -EBADF;
    }

    // if we are asked to delete the '.' or '..' entries, say
    // we did, and exit.

    if (((namelen == 1) && (name[0] == '.')) ||
       ((namelen == 2) && (name[0] == '.') && (name[1] == '.')))
    {
       iput(dir);
       return 0;
    }

    if (!dirhash)
    {
       iput(dir);
       return -ENOENT;
    }

    hash = get_directory_hash(volume, name, namelen, dir->i_ino);
    if (!hash)
    {
       iput(dir);
       return -ENOENT;
    }
    ino = hash->DirNo;

    if (!(inode = iget(dir->i_sb, ino)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in unlink\n");
       iput(dir);
       return -EBADF;
    }

    // do not allow '.' to be deleted, however, '../dir' is ok
    if (inode == dir)
    {
       iput(inode);
       iput(dir);
       return -EPERM;
    }

    retCode = ReadDirectoryRecord(volume, dos, ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) read dir error [%d] (unlink)\n",
	      (int)ino, (int)retCode);
       iput(dir);
       return -EIO;
    }

#if (SALVAGEABLE_FS)

    retCode = NWShadowDirectoryEntry(volume, dos, hash);
    if (retCode)
    {
       iput(inode);
       iput(dir);
       return nwfs_to_linux_error(retCode);
    }

#else

    if (hash->Flags & HARD_LINKED_FILE)
    {
       retCode = NWDeleteDirectoryEntry(volume, dos, hash);
       if (retCode)
       {
          iput(inode);
          iput(dir);
          return nwfs_to_linux_error(retCode);
       }
    }
    else
    {
       retCode = NWDeleteDirectoryEntryAndData(volume, dos, hash);
       if (retCode)
       {
          iput(inode);
          iput(dir);
          return nwfs_to_linux_error(retCode);
       }
    }

#endif

    // adjust directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    if (dir->i_nlink)
       dir->i_nlink--;
    if (dir->i_size)
       dir->i_size -= sizeof(ROOT);
    dir->i_dirt = 1;

    inode->i_nlink--;
    inode->i_dirt = 1;

    iput(inode);
    iput(dir);
    return 0;

}

int nwfs_mkdir(struct inode *dir, const char *name,
		 int namelen, int mode)
{
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

    if ((name[0] == '.') && ((namelen == 1) || ((namelen == 2) &&
	(name[1] == '.'))))
    {
       iput(dir);
       return -EEXIST;
    }

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in mkdir\n");
       iput(dir);
       return -ENOTDIR;
    }

    if (!hash)
    {
       iput(dir);
       return -ENOENT;
    }

    // if there is a name collision, then report that the directory exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
    {
       // we found a matching entry
       iput(dir);
       return -EEXIST;
    }

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				     SUBDIRECTORY, dir->i_ino,
				     NW_SUBDIRECTORY_FILE,
				     current->fsuid,
				     ((dir->i_mode & S_ISGID)
				     ? dir->i_gid
				     : current->fsgid),
				     0, mode, &DirNo,
				     (ULONG) -1, 0,
				     (ULONG) -1, 0,
				     0, 0);
    if (retCode)
    {
       iput(dir);
       return (nwfs_to_linux_error(retCode));
    }

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in mkdir\n");
       iput(dir);
       return -EINVAL;
    }

    inode->i_uid = current->fsuid;
    inode->i_gid = (dir->i_mode & S_ISGID) ? dir->i_gid : current->fsgid;
    inode->i_dirt = 1;

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    dir->i_dirt = 1;

    iput(inode);
    iput(dir);
    return 0;

}

int nwfs_rmdir(struct inode *dir, const char *name, int namelen)
{
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *dirhash = (HASH *) dir->u.generic_ip;
    register HASH *hash;
    register ULONG retCode;
#if (CHECK_NLINK)
    register ULONG i;
#endif
    register struct inode *inode;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory\n");
       iput(dir);
       return -ENOTDIR;
    }

    // if we are asked to delete the '.' or '..' entries, say
    // we did, and exit.

    if (((namelen == 1) && (name[0] == '.')) ||
       ((namelen == 2) && (name[0] == '.') && (name[1] == '.')))
    {
       iput(dir);
       return 0;
    }

    if (!dirhash)
    {
       iput(dir);
       return -ENOENT;
    }

    hash = get_directory_hash(volume, name, namelen, dir->i_ino);
    if (!hash)
    {
       iput(dir);
       return -ENOENT;
    }
    ino = hash->DirNo;

    // don't allow DELETED.SAV to be removed from the root directory
    if ((!hash->Parent) && (!NWFSCompare((BYTE *)name, "DELETED.SAV", 11)))
    {
       iput(dir);
       return -EPERM;
    }

    retCode = ReadDirectoryRecord(volume, dos, ino);
    if (retCode)
    {
       iput(dir);
       return -EIO;
    }

#if (SALVAGEABLE_FS)
    retCode = NWShadowDirectoryEntry(volume, dos, hash);
    if (retCode)
    {
       iput(dir);
       return nwfs_to_linux_error(retCode);
    }

#else

    // really delete the file from the volume
    retCode = NWDeleteDirectoryEntryAndData(volume, dos, hash);

    if (retCode)
    {
       iput(dir);
       return nwfs_to_linux_error(retCode);
    }
#endif

    if (!(inode = iget(dir->i_sb, ino)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in rmdir\n");
       iput(dir);
       return -EBADF;
    }

    // adjust directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    if (dir->i_nlink)
       dir->i_nlink--;
    if (dir->i_size)
       dir->i_size -= sizeof(ROOT);
    dir->i_dirt = 1;

#if (CHECK_NLINK)
    if (inode->i_nlink != 2)
    {
       NWFSPrint("nwfs:  empty dir inode has nlink error (%X) [",
		 (unsigned int) inode->i_ino);

       for (i=0; i < namelen; i++)
	  NWFSPrint("%c", name[i]);
       NWFSPrint("]\n");
    }
#endif

    // if we are deleting the last instance of a busy directory
    // then make directory empty
    if (inode->i_count > 1)
       inode->i_size = 0;

    inode->i_nlink = 0;  // zap the link count
    inode->i_dirt = 1;

    iput(inode);
    iput(dir);

    return 0;
}

int nwfs_rename(struct inode *oldDir, const char *oldName, int oldLen,
		  struct inode *newDir, const char *newName, int newLen,
		  int must_be_dir)
{
    register VOLUME *volume = (VOLUME *) oldDir->i_sb->u.generic_sbp;
    register HASH *odirhash = (HASH *) oldDir->u.generic_ip;
    register HASH *ndirhash = (HASH *) newDir->u.generic_ip;
    register HASH *hash;
    register ULONG retCode, oldDirNo, newDirNo;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;

    if (!oldDir || !S_ISDIR(oldDir->i_mode))
    {
       NWFSPrint("nwfs: old inode is NULL or not a directory\n");
       iput(oldDir);
       iput(newDir);
       return -EBADF;
    }

    if (!newDir || !S_ISDIR(newDir->i_mode))
    {
       NWFSPrint("nwfs_rename: new inode is NULL or not a directory\n");
       iput(oldDir);
       iput(newDir);
       return -EBADF;
    }

    if (!odirhash)
    {
       iput(oldDir);
       iput(newDir);
       return -ENOENT;
    }

    if (!ndirhash)
    {
       iput(oldDir);
       iput(newDir);
       return -ENOENT;
    }

    oldDirNo = odirhash->DirNo;
    newDirNo = ndirhash->DirNo;

    // if there is a name collision, then remove the target file
    ino = get_directory_number(volume, newName, newLen, newDir->i_ino);
    if (ino)
    {
       retCode = nwfs_unlink(newDir, newName, newLen);
       if (retCode)
       {
          iput(oldDir);
          iput(newDir);
          return retCode;
       }
    }

    hash = get_directory_hash(volume, oldName, oldLen, oldDir->i_ino);
    if (!hash)
    {
       iput(oldDir);
       iput(newDir);
       return -ENOENT;
    }

    // read source file record
    retCode = ReadDirectoryRecord(volume, dos, oldDir->i_ino);
    if (retCode)
    {
       iput(oldDir);
       iput(newDir);
       return -EIO;
    }

    retCode = NWRenameEntry(volume, dos, hash,
			    (BYTE *)newName, newLen,
			    newDir->i_ino);

    if (retCode)
    {
       iput(oldDir);
       iput(newDir);
       return (nwfs_to_linux_error(retCode));
    }

    // adjust old directory link count and size
    oldDir->i_mtime = NWFSGetSystemTime();
    if (oldDir->i_nlink)
       oldDir->i_nlink--;
    if (oldDir->i_size)
       oldDir->i_size -= sizeof(ROOT);
    oldDir->i_dirt = 1;

    // bump new directory link count and size
    newDir->i_mtime = NWFSGetSystemTime();
    newDir->i_nlink++;
    newDir->i_size += sizeof(ROOT);
    newDir->i_dirt = 1;

    iput(oldDir);
    iput(newDir);

    return 0;
}

int nwfs_symlink(struct inode *dir, const char *name,
		 int namelen, const char *path)
{
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

    // don't allow symlinks unless the NFS namespace is present
    if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
       return -EPERM;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in symlink\n");
       iput(dir);
       return -EBADF;
    }

    if (!hash)
    {
       iput(dir);
       return -ENOENT;
    }

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
    {
       // we found a matching entry
       iput(dir);
       return -EEXIST;
    }

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				     0, dir->i_ino,
				     NW_SYMBOLIC_FILE,
				     current->fsuid,
				     ((dir->i_mode & S_ISGID)
				     ? dir->i_gid
				     : current->fsgid),
				     0,
				     (S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO),
				     &DirNo,
				     (ULONG) -1, 0,
				     (ULONG) -1, 0,
				     path, strlen(path));

    if (retCode)
    {
       iput(dir);
       return (nwfs_to_linux_error(retCode));
    }

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in symlink\n");
       iput(dir);
       return -EBADF;
    }

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    dir->i_dirt = 1;

    iput(inode);
    iput(dir);
    return 0;
}

int nwfs_link(struct inode *oldinode, struct inode *dir,
	      const char *name, int namelen)
{
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *dirhash = (HASH *) dir->u.generic_ip;
    register HASH *hash = (HASH *) oldinode->u.generic_ip;
    register HASH *newHash;
    register ULONG retCode;
    ULONG DirNo, NFSDirNo, NewNFSDirNo;
    register struct inode *inode;
    register ino_t ino;
    DOS dosst;
    register DOS *dos = &dosst;
    NFS nfsst;
    register NFS *nfs = &nfsst;
    NFS nfsNewst;
    register NFS *nfsNew = &nfsNewst;

    // don't allow hard links unless the NFS namespace is present
    if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
       return -EPERM;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in hardlink\n");
       iput(dir);
       return -EBADF;
    }

    // get the target file hash
    if (!dirhash)
    {
       iput(dir);
       return -ENOENT;
    }

    // get the target file hash
    if (!hash)
    {
       iput(dir);
       return -ENOENT;
    }

    if (hash->Flags & SUBDIRECTORY_FILE)
    {
       iput(dir);
       return -EPERM;
    }

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
    {
       // we found a matching entry
       iput(dir);
       return -EEXIST;
    }

    // get the nfs name space directory number for this file
    NFSDirNo = get_namespace_directory_number(volume, hash, UNIX_NAME_SPACE);
    if (NFSDirNo == (ULONG) -1)
    {
       iput(dir);
       return -ENOENT;
    }

    // read the nfs directory information
    retCode = ReadDirectoryRecord(volume, (DOS *)nfs, NFSDirNo);
    if (retCode)
    {
       iput(dir);
       return -EIO;
    }

    // read the root directory record
    retCode = ReadDirectoryRecord(volume, dos, hash->DirNo);
    if (retCode)
    {
       iput(dir);
       return -EIO;
    }

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				     0, dir->i_ino,
				     NW_HARD_LINKED_FILE,
				     current->fsuid,
				     ((dir->i_mode & S_ISGID)
				     ? dir->i_gid
				     : current->fsgid),
				     nfs->rdev, nfs->mode,
				     &DirNo,
				     dos->FirstBlock,
				     dos->FileSize,
				     (ULONG) -1, 0,
				     0, 0);

    if (retCode)
    {
       iput(dir);
       return (nwfs_to_linux_error(retCode));
    }

    // get the new file hash
    newHash = get_directory_record(volume, DirNo);
    if (!newHash)
    {
       iput(dir);
       return -ENOENT;
    }

    // get the nfs name space directory number for this file
    NewNFSDirNo = get_namespace_directory_number(volume, newHash, 
		                                 UNIX_NAME_SPACE);
    if (NewNFSDirNo == (ULONG) -1)
    {
       iput(dir);
       return -ENOENT;
    }

    // read new NFS record
    retCode = ReadDirectoryRecord(volume, (DOS *)nfsNew, NewNFSDirNo);
    if (retCode)
    {
       iput(dir);
       return -EIO;
    }

    nfs->LinkedFlag = NFS_HASSTREAM_HARD_LINK;
    nfs->FirstCreated = 0;
//    nfs->nlinks++;

    nfsNew->LinkedFlag = NFS_HARD_LINK;
    nfsNew->FirstCreated = 1;
    nfsNew->mode = 0;
    nfsNew->LinkEndDirNo = NFSDirNo;

    // write the new NFS record
    retCode = WriteDirectoryRecord(volume, (DOS *)nfsNew, NewNFSDirNo);
    if (retCode)
    {
       iput(dir);
       return -EIO;
    }

    // write the root NFS record
    retCode = WriteDirectoryRecord(volume, (DOS *)nfs, NFSDirNo);
    if (retCode)
    {
       iput(dir);
       return -EIO;
    }

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in link\n");
       iput(dir);
       return -EBADF;
    }

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    dir->i_dirt = 1;

    iput(inode);
    iput(oldinode);
    iput(dir);

    return 0;
}

int nwfs_mknod(struct inode *dir, const char *name,
	       int namelen, int mode, int rdev)
{
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) dir->u.generic_ip;
    register ULONG retCode;
    ULONG DirNo;
    register struct inode *inode;
    register ino_t ino;

    // don't allow device files unless the NFS namespace is present
    if (volume->NameSpaceDefault != UNIX_NAME_SPACE)
       return -EPERM;

    if (!dir || !S_ISDIR(dir->i_mode))
    {
       NWFSPrint("nwfs: inode is NULL or not a directory in mknod\n");
       iput(dir);
       return -EBADF;
    }

    if (!hash)
    {
       iput(dir);
       return -ENOENT;
    }

    // if there is a name collision, then report that the file exists
    ino = get_directory_number(volume, name, namelen, dir->i_ino);
    if (ino)
    {
       // we found a matching entry
       iput(dir);
       return -EEXIST;
    }

    retCode = NWCreateDirectoryEntry(volume, (BYTE *)name, namelen,
				   0, dir->i_ino,
				   NW_DEVICE_FILE,
				   current->fsuid,
				   ((dir->i_mode & S_ISGID)
				   ? dir->i_gid
				   : current->fsgid),
				   rdev, mode,
				   &DirNo,
				   (ULONG) -1, 0,
				   (ULONG) -1, 0,
				   0, 0);

    if (retCode)
    {
       iput(dir);
       return (nwfs_to_linux_error(retCode));
    }

    if (!(inode = iget(dir->i_sb, DirNo)) < 0)
    {
       NWFSPrint("nwfs:  iget failed in mknod\n");
       iput(dir);
       return -EBADF;
    }

#if (VFS_VERBOSE)
    NWFSPrint("nwfs:  mknod mode-%X rdev-%X inode-mode-%X\n",
	      (unsigned int)mode, (unsigned int)rdev,
	      (unsigned int)inode->i_mode);

    NWFSPrint("nwfs:  IFBLK-%X IFCHR-%X IFLNK-%X\n",
	      (unsigned int)S_IFBLK,(unsigned int)S_IFCHR,
	      (unsigned int)S_IFLNK);
#endif

    // bump directory link count and size
    dir->i_mtime = NWFSGetSystemTime();
    dir->i_nlink++;
    dir->i_size += sizeof(ROOT);
    dir->i_dirt = 1;

    iput(inode);
    iput(dir);
    return 0;
}

#endif
