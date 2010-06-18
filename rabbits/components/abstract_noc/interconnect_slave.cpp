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

#include <interconnect_slave.h>
#include <interconnect_master.h>

interconnect_slave::interconnect_slave (sc_module_name name, interconnect *parent, int srcid)
: sc_module (name)
{
    m_srcid = srcid;
    m_parent = parent;

    m_queue_requests = new mwsr_ta_fifo<vci_request> (8);
    m_queue_responses = new mwsr_ta_fifo<vci_response> (8);

    SC_THREAD (dispatch_responses_thread);
}

interconnect_slave::~interconnect_slave ()
{
    if (m_queue_requests)
        delete m_queue_requests;

    if (m_queue_responses)
        delete m_queue_responses;

}

//put interface
void interconnect_slave::put (vci_response &rsp)
{
    m_queue_responses->Write (rsp);
}

//get interface
void interconnect_slave::get (vci_request &req)
{	
    req = m_queue_requests->Read ();
}

void interconnect_slave::dispatch_responses_thread ()
{
    vci_response			rsp;
    interconnect_master		*master;

    while (1)
    {
        rsp = m_queue_responses->Read ();

        wait (1, SC_NS);

        master = m_parent->get_master (rsp.rsrcid);
        if (master == NULL)
        {
            printf ("Error (slaveid=%d): Cannot find the masterid %d!\n",
                    m_srcid, rsp.rsrcid);
            exit (1);
        }

        master->add_response (rsp);
    }
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
