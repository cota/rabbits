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

#ifndef _H264DBF_IOCTL_H
#define _H264DBF_IOCTL_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/kernel.h>
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#include <linux/ioctl.h>

#define H264DBF_IOC_MAGIC  'x'

#define H264DBF_IOCRESET                     _IO  (H264DBF_IOC_MAGIC, 0)
#define H264DBF_IOCSIRQREG                   _IOWR(H264DBF_IOC_MAGIC, 1, h264dbf_ioc_irq_t *)
#define H264DBF_IOCSIRQWAIT                  _IOW (H264DBF_IOC_MAGIC, 2, h264dbf_ioc_irq_t *)
#define H264DBF_IOCSIRQUNREG                 _IOW (H264DBF_IOC_MAGIC, 3, h264dbf_ioc_irq_t *)
#define H264DBF_IOCS_SET_TRANSFER_ADDR       _IOW (H264DBF_IOC_MAGIC, 4, unsigned long *)
#define H264DBF_IOCS_SET_NSLICES_INTR        _IOW (H264DBF_IOC_MAGIC, 5, unsigned char *)
#define H264DBF_IOCS_GET_HW_NSLICES          _IOR (H264DBF_IOC_MAGIC, 6, unsigned char *)

typedef struct h264dbf_ioc_irq  h264dbf_ioc_irq_t;

struct h264dbf_ioc_irq
{
    uint32_t id;         /* Identification of logical irq   */
    uint32_t type;       /* Bitfield of possible interrupts */
    int32_t  timeout;    /* Wait time for interrupt         */
};

#endif /* _H264DBF_IOCTL_H */

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
