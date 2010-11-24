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

#if 0
#define DEBUG
#define HDEBUG
#endif

#include "debug.h"
#include "rabbitsfb_types.h"
#include "rabbitsfb_ioctl.h"
#include "rabbitsfb_regs.h"
#include "rabbitsfb_int.h"

MODULE_AUTHOR       ("Nicolas Fournel and Marius Gligor");
MODULE_DESCRIPTION  ("RABBITS FB driver");
MODULE_LICENSE      ("GPL"); 
MODULE_VERSION      ("0.0.1");

#define RABBITSFB_MODULE_NAME     "rabbitsfb"
#define RABBITSFB_VERSION         0x000001

#define RABBITSFB_MAJOR           252
#define RABBITSFB_MINOR           0

static rabbitsfb_driver_t rabbitsfb_drv;

static int     rabbitsfb_chr_open   (struct inode *, struct file *);
static int     rabbitsfb_chr_ioctl  (struct inode *, struct file *, unsigned int, unsigned long);
static int     rabbitsfb_chr_release(struct inode *, struct file *);

static ssize_t rabbitsfb_chr_read   (struct file *,       char __user *, size_t, loff_t *);
static ssize_t rabbitsfb_chr_write  (struct file *, const char __user *, size_t, loff_t *);
static int     rabbitsfb_chr_mmap   (struct file *, struct vm_area_struct *);

static struct file_operations rabbitsfb_chr_fops =
{
  .owner        = THIS_MODULE,
  .read         = rabbitsfb_chr_read,
  .write        = rabbitsfb_chr_write,
  .ioctl        = rabbitsfb_chr_ioctl,
  .mmap         = rabbitsfb_chr_mmap,
  .open         = rabbitsfb_chr_open,
  .release      = rabbitsfb_chr_release,
};

int
rabbitsfb_stop(rabbitsfb_device_t *dev)
{

    u32 val = 0;

    val = readl(dev->base_addr + RABBITSFB_CTRL);
    val &= ~RABBITSFB_CTRL_START;
    writel(val, dev->base_addr + RABBITSFB_CTRL);

    return 0;
}

int
rabbitsfb_reset(rabbitsfb_device_t *dev)
{

    u32 val = 0;

    val = readl(dev->base_addr + RABBITSFB_CTRL);
    val &= ~RABBITSFB_CTRL_START;
    writel(val, dev->base_addr + RABBITSFB_CTRL);
    val |= RABBITSFB_CTRL_START;
    writel(val, dev->base_addr + RABBITSFB_CTRL);

    return 0;
}

int
rabbitsfb_set_size(rabbitsfb_device_t *dev, rabbitsfb_ioc_size_t *sz)
{
    u32                       val;
    int                       ret = 0;

    DMSG("Set size to %d %d\n", sz->width, sz->height);

    val = (sz->width & 0xFFFF) | ((sz->height & 0xFFFF) << 16);
    writel(val, dev->base_addr + RABBITSFB_SIZE);

    return ret;
}

int
rabbitsfb_set_addr(rabbitsfb_device_t *dev, rabbitsfb_ioc_addr_t *addr)
{
    int                     ret = 0;

    DMSG("Set addr to 0x%x\n", addr->addr);

    writel(addr->addr, (dev->base_addr) + RABBITSFB_FBADDR);

    return ret;
}

int
rabbitsfb_dma_enable(rabbitsfb_device_t *dev)
{
    int ret = 0;
    u32 val = 0;

    DMSG("DMA Enable\n");

    val = readl((dev->base_addr) + RABBITSFB_CTRL);
    val |= RABBITSFB_CTRL_DMAEN;
    writel(val, (dev->base_addr) + RABBITSFB_CTRL);

    return ret;
}

int
rabbitsfb_display(rabbitsfb_device_t *dev)
{
    int ret = 0;
    u32 val = 0;

    DMSG("display !\n");

    val = readl((dev->base_addr) + RABBITSFB_CTRL);
    val |= RABBITSFB_CTRL_DISPLAY;
    writel(val, (dev->base_addr) + RABBITSFB_CTRL);

    return ret;
}

