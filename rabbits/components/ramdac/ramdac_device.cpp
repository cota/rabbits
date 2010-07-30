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

#include <systemc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <ramdac_device.h>

extern unsigned long no_frames_to_simulate;

static int          pids_ramdac[10];
static int          nramdac = 0;

void close_ramdacs ()
{
    int         i, status;

    if (nramdac == 0)
        return;

    for (i = 0; i < nramdac; i++)
    {
        //cout << "Killing RAMDAC " << (int) i << endl;
        kill (pids_ramdac[i], SIGKILL);
        ::wait (&status);
    }

    nramdac = 0;
}

ramdac_device::ramdac_device (const char *_name) : slave_device (_name)
{
    ii = 0;
    nb_images = 0;
    width = 0;
    height = 0;
    m_ready = false;

    if (nramdac == 0)
        atexit (close_ramdacs);
}

ramdac_device::~ramdac_device ()
{
}

void ramdac_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    unsigned long           val1 = ((unsigned long*)data)[0];
    unsigned long           val2 = ((unsigned long*)data)[1];

    bErr = false;

    switch (ofs)
    {
    case 0:
        if (!m_ready)
        {
            receive_size (val1);
        }
        else
        {
            unsigned int * ptr;
            ptr = (unsigned int*) image[ii];
            ptr[m_y * width * components / 4 + m_x] = val1;
            m_x = (m_x + 1) % ((width / 4) * components);
            if (!m_x)
                m_y = (m_y + 1) % height;
            if (!m_x && !m_y)
            {
                nb_images++;
                display();
                //printf ("Image %d\n", nb_images);
                if (no_frames_to_simulate && nb_images == no_frames_to_simulate)
                {
                    exit (1);
                }
            }
        }

        break;
    case 0x880:
        break;
    default:
        printf ("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned long*)data + 0), (unsigned int) *((unsigned long*)data + 1));
        exit (1);
        break;
    }
}

void ramdac_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    bErr = false;
    return;
    printf ("Bad %s::%s ofs=0x%X, be=0x%X\n", name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);

}

void ramdac_device::receive_size (unsigned int data)
{
    if (width == 0)
    {
        width = data;
        printf ("width %d\n",width);
        return;
    }

    if(height == 0)
    {
        height = data;
        printf("height %d\n",height);
        return;
    }

    if (components == 0)
    {
        m_ready = true;
        m_x = m_y = 0;
        components = data;
        printf ("components %d\n",components);
        viewer ();
    }
}

void ramdac_device::viewer ()
{
    char            xname[80] = "*** The Screen ***";
    int             ppout[2];
    key_t           key[2];
    int             i, k;

    pipe(ppout);
    if (!(pid = fork ()))
    {
        setpgrp();
        close (0);
        dup (ppout[0]);
        if (execlp ("xramdac", "xramdac", NULL) == -1)
        {
            perror ("viewer: execlp failed");
            _exit (1);
        }
    }
    if (pid == -1)
    {
        perror("viewer: fork failed");
        exit(1);
    }

    pids_ramdac[nramdac++] = pid;

    pout = ppout[1];

    for (i = 0; i < 2; i++)
    {
        for (k = 1;; k++)
            if ((shmid[i] = shmget (k, components * width * height * sizeof (char), IPC_CREAT | IPC_EXCL | 0600)) != -1)
                break;
        key[i] = k;
        image[i] = (char *) shmat (shmid[i], 0, 00200);
        if (image[i] == (void *) -1)
        {
            perror ("ERROR: Ramdac.shmat");
            exit (1);
        }
    }

    ::write (pout, key, sizeof (key));
    ::write (pout, &width, sizeof (int));
    ::write (pout, &height, sizeof (int));
    ::write (pout, &components, sizeof (unsigned int));
    ::write (pout, xname, sizeof (xname));
}

void ramdac_device::display ()
{
    ii = (ii + 1) % 2;
    kill (pid, SIGUSR1);
}

void ramdac_device::rcv_rqst (unsigned long ofs, unsigned char be,
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
