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

#ifndef _RABBITSHA_TYPES_H
#define _RABBITSHA_TYPES_H

#include <linux/spinlock.h>
#include <linux/cdev.h>

#define RABBITSHA_MAXDEV    1
#define RABBITSHA_MAXIRQ    1


enum sl_block_registers {
    BLOCK_DEVICE_BUFFER     = 0x00,
    BLOCK_DEVICE_LBA        = 0x04,
    BLOCK_DEVICE_COUNT      = 0x08,
    BLOCK_DEVICE_OP         = 0x0C,
    BLOCK_DEVICE_STATUS     = 0x10,
    BLOCK_DEVICE_IRQ_ENABLE = 0x14,
    BLOCK_DEVICE_SIZE       = 0x18,
    BLOCK_DEVICE_BLOCK_SIZE = 0x1C,
};

enum sl_block_op {
	BLOCK_DEVICE_NOOP,
	BLOCK_DEVICE_READ,
	BLOCK_DEVICE_WRITE,
	BLOCK_DEVICE_FILE_NAME,
};

enum sl_block_status {
	BLOCK_DEVICE_IDLE          = 0,
	BLOCK_DEVICE_BUSY          = 1,
	BLOCK_DEVICE_READ_SUCCESS  = 2,
	BLOCK_DEVICE_WRITE_SUCCESS = 3,
	BLOCK_DEVICE_READ_ERROR    = 4,
	BLOCK_DEVICE_WRITE_ERROR   = 5,
	BLOCK_DEVICE_ERROR         = 6,
};

#define BLOCK_DEVICE_DRV_BUF_SIZE 1024

struct rabbitsha_device {
    int               minor;                  /* Char device minor            */
    int               index;                  /* Char device major            */
    struct cdev       cdev;                   /* Char device                  */

    void __iomem	  *base_addr;              /* Register file mapped address */
                                              /*        - logical address     */
    char              *buffer;
    int               state;
    int               open_count;
};
typedef struct rabbitsha_device rabbitsha_device_t;

struct rabbitsha_driver
{
    int                major;
    int                minor;
    int                nb_dev;
    rabbitsha_device_t devs[RABBITSHA_MAXDEV];
};

typedef struct rabbitsha_driver rabbitsha_driver_t;

#endif /* _RABBITSHA_TYPES_H */

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
