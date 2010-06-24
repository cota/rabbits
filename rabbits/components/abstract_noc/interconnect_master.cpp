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

#include <interconnect_master.h>
#include <interconnect_slave.h>
#include <mwsr_ta_fifo.h>

interconnect_master::interconnect_master (sc_module_name name, interconnect *parent, int srcid)
: sc_module (name)
{
    m_srcid = srcid;
    m_parent = parent;

    int				i, k;
    FILE			*file;
    char			file_name[256];

    sprintf (file_name, "%s/node%d.map", MAP_FILES_DIR, m_srcid);
    if ((file = fopen (file_name, "rt")) == NULL)
    {
        printf ("Error (masterid=%d): Cannot open the map file %s!\n", m_srcid, file_name);
        exit (1);
    }

    m_map = new interconnect_master_map_el[50];
    i = 0;
    while (1)
    {
        k = fscanf (file, "0x%lX 0x%lX 0x%lX %d\n", 
                    &m_map[i].begin_address, &m_map[i].end_address, &m_map[i].intern_offset, &m_map[i].slave_id);
        if (k <= 0)
            break;
        i++;
        if (k > 0 && k < 4)
        {
            printf ("Error (masterid=%d): Invalid map file %s, line %d!\n", m_srcid, file_name, i);
            exit (1);
        }
    }
    m_nmap = i;
    fclose (file);

    m_queue_responses = new mwsr_ta_fifo<vci_response> (8);
    m_queue_requests = new mwsr_ta_fifo<vci_request> (8);

    SC_THREAD (dispatch_requests_thread);
}

interconnect_master::~interconnect_master ()
{
    if (m_map)
        delete [] m_map;

    if (m_queue_responses)
        delete m_queue_responses;

    if (m_queue_requests)
        delete m_queue_requests;
}

//put interface
void interconnect_master::put (vci_request &req)
{
    m_queue_requests->Write (req);
}

//get interface
void interconnect_master::get (vci_response &rsp)
{
    rsp = m_queue_responses->Read ();
}

void interconnect_master::dispatch_requests_thread ()
{
    unsigned long			addr;
    int						i;
    vci_request				req;
    interconnect_slave		*slave;

    while (1)
    {
        req = m_queue_requests->Read ();

        wait (3, SC_NS);

        addr = req.address;
        for (i = 0; i < m_nmap; i++)
            if (addr >= m_map[i].begin_address && addr < m_map[i].end_address)
                break;
        if (i == m_nmap)
        {
            printf ("Error (masterid=%d): Cannot map the address 0x%lx to a slave!\n", m_srcid, addr);
            exit (1);
        }
        slave = m_parent->get_slave (m_map[i].slave_id);
        if (slave == NULL)
        {
            printf ("Error (masterid=%d): Cannot find the slaveid %d!\n", m_srcid, m_map[i].slave_id);
            exit (1);
        }

        req.initial_address = req.address;
        req.address = addr - m_map[i].begin_address + m_map[i].intern_offset;

        slave->add_request (req);
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
