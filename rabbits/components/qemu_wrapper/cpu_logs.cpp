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

#include <cfg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cpu_logs.h>

//#define TIME_AT_FV_LOG_FILE

#define FILE_TIME_AT_FV "logTimeAtFv"

#define macro_ns_time_at_fv ((unsigned long long (*)[m_cpu_nb_fv_levels_1]) m_ns_time_at_fv)
#define macro_ns_time_at_fv_prev ((unsigned long long (*)[m_cpu_nb_fv_levels_1]) m_ns_time_at_fv_prev)

static int      s_pid_chrono_cpu_fvs[20];
static int      s_nb_chrono_cpu_fvs = 0;

static inline void writepipe (int &pipe, void* addr, int nbytes)
{
    if (pipe != 0 && nbytes != ::write (pipe, addr, nbytes))
        pipe = 0;
}

cpu_logs::cpu_logs (int ncpu, const char *cpufamily, const char *cpumodel)
{
    m_ncpu = ncpu;

    m_cpu_nb_fv_levels  = get_cpu_nb_fv_levels (cpufamily, cpumodel);
    m_cpu_nb_fv_levels_1 = m_cpu_nb_fv_levels + 1;
    m_cpu_boot_fv_level = get_cpu_boot_fv_level (cpufamily, cpumodel);
    m_cpu_fv_percents   = get_cpu_fv_percents (cpufamily, cpumodel);
    m_cpu_fvs           = get_cpu_fvs (cpufamily, cpumodel);
    m_cycles_max_fv_per_ns = m_cpu_fvs[m_cpu_nb_fv_levels - 1] / ((double) 1000);

    internal_init ();
}

cpu_logs::~cpu_logs ()
{
    if (m_ns_time_at_fv)
    {
        delete [] m_ns_time_at_fv;
        m_ns_time_at_fv = NULL;
    }

    if (m_ns_time_at_fv_prev)
    {
        delete [] m_ns_time_at_fv_prev;
        m_ns_time_at_fv_prev = NULL;
    }

    if (m_hword_ncycles)
    {
        delete [] m_hword_ncycles;
        m_hword_ncycles = NULL;
    }

    #ifdef TIME_AT_FV_LOG_FILE
    if (file_fv)
    {
        fclose (file_fv);
        file_fv = NULL;
    }
    #endif
}

void close_chrono_cpu_fvs ()
{
    int         i, status;

    if (!s_nb_chrono_cpu_fvs)
        return;

    for (i = 0; i < s_nb_chrono_cpu_fvs; i++)
    {
        kill (s_pid_chrono_cpu_fvs[i], SIGKILL);
        ::wait (&status);
    }

    s_nb_chrono_cpu_fvs = 0;
}

void cpu_logs::internal_init ()
{
    int					i, cpu;

    m_ns_time_at_fv = new unsigned long long [m_ncpu * m_cpu_nb_fv_levels_1];
    m_ns_time_at_fv_prev = new unsigned long long [m_ncpu * m_cpu_nb_fv_levels_1];
    m_hword_ncycles = new unsigned long [m_ncpu];

    for (cpu = 0; cpu < m_ncpu; cpu++)
    {
        for (i = 0; i < m_cpu_nb_fv_levels_1; i++)
        {
            macro_ns_time_at_fv[cpu][i] = 0;
            macro_ns_time_at_fv_prev[cpu][i] = 0;
        }
        m_hword_ncycles[cpu] = (unsigned long) -1;
    }
    m_pipe_grf_run_at_fv = 0;
    m_pid_grf_run_at_fv = 0;
    file_fv = NULL;

    #ifdef TIME_AT_FV_LOG_FILE
    //"time at fv" file
    if ((file_fv = fopen (FILE_TIME_AT_FV, "w")) == NULL)
    {
        printf ("Cannot open log %s", FILE_TIME_AT_FV);
        exit (1);
    }
    tm				ct;
    time_t			tt;
    time (&tt);
    localtime_r (&tt, &ct);
    fprintf (file_fv, "\nSimulation start time=\t%02d-%02d-%04d %02d:%02d:%02d\n",
             ct.tm_mday, ct.tm_mon + 1, ct.tm_year + 1900, ct.tm_hour, ct.tm_min, ct.tm_sec);
    fprintf (file_fv, 
             "                       \t           Fmax\t       Fmax*3/4\t"
             "         Fmax/2\t         Fmax/4\t              0\n");
    #endif

    #ifdef TIME_AT_FV_LOG_GRF
    //"time at fv" grf
    int					apipe[2] = {0, 0};
    char				*ps, s[100];
    unsigned long		val;
    long long			vall;

    pipe (apipe);
    if ((m_pid_grf_run_at_fv = fork()) == 0)
    {
        setpgrp();

        close (0);
        dup (apipe[0]);
        close (apipe[0]);
        close (apipe[1]);

        int			fdnull = open ("/dev/null", O_WRONLY);
        close (1);
        dup2 (1, fdnull);
//        close (2);
//        dup2 (2, fdnull);
        close (fdnull);

        if (execlp ("chronograph", "chronograph", NULL) == -1)
        {
            perror ("chronograph: execlp failed");
            _exit(1);
        }
    }

    signal (SIGPIPE, SIG_IGN);
    close (apipe[0]);
    m_pipe_grf_run_at_fv = apipe[1];
    s_pid_chrono_cpu_fvs[s_nb_chrono_cpu_fvs++] = m_pid_grf_run_at_fv;

    atexit (close_chrono_cpu_fvs);

    // (4 (for string length) + (strlen + 1)) title of the graphs window
    ps = (char *) "Average frequency";
    val = strlen (ps) + 1;
    writepipe (m_pipe_grf_run_at_fv, &val, 4);
    writepipe (m_pipe_grf_run_at_fv, ps, val);

    val = m_ncpu | 0x80;
    // (4) the number of graphs (m_ncpu + the avg (MSb=1))
    writepipe (m_pipe_grf_run_at_fv, &val, 4);
    // (4) number of ms between 2 consecutive updates of the graphs
    val = 20;
    writepipe (m_pipe_grf_run_at_fv, &val, 4);

    for (i = 0; i < m_ncpu; i++)
    {
        //for each graph (except avg, if the case)
        // (4) number of curves in the crt graph
        val = 1;
        writepipe (m_pipe_grf_run_at_fv, &val, 4);
        // (8) minimum value possible in the crt graph
        vall = 0;
        writepipe (m_pipe_grf_run_at_fv, &vall, 8);
        // (8) maximum value possible in the crt graph
        vall = 100;
        writepipe (m_pipe_grf_run_at_fv, &vall, 8);
    }

    for (i = 0; i  <= m_ncpu; i++)
    {
        //for each graph (including the avg, if the case)
        if (i == m_ncpu)
            strcpy (s, "All CPUs (%)");
        else
            sprintf (s, "CPU %4d (%%)", i);
        val = strlen (s) + 1;
        // (4 + strlen  + 1) name of the graph (including measuring units)
        writepipe (m_pipe_grf_run_at_fv, &val, 4);
        writepipe (m_pipe_grf_run_at_fv, s, val);
    }
    #endif
}

