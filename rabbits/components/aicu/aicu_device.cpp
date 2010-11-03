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

#include <cfg.h>

#include <stdio.h>
#include <stdlib.h>
#include <aicu_device.h>

/* #define DEBUG_DEVICE_AICU */

#ifdef DEBUG_DEVICE_AICU
#define DPRINTF(fmt, args...)                               \
    do { printf("aicu_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

aicu_device::aicu_device (sc_module_name module_name, int nb_out, 
                          int nb_global_in, int nb_local_in) : slave_device (module_name)
{
    int i = 0;

    if((nb_global_in + nb_local_in) > 32){
        fprintf(stderr, "Interruption number too high\n"
               "Limited to 32 interrupts for now\n");
        exit(EXIT_FAILURE);
    }
    O = nb_out;
    G = nb_global_in;
    L = nb_local_in;

    DPRINTF("Creating an AICU with %d outputs, %d local interrupts and %d global interrupts\n",
            O, L, G);

    // inputs and outputs
    DPRINTF("Allocating: %d inputs and %d outputs\n",
            nb_global_in + (nb_out*nb_local_in), nb_out);

    irq_in  = new sc_in<bool> [nb_global_in + (nb_out*nb_local_in)];
    irq_out = new sc_out<bool> [nb_out];

    // Global registers
    m_control  = 0;
    m_handlers = new uint32_t [nb_global_in + nb_local_in];

    // Local registers
    m_stat    = new uint32_t [nb_out];
    m_mask    = new uint32_t [nb_out];
    m_crt_irq = new uint32_t [nb_out];

    reset_registers();

    //SC_THREAD (aicu_thread);
    SC_THREAD (irq_update_thread);
    
}

aicu_device::~aicu_device ()
{
}

void aicu_device::reset_registers(void)
{
    int i = 0;

    for(i = 0; i < (G + L); i++){
        m_handlers[i] = 0;
    }

    for(i = 0; i < O; i++){
        m_stat[i]    = 0;
        m_mask[i]    = 0;
        m_crt_irq[i] = 0;
    }

}

void aicu_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
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

    if(ofs < AICU_LOCAL){
        switch (ofs)
        {
        case AICU_CTRL:
            DPRINTF("Control register write: 0x%x\n", value);
            if( (m_control ^ value) & 0x1 ){ /* the S bit */ 
                reset_registers();
            }
            m_control = value;
            // config changed ?
            break;
        case AICU_HANDLER0  :
        case AICU_HANDLER1  :
        case AICU_HANDLER2  :
        case AICU_HANDLER3  :
        case AICU_HANDLER4  :
        case AICU_HANDLER5  :
        case AICU_HANDLER6  :
        case AICU_HANDLER7  :
        case AICU_HANDLER8  :
        case AICU_HANDLER9  :
        case AICU_HANDLER10 :
        case AICU_HANDLER11 :
        case AICU_HANDLER12 :
        case AICU_HANDLER13 :
        case AICU_HANDLER14 :
        case AICU_HANDLER15 :
        case AICU_HANDLER16 :
        case AICU_HANDLER17 :
        case AICU_HANDLER18 :
        case AICU_HANDLER19 :
        case AICU_HANDLER20 :
        case AICU_HANDLER21 :
        case AICU_HANDLER22 :
        case AICU_HANDLER23 :
        case AICU_HANDLER24 :
        case AICU_HANDLER25 :
        case AICU_HANDLER26 :
        case AICU_HANDLER27 :
        case AICU_HANDLER28 :
        case AICU_HANDLER29 :
        case AICU_HANDLER30 :
        case AICU_HANDLER31 :
            DPRINTF("Setting Handler%d: 0x%x\n", (ofs - AICU_HANDLER0), value);
            m_handlers[ofs - AICU_HANDLER0] = value;
            break;
        default:
            printf ("Bad %s::%s ofs=0x%X, be=0x%X! (GLOBAL)\n",
                    name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
            exit (1);
        }
    }else{
        unsigned long nb_icu  = (ofs - AICU_LOCAL) / AICU_SPAN;
        unsigned long lcl_ofs = (ofs - AICU_LOCAL)%(AICU_SPAN);

        if(nb_icu >= O){
            fprintf(stderr, "Out of range\n");
            exit(EXIT_FAILURE);
        }
        switch (lcl_ofs)
        {
        case AICU_STAT:
            DPRINTF("AICU_STAT[%d] write: 0x%x\n", nb_icu, value);
            m_stat[nb_icu] &= ~value;
            ev_irq_update.notify();
            break;
        case AICU_MASK:
            DPRINTF("AICU_MASK[%d] write: 0x%x\n", nb_icu, value);
            m_mask[nb_icu] = value;
            ev_irq_update.notify();
            break;

        case AICU_ADDR: /* Unsupported write */

        default:
            printf ("Bad %s::%s ofs=0x%x, be=0x%x data: 0x%x! (LOCAL) %d\n",
                    name (), __FUNCTION__, (unsigned int) lcl_ofs, (unsigned int) be,
                    value, nb_icu);
            exit (1);
        }

    }
    bErr = false;
}

void aicu_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{

    uint32_t  *val = (uint32_t *)data;

    ofs >>= 2;
    if (be & 0xF0){   ofs++;val++;   }
    *val = 0;

    if(ofs < AICU_LOCAL){
        switch (ofs)
        {
        case AICU_CTRL:
            *val = m_control;
            DPRINTF("Control register read: 0x%x\n", *val);
            break;

        case AICU_HANDLER0  :
        case AICU_HANDLER1  :
        case AICU_HANDLER2  :
        case AICU_HANDLER3  :
        case AICU_HANDLER4  :
        case AICU_HANDLER5  :
        case AICU_HANDLER6  :
        case AICU_HANDLER7  :
        case AICU_HANDLER8  :
        case AICU_HANDLER9  :
        case AICU_HANDLER10 :
        case AICU_HANDLER11 :
        case AICU_HANDLER12 :
        case AICU_HANDLER13 :
        case AICU_HANDLER14 :
        case AICU_HANDLER15 :
        case AICU_HANDLER16 :
        case AICU_HANDLER17 :
        case AICU_HANDLER18 :
        case AICU_HANDLER19 :
        case AICU_HANDLER20 :
        case AICU_HANDLER21 :
        case AICU_HANDLER22 :
        case AICU_HANDLER23 :
        case AICU_HANDLER24 :
        case AICU_HANDLER25 :
        case AICU_HANDLER26 :
        case AICU_HANDLER27 :
        case AICU_HANDLER28 :
        case AICU_HANDLER29 :
        case AICU_HANDLER30 :
        case AICU_HANDLER31 :
            *val = m_handlers[ofs - AICU_HANDLER0];
            break;
            
        default:
            printf ("Bad %s::%s ofs=0x%X, be=0x%X!\n",
                    name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
            exit (1);
        }
    }else{
        int nb_icu  = (ofs - AICU_LOCAL) / AICU_SPAN;
        int lcl_ofs = (ofs - AICU_LOCAL) % AICU_SPAN;

        if(nb_icu >= O){
            fprintf(stderr, "Out of range\n");
            exit(EXIT_FAILURE);
        }
        switch (lcl_ofs)
        {
        case AICU_STAT:
            *val = m_stat[nb_icu];
            DPRINTF("AICU_STAT[%d] read: 0x%x\n", nb_icu, *val);
            break;
        case AICU_MASK:
            *val = m_mask[nb_icu];
            DPRINTF("AICU_MASK[%d] read: 0x%x\n", nb_icu, *val);
            break;
        case AICU_ADDR:
            *val = m_handlers[m_crt_irq[nb_icu]];
            // auto clearing IRQ
            m_stat[nb_icu] &= ~(1 << m_crt_irq[nb_icu]);
            DPRINTF("AICU_ADDR[%d] read: 0x%x\n", nb_icu, *val);
            ev_irq_update.notify();
            break;
        case AICU_IRQ:
            *val = m_crt_irq[nb_icu];
            DPRINTF("AICU_IRQ[%d] read: 0x%x\n", nb_icu, *val);
            break;
        default:
            printf ("Bad %s::%s ofs=0x%X, be=0x%X!\n",
                    name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
            exit (1);
        }

    }


    bErr = false;
}

class my_sc_event_or_list : public sc_event_or_list
{
public:
    inline my_sc_event_or_list (const sc_event& e, bool del = false) : sc_event_or_list (e, del) {}
    inline my_sc_event_or_list& operator | (const sc_event& e){push_back (e);return *this;}
};

void aicu_device::irq_update_thread ()
{
    unsigned long       flags;
    int i = 0, j = 0;
    my_sc_event_or_list event_list(ev_irq_update);

    for(i = 0; i < (G + L*O); i++){
        event_list | irq_in[i].default_event();
    }

    while(1) {

        wait(event_list);

        if(!(m_control & 0x1)){
            continue;
        }

        // for each output
        for(i = 0; i < O; i++){

            // look at each interrupt
            for(j = 0; j < (L + G); j++){
                int ind_in = 0;

                if(j < L){
                    ind_in = O*j + i;
                }else{
                    ind_in = O*L + (j - L);
                }

                DPRINTF("For output %d looking input %d\n", i, ind_in);

                if(irq_in[ind_in].posedge()){
                    m_stat[i] |= (1 << j);
                }else{
                    if(irq_in[ind_in].negedge()){
                        m_stat[i] &= ~(1 << j);
                    }
                }
            }
            
            // IRQ status up to date.
            for(j = 0; j < (L + G); j++){
                if((m_stat[i] & m_mask[i]) & (1 << j)){
                    m_crt_irq[i] = j;
                    DPRINTF("Raising IRQ%d for OUT%d\n", j, i);
                    break;
                }
            }
           
            // Update irq outputs
            irq_out[i] = ((m_stat[i] & m_mask[i]) != 0);

        }
    }
}

void aicu_device::rcv_rqst (unsigned long ofs, unsigned char be,
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
