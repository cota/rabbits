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
#include <linux/slab.h>
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

#if 0
#define DEBUG
#define HDEBUG
#endif

#include "debug.h"
#include "rabbitsha_types.h"
#include "rabbitsha_ioctl.h"
#include "rabbitsha_regs.h"

MODULE_AUTHOR       ("Marius Gligor");
MODULE_DESCRIPTION  ("RABBITS host access driver");
MODULE_LICENSE      ("GPL"); 
MODULE_VERSION      ("0.0.1");

#define RABBITSHA_MODULE_NAME     "rabbitsha"
#define RABBITSHA_VERSION         0x000001

#define RABBITSHA_MAJOR           248
#define RABBITSHA_MINOR           0

static rabbitsha_driver_t rabbitsha_drv;

static int     rabbitsha_chr_open   (struct inode *, struct file *);
static long    rabbitsha_chr_ioctl  (struct file *, unsigned int, unsigned long);
static int     rabbitsha_chr_release(struct inode *, struct file *);

static ssize_t rabbitsha_chr_read   (struct file *,       char __user *, size_t, loff_t *);
static ssize_t rabbitsha_chr_write  (struct file *, const char __user *, size_t, loff_t *);
static loff_t rabbitsha_chr_llseek  (struct file *file, loff_t off, int whence);

static struct file_operations rabbitsha_chr_fops =
{
  .owner        = THIS_MODULE,
  .read         = rabbitsha_chr_read,
  .write        = rabbitsha_chr_write,
  .llseek       = rabbitsha_chr_llseek,
  .unlocked_ioctl = rabbitsha_chr_ioctl,
  .open         = rabbitsha_chr_open,
  .release      = rabbitsha_chr_release,
};

int
rabbitsha_stop(rabbitsha_device_t *dev)
{
    return 0;
}

/*
 * ioctl() Handling
 */
#define COPY_FROM_USER(dst,src,size)                                    \
do                                                                      \
{                                                                       \
    int                     cpy;                                        \
    cpy = copy_from_user(dst, src, size);                               \
    if(cpy < 0){                                                        \
        EMSG("ioctl() invalid arguments in copy_from_user\n");          \
        return cpy;                                                     \
    }                                                                   \
} while (0)

#define COPY_TO_USER(dst,src,size)                                      \
do                                                                      \
{                                                                       \
    int                     cpy;                                        \
    cpy = copy_to_user(dst, src, size);                                 \
    if(cpy < 0){                                                        \
        EMSG("ioctl() invalid arguments in copy_to_user\n");            \
        return cpy;                                                     \
    }                                                                   \
} while (0)

/*
 * Char dev Handling
 */
static int
rabbitsha_chr_open(struct inode *inode, struct file *file)
{
    const int           major  = MAJOR(inode->i_rdev);
    const int           minor  = MINOR(inode->i_rdev);
    rabbitsha_device_t *device = NULL;
    int                 index;

    if(major != rabbitsha_drv.major){
		 EMSG("open() major number mismatch. drv(%d) != open(%d)\n",
			  rabbitsha_drv.major, major);
		 return -ENXIO;
    }

    if( (minor < rabbitsha_drv.minor)                          || 
		(minor > (rabbitsha_drv.minor + rabbitsha_drv.nb_dev)) )
    {
		 EMSG ("open() cannot find device, minor %d not associated\n", minor);
		 return -ENXIO;
    }

    index = minor - rabbitsha_drv.minor;
    device = &rabbitsha_drv.devs[index];

    if (device == NULL)
    {
        EMSG(" open() cannot find registered device\n");
        return -ENXIO;
    }

    DMSG("open() major %d minor %d linked to device index %d, state=%d\n",
		 major, minor, index, device->state);

    switch (device->state)
    {
    case 0: // waiting for host file name
        device->buffer[0] = 0;
        device->state = 1;
        break;
    case 1: // getting host file name in progress
        EMSG ("getting host file name in progress on another descriptor.\n");
        return -ENXIO;
    case 2: // waiting for communication with host file
        device->state = 3;
        break;
    case 3: // communication with host file in progress
          break;
    default:
        EMSG ("invalid host file status %d in open function\n", device->state);
        return -ENXIO;
    }

    device->open_count ++;
    file->private_data = device;

    return 0;
}

