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

#ifndef _AICU_H_
#define _AICU_H_

#include <slave_device.h>

enum aicu_global_registers {
    AICU_CTRL       = 0,
    AICU_HANDLER0   =  4,
    AICU_HANDLER1   =  5,
    AICU_HANDLER2   =  6,
    AICU_HANDLER3   =  7,
    AICU_HANDLER4   =  8,
    AICU_HANDLER5   =  9,
    AICU_HANDLER6   = 10,
    AICU_HANDLER7   = 11,
    AICU_HANDLER8   = 12,
    AICU_HANDLER9   = 13,
    AICU_HANDLER10  = 14,
    AICU_HANDLER11  = 15,
    AICU_HANDLER12  = 16,
    AICU_HANDLER13  = 17,
    AICU_HANDLER14  = 18,
    AICU_HANDLER15  = 19,
    AICU_HANDLER16  = 20,
    AICU_HANDLER17  = 21,
    AICU_HANDLER18  = 22,
    AICU_HANDLER19  = 23,
    AICU_HANDLER20  = 24,
    AICU_HANDLER21  = 25,
    AICU_HANDLER22  = 26,
    AICU_HANDLER23  = 27,
    AICU_HANDLER24  = 28,
    AICU_HANDLER25  = 29,
    AICU_HANDLER26  = 30,
    AICU_HANDLER27  = 31,
    AICU_HANDLER28  = 32,
    AICU_HANDLER29  = 33,
    AICU_HANDLER30  = 34,
    AICU_HANDLER31  = 35,
    AICU_LOCAL      = 0x100 >> 2,
};

enum aicu_local_registers {
    AICU_STAT   = 0,
    AICU_MASK   = 1,
    AICU_ADDR   = 2,
    AICU_IRQ    = 3,
    AICU_SPAN   = 0x10 >> 2,
};

class aicu_device : public slave_device
{
public:
    SC_HAS_PROCESS (aicu_device);
    aicu_device (sc_module_name module_name, int nb_out, 
                          int nb_global_in, int nb_local_in);
    virtual ~aicu_device ();

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

    void irq_update_thread (void);
    void reset_registers (void);

public:
    //ports
    sc_in<bool>        *irq_in;
    sc_out<bool>       *irq_out;

private:
    int                 O; // nb_irq_out
    int                 G; // nb_global_irq
    int                 L; // nb_local_irq
    sc_event            ev_irq_update;

    // Global registers
    uint32_t            m_control;
    uint32_t           *m_handlers;

    // Local registers
    uint32_t           *m_stat;
    uint32_t           *m_mask;
    uint32_t           *m_crt_irq;


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
