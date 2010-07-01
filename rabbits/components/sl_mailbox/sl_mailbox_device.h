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

#ifndef _SL_MAILBOX_DEVICE_H_
#define _SL_MAILBOX_DEVICE_H_

#include <slave_device.h>

enum sl_mailbox_registers {
    MAILBOX_COMM_ADR	= 0,
    MAILBOX_DATA_ADR	= 1,
    // free addr	= 2
    MAILBOX_RESET_ADR	= 3,
    MAILBOX_SPAN    	= 4,
};

enum sl_mailbox_status_register{
    MAILBOX_CLEAR       = 0,
    MAILBOX_NEW_MESSAGE = 1,
};

class sl_mailbox_device : public slave_device
{
public:
    SC_HAS_PROCESS (sl_mailbox_device);
    sl_mailbox_device (sc_module_name module_name, int nb_mailbox);
    virtual ~sl_mailbox_device ();

public:
    /*
     *   Obtained from father
     *   void send_rsp (bool bErr);
     */
    virtual void rcv_rqst (unsigned long ofs, unsigned char be,
                           unsigned char *data, bool bWrite);

private:
    void write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr);
    void read  (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr);

public:
    //ports
    sc_out<bool>        *irq;

private:
    uint32_t            m_nb_mailbox;

    uint32_t            *m_command;
    uint32_t            *m_data;
    uint32_t            *m_status;
};

#endif

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
