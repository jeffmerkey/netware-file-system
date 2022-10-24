
/***************************************************************************
*
*   Copyright (c) 1998, 2022 Jeff V. Merkey
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
*   AUTHOR   :  Jeff V. Merkey (jeffmerkey@gmail.com)
*   FILE     :  NWVFS.H
*   DESCRIP  :   Linux Virtual File System Definitions
*   DATE     :  November 1, 1998
*
*
***************************************************************************/

#ifndef _NWFS_VFS_
#define _NWFS_VFS_

#define _REPORT_VOLUMES    25
#define _LINUX_FS_ID       0x777
#define LINUX_512_SHIFT          9
#define LINUX_1024_SHIFT         10
#define LINUX_2048_SHIFT         11
#define LINUX_4096_SHIFT         12

extern struct  vm_operations_struct  nwfs_file_mmap;
extern struct  file_operations       nwfs_file_operations;
extern struct  inode_operations      nwfs_file_inode_operations;
extern struct  file_operations       nwfs_dir_operations;
extern struct  inode_operations      nwfs_dir_inode_operations;
extern struct  file_system_type      nwfs_type;
extern struct  super_operations      nwfs_sops;
extern struct  file_operations       nwfs_symlink_operations;
extern struct  inode_operations      nwfs_symlink_inode_operations;

extern struct super_block *nwfs_read_super(struct super_block *sb, void *data, int silent);
extern void nwfs_put_super(struct super_block *sb);
extern void nwfs_put_inode(struct inode *inode);
extern void nwfs_read_inode(struct inode *inode);
extern int nwfs_remount(struct super_block *sb, int *flags, char *data);

extern ULONG nwfs_file_mmap_nopage(struct vm_area_struct *area, ULONG address, int no_share);
extern int nwfs_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
extern void nwfs_truncate(struct inode *inode);

#if (LINUX_24)
extern void nwfs_write_inode(struct inode *inode, int wait);
extern int nwfs_notify_change(struct dentry *dentry, struct iattr *attr);
extern int nwfs_statfs(struct super_block *sb, struct statfs *buf);
extern void nwfs_delete_inode(struct inode *inode);
extern int nwfs_dir_ioctl(struct inode * inode, struct file * filp,
			  unsigned int cmd, unsigned long arg);
extern int nwfs_dir_readdir(struct file *filp, void *dirent, filldir_t filldir);
extern ssize_t nwfs_dir_read(struct file *filp, char *buf, size_t count, loff_t *ppos);
extern int nwfs_file_read(struct file *file, char *buf, size_t count, loff_t *ppos);
extern int nwfs_file_read_kernel(struct file *file, char *buf, size_t count, loff_t *ppos);
extern int nwfs_file_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
extern int nwfs_fsync(struct file *file, struct dentry *dentry);
extern int nwfs_mmap(struct file *file, struct vm_area_struct *vma);
extern int nwfs_symlink(struct inode *inode, struct dentry *dentry, const char *path);
extern int nwfs_link(struct dentry *olddentry, struct inode *dir, struct dentry *dentry);
extern int nwfs_create(struct inode *inode, struct dentry *dentry, int mode);
extern int nwfs_unlink(struct inode *inode, struct dentry *dentry);
extern int nwfs_mkdir(struct inode *inode, struct dentry *dentry, int mode);
extern int nwfs_rmdir(struct inode *inode, struct dentry *dentry);
extern int nwfs_mknod(struct inode *inode, struct dentry *dentry, int mode, int rdev);
extern int nwfs_rename(struct inode *oldNode, struct dentry *old_dentry,
			 struct inode *newNode, struct dentry *new_dentry);
extern struct dentry *nwfs_dir_lookup(struct inode *dir, struct dentry *dentry);
extern int nwfs_readlink(struct dentry *dentry, char *buffer, int bufsiz);
extern struct dentry *nwfs_follow_link(struct dentry *dentry, struct dentry *base,
			               unsigned int follow);
#endif

#if (LINUX_22)
extern void nwfs_write_inode(struct inode *inode);
extern int nwfs_notify_change(struct dentry *dentry, struct iattr *attr);
extern int nwfs_statfs(struct super_block *sb, struct statfs *buf, int bufsiz);
extern void nwfs_delete_inode(struct inode *inode);
extern int nwfs_dir_ioctl(struct inode * inode, struct file * filp,
			  unsigned int cmd, unsigned long arg);
