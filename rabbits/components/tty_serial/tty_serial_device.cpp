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
#include <tty_serial_device.h>
#include <cfg.h>

//#define DEBUG_DEVICE_TTY_SERIAL

#ifdef DEBUG_DEVICE_TTY_SERIAL
#define DPRINTF(fmt, args...)                                           \
    do { fprintf(stderr, "DEBUG tty_serial_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

#define TTY_INT_READ        1

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

void tty_serial_device::read_thread ()
{
    fd_set              rfds;
    int                 pos, ret, nfds;
    unsigned char       ch;
    struct timeval      tv;

    nfds = pin + 1;

    while (1)
    {
        wait (10, SC_US);

        tv.tv_sec = 0;
        tv.tv_usec = 0;
        FD_ZERO (&rfds);
        FD_SET (pin, &rfds);
        ret = select (nfds, &rfds, NULL, NULL, &tv);
        if (ret > 0) 
        {
            if (FD_ISSET (pin, &rfds))
            {
                ::read (pin, &ch, 1);
                DPRINTF ("%s read 0x%x, %c\n", __FUNCTION__, (unsigned int) ch, ch);

                while (state.read_count == READ_BUF_SIZE)
                    wait (evRead);

                pos = (state.read_pos + state.read_count) % READ_BUF_SIZE;
                state.read_buf[pos] = ch;
                state.read_count++;

                state.int_level |= TTY_INT_READ;
                irq_update.notify ();
            }
        }
    }
}

tty_serial_device::tty_serial_device (sc_module_name _name) : slave_device (_name)
{
    int             ppout[2], ppin[2];
    char            spipeout[16], spipein[16];

    memset (&state, 0, sizeof (state));

    signal (SIGHUP, sig_hup);
    atexit (close_ttys);

    signal (SIGPIPE, SIG_IGN);

    if (pipe (ppout) < 0)
    {
        cerr << name () << " can't open out pipe!" << endl;
        exit (0);
    }
    if (pipe (ppin) < 0)
    {
        cerr << name () << " can't open in pipe!" << endl;
        exit (0);
    }

    pout = ppout[1];
    sprintf (spipeout, "%d", ppout[0]);
    pin = ppin[0];
    sprintf (spipein, "%d", ppin[1]);

    if (!(s_pid_tty[s_nb_tty++] = fork ()))
    {
        setpgrp();

        if (execlp ("xterm",
                    "xterm",
                    "-sb","-sl","1000",
                    "-l", "-lf", "logCPUs",
                    "-n", "Console", "-T", "Console",
                    "-e", "tty_term_rw", spipeout, spipein,
                    NULL) == -1)
        {
            perror ("tty_term: execlp failed!");
            _exit(1);
        }
    }

    SC_THREAD (read_thread);
    SC_THREAD (irq_update_thread);
}

tty_serial_device::~tty_serial_device ()
{
    close (pout);
    close_ttys ();
}

//void tty_serial_device::irq_update ()
void tty_serial_device::irq_update_thread ()
{
    unsigned long       flags;

    while(1) {

        wait(irq_update);

        flags = state.int_level & state.int_enabled;

        DPRINTF ("%s - %s\n", __FUNCTION__, (flags != 0) ? "1" : "0");
        
        irq_line = (flags != 0);
    }
}

void tty_serial_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    unsigned char               ch;
    unsigned long               value;

    bErr = false;

    ofs >>= 2;
    if (be & 0xF0)
    {
        ofs++;
        value = * ((unsigned long *) data + 1);
    }
    else
        value = * ((unsigned long *) data + 0);

    if (ofs != 0)
        DPRINTF ("%s to 0x%lx - value 0x%lx\n", __FUNCTION__, ofs, value);

    switch (ofs)
    {
    case 0: //write data
        ch = data[0];
        ::write (pout, &ch, 1);
        break;

    case 1: //set int enable
        state.int_enabled = value;
        irq_update.notify ();
        break;

    default:
    case_error_write:
        printf ("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned long*)data + 0), (unsigned int) *((unsigned long*)data + 1));
        exit (1);
    }
}

void tty_serial_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int                 i;
    unsigned long       c, *pdata;

    pdata = (unsigned long *) data;
    pdata[0] = 0;
    pdata[1] = 0;
    bErr = false;

    ofs >>= 2;
    if (be & 0xF0)
    {
        ofs++;
        pdata++;
    }

    switch (ofs)
    {
    case 0: //read char
        c = state.read_buf[state.read_pos];
        if (state.read_count > 0)
        {
            state.read_count--;
            if (++state.read_pos == READ_BUF_SIZE)
                state.read_pos = 0;

            if (0 == state.read_count)
            {
                state.int_level &= ~TTY_INT_READ;
                irq_update.notify ();
            }

            evRead.notify (0, SC_NS);
        }
        *pdata = c;
        break;

    case 1: //can write?
        *pdata = 1;
        break;

    case 2: //can read?
        *pdata = (state.read_count > 0) ? 1 : 0;
        break;

    case 3: //get int_enable
        *pdata = state.int_enabled;
        break;

    case 4: //get int_level
        *pdata = state.int_level;
        break;

    case 5: //active int
        *pdata = state.int_level & state.int_enabled;
        break;

    case 6:
        *pdata = 0;
        break;

    default:
    case_error_read:
        printf ("Bad %s::%s ofs=0x%X, be=0x%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
        exit (1);
    }

    if (ofs != 6)
        DPRINTF ("%s from 0x%lx - value 0x%lx\n", __FUNCTION__, ofs, *pdata);
}

void tty_serial_device::rcv_rqst (unsigned long ofs, unsigned char be,
                                  unsigned char *data, bool bWrite)
{

    bool bErr = false;

    if (bWrite)
        this->write(ofs, be, data, bErr);
    else
        this->read(ofs, be, data, bErr);

    send_rsp (bErr);
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
