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

#include <interconnect.h>
#include <interconnect_master.h>
#include <interconnect_slave.h>
#include <string.h>

interconnect::interconnect (sc_module_name name, int nmasters, int nslaves)
: sc_module (name)
{
    m_nMasters = nmasters;
    m_nSlaves = nslaves;

    internal_init ();
}

interconnect::~interconnect ()
{
    int				i;
    if (m_masters)
    {
        for (i = 0; i < m_nMasters; i++)
            delete m_masters[i];
        delete [] m_masters;
    }
    if (m_slaves)
    {
        for (i = 0; i < m_nSlaves; i++)
            delete m_slaves[i];
        delete [] m_slaves;
    }
}

void interconnect::internal_init ()
{
    int				i;
    char			s[50];

    m_masters = new interconnect_master* [m_nMasters];
    for (i = 0; i < m_nMasters; i++)
    {
        sprintf (s, "NOC_master_%02d", i);
        m_masters[i] = new interconnect_master (s, this, i);
    }

    m_slaves = new interconnect_slave* [m_nSlaves];
    for (i = 0; i < m_nSlaves; i++)
    {
        sprintf (s, "NOC_slave_%02d", i);
        m_slaves[i] = new interconnect_slave (s, this, i);
    }	
}

void interconnect::connect_master_64 (int devid, sc_port<VCI_PUT_REQ_IF> &putp, 
                                      sc_port<VCI_GET_RSP_IF> &getp)
{
    if (devid < 0 || devid >= m_nMasters)
    {
        printf ("Wrong devid %d for %s!\n", devid, __FUNCTION__);
        exit (1);
    }

    putp (*m_masters[devid]);
    getp (*m_masters[devid]);
}

void interconnect::connect_slave_64 (int devid, sc_port<VCI_GET_REQ_IF> &getp,
                                     sc_port<VCI_PUT_RSP_IF> &putp)
{
    if (devid < 0 || devid >= m_nSlaves)
    {
        printf ("Wrong devid %d for %s!\n", devid, __FUNCTION__);
        exit (1);
    }

    getp (*m_slaves[devid]);
    putp (*m_slaves[devid]);
}


ostream &operator<<(ostream& output, const vci_request& value)
{
    output << "The fields of the request transactions are : " << endl;
    output << "------------------------------------------------- " << endl;
    output << "address is " << std::hex << value.address << endl;
    output << "cmd is " << std::hex << value.cmd << endl;
    output << "srcid is " << (unsigned int) value.srcid << endl;
    output << "trdid is " <<(unsigned int) value.trdid << endl;
    output << "be is " << std::hex << (unsigned int) value.be << endl;
    output << "wdata ";
    for (unsigned int j(0);j< 8;++j)
        output << std::hex << (unsigned int) value.wdata[j] << '\n';
    output << endl;
    output << "------------------------------------------------- " << endl;

    return output;
}

ostream &operator<<(ostream& output, const vci_response& value)
{
    output << "The fields of the response transactions are : " << endl;
    output << "------------------------------------------------- " << endl;
    output << "rerror is " << value.rerror << endl;
    output << "rsrcid is " << (unsigned int) value.rsrcid << endl;
    output << "rtrdid is " <<(unsigned int) value.rtrdid << endl;
    output << "rbe is " << std::hex << (unsigned int) value.rbe << endl;
    output << "rdata ";
    for (unsigned int j(0);j< 8;++j)
        output << std::hex << (unsigned int) value.rdata[j] << '\n';
    output << endl;
    output << "------------------------------------------------- " << endl;

    return output;
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
