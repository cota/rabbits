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

#include <stdio.h>
#include <stdlib.h>
#include <sl_timer_device.h>
#include <cfg.h>

//#define DEBUG_DEVICE_TIMER

#ifdef DEBUG_DEVICE_TIMER
#define DPRINTF(fmt, args...)                               \
    do { printf("sl_timer_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

sl_timer_device::sl_timer_device (sc_module_name module_name) : slave_device (module_name)
{

    m_period    = 0;
    m_mode      = 0;
    m_ns_period = 0;
    m_last_period = 0;
    m_irq = false;
    m_config_mod = false;

    SC_THREAD (sl_timer_thread);
    SC_THREAD (irq_update_thread);
    
}

sl_timer_device::~sl_timer_device ()
{
}

void sl_timer_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    unsigned long           val1 = ((unsigned long*)data)[0];
    unsigned long           val2 = ((unsigned long*)data)[1];

    switch (ofs)
    {
    case TIMER_VALUE:
        DPRINTF("Unsupported write to VALUE register\n");
        break;

    case TIMER_MODE:
        DPRINTF("Mode write : %x\n", val1); 
        if((m_mode ^ val1) & 0x1){
            m_config_mod = true;
            m_mode = val1 & 0x3;
            ev_wake.notify();
        }else{
            m_mode = *data & 0x3;
        }
        break;

    case TIMER_PERIOD:
        m_period = val1;
        m_ns_period = ((double)1000000000) / SYSTEM_CLOCK_FV * m_period;
        m_config_mod = true;
        ev_wake.notify();
        break;

    case TIMER_RESETIRQ:
        m_irq = false;
        ev_irq_update.notify();
        break;

    default:
        DPRINTF ("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                 name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                 (unsigned int) *((unsigned long*)data + 0), (unsigned int) *((unsigned long*)data + 1));
        exit (1);
        break;
    }
    bErr = false;
}

void sl_timer_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int             i;
    uint32_t  *val = (uint32_t *)data;

    val = 0;
    *((unsigned long *)data + 1) = 0;

    switch (ofs)
    {
    case TIMER_VALUE:
        *val = (sc_time_stamp().value() - m_last_period) / 1000 / SYSTEM_CLOCK_FV;
        break;

    case TIMER_MODE:
        *val = m_mode;
        break;

    case TIMER_PERIOD:
        *val = m_period;
        break;

    case TIMER_RESETIRQ:
        *val = (m_irq == true);
        break;


    default:
        printf ("Bad %s::%s ofs=0x%X, be=0x%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
        exit (1);
    }
    bErr = false;
}

void sl_timer_device::irq_update_thread ()
{
    unsigned long       flags;

    while(1) {

        wait(ev_irq_update);

        irq = (m_irq == true);
    }
}

void sl_timer_device::sl_timer_thread (void)
{
    while (1)
    {
        if (m_ns_period == 0){
            wait (ev_wake);
        }else{
            DPRINTF("Timer running\n");
            wait (m_ns_period, SC_NS, ev_wake);
        }

        if(m_config_mod){
            DPRINTF("Timer Started/Stopped\n");
            m_irq = 0;
            ev_irq_update.notify();
        }else{ // end of period
            if(m_mode & TIMER_IRQ_ENABLED){
                DPRINTF("Timer raise an IRQ\n");
                m_irq = 1;
                ev_irq_update.notify();
            }
        }
        m_last_period = sc_time_stamp().value();
    }
}

void sl_timer_device::rcv_rqst (unsigned long ofs, unsigned char be,
                                unsigned char *data, bool bWrite)
{

    bool bErr = false;

    if(bWrite){
        this->write(ofs, be, data, bErr);
    }else{
        this->read(ofs, be, data, bErr);
    }

    send_rsp(bErr);

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
