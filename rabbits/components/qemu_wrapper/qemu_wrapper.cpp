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
#include <qemu_wrapper.h>
#include <qemu_cpu_wrapper.h>
#include <qemu_imported.h>
#include <qemu_wrapper_request.h>
#include <qemu_wrapper_cts.h>
#include <cpu_logs.h>
#include <dlfcn.h>

#ifdef ENERGY_TRACE_ENABLED
#include <etrace_if.h>
#endif

#include <../../qemu/qemu-0.9.1/systemc_imports.h>

//#define _DEBUG_WRAPPER_QEMU_

#ifdef _DEBUG_WRAPPER_QEMU_
#define DCOUT cout
#else
#define DCOUT if (0) cout
#endif

#if defined (ENERGY_TRACE_ENABLED) && !defined (ETRACE_NB_CPU_IN_GROUP)
#define ETRACE_NB_CPU_IN_GROUP 4
#endif

#define CPU_TIMEOUT         20000000

qemu_wrapper                *qemu_wrapper::s_wrappers[20];
int                         qemu_wrapper::s_nwrappers = 0;

extern "C"
{

    void qemu_wrapper_SLS_banner (void) __attribute__ ((constructor));

    extern void systemc_qemu_wakeup (qemu_cpu_wrapper_t *_this);
    extern void systemc_qemu_consume_instruction_cycles (
        qemu_cpu_wrapper_t *_this, int ninstr);
    extern void systemc_qemu_consume_ns (unsigned long ns);
    extern unsigned long systemc_qemu_read_memory (qemu_cpu_wrapper_t *_this, 
        unsigned long address, unsigned long nbytes, int bIO);
    extern void systemc_qemu_write_memory (qemu_cpu_wrapper_t *_this, 
        unsigned long address, unsigned long data, unsigned char nbytes, int bIO);
    extern unsigned long long systemc_qemu_get_time ();
    extern unsigned long systemc_get_mem_addr (qemu_cpu_wrapper_t *qw, unsigned long addr);
    extern unsigned long systemc_qemu_get_crt_thread (qemu_cpu_wrapper_t *_this);
    extern void memory_mark_exclusive (int cpu, unsigned long addr);
    extern int memory_test_exclusive (int cpu, unsigned long addr);
    extern void memory_clear_exclusive (int cpu, unsigned long addr);
    extern void wait_wb_empty (qemu_cpu_wrapper_t *_this);
    extern unsigned long    no_cycles_cpu0;
}

qemu_wrapper::qemu_wrapper (sc_module_name name, unsigned int node, 
    int ninterrupts, int *int_cpu_mask, int nocpus, int firstcpuindex,
    const char *cpufamily, const char *cpumodel, int ramsize)
