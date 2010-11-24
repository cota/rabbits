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

#include <linux/io.h>
#include <asm/page.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/page-flags.h>

#include "debug.h"
#include "h264dbf_types.h"
#include "h264dbf_ioctl.h"
#include "h264dbf_regs.h"
#include "h264dbf_int.h"

MODULE_AUTHOR       ("Nicolas Fournel and Marius Gligor");
MODULE_DESCRIPTION  ("H264 DBF driver");
MODULE_LICENSE      ("GPL"); 
MODULE_VERSION      ("0.0.1");

#define H264DBF_MODULE_NAME     "h264dbf"
#define H264DBF_VERSION         0x000001

static h264dbf_driver_t h264dbf_drv;

static int     h264dbf_chr_open    (struct inode *, struct file *);
static int     h264dbf_chr_ioctl   (struct inode *, struct file *, unsigned int, unsigned long);
static int     h264dbf_chr_release (struct inode *, struct file *);

static ssize_t h264dbf_chr_read    (struct file *, char __user *, size_t, loff_t *);
static ssize_t h264dbf_chr_write   (struct file *, const char __user *, size_t, loff_t *);
static int     h264dbf_chr_mmap    (struct file *, struct vm_area_struct *);

static struct file_operations h264dbf_chr_fops =
{
    .owner        = THIS_MODULE,
    .read         = h264dbf_chr_read,
    .write        = h264dbf_chr_write,
    .ioctl        = h264dbf_chr_ioctl,
    .mmap         = h264dbf_chr_mmap,
    .open         = h264dbf_chr_open,
    .release      = h264dbf_chr_release,
};

#define COPY_FROM_USER(dst,src,size)                                \
    do {                                                            \
        int cpy;                                                    \
        cpy = copy_from_user (dst,src,size);                        \
        if (cpy < 0)                                                \
        {                                                           \
            EMSG("ioctl() invalid arguments in copy_from_user\n");  \
            return cpy;                                             \
        }                                                           \
    } while(0)

#define COPY_TO_USER(dst,src,size)                                  \
    do {                                                            \
        int cpy;                                                    \
        cpy = copy_to_user(dst,src,size);                           \
        if (cpy < 0)                                                \
        {                                                           \
            EMSG("ioctl() invalid arguments in copy_to_user\n");    \
            return cpy;                                             \
        }                                                           \
    } while (0)

static int
h264dbf_chr_ioctl_reset(h264dbf_device_t *dev)
{
    return 0;
}

static int
h264dbf_chr_ioctl_irq_register(h264dbf_device_t *dev, void *buf)
{
    int                     ret = 0;
    h264dbf_ioc_irq_t        irq;

    COPY_FROM_USER ((void *) &irq, buf, sizeof(irq));
    ret = h264dbf_int_register (dev, &irq);

    return ret;
}

static int
h264dbf_chr_ioctl_irq_wait(h264dbf_device_t *dev, void *buf)
{
    int                     ret = 0;
    h264dbf_ioc_irq_t      irq;

    COPY_FROM_USER ((void *) &irq, buf, sizeof(irq));
    ret = h264dbf_int_wait (dev, &irq);

    return ret;
}

static int
h264dbf_chr_ioctl_irq_unregister(h264dbf_device_t *dev, void *buf)
{
    int                       ret = 0;
    h264dbf_ioc_irq_t          irq;

    COPY_FROM_USER ((void *) &irq, buf, sizeof(irq));
    ret = h264dbf_int_unregister(dev, &irq);

    return ret;
}

static int
h264dbf_chr_ioctl_set_register_32 (h264dbf_device_t *dev, void *user_buf, unsigned long reg_ofs)
{
    int                         ret = 0;
    unsigned long               val;

    COPY_FROM_USER ((void *) &val, user_buf, sizeof (unsigned long));
    writel (val, dev->base_addr + reg_ofs);

    return ret;
}

static int
h264dbf_chr_ioctl_set_register_8 (h264dbf_device_t *dev, void *user_buf, unsigned long reg_ofs)
{
    int                         ret = 0;
    unsigned char               val;

    COPY_FROM_USER ((void *) &val, user_buf, sizeof (unsigned char));
    writeb (val, dev->base_addr + reg_ofs);

    return ret;
}