int
rabbitsfb_set_mode(rabbitsfb_device_t *dev, rabbitsfb_ioc_mode_t *mode)
{
    int ret = 0;

    DMSG("Set mode to 0x%x\n", mode->mode);

    writel(mode->mode, (dev->base_addr) + RABBITSFB_MODE);

    return ret;
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

static int
rabbitsfb_chr_ioctl_reset(rabbitsfb_device_t *dev)
{
    return rabbitsfb_reset(dev);
}

static int
rabbitsfb_chr_ioctl_setsize(rabbitsfb_device_t *dev, void *buf)
{
    int                  ret = 0;
    rabbitsfb_ioc_size_t sz;

    COPY_FROM_USER((void *)&sz, buf, sizeof(sz));
    ret = rabbitsfb_set_size (dev, &sz);

    return ret;
}

static int
rabbitsfb_chr_ioctl_setaddr(rabbitsfb_device_t *dev, void *buf)
{
    rabbitsfb_ioc_addr_t addr;
    int                  ret = 0;

    COPY_FROM_USER((void *)&addr, buf, sizeof(addr));
    ret = rabbitsfb_set_addr(dev, &addr);

    return ret;
}

static int
rabbitsfb_chr_ioctl_irq_register(rabbitsfb_device_t *dev, void *buf)
{
    rabbitsfb_ioc_irq_t irq;
    int                 ret = 0;

    COPY_FROM_USER((void *)&irq, buf, sizeof(irq));
    ret = rabbitsfb_int_register(dev, &irq);

    return ret;
}

static int
rabbitsfb_chr_ioctl_irq_wait(rabbitsfb_device_t *dev, void *buf)
{
    rabbitsfb_ioc_irq_t irq;
    int                 ret = 0;

    COPY_FROM_USER((void *)&irq, buf, sizeof(irq));
    ret = rabbitsfb_int_wait (dev, &irq);

    return ret;
}

static int
rabbitsfb_chr_ioctl_irq_unregister(rabbitsfb_device_t *dev, void *buf)
{
    rabbitsfb_ioc_irq_t irq;
    int                 ret = 0;

    COPY_FROM_USER((void *) &irq, buf, sizeof(irq));
    ret = rabbitsfb_int_unregister(dev, &irq);

    return ret;
}

static int
rabbitsfb_chr_ioctl_dmaen(rabbitsfb_device_t *dev)
{
    return rabbitsfb_dma_enable(dev); 
}

static int
rabbitsfb_chr_ioctl_display(rabbitsfb_device_t *dev)
{
    return rabbitsfb_display(dev); 
}

static int
rabbitsfb_chr_ioctl_setmode(rabbitsfb_device_t *dev, void *buf)
{
    rabbitsfb_ioc_mode_t mode;
    int                  ret = 0;

    COPY_FROM_USER((void *)&mode, buf, sizeof(mode));
    ret = rabbitsfb_set_mode(dev, &mode);

    return ret;
}


/*
 * Char dev Handling
 */
static int
rabbitsfb_chr_open(struct inode *inode, struct file *file)
{
    const int           major  = MAJOR(inode->i_rdev);
    const int           minor  = MINOR(inode->i_rdev);
    rabbitsfb_device_t *device = NULL;
    int                 index;

    if(major != rabbitsfb_drv.major){
		 EMSG("open() major number mismatch. drv(%d) != open(%d)\n",
			  rabbitsfb_drv.major, major);
		 return -ENXIO;
    }

    if( (minor < rabbitsfb_drv.minor)                          || 
		(minor > (rabbitsfb_drv.minor + rabbitsfb_drv.nb_dev)) ){
		 EMSG ("open() cannot find device, minor %d not associated\n", minor);
		 return -ENXIO;
    }

    index = minor - rabbitsfb_drv.minor;
    DMSG("open() major %d minor %d linked to device index %d\n",
		 major, minor, index);

    device = &rabbitsfb_drv.devs[index];

    if(device == NULL){
        EMSG(" open() cannot find registered device\n");
        return -ENXIO;
    }

    file->private_data = device;
    return 0;
}

static int
rabbitsfb_chr_ioctl(struct inode *inode, struct file *file,
					unsigned int code, unsigned long buffer)
{
    rabbitsfb_device_t *dev;
    int                 ret = 0;

    dev = file->private_data;

    if(dev == NULL){
        EMSG("ioctl() on undefined device\n");
        return -EBADF;
    }

    switch(code){

    case RABBITSFB_IOCRESET:
        DMSG("RABBITSFB_IOCRESET\n");
        ret = rabbitsfb_chr_ioctl_reset(dev);
        break;
    case RABBITSFB_IOCSSIZE:
        DMSG("RABBITSFB_IOCSSIZE\n");
        ret = rabbitsfb_chr_ioctl_setsize(dev, (void *)buffer);
        break;
    case RABBITSFB_IOCSADDR:
        DMSG("RABBITSFB_IOCSADDR\n");
        ret = rabbitsfb_chr_ioctl_setaddr(dev, (void *)buffer);
        break;
    case RABBITSFB_IOCSIRQREG:
        DMSG("RABBITSFB_IOCSIRQREG\n");
        ret = rabbitsfb_chr_ioctl_irq_register(dev, (void *)buffer);
        break;
    case RABBITSFB_IOCSIRQWAIT:
        DMSG("RABBITSFB_IOCSIRQWAIT\n");
        ret = rabbitsfb_chr_ioctl_irq_wait(dev, (void *)buffer);
        break;
    case RABBITSFB_IOCSIRQUNREG:
        DMSG("RABBITSFB_IOCSIRQUNREG\n");
        ret = rabbitsfb_chr_ioctl_irq_unregister(dev, (void *)buffer);
        break;
    case RABBITSFB_IOCDMAEN:
        DMSG("RABBITSFB_IOCDMAEN\n");
        ret = rabbitsfb_chr_ioctl_dmaen(dev);
        break;
    case RABBITSFB_IOCDISPLAY:
        DMSG("RABBITSFB_IOCDISPLAY\n");
        ret = rabbitsfb_chr_ioctl_display(dev);
        break;
    case RABBITSFB_IOCSMODE:
        DMSG("RABBITSFB_IOCSMODE\n");
        ret = rabbitsfb_chr_ioctl_setmode(dev, (void *)buffer);
        break;

    default:
        EMSG("ioctl(0x%x, 0x%lx) not implemented\n", code, buffer);
        ret = -ENOTTY;
    }

    return ret;
}

static int
rabbitsfb_chr_release(struct inode *inode, struct file *file)
{
    rabbitsfb_device_t *dev = (rabbitsfb_device_t *)file->private_data;

    DMSG("release() major %d minor %d\n", 
		 MAJOR(inode->i_rdev), MINOR(inode->i_rdev));

    rabbitsfb_int_cleanup(dev);
    rabbitsfb_stop(dev);

    file->private_data = 0;

    return 0;
}

static ssize_t
rabbitsfb_chr_read(struct file *file, char __user *data,
				   size_t size, loff_t *loff)
{
    DMSG("read()\n");
    return 0;
}

static ssize_t
rabbitsfb_chr_write(struct file *file, const char __user *data,
					size_t size, loff_t *loff)
{
    rabbitsfb_device_t *dev = (rabbitsfb_device_t *)file->private_data;


    DMSG("write() %x bytes from 0x%08x\n", size, (unsigned int)data);

    if(*loff != 0){
        EMSG("Bad write()\n");
    }

    memcpy_toio((void *)dev->mem_addr, (void *)data, size);

    DMSG("write() done\n");

    return 0;
}

static int
rabbitsfb_chr_mmap(struct file *file, struct vm_area_struct *vma)
{
    int                 ret      = 0; 
    rabbitsfb_device_t *dev      = (rabbitsfb_device_t *)file->private_data;
    uint32_t            vsize    = vma->vm_end - vma->vm_start;
    unsigned long       physical = RABBITSFB_BASE_ADDRESS + RABBITSFB_MEM_BASE;

    vma->vm_flags |= VM_IO | VM_RESERVED;

    MSG("Got a mmap: for dev: %d\n", dev->minor);
    MSG("          vma start: 0x%lx\n", vma->vm_start);
    MSG("              end  : 0x%lx\n", vma->vm_end);
    MSG("              pgoff: 0x%lx\n", vma->vm_pgoff);
    MSG("              flags: 0x%lx\n", vma->vm_flags);
    MSG(" physical: 0x%lx\n", physical);


    /* remap_pfn_range(vma, vma->vm_start, (physical>>PAGE_SHIFT), vsize, vma->vm_page_prot); */
    io_remap_pfn_range(vma, vma->vm_start, (physical>>PAGE_SHIFT), vsize, vma->vm_page_prot);

    /* writel(0x000000000, dev->mem_addr); */

    return ret;
}

static int
rabbitsfb_cdev_setup(rabbitsfb_device_t *dev, int index)
{
    int devno = MKDEV(rabbitsfb_drv.major, rabbitsfb_drv.minor + index);
    int ret = 0;

    cdev_init(&dev->cdev, &rabbitsfb_chr_fops);
    dev->cdev.owner  = THIS_MODULE;
    dev->cdev.ops    = &rabbitsfb_chr_fops;
    dev->minor       = MINOR(devno);
    ret              = cdev_add(&dev->cdev, devno, 1);

    if(ret){
        DMSG("error %d adding rabbitsfb char device %d\n", ret, index);
        return ret;
    }

    DMSG("registered cdev major %d minor %d, index %d\n",
		 rabbitsfb_drv.major, dev->minor, index);

    return ret;
}

static int
rabbitsfb_cdev_release(rabbitsfb_device_t *dev)
{
    DMSG("released cdev major %d minor %d\n", rabbitsfb_drv.major, dev->minor);
    cdev_del(&dev->cdev);

    return 0;
}

static int __devinit
rabbitsfb_dev_init(void)
{
    rabbitsfb_device_t *rabbitsfb_dev;
    int                 rabbitsfb_idx;
    void __iomem       *base_addr;
    void __iomem       *mem_addr;

    DMSG("Device init\n");

    rabbitsfb_idx = rabbitsfb_drv.nb_dev;
    rabbitsfb_dev = &rabbitsfb_drv.devs[rabbitsfb_idx];
    rabbitsfb_drv.nb_dev++;

    base_addr = ioremap((unsigned long)RABBITSFB_BASE_ADDRESS, SZ_4K);
    mem_addr = ioremap((unsigned long)RABBITSFB_BASE_ADDRESS + RABBITSFB_MEM_BASE, SZ_4M);
 
    if(!base_addr)
        return -ENOMEM;
    else
        rabbitsfb_dev->base_addr = (unsigned long)base_addr;

    if(!mem_addr)
        return -ENOMEM;
    else
        rabbitsfb_dev->mem_addr = (unsigned long)mem_addr;

    DMSG("Got a base_addr : 0x%08lx\n", (unsigned long)base_addr);
    DMSG("Got a mem_addr : 0x%08lx\n", (unsigned long)mem_addr);

    rabbitsfb_int_init(rabbitsfb_dev);

    /* setup char dev here */
    rabbitsfb_cdev_setup(rabbitsfb_dev, rabbitsfb_idx);

    return 0;
}

static int __devexit
rabbitsfb_dev_exit(rabbitsfb_device_t *rabbitsfb_dev)
{
    rabbitsfb_cdev_release(rabbitsfb_dev);
    rabbitsfb_int_cleanup(rabbitsfb_dev);

    iounmap((void *)rabbitsfb_dev->base_addr);
    iounmap((void *)rabbitsfb_dev->mem_addr);

    return 0;
}

static int __init
rabbitsfb_drv_init(void)
{
    dev_t chrdev;
    int   ret;

    MSG("module version %d.%d.%d\n",
		(RABBITSFB_VERSION >> 16), (RABBITSFB_VERSION >> 8) & 0xff,
		(RABBITSFB_VERSION) & 0xff);

    memset(&rabbitsfb_drv, 0, sizeof(rabbitsfb_driver_t));

    ret = alloc_chrdev_region(&chrdev, 0, RABBITSFB_MAXDEV,
							  RABBITSFB_MODULE_NAME);

    if(ret == 0){
        rabbitsfb_drv.major = MAJOR(chrdev);
        rabbitsfb_drv.minor = MINOR(chrdev);
        MSG("char driver registered using major %d, first minor %d\n",
			rabbitsfb_drv.major, rabbitsfb_drv.minor);
    }else{
        MSG("driver could not register chr major (%d)\n", ret);
        goto rabbitsfb_init_err_chr_register;
    }

    rabbitsfb_dev_init();

    return 0;

rabbitsfb_init_err_chr_register:
    unregister_chrdev_region(MKDEV(rabbitsfb_drv.major, rabbitsfb_drv.minor),
							 RABBITSFB_MAXDEV);
    return ret;
}

static void __exit
rabbitsfb_drv_cleanup(void)
{
    int                                 i;

    for(i = 0; i < rabbitsfb_drv.nb_dev; i++)
    {
        rabbitsfb_dev_exit(&rabbitsfb_drv.devs[i]);
    }

    DMSG("CHR unregister driver\n");
    unregister_chrdev_region(MKDEV(rabbitsfb_drv.major, rabbitsfb_drv.minor),
							 RABBITSFB_MAXDEV);
}

module_init(rabbitsfb_drv_init);
module_exit(rabbitsfb_drv_cleanup);

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
