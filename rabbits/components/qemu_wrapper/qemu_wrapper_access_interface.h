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

#ifndef _QEMU_WRAPPER_ACCESS_INTERFACE_
#define _QEMU_WRAPPER_ACCESS_INTERFACE_

#include <systemc.h>

class qemu_wrapper_access_interface : public sc_interface
{
public :	
    virtual unsigned long get_no_cpus  () = 0;
    virtual unsigned long get_cpu_fv_level (unsigned long cpu) = 0;
    virtual void set_cpu_fv_level (unsigned long cpu, unsigned long val) = 0;
    virtual void generate_swi (unsigned long cpu_mask, unsigned long swi) = 0;
    virtual void swi_ack (int cpu, unsigned long swi_mask) = 0;

    virtual unsigned long get_cpu_ncycles (unsigned long cpu) = 0;
    virtual uint64 get_no_cycles_cpu (int cpu) = 0;

    virtual unsigned long get_int_status () = 0;
    virtual unsigned long get_int_enable () = 0;
    virtual void set_int_enable (unsigned long val) = 0;
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
