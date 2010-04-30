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

#ifndef _INTERCONNECT_SLAVE_H_
#define _INTERCONNECT_SLAVE_H_

#include <interconnect.h>
#include <mwsr_ta_fifo.h>

class interconnect_slave : public sc_module, public VCI_GET_REQ_IF, public VCI_PUT_RSP_IF
{
public:
    SC_HAS_PROCESS (interconnect_slave);
    interconnect_slave (sc_module_name name, interconnect *parent, int srcid);
    ~interconnect_slave ();

public:
    inline void add_request (vci_request &req)
    {
        m_queue_requests->Write (req);
    }

public:
    //get interface
    virtual void get (vci_request&);
    //put interface
    virtual void put (vci_response&);

private:
    //thread
    void dispatch_responses_thread ();

public:
    int                         m_srcid;
    interconnect               *m_parent;
    mwsr_ta_fifo<vci_request>  *m_queue_requests;
    mwsr_ta_fifo<vci_response> *m_queue_responses;

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
