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

#include "debug.h"
#include "qemu_drv_types.h"
#include "qemu_drv_ioctl.h"
#include "qemu_drv_regs.h"

#define MAX_FV_LEVEL 6

MODULE_AUTHOR     ("Marius Gligor and Nicolas Fournel");
MODULE_DESCRIPTION("QEMU driver");
MODULE_LICENSE    ("GPL"); 
MODULE_VERSION    ("0.0.1");

#define QEMU_DRV_MODULE_NAME     "qemu_drv"
#define QEMU_DRV_VERSION         0x000001

static qemu_drv_driver_t qemu_drv_drv;

static int     qemu_drv_chr_open    (struct inode *, struct file *);
static long    qemu_drv_chr_ioctl   (struct file *, unsigned int, unsigned long);
static int     qemu_drv_chr_release (struct inode *, struct file *);

static ssize_t qemu_drv_chr_read    (struct file *, char __user *, size_t, loff_t *);
static ssize_t qemu_drv_chr_write   (struct file *, const char __user *, size_t, loff_t *);
static int     qemu_drv_chr_mmap    (struct file *, struct vm_area_struct *);

static struct file_operations qemu_drv_chr_fops =
{
    .owner        = THIS_MODULE,
    .read         = qemu_drv_chr_read,
    .write        = qemu_drv_chr_write,
    .unlocked_ioctl = qemu_drv_chr_ioctl,
    .mmap         = qemu_drv_chr_mmap,
    .open         = qemu_drv_chr_open,
    .release      = qemu_drv_chr_release,
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

static int
qemu_drv_chr_ioctl_reset (qemu_drv_device_t *dev)
{
    return 0;
}

static int
qemu_drv_chr_ioctl_set_register_32 (qemu_drv_device_t *dev, void __user *user_buf, unsigned long reg_ofs)
{
    int                         ret = 0;
    unsigned long               val;

    COPY_FROM_USER ((void *) &val, user_buf, sizeof (unsigned long));
    writel (val, dev->base_addr + reg_ofs);

    return ret;
}

static int
qemu_drv_chr_ioctl_set_cpux_fv (qemu_drv_device_t *dev, void __user *user_buf)
{
    int                         ret = 0;
    qemu_ioc_set_cpux_fv_t      param;
    uint32_t                    val;

    COPY_FROM_USER ((void *) &param, user_buf, sizeof (qemu_ioc_set_cpux_fv_t));
    val = param.cpu + (param.frequency_level << 8);
    writel (val, dev->base_addr + SET_SYSTEMC_CPUX_FV_LEVEL);

    return ret;
}

/*
static int
qemu_drv_chr_ioctl_set_register_8 (qemu_drv_device_t *dev, void *user_buf, unsigned long reg_ofs)
{
    int                         ret = 0;
    unsigned char               val;

    COPY_FROM_USER ((void *) &val, user_buf, sizeof (unsigned char));
    writeb (val, dev->base_addr + reg_ofs);

    return ret;
}
*/
static int
qemu_drv_chr_ioctl_get_register_32 (qemu_drv_device_t *dev, void __user *user_buf, unsigned long reg_ofs)
{
    int                         ret = 0;
    unsigned long               val;

    val = readl (dev->base_addr + reg_ofs);
    COPY_TO_USER (user_buf, (void *) &val, sizeof (unsigned long));

    return ret;
}
/*
static int
qemu_drv_chr_ioctl_get_register_8 (qemu_drv_device_t *dev, void *user_buf, unsigned long reg_ofs)
{
    int                         ret = 0;
    unsigned char               val;


    val = readb (dev->base_addr + reg_ofs);
    COPY_TO_USER (user_buf, (void *) &val, sizeof (unsigned char));

    return ret;
}
*/
static int
qemu_drv_chr_open (struct inode *inode, struct file *file)
{
    int                         index;
    const int                   major  = MAJOR (inode->i_rdev);
    const int                   minor  = MINOR (inode->i_rdev);
    qemu_drv_device_t           *device = NULL;

    if (major != qemu_drv_drv.major)
    {
        EMSG ("open() major number mismatch. drv(%d) != open(%d)\n", qemu_drv_drv.major, major);
        return -ENXIO;
    }

    if ((minor < qemu_drv_drv.minor) || minor > (qemu_drv_drv.minor + qemu_drv_drv.nb_dev))
    {
        EMSG ("open() cannot find device, minor %d not associated\n", minor);
        return -ENXIO;
    }

    index = minor - qemu_drv_drv.minor;
    DMSG ("open() major %d minor %d linked to device index %d\n", major, minor, index);

    device = &qemu_drv_drv.devs[index];

    if(device == NULL)
    {
        EMSG (" open() cannot find registered device\n");
        return -ENXIO;
    }

    file->private_data = device;
    return 0;
}

static int
qemu_drv_chr_ioctl_set_process_fv (unsigned long level)
{
/*    struct task_struct                  *p, *head;*/

    if (level <= 0 || level > MAX_FV_LEVEL)
        return -1;

//§§
/*    head = get_current ()->group_leader;*/
/*    head->process_fv_level = level;*/
/*    list_for_each_entry (p, &head->thread_group, thread_group)*/
/*    {*/
/*        if (p->thread_fv_level == 0)*/
/*            p->thread_fv_level = -1;*/
/*    }*/

/*    if (head->thread_fv_level == 0)*/
/*        head->thread_fv_level = -1;*/

    return 0;
}


static long
qemu_drv_chr_ioctl (struct file *file, unsigned int code, unsigned long buffer)
{
    int                         ret = 0;
    qemu_drv_device_t           *dev = NULL;
    void __user *buf = (void __user *)buffer;

    dev  = file->private_data;

    if (dev == NULL)
    {
        EMSG ("ioctl() on undefined device\n");
        return -EBADF;
    }

    switch (code)
    {
        case QEMU_DRV_IOCRESET:
            DMSG ("QEMU_DRV_IOCRESET\n");
            ret = qemu_drv_chr_ioctl_reset (dev);
            break;
        case QEMU_DRV_IOCS_SET_CPUS_FV:
            DMSG ("QEMU_DRV_IOCS_SET_CPUS_FV\n");
            ret = qemu_drv_chr_ioctl_set_register_32 (dev, buf, SET_SYSTEMC_ALL_FV_LEVEL);
            break;
        case QEMU_DRV_IOCS_SET_CPUX_FV:
            DMSG ("QEMU_DRV_IOCS_SET_CPUX_FV\n");
            ret = qemu_drv_chr_ioctl_set_cpux_fv (dev, buf);
            break;
		case QEMU_DRV_IOC_MEASURE_STA:
			DMSG("QEMU_DRV_IOCS_MEASURE_STA\n");
			/* 0xC0DE is a magic value */
			writel (0xC0DE, dev->base_addr + SET_MEASURE_START);
			break;
			
		case QEMU_DRV_IOC_MEASURE_STO:
			DMSG("QEMU_DRV_IOCS_MEASURE_STO\n");
			ret = qemu_drv_chr_ioctl_get_register_32 (dev, buf, GET_MEASURE_RES);
			break;

        case QEMU_DRV_IOC_TEST:
        {
            //§§
            /*
            struct task_struct              *crt = get_current ();
            printk (KERN_INFO "KERNEL CRT THREAD ADR = 0x%X, parent = 0x%X, thfvlevel=%d,procfvlevel=%d\n",
                    (unsigned int) crt, (unsigned int) crt->group_leader, crt->thread_fv_level, crt->process_fv_level);
            */
            ret = 0;
        }
            break;
        case QEMU_DRV_IOCS_PROCESS_FV:
            ret = qemu_drv_chr_ioctl_set_process_fv (buffer);
            break;
    default:
        EMSG ("ioctl(0x%x, 0x%lx) not implemented\n", code, buffer);
        ret = -ENOTTY;
    }

    return ret;
}

static int
qemu_drv_chr_release (struct inode *inode, struct file *file)
{
    DMSG ("release() major %d minor %d\n", MAJOR (inode->i_rdev), MINOR (inode->i_rdev));

    file->private_data = NULL;

    return 0;
}

static ssize_t
qemu_drv_chr_read (struct file *file, char __user *data, size_t size, loff_t *loff)
{
    DMSG ("read() not implemented!\n");
    return 0;
}

static ssize_t
qemu_drv_chr_write (struct file *file, const char __user *data, size_t size, loff_t *loff)
{
    DMSG ("write() not implemented!\n");
    return 0;
}

static int
qemu_drv_chr_mmap (struct file *file, struct vm_area_struct *vma)
{
    DMSG ("mmap() not implemented!\n");
    return 0;
}

static int
qemu_drv_cdev_setup (qemu_drv_device_t *dev, int index)
{
    int                           ret = 0;
    int                           devno = MKDEV (qemu_drv_drv.major, qemu_drv_drv.minor + index);

    cdev_init (&dev->cdev, &qemu_drv_chr_fops);
    dev->cdev.owner  = THIS_MODULE;
    dev->cdev.ops    = &qemu_drv_chr_fops;
    dev->minor       = MINOR (devno);
    ret              = cdev_add (&dev->cdev, devno, 1);

    if (ret)
    {
        DMSG ("error %d adding qemu_drv char device %d\n", ret, index);
        return ret;
    }

    DMSG ("registered cdev major %d minor %d, index %d\n", qemu_drv_drv.major, dev->minor, index);

    return ret;
}

static ssize_t
qemu_drv_proc_fv_read (struct file *file, char __user *data, size_t size, loff_t *loff)
{
    int             idx = (int) PDE (file->f_dentry->d_inode)->data;
    printk (KERN_INFO "%s not implemented for cpu %d\n", __FUNCTION__, idx);
    return 0;
}

static ssize_t
qemu_drv_proc_fv_write (struct file *file, const char __user *data, size_t size, loff_t *loff)
{
    int             idx = (int) PDE (file->f_dentry->d_inode)->data;
    printk (KERN_INFO "%s not implemented for cpu %d\n", __FUNCTION__, idx);
    return 0;
}

