
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
*   or linux-kernel@vger.kernel.org.  New releases, patches, bug fixes, and
*   technical documentation can be found at www.kernel.org.  We will
*   periodically post new releases of this software to www.kernel.org
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
*   FILE     :  INODE.C
*   DESCRIP  :   VFS inode Module for Linux
*   DATE     :  November 16, 1998
*
*
***************************************************************************/

#include "globals.h"

//
//  NOTE:  nwfs will only preserve unix style permissions if the NFS
//  namespace has been installed on a netware volume.  Otherwise,
//  nwfs will either emulate fat based files systems if the DOS namespace
//  or LONG namespace if the default.  You can create device special files
//  with fenris, however, the device information will not be valid unless
//  on NFS namespace is installed on the volume.
//

#if (LINUX_24)

int nwfs_notify_change(struct dentry *dentry, struct iattr *attr)
{
    struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG retCode, DirNo, flags;
    DOS stdos;
    DOS *dos = &stdos;
    NFS stnfs;
    NFS *nfs = &stnfs;

#if (VERBOSE)
    NWFSPrint("notify change %s\n", dentry->d_name.name);
#endif

    if (inode == NULL)
    {
       NWFSPrint("nwfs: inode = NULL\n");
       return -EACCES;
    }

    if (!hash)
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  NULL inode hash in notify_change\n");
#endif
       hash = get_directory_record(volume, inode->i_ino);
       if (!hash)
       {
          NWFSPrint("nwfs:  inode number (%d) not found notify_change\n", 
		   (int)inode->i_ino);
          return -EACCES;
       }

       // save the hash pointer for this inode
       inode->u.generic_ip = hash;
    }

    if (hash->DirNo != inode->i_ino)
    {
       NWFSPrint("nwfs:  inode and parent hash are inconsistent\n");
       return -EACCES;
    }
    
    NWLockFileExclusive(hash);

    if (volume->NameSpaceDefault == UNIX_NAME_SPACE)
    {
       DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
		                         hash, UNIX_NAME_SPACE);
       if (DirNo != (ULONG) -1)
       {
          if (attr->ia_valid & ATTR_UID) 
	  {
             inode->i_uid = attr->ia_uid;
	     nfs->uid = inode->i_uid;
	  }

          if (attr->ia_valid & ATTR_GID)
	  {
             inode->i_gid = attr->ia_gid;
	     nfs->gid = inode->i_gid;
	  }
	    
          if (attr->ia_valid & ATTR_MODE)
	  {
             inode->i_mode = attr->ia_mode;
	     nfs->mode = inode->i_mode;
	  }

	  // save the redev info 
	  nfs->rdev = inode->i_rdev;

          retCode = WriteDirectoryRecord(volume, (DOS *)nfs, DirNo);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		      (int)inode->i_ino, (int)retCode);
	     NWUnlockFile(hash);
             return -EIO;
	  }
       }
    }

    retCode = ReadDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
		   (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return -EIO;
    }

    if (attr->ia_valid & ATTR_SIZE) 
    { 
       dos->FileSize = attr->ia_size;
       inode->i_size = attr->ia_size;
    }

#if (HASH_FAT_CHAINS)
    dos->FirstBlock = hash->FirstBlock;
#endif
    
    if (attr->ia_valid & ATTR_CTIME) 
    {
       if (hash->Parent == DELETED_DIRECTORY)
          dos->DeletedDateAndTime = NWFSSystemToNetwareTime(attr->ia_ctime);
       else
          dos->CreateDateAndTime =  NWFSSystemToNetwareTime(attr->ia_ctime);
       inode->i_ctime = attr->ia_ctime;
    }
    
    if (attr->ia_valid & ATTR_MTIME)
    { 
       dos->LastUpdatedDateAndTime =  NWFSSystemToNetwareTime(attr->ia_mtime);
       inode->i_mtime = attr->ia_mtime;
    }
    
    if (attr->ia_valid & ATTR_ATIME) 
    {
       dos->LastArchivedDateAndTime = NWFSSystemToNetwareTime(attr->ia_atime);
       inode->i_atime = attr->ia_atime;
    }

    if (attr->ia_valid & ATTR_ATTR_FLAG) 
    {
       flags = attr->ia_attr_flags;
       if (flags & ATTR_FLAG_SYNCRONOUS) 
	  inode->i_flags |= MS_SYNCHRONOUS;
       else 
	  inode->i_flags &= ~MS_SYNCHRONOUS;
       
       if (flags & ATTR_FLAG_NOATIME) 
	  inode->i_flags |= MS_NOATIME;
       else 
	  inode->i_flags &= ~MS_NOATIME;
       
       if (flags & ATTR_FLAG_APPEND) 
	  inode->i_flags |= S_APPEND;
       else 
	  inode->i_flags &= ~S_APPEND;
       
       if (flags & ATTR_FLAG_IMMUTABLE) 
	  inode->i_flags |= S_IMMUTABLE;
       else 
  	  inode->i_flags &= ~S_IMMUTABLE;
    }

    retCode = WriteDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		   (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return -EIO;
    }

    NWUnlockFile(hash);

    return 0;
}

