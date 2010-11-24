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

#ifndef _H264DBF_TYPES_H
#define _H264DBF_TYPES_H

#include <linux/spinlock.h>
#include <linux/cdev.h>

#define H264DBF_MAXDEV    1
#define H264DBF_MAXIRQ    2

typedef struct h264dbf_driver h264dbf_driver_t;
typedef struct h264dbf_device h264dbf_device_t;
typedef struct h264dbf_int    h264dbf_int_t;

struct h264dbf_int
{
    unsigned int        type;
    unsigned int        status;
    unsigned int        pending;
    wait_queue_head_t   wait_queue;
};

struct h264dbf_device
{
    int                 minor;                      /* Char device minor              */
    int                 index;                      /* Char device major              */
    struct cdev         cdev;                       /* Char device                    */

    unsigned long       base_addr;                  /* Register file mapped address   */
                                                    /*        - logical address       */

    spinlock_t          irq_lock;                   /* IRQ spinlock                   */
    int                 irq_en;                     /* Are hardware IRQ enabled ?     */
    int                 nb_irq_en;                  /* Number of logical interrupts   */
    h264dbf_int_t       irqs[H264DBF_MAXIRQ];       /* Logical interrupts descriptors */
};

struct h264dbf_driver
{
    int                   major;
    int                   minor;
    int                   nb_dev;
    h264dbf_device_t      devs[H264DBF_MAXDEV];
};

#endif /* _H264DBF_TYPES_H */

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