static int
open_host_file (rabbitsha_device_t *device)
{
    unsigned long   status;
    char            ch;
    int             i;

    for (i = 0; ;i++)
    {
        ch = device->buffer[i];
        if (ch == '\n' || ch == 0)
        {
            device->buffer[i] = 0;
            break;
        }
        //DMSG ("fn[%d]=%d,%c,\n", i, ch, ch);
    }
    for (i = strlen (device->buffer) - 1; i >= 0 && device->buffer[i] == ' '; i--)
        device->buffer[i] = 0;
    DMSG("host filename=%s, filename length=%d\n", device->buffer, strlen (device->buffer));

    writel (virt_to_phys (device->buffer), device->base_addr + BLOCK_DEVICE_BUFFER);
    writel (strlen (device->buffer) + 1, device->base_addr + BLOCK_DEVICE_COUNT);
    writel (BLOCK_DEVICE_FILE_NAME, device->base_addr + BLOCK_DEVICE_OP);

    while ((status = readl(device->base_addr + BLOCK_DEVICE_STATUS)) == BLOCK_DEVICE_BUSY) ;

    if (status != BLOCK_DEVICE_WRITE_SUCCESS)
    {
        EMSG ("cannot open host file %s\n", device->buffer);
        return -ENXIO;
    }

    device->state = 3;

    return 0;
}


static int
rabbitsha_chr_ioctl_open (rabbitsha_device_t *device, char *file_name)
{
    long ret;

    if (device->state == 3 && device->open_count > 1)
    {
        EMSG ("device busy, state=%d\n", device->state);
        return -EBUSY;
    }
    device->state = 1;

    device->buffer[0] = 0;
    ret = strncpy_from_user (device->buffer, file_name, BLOCK_DEVICE_DRV_BUF_SIZE);
    
    return open_host_file (device);
}

static long
rabbitsha_chr_ioctl(struct file *file, unsigned int code, unsigned long buffer)
{
    rabbitsha_device_t *device;
    int                 ret = 0;

    device = file->private_data;

    if(device == NULL)
    {
        EMSG("ioctl() on undefined device\n");
        return -EBADF;
    }

    switch(code){
    case RABBITSHA_IOCSOPEN:
        ret = rabbitsha_chr_ioctl_open (device, (char *) buffer);
        break;
    default:
        EMSG("ioctl(0x%x, 0x%lx) not implemented\n", code, buffer);
        ret = -ENOTTY;
    }

    return ret;
}

#define strf(x) # x
#define stf(x) strf(x)

static int
rabbitsha_chr_release(struct inode *inode, struct file *file)
{
    rabbitsha_device_t  *device = (rabbitsha_device_t *)file->private_data;

    DMSG("release() major %d minor %d, state=%d\n", 
		 MAJOR(inode->i_rdev), MINOR(inode->i_rdev), device->state);

    device->open_count --;
    file->private_data = 0;

    switch (device->state)
    {
    case 1: // getting host file name in progress
        device->state = 0;
        break;
    case 3: // communication with host file in progress
        if (!device->open_count)
            device->state = 2;
        break;
    default:
        EMSG ("invalid host file status %d in release function\n", device->state);
        return -ENXIO;
    }

    return 0;
}