 // set of file operations for our proc file
static struct file_operations qemu_drv_proc_fv_ops =
{
    .owner   = THIS_MODULE,
    .write   = qemu_drv_proc_fv_write,
    .read    = qemu_drv_proc_fv_read
};

static int
qemu_drv_create_proc (qemu_drv_device_t *dev)
{
    struct proc_dir_entry           *entry, *dir, *subdir;
    int                             i;
    char                            sname[100];

    dir = proc_mkdir (QEMU_DRV_MODULE_NAME, NULL);
    if (dir == NULL)
        return -1;

    for (i = 0; i < nr_cpu_ids; i++)
    {
        sprintf (sname, "%d", i);
        subdir = proc_mkdir (sname, dir);
        if (dir == NULL)
            return -1;

        entry = create_proc_entry ("fv", 0, subdir);
        if (entry == NULL)
            return -1;
        entry->proc_fops = &qemu_drv_proc_fv_ops;
        entry->data = (void *) i;
    }
    entry = create_proc_entry ("fv", 0, dir);
    if (entry == NULL)
        return -1;
    entry->proc_fops = &qemu_drv_proc_fv_ops;
    entry->data = (void *) -1;

    return 0;
}

static int
qemu_drv_cdev_release (qemu_drv_device_t *dev)
{
    DMSG ("released cdev major %d minor %d\n", qemu_drv_drv.major, dev->minor);
    cdev_del (&dev->cdev);

    return 0;
}

static int __devinit
qemu_drv_dev_init (void)
{
    qemu_drv_device_t                   *qemu_drv_dev;
    int                                 ret, qemu_drv_idx;
    void __iomem                        *base_addr;

    DMSG ("Device init\n");

    qemu_drv_idx = qemu_drv_drv.nb_dev;
    qemu_drv_dev = &qemu_drv_drv.devs[qemu_drv_idx];
    qemu_drv_drv.nb_dev++;

    base_addr = __io_address (QEMU_ADDR_BASE);
    if (!base_addr)
        return -ENOMEM;
    else
        qemu_drv_dev->base_addr = base_addr;

    /* setup char dev here */
    ret = qemu_drv_cdev_setup (qemu_drv_dev, qemu_drv_idx);
    if (ret)
        return ret;

    ret = qemu_drv_create_proc (qemu_drv_dev);
    if (ret)
        return ret;

    return 0;
}

static int __devexit
qemu_drv_dev_exit (qemu_drv_device_t *qemu_drv_dev)
{
    qemu_drv_cdev_release (qemu_drv_dev);

    return 0;
}

static int __init
qemu_drv_drv_init (void)
{
    int                             ret;
    dev_t                           chrdev;

    MSG ("module version %d.%d.%d\n", (QEMU_DRV_VERSION >> 16), (QEMU_DRV_VERSION >> 8) & 0xff,
         (QEMU_DRV_VERSION) & 0xff);

    memset (&qemu_drv_drv, 0, sizeof (qemu_drv_driver_t));

    ret = alloc_chrdev_region (&chrdev, 0, QEMU_DRV_MAXDEV, QEMU_DRV_MODULE_NAME);
    if (ret == 0)
    {
        qemu_drv_drv.major = MAJOR (chrdev);
        qemu_drv_drv.minor = MINOR (chrdev);
        MSG ("char driver registered using major %d, first minor %d\n", qemu_drv_drv.major, qemu_drv_drv.minor);
    }
    else
    {
        MSG ("driver could not register chr major (%d)\n", ret);
        goto qemu_drv_init_err_chr_register;
    }

    qemu_drv_dev_init ();

    return 0;

qemu_drv_init_err_chr_register:
    unregister_chrdev_region (MKDEV (qemu_drv_drv.major, qemu_drv_drv.minor), QEMU_DRV_MAXDEV);
    return ret;
}

static void __exit
qemu_drv_drv_cleanup(void)
{
    int                     i;

    for (i = 0; i < qemu_drv_drv.nb_dev; i++)
        qemu_drv_dev_exit (&qemu_drv_drv.devs[i]);

    DMSG ("CHR unregister driver\n");
    unregister_chrdev_region (MKDEV (qemu_drv_drv.major, qemu_drv_drv.minor), QEMU_DRV_MAXDEV);
    MSG ("module cleanup done.\n");
}

module_init (qemu_drv_drv_init);
module_exit (qemu_drv_drv_cleanup);

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
