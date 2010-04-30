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

slave_device::slave_device (sc_module_name module_name) : sc_module (module_name)
{
    SC_THREAD (request_thread);
}

slave_device::~slave_device ()
{
}

void slave_device::request_thread ()
{
    unsigned char           be, cmd;
    unsigned long           addr;
    vci_request             req;

    while (1)
    {
        get_port->get (req);

        if(bProcessing_rq)
        {
            fprintf(stdout, "Recieved a request while processing previous one: drop it\n");
            continue;
        }

        bProcessing_rq = true;

        addr   = req.address;
        be     = req.be;
        cmd    = req.cmd;

        rsp.rsrcid   = req.srcid;
        rsp.rtrdid   = req.trdid;
        rsp.reop     = 1;
        rsp.rbe      = be;

        switch(cmd){
        case CMD_WRITE:
            this->rcv_rqst (addr, be, req.wdata, 1);
            break;
        case CMD_READ:
            this->rcv_rqst (addr, be, rsp.rdata, 0);
            break;
        default:
            cerr << "Unknown command" << endl;
        }

    } // while(1)

    // Point Never Reached...
    return;
}


void
slave_device::send_rsp(bool bErr)
{
    rsp.rerror = bErr;

    put_port->put (rsp);

    bProcessing_rq = false;

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