void nwfs_put_inode(struct inode *inode)
{

#if (VERBOSE)
    NWFSPrint("put inode %X\n", (unsigned)(inode ? inode->i_ino : 0));
#endif

    if (!inode)
       return;

    if (!inode->u.generic_ip)
       return;

//    inode->u.generic_ip = 0;

    if (inode->i_count.counter == 1)
       inode->i_nlink = 0;

    return;
}

void nwfs_read_inode(struct inode *inode)
{
    register HASH *hash;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register ino_t ino = inode->i_ino;
    register ULONG retCode;
    register ULONG DirNo;
    DOS stdos;
    DOS *dos = &stdos;
    NFS stnfs;
    NFS *nfs = &stnfs;
    extern struct address_space_operations nwfs_symlink_aops;
    extern struct address_space_operations nwfs_aops;

#if (VERBOSE)
    NWFSPrint("read inode %X\n", (unsigned)(inode ? inode->i_ino : 0));
#endif

    if (!inode)
       return;

    inode->i_op = NULL;
    inode->i_mode = 0;
    inode->i_size = 0;
    inode->i_mtime = inode->i_atime = inode->i_ctime = 0;
    inode->i_blocks = 0;
    inode->i_blksize = 512;

    hash = get_directory_record(volume, inode->i_ino);
    if (!hash)
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  inode number (%d) not found in directory\n", (int)ino);
#endif
       return;
    }

    // save the hash pointer for this inode
    inode->u.generic_ip = hash;

    NWLockFile(hash);

    retCode = ReadDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) directory read error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return;
    }

    if (hash->Parent == DELETED_DIRECTORY) 
       inode->i_ctime = 
	   NWFSNetwareToSystemTime(dos->DeletedDateAndTime);
    else
       inode->i_ctime =
	   NWFSNetwareToSystemTime(dos->CreateDateAndTime);
    
    inode->i_mtime =
	   NWFSNetwareToSystemTime(dos->LastUpdatedDateAndTime);
    inode->i_atime =
	   NWFSNetwareToSystemTime(dos->LastArchivedDateAndTime);

    //
    // Root entry is inode number 0.  We use the Netware directory numbers
    // as the inode numbers and we fill in the numbers inside of this
    // function.  By default, if the volume does not host an NFS namespace
    // we default to an MSDOS behavior relative to unix-style file
    // permissions.  If the LONG namespace is present, we support unix
    // names, however, unix permissions are emulated as in the FAT
    // filesystem.  If the NFS namespace is present on the volume, then
    // we support unix permissions and device files on a NetWare volume.
    //
    // The code below determines which namespace is present as the default,
    // then assigns permissions accordingly.
    //

    // fill in the inode for the root of the volume if we get an inode
    // number of zero.

    if (!ino)
    {
       // if root, initialize the uid and gid volume fields
       volume->uid = current->fsuid;
       volume->gid = current->fsgid;
       inode->i_uid = current->fsuid;
       inode->i_gid = current->fsgid;

       if (volume->VolumeFlags & READ_ONLY_VOLUME)
	  inode->i_mode = S_IRUGO | S_IXUGO | S_IFDIR;
       else
	  inode->i_mode = S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;

       switch (volume->NameSpaceDefault)
       {
	  case UNIX_NAME_SPACE:
	     DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
			                      hash, UNIX_NAME_SPACE);
	     if (DirNo != (ULONG) -1)
	     {
		// if nfs namespace has been initialized
		if (nfs->mode)
		{
		   inode->i_uid = nfs->uid;
		   inode->i_gid = nfs->gid;
		   inode->i_mode = nfs->mode | S_IFDIR;
		   inode->i_rdev = nfs->rdev;
		   break;
		}
	     }
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;
	     break;

	  case DOS_NAME_SPACE:
	  case MAC_NAME_SPACE:
	  case LONG_NAME_SPACE:
	  case NT_NAME_SPACE:
	  default:
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;
	     break;
       }

       inode->i_op = &nwfs_dir_inode_operations;
       inode->i_fop = &nwfs_dir_operations;
       inode->i_nlink = 2 + hash->Blocks;
       inode->i_size = hash->Blocks * sizeof(ROOT);
       inode->i_blocks = hash->Blocks;

       NWUnlockFile(hash);
       return;
    }

    // we detected a directory
    if ((hash->Flags & SUBDIRECTORY_FILE) || (hash->Parent == (ULONG) -1))
    {
       switch (volume->NameSpaceDefault)
       {
	  case UNIX_NAME_SPACE:
	     DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
			                      hash, UNIX_NAME_SPACE);
	     if (DirNo != (ULONG) -1)
	     {
		// if nfs namespace has been initialized
		if (nfs->mode)
		{
		   inode->i_uid = nfs->uid;
		   inode->i_gid = nfs->gid;
		   inode->i_mode = nfs->mode | S_IFDIR;
		   inode->i_rdev = nfs->rdev;
		   break;
		}
	     }
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;
	     break;

	  case DOS_NAME_SPACE:
	  case MAC_NAME_SPACE:
	  case LONG_NAME_SPACE:
	  case NT_NAME_SPACE:
	  default:
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;
	     break;
       }
       if (S_ISLNK(inode->i_mode))
       {
	  inode->i_op = &nwfs_symlink_inode_operations;
	  inode->i_data.a_ops = &nwfs_symlink_aops;
       }
       else
       {
	  inode->i_op = &nwfs_dir_inode_operations;
	  inode->i_fop = &nwfs_dir_operations;
       }
       inode->i_nlink = 2 + hash->Blocks;  // dirs have 2 links + dirs
       inode->i_size = hash->Blocks * sizeof(ROOT);
       inode->i_blocks = hash->Blocks;
    }
    else
    {
       // we detected a file
       switch (volume->NameSpaceDefault)
       {
	  case UNIX_NAME_SPACE:
	     DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
			                      hash, UNIX_NAME_SPACE);
	     if (DirNo != (ULONG) -1)
	     {
		// if nfs namespace has been initialized
		if (nfs->mode)
		{
		   inode->i_uid = nfs->uid;
		   inode->i_gid = nfs->gid;
		   inode->i_mode = nfs->mode;
		   inode->i_rdev = nfs->rdev;

		   if (nfs->nlinks)
                      inode->i_nlink = nfs->nlinks;
                   else
                      inode->i_nlink = 1;
		   break;
		}
	     }
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFREG;
             inode->i_nlink = 1;
	     break;

	  case DOS_NAME_SPACE:
	  case MAC_NAME_SPACE:
	  case LONG_NAME_SPACE:
	  case NT_NAME_SPACE:
	  default:
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFREG;
             inode->i_nlink = 1;
	     break;
       }

       if (S_ISLNK(inode->i_mode))
       {
	  inode->i_op = &nwfs_symlink_inode_operations;
	  inode->i_mapping->a_ops = &nwfs_symlink_aops;
       }
       else
       {
          inode->i_op = &nwfs_file_inode_operations;
          inode->i_fop = &nwfs_file_operations;
          inode->i_mapping->a_ops = &nwfs_aops;
       }
       inode->i_size = dos->FileSize;
       inode->i_blocks = (dos->FileSize + 511) / 512;
    }

    NWUnlockFile(hash);
    return;

}