: sc_module (name)
{
    m_ncpu = nocpus;
    m_firstcpuindex = firstcpuindex;
    m_qemuLoaded = false;
    m_ninterrupts = ninterrupts;
    interrupt_ports = NULL;
    m_irq_cpu_mask = 0;
    s_wrappers[s_nwrappers++] = this;

    m_interrupts_raw_status = 0;
    m_interrupts_enable = 0;
    if (m_ninterrupts)
    {
        interrupt_ports = new sc_in<bool> [m_ninterrupts];
        m_irq_cpu_mask = new int[m_ninterrupts];
        for (int i = 0; i < m_ninterrupts; i++)
            m_irq_cpu_mask[i] = int_cpu_mask[i];
    }

    char    buf[200];
    sprintf (buf, "libqemu-system-%s.so", cpufamily);
    m_lib_handle = dlopen (buf, RTLD_NOW);
    if (!m_lib_handle)
    {
        printf ("Cannot load library %s in %s\n", buf, __FUNCTION__);
        exit (1);
    }
    sprintf (buf, "%s_qemu_init", cpufamily);
    m_qemu_import.qemu_init = (qemu_init_fc_t) dlsym (m_lib_handle, buf);
    if (!m_qemu_import.qemu_init)
    {
        printf ("Cannot load qemu_init symbol from library %s in %s\n", buf, __FUNCTION__);
        exit (1);
    }

    systemc_import_t        sc_exp_fcs;
    sc_exp_fcs.systemc_qemu_wakeup = (systemc_qemu_wakeup_fc_t) systemc_qemu_wakeup;
    sc_exp_fcs.systemc_qemu_consume_instruction_cycles = 
        (systemc_qemu_consume_instruction_cycles_fc_t) systemc_qemu_consume_instruction_cycles;
    sc_exp_fcs.systemc_qemu_consume_ns =
        (systemc_qemu_consume_ns_fc_t) systemc_qemu_consume_ns;
    sc_exp_fcs.systemc_qemu_read_memory = (systemc_qemu_read_memory_fc_t) systemc_qemu_read_memory;
    sc_exp_fcs.systemc_qemu_write_memory = (systemc_qemu_write_memory_fc_t) systemc_qemu_write_memory;
    sc_exp_fcs.systemc_qemu_get_time = (systemc_qemu_get_time_fc_t) systemc_qemu_get_time;
    sc_exp_fcs.systemc_get_mem_addr = (systemc_get_mem_addr_fc_t) systemc_get_mem_addr;
    sc_exp_fcs.systemc_qemu_get_crt_thread = (systemc_qemu_get_crt_thread_fc_t) systemc_qemu_get_crt_thread;
    sc_exp_fcs.memory_mark_exclusive = memory_mark_exclusive;
    sc_exp_fcs.memory_test_exclusive = memory_test_exclusive;
    sc_exp_fcs.memory_clear_exclusive = memory_clear_exclusive;
    sc_exp_fcs.wait_wb_empty = (wait_wb_empty_fc_t) wait_wb_empty;
    sc_exp_fcs.no_cycles_cpu0 = &no_cycles_cpu0;

    m_qemu_instance = m_qemu_import.qemu_init (node, m_ncpu, firstcpuindex, 
        cpumodel, ramsize, &m_qemu_import, &sc_exp_fcs);

    printf ("QEMU %d has %d %s cpus ([node_id, cpu_id] = ",
        s_nwrappers - 1, m_ncpu, cpufamily);
    for (int i = 0; i < m_ncpu; i++)
    {
        if (i)
            printf (", ");
        printf ("[%d, %d]", node + i, firstcpuindex + i);
    }
    printf (")\n");
    m_qemuLoaded = true;

    m_cpu_interrupts_status     = new unsigned long[m_ncpu];
    m_cpu_interrupts_raw_status = new unsigned long[m_ncpu];
    for (int i = 0; i < m_ncpu; i++)
    {
        m_cpu_interrupts_status[i]     = 0;
        m_cpu_interrupts_raw_status[i] = 0;
    }

    m_logs = new cpu_logs (m_ncpu, cpufamily, cpumodel);

    #ifdef ENERGY_TRACE_ENABLED
    int             etrace_group_id;
    unsigned long   periph_id;
    char            etrace_group_name[50];
    #endif

    m_cpus = new qemu_cpu_wrapper_t * [m_ncpu];
    for (int i = 0; i < m_ncpu; i++)
    {
        char            *s = new char [50];
        sprintf (s, "qemu-cpu-%d", i);
        m_cpus[i] = new qemu_cpu_wrapper_t (s, m_qemu_instance,
                                           node + i, i, m_logs, &m_qemu_import);
        m_cpus[i]->m_port_access (*this);

        #ifdef ENERGY_TRACE_ENABLED
        if ((i % ETRACE_NB_CPU_IN_GROUP) == 0)
        {
            etrace_group_id = -1;
            int end_cpu = i + ETRACE_NB_CPU_IN_GROUP - 1;
            if (end_cpu > m_ncpu - 1)
                end_cpu = m_ncpu - 1;
            if (i == end_cpu)
                sprintf (etrace_group_name, "CPU %d", i);
            else
                sprintf (etrace_group_name, "CPU %d-%d", i, end_cpu);
        }
        sprintf (buf, "CPU %d", i);
        periph_id = etrace.add_periph (buf,
            get_cpu_etrace_class (cpufamily, cpumodel),
            etrace_group_id, etrace_group_name);
        m_cpus[i]->set_etrace_periph_id (periph_id);
        #endif
    }

    SC_THREAD (interrupts_thread);
    SC_THREAD (timeout_thread);
}

