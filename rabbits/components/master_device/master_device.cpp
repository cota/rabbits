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

#include <master_device.h>

#define QEMU_NOC_ADDRESS_DIFFERENCE 0x00000000
//static unsigned char s_operation_codes[] = {0xDE, 0x00, 0x10, 0xDE, 0x20, 0xDE, 0xDE, 0xDE, 0x30};

static unsigned char s_operation_mask_be[] = {0xDE, 0x01, 0x03, 0xDE, 0x0F, 0xDE, 0xDE, 0xDE, 0xFF};


master_device::master_device (sc_module_name module_name) : sc_module (module_name)
{
    SC_THREAD (response_thread);
}

master_device::~master_device (void)
{
}

void master_device::response_thread ()
{
    vci_response                    resp;
    unsigned char                   tid, ropc, rerror;
    unsigned char be;
    unsigned char data[8];

    for(;;)
    {
        int ofs = 0;
        int nbytes = 0;
        int i = 0;



        get_port->get (resp);
        //ropc = resp.controls.r_opcode;
        tid = resp.rtrdid;
        rerror = resp.rerror;

        //COUT_TIMES << sc_time_stamp ().value () << " GET After - " << name () << endl;

        if (resp.rsrcid != m_node_id)
        {
            cout << "[Error: " << name () <<  " (id = " << m_node_id
                 << ") has received a response for " <<  resp.rsrcid
                 << "]" << endl;
            continue;
        }

        if (resp.reop == 0)
        {
            cout << "[Error: " << name () <<  " (id = " << m_node_id << 
                ") has received a response without EOP set]" << endl;
            continue;
        }

        if (rerror)
            cout << "[Error in " << name () <<  endl << resp << "]" << endl;

        be = resp.rbe;

        if (be)
        {
            while (!(be & 0x1)){
                ofs++;
                be >>= 1;
            }

            while (be & 0x1){
                nbytes++;
                be >>= 1;
            }

            for (i = 0; i < nbytes; i++){
                data[i] = resp.rdata[i + ofs];
            }
        }

        for (i = 0; i < nbytes; i++){
            data[i] = resp.rdata[i + ofs];
        }

        // ... rcv_rsp function () ...
        rcv_rsp(tid, data, rerror, 0);

    }

    return;

}

void master_device::send_req(unsigned char tid, unsigned long addr,
                             unsigned char *data, unsigned char bytes, bool bWrite)
{

    vci_request req;
    // compute be and data from nbytes.
    int i;

    unsigned char           ofs, mask_be;
    ofs = addr & 0x000000007;
    addr &= 0xFFFFFFF8;
    mask_be = s_operation_mask_be[bytes] << ofs;

    req.cmd     = (bWrite?CMD_WRITE:CMD_READ);
    req.be      = mask_be;
    req.address = addr - QEMU_NOC_ADDRESS_DIFFERENCE;
    req.trdid   = tid;
    req.srcid   = m_node_id;
    req.eop     = 1;
    memset(&req.wdata, 0, 8);

    if(bWrite){
        for (i = 0; i < bytes; i++)
            req.wdata[i + ofs] = data[i];
    }

    put_port->put (req);
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