void nwfs_write_inode(struct inode *inode, int wait)
{
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG retCode, DirNo;
    DOS stdos;
    DOS *dos = &stdos;
    NFS stnfs;
    NFS *nfs = &stnfs;

#if (VERBOSE)
    NWFSPrint("write inode %X\n", (unsigned)(inode ? inode->i_ino : 0));
#endif

    if (inode == NULL)
    {
       NWFSPrint("nwfs: inode = NULL\n");
       return;
    }

    if (!hash)
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  NULL inode hash in write_inode\n");
#endif
       hash = get_directory_record(volume, inode->i_ino);
       if (!hash)
       {
          NWFSPrint("nwfs:  inode number (%d) not found write_inode\n", 
		   (int)inode->i_ino);
          return;
       }

       // save the hash pointer for this inode
       inode->u.generic_ip = hash;
    }

    if (hash->DirNo != inode->i_ino)
    {
       NWFSPrint("nwfs:  inode and parent hash are inconsistent\n");
       return;
    }
    
    NWLockFileExclusive(hash);

    if (volume->NameSpaceDefault == UNIX_NAME_SPACE)
    {
       DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
		                         hash, UNIX_NAME_SPACE);
       if (DirNo != (ULONG) -1)
       {
	  nfs->uid = inode->i_uid;
	  nfs->gid = inode->i_gid;
	  nfs->mode = inode->i_mode;
	  nfs->rdev = inode->i_rdev;

	  retCode = WriteDirectoryRecord(volume, (DOS *)nfs, DirNo);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		      (int)inode->i_ino, (int)retCode);
	     NWUnlockFile(hash);
	     return;
          }
       }
    }

    retCode = ReadDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
		   (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return;
    }

    dos->FileSize = inode->i_size;
#if (HASH_FAT_CHAINS)
    dos->FirstBlock = hash->FirstBlock;
#endif
    
    if (hash->Parent == DELETED_DIRECTORY)
       dos->DeletedDateAndTime =  NWFSSystemToNetwareTime(inode->i_ctime);
    else
       dos->CreateDateAndTime =  NWFSSystemToNetwareTime(inode->i_ctime);
    dos->LastUpdatedDateAndTime =  NWFSSystemToNetwareTime(inode->i_mtime);
    dos->LastArchivedDateAndTime =  NWFSSystemToNetwareTime(inode->i_atime);

    retCode = WriteDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		   (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return;
    }

    NWUnlockFile(hash);

    return;
}

