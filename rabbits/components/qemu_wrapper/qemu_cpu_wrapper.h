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

#ifndef _QEMU_CPU_WRAPPER_
#define _QEMU_CPU_WRAPPER_

#include <cfg.h>
#include <qemu_wrapper_request.h>
#include <qemu_wrapper_access_interface.h>
#include <qemu_imported.h>

#include <master_device.h>

using namespace noc;

class cpu_logs;

class qemu_cpu_wrapper : public master_device
{
public:
    SC_HAS_PROCESS (qemu_cpu_wrapper);
    qemu_cpu_wrapper (sc_module_name name, void *qemu_instance, unsigned int node_id,
                      int cpuindex, cpu_logs *logs, qemu_import_t *qi);
    ~qemu_cpu_wrapper ();

public:
    void set_base_address (unsigned long base_address);
    void set_cpu_fv_level (unsigned long val);
    unsigned long get_cpu_fv_level ();
    unsigned long get_cpu_ncycles ();
    void set_unblocking_write (bool val);
    //qemu interface
    unsigned long systemc_qemu_read_memory (unsigned long address,
                                            unsigned char nbytes, int bIO);
    void systemc_qemu_write_memory (unsigned long address, unsigned long data,
                                    unsigned char nbytes, int bIO);
    void consume_instruction_cycles_with_sync (unsigned long ns);
    void add_time_at_fv (unsigned long ns);
    uint64 get_no_cycles ();
    void wait_wb_empty ();
    void wakeup ();
    void sync ();

    #ifdef ENERGY_TRACE_ENABLED
    //etrace
    void set_etrace_periph_id (unsigned long id);
    #endif

private:
    //threads
    void cpu_thread ();

    void rcv_rsp(unsigned char tid, unsigned char *data, bool bErr, bool bWrite);

private:
    //local functions
    unsigned long read (unsigned long address, unsigned char nbytes, int bIO);
    void write (unsigned long address, unsigned long data,
                unsigned char nbytes, int bIO);
    /* long blocking_raw_read (unsigned long addr8Balgn, unsigned char be, */
    /*     unsigned char bytes, unsigned char *data, int bIO); */
    /* long blocking_raw_write (unsigned long addr8Balgn, unsigned char be, */
    /*     unsigned char bytes, unsigned char *data, int bIO); */


public:
    //ports
    sc_port<qemu_wrapper_access_interface>  m_port_access;

private:
    //signals & events
    sc_event                                m_ev_wakeup;
    sc_event                                m_ev_sync;
    sc_signal <unsigned long>               m_fv_level;

    //other attributes
    qemu_wrapper_requests                   *m_rqs;
    void                                    *m_cpuenv;
    void                                    *m_qemu_instance;
    unsigned long                           m_base_address;
    unsigned long                           m_end_address;
    bool                                    m_unblocking_write;
    double                                  m_ns_per_cpu_cycle_max_fv;

    qemu_import_t                           *m_qemu_import;

    //tmp regs
    uint64									m_last_read_sctime;
    unsigned long                           m_last_no_cycles_high;

    //counters
    uint64									m_no_total_cycles;

    //log
    cpu_logs								*m_logs;

    #ifdef ENERGY_TRACE_ENABLED
    //etrace
    unsigned long                           m_etrace_periph_id;
    #endif

public:
    int								        m_cpuindex;
    //    unsigned int                      m_node_id;
    unsigned long                           m_crt_cpu_thread;
    unsigned long                           m_swi;
 
};

//typedef qemu_cpu_wrapper< stbus_bca_request<64>, stbus_bca_response<64> > qemu_cpu_wrapper_t;
typedef qemu_cpu_wrapper qemu_cpu_wrapper_t;

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