static ssize_t
rabbitsha_chr_read(struct file *file, char __user *data,
				   size_t size, loff_t *loff)
{
    rabbitsha_device_t *device = (rabbitsha_device_t *) file->private_data;
    DMSG("read() 0x%x bytes from 0x%08x\n", size, (unsigned int) loff);

    switch (device->state)
    {
    case 1: // getting host file name in progress
        EMSG ("No host file opened. Cannot read.\n");
        return -ENXIO;
    case 3: // communication with host file in progress
    {
        unsigned long       status;

        if (size > BLOCK_DEVICE_DRV_BUF_SIZE)
            size = BLOCK_DEVICE_DRV_BUF_SIZE;

        writel (virt_to_phys (device->buffer), device->base_addr + BLOCK_DEVICE_BUFFER);
        writel ((unsigned long) *loff, device->base_addr + BLOCK_DEVICE_LBA);
        writel (size, device->base_addr + BLOCK_DEVICE_COUNT);
        writel (BLOCK_DEVICE_READ, device->base_addr + BLOCK_DEVICE_OP);

        while ((status = readl(device->base_addr + BLOCK_DEVICE_STATUS)) == BLOCK_DEVICE_BUSY) ;

        if (status != BLOCK_DEVICE_READ_SUCCESS)
        {
            device->state = 0;
            EMSG ("error writting host file at ofs 0x%lx\n", (unsigned long) *loff);
            return -ENXIO;
        }
        else
        {
            COPY_TO_USER (data, device->buffer, size);
            size = readl (device->base_addr + BLOCK_DEVICE_COUNT);
            DMSG("read() read 0x%x bytes\n", size);
        }
        break;
    }
    default:
        EMSG ("invalid host file status %d in release function\n", device->state);
        return -ENXIO;
    }

    *loff += size;

    return size;
}

static ssize_t
rabbitsha_chr_write(struct file *file, const char __user *data,
					size_t size, loff_t *loff)
{
    rabbitsha_device_t *device = (rabbitsha_device_t *) file->private_data;

    DMSG("write() 0x%x bytes from 0x%08x, state=%d\n", size, (unsigned int) data, device->state);

    switch (device->state)
    {
    case 1: // getting host file name in progress
        EMSG ("no host file opened\n");
        return -ENXIO;
    case 3: // communication with host file in progress
    {
        unsigned long       status;

        if (size > BLOCK_DEVICE_DRV_BUF_SIZE)
            size = BLOCK_DEVICE_DRV_BUF_SIZE;

        COPY_FROM_USER (device->buffer, data, size);
        writel (virt_to_phys (device->buffer), device->base_addr + BLOCK_DEVICE_BUFFER);
        writel ((unsigned long) *loff, device->base_addr + BLOCK_DEVICE_LBA);
        writel (size, device->base_addr + BLOCK_DEVICE_COUNT);
        writel (BLOCK_DEVICE_WRITE, device->base_addr + BLOCK_DEVICE_OP);

        while ((status = readl(device->base_addr + BLOCK_DEVICE_STATUS)) == BLOCK_DEVICE_BUSY) ;

        if (status != BLOCK_DEVICE_WRITE_SUCCESS)
        {
            device->state = 0;
            EMSG ("error writting host file at ofs 0x%lx\n", (unsigned long) *loff);
            return -ENXIO;
        }
        break;
    }
    default:
        EMSG ("invalid host file status %d in release function\n", device->state);
        return -ENXIO;
    }

    *loff += size;
    
    return size;
}

static loff_t
rabbitsha_chr_llseek (struct file *file, loff_t off, int whence)
{
    rabbitsha_device_t  *device = (rabbitsha_device_t *) file->private_data;
    loff_t              newpos;

    if (device->state == 1)
    {
        EMSG ("no host file opened\n");
        return -ENXIO;
    }

    switch(whence)
    {
    case 0: /* SEEK_SET */
        newpos = off;
        break;
    case 1: /* SEEK_CUR */
        newpos = file->f_pos + off;
        break;
    case 2: /* SEEK_END */
        newpos = readl (device->base_addr + BLOCK_DEVICE_SIZE) + off;
        break;
    default: /* can’t happen */
        return -EINVAL;
    }

    file->f_pos = newpos;
    return newpos;
}


