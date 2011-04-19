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

#ifndef _INTERCONNECT_H
#define _INTERCONNECT_H

#include <abstract_noc.h>

class interconnect_master;
class interconnect_slave;

class interconnect : public sc_module
{
public:
    SC_HAS_PROCESS (interconnect);
    interconnect (sc_module_name name, int nmasters, int nslaves);
    ~interconnect ();

public:
    void connect_master_64 (int devid, sc_port<VCI_PUT_REQ_IF> &putp,
                            sc_port<VCI_GET_RSP_IF> &getp);
    void connect_slave_64 (int devid, sc_port<VCI_GET_REQ_IF> &getp,
                           sc_port<VCI_PUT_RSP_IF> &putp);

public:
    inline interconnect_master* get_master (int id)
    {
        return m_masters[id];
    }

    inline interconnect_slave* get_slave (int id)
    {
        return m_slaves[id];
    }

    inline int get_nmasters () {return m_nMasters; }
    inline int get_nslaves () {return m_nSlaves; }

private:
    void internal_init ();

protected:
    int                                 m_nMasters;
    interconnect_master                 **m_masters;
    int                                 m_nSlaves;
    interconnect_slave                  **m_slaves;
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
