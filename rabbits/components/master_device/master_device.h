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

#ifndef _MASTER_DEVICE_H_
#define _MASTER_DEVICE_H_

#include <abstract_noc.h>

class master_device : public sc_module
{
public:
    //constructor & destructor
    SC_HAS_PROCESS (master_device);
    master_device (sc_module_name module_name);
    virtual ~master_device ();

private:
    //sc threads
    void response_thread ();

public:
    void send_req(unsigned char tid, unsigned long addr,
                  unsigned char *data, unsigned char bytes, bool bWrite);

    // Master modules have to implement the recieve function ...
    virtual void rcv_rsp(unsigned char tid, unsigned char *data,
                         bool bErr, bool bWrite) = 0;

public:
    //ports
    sc_port<VCI_PUT_REQ_IF>                             put_port;
    sc_port<VCI_GET_RSP_IF>                             get_port;


public:
    unsigned int                                        m_node_id;


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
