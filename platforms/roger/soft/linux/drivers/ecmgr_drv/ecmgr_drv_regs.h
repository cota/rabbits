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

#ifndef _ECMGR_DRV_REGS_H
#define _ECMGR_DRV_REGS_H

#define ECMGR_ADDR_BASE             0xAC000000UL

#define ECMGR_REG_COMMAND           0x0000
#define ECMGR_REG_DATA              0x0004
#define ECMGR_REG_RESERVED          0x0008
#define ECMGR_REG_RESET             0x000C
#define ECMGR_REG_DNA_THREAD        0x1000

#define DNA_IPI_DISPATCH_COMMAND    0xFFFF

#endif /* _ECMGR_DRV_REGS_H */

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
