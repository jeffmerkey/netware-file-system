
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
*   FILE     :  DIR.C
*   DESCRIP  :   VFS Dir Module for Linux
*   DATE     :  December 5, 1998
*
*
***************************************************************************/

#include "globals.h"

#if (LINUX_24)

int nwfs_dentry_validate(struct dentry *dentry, int flags)
{
    struct inode *inode = dentry->d_inode;

#if (VERBOSE)
    NWFSPrint("dentry validate\n");
#endif
    
    if (!inode)
       return 1;

    // see if this inode is the root entry
    if (inode->i_sb->s_root->d_inode == inode)
       return 1;

    // see if this inode is still valid.
    if (is_bad_inode(inode))
       return 0;

    if (atomic_read(&dentry->d_count) > 1)
       return 1;

    return 0;
}

int nwfs_delete_dentry(struct dentry * dentry)
{
#if (VERBOSE)
    NWFSPrint("dentry delete\n");
#endif

    if (!dentry->d_inode)
       return 0;

    if (is_bad_inode(dentry->d_inode))
       return 1;

    return 0;
}

struct dentry_operations nwfs_dentry_operations =
{
    d_revalidate:   nwfs_dentry_validate,
    d_delete:       nwfs_delete_dentry,
};

struct file_operations nwfs_dir_operations = {
    read:      generic_read_dir,
    readdir:   nwfs_dir_readdir,
    ioctl:     nwfs_dir_ioctl,
};

struct inode_operations nwfs_dir_inode_operations =
{
    create:    nwfs_create,
    lookup:    nwfs_dir_lookup,
//    link:      nwfs_link,    // does not work at present.  
    unlink:    nwfs_unlink,
    symlink:   nwfs_symlink,
    mkdir:     nwfs_mkdir,
    rmdir:     nwfs_rmdir,
    mknod:     nwfs_mknod,
    rename:    nwfs_rename,
    setattr:   nwfs_notify_change,
};

int nwfs_dir_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
    register ULONG i, count = 0;
    struct inode *inode = filp->f_dentry->d_inode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = 0;
    HASH *dirHash = 0;

    if (!inode || !S_ISDIR(inode->i_mode))
       return -ENOTDIR;

    i = filp->f_pos;
    switch (i)
    {
       case 0:
	  if (filldir(dirent, ".",  1, filp->f_pos, inode->i_ino, DT_DIR) < 0)
	     goto readdir_end;
	  filp->f_pos++;
          count++;
       case 1:
	  if (filldir(dirent, "..", 2, filp->f_pos,
                      filp->f_dentry->d_parent->d_inode->i_ino, DT_DIR) < 0)
	     goto readdir_end;
	  filp->f_pos++;
          count++;

       default:
          while (1)
          {
             hash = get_subdirectory_record(volume, (filp->f_pos - 2), 
	                                    inode->i_ino,
       				            &dirHash);
             if (!hash)
               break;

             // provide name and root name space (MSDOS) record DirNo
             if (filldir(dirent, hash->Name, hash->NameLength,
			 (hash->Root + 2), hash->Root, DT_UNKNOWN) < 0)

	        goto readdir_end;

             filp->f_pos = (hash->Root + 2);
             filp->f_pos++;
	     count++;
          }
          break;
    }

readdir_end:;

#if (VERBOSE)
    NWFSPrint("readdir count=%d pos1-%d pos2-%d\n", (int)count,
             (int)i, (int)filp->f_pos);
#endif

    return count;
}

int nwfs_dir_ioctl(struct inode * inode, struct file * filp,
		  unsigned int cmd, unsigned long arg)
{

#if (VERBOSE)
    NWFSPrint("ioctl cmd-%d\n", (int)cmd);
#endif

    switch (cmd)
    {
       default:
	  return -EINVAL;
    }
    return 0;
}