qemu_wrapper::~qemu_wrapper ()
{
    int                 i;

    if (m_qemuLoaded)
        m_qemu_import.qemu_release (m_qemu_instance);

    if (m_cpus)
    {
        for (i = 0; i < m_ncpu; i++)
            delete m_cpus [i];

        delete [] m_cpus;
    }

    if (interrupt_ports)
        delete [] interrupt_ports;

    if (m_irq_cpu_mask)
        delete [] m_irq_cpu_mask;

    delete [] m_cpu_interrupts_raw_status;
    delete [] m_cpu_interrupts_status;

    if (m_lib_handle)
        dlclose (m_lib_handle);
}

void qemu_wrapper::set_unblocking_write (bool val)
{
    int                 i;
    for (i = 0; i < m_ncpu; i++)
        m_cpus [i]->set_unblocking_write (val);
}

void qemu_wrapper::add_map (unsigned long base_address, unsigned long size)
{
    m_qemu_import.qemu_add_map (m_qemu_instance, base_address, size, 0);
}

void qemu_wrapper::set_base_address (unsigned long base_address)
{
    m_qemu_import.qemu_add_map (m_qemu_instance, base_address, SIZE_QEMU_WRAPPER_MEMORY, 1);

    for (int i = 0; i < m_ncpu; i++)
        m_cpus[i]->set_base_address (base_address);
}

void qemu_wrapper::timeout_thread ()
{
    #ifdef TIME_AT_FV_LOG_GRF
    while (1)
    {
        wait (CPU_TIMEOUT, SC_NS);

        for (int i = 0; i < m_ncpu; i++)
            m_cpus[i]->sync ();
        wait (0, SC_NS);

        m_logs->update_fv_grf ();
    }
    #endif
}

class my_sc_event_or_list : public sc_event_or_list
{
public:
    inline my_sc_event_or_list (const sc_event& e, bool del = false)
        : sc_event_or_list (e, del) {}
    inline my_sc_event_or_list& operator | (const sc_event& e)
    {push_back (e);return *this;}
};

void qemu_wrapper::interrupts_thread ()
{
    if (!m_ninterrupts)
        return;

    int                     i, cpu;
    bool                    *bup, *bdown;
    unsigned long           val;
    my_sc_event_or_list     el (interrupt_ports[0].default_event ());

    bup = new bool[m_ncpu];
    bdown = new bool[m_ncpu];

    for (i = 1; i < m_ninterrupts; i++)
        el | interrupt_ports[i].default_event ();

    while (1)
    {
        wait (el);

        for (cpu = 0; cpu < m_ncpu; cpu++)
        {
            bup[cpu] = false;
            bdown[cpu] = false;
        }

        val = 1;
        for (i = 0; i < m_ninterrupts; i++)
        {
            if (interrupt_ports[i].posedge ())
            {
                m_interrupts_raw_status |= val;
                for (cpu = 0; cpu < m_ncpu; cpu++)
                {
                    if (m_irq_cpu_mask[i] & (1 << cpu))
                    {
                        m_cpu_interrupts_raw_status[cpu] |= val;
                        if (val & m_interrupts_enable)
                        {
                            if (!m_cpu_interrupts_status[cpu]){
                                bup[cpu] = true;
                            }
                            m_cpu_interrupts_status[cpu] |= val;
                        }
                    }
                }
            }
            else
                if (interrupt_ports[i].negedge ())
                {
                    m_interrupts_raw_status &= ~val;
                    for (cpu = 0; cpu < m_ncpu; cpu++)
                    {
                        if (m_irq_cpu_mask[i] & (1 << cpu))
                        {
                            m_cpu_interrupts_raw_status[cpu] &= ~val;
                            if (val & m_interrupts_enable)
                            {
                                m_cpu_interrupts_status[cpu] &= ~val;
                                if (!m_cpu_interrupts_status[cpu])
                                    bdown[cpu] = true;
                            }
                        }
                    }
                }

            val <<= 1;
        }

        for (cpu = 0; cpu < m_ncpu; cpu++)
        {
            if (bup[cpu] && !m_cpus[cpu]->m_swi)
            {
                DCOUT << "******INT SENT***** to cpu " << cpu << endl;
                m_qemu_import.qemu_irq_update (m_qemu_instance, 1 << cpu, 1);
                m_cpus[cpu]->wakeup ();
            }
            else
                if (bdown[cpu] && !m_cpus[cpu]->m_swi)
                {
                    //wait (2.5, SC_NS);
                    m_qemu_import.qemu_irq_update (m_qemu_instance, 1 << cpu, 0);
                }
        }
    }
}

