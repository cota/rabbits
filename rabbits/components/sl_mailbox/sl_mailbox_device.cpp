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
#include <sl_mailbox_device.h>
#include <cfg.h>

//#define DEBUG_DEVICE_MAILBOX

#ifdef DEBUG_DEVICE_MAILBOX
#define DPRINTF(fmt, args...)                               \
    do { printf("sl_mailbox_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

sl_mailbox_device::sl_mailbox_device (sc_module_name module_name, int nb_mailbox) : slave_device (module_name)
{
    int i = 0;

    irq = new sc_out<bool>[nb_mailbox];

    m_nb_mailbox = nb_mailbox;

    m_command    = new uint32_t[nb_mailbox];
    m_data       = new uint32_t[nb_mailbox];
    m_status     = new uint32_t[nb_mailbox];

    for(i = 0; i < nb_mailbox; i++){
        m_command[i] = 0;
        m_data[i]    = 0;
        m_status[i]  = 0;
    }

}

sl_mailbox_device::~sl_mailbox_device ()
{
}

void sl_mailbox_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    uint32_t                value;
    uint32_t mailbox;
    int lcl_ofs;

    ofs >>= 2;
    if (be & 0xF0)
    {
        ofs++;
        value = * ((uint32_t *) data + 1);
    }
    else
        value = * ((uint32_t *) data + 0);

    mailbox = ofs / MAILBOX_SPAN;
    lcl_ofs = ofs % MAILBOX_SPAN;

    switch (lcl_ofs)
    {

    case MAILBOX_COMM_ADR:
        DPRINTF("MAILBOX_COMM_ADR[%d] write: 0x%08x\n", mailbox, value);
        m_command[mailbox] = value;
        m_status[mailbox]  = MAILBOX_NEW_MESSAGE;
        irq[mailbox]       = true;
        break;

    case MAILBOX_DATA_ADR:
        DPRINTF("MAILBOX_DATA_ADR[%d] write: 0x%08x\n", mailbox, value);
        m_data[mailbox]  = value;
        break;

    case MAILBOX_RESET_ADR:
        DPRINTF("MAILBOX_RESET_ADR[%d] write: 0x%x\n", mailbox, value);
        m_status[mailbox] = MAILBOX_CLEAR;
        irq[mailbox] = false;
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

void sl_mailbox_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int        i;
    int        mailbox = 0;
    uint32_t  *val = (uint32_t *)data;
    int        lcl_ofs = 0; 

    ofs >>= 2;
    if (be & 0xF0)
    {
        ofs++;
        val++;
    }

    *val = 0;

    mailbox = ofs / MAILBOX_SPAN;
    lcl_ofs = ofs % MAILBOX_SPAN;

    switch (lcl_ofs)
    {
    case MAILBOX_COMM_ADR:
        *val = m_command[mailbox];
        DPRINTF("MAILBOX_COMM_ADR[%d] read: 0x%08x\n", mailbox, *val);
        break;

    case MAILBOX_DATA_ADR:
        *val = m_data[mailbox];
        DPRINTF("MAILBOX_DATA_ADR[%d] read: 0x%08x\n", mailbox, *val);
        break;

    case MAILBOX_RESET_ADR:
        *val = m_status[mailbox];
        DPRINTF("MAILBOX_RESET_ADR[%d] read: 0x%x\n", mailbox, *val);
        break;

    default:
        printf ("Bad %s::%s ofs=0x%X, be=0x%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
        exit (1);
    }
    bErr = false;
}

void sl_mailbox_device::rcv_rqst (unsigned long ofs, unsigned char be,
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
