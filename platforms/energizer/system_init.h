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

#ifndef _SYSTEM_INIT_H_
#define _SYSTEM_INIT_H_

typedef struct
{
    const char          *cpu_family;
    const char          *cpu_model;
    const char          *kernel_filename;
    const char          *ec_kernel_filename;
    const char          *initrd_filename;
    const char          *kernel_cmdline;
    int                 no_cpus;
    int                 ramsize;
    int                 ec_ramsize;
    int                 sramsize;
    int                 gdb_port;
    int                 ec_gdb_port;
} init_struct;

void parse_cmdline (int argc, char **argv, init_struct *is);
int check_init (init_struct *is);
void arm_load_dnaos (init_struct *is);
void arm_load_kernel (init_struct *is);

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