struct dentry *nwfs_dir_lookup(struct inode *dir, struct dentry *dentry)
{
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register ino_t ino;
    struct inode **result = &(dentry->d_inode);

#if (VERBOSE)
    NWFSPrint("lookup %s\n", dentry->d_name.name);
#endif

    *result = NULL;
    if (!dir)
       return ERR_PTR(-ENOENT);

    if (!S_ISDIR(dir->i_mode))
       return ERR_PTR(-ENOENT);

    if ((dentry->d_name.len == 0) || ((dentry->d_name.name[0] == '.') &&
	(dentry->d_name.len == 1)))
    {
       *result = dir;
       return 0;
    }

    ino = get_directory_number(volume,
			       &dentry->d_name.name[0],
			       dentry->d_name.len,
	                       dir->i_ino);
    if (ino)
    {
       *result = iget(dir->i_sb, ino);
       if (!(*result))
       {
          d_add(dentry, NULL); // assume create or mkdir being attempted
          dentry->d_time = 0;
          dentry->d_op = &nwfs_dentry_operations;
          return 0;
       }

       d_add(dentry, *result);
       dentry->d_time = 0;
       dentry->d_op = &nwfs_dentry_operations;
       return 0;
    }
    else
    {
       d_add(dentry, NULL); // assume create or mkdir being attempted
       dentry->d_time = 0;
       dentry->d_op = &nwfs_dentry_operations;
       return 0;
    }
}

#endif


#if (LINUX_22)

int nwfs_dentry_validate(struct dentry *dentry, int flags)
{
    struct inode *inode = dentry->d_inode;

#if (VERBOSE)
    NWFSPrint("dentry validate %s\n", dentry->d_name.name);
#endif
    
    if (!inode)
       return 1;

    // see if this inode is the root entry
    if (inode->i_sb->s_root->d_inode == inode)
       return 1;

    // see if this inode is still valid.
    if (is_bad_inode(inode))
       return 0;

    if (dentry->d_count > 1)
       return 1;

    return 0;
}

void nwfs_delete_dentry(struct dentry * dentry)
{

#if (VERBOSE)
    NWFSPrint("dentry delete %s\n", dentry->d_name.name);
#endif

    if (!dentry->d_inode)
       return;

    if (is_bad_inode(dentry->d_inode))
       d_drop(dentry);

    return;
}

struct dentry_operations nwfs_dentry_operations =
{
    nwfs_dentry_validate,	// d_revalidate(struct dentry *, int)
    NULL,		        // d_hash
    NULL,		   	// d_compare
    nwfs_delete_dentry,	        // d_delete(struct dentry *)
    NULL,
    NULL,
};

ssize_t nwfs_dir_read(struct file *filp, char *buf,
		      size_t count, loff_t *ppos)
{
    return -EISDIR;
}

int nwfs_dir_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
    register ULONG i, count = 0;
    struct inode *inode = filp->f_dentry->d_inode;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = 0;
    HASH *dirHash = 0;

    if (!inode || !S_ISDIR(inode->i_mode))
       return -ENOTDIR;

#if (VERBOSE)
    NWFSPrint("readdir called pos-%d ino-%d\n", (int)filp->f_pos, 
	      (int)inode->i_ino);
#endif

    i = filp->f_pos;
    switch (i)
    {
       case 0:
	  if (filldir(dirent, ".",  1, filp->f_pos, inode->i_ino) < 0)
	     goto readdir_end;
	  filp->f_pos++;
          count++;
       case 1:
	  if (filldir(dirent, "..", 2, filp->f_pos,
                      filp->f_dentry->d_parent->d_inode->i_ino) < 0)
	     goto readdir_end;
	  filp->f_pos++;
          count++;

       default:
          while (1)
          {
	     hash = get_subdirectory_record(volume, (filp->f_pos - 2), 
	                                    inode->i_ino,
       				            &dirHash);
             if (!hash)
               break;

	     // provide name and root name space (MSDOS) record DirNo
             if (filldir(dirent, hash->Name, hash->NameLength,
			 (hash->Root + 2), hash->Root) < 0)

	        goto readdir_end;

             filp->f_pos = (hash->Root + 2);
             filp->f_pos++;
             count++;
          }
          break;
    }

