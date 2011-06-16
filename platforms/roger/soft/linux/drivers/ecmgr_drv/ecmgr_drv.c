/*
 *  Copyright (c) 2010 TIMA Laboratory
 *
 *  This file is part of Rabbits.
 *
 *  Rabbits is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Rabbits is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Rabbits.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>

#include <linux/io.h>
#include <mach/hardware.h>
#include <asm/page.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/page-flags.h>

//#define DEBUG
//#define HDEBUG

#include "debug.h"
#include "ecmgr_drv_types.h"
#include "ecmgr_drv_ioctl.h"
#include "ecmgr_drv_regs.h"

#define MAX_FV_LEVEL 6

MODULE_AUTHOR     ("Marius Gligor");
MODULE_DESCRIPTION("ECMGR driver");
MODULE_LICENSE    ("GPL"); 
MODULE_VERSION    ("0.0.1");

#define ECMGR_DRV_MODULE_NAME       "ecmgr_drv"
#define ECMGR_DRV_VERSION           0x000001

static ecmgr_drv_driver_t ecmgr_drv;

static int     ecmgr_drv_chr_open    (struct inode *, struct file *);
static int     ecmgr_drv_chr_ioctl   (struct inode *, struct file *, unsigned int, unsigned long);
static int     ecmgr_drv_chr_release (struct inode *, struct file *);

static ssize_t ecmgr_drv_chr_read    (struct file *, char __user *, size_t, loff_t *);
static ssize_t ecmgr_drv_chr_write   (struct file *, const char __user *, size_t, loff_t *);
static int     ecmgr_drv_chr_mmap    (struct file *, struct vm_area_struct *);

static struct file_operations ecmgr_drv_chr_fops =
{
    .owner        = THIS_MODULE,
    .read         = ecmgr_drv_chr_read,
    .write        = ecmgr_drv_chr_write,
    .ioctl        = ecmgr_drv_chr_ioctl,
    .mmap         = ecmgr_drv_chr_mmap,
    .open         = ecmgr_drv_chr_open,
    .release      = ecmgr_drv_chr_release,
};

#define COPY_FROM_USER(dst,src,size)                                   \
do {                                                                   \
  int cpy;                                                             \
  cpy = copy_from_user (dst,src,size);                                 \
  if (cpy < 0)                                                         \
    {                                                                  \
      EMSG("ioctl() invalid arguments in copy_from_user\n");           \
      return cpy;                                                      \
    }                                                                  \
} while(0)

#define COPY_TO_USER(dst,src,size)                                     \
do {                                                                   \
  int cpy;                                                             \
  cpy = copy_to_user(dst,src,size);                                    \
  if (cpy < 0)                                                         \
    {                                                                  \
      EMSG("ioctl() invalid arguments in copy_to_user\n");             \
      return cpy;                                                      \
    }                                                                  \
} while (0)

#if 0
static int
ecmgr_drv_chr_ioctl_set_register_32 (ecmgr_drv_device_t *dev, void *user_buf, unsigned long reg_ofs)
{
    int                         ret = 0;
    unsigned long               val;

    COPY_FROM_USER ((void *) &val, user_buf, sizeof (unsigned long));
    writel (val, dev->base_addr + reg_ofs);

    return ret;
}

static int
ecmgr_drv_chr_ioctl_get_register_32 (ecmgr_drv_device_t *dev, void *user_buf, unsigned long reg_ofs)
{
    int                         ret = 0;
    unsigned long               val;

    val = readl (dev->base_addr + reg_ofs);
    COPY_TO_USER (user_buf, (void *) &val, sizeof (unsigned long));

    return ret;
}
#endif

static int
ecmgr_drv_chr_ioctl_reset (ecmgr_drv_device_t *dev)
{
    return 0;
}

static int
ecmgr_drv_chr_ioctl_send_use (ecmgr_drv_device_t *dev, unsigned long use100)
{
    DMSG ("use100=%lu\n", use100);
    writel (dev->addr_core_thread_ec_cpus, dev->base_addr + ECMGR_REG_DATA);
    writel (use100, dev->base_addr + ECMGR_REG_RESERVED);
    writel (DNA_IPI_DISPATCH_COMMAND, dev->base_addr + ECMGR_REG_COMMAND);

    return 0;
}

static int
ecmgr_drv_chr_open (struct inode *inode, struct file *file)
{
    int                         index;
    const int                   major  = MAJOR (inode->i_rdev);
    const int                   minor  = MINOR (inode->i_rdev);
    ecmgr_drv_device_t          *device = NULL;

    if (major != ecmgr_drv.major)
    {
        EMSG ("open() major number mismatch. drv(%d) != open(%d)\n", ecmgr_drv.major, major);
        return -ENXIO;
    }

    if ((minor < ecmgr_drv.minor) || minor > (ecmgr_drv.minor + ecmgr_drv.nb_dev))
    {
        EMSG ("open() cannot find device, minor %d not associated\n", minor);
        return -ENXIO;
    }

    index = minor - ecmgr_drv.minor;
    DMSG ("open() major %d minor %d linked to device index %d\n", major, minor, index);

    device = &ecmgr_drv.devs[index];

    if (device == NULL)
    {
        EMSG (" open() cannot find registered device\n");
        return -ENXIO;
    }

    file->private_data = device;
    return 0;
}

static int
ecmgr_drv_chr_ioctl (struct inode *inode, struct file *file, 
    unsigned int code, unsigned long buffer)
{
    int                         ret = 0;
    ecmgr_drv_device_t          *dev = NULL;

    dev  = file->private_data;

    if (dev == NULL)
    {
        EMSG ("ioctl() on undefined device\n");
        return -EBADF;
    }

    switch (code)
    {
        case ECMGR_DRV_IOCRESET:
            DMSG ("ECMGR_DRV_IOCRESET\n");
            ret = ecmgr_drv_chr_ioctl_reset (dev);
            break;
        case ECMGR_DRV_IOCS_SEND_USE:
            ret = ecmgr_drv_chr_ioctl_send_use (dev, buffer);
            break;
    default:
        EMSG ("ioctl(0x%x, 0x%lx) not implemented\n", code, buffer);
        ret = -ENOTTY;
    }

    return ret;
}

static int
ecmgr_drv_chr_release (struct inode *inode, struct file *file)
{
    DMSG ("release() major %d minor %d\n", MAJOR (inode->i_rdev), MINOR (inode->i_rdev));

    file->private_data = 0;

    return 0;
}

static ssize_t
ecmgr_drv_chr_read (struct file *file, char __user *data, size_t size, loff_t *loff)
{
    DMSG ("read() not implemented!\n");
    return 0;
}

static ssize_t
ecmgr_drv_chr_write (struct file *file, const char __user *data, size_t size, loff_t *loff)
{
    DMSG ("write() not implemented!\n");
    return 0;
}

static int
ecmgr_drv_chr_mmap (struct file *file, struct vm_area_struct *vma)
{
    DMSG ("mmap() not implemented!\n");
    return 0;
}

static int
ecmgr_drv_cdev_setup (ecmgr_drv_device_t *dev, int index)
{
    int                 ret = 0;
    int                 devno = MKDEV (ecmgr_drv.major, ecmgr_drv.minor + index);

    cdev_init (&dev->cdev, &ecmgr_drv_chr_fops);
    dev->cdev.owner  = THIS_MODULE;
    dev->cdev.ops    = &ecmgr_drv_chr_fops;
    dev->minor       = MINOR (devno);
    ret              = cdev_add (&dev->cdev, devno, 1);

    if (ret)
    {
        DMSG ("error %d adding ecmgr_drv char device %d\n", ret, index);
        return ret;
    }

    DMSG ("registered cdev major %d minor %d, index %d\n", ecmgr_drv.major, dev->minor, index);

    return ret;
}

static int
ecmgr_drv_cdev_release (ecmgr_drv_device_t *dev)
{
    DMSG ("released cdev major %d minor %d\n", ecmgr_drv.major, dev->minor);
    cdev_del (&dev->cdev);

    return 0;
}

static int __devinit
ecmgr_drv_dev_init (void)
{
    ecmgr_drv_device_t                  *ecmgr_drv_dev;
    int                                 ret, ecmgr_drv_idx;
    void __iomem                        *base_addr;

    MSG ("Device init\n");

    ecmgr_drv_idx = ecmgr_drv.nb_dev;
    ecmgr_drv_dev = &ecmgr_drv.devs[ecmgr_drv_idx];
    ecmgr_drv.nb_dev++;

    base_addr = ioremap (ECMGR_ADDR_BASE, 0x10000);
    if (!base_addr)
        return -ENOMEM;
    else
    {
        ecmgr_drv_dev->base_addr = (unsigned long) base_addr;
        ecmgr_drv_dev->addr_core_thread_ec_cpus = readl (ecmgr_drv_dev->base_addr + ECMGR_REG_DNA_THREAD);
        MSG ("dna_core_thred address = %lu\n", ecmgr_drv_dev->addr_core_thread_ec_cpus);
    }

    /* setup char dev here */
    ret = ecmgr_drv_cdev_setup (ecmgr_drv_dev, ecmgr_drv_idx);
    if (ret)
        return ret;

    return 0;
}

