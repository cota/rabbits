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

#ifndef _CFG_H_152G_
#define _CFG_H_152G_

#define ENERGY_TRACE_ENABLED
//#define TIME_AT_FV_LOG_GRF

#ifdef ENERGY_TRACE_ENABLED
#define ETRACE_NB_CPU_IN_GROUP 4
#endif

#ifndef BIT
#define BIT(nr)         (1 << (nr))
#endif

#define CONFIG_L2M
//#define CONFIG_L3

#define NO_CPUS 4
/*
 * NOTE: we in fact get 2**n - 1 acc's. This allows for a regular
 * split of the address space.
 */
#define NO_L2MS_BITS 2
/* remember: update the maps file! */
#define NO_L2MS	(BIT(NO_L2MS_BITS) - 1)

#define L2M_SIZE_BITS	19
#define L2M_THRESHOLD_BITS	13
#define L2M_THRESHOLD	BIT(L2M_THRESHOLD_BITS)
#define L2M_SLAVE_ID	(NO_CPUS + 3)
#define L2M_ASSOC		(L2M_SIZE_BITS - L2M_THRESHOLD_BITS)

/* retval: 0 means local cache block; remote otherwise. */
static inline int addr_to_l2m(unsigned long addr)
{
    int l2m_id = (addr >> L2M_THRESHOLD_BITS) & NO_L2MS;

    if (!l2m_id)
        return 0;

    return l2m_id - 1 + L2M_SLAVE_ID;
}

#define from_l2m(src)	(src >= L2M_SLAVE_ID && src < (L2M_SLAVE_ID + NO_L2MS))

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
