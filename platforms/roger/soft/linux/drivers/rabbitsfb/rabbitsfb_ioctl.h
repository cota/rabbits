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

#ifndef _RABBITSFB_IOCTL_H
#define _RABBITSFB_IOCTL_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/kernel.h>
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#include <linux/ioctl.h>

typedef struct rabbitsfb_ioc_size rabbitsfb_ioc_size_t;
typedef struct rabbitsfb_ioc_addr rabbitsfb_ioc_addr_t;
typedef struct rabbitsfb_ioc_irq  rabbitsfb_ioc_irq_t;
typedef struct rabbitsfb_ioc_mode rabbitsfb_ioc_mode_t;

#define RABBITSFB_IOC_MAGIC  'w'

#define RABBITSFB_IOCRESET     _IO  (RABBITSFB_IOC_MAGIC, 0)
#define RABBITSFB_IOCSSIZE     _IOW (RABBITSFB_IOC_MAGIC, 1, rabbitsfb_ioc_size_t *)
#define RABBITSFB_IOCSADDR     _IOW (RABBITSFB_IOC_MAGIC, 2, rabbitsfb_ioc_addr_t *)

#define RABBITSFB_IOCSIRQREG   _IOWR(RABBITSFB_IOC_MAGIC, 3, rabbitsfb_ioc_irq_t *)
#define RABBITSFB_IOCSIRQWAIT  _IOW (RABBITSFB_IOC_MAGIC, 4, rabbitsfb_ioc_irq_t *)
#define RABBITSFB_IOCSIRQUNREG _IOW (RABBITSFB_IOC_MAGIC, 5, rabbitsfb_ioc_irq_t *)

#define RABBITSFB_IOCDMAEN     _IO  (RABBITSFB_IOC_MAGIC, 6)
#define RABBITSFB_IOCDISPLAY   _IO  (RABBITSFB_IOC_MAGIC, 7)
#define RABBITSFB_IOCSMODE     _IOW (RABBITSFB_IOC_MAGIC, 8, rabbitsfb_ioc_mode_t *)

struct rabbitsfb_ioc_size {
    uint16_t width;      /* Frame buffer width */
    uint16_t height;     /* Frame buffer width */
};

struct rabbitsfb_ioc_addr {
    uint32_t addr;       /* Frame buffer address */
};


enum rabbitsfb_mode {
    RABBITSFB_MODE_NONE   = 0,
    /* ******************************************* */
    RABBITSFB_MODE_GREY   = 1,
    /* ******************************************* */
    RABBITSFB_MODE_RGB    = 2, /* pix[0] = R       *
                                * pix[1] = G       *
                                * pix[2] = B       */
    /* ******************************************* */
    RABBITSFB_MODE_BGR    = 3, /* pix[0] = B       *
                                * pix[1] = G       *
                                * pix[2] = R       */
    /* ******************************************* */
    RABBITSFB_MODE_ARGB   = 4, /* pix[0] = A       *
                                * pix[1] = R       *
                                * pix[2] = G       *
                                * pix[3] = B       */
    /* ******************************************* */
    RABBITSFB_MODE_BGRA   = 5, /* pix[0] = B       *
                                * pix[1] = G       *
                                * pix[2] = R       *
                                * pix[3] = A       */
    /* ******************************************* */
    RABBITSFB_MODE_YVYU   = 6, /* Packed YUV 4:2:2 */
    /* ******************************************* */
    RABBITSFB_MODE_YV12   = 7, /* Planar YUV 4:2:0 */
    /* ******************************************* */
    RABBITSFB_MODE_IYUV   = 8, /* Planar YUV 4:2:0 */
    /* ******************************************* */
    RABBITSFB_MODE_YV16   = 9, /* Planar YUV 4:2:2 */
};

struct rabbitsfb_ioc_mode {
    uint32_t mode;       /* Frame buffer address */
};

struct rabbitsfb_ioc_irq {
    uint32_t id;         /* Identification of logical irq   */
    uint32_t type;       /* Bitfield of possible interrupts */
    int32_t  timeout;    /* Wait time for interrupt         */
};

#endif /* _RABBITSFB_IOCTL_H */

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