void nwfs_delete_inode(struct inode *inode)
{
#if (VERBOSE)
    NWFSPrint("delete inode %X\n", (unsigned)(inode ? inode->i_ino : 0));
#endif

    clear_inode(inode);
    return;
}

#endif


#if (LINUX_22)

int nwfs_notify_change(struct dentry *dentry, struct iattr *attr)
{
    struct inode *inode = dentry->d_inode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG retCode, DirNo, flags;
    DOS stdos;
    DOS *dos = &stdos;
    NFS stnfs;
    NFS *nfs = &stnfs;

#if (VERBOSE)
    NWFSPrint("notify change %s\n", dentry->d_name.name);
#endif

    if (inode == NULL)
    {
       NWFSPrint("nwfs: inode = NULL\n");
       return -EACCES;
    }

    if (!hash)
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  NULL inode hash in notify_change\n");
#endif
       hash = get_directory_record(volume, inode->i_ino);
       if (!hash)
       {
          NWFSPrint("nwfs:  inode number (%d) not found notify_change\n", 
		   (int)inode->i_ino);
          return -EACCES;
       }

       // save the hash pointer for this inode
       inode->u.generic_ip = hash;
    }

    if (hash->DirNo != inode->i_ino)
    {
       NWFSPrint("nwfs:  inode and parent hash are inconsistent\n");
       return -EACCES;
    }
    
    NWLockFileExclusive(hash);

    if (volume->NameSpaceDefault == UNIX_NAME_SPACE)
    {
       DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
		                         hash, UNIX_NAME_SPACE);
       if (DirNo != (ULONG) -1)
       {
          if (attr->ia_valid & ATTR_UID) 
	  {
             inode->i_uid = attr->ia_uid;
	     nfs->uid = inode->i_uid;
	  }

          if (attr->ia_valid & ATTR_GID)
	  {
             inode->i_gid = attr->ia_gid;
	     nfs->gid = inode->i_gid;
	  }
	    
          if (attr->ia_valid & ATTR_MODE)
	  {
             inode->i_mode = attr->ia_mode;
	     nfs->mode = inode->i_mode;
	  }

	  // save the redev info 
	  nfs->rdev = inode->i_rdev;

          retCode = WriteDirectoryRecord(volume, (DOS *)nfs, DirNo);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		      (int)inode->i_ino, (int)retCode);
	     NWUnlockFile(hash);
             return -EIO;
	  }
       }
    }

    retCode = ReadDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
		   (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return -EIO;
    }

    if (attr->ia_valid & ATTR_SIZE) 
    { 
       dos->FileSize = attr->ia_size;
       inode->i_size = attr->ia_size;
    }

#if (HASH_FAT_CHAINS)
    dos->FirstBlock = hash->FirstBlock;
#endif
    
    if (attr->ia_valid & ATTR_CTIME) 
    {
       if (hash->Parent == DELETED_DIRECTORY)
          dos->DeletedDateAndTime = NWFSSystemToNetwareTime(attr->ia_ctime);
       else
          dos->CreateDateAndTime =  NWFSSystemToNetwareTime(attr->ia_ctime);
       inode->i_ctime = attr->ia_ctime;
    }
    
    if (attr->ia_valid & ATTR_MTIME)
    { 
       dos->LastUpdatedDateAndTime =  NWFSSystemToNetwareTime(attr->ia_mtime);
       inode->i_mtime = attr->ia_mtime;
    }
    
    if (attr->ia_valid & ATTR_ATIME) 
    {
       dos->LastArchivedDateAndTime = NWFSSystemToNetwareTime(attr->ia_atime);
       inode->i_atime = attr->ia_atime;
    }

    if (attr->ia_valid & ATTR_ATTR_FLAG) 
    {
       flags = attr->ia_attr_flags;
       if (flags & ATTR_FLAG_SYNCRONOUS) 
	  inode->i_flags |= MS_SYNCHRONOUS;
       else 
	  inode->i_flags &= ~MS_SYNCHRONOUS;
       
       if (flags & ATTR_FLAG_NOATIME) 
	  inode->i_flags |= MS_NOATIME;
       else 
	  inode->i_flags &= ~MS_NOATIME;
       
       if (flags & ATTR_FLAG_APPEND) 
	  inode->i_flags |= S_APPEND;
       else 
	  inode->i_flags &= ~S_APPEND;
       
       if (flags & ATTR_FLAG_IMMUTABLE) 
	  inode->i_flags |= S_IMMUTABLE;
       else 
  	  inode->i_flags &= ~S_IMMUTABLE;
    }

    retCode = WriteDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		   (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return -EIO;
    }

    NWUnlockFile(hash);

    return 0;
}

