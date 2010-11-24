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

#ifndef _RABBITSFB_REGS_H
#define _RABBITSFB_REGS_H

#define RABBITSFB_BASE_ADDRESS     0xA5000000

#define RABBITSFB_SIZE             0x0000
#define RABBITSFB_FBADDR           0x0004
#define RABBITSFB_IRQ_STAT         0x0008
#define RABBITSFB_IRQCLEAR         0x0008
#define RABBITSFB_STAT             0x000C
#define RABBITSFB_CTRL             0x000C
#define RABBITSFB_MODE             0x0010

#define RABBITSFB_MEM_BASE         0x1000 /* one PAGE further */


#define RABBITSFB_CTRL_START      (1 << 0)
#define RABBITSFB_CTRL_IRQEN      (1 << 1)
#define RABBITSFB_CTRL_DMAEN      (1 << 2)
#define RABBITSFB_CTRL_DISPLAY    (1 << 3)

/* TO BE REMOVED */
#define QEMU_INT_RAMDAC_MASK        0x04

#endif /* _RABBITSFB_REGS_H */

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

