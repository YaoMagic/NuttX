/****************************************************************************
 * fs/mount/fs_mount.c
 *
 *   Copyright (C) 2007-2009, 2011-2013, 2015, 2017-2019 Gregory Nutt. All
 *     rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/mount.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/fs/fs.h>

#include "inode/inode.h"
#include "driver/driver.h"

/* At least one filesystem must be defined, or this file will not compile.
 * It may be desire-able to make file systems dynamically registered at
 * some time in the future, but at present, this file needs to know about
 * every configured filesystem.
 */

#ifdef CONFIG_FS_READABLE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* In the canonical case, a file system is bound to a block driver.  However,
 * some less typical cases a block driver is not required.  Examples are
 * pseudo file systems (like BINFS or PROCFS) and MTD file systems (like
 * NXFFS).
 *
 * These file systems all require block drivers:
 */

#if defined(CONFIG_FS_FAT) || defined(CONFIG_FS_ROMFS) || \
    defined(CONFIG_FS_SMARTFS) || defined(CONFIG_FS_LITTLEFS)
#  define BDFS_SUPPORT 1
#endif

/* These file systems require MTD drivers */

#if defined(CONFIG_FS_SPIFFS) || defined(CONFIG_FS_LITTLEFS)
#  define MDFS_SUPPORT 1
#endif

/* These file systems do not require block or MTD drivers */

#if defined(CONFIG_FS_NXFFS) || defined(CONFIG_FS_BINFS) || \
    defined(CONFIG_FS_PROCFS) || defined(CONFIG_NFS) || \
    defined(CONFIG_FS_TMPFS) || defined(CONFIG_FS_USERFS) || \
    defined(CONFIG_FS_CROMFS) || defined(CONFIG_FS_UNIONFS)
