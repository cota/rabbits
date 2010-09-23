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
#include <timer_device.h>
#include <cfg.h>

//#define DEBUG_DEVICE_TIMER

#ifdef DEBUG_DEVICE_TIMER
#define DPRINTF(fmt, args...)                               \
    do { printf("timer_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

#define TIMER_CLOCK_FV 1000000000

timer_device::timer_device (sc_module_name module_name) : slave_device (module_name)
{
    divisor = 0;
    ns_period = 0;
    divisor_changed = false;
    bOneShot = false;

    SC_THREAD (timer_thread);
}

timer_device::~timer_device ()
{
}

void timer_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    unsigned long           val1 = ((unsigned long*)data)[0];
    unsigned long           val2 = ((unsigned long*)data)[1];

    switch (ofs)
    {
    case 0x00:
        if (val1 != 0xFFFFFFFF)
        {
            divisor = val1;
            ns_period = ((double)1000000000) / TIMER_CLOCK_FV * divisor;
            divisor_changed = true;
            ev_wake.notify(SC_ZERO_TIME);
        }
        else
        {
            DCOUT << "HW " << name () << " ACK" << endl;
            ev_wake.notify();
        }
        break;

    case 0x08:
        bOneShot = (val1 != 0);
        break;

    default:
        printf ("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned long*)data + 0), (unsigned int) *((unsigned long*)data + 1));
        exit (1);
        break;
    }
    bErr = false;
}

void timer_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int             i;

    *((unsigned long *)data + 0) = 0;
    *((unsigned long *)data + 1) = 0;

    switch (ofs)
    {
    case 0x00:
        if (be == 0x0F)
        {
            *((unsigned long *) data + 0) = 1;
        }
        else
        {
            goto _err;
        }
        break;
    default:
        _err:
        printf ("Bad %s::%s ofs=0x%X, be=0x%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
        exit (1);
    }
    bErr = false;
}

void timer_device::timer_thread ()
{
    while (1)
    {
        if (divisor == 0)
            wait (ev_wake);
        else
            wait (ns_period, SC_NS, ev_wake);

        if (divisor_changed)
        {
            divisor_changed = false;
            irq = false;
        }
        else
        {
            DCOUT << "HW " << name () << ": set IRQ=1, time = " << sc_time_stamp () << endl;
            if (bOneShot)
                divisor = 0;
            irq = true;
            wait(ev_wake);
            irq = false;
        }
    }
}

void timer_device::rcv_rqst (unsigned long ofs, unsigned char be,
                             unsigned char *data, bool bWrite)
{
    bool bErr = false;

    if (bWrite)
        this->write (ofs, be, data, bErr);
    else
        this->read (ofs, be, data, bErr);

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
