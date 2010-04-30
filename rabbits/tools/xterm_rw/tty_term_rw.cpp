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

#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>

// #define DEBUG_TTY_TERM_RW
#ifdef DEBUG_TTY_TERM_RW
#define DPRINTF(fmt, args...)                                           \
    do { fprintf(stderr, "DEBUG - HW tty_term_rw: " fmt , ##args); } while (0)
#else
#define DPRINTF if (0) printf
#endif

static struct       termios oldtty;
static int          old_fd0_flags;

static void term_exit (void)
{
    tcsetattr (0, TCSANOW, &oldtty);
    fcntl (0, F_SETFL, old_fd0_flags);
}

static void term_init (void)
{
    struct termios          tty;

    tcgetattr (0, &tty);
    oldtty = tty;
    old_fd0_flags = fcntl (0, F_GETFL);

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_oflag |= OPOST;
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
    tty.c_lflag &= ~ISIG;
    tty.c_cflag &= ~(CSIZE | PARENB);
    tty.c_cflag |= CS8;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;
    tcsetattr (0, TCSANOW, &tty);

    atexit (term_exit);

    //    fcntl (0, F_SETFL, O_NONBLOCK);
}

int main (int argc, char * argv[])
{
    unsigned int        pipe_in, pipe_out;

    if (argc != 3)
    {
        fprintf (stderr, "usage: tty_term <input pipe> <output pipe>\n");
        exit (0);
    }

    pipe_in  = atoi (argv[1]);
    pipe_out = atoi (argv[2]);

    term_init ();

    fd_set              rfds;
    int                 ret, nfds;
    unsigned char       ch;

    nfds = pipe_in + 1;
    do
    {
        FD_ZERO (&rfds);
        FD_SET (pipe_in, &rfds);
        FD_SET (0, &rfds);

        ret = select (nfds, &rfds, NULL, NULL, NULL);
        if (ret > 0) 
        {
            if (FD_ISSET (0, &rfds))
            {
                if (1 == read (0, &ch, 1))
                {
                    DPRINTF ("read ch=0x%x, %c\n", (unsigned int) ch, ch);
                    write (pipe_out, &ch, 1);
                }
            }

            if (FD_ISSET (pipe_in, &rfds))
            {
                if (1 == read (pipe_in, &ch, 1))
                {
                    putchar (ch);
                    fflush (stdout);
                }
            }
        }
    }
    while (1);


    return 0;
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
