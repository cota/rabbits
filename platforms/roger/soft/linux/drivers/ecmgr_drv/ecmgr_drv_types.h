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

#ifndef _ECMGR_DRV_TYPES_H
#define _ECMGR_DRV_TYPES_H

#include <linux/spinlock.h>
#include <linux/cdev.h>

#define ECMGR_DRV_MAXDEV    1

typedef struct ecmgr_drv_driver ecmgr_drv_driver_t;
typedef struct ecmgr_drv_device ecmgr_drv_device_t;

struct ecmgr_drv_device
{
    int                 minor;                      /* Char device minor              */
    int                 index;                      /* Char device major              */
    struct cdev         cdev;                       /* Char device                    */

    unsigned long       base_addr;                  /* Register file mapped address   */
                                                    /*        - logical address       */
    unsigned long       addr_core_thread_ec_cpus;
};

struct ecmgr_drv_driver
{
  int                   major;
  int                   minor;
  int                   nb_dev;
  ecmgr_drv_device_t    devs[ECMGR_DRV_MAXDEV];
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
