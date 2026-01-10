/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_FS_COMPAT_H
#define _LINUX_FS_COMPAT_H

#include <linux/types.h>

struct vfsmount;
struct dentry;
struct inode;
struct file;
struct iattr;

int vfs_fchown(struct file *file, uid_t user, gid_t group);
int vfs_fchmod(struct file *file, umode_t mode);
int stream_open(struct inode *inode, struct file *filp);

int notify_change2(struct vfsmount *mnt, struct dentry *dentry,
                   struct iattr *attr, struct inode **delegated_inode);

int inode_permission2(struct vfsmount *mnt, struct inode *inode, int mask);

int vfs_mkobj2(struct vfsmount *mnt, struct dentry *dentry, umode_t mode,
               int (*f)(struct dentry *, umode_t, void *), void *arg);

int vfs_rmdir2(struct vfsmount *mnt, struct inode *dir, struct dentry *dentry);
int vfs_symlink2(struct vfsmount *mnt, struct inode *dir, struct dentry *dentry,
                 const char *oldname);
int vfs_mknod2(struct vfsmount *mnt, struct inode *dir, struct dentry *dentry,
               umode_t mode, dev_t dev);
int vfs_mkdir2(struct vfsmount *mnt, struct inode *dir, struct dentry *dentry,
               umode_t mode);
int vfs_unlink2(struct vfsmount *mnt, struct inode *dir, struct dentry *dentry,
                struct inode **delegated_inode);

#endif /* _LINUX_FS_COMPAT_H */