void nwfs_put_inode(struct inode *inode)
{

#if (VERBOSE)
    NWFSPrint("put inode %X\n", (unsigned)(inode ? inode->i_ino : 0));
#endif

    if (!inode)
       return;

    if (!inode->u.generic_ip)
       return;

//    inode->u.generic_ip = 0;

    if (inode->i_count == 1)
       inode->i_nlink = 0;

    return;
}

void nwfs_read_inode(struct inode *inode)
{
    register HASH *hash;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register ino_t ino = inode->i_ino;
    register ULONG retCode;
    register ULONG DirNo;
    DOS stdos;
    DOS *dos = &stdos;
    NFS stnfs;
    NFS *nfs = &stnfs;

#if (VERBOSE)
    NWFSPrint("read inode %X\n", (unsigned)(inode ? inode->i_ino : 0));
#endif

    if (!inode)
       return;

    inode->i_op = NULL;
    inode->i_mode = 0;
    inode->i_size = 0;
    inode->i_mtime = inode->i_atime = inode->i_ctime = 0;
    inode->i_blocks = 0;
    inode->i_blksize = 512;

    hash = get_directory_record(volume, inode->i_ino);
    if (!hash)
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  inode number (%d) not found in directory\n", (int)ino);
#endif
       return;
    }

    // save the hash pointer for this inode
    inode->u.generic_ip = hash;

    NWLockFile(hash);

    retCode = ReadDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) directory read error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return;
    }

    if (hash->Parent == DELETED_DIRECTORY) 
       inode->i_ctime = 
	   NWFSNetwareToSystemTime(dos->DeletedDateAndTime);
    else
       inode->i_ctime =
	   NWFSNetwareToSystemTime(dos->CreateDateAndTime);
    
    inode->i_mtime =
	   NWFSNetwareToSystemTime(dos->LastUpdatedDateAndTime);
    inode->i_atime =
	   NWFSNetwareToSystemTime(dos->LastArchivedDateAndTime);

    //
    // Root entry is inode number 0.  We use the Netware directory numbers
    // as the inode numbers and we fill in the numbers inside of this
    // function.  By default, if the volume does not host an NFS namespace
    // we default to an MSDOS behavior relative to unix-style file
    // permissions.  If the LONG namespace is present, we support unix
    // names, however, unix permissions are emulated as in the FAT
    // filesystem.  If the NFS namespace is present on the volume, then
    // we support unix permissions and device files on a NetWare volume.
    //
    // The code below determines which namespace is present as the default,
    // then assigns permissions accordingly.
    //

    // fill in the inode for the root of the volume if we get an inode
    // number of zero.

    if (!ino)
    {
       // if root, initialize the uid and gid volume fields
       volume->uid = current->fsuid;
       volume->gid = current->fsgid;
       inode->i_uid = current->fsuid;
       inode->i_gid = current->fsgid;

       if (volume->VolumeFlags & READ_ONLY_VOLUME)
          inode->i_mode = S_IRUGO | S_IXUGO | S_IFDIR;
       else
          inode->i_mode = S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;

       switch (volume->NameSpaceDefault)
       {
	  case UNIX_NAME_SPACE:
	     DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
			                      hash, UNIX_NAME_SPACE);
	     if (DirNo != (ULONG) -1)
	     {
		// if nfs namespace has been initialized
		if (nfs->mode)
		{
		   inode->i_uid = nfs->uid;
		   inode->i_gid = nfs->gid;
		   inode->i_mode = nfs->mode | S_IFDIR;
		   inode->i_rdev = nfs->rdev;
		   break;
		}
	     }
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;
	     break;

	  case DOS_NAME_SPACE:
	  case MAC_NAME_SPACE:
	  case LONG_NAME_SPACE:
	  case NT_NAME_SPACE:
	  default:
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;
	     break;
       }

       inode->i_op = &nwfs_dir_inode_operations;
       inode->i_nlink = 2 + hash->Blocks;
       inode->i_size = hash->Blocks * sizeof(ROOT);
       inode->i_blocks = hash->Blocks;

       NWUnlockFile(hash);
       return;
    }

    // we detected a directory
    if ((hash->Flags & SUBDIRECTORY_FILE) || (hash->Parent == (ULONG) -1))
    {
       switch (volume->NameSpaceDefault)
       {
	  case UNIX_NAME_SPACE:
	     DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
			                      hash, UNIX_NAME_SPACE);
	     if (DirNo != (ULONG) -1)
	     {
		// if nfs namespace has been initialized
		if (nfs->mode)
		{
		   inode->i_uid = nfs->uid;
		   inode->i_gid = nfs->gid;
		   inode->i_mode = nfs->mode | S_IFDIR;
		   inode->i_rdev = nfs->rdev;
		   break;
		}
	     }
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;
	     break;

	  case DOS_NAME_SPACE:
	  case MAC_NAME_SPACE:
	  case LONG_NAME_SPACE:
	  case NT_NAME_SPACE:
	  default:
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;
	     break;
       }
       if (S_ISLNK(inode->i_mode))
	  inode->i_op = &nwfs_symlink_inode_operations;
       else
          inode->i_op = &nwfs_dir_inode_operations;

       inode->i_nlink = 2 + hash->Blocks;  // dirs have 2 links + dirs
       inode->i_size = hash->Blocks * sizeof(ROOT);
       inode->i_blocks = hash->Blocks;
    }
    else
    {
       // we detected a file
       switch (volume->NameSpaceDefault)
       {
	  case UNIX_NAME_SPACE:
	     DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
			                      hash, UNIX_NAME_SPACE);
	     if (DirNo != (ULONG) -1)
	     {
		// if nfs namespace has been initialized
		if (nfs->mode)
		{
		   inode->i_uid = nfs->uid;
		   inode->i_gid = nfs->gid;
		   inode->i_mode = nfs->mode;
		   inode->i_rdev = nfs->rdev;

                   if (nfs->nlinks)
                      inode->i_nlink = nfs->nlinks;
                   else
                      inode->i_nlink = 1;
		   break;
		}
	     }
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFREG;
             inode->i_nlink = 1;
	     break;

	  case DOS_NAME_SPACE:
	  case MAC_NAME_SPACE:
	  case LONG_NAME_SPACE:
	  case NT_NAME_SPACE:
	  default:
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFREG;
             inode->i_nlink = 1;
	     break;
       }

       if (S_ISCHR(inode->i_mode))
	  inode->i_op = &chrdev_inode_operations;
       else
       if (S_ISBLK(inode->i_mode))
	  inode->i_op = &blkdev_inode_operations;
       else
       if (S_ISLNK(inode->i_mode))
	  inode->i_op = &nwfs_symlink_inode_operations;
       else
       if (S_ISFIFO(inode->i_mode))
	  init_fifo(inode);
       else
	  inode->i_op = &nwfs_file_inode_operations;
       inode->i_size = dos->FileSize;
       inode->i_blocks = (dos->FileSize + 511) / 512;
    }
    NWUnlockFile(hash);
    return;

}