static int
h264dbf_chr_ioctl_get_register_8 (h264dbf_device_t *dev, void *user_buf, unsigned long reg_ofs)
{
    int                         ret = 0;
    unsigned char               val;

    val = readb (dev->base_addr + reg_ofs);
    COPY_TO_USER (user_buf, (void *) &val, sizeof (unsigned char));

    return ret;
}

static int
h264dbf_chr_open (struct inode *inode, struct file *file)
{
    int                         index;
    const int                   major  = MAJOR (inode->i_rdev);
    const int                   minor  = MINOR (inode->i_rdev);
    h264dbf_device_t           *device = NULL;

    if (major != h264dbf_drv.major)
    {
        EMSG ("open() major number mismatch. drv(%d) != open(%d)\n", h264dbf_drv.major, major);
        return -ENXIO;
    }

    if ((minor < h264dbf_drv.minor) || minor > (h264dbf_drv.minor + h264dbf_drv.nb_dev))
    {
        EMSG ("open() cannot find device, minor %d not associated\n", minor);
        return -ENXIO;
    }

    index = minor - h264dbf_drv.minor;
    DMSG ("open() major %d minor %d linked to device index %d\n", major, minor, index);

    device = &h264dbf_drv.devs[index];

    if(device == NULL)
    {
        EMSG (" open() cannot find registered device\n");
        return -ENXIO;
    }

    file->private_data = device;
    return 0;
}

static int
h264dbf_chr_ioctl (struct inode *inode, struct file *file, unsigned int code, unsigned long buffer)
{
    int                         ret = 0;
    h264dbf_device_t            *dev = NULL;

    dev  = file->private_data;

    if (dev == NULL)
    {
        EMSG ("ioctl() on undefined device\n");
        return -EBADF;
    }

    switch (code)
    {
    case H264DBF_IOCRESET:
        DMSG ("H264DBF_IOCRESET\n");
        ret = h264dbf_chr_ioctl_reset (dev);
        break;
    case H264DBF_IOCSIRQREG:
        DMSG ("H264DBF_IOCSIRQREG\n");
        ret = h264dbf_chr_ioctl_irq_register (dev, (void *) buffer);
        break;
    case H264DBF_IOCSIRQWAIT:
        DMSG ("H264DBF_IOCSIRQWAIT\n");
        ret = h264dbf_chr_ioctl_irq_wait (dev, (void *) buffer);
        break;
    case H264DBF_IOCSIRQUNREG:
        DMSG ("H264DBF_IOCSIRQUNREG\n");
        ret = h264dbf_chr_ioctl_irq_unregister (dev, (void *) buffer);
        break;
    case H264DBF_IOCS_SET_TRANSFER_ADDR:
        DMSG ("H264DBF_IOCS_SET_TRANSFER_ADDR\n");
        ret = h264dbf_chr_ioctl_set_register_32 (dev, (void *) buffer, REG_SRAM_ADDRESS);
        break;
    case H264DBF_IOCS_SET_NSLICES_INTR:
        DMSG ("H264DBF_IOCS_SET_NSLICES_INTR\n");
        ret = h264dbf_chr_ioctl_set_register_8 (dev, (void *) buffer, REG_NSLICES_INTR);
        break;
    case H264DBF_IOCS_GET_HW_NSLICES:
        DMSG ("H264DBF_IOCS_GET_HW_NSLICES\n");
        ret = h264dbf_chr_ioctl_get_register_8 (dev, (void *) buffer, REG_NSLICES);
        break;
    default:
        EMSG ("ioctl(0x%x, 0x%lx) not implemented\n", code, buffer);
        ret = -ENOTTY;
    }

    return ret;
}

static int
h264dbf_chr_release (struct inode *inode, struct file *file)
{
    h264dbf_device_t   *dev = (h264dbf_device_t *)file->private_data;

    DMSG ("release() major %d minor %d\n", MAJOR (inode->i_rdev), MINOR (inode->i_rdev));

    h264dbf_int_cleanup (dev);

    file->private_data = 0;

    return 0;
}

static ssize_t
h264dbf_chr_read (struct file *file, char __user *data, size_t size, loff_t *loff)
{
    DMSG ("read() not implemented!\n");
    return 0;
}

