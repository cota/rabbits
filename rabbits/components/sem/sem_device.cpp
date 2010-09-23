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
#include <sem_device.h>

//#define DEBUG_DEVICE_SRAM

#ifdef DEBUG_DEVICE_SRAM
#define DPRINTF(fmt, args...)                   \
    do { printf(fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

sem_device::sem_device (const char *_name, unsigned long _size) : slave_device (_name)
{
    mem = NULL;
    size = _size + 8;
    mem = new unsigned char [size];
    memset (mem, 0, size);
}

sem_device::~sem_device ()
{
    if (mem)
        delete [] mem;
}

void sem_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int                 offset_dd = 0;
    int                 mod = 0;
    int                 err = 0;

#ifdef DEBUG_DEVICE_SRAM
    unsigned long long  ns_tm = sc_time_stamp ().value () / 1000;
#endif

    bErr = false;

    if (ofs > size)
        err = 1;

    if (!err)
    {
        switch (be)
        {
            //byte access
        case 0x01: offset_dd = 0; mod = 1; break;
        case 0x02: offset_dd = 1; mod = 1; break;
        case 0x04: offset_dd = 2; mod = 1; break;
        case 0x08: offset_dd = 3; mod = 1; break;
        case 0x10: offset_dd = 4; mod = 1; break;
        case 0x20: offset_dd = 5; mod = 1; break;
        case 0x40: offset_dd = 6; mod = 1; break;
        case 0x80: offset_dd = 7; mod = 1; break;
            //word access
        case 0x03: offset_dd = 0; mod = 2; break;
        case 0x0C: offset_dd = 1; mod = 2; break;
        case 0x30: offset_dd = 2; mod = 2; break;
        case 0xC0: offset_dd = 3; mod = 2; break;
            //dword access
        case 0x0F: offset_dd = 0; mod = 4; break;
        case 0xF0: offset_dd = 1; mod = 4; break;
        }
        if (mod == 1)
        {
            *((unsigned char*) (mem + ofs) + offset_dd) = *((unsigned char*) data + offset_dd);
        }
        else
            if (mod == 2)
            {
                *((unsigned short*) (mem + ofs) + offset_dd) = *((unsigned short*) data + offset_dd);
            }
            else
                if (mod == 4)
                {
                    unsigned long newval = *((unsigned long*) data + offset_dd);
                    DPRINTF ("sem_write [%X]<-%lu (tm = %llu ns)\n", ofs + offset_dd * 4, newval, ns_tm);

                    if (newval == 1)
                    {
                        unsigned long oldval = *((unsigned long*) (mem + ofs) + offset_dd);

                        if (oldval == 1)
                            newval = 2;
                        else
                            if (oldval == 2)
                            {
                                printf ("%s:%s sem double locking!\n", name (), __FUNCTION__);
                                exit (1);
                            }
                    }
                    *((unsigned long*) (mem + ofs) + offset_dd) = newval;
                }
                else
                    err = 1;
    }

    if (err)
    {
        printf ("Bad %s:%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned long*)data + 0), (unsigned int) *((unsigned long*)data + 1));
        exit (1);
    }
}

void sem_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int                 offset_dd = 0;
    int                 mod = 0;
    int                 err = 0;

#ifdef DEBUG_DEVICE_SRAM
    unsigned long long  ns_tm = sc_time_stamp ().value () / 1000;
#endif

    bErr = false;
    *((unsigned long *)data + 0) = 0;
    *((unsigned long *)data + 1) = 0;

    if (ofs > size)
        err = 1;

    if (!err)
    {
        switch (be)
        {
            //byte access
        case 0x01: offset_dd = 0; mod = 1; break;
        case 0x02: offset_dd = 1; mod = 1; break;
        case 0x04: offset_dd = 2; mod = 1; break;
        case 0x08: offset_dd = 3; mod = 1; break;
        case 0x10: offset_dd = 4; mod = 1; break;
        case 0x20: offset_dd = 5; mod = 1; break;
        case 0x40: offset_dd = 6; mod = 1; break;
        case 0x80: offset_dd = 7; mod = 1; break;
            //word access
        case 0x03: offset_dd = 0; mod = 2; break;
        case 0x0C: offset_dd = 1; mod = 2; break;
        case 0x30: offset_dd = 2; mod = 2; break;
        case 0xC0: offset_dd = 3; mod = 2; break;
            //dword access
        case 0x0F: offset_dd = 0; mod = 4; break;
        case 0xF0: offset_dd = 1; mod = 4; break;
        }
        if (mod == 1)
        {
            unsigned char val = *((unsigned char*) (mem + ofs) + offset_dd);
            *((unsigned char*) data + offset_dd) = val;
        }
        else
            if (mod == 2)
            {
                unsigned short val = *((unsigned short*) (mem + ofs) + offset_dd);
                *((unsigned short*) data + offset_dd) = val;
            }
            else
                if (mod == 4)
                {
                    unsigned long val = *((unsigned long*) (mem + ofs) + offset_dd);
                    DPRINTF ("sem_read [%X]->%lu (tm = %llu ns)\n", ofs + offset_dd * 4, val, ns_tm);
                    *((unsigned long*) data + offset_dd) = (val == 2) ? 1 : val;
                    if (val == 0)
                        *((unsigned long*) (mem + ofs) + offset_dd) = 1;
                }
                else
                    err = 1;
    }

    if (err)
    {
        printf ("Bad %s:%s ofs=0x%X, be=0x%X!\n", name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
        exit (1);
    }
}

void sem_device::rcv_rqst (unsigned long ofs, unsigned char be,
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