static int __devexit
ecmgr_drv_dev_exit (ecmgr_drv_device_t *ecmgr_drv_dev)
{
    ecmgr_drv_cdev_release (ecmgr_drv_dev);

    return 0;
}

static int __init
ecmgr_drv_init (void)
{
    int                             ret;
    dev_t                           chrdev;

    MSG ("module version %d.%d.%d\n", (ECMGR_DRV_VERSION >> 16), (ECMGR_DRV_VERSION >> 8) & 0xff,
         (ECMGR_DRV_VERSION) & 0xff);

    memset (&ecmgr_drv, 0, sizeof (ecmgr_drv_driver_t));

    ret = alloc_chrdev_region (&chrdev, 0, ECMGR_DRV_MAXDEV, ECMGR_DRV_MODULE_NAME);
    if (ret == 0)
    {
        ecmgr_drv.major = MAJOR (chrdev);
        ecmgr_drv.minor = MINOR (chrdev);
        MSG ("char driver registered using major %d, first minor %d\n", ecmgr_drv.major, ecmgr_drv.minor);
    }
    else
    {
        MSG ("driver could not register chr major (%d)\n", ret);
        goto ecmgr_drv_init_err_chr_register;
    }

    ecmgr_drv_dev_init ();

    return 0;

ecmgr_drv_init_err_chr_register:
    unregister_chrdev_region (MKDEV (ecmgr_drv.major, ecmgr_drv.minor), ECMGR_DRV_MAXDEV);
    return ret;
}

static void __exit
ecmgr_drv_cleanup (void)
{
    int                     i;

    for (i = 0; i < ecmgr_drv.nb_dev; i++)
        ecmgr_drv_dev_exit (&ecmgr_drv.devs[i]);

    DMSG ("CHR unregister driver\n");
    unregister_chrdev_region (MKDEV (ecmgr_drv.major, ecmgr_drv.minor), ECMGR_DRV_MAXDEV);
    MSG ("module cleanup done.\n");
}

module_init (ecmgr_drv_init);
module_exit (ecmgr_drv_cleanup);

/*
 * Vim standard variables
 * vim:set ts=4 expandtab tw=80 cindent syntax=c:
 *
 * Emacs standard variables
 * Local Variables:
 * mode: c
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