void cpu_logs::add_time_at_fv (int cpu, int fv_level, unsigned long long time)
{
    macro_ns_time_at_fv[cpu][fv_level] += time;
}

unsigned long cpu_logs::get_cpu_ncycles (unsigned long cpu)
{
    unsigned long		ret;
    if (m_hword_ncycles[cpu] == (unsigned long)-1)
    {
        unsigned long long	sum = 0;
        for (int i = 0; i < m_cpu_nb_fv_levels_1; i++)
            sum += (unsigned long long) (macro_ns_time_at_fv[cpu][i] * 
                m_cpu_fv_percents[i] * m_cycles_max_fv_per_ns);
        ret = sum & 0xFFFFFFFF;
        m_hword_ncycles[cpu] = sum >> 32;
    }
    else
    {
        ret = m_hword_ncycles[cpu];
        m_hword_ncycles[cpu] = (unsigned long)-1;
    }

    return ret;
}

void cpu_logs::update_fv_grf ()
{
    int						i, cpu;

    #ifdef TIME_AT_FV_LOG_FILE
    //"time at fv" file
    static int			cnt1 = 0;
    if (++cnt1 == 100)
    {
        cnt1 = 0;

        fprintf (file_fv, "time(us) = %llu\n", sc_time_stamp ().value () / 1000000);
        for (cpu = 0; cpu < m_ncpu; cpu++)
        {
            fprintf (file_fv, "CPU %2d          \t", cpu);
            for (i = 0; i < m_cpu_nb_fv_levels_1; i++)
                fprintf (file_fv, "%15llu\t", 
                    macro_ns_time_at_fv[cpu][m_cpu_nb_fv_levels_1 - 1 - i] / 1000);
            fprintf (file_fv, "\n");
        }
    }	
    #endif

    #ifdef TIME_AT_FV_LOG_GRF
    //"time at fv" grf
    if (m_pipe_grf_run_at_fv)
    {
        double					s1, s2, diff;
        unsigned char			val[32];
        for (cpu = m_ncpu - 1; cpu >= 0; cpu--)
        {
            val[cpu] = 0;
            s1 = 0;
            s2 = 0;

            for (i = 0; i < m_cpu_nb_fv_levels_1; i++)
            {
                diff = macro_ns_time_at_fv[cpu][i] - macro_ns_time_at_fv_prev[cpu][i];
                s1 += diff * m_cpu_fv_percents[i];
                s2 += diff;
            }

            if (s2)
                val[cpu] = (unsigned long) (s1 / s2);
        }
        writepipe (m_pipe_grf_run_at_fv, val, m_ncpu);
        memcpy (macro_ns_time_at_fv_prev, macro_ns_time_at_fv,
            m_ncpu * m_cpu_nb_fv_levels_1 * sizeof (unsigned long long));
    }
    #endif
}

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
