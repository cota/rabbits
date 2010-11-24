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

#ifndef _H264DBF_REGS_H
#define _H264DBF_REGS_H

#define H264DBF_BASE_ADDRESS       0xA6000000
#define H264DBF_SIZE               0x01000000

#define REG_NSLICES                 0x39000 //read only
#define H264DBF_INT_ACK            0x39004
#define H264DBF_INT_STATUS         0x39008 //read only
#define H264DBF_INT_ENABLE         0x3900C
#define REG_SRAM_ADDRESS            0x39010
#define REG_INC_ADDRESS             0x39014
#define REG_NSLICES_INTR            0x39018

#define QEMU_INT_DBF_MASK           0x00008

#endif /* _H264DBF_REGS_H */

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
