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

#ifndef _SL_TIMER_DEVICE_H_
#define _SL_TIMER_DEVICE_H_

#include <slave_device.h>

enum sl_timer_registers {
    TIMER_VALUE    = 0,
    TIMER_MODE     = 1,
    TIMER_PERIOD   = 2,
    TIMER_RESETIRQ = 3,
    TIMER_SPAN     = 4,
};

enum sl_timer_mode {
    TIMER_RUNNING = 1,
    TIMER_IRQ_ENABLED = 2,
};


class sl_timer_device : public slave_device
{
public:
    SC_HAS_PROCESS (sl_timer_device);
    sl_timer_device (sc_module_name module_name);
    virtual ~sl_timer_device ();

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

    void sl_timer_thread ();
    void irq_update_thread ();


public:
    //ports
    sc_out<bool>        irq;

private:
    uint32_t            m_period;
    uint32_t            m_mode;
    uint32_t            m_value;

    double              m_ns_period;
    sc_event            ev_wake;
    sc_event            ev_irq_update;
    
    uint64_t            m_last_period;
    uint64_t            m_next_period;

    bool                m_irq;
    bool                m_config_mod;

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