void nwfs_write_inode(struct inode *inode)
{
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG retCode, DirNo;
    DOS stdos;
    DOS *dos = &stdos;
    NFS stnfs;
    NFS *nfs = &stnfs;

#if (VERBOSE)
    NWFSPrint("write inode %X\n", (unsigned)(inode ? inode->i_ino : 0));
#endif

    if (inode == NULL)
    {
       NWFSPrint("nwfs: inode = NULL\n");
       return;
    }

    if (!hash)
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  NULL inode hash in write_inode\n");
#endif
       hash = get_directory_record(volume, inode->i_ino);
       if (!hash)
       {
          NWFSPrint("nwfs:  inode number (%d) not found write_inode\n", 
		   (int)inode->i_ino);
          return;
       }

       // save the hash pointer for this inode
       inode->u.generic_ip = hash;
    }

    if (hash->DirNo != inode->i_ino)
    {
       NWFSPrint("nwfs:  inode and parent hash are inconsistent\n");
       return;
    }

    NWLockFileExclusive(hash);

    if (volume->NameSpaceDefault == UNIX_NAME_SPACE)
    {
       DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
		                        hash, UNIX_NAME_SPACE);
       if (DirNo != (ULONG) -1)
       {
	  nfs->uid = inode->i_uid;
	  nfs->gid = inode->i_gid;
	  nfs->mode = inode->i_mode;
	  nfs->rdev = inode->i_rdev;

	  retCode = WriteDirectoryRecord(volume, (DOS *)nfs, DirNo);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		      (int)inode->i_ino, (int)retCode);
	     NWUnlockFile(hash);
	     return;
          }
       }
    }

    retCode = ReadDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
		   (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return;
    }

    dos->FileSize = inode->i_size;
#if (HASH_FAT_CHAINS)
    dos->FirstBlock = hash->FirstBlock;
#endif

    if (hash->Parent == DELETED_DIRECTORY)
       dos->DeletedDateAndTime =  NWFSSystemToNetwareTime(inode->i_ctime);
    else
       dos->CreateDateAndTime =  NWFSSystemToNetwareTime(inode->i_ctime);
    dos->LastUpdatedDateAndTime =  NWFSSystemToNetwareTime(inode->i_mtime);
    dos->LastArchivedDateAndTime =  NWFSSystemToNetwareTime(inode->i_atime);

    retCode = WriteDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		   (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return;
    }

    NWUnlockFile(hash);
    return;
}

void nwfs_delete_inode(struct inode *inode)
{
#if (VERBOSE)
    NWFSPrint("delete inode %X\n", (unsigned)(inode ? inode->i_ino : 0));
#endif

    clear_inode(inode);
    return;
}

