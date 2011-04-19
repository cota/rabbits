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

#include <slave_device.h>

void invalidate_address (unsigned long addr, int slave_id,
        unsigned long offset_slave, int src_id);

slave_device::slave_device (sc_module_name module_name) : sc_module (module_name)
{
    m_write_invalidate = false;
    m_bProcessing_rq = false;
    SC_THREAD (request_thread);
}

slave_device::~slave_device ()
{
}

void slave_device::request_thread ()
{
    unsigned char           be, cmd;
    unsigned long           addr;

    while (1)
    {
        get_port->get (m_req);

        if(m_bProcessing_rq)
        {
            fprintf(stdout, "Recieved a request while processing previous one: drop it\n");
            continue;
        }

        m_bProcessing_rq = true;

        addr   = m_req.address;
        be     = m_req.be;
        cmd    = m_req.cmd;

        m_rsp.rsrcid   = m_req.srcid;
        m_rsp.rtrdid   = m_req.trdid;
        m_rsp.reop     = 1;
        m_rsp.rbe      = be;

        switch (cmd)
        {
        case CMD_WRITE:
            this->rcv_rqst (addr, be, m_req.wdata, true);
            break;
        case CMD_READ:
            this->rcv_rqst (addr, be, m_rsp.rdata, false);
            break;
        default:
            cerr << "Unknown command" << endl;
        } //switch (cmd)
    } // while(1)
}


void
slave_device::send_rsp (bool bErr)
{
    if (m_write_invalidate && m_req.cmd == CMD_WRITE)
        invalidate_address (m_req.initial_address,
            m_req.slave_id, m_req.initial_address, m_req.srcid);

    m_rsp.rerror = bErr;
    put_port->put (m_rsp);

    m_bProcessing_rq = false;
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
