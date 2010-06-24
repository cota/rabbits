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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <tty_device.h>
#include <cfg.h>

#define DEBUG_DEVICE_TTY

#ifdef DEBUG_DEVICE_TTY
#define DPRINTF(fmt, args...)                               \
    do { printf("tty_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

static int      s_pid_tty[20];
static int      s_nb_tty = 0;

static void close_ttys ()
{
    int         i, status;

    if (s_nb_tty == 0)
        return;

    for (i = 0; i < s_nb_tty; i++)
    {
        kill (s_pid_tty[i], SIGKILL);
        ::wait (&status);
    }

    s_nb_tty = 0;
}

void sig_hup (int)
{
    exit (1);
}

tty_device::tty_device (const char *_name, int ntty) : slave_device (_name)
{
    int             i, ppout[2];
    char            spipe[16], slog[50], sname[50];

    nb_tty = ntty;
    pout = new int[nb_tty];

    signal (SIGHUP, sig_hup);
    atexit (close_ttys);

    for (i = 0; i < nb_tty; i++)
    {
        if (pipe (ppout) < 0)
        {
            cerr << name () << " can't open pipe!" << endl;
            exit(0);
        }

        pout[i] = ppout[1];
        sprintf (spipe, "%d", ppout[0]);

        if (!(s_pid_tty[s_nb_tty++] = fork ()))
        {
            setpgrp();
            sprintf (slog, "logCPU%02d", i);
            sprintf (sname, "CPU %d", i);

            if (execlp ("xterm",
                        "xterm",
                        "-sb","-sl","1000",
                        "-l", "-lf", slog,
                        "-n", sname, "-T", sname,
                        "-e",
                        "tty_term",
                        spipe,
                        NULL) == -1)
            {
                perror ("tty_term: execlp failed!");
                exit(1);
            }
        }
    }
}

void close_ttys ()
{
    int         i, status;

    if (s_nb_tty == 0)
        return;

    for (i = 0; i < s_nb_tty; i++)
    {
        //cout << "Killing TTY " << (int) i << endl;
        kill (s_pid_tty[i], SIGKILL);
        ::wait (&status);
    }

    s_nb_tty = 0;
}

tty_device::~tty_device ()
{
    int             i;
    for (i = 0; i < nb_tty; i++)
        close (pout[i]);

    close_ttys ();
}

void tty_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    unsigned char           val, tty, be_ok = true;

    switch (be)
    {
    case 0x01: tty = 0; break;
    case 0x02: tty = 1; break;
    case 0x04: tty = 2; break;
    case 0x08: tty = 3; break;
    case 0x10: tty = 4; break;
    case 0x20: tty = 5; break;
    case 0x40: tty = 6; break;
    case 0x80: tty = 7; break;
    default:
        be_ok = false;
    }

    //    printf ("tty_device ofs=%lu,be=%d, val=%d\n", ofs, (int) be, (int) val);

    if (ofs == 0 && be_ok)
    {
        val = data[tty];
        ::write (pout[tty], &val, 1);
    }
    else
    {
        printf ("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned long*)data + 0), (unsigned int) *((unsigned long*)data + 1));
        exit (1);
    }
    bErr = false;
}

void tty_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int             i;

    *((unsigned long *)data + 0) = 0;
    *((unsigned long *)data + 1) = 0;

    switch (ofs)
    {
    default:
        printf ("Bad %s::%s ofs=0x%X, be=0x%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
        exit (1);
    }
    bErr = false;
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