#endif


#if (LINUX_20)

void nwfs_put_inode(struct inode *inode)
{
    if (!inode)
       return;

    // file has not been deleted so just return
    if (inode->i_nlink)
       return;

    // if all links were removed at some point, we truncate the
    // file length to 0

    inode->i_size = 0;
    nwfs_truncate(inode);

    // clear the hash pointer
//    inode->u.generic_ip = 0;

    // now mark the inode as free
    clear_inode(inode);

    return;
}

void nwfs_read_inode(struct inode *inode)
{
    register HASH *hash;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register ino_t ino = inode->i_ino;
    register ULONG retCode;
    register ULONG DirNo;
    DOS stdos;
    DOS *dos = &stdos;
    NFS stnfs;
    NFS *nfs = &stnfs;
    inode->i_op = NULL;
    inode->i_mode = 0;
    inode->i_size = 0;
    inode->i_mtime = inode->i_atime = inode->i_ctime = 0;
    inode->i_blocks = 0;
    inode->i_blksize = 512;

    hash = get_directory_record(volume, inode->i_ino);
    if (!hash)
    {
       NWFSPrint("nwfs:  inode number (%d) not found in directory\n", (int)ino);
       return;
    }

    // save the hash pointer for this inode
    inode->u.generic_ip = hash;

    NWLockFile(hash);

    retCode = ReadDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) directory read error [%d]\n",
	      (int)inode->i_ino, (int)retCode);
       NWUnlockFile(hash);
       return;
    }

    if (hash->Parent == DELETED_DIRECTORY) 
       inode->i_ctime = 
	   NWFSNetwareToSystemTime(dos->DeletedDateAndTime);
    else
       inode->i_ctime =
	   NWFSNetwareToSystemTime(dos->CreateDateAndTime);
    
    inode->i_mtime =
	   NWFSNetwareToSystemTime(dos->LastUpdatedDateAndTime);
    inode->i_atime =
	   NWFSNetwareToSystemTime(dos->LastArchivedDateAndTime);

    //
    // Root entry is inode number 0.  We use the Netware directory numbers
    // as the inode numbers and we fill in the numbers inside of this
    // function.  By default, if the volume does not host an NFS namespace
    // we default to an MSDOS behavior relative to unix-style file
    // permissions.  If the LONG namespace is present, we support unix
    // names, however, unix permissions are emulated as in the FAT
    // filesystem.  If the NFS namespace is present on the volume, then
    // we support unix permissions and device files on a NetWare volume.
    //
    // The code below determines which namespace is present as the default,
    // then assigns permissions accordingly.
    //

    // fill in the inode for the root of the volume if we get an inode
    // number of zero.
    if (!ino)
    {
       // if root, initialize the uid and gid volume fields
       volume->uid = current->fsuid;
       volume->gid = current->fsgid;
       inode->i_uid = current->fsuid;
       inode->i_gid = current->fsgid;

       if (volume->VolumeFlags & READ_ONLY_VOLUME)
          inode->i_mode = S_IRUGO | S_IXUGO | S_IFDIR;
       else
          inode->i_mode = S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;

       inode->i_op = &nwfs_dir_inode_operations;
       inode->i_nlink = 2 + hash->Blocks;
       inode->i_size = hash->Blocks * sizeof(ROOT);
       inode->i_blocks = hash->Blocks;
       NWUnlockFile(hash);
       return;
    }

    // we detected a directory
    if ((hash->Flags & SUBDIRECTORY_FILE) || (hash->Parent == (ULONG) -1))
    {
       switch (volume->NameSpaceDefault)
       {
	  case UNIX_NAME_SPACE:
	     DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
			                      hash, UNIX_NAME_SPACE);
	     if (DirNo != (ULONG) -1)
	     {
		// if nfs namespace has been initialized
		if (nfs->mode)
		{
		   inode->i_uid = nfs->uid;
		   inode->i_gid = nfs->gid;
		   inode->i_mode = (nfs->mode | S_IFDIR);
		   inode->i_rdev = nfs->rdev;
		   break;
		}
	     }
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;
	     break;

	  case DOS_NAME_SPACE:
	  case MAC_NAME_SPACE:
	  case LONG_NAME_SPACE:
	  case NT_NAME_SPACE:
	  default:
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFDIR;
	     break;
       }

       if (S_ISLNK(inode->i_mode))
	  inode->i_op = &nwfs_symlink_inode_operations;
       else
	  inode->i_op = &nwfs_dir_inode_operations;

       inode->i_nlink = 2 + hash->Blocks;  // dirs have 2 links + dirs
       inode->i_size = hash->Blocks * sizeof(ROOT);
       inode->i_blocks = hash->Blocks;
    }
    else
    {
       // we detected a file
       switch (volume->NameSpaceDefault)
       {
	  case UNIX_NAME_SPACE:
	     DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
			                      hash, UNIX_NAME_SPACE);
	     if (DirNo != (ULONG) -1)
	     {
		// if nfs namespace has been initialized
		if (nfs->mode)
		{
		   inode->i_uid = nfs->uid;
		   inode->i_gid = nfs->gid;
		   inode->i_mode = nfs->mode;
		   inode->i_rdev = nfs->rdev;

                   if (nfs->nlinks)
                      inode->i_nlink = nfs->nlinks;
                   else
                      inode->i_nlink = 1;
		   break;
		}
	     }
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFREG;
             inode->i_nlink = 1;
	     break;

	  case DOS_NAME_SPACE:
	  case MAC_NAME_SPACE:
	  case LONG_NAME_SPACE:
	  case NT_NAME_SPACE:
	  default:
	     inode->i_uid = volume->uid;
	     inode->i_gid = volume->gid;
	     inode->i_mode |= S_IRUGO | S_IXUGO | S_IWUSR | S_IFREG;
             inode->i_nlink = 1;
	     break;
       }

       if (S_ISCHR(inode->i_mode))
	  inode->i_op = &chrdev_inode_operations;
       else
       if (S_ISBLK(inode->i_mode))
	  inode->i_op = &blkdev_inode_operations;
       else
       if (S_ISLNK(inode->i_mode))
	  inode->i_op = &nwfs_symlink_inode_operations;
       else if (S_ISFIFO(inode->i_mode))
	  init_fifo(inode);
       else
	  inode->i_op = &nwfs_file_inode_operations;

       inode->i_size = dos->FileSize;
       inode->i_blocks = ((dos->FileSize + 511) / 512);
    }
    NWUnlockFile(hash);
    return;

}

