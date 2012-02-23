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

#ifndef _QEMU_WRAPPER_SLAVE_H_
#define _QEMU_WRAPPER_SLAVE_H_

#include "slave_device.h"
#include "qemu_wrapper.h"

enum qemu_wrapper_slave_registers {
    QEMU_WRAPPER_SLAVE_SET_CPUS_FQ = 0,
};

class qemu_wrapper_slave_device : public slave_device
{
public:
    SC_HAS_PROCESS (qemu_wrapper_slave_device);
    qemu_wrapper_slave_device (sc_module_name module_name);
    virtual ~qemu_wrapper_slave_device ();

public:
    void set_master (qemu_wrapper *qemu_master);
    /*
     *   Obtained from father
     *   void send_rsp (bool bErr, uint8_t oob);
     */
    virtual void rcv_rqst (unsigned long ofs, unsigned char be,
                           unsigned char *data, bool bWrite, bool sleep);

private:
    void write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr);
    void read  (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr);

private:
    qemu_wrapper        *m_qemu_master;
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