readdir_end:;

#if (VERBOSE)
    NWFSPrint("readdir count-%d pos1-%d pos2-%d ino-%d\n", (int)count,
             (int)i, (int)filp->f_pos, (int)inode->i_ino);
#endif

    return count;
}

int nwfs_dir_ioctl(struct inode * inode, struct file * filp,
		  unsigned int cmd, unsigned long arg)
{

#if (VERBOSE)
    NWFSPrint("ioctl cmd-%d\n", (int)cmd);
#endif

    switch (cmd)
    {
       default:
	  return -EINVAL;
    }
    return 0;
}

struct dentry *nwfs_dir_lookup(struct inode *dir, struct dentry *dentry)
{
    register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
    register ino_t ino;
    struct inode **result = &(dentry->d_inode);

#if (VERBOSE)
    NWFSPrint("lookup %s\n", dentry->d_name.name);
#endif

    *result = NULL;
    if (!dir)
       return ERR_PTR(-ENOENT);

    if (!S_ISDIR(dir->i_mode))
       return ERR_PTR(-ENOENT);

    if ((dentry->d_name.len == 0) || ((dentry->d_name.name[0] == '.') &&
	(dentry->d_name.len == 1)))
    {
       *result = dir;
       return 0;
    }

    ino = get_directory_number(volume,
			       &dentry->d_name.name[0],
			       dentry->d_name.len,
	                       dir->i_ino);
    if (ino)
    {
#if (VERBOSE)
       NWFSPrint("lookup %s (found)\n", dentry->d_name.name);
#endif
       
       *result = iget(dir->i_sb, ino);
       if (!(*result))
       {
          d_add(dentry, NULL); // assume create or mkdir being attempted
          dentry->d_time = 0;
          dentry->d_op = &nwfs_dentry_operations;
          return 0;
       }
       d_add(dentry, *result);
       dentry->d_time = 0;
       dentry->d_op = &nwfs_dentry_operations;
       return 0;
    }
    else
    {
#if (VERBOSE)
       NWFSPrint("lookup %s (negative)\n", dentry->d_name.name);
#endif

       d_add(dentry, NULL); // assume create or mkdir being attempted
       dentry->d_time = 0;
       dentry->d_op = &nwfs_dentry_operations;
       return 0;
    }
}

struct file_operations nwfs_dir_operations = {
    NULL,			// lseek
    nwfs_dir_read,		// read
    NULL,			// write
    nwfs_dir_readdir,		// readdir
    NULL,			// select v2.0.x/poll v2.1.x - default
    nwfs_dir_ioctl,		// ioctl
    NULL,			// mmap
    NULL,			// open
    NULL,			// flush
    NULL,			// release
    NULL,		        // fsync
    NULL,
    NULL,
    NULL
};

struct inode_operations nwfs_dir_inode_operations =
{
    &nwfs_dir_operations,	// file operations
    nwfs_create,		// create
    nwfs_dir_lookup,		// lookup
//    nwfs_link,             	// link   // does not work at present
    NULL,			// link
    nwfs_unlink,		// unlink
    nwfs_symlink,	        // symlink
    nwfs_mkdir,			// mkdir
    nwfs_rmdir,			// rmdir
    nwfs_mknod,   		// mknod
    nwfs_rename,		// rename
    NULL,			// readlink
    NULL,			// follow_link
    NULL,			// read_page
    NULL,			// writepage
    NULL,			// bmap
    NULL,			// truncate
    NULL,			// permission
    NULL,			// smap
    NULL,			// update page
    NULL, 		        // revalidate inode
};

#endif


#if (LINUX_20)

int nwfs_dir_read(struct inode *inode, struct file *filp,
		    char *buf, int count)
{
    return -EISDIR;
}

