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
#include "qemu_wrapper_slave.h"
#include "cfg.h"

#ifdef DEBUG_QEMU_WRAPPER_SLAVE
#define DPRINTF(fmt, args...)                               \
    do { printf("qemu_wrapper_slave_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

qemu_wrapper_slave_device::qemu_wrapper_slave_device (sc_module_name module_name)
    : slave_device (module_name)
{
}

qemu_wrapper_slave_device::~qemu_wrapper_slave_device ()
{
}

void qemu_wrapper_slave_device::set_master (qemu_wrapper *qemu_master)
{
    m_qemu_master = qemu_master;
}

void qemu_wrapper_slave_device::write (unsigned long ofs, unsigned char be,
    unsigned char *data, bool &bErr)
{
    uint32_t                value;

    ofs >>= 2;
    if (be & 0xF0)
    {
        ofs++;
        value = * ((uint32_t *) data + 1);
    }
    else
        value = * ((uint32_t *) data + 0);

    switch (ofs)
    {
    case QEMU_WRAPPER_SLAVE_SET_CPUS_FQ:
        DPRINTF("QEMU_WRAPPER_SLAVE_SET_CPUS_FQ write: 0x%08x\n",
            mailbox, value);
        m_qemu_master->set_cpu_fv_level (-1, value);
        break;
    default:
        DPRINTF ("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                 name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                 (unsigned int) *((unsigned long*)data + 0),
                 (unsigned int) *((unsigned long*)data + 1));
        exit (1);
        break;
    }

    bErr = false;
}

void qemu_wrapper_slave_device::read (unsigned long ofs, unsigned char be,
    unsigned char *data, bool &bErr)
{
    uint32_t  *val = (uint32_t *)data;

    ofs >>= 2;
    if (be & 0xF0)
    {
        ofs++;
        val++;
    }

    *val = 0;

    switch (ofs)
    {
    default:
        printf ("Bad %s::%s ofs=0x%X, be=0x%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
        exit (1);
    }
    bErr = false;
}

void qemu_wrapper_slave_device::rcv_rqst (unsigned long ofs, unsigned char be,
    unsigned char *data, bool bWrite)
{

    bool bErr = false;

    if(bWrite)
        this->write(ofs, be, data, bErr);
    else
        this->read(ofs, be, data, bErr);

    send_rsp(bErr, 0);
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
