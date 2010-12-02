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

#ifndef _QEMU_DRV_IOCTL_H
#define _QEMU_DRV_IOCTL_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/kernel.h>
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#include <linux/ioctl.h>

typedef struct qemu_ioc_set_cpux_fv qemu_ioc_set_cpux_fv_t;


#define QEMU_DRV_IOC_MAGIC  'q'

#define QEMU_DRV_IOCRESET                    _IO  (QEMU_DRV_IOC_MAGIC, 0)
#define QEMU_DRV_IOCS_SET_CPUS_FV            _IOW (QEMU_DRV_IOC_MAGIC, 1, unsigned char *)
#define QEMU_DRV_IOC_TEST                    _IO  (QEMU_DRV_IOC_MAGIC, 2)
#define QEMU_DRV_IOCS_PROCESS_FV             _IOW (QEMU_DRV_IOC_MAGIC, 3, unsigned long)
#define QEMU_DRV_IOC_MEASURE_STA             _IO  (QEMU_DRV_IOC_MAGIC, 4)
#define QEMU_DRV_IOC_MEASURE_STO             _IOR (QEMU_DRV_IOC_MAGIC, 5, unsigned char *)
#define QEMU_DRV_IOCS_SET_CPUX_FV            _IOW (QEMU_DRV_IOC_MAGIC, 6,  qemu_ioc_set_cpux_fv_t *)

struct qemu_ioc_set_cpux_fv
{
    uint32_t cpu;
    uint32_t frequency_level;
};

#endif

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