extern int nwfs_dir_readdir(struct file *filp, void *dirent, filldir_t filldir);
extern ssize_t nwfs_dir_read(struct file *filp, char *buf, size_t count, loff_t *ppos);
extern int nwfs_file_read(struct file *file, char *buf, size_t count, loff_t *ppos);
extern int nwfs_file_read_kernel(struct file *file, char *buf, size_t count, loff_t *ppos);
extern int nwfs_file_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
extern int nwfs_fsync(struct file *file, struct dentry *dentry);
extern int nwfs_mmap(struct file *file, struct vm_area_struct *vma);
extern int nwfs_symlink(struct inode *inode, struct dentry *dentry, const char *path);
extern int nwfs_link(struct dentry *olddentry, struct inode *dir, struct dentry *dentry);
extern int nwfs_create(struct inode *inode, struct dentry *dentry, int mode);
extern int nwfs_unlink(struct inode *inode, struct dentry *dentry);
extern int nwfs_mkdir(struct inode *inode, struct dentry *dentry, int mode);
extern int nwfs_rmdir(struct inode *inode, struct dentry *dentry);
extern int nwfs_mknod(struct inode *inode, struct dentry *dentry, int mode, int rdev);
extern int nwfs_rename(struct inode *oldNode, struct dentry *old_dentry,
			 struct inode *newNode, struct dentry *new_dentry);
extern struct dentry *nwfs_dir_lookup(struct inode *dir, struct dentry *dentry);
extern int nwfs_readlink(struct dentry *dentry, char *buffer, int bufsiz);
extern struct dentry *nwfs_follow_link(struct dentry *dentry, struct dentry *base, unsigned int follow);

extern int nwfs_bmap(struct inode *inode, int block);

#endif

#if (LINUX_20)
extern void nwfs_write_inode(struct inode *inode);
extern int nwfs_notify_change(struct inode *inode, struct iattr *attr);
extern void nwfs_statfs(struct super_block *sb, struct statfs *buf, int bufsiz);
extern int nwfs_dir_readdir(struct inode *inode, struct file *filp, void *dirent, filldir_t filldir);
extern int nwfs_dir_read(struct inode *inode, struct file *filp, char *buf, int count);
extern int nwfs_file_read(struct inode *inode, struct file *file, char *buf, int count);
extern int nwfs_file_read_kernel(struct inode *inode, struct file *file, char *buf, int count);
extern int nwfs_file_write(struct inode *inode, struct file *file, const char *buf, int count);
extern int nwfs_fsync(struct inode *inode, struct file *file);
extern int nwfs_mmap(struct inode *inode, struct file *file, struct vm_area_struct *vma);
extern int nwfs_symlink(struct inode *inode, const char *name, int namelen, const char *path);
extern int nwfs_link(struct inode *oldinode, struct inode *dir, const char *name, int namelen);
extern int nwfs_create(struct inode *inode, const char *name, int namelen, int mode, struct inode **inode_result);
extern int nwfs_unlink(struct inode *inode, const char *name, int namelen);
extern int nwfs_mkdir(struct inode *inode, const char *name, int namelen, int mode);
extern int nwfs_rmdir(struct inode *inode, const char *name, int namelen);
extern int nwfs_mknod(struct inode *inode, const char *name, int namelen, int mode, int rdev);
extern int nwfs_rename(struct inode *oldNode, const char *oldName, int oldLen,
			 struct inode *newNode, const char *newName, int newLen,
			 int must_be_dir);
extern int nwfs_dir_lookup(struct inode *dir, const char *name, int len, struct inode **result);
extern int nwfs_readlink_common(struct inode *inode, char *buffer,
				int bufsiz, ULONG as);
extern int nwfs_follow_link(struct inode * dir, struct inode *inode,
			    int flag, int mode, struct inode **res_inode);
extern int nwfs_readlink(struct inode * inode, char * buffer, int buflen);

extern int nwfs_bmap(struct inode *inode, int block);

#endif


#endif
