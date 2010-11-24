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

#ifndef _QEMU_DRV_TYPES_H
#define _QEMU_DRV_TYPES_H

#include <linux/spinlock.h>
#include <linux/cdev.h>

#define QEMU_DRV_MAXDEV    1

typedef struct qemu_drv_driver qemu_drv_driver_t;
typedef struct qemu_drv_device qemu_drv_device_t;

struct qemu_drv_device
{
    int                 minor;                      /* Char device minor              */
    int                 index;                      /* Char device major              */
    struct cdev         cdev;                       /* Char device                    */

    unsigned long       base_addr;                  /* Register file mapped address   */
                                                    /*        - logical address       */
};

struct qemu_drv_driver
{
  int                   major;
  int                   minor;
  int                   nb_dev;
  qemu_drv_device_t     devs[QEMU_DRV_MAXDEV];
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