void nwfs_write_inode(struct inode *inode)
{
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = (HASH *) inode->u.generic_ip;
    register ULONG retCode, DirNo;
    DOS stdos;
    DOS *dos = &stdos;
    NFS stnfs;
    NFS *nfs = &stnfs;

    if (inode == NULL)
    {
       NWFSPrint("nwfs: inode = NULL\n");
       return;
    }

    if (!hash)
    {
#if (VERBOSE)
       NWFSPrint("nwfs:  NULL inode hash in write_inode\n");
#endif
       hash = get_directory_record(volume, inode->i_ino);
       if (!hash)
       {
          NWFSPrint("nwfs:  inode number (%d) not found write_inode\n", 
		   (int)inode->i_ino);
          inode->i_dirt = 0;
          return;
       }

       // save the hash pointer for this inode
       inode->u.generic_ip = hash;
    }

    if (hash->DirNo != inode->i_ino)
    {
       NWFSPrint("nwfs:  inode and parent hash are inconsistent\n");
       inode->i_dirt = 0;
       return;
    }

    NWLockFileExclusive(hash);

    // if there is an NFS namespace record for this entry, see if it
    // needs to be updated.

    if (volume->NameSpaceDefault == UNIX_NAME_SPACE)
    {
       DirNo = get_namespace_dir_record(volume, (DOS *)nfs, 
		                        hash, UNIX_NAME_SPACE);
       if (DirNo != (ULONG) -1)
       {
	  nfs->uid = inode->i_uid;
	  nfs->gid = inode->i_gid;
	  nfs->mode = inode->i_mode;
	  nfs->rdev = inode->i_rdev;

	  retCode = WriteDirectoryRecord(volume, (DOS *)nfs, DirNo);
	  if (retCode)
	  {
	     NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		      (int)inode->i_ino, (int)retCode);
	     inode->i_dirt = 0;
	     NWUnlockFile(hash);
	     return;
	  }
       }
    }

    retCode = ReadDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) file read error [%d]\n",
		   (int)inode->i_ino, (int)retCode);
       inode->i_dirt = 0;
       NWUnlockFile(hash);
       return;
    }

    dos->FileSize = inode->i_size;
#if (HASH_FAT_CHAINS)
    dos->FirstBlock = hash->FirstBlock;
#endif

    if (hash->Parent == DELETED_DIRECTORY)
       dos->DeletedDateAndTime =  NWFSSystemToNetwareTime(inode->i_ctime);
    else
       dos->CreateDateAndTime =  NWFSSystemToNetwareTime(inode->i_ctime);
    dos->LastUpdatedDateAndTime =  NWFSSystemToNetwareTime(inode->i_mtime);
    dos->LastArchivedDateAndTime =  NWFSSystemToNetwareTime(inode->i_atime);

    retCode = WriteDirectoryRecord(volume, dos, inode->i_ino);
    if (retCode)
    {
       NWFSPrint("nwfs:  inode number (%d) directory write error [%d]\n",
		   (int)inode->i_ino, (int)retCode);
       inode->i_dirt = 0;
       NWUnlockFile(hash);
       return;
    }

    NWUnlockFile(hash);
    inode->i_dirt = 0;
    return;
}

#endif
