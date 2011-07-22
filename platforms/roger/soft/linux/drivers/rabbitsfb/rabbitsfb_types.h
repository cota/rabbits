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

#ifndef _RABBITSFB_TYPES_H
#define _RABBITSFB_TYPES_H

#include <linux/spinlock.h>
#include <linux/cdev.h>

#define RABBITSFB_MAXDEV    1
#define RABBITSFB_MAXIRQ    1

typedef struct rabbitsfb_driver rabbitsfb_driver_t;
typedef struct rabbitsfb_device rabbitsfb_device_t;
typedef struct rabbitsfb_int    rabbitsfb_int_t;


struct rabbitsfb_int {
    unsigned int      type;
    unsigned int      status;
    unsigned int      pending;
    wait_queue_head_t wait_queue;
};

struct rabbitsfb_device {
    int               minor;                  /* Char device minor            */
    int               index;                  /* Char device major            */
    struct cdev       cdev;                   /* Char device                  */

    void __iomem	 *base_addr;              /* Register file mapped address */
                                              /*        - logical address     */
    void __iomem	 *mem_addr;               /* Memory mapped address        */
                                              /*        - logical address     */

    spinlock_t        irq_lock;               /* IRQ spinlock                 */
    int               irq_en;                 /* Are hardware IRQ enabled ?   */
    int               nb_irq_en;              /* Number of logical interrupts */
    rabbitsfb_int_t   irqs[RABBITSFB_MAXIRQ]; /* Logical IRQ descriptors      */
};


struct rabbitsfb_driver
{
    int                major;
    int                minor;
    int                nb_dev;
    rabbitsfb_device_t devs[RABBITSFB_MAXDEV];
};

#endif /* _RABBITSFB_TYPES_H */

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