static ssize_t
h264dbf_chr_write (struct file *file, const char __user *data, size_t size, loff_t *loff)
{
    DMSG ("write() not implemented!\n");
    return 0;
}

static int
h264dbf_chr_mmap(struct file *file, struct vm_area_struct *vma)
{
    DMSG ("mmap() not implemented!\n");
    return 0;
}

static int
h264dbf_cdev_setup (h264dbf_device_t *dev, int index)
{
    int                           ret = 0;
    int                           devno = MKDEV (h264dbf_drv.major, h264dbf_drv.minor + index);

    cdev_init (&dev->cdev, &h264dbf_chr_fops);
    dev->cdev.owner  = THIS_MODULE;
    dev->cdev.ops    = &h264dbf_chr_fops;
    dev->minor       = MINOR (devno);
    ret              = cdev_add (&dev->cdev, devno, 1);

    if (ret)
    {
        DMSG ("error %d adding h264dbf char device %d\n", ret, index);
        return ret;
    }

    DMSG ("registered cdev major %d minor %d, index %d\n", h264dbf_drv.major, dev->minor, index);

    return ret;
}

static int
h264dbf_cdev_release (h264dbf_device_t *dev)
{
    DMSG ("released cdev major %d minor %d\n", h264dbf_drv.major, dev->minor);
    cdev_del (&dev->cdev);

    return 0;
}

static int __devinit
h264dbf_dev_init (void)
{
    h264dbf_device_t                   *h264dbf_dev;
    int                                 h264dbf_idx;
    void __iomem                        *base_addr;

    DMSG ("Device init\n");

    h264dbf_idx = h264dbf_drv.nb_dev;
    h264dbf_dev = &h264dbf_drv.devs[h264dbf_idx];
    h264dbf_drv.nb_dev++;

    base_addr = ioremap ((unsigned long) H264DBF_BASE_ADDRESS, H264DBF_SIZE);
    if (!base_addr)
        return -ENOMEM;
    else
        h264dbf_dev->base_addr = (unsigned long) base_addr;

    DMSG ("Got a base_addr : 0x%08lx\n", (unsigned long) base_addr);

    h264dbf_int_init (h264dbf_dev);

    /* setup char dev here */
    h264dbf_cdev_setup (h264dbf_dev, h264dbf_idx);

    return 0;
}

static int __devexit
h264dbf_dev_exit(h264dbf_device_t *h264dbf_dev)
{
    h264dbf_cdev_release (h264dbf_dev);
    h264dbf_int_cleanup (h264dbf_dev);

    iounmap ((void *) h264dbf_dev->base_addr);

    return 0;
}

static int __init
h264dbf_drv_init (void)
{
    int                             ret;
    dev_t                           chrdev;

    MSG ("module version %d.%d.%d\n", (H264DBF_VERSION >> 16), (H264DBF_VERSION >> 8) & 0xff, (H264DBF_VERSION) & 0xff);

    memset (&h264dbf_drv, 0, sizeof (h264dbf_driver_t));

    ret = alloc_chrdev_region (&chrdev, 0, H264DBF_MAXDEV, H264DBF_MODULE_NAME);
    if (ret == 0)
    {
        h264dbf_drv.major = MAJOR (chrdev);
        h264dbf_drv.minor = MINOR (chrdev);
        MSG ("char driver registered using major %d, first minor %d\n", h264dbf_drv.major, h264dbf_drv.minor);
    }
    else
    {
        MSG ("driver could not register chr major (%d)\n", ret);
        goto h264dbf_init_err_chr_register;
    }

    h264dbf_dev_init ();

    return 0;

h264dbf_init_err_chr_register:
    unregister_chrdev_region (MKDEV (h264dbf_drv.major, h264dbf_drv.minor), H264DBF_MAXDEV);
    return ret;
}

static void __exit
h264dbf_drv_cleanup(void)
{
    int                     i;

    for (i = 0; i < h264dbf_drv.nb_dev; i++)
        h264dbf_dev_exit (&h264dbf_drv.devs[i]);

    DMSG ("CHR unregister driver\n");
    unregister_chrdev_region (MKDEV (h264dbf_drv.major, h264dbf_drv.minor), H264DBF_MAXDEV);
    MSG ("module cleanup done.\n");
}

module_init (h264dbf_drv_init);
module_exit (h264dbf_drv_cleanup);

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
