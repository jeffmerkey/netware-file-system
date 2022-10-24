
/***************************************************************************
*
*   Copyright (c) 1998, 1999 Jeff V. Merkey
*   895 West Center Street
*   Orem, Utah  84057
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
*   FILE     :  SYMLINK.C
*   DESCRIP  :   Symbolic Link Code for Linux
*   DATE     :  June 24, 1999
*
*   This code was adapted from the Linux UMSDOS File system for nwfs.
*
***************************************************************************/

#include "globals.h"

#if (LINUX_24)

struct inode_operations nwfs_symlink_inode_operations =
{
    readlink:	    page_readlink,
    follow_link:    page_follow_link,
    setattr:        nwfs_notify_change,
};

static int nwfs_symlink_readpage(struct file *file, struct page *page)
{
    register int length, ccode;
    register struct inode *inode = (struct inode *)page->mapping->host;
    BYTE *buf = (BYTE *) kmap(page);
    loff_t loffs = 0;
    extern int nwfs_read_file_link(struct inode *, BYTE *, int, loff_t *);

    length = inode->i_size;
    ccode = nwfs_read_file_link(inode, buf, length, &loffs);
    if (ccode != length)
    {
       SetPageError(page);
       kunmap(page);
       UnlockPage(page);
       return -EIO;
    }

    buf[length] = '\0';
    SetPageUptodate(page);
    kunmap(page);
    UnlockPage(page);

    return 0;
}

struct address_space_operations nwfs_symlink_aops =
{
    readpage:	nwfs_symlink_readpage,
};

#endif

#if (LINUX_22)

int nwfs_readlink(struct dentry *dentry, char *buffer, int bufsiz)
{
    size_t size = dentry->d_inode->i_size;
    loff_t loffs = 0;
    ssize_t ret;
    struct file filp;

    NWFSSet(&filp, 0, sizeof(struct file));
    filp.f_reada = 1;
    filp.f_dentry = dentry;
    filp.f_reada = 0;
    filp.f_flags = O_RDONLY;
    filp.f_op = &nwfs_symlink_operations;

    if (size > bufsiz)
       size = bufsiz;

    ret = nwfs_file_read_kernel(&filp, buffer, size, &loffs);
    if (ret != size)
    {
       ret = -EIO;
    }
    return ret;
}

struct dentry *nwfs_follow_link(struct dentry *dentry, struct dentry *base,
			       unsigned int follow)
{
    struct inode *inode = dentry->d_inode;
    char *symname;
    int len, cnt;
    mm_segment_t old_fs = get_fs ();

    len = inode->i_size;
    if (!(symname = kmalloc(len + 1, GFP_KERNEL)))
    {
       dentry = ERR_PTR (-ENOMEM);
       dput(base);
       return dentry;
    }

    set_fs (KERNEL_DS);
    cnt = nwfs_readlink(dentry, symname, len);
    set_fs (old_fs);

    if (len != cnt)
    {
       dentry = ERR_PTR(-EIO);
       kfree(symname);
       dput(base);
       return dentry;
    }
    symname[len] = 0;
    dentry = lookup_dentry(symname, base, follow);
    kfree (symname);

    return dentry;
}

struct file_operations nwfs_symlink_operations =
{
    NULL,   // lseek - default
    NULL,   // read
    NULL,   // write
    NULL,   // readdir - bad
    NULL,   // select - default
    NULL,   // ioctl - default
    NULL,   // mmap
    NULL,   // no special open is needed
    NULL,   // release
    NULL    // fsync
};

struct inode_operations nwfs_symlink_inode_operations =
{
    &nwfs_symlink_operations,	// default file operations
    NULL,			// create
    NULL,			// lookup
    NULL,			// link
    NULL,			// unlink
    NULL,			// symlink
    NULL,			// mkdir
    NULL,			// rmdir
    NULL,			// mknod
    NULL,			// rename
    nwfs_readlink,	        // readlink
    nwfs_follow_link,	        // follow_link
    NULL,			// readpage
    NULL,			// writepage
    NULL,			// bmap
    NULL,			// truncate
    NULL,			// permission
    NULL,                       // smap
    NULL,                       // update page
    NULL,                       // revalidate
};

