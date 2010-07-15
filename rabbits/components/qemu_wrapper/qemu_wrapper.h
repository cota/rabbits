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

#ifndef _QEMU_WRAPPER_
#define _QEMU_WRAPPER_

#include <cfg.h>
#include <qemu_wrapper_access_interface.h>
#include <qemu_imported.h>
#include <qemu_cpu_wrapper.h>
using namespace noc;

class cpu_logs;

class qemu_wrapper : public sc_module, public qemu_wrapper_access_interface
{
public:
    SC_HAS_PROCESS (qemu_wrapper);
    qemu_wrapper (sc_module_name name, unsigned int node, int ninterrupts, int *int_cpu_mask,
                  int nocpus, int firstcpuindex, const char *cpufamily, const char *cpumodel, int ramsize);
    ~qemu_wrapper ();

public:
    void add_map (unsigned long base_address, unsigned long size);
    void set_base_address (unsigned long base_address);
    void set_unblocking_write (bool val);

    //inline qemu_cpu_wrapper <stbus_bca_request<64>, stbus_bca_response<64> > * get_cpu (int i) {return m_cpus[i];}
    inline qemu_cpu_wrapper_t * get_cpu (int i) {return m_cpus[i];}
    static void invalidate_address (unsigned long addr, unsigned int node_id);

    //qemu_wrapper_access_interface
    virtual unsigned long get_no_cpus  ();
    virtual unsigned long get_cpu_fv_level (unsigned long cpu);
    virtual void set_cpu_fv_level (unsigned long cpu, unsigned long val);
    virtual void generate_swi (unsigned long cpu_mask, unsigned long swi);
    virtual void swi_ack (int cpu, unsigned long swi_mask);
    virtual unsigned long get_cpu_ncycles (unsigned long cpu);
    virtual unsigned long get_int_status ();
    virtual unsigned long get_int_enable ();
    virtual void set_int_enable (unsigned long val);
    virtual uint64 get_no_cycles_cpu (int cpu);

private:
    //threads
    void stnoc_interrupts_thread ();
    void timeout_thread ();

public:
    //ports
    sc_in<bool>                         *interrupt_ports;

    void                                *m_qemu_instance;
    qemu_import_t                       m_qemu_import;

    static qemu_wrapper                 *s_wrappers[20];
    static int                           s_nwrappers;

private:
    // attr
    unsigned long                       *m_cpu_interrupts_raw_status;
    unsigned long                       *m_cpu_interrupts_status;
    unsigned long                       m_interrupts_raw_status;
    unsigned long                       m_interrupts_enable;
    int                                 *m_irq_cpu_mask;

    void                                *m_lib_handle;

    int                                 m_ninterrupts;
    bool                                m_qemuLoaded;
    int                                 m_ncpu;
    int                                 m_firstcpuindex;
    qemu_cpu_wrapper_t                **m_cpus;

    cpu_logs                            *m_logs;
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