static int
rabbitsha_cdev_setup(rabbitsha_device_t *dev, int index)
{
    int devno = MKDEV(rabbitsha_drv.major, rabbitsha_drv.minor + index);
    int ret = 0;

    cdev_init(&dev->cdev, &rabbitsha_chr_fops);
    dev->cdev.owner  = THIS_MODULE;
    dev->cdev.ops    = &rabbitsha_chr_fops;
    dev->minor       = MINOR(devno);
    ret              = cdev_add(&dev->cdev, devno, 1);

    if(ret){
        DMSG("error %d adding rabbitsha char device %d\n", ret, index);
        return ret;
    }

    DMSG("registered cdev major %d minor %d, index %d\n",
		 rabbitsha_drv.major, dev->minor, index);

    return ret;
}

static int
rabbitsha_cdev_release(rabbitsha_device_t *dev)
{
    DMSG("released cdev major %d minor %d\n", rabbitsha_drv.major, dev->minor);
    cdev_del(&dev->cdev);

    return 0;
}

static int __devinit
rabbitsha_dev_init(void)
{
    rabbitsha_device_t *rabbitsha_dev;
    int                 rabbitsha_idx;
    void __iomem       *base_addr;

    DMSG("Device init\n");

    rabbitsha_idx = rabbitsha_drv.nb_dev;
    rabbitsha_dev = &rabbitsha_drv.devs[rabbitsha_idx];
    rabbitsha_dev->state = 0;
    rabbitsha_drv.nb_dev++;
    rabbitsha_dev->buffer = kmalloc (BLOCK_DEVICE_DRV_BUF_SIZE, GFP_KERNEL);

    base_addr = ioremap((unsigned long)RABBITSHA_BASE_ADDRESS, SZ_4K);
 
    if(!base_addr)
        return -ENOMEM;
    else
        rabbitsha_dev->base_addr = base_addr;

    DMSG("Got a base_addr : 0x%08lx\n", (unsigned long) base_addr);

    /* setup char dev here */
    rabbitsha_cdev_setup(rabbitsha_dev, rabbitsha_idx);

    return 0;
}

static int __devexit
rabbitsha_dev_exit(rabbitsha_device_t *rabbitsha_dev)
{
    rabbitsha_cdev_release(rabbitsha_dev);

    iounmap(rabbitsha_dev->base_addr);
    if (rabbitsha_dev->buffer)
        kfree (rabbitsha_dev->buffer);

    return 0;
}

static int __init
rabbitsha_drv_init(void)
{
    dev_t chrdev;
    int   ret;

    MSG("module version %d.%d.%d\n",
		(RABBITSHA_VERSION >> 16), (RABBITSHA_VERSION >> 8) & 0xff,
		(RABBITSHA_VERSION) & 0xff);

    memset(&rabbitsha_drv, 0, sizeof(rabbitsha_driver_t));

    ret = alloc_chrdev_region(&chrdev, 0, RABBITSHA_MAXDEV,
							  RABBITSHA_MODULE_NAME);

    if(ret == 0){
        rabbitsha_drv.major = MAJOR(chrdev);
        rabbitsha_drv.minor = MINOR(chrdev);
        MSG("char driver registered using major %d, first minor %d\n",
			rabbitsha_drv.major, rabbitsha_drv.minor);
    }else{
        MSG("driver could not register chr major (%d)\n", ret);
        goto rabbitsha_init_err_chr_register;
    }

    rabbitsha_dev_init();

    return 0;

rabbitsha_init_err_chr_register:
    unregister_chrdev_region(MKDEV(rabbitsha_drv.major, rabbitsha_drv.minor),
							 RABBITSHA_MAXDEV);
    return ret;
}

static void __exit
rabbitsha_drv_cleanup(void)
{
    int                                 i;

    for(i = 0; i < rabbitsha_drv.nb_dev; i++)
    {
        rabbitsha_dev_exit(&rabbitsha_drv.devs[i]);
    }

    DMSG("CHR unregister driver\n");
    unregister_chrdev_region(MKDEV(rabbitsha_drv.major, rabbitsha_drv.minor),
							 RABBITSHA_MAXDEV);
}

module_init(rabbitsha_drv_init);
module_exit(rabbitsha_drv_cleanup);

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
