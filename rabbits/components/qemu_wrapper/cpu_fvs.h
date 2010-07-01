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

#ifndef _CPU_FVS_H_
#define _CPU_FVS_H_

#include <cfg.h>

#ifdef ENERGY_TRACE_ENABLED
#include <etrace_if.h>
#endif

int get_cpu_nb_fv_levels (const char *cpufamily, const char *cpumodel);
int get_cpu_boot_fv_level (const char *cpufamily, const char *cpumodel);
double *get_cpu_fv_percents (const char *cpufamily, const char *cpumodel);
unsigned long  *get_cpu_fvs (const char *cpufamily, const char *cpumodel);

#ifdef ENERGY_TRACE_ENABLED
periph_class_t *get_cpu_etrace_class (const char *cpufamily, const char *cpumodel);
#endif

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