#endif

#if (LINUX_20)

int nwfs_readlink_common(struct inode *inode, char *buffer, int bufsiz,
			 ULONG as)
{
    int ret = inode->i_size;
    struct file filp;

    filp.f_pos = 0;
    filp.f_reada = 0;

    if (ret > bufsiz)
       ret = bufsiz;

    switch (as)
    {
       case USER_ADDRESS_SPACE:
	  if (nwfs_file_read(inode, &filp, buffer, ret) != ret)
	     ret = -EIO;
	  break;

       case KERNEL_ADDRESS_SPACE:
	  if (nwfs_file_read_kernel(inode, &filp, buffer, ret) != ret)
	     ret = -EIO;
	  break;

       default:
	  return -EINVAL;
    }
    return ret;
}

int nwfs_follow_link(struct inode *dir, struct inode *inode,
		     int flag, int mode, struct inode **res_inode)
{
    int ret = -ELOOP;
    *res_inode = NULL;

#if (VERBOSE)
    NWFSPrint("nwfs:  followlink\n");
#endif

    if (current->link_count < 5)
    {
       char *path = (char *) kmalloc(PATH_MAX, GFP_KERNEL);
       if (path == NULL)
       {
	  ret = -ENOMEM;
       }
       else
       {
	  if (!dir)
	  {
	     dir = current->fs[1].root;
	     dir->i_count++;
	  }
	  if (!inode)
	  {
	     ret = -ENOENT;
	  }
	  else
	  if (!S_ISLNK(inode->i_mode))
	  {
	     *res_inode = inode;
	     inode = NULL;
	     ret = 0;
	  }
	  else
	  {
	     ret = nwfs_readlink_common(inode, path, (PATH_MAX - 1),
					KERNEL_ADDRESS_SPACE);
	     if (ret > 0)
	     {
		path[ret] = '\0';
		iput(inode);
		inode = NULL;
		current->link_count++;

		ret = open_namei(path, flag, mode, res_inode, dir);

		current->link_count--;
		dir = NULL;
	     }
	     else
	     {
		ret = -EIO;
	     }
	  }
	  kfree (path);
       }
    }
    iput(inode);
    iput(dir);
    return ret;

}

int nwfs_readlink(struct inode *inode, char *buffer, int buflen)
{
    int ret = -EINVAL;

#if (VERBOSE)
    NWFSPrint("nwfs:  readlink\n");
#endif

    if (S_ISLNK(inode->i_mode))
    {
       ret =  nwfs_readlink_common(inode, buffer, buflen,
				   USER_ADDRESS_SPACE);
    }
    iput(inode);
    return ret;
}

static struct file_operations nwfs_symlink_operations =
{
    NULL,   // lseek - default
    NULL,   // read
    NULL,   // write
    NULL,   // readdir - bad
    NULL,   // select - default
    NULL,   // ioctl - default
    NULL,   // mmap
    NULL,   // no special open is needed
    NULL,   // release
    NULL    // fsync
};

struct inode_operations nwfs_symlink_inode_operations =
{
    &nwfs_symlink_operations,	// default file operations
    NULL,			// create
    NULL,			// lookup
    NULL,			// link
    NULL,			// unlink
    NULL,			// symlink
    NULL,			// mkdir
    NULL,			// rmdir
    NULL,			// mknod
    NULL,			// rename
    nwfs_readlink,	        // readlink
    nwfs_follow_link,	        // follow_link
    NULL,			// readpage
    NULL,			// writepage
    NULL,			// bmap
    NULL,			// truncate
    NULL			// permission
};

#endif




