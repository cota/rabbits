
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
#include <sl_tty_device.h>
#include <cfg.h>

//#define DEBUG_DEVICE_TTY

#ifdef DEBUG_DEVICE_TTY
#define DPRINTF(fmt, args...)                               \
    do { printf("sl_tty_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

static int      s_pid_tty[20];
static int      s_nb_tty = 0;

extern void close_ttys ();

void sig_hup (int)
{
    close_ttys ();
    kill (0, 9);
}

sl_tty_device::sl_tty_device (const char *_name, int ntty) : slave_device (_name)
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
            sprintf (slog, "%s%02d", _name, i);
            sprintf (sname, "CPU %d", i);

            if (execlp ("xterm",
                        "xterm",
                        "-sb","-sl","1000",
                        "-l", "-lf", slog,
                        "-n", sname, "-T", sname,
                        "-e",
                        "tty_term_rw",
                        spipe/*in*/, "1"/*out*/,
                        NULL) == -1)
            {
                perror ("sl_tty_term: execlp failed!");
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
        kill (s_pid_tty[i], SIGKILL);
        ::wait (&status);
    }

    s_nb_tty = 0;
}

sl_tty_device::~sl_tty_device ()
{
    int             i;
    for (i = 0; i < nb_tty; i++)
        close (pout[i]);

    close_ttys ();
}

void sl_tty_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    unsigned char           val, tty, be_ok = true;
    unsigned long           value;

    ofs >>= 2;
    if (be & 0xF0)
    {
        ofs++;
        value = * ((unsigned long *) data + 1);
    }
    else
        value = * ((unsigned long *) data + 0);


    tty = ofs / TTY_SPAN;
    ofs = ofs % TTY_SPAN;

    if(tty >= nb_tty){
        DPRINTF("(TTY too high) Bad %s::%s tty=%d ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                name (), __FUNCTION__, tty, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned long*)data + 0), (unsigned int) *((unsigned long*)data + 1));
        exit (1);
    }

    switch(ofs)
    {
    case 0: //TTY_WRITE
        DPRINTF("TTY_WRITE[%d]: 0x%x (%c)\n", tty, (char)value, (char) value);
        ::write (pout[tty], &value, 1);
        break;
        

    default:
        printf ("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned long*)data + 0), (unsigned int) *((unsigned long*)data + 1));
        exit (1);
    }
    bErr = false;
}

void sl_tty_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int             i;
    unsigned long               value;


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


void sl_tty_device::rcv_rqst (unsigned long ofs, unsigned char be,
                              unsigned char *data, bool bWrite)
{

    bool bErr = false;

    if(bWrite){
        this->write(ofs, be, data, bErr);
    }else{
        this->read(ofs, be, data, bErr);
    }

    send_rsp(bErr);

    return;
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
