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

#define TIMER_DIV 10

sl_timer_device::sl_timer_device (sc_module_name module_name) : slave_device (module_name)
{

    m_period    = 0;
    m_mode      = 0;
    m_value     = 0;
    m_ns_period = 0;
    m_last_period = 0;
    m_next_period = 0;
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
    uint32_t                value;

    ofs >>= 2;
    if (be & 0xF0)
    {
        ofs++;
        value = * ((uint32_t *) data + 1);
    }
    else
        value = * ((uint32_t *) data + 0);

    switch (ofs)
    {
    case TIMER_VALUE:
        DPRINTF("Unsupported write to VALUE register\n");
        break;

    case TIMER_MODE:
        DPRINTF("Mode write : %x\n", value); 
        if((m_mode ^ value) & 0x1){
            if(value & 0x1){ /* Starting */
                m_last_period = sc_time_stamp().value();
                m_value       = 0;
            }
            m_config_mod = true;
            ev_wake.notify();
        }
        m_mode = value & 0x3;
        break;

    case TIMER_PERIOD:
        m_period      = value;
        m_next_period = (sc_time_stamp().value() - m_last_period) + 
            ((uint64_t)1000000000000ULL /* 1s -> ps */ / (SYSTEM_CLOCK_FV / TIMER_DIV) * m_period);

        DPRINTF("TIMER_PERIOD write : %d (curr_tick: %lld next: %lld)\n",
               value, (sc_time_stamp().value() - m_last_period)/1000,
               m_next_period/1000);
        m_config_mod = true;
        ev_wake.notify();
        break;

    case TIMER_RESETIRQ:
        m_irq = 0;
        ev_irq_update.notify();
        break;

    default:
        DPRINTF ("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                 name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                 (unsigned int) *((unsigned long*)data + 0),
                 (unsigned int) *((unsigned long*)data + 1));
        exit (1);
        break;
    }
    bErr = false;
}

void sl_timer_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int             i;
    uint32_t  *val = (uint32_t *)data;

    ofs >>= 2;
    if (be & 0xF0)
    {
        ofs++;
        val++;
    }

    *val = 0;

    switch (ofs)
    {
    case TIMER_VALUE:
        *val = (sc_time_stamp().value() - m_last_period) / 1000 * 
            (SYSTEM_CLOCK_FV / TIMER_DIV) / 1000000000;
        DPRINTF("TIMER_VALUE read: %d [sc: %lld last: %lld diff: %lld]\n",
               *val, sc_time_stamp().value()/1000, m_last_period/1000,
               (sc_time_stamp().value() -  m_last_period)/1000);
        break;

    case TIMER_MODE:
        *val = m_mode;
        break;

    case TIMER_PERIOD:
        *val = m_period;
        break;

    case TIMER_RESETIRQ:
        *val = (m_irq == 1);
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

        irq = (m_irq == 1);
    }
}

void sl_timer_device::sl_timer_thread (void)
{
    while (1)
    {
        if (m_period == 0){
            wait (ev_wake);
        }else{
            uint64_t wait_time = 0;
            //DPRINTF("Timer running\n");
            wait_time = m_next_period - (sc_time_stamp().value() - m_last_period);
            //DPRINTF("Waiting %lld ns\n", wait_time/1000);
            wait(wait_time, SC_PS, ev_wake);
          
            //wait (m_ns_period, SC_NS, ev_wake);
        }

        if(m_config_mod){
            //DPRINTF("Timer Started/Stopped\n");
            m_irq = 0;
            ev_irq_update.notify();
            m_config_mod = 0;
        }else{ // end of period
            if(m_mode & TIMER_IRQ_ENABLED){
                DPRINTF("Timer raise an IRQ @%lld\n",
                       (sc_time_stamp().value() - m_last_period)/1000);
                m_irq = 1;
                ev_irq_update.notify();
            }
            m_next_period = (sc_time_stamp().value() - m_last_period) + 
                ((uint64_t)1000000000000ULL /* 1s -> ps */ / (SYSTEM_CLOCK_FV / TIMER_DIV) * m_period);
        }
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
