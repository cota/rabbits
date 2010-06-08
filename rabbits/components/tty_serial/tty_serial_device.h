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

#ifndef _TTY_SERIAL_DEVICE_H_
#define _TTY_SERIAL_DEVICE_H_

#include <slave_device.h>

#define READ_BUF_SIZE           256

typedef struct
{
    unsigned long       int_enabled;
    unsigned long       int_level;
    unsigned long       read_buf[READ_BUF_SIZE];
    unsigned long       read_pos;
    unsigned long       read_count;
    unsigned long       read_trigger;
} tty_state;

class tty_serial_device : public slave_device
{
public:
    SC_HAS_PROCESS (tty_serial_device);
    tty_serial_device (sc_module_name _name);
    virtual ~tty_serial_device ();

public:
    /*
     *   Obtained from father
     *   void send_rsp (bool bErr);
     */
    virtual void rcv_rqst (unsigned long ofs, unsigned char be,
                           unsigned char *data, bool bWrite);

private:
    void write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr);
    void read  (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr);

    sc_event irq_update;
    //void irq_update ();
    void read_thread ();
    void irq_update_thread();

public:
    //ports
    sc_out<bool>        irq_line;

private:
    sc_event            evRead;

    int                 pin;
    int                 pout;
    tty_state           state;
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
