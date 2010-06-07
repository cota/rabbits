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

#ifndef _ABRSTACT_NOC_H
#define _ABRSTACT_NOC_H

#define MAP_FILES_DIR "./maps"

#include <stdint.h>
#include <systemc.h>

namespace noc
{

    // B = 8
    // K = 8
    // N = 32
    // E = 1
    // Q = 1
    // F = 1
    // S = 14
    // P = 4
    // T = 4
    // W = 1


    class vci_request
    {
    public:
        uint32_t    address;
        uint32_t    initial_address;
        uint8_t     be;
        uint8_t     cmd;
        bool        contig;
        uint8_t     wdata[8];
        bool        eop;
        bool        cons;
        uint8_t     plen;
        bool        wrap;
        bool        cfixed;
        bool        clen;
        uint16_t    srcid;
        uint8_t     trdid;
        uint8_t     pktid;  
    };
    enum {
        CMD_NOP,
        CMD_READ,
        CMD_WRITE,
        CMD_LOCKED_READ,
        CMD_STORE_COND = CMD_NOP,
    };



    class vci_response
    {
    public:
        uint8_t   rdata[8];
        bool      reop;
        bool      rerror;
        uint16_t  rsrcid;
        uint8_t   rtrdid;
        uint8_t   rpktid;

        // **************
        // Extra field
        uint8_t   rbe;
    };

    template <typename T>
        class tlm_blocking_put_if : public sc_interface
    {
    public:
        virtual void put (T&) = 0;
    };

    template <typename T>
        class tlm_blocking_get_if : public sc_interface
    {
    public:
        virtual void get (T&) = 0;
    };


    typedef tlm_blocking_put_if<vci_request>  VCI_PUT_REQ_IF;
    typedef tlm_blocking_put_if<vci_response> VCI_PUT_RSP_IF;

    typedef tlm_blocking_get_if<vci_response> VCI_GET_RSP_IF;
    typedef tlm_blocking_get_if<vci_request>  VCI_GET_REQ_IF;

}

using namespace noc;

ostream &operator<<(ostream& output, const vci_request& value);
ostream &operator<<(ostream& output, const vci_response& value);

#endif /* _ABRSTACT_NOC_H */

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
