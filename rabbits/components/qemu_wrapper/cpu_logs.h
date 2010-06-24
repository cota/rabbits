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

#ifndef _LOGS_H_
#define _LOGS_H_

#include <cpu_fvs.h>

class cpu_logs
{
public:
    cpu_logs (int ncpu, const char *cpufamily, const char *cpumodel);
    ~cpu_logs ();

public:
    void add_time_at_fv (int cpu, int fv_level, unsigned long long time);
    unsigned long get_cpu_ncycles (unsigned long cpu);
    void update_fv_grf ();

protected:
    void internal_init ();

protected:
    int					m_ncpu;
    unsigned long long	*m_ns_time_at_fv;
    unsigned long long	*m_ns_time_at_fv_prev;
    unsigned long		*m_hword_ncycles;
    FILE				*file_fv;
    int					m_pipe_grf_run_at_fv;
    int					m_pid_grf_run_at_fv;
    int                 m_cpu_nb_fv_levels_1;

public:
    int                 m_cpu_nb_fv_levels;
    double              *m_cpu_fv_percents;
    unsigned long       *m_cpu_fvs;
    int                 m_cpu_boot_fv_level;
    double              m_cycles_max_fv_per_ns;
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