//qemu_wrapper_access_interface
unsigned long qemu_wrapper::get_no_cpus  ()
{
    return m_ncpu;
}

unsigned long qemu_wrapper::get_cpu_fv_level (unsigned long cpu)
{
    return m_cpus[cpu - m_firstcpuindex]->get_cpu_fv_level ();
}

void qemu_wrapper::set_cpu_fv_level (unsigned long cpu, unsigned long val)
{
    if (cpu == (unsigned long) -1)
    {
        for (cpu = 0; cpu < m_ncpu; cpu++)
            m_cpus[cpu]->set_cpu_fv_level (val);
    }
    else
        m_cpus[cpu - m_firstcpuindex]->set_cpu_fv_level (val);
}

void qemu_wrapper::generate_swi (unsigned long cpu_mask, unsigned long swi)
{
    int                         cpu;
    for (cpu = 0; cpu < m_ncpu; cpu++)
    {
        if (cpu_mask & (1 << cpu))
        {
            m_cpus[cpu]->m_swi |= swi;
            if (!m_cpu_interrupts_status[cpu])
            {
                m_qemu_import.qemu_irq_update (m_qemu_instance, 1 << cpu, 1);
                m_cpus[cpu]->wakeup ();
            }
        }
    }
}

void qemu_wrapper::swi_ack (int cpu, unsigned long swi_mask)
{
    unsigned long   swi = m_cpus[cpu]->m_swi;
    if (swi == 0)
        return;
    swi &= ~swi_mask;
    m_cpus[cpu]->m_swi = swi;

    if (!m_cpu_interrupts_status[cpu] && !swi)
        m_qemu_import.qemu_irq_update (m_qemu_instance, 1 << cpu, 0);
}

unsigned long qemu_wrapper::get_cpu_ncycles (unsigned long cpu)
{
    return m_cpus[cpu - m_firstcpuindex]->get_cpu_ncycles ();
}

uint64 qemu_wrapper::get_no_cycles_cpu (int cpu)
{
    uint64                      ret = 0;
    if (cpu == -1)
    {
        for (int i = 0; i < s_nwrappers; i++)
            for (cpu = 0; cpu < s_wrappers[i]->m_ncpu; cpu++)
                ret += s_wrappers[i]->m_cpus[cpu]->get_no_cycles ();
    }
    else
        ret = m_cpus[cpu - m_firstcpuindex]->get_no_cycles ();

    return ret;
}

unsigned long qemu_wrapper::get_int_status ()
{
    return m_interrupts_raw_status & m_interrupts_enable;
}

unsigned long qemu_wrapper::get_int_enable ()
{
    return m_interrupts_enable;
}

void qemu_wrapper::set_int_enable (unsigned long val)
{
    int            cpu;
    for (cpu = 0; cpu < m_ncpu; cpu++)
    {
        if (!m_cpu_interrupts_status[cpu] && (m_cpu_interrupts_raw_status[cpu] & val)
            && !m_cpus[cpu]->m_swi)
            m_qemu_import.qemu_irq_update (m_qemu_instance, 1 << cpu, 1);
        else
            if (m_cpu_interrupts_status[cpu] && !(m_cpu_interrupts_raw_status[cpu] & val)
                && !m_cpus[cpu]->m_swi)
                m_qemu_import.qemu_irq_update (m_qemu_instance, 1 << cpu, 0);

        m_cpu_interrupts_status[cpu] = m_cpu_interrupts_raw_status[cpu] & val;
    }

    m_interrupts_enable = val;
}

void
qemu_wrapper::invalidate_address (unsigned long addr, int idx_src)
{
    m_qemu_import.qemu_invalidate_address (m_qemu_instance,
                addr, idx_src);
}

extern "C"
{
    void
    qemu_wrapper_SLS_banner (void)
    {
        fprintf (stdout,
                "================================================================================\n"
                "|  This simulation uses the QEMU/SystemC Wrapper from the RABBITS' framework   |\n"
                "|                     Copyright (c) 2009 - 2010 Tima SLS                       |\n"
                "================================================================================\n");
    }
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