int nwfs_dir_readdir(struct inode *inode, struct file *filp,
		       void *dirent, filldir_t filldir)
{
    register ULONG i, count = 0;
    register VOLUME *volume = (VOLUME *) inode->i_sb->u.generic_sbp;
    register HASH *hash = 0;
    HASH *dirHash = 0;

    if (!inode || !S_ISDIR(inode->i_mode))
       return -ENOTDIR;

    i = filp->f_pos;
    switch (i)
    {
       case 0:
	  if (filldir(dirent, ".",  1, filp->f_pos, inode->i_ino) < 0)
	     goto readdir_end;
	  filp->f_pos++;
          count++;

       case 1:
	  if (filldir(dirent, "..", 2, filp->f_pos,
                      get_parent_directory_number(volume, inode->i_ino)) < 0)
	     goto readdir_end;
	  filp->f_pos++;
          count++;

       default:
          while (1)
          {
             hash = get_subdirectory_record(volume, (filp->f_pos - 2), 
	                               inode->i_ino,
       				       &dirHash);
             if (!hash)
                break;

             // provide name and root name space (MSDOS) record DirNo
             if (filldir(dirent, hash->Name, hash->NameLength,
			 (hash->Root + 2), hash->Root) < 0)

	        goto readdir_end;

             filp->f_pos = (hash->Root + 2);
             filp->f_pos++;
             count++;
          }
          break;
    }

readdir_end:;

    return count;
}

int nwfs_dir_lookup(struct inode *dir, const char *name, int len,
		      struct inode **result)
{
	register VOLUME *volume = (VOLUME *) dir->i_sb->u.generic_sbp;
	register ino_t ino;
	register struct super_block *sb;

	*result = 0;
	if (!dir)
	{
	   return -ENOENT;
	}

	if (!S_ISDIR(dir->i_mode))
	{
	   iput(dir);
	   return -ENOENT;
	}

	if ((len == 0) || ((name[0] == '.') && (len == 1)))
	{
	   *result = dir;
	   return 0;
	}

	if ((name[0] == '.') &&( name[1] == '.') && (len == 2))
	{
	   sb = dir->i_sb;
	   ino = get_parent_directory_number(volume, dir->i_ino);
	   iput(dir);

	   *result = iget(sb, ino);
	   if (!*result)
	      return -EACCES;
	   return 0;
	}

	ino = get_directory_number(volume, name, len, dir->i_ino);
	if (!ino)
	{
	   iput(dir);
	   return -ENOENT;
	}

	*result = iget(dir->i_sb, ino);
	if (!*result)
	{
	   iput(dir);
	   return -EACCES;
	}
	iput(dir);
	return 0;

}

struct file_operations nwfs_dir_operations =
{
	NULL,			// lseek
	nwfs_dir_read,	// read
	NULL,			// write
	nwfs_dir_readdir,	// readdir
	NULL,			// select
	NULL,			// ioctl
	NULL,			// mmap
	NULL,			// open
	NULL,			// release
	NULL,			// fsync
	NULL,			// fasync
	NULL,			// check_media_change
	NULL			// revalidate
};

struct inode_operations nwfs_dir_inode_operations =
{
	&nwfs_dir_operations,	// file operations
	nwfs_create,		// create
	nwfs_dir_lookup,	// lookup
//	nwfs_link,             	// link   // does not work st present
        NULL,			// link
	nwfs_unlink,		// unlink
	nwfs_symlink,	        // symlink
	nwfs_mkdir,		// mkdir
	nwfs_rmdir,		// rmdir
	nwfs_mknod,   		// mknod
	nwfs_rename,		// rename
        NULL,			// readlink
	NULL,			// follow_link
	NULL,			// read_page
	NULL,			// writepage
	NULL,			// bmap
	NULL,			// truncate
	NULL,			// permission
	NULL			// smap
};

#endif