#  define NODFS_SUPPORT
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fsmap_t
{
  FAR const char                      *fs_filesystemtype;
  FAR const struct mountpt_operations *fs_mops;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef BDFS_SUPPORT
/* File systems that require block drivers */

#ifdef CONFIG_FS_FAT
extern const struct mountpt_operations fat_operations;
#endif
#ifdef CONFIG_FS_ROMFS
extern const struct mountpt_operations romfs_operations;
#endif
#ifdef CONFIG_FS_SMARTFS
extern const struct mountpt_operations smartfs_operations;
#endif
#ifdef CONFIG_FS_LITTLEFS
extern const struct mountpt_operations littlefs_operations;
#endif

static const struct fsmap_t g_bdfsmap[] =
{
#ifdef CONFIG_FS_FAT
    { "vfat", &fat_operations },
#endif
#ifdef CONFIG_FS_ROMFS
    { "romfs", &romfs_operations },
#endif
#ifdef CONFIG_FS_SMARTFS
    { "smartfs", &smartfs_operations },
#endif
#ifdef CONFIG_FS_LITTLEFS
    { "littlefs", &littlefs_operations },
#endif
    { NULL,   NULL },
};
#endif /* BDFS_SUPPORT */

#ifdef MDFS_SUPPORT
/* File systems that require MTD drivers */

#ifdef CONFIG_FS_ROMFS
extern const struct mountpt_operations romfs_operations;
#endif
#ifdef CONFIG_FS_SPIFFS
extern const struct mountpt_operations spiffs_operations;
#endif
#ifdef CONFIG_FS_LITTLEFS
extern const struct mountpt_operations littlefs_operations;
#endif

static const struct fsmap_t g_mdfsmap[] =
{
#ifdef CONFIG_FS_ROMFS
    { "romfs", &romfs_operations },
#endif
#ifdef CONFIG_FS_SPIFFS
    { "spiffs", &spiffs_operations },
#endif
#ifdef CONFIG_FS_LITTLEFS
    { "littlefs", &littlefs_operations },
#endif
    { NULL,   NULL },
};
#endif /* MDFS_SUPPORT */

#ifdef NODFS_SUPPORT
/* File systems that require neither block nor MTD drivers */

#ifdef CONFIG_FS_NXFFS
extern const struct mountpt_operations nxffs_operations;
#endif
#ifdef CONFIG_FS_TMPFS
extern const struct mountpt_operations tmpfs_operations;
#endif
#ifdef CONFIG_NFS
extern const struct mountpt_operations nfs_operations;
#endif
#ifdef CONFIG_FS_BINFS
extern const struct mountpt_operations binfs_operations;
#endif
#ifdef CONFIG_FS_PROCFS
extern const struct mountpt_operations procfs_operations;
#endif
#ifdef CONFIG_FS_USERFS
extern const struct mountpt_operations userfs_operations;
#endif
#ifdef CONFIG_FS_HOSTFS
extern const struct mountpt_operations hostfs_operations;
#endif
#ifdef CONFIG_FS_CROMFS
extern const struct mountpt_operations cromfs_operations;
#endif
#ifdef CONFIG_FS_UNIONFS
extern const struct mountpt_operations unionfs_operations;
#endif

static const struct fsmap_t g_nonbdfsmap[] =
{
#ifdef CONFIG_FS_NXFFS
    { "nxffs", &nxffs_operations },
#endif
#ifdef CONFIG_FS_TMPFS
    { "tmpfs", &tmpfs_operations },
#endif
#ifdef CONFIG_NFS
    { "nfs", &nfs_operations },
#endif
#ifdef CONFIG_FS_BINFS
    { "binfs", &binfs_operations },
#endif
#ifdef CONFIG_FS_PROCFS
    { "procfs", &procfs_operations },
#endif
#ifdef CONFIG_FS_USERFS
    { "userfs", &userfs_operations },
#endif
#ifdef CONFIG_FS_HOSTFS
    { "hostfs", &hostfs_operations },
#endif
#ifdef CONFIG_FS_CROMFS
    { "cromfs", &cromfs_operations },
#endif
#ifdef CONFIG_FS_UNIONFS
    { "unionfs", &unionfs_operations },
#endif
    { NULL, NULL },
};
#endif /* NODFS_SUPPORT */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mount_findfs
 *
 * Description:
 *    find the specified filesystem
 *
 ****************************************************************************/

#if defined(BDFS_SUPPORT) || defined(MDFS_SUPPORT) || defined(NODFS_SUPPORT)
static FAR const struct mountpt_operations *
mount_findfs(FAR const struct fsmap_t *fstab, FAR const char *filesystemtype)
{
  FAR const struct fsmap_t *fsmap;

  for (fsmap = fstab; fsmap->fs_filesystemtype; fsmap++)
    {
      if (strcmp(filesystemtype, fsmap->fs_filesystemtype) == 0)
        {
          return fsmap->fs_mops;
        }
    }

  return NULL;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mount
 *
 * Description:
 *   mount() attaches the filesystem specified by the 'source' block device
 *   name into the root file system at the path specified by 'target.'
 *
 * Returned Value:
 *   Zero is returned on success; -1 is returned on an error and errno is
 *   set appropriately:
 *
 *   EACCES A component of a path was not searchable or mounting a read-only
 *      filesystem was attempted without giving the MS_RDONLY flag.
 *   EBUSY 'source' is already  mounted.
 *   EFAULT One of the pointer arguments points outside the user address
 *      space.
 *   EINVAL 'source' had an invalid superblock.
 *   ENODEV 'filesystemtype' not configured
 *   ENOENT A pathname was empty or had a nonexistent component.
 *   ENOMEM Could not allocate a memory to copy filenames or data into.
 *   ENOTBLK 'source' is not a block device
 *
 ****************************************************************************/

int mount(FAR const char *source, FAR const char *target,
          FAR const char *filesystemtype, unsigned long mountflags,
          FAR const void *data)
{
#if defined(BDFS_SUPPORT) || defined(MDFS_SUPPORT) || defined(NODFS_SUPPORT)
#if defined(BDFS_SUPPORT) || defined(MDFS_SUPPORT)
  FAR struct inode *drvr_inode = NULL;
#endif
  FAR struct inode *mountpt_inode;
  FAR const struct mountpt_operations *mops;
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  struct inode_search_s desc;
#endif
  void *fshandle;
  int errcode;
  int ret;

  /* Verify required pointer arguments */

  DEBUGASSERT(target && filesystemtype);

  /* Find the specified filesystem.  Try the block driver file systems first */

#ifdef BDFS_SUPPORT
  if (source != NULL &&
      (ret = find_blockdriver(source, mountflags, &drvr_inode)) >= 0)
    {
      /* Find the block based file system */

      mops = mount_findfs(g_bdfsmap, filesystemtype);
      if (mops == NULL)
        {
          ferr("ERROR: Failed to find block based file system %s\n",
               filesystemtype);

          errcode = ENODEV;
          goto errout_with_inode;
        }
    }
  else
#endif /* BDFS_SUPPORT */
#ifdef MDFS_SUPPORT
  if (source != NULL && (ret = find_mtddriver(source, &drvr_inode)) >= 0)
    {
      /* Find the MTD based file system */

      mops = mount_findfs(g_mdfsmap, filesystemtype);
      if (mops == NULL)
        {
          ferr("ERROR: Failed to find MTD based file system %s\n",
               filesystemtype);

          errcode = ENODEV;
          goto errout_with_inode;
        }
    }
  else
#endif /* MDFS_SUPPORT */
#ifdef NODFS_SUPPORT
  if ((mops = mount_findfs(g_nonbdfsmap, filesystemtype)) != NULL)
    {
    }
  else
#endif /* NODFS_SUPPORT */
    {
      ferr("ERROR: Failed to find block driver %s\n", source);

      errcode = ENOTBLK;
      goto errout;
    }

  inode_semtake();

#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  /* Check if the inode already exists */

  SETUP_SEARCH(&desc, target, false);

  ret = inode_find(&desc);
  if (ret >= 0)
    {
      /* Successfully found.  The reference count on the inode has been
       * incremented.
       */

      mountpt_inode = desc.node;
      DEBUGASSERT(mountpt_inode != NULL);

      /* But is it a directory node (i.e., not a driver or other special
       * node)?
       */

      if (INODE_IS_SPECIAL(mountpt_inode))
        {
          ferr("ERROR: target %s exists and is a special node\n", target);
          errcode = -ENOTDIR;
          inode_release(mountpt_inode);
          goto errout_with_semaphore;
        }
    }
  else
#endif

  /* Insert a dummy node -- we need to hold the inode semaphore
   * to do this because we will have a momentarily bad structure.
   * NOTE that the new inode will be created with an initial reference
   * count of zero.
   */

    {
      ret = inode_reserve(target, &mountpt_inode);
      if (ret < 0)
        {
          /* inode_reserve can fail for a couple of reasons, but the most
           * likely one is that the inode already exists. inode_reserve may
           * return:
           *
           *  -EINVAL - 'path' is invalid for this operation
           *  -EEXIST - An inode already exists at 'path'
           *  -ENOMEM - Failed to allocate in-memory resources for the
           *            operation
           */

          ferr("ERROR: Failed to reserve inode for target %s\n", target);
          errcode = -ret;
          goto errout_with_semaphore;
        }
    }

  /* Bind the block driver to an instance of the file system.  The file
   * system returns a reference to some opaque, fs-dependent structure
   * that encapsulates this binding.
   */

  if (mops->bind == NULL)
    {
      /* The filesystem does not support the bind operation ??? */

      ferr("ERROR: Filesystem does not support bind\n");
      errcode = EINVAL;
      goto errout_with_mountpt;
    }

  /* Increment reference count for the reference we pass to the file system */

#if defined(BDFS_SUPPORT) || defined(MDFS_SUPPORT)
#ifdef NODFS_SUPPORT
  if (drvr_inode != NULL)
#endif
    {
      drvr_inode->i_crefs++;
    }
#endif

  /* On failure, the bind method returns -errorcode */

#if defined(BDFS_SUPPORT) || defined(MDFS_SUPPORT)
  ret = mops->bind(drvr_inode, data, &fshandle);
#else
  ret = mops->bind(NULL, data, &fshandle);
#endif
  if (ret < 0)
    {
      /* The inode is unhappy with the driver for some reason.  Back out
       * the count for the reference we failed to pass and exit with an
       * error.
       */

      ferr("ERROR: Bind method failed: %d\n", ret);

#if defined(BDFS_SUPPORT) || defined(MDFS_SUPPORT)
#ifdef NODFS_SUPPORT
      if (drvr_inode != NULL)
#endif
        {
          drvr_inode->i_crefs--;
        }
#endif

      errcode = -ret;
      goto errout_with_mountpt;
    }

  /* We have it, now populate it with driver specific information. */

  INODE_SET_MOUNTPT(mountpt_inode);

  mountpt_inode->u.i_mops  = mops;
#ifdef CONFIG_FILE_MODE
  mountpt_inode->i_mode    = mode;
#endif
  mountpt_inode->i_private = fshandle;
  inode_semgive();

  /* We can release our reference to the blkdrver_inode, if the filesystem
   * wants to retain the blockdriver inode (which it should), then it must
   * have called inode_addref().  There is one reference on mountpt_inode
   * that will persist until umount2() is called.
   */

#if defined(BDFS_SUPPORT) || defined(MDFS_SUPPORT)
#ifdef NODFS_SUPPORT
  if (drvr_inode != NULL)
#endif
    {
      inode_release(drvr_inode);
    }
#endif

#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  RELEASE_SEARCH(&desc);
#endif
  return OK;

  /* A lot of goto's!  But they make the error handling much simpler */

errout_with_mountpt:
  inode_remove(target);
  inode_release(mountpt_inode);

errout_with_semaphore:
  inode_semgive();
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  RELEASE_SEARCH(&desc);
#endif

#if defined(BDFS_SUPPORT) || defined(MDFS_SUPPORT)
errout_with_inode:

#ifdef NODFS_SUPPORT
  if (drvr_inode != NULL)
#endif
    {
      inode_release(drvr_inode);
    }
#endif

errout:
  set_errno(errcode);
  return ERROR;

#else
  ferr("ERROR: No filesystems enabled\n");
  set_errno(ENOSYS);
  return ERROR;
#endif /* BDFS_SUPPORT || MDFS_SUPPORT || NODFS_SUPPORT */
}

#endif /* CONFIG_FS_READABLE */
