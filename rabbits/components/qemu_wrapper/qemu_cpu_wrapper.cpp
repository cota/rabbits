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
#include <qemu_cpu_wrapper.h>
#include <qemu_imported.h>
#include <qemu_wrapper_cts.h>
#include <cpu_logs.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <../../qemu/qemu-0.9.1/qemu_systemc.h>

#ifdef ENERGY_TRACE_ENABLED
#include <etrace_if.h>
#endif

#include <master_device.h>

static unsigned long                        s_addr_startup_secondary = 0xFFFFFFFF;

//#define DEBUG_QEMU_CPU_WRAPPER
#ifdef DEBUG_QEMU_CPU_WRAPPER
#define DCOUT cout
#else
#define DCOUT if (0) cout
#endif

//#define COUT_TIMES cout
#define COUT_TIMES if(0) cout

#define EXCP_INTERRUPT      0x10000     /* async interruption */
#define EXCP_HLT            0x10001     /* hlt instruction reached */
#define EXCP_DEBUG          0x10002     /* cpu stopped after a breakpoint or singlestep */
#define EXCP_HALTED         0x10003     /* cpu is halted (waiting for external event) */
#define EXCP_POWER_DOWN     0x10004 
#define EXCP_RESET          0x10005
#define EXCP_SHUTDOWN       0x10006
#define EXCP_VM_NOT_RUNNING 0x10007

static struct timeval                   start_time;


qemu_cpu_wrapper::qemu_cpu_wrapper (sc_module_name name, 
                                    void *qemu_instance, unsigned int node_id,
                                    int cpuindex, cpu_logs *logs, qemu_import_t *qi) 
: master_device (name)
{
    m_qemu_import = qi;
    m_qemu_instance = qemu_instance;
    m_node_id = node_id;
    m_cpuindex = cpuindex;
    m_rqs = new qemu_wrapper_requests (100);

    m_logs = logs;
    m_fv_level = m_logs->m_cpu_boot_fv_level;
    m_last_read_sctime = (uint64) -1;
    m_last_no_cycles_high = (unsigned long) -1;

    m_ns_per_cpu_cycle_max_fv = 
        ((double) 1000) / m_logs->m_cpu_fvs[m_logs->m_cpu_nb_fv_levels - 1];
    m_crt_cpu_thread = 0;
    m_no_total_cycles = 0;
    m_unblocking_write = 1;
    m_swi = 0;

    m_cpuenv = m_qemu_import->qemu_get_set_cpu_obj (m_qemu_instance, cpuindex, this);

    if (cpuindex == 0)
        gettimeofday (&start_time, NULL);

    SC_THREAD (cpu_thread);
}

qemu_cpu_wrapper::~qemu_cpu_wrapper ()
{
    if (m_rqs)
        delete m_rqs;
}

void qemu_cpu_wrapper::set_unblocking_write (bool val)
{
    m_unblocking_write = val;
}

void qemu_cpu_wrapper::consume_instruction_cycles_with_sync (unsigned long ncycles)
{
    uint64              ps_start_time;
    double              cycles_done;
    double              start_fv_percent;
    unsigned long       start_fv_level;
    uint64              start_no_cycles;
    uint64              ps_time_passed;
    double              cycles = ncycles;
    bool                bIdle = false;

    start_no_cycles = m_no_total_cycles;

    do
    {
        ps_start_time = sc_time_stamp ().value ();

        start_fv_percent = m_logs->m_cpu_fv_percents[m_fv_level];
        if (start_fv_percent == 0)
        {
            bIdle = true;
            start_fv_percent = m_logs->m_cpu_fv_percents[0];
        }
        start_fv_level = m_fv_level;

        wait ((cycles * m_ns_per_cpu_cycle_max_fv * 100) / start_fv_percent, SC_NS, m_ev_sync);

        ps_time_passed = sc_time_stamp ().value () - ps_start_time;
        m_logs->add_time_at_fv (m_cpuindex, start_fv_level, ps_time_passed / 1000);
        cycles_done = (ps_time_passed * start_fv_percent) / (1000 * 100 * m_ns_per_cpu_cycle_max_fv);

        if (!bIdle)
            m_no_total_cycles += (uint64) cycles_done;
        cycles -= cycles_done;
    } while (cycles >= 1);

    if (!bIdle)
        m_no_total_cycles = start_no_cycles + ncycles;
}


void qemu_cpu_wrapper::cpu_thread ()
{
    int                 hr;

    while (1)
    {
        hr = m_qemu_import->qemu_cpu_loop (m_cpuenv);

        DCOUT << "CPU " << m_cpuindex << " exits qemu_cpu_loop, hr = 0x" << std::hex << hr << std::dec << endl;

        switch (hr)
        {
        case EXCP_HLT:
        case EXCP_HALTED:
        {
            uint64      ps_start_time = sc_time_stamp ().value ();

            #ifdef ENERGY_TRACE_ENABLED
            etrace.change_energy_mode (m_etrace_periph_id, ETRACE_MODE(ETRACE_CPU_IDLE, m_fv_level));
            #endif

            wait (m_ev_wakeup);
            m_logs->add_time_at_fv (m_cpuindex, m_logs->m_cpu_nb_fv_levels,
                (sc_time_stamp ().value () - ps_start_time) / 1000);

            #ifdef ENERGY_TRACE_ENABLED
            etrace.change_energy_mode (m_etrace_periph_id, ETRACE_MODE(ETRACE_CPU_RUNNING, m_fv_level));
            #endif
        }
        break;

        case EXCP_INTERRUPT:
        case EXCP_RESET:
            break;

        case EXCP_VM_NOT_RUNNING:
        case EXCP_POWER_DOWN:
        case EXCP_SHUTDOWN:
            printf ("Received power down signal (0x%X).\n", hr);
            sc_stop ();
            break;

        default:
            printf ("Error: unknown return code 0x%X from qemu_cpu_loop in %s\n", hr, __FUNCTION__);
            break;
        }
    }
}

void qemu_cpu_wrapper::set_base_address (unsigned long base_address)
{
    m_base_address = base_address;
    m_end_address = base_address + SIZE_QEMU_WRAPPER_MEMORY - 1;
}


void qemu_cpu_wrapper::set_cpu_fv_level (unsigned long val)
{
    if (val == m_fv_level)
        return;

    if  (val >= m_logs->m_cpu_nb_fv_levels)
    {
        cerr << "Error: Bad cpu percent specified for qemu_wrapper " << std::dec << val << endl;
        exit (1);
    }

    m_qemu_import->qemu_set_cpu_fv_percent (m_cpuenv, (unsigned long) m_logs->m_cpu_fv_percents[val]);
    m_fv_level = val;
    m_ev_sync.notify ();

    #ifdef ENERGY_TRACE_ENABLED
    etrace.change_energy_mode (m_etrace_periph_id, ETRACE_MODE(ETRACE_CPU_RUNNING, val));
    #endif
}

unsigned long qemu_cpu_wrapper::get_cpu_fv_level ()
{
    return m_fv_level;
}


unsigned long qemu_cpu_wrapper::get_cpu_ncycles ()
{
    m_ev_sync.notify ();
    wait (0, SC_NS);

    return m_logs->get_cpu_ncycles (m_cpuindex);
}


uint64 qemu_cpu_wrapper::get_no_cycles ()
{
    m_ev_sync.notify ();
    wait (0, SC_NS);

    return m_no_total_cycles;
}


unsigned long qemu_cpu_wrapper::systemc_qemu_read_memory (
    unsigned long addr, unsigned long nbytes, int bIO)
{
    unsigned long           val;

    if (addr >= m_base_address && addr <= m_end_address)
    {
        addr -= m_base_address;
        switch (addr)
        {
        case GET_SYSTEMC_NO_CPUS:
            val = m_port_access->get_no_cpus ();
            break;

        case GET_SYSTEMC_TIME_LOW:
            if (m_last_read_sctime == (uint64) -1)
            {
                m_last_read_sctime = sc_time_stamp ().value () / 1000;
                val = m_last_read_sctime & 0xFFFFFFFF;
            }
            else
            {
                val = m_last_read_sctime & 0xFFFFFFFF;
                m_last_read_sctime = (uint64) -1;
            }
            //cout << "GET_SYSTEMC_TIME_LOW = " << val << endl;
            break;

        case GET_SYSTEMC_TIME_HIGH:
            if (m_last_read_sctime == (uint64) -1)
            {
                m_last_read_sctime = sc_time_stamp ().value () / 1000;
                val = (m_last_read_sctime >> 32);
            }
            else
            {
                val = (m_last_read_sctime >> 32);
                m_last_read_sctime = (uint64) -1;
            }
            //cout << "GET_SYSTEMC_TIME_HIGH = " << val << endl;
            break;

        case GET_SYSTEMC_CRT_CPU_FV_LEVEL:
            val = m_fv_level;
            break;

        case GET_SYSTEMC_CPU1_FV_LEVEL:
        case GET_SYSTEMC_CPU2_FV_LEVEL:
        case GET_SYSTEMC_CPU3_FV_LEVEL:
        case GET_SYSTEMC_CPU4_FV_LEVEL:
            val = m_port_access->get_cpu_fv_level ((addr - GET_SYSTEMC_CPU1_FV_LEVEL) >> 2);
            break;

        case GET_SYSTEMC_CPU1_NCYCLES:
        case GET_SYSTEMC_CPU2_NCYCLES:
        case GET_SYSTEMC_CPU3_NCYCLES:
        case GET_SYSTEMC_CPU4_NCYCLES:
            val = m_port_access->get_cpu_ncycles ((addr - GET_SYSTEMC_CPU1_NCYCLES) >> 2);
            break;

        case GET_SYSTEMC_INT_ENABLE:
            val = m_port_access->get_int_enable ();
            break;

        case GET_SYSTEMC_INT_STATUS:
            val = m_port_access->get_int_status ();
            break;

        case GET_SYSTEMC_MAX_INT_PENDING:
            if (m_swi)
            {
                unsigned long           i = 1;
                while ((i & m_swi) == 0)
                    i <<= 1;
                val = i;
            }
            else
            {
                val = m_port_access->get_int_status ();
                if (!val)
                    val = 1023;
                else
                {
                    unsigned long i = 0;
                    val = val & (-val);
                    while (val)
                    {
                        val >>= 1;
                        i++;
                    }
                    val = 32 + i - 1;
                }
            }
            break;

        case GET_ALL_CPUS_NO_CYCLES:
        case GET_NO_CYCLES_CPU1:
        case GET_NO_CYCLES_CPU2:
        case GET_NO_CYCLES_CPU3:
        case GET_NO_CYCLES_CPU4:
            if (m_last_no_cycles_high == (unsigned long) -1)
            {
                uint64                      no_cycles;
                int                         cpu;
                cpu = (int) ((addr - GET_ALL_CPUS_NO_CYCLES) >> 2) - 1;
                no_cycles = m_port_access->get_no_cycles_cpu (cpu) ;
                m_last_no_cycles_high = no_cycles >> 32;
                val = (unsigned long) (no_cycles & 0xFFFFFFFF);
            }
            else
            {
                val = m_last_no_cycles_high;
                m_last_no_cycles_high = (unsigned long) -1;
            }
            break;
        case GET_SECONDARY_STARTUP_ADDRESS:
            val = s_addr_startup_secondary;
            break;
        case GET_MEASURE_RES:
        {
            unsigned long     mean_use, mean_power = 0;
            
#ifdef ENERGY_TRACE_ENABLED
            mean_power = etrace.stop_measure ();
            mean_use = m_logs->stop_measure ();
#endif
            val = mean_use | (mean_power << 16);
        }
        break;
        default:
            val = 0xFFFFFFFF;
            cerr << "Error: Bad qemu_wrapper address " << std::hex << addr <<
                " in function " << __FUNCTION__ << "." << endl;
            break;
        }
    }
    else
    {
        #ifdef ENERGY_TRACE_ENABLED
        etrace.energy_event (m_etrace_periph_id, READ_COMMAND, 0);
        #endif

        val = read (addr, nbytes, bIO);
    }

    return val;
}


void qemu_cpu_wrapper::systemc_qemu_write_memory (unsigned long addr, 
                                                  unsigned long data, unsigned char nbytes, int bIO)
{
    static unsigned long us_prev_img = 0;

    if (addr >= m_base_address && addr <= m_end_address)
    {
        addr -= m_base_address;
        switch (addr)
        {
        case SET_SYSTEMC_CRT_CPU_FV_LEVEL:
            set_cpu_fv_level (data);
            break;

        case SET_SYSTEMC_CPUX_FV_LEVEL:
            m_port_access->set_cpu_fv_level (data & 0xFF, data >> 8);
        break;
        case SET_SYSTEMC_ALL_FV_LEVEL:
            m_port_access->set_cpu_fv_level ((unsigned long) -1, data);
            break;
        case TEST_WRITE_SYSTEMC:
            wait (100 * m_ns_per_cpu_cycle_max_fv, SC_NS);
            break;
        case SYSTEMC_SHUTDOWN:
            if (data != 0)
            {
                cout << "Shutdown requested!" << endl;
                exit (1);
            }
            break;

        case SET_SYSTEMC_INT_ENABLE:
            m_port_access->set_int_enable (data);
            break;

        case LOG_END_OF_IMAGE:
        {
            unsigned long us_crt_sc_time = sc_time_stamp ().value () / 1000000;
            cout << "Log end of image " << data << ", sc_time=" << us_crt_sc_time/1000 <<
                "ms, after " << (us_crt_sc_time - us_prev_img)/1000 << " ms." << endl;
            us_prev_img = us_crt_sc_time;

            unsigned long long no_instr, no_dcache_miss, no_write, no_icache_miss, no_uncached;
            m_qemu_import->qemu_get_counters (&no_instr, &no_dcache_miss, &no_write, &no_icache_miss, &no_uncached);

            struct timeval      crt_time;
            gettimeofday (&crt_time, NULL);

            fprintf (stderr,
                     "#QEMU: SIMULATED_TIME= %lu us,SIMULATION_TIME= %lu ms,"
                     "INSTR= %llu ,CYCLES_INSTR= %llu"
                     " ,DCACHE_MISS= %llu ,ICACHE_MISS= %llu"
                     " ,UNCACHED= %llu ,WRITE_NO= %llu"
                     "\n",
                     us_crt_sc_time,
                     (crt_time.tv_sec - start_time.tv_sec) * 1000 +
                     (crt_time.tv_usec - start_time.tv_usec) / 1000,
                     no_instr, m_port_access->get_no_cycles_cpu (-1),
                     no_dcache_miss, no_icache_miss,
                     no_uncached, no_write
                );
            break;
        }
        case TEST1_WRITE_SYSTEMC:
        {
            static unsigned long cnt1 = 0;
            printf ("TEST1_WRITE_SYSTEMC %lu, sc_time=%llu ps, ninstr=%llu\n",
                    ++cnt1, sc_time_stamp ().value (), m_port_access->get_no_cycles_cpu (-1));
        }
        break;
        case TEST2_WRITE_SYSTEMC:
            printf ("TEST2_WRITE_SYSTEMC, ninstr=%llu\n", m_port_access->get_no_cycles_cpu (-1));
            break;

        case TEST3_WRITE_SYSTEMC:
            printf ("TEST3_WRITE_SYSTEMC, ninstr=%llu\n", m_port_access->get_no_cycles_cpu (-1));
            break;
        case LOG_SET_THREAD_CPU:
            m_crt_cpu_thread = data;
            break;
        case SET_SECONDARY_STARTUP_ADDRESS:
            s_addr_startup_secondary = data;
            break;
        case GENERATE_SWI:
        {
            #if 0
            static int cnt = 0;
            printf ("SWI data=%lu (%d)\n", data, ++cnt);
            #endif

            m_port_access->generate_swi (data, 1);
        }
            break;
        case SWI_ACK:
        {
#if 0
            static int cnt = 0;
            printf ("\t\t\tACK cpu=%d, data=%lu (%d)\n", m_cpuindex, data, ++cnt);
#endif
            
            m_port_access->swi_ack (m_cpuindex, data);
        }
        break;
        case SET_MEASURE_START:
#ifdef ENERGY_TRACE_ENABLED
            etrace.start_measure ();
            m_logs->start_measure ();
#endif
            break;
        default:
            cerr << "Error: Bad qemu_wrapper address " << std::hex << addr 
                 << " in function " << __FUNCTION__ << "." << endl;
            break;
        }
    }
    else
    {
        #ifdef ENERGY_TRACE_ENABLED
        etrace.energy_event (m_etrace_periph_id, WRITE_COMMAND, 0);
        #endif

        write (addr, data, nbytes, bIO);
    }
}

void qemu_cpu_wrapper::add_time_at_fv (unsigned long ns)
{
    m_logs->add_time_at_fv (m_cpuindex, m_fv_level, ns);
}


void qemu_cpu_wrapper::wakeup ()
{
    m_ev_wakeup.notify ();
}

void qemu_cpu_wrapper::sync ()
{
    m_ev_sync.notify ();
}

#ifdef ENERGY_TRACE_ENABLED
void qemu_cpu_wrapper::set_etrace_periph_id (unsigned long id)
{
    this->m_etrace_periph_id = id;
    etrace.change_energy_mode (m_etrace_periph_id,
        ETRACE_MODE(ETRACE_CPU_RUNNING, m_logs->m_cpu_boot_fv_level));
}
#endif

void qemu_cpu_wrapper::rcv_rsp (unsigned char tid, unsigned char *data,
                                bool bErr, bool bWrite)
{

    qemu_wrapper_request            *localrq;

    localrq = m_rqs->GetRequestByTid (tid);
    if (localrq == NULL)
    {
        cout << "[Error: " << name () << " received a response for an unknown TID 0x" 
             << std::hex << (unsigned long) tid << "]" << endl;
        return;
    }

    //DCOUT << name () << " received with success: " << endl << resp << endl;

    if (m_unblocking_write && localrq->bWrite) //response for a write cmd
    {
        m_rqs->FreeRequest (localrq);
        return;
    }

    localrq->rcv_data = * (unsigned long *) data;
    localrq->bDone = 1;
    localrq->evDone.notify ();
}

unsigned long qemu_cpu_wrapper::read (unsigned long addr,
                                      unsigned long nbytes, int bIO)
{
    unsigned long           ret;
    int                     i;
    qemu_wrapper_request    *localrq;

    if (m_unblocking_write)
        localrq = m_rqs->GetNewRequest (1);
    else
        localrq = m_rqs->GetNewRequest (0);

    if (localrq == NULL)
        return -2;

    localrq->bWrite = 0;

    send_req (localrq->tid, addr, NULL, nbytes, 0);

    while (!localrq->bDone)
        wait (localrq->evDone);

    if (nbytes > 4)
        nbytes = 4;
    for (i = 0; i < nbytes; i++)
        ((unsigned char *) &ret)[i] = ((unsigned char *) &localrq->rcv_data)[i];

    m_rqs->FreeRequest (localrq);

    return ret;
}


void qemu_cpu_wrapper::write (unsigned long addr,
                              unsigned long data, unsigned char nbytes, int bIO)
{
    unsigned char                   tid;
    qemu_wrapper_request            *localrq;

    if (m_unblocking_write)
        localrq = m_rqs->GetNewRequest (bIO);
    else
        localrq = m_rqs->GetNewRequest (0);

    if (localrq == NULL)
        return;

    localrq->bWrite = 1;
    tid = localrq->tid;

    send_req(tid, addr, (unsigned char *)&data, nbytes, 1);

    if (!m_unblocking_write)
    {
        while (!localrq->bDone)
            wait (localrq->evDone);
        m_rqs->FreeRequest (localrq);
    }

    return;
}


void qemu_cpu_wrapper::wait_wb_empty ()
{
    m_rqs->WaitWBEmpty ();
}


//template class qemu_cpu_wrapper< stbus_bca_request<64>, stbus_bca_response<64> >;

extern "C"
{
    unsigned long no_cycles_cpu0;

    void
    systemc_qemu_wakeup (qemu_cpu_wrapper_t *_this)
    {
        _this->wakeup ();
    }

    void
    systemc_qemu_consume_instruction_cycles (
        qemu_cpu_wrapper_t *_this, int ninstr)
    {
        _this->consume_instruction_cycles_with_sync (ninstr);
        if (!_this->m_cpuindex)
            no_cycles_cpu0 += ninstr;
    }

    void
    systemc_qemu_consume_ns (unsigned long ns)
    {
        wait (ns, SC_NS);
    }

    unsigned long
    systemc_qemu_read_memory (qemu_cpu_wrapper_t *_this, unsigned long addr,
                              unsigned long nbytes, int bIO)
    {
        unsigned long                   ret;
        unsigned long long              diff, starttime = sc_time_stamp ().value ();

        ret = _this->systemc_qemu_read_memory (addr, nbytes, bIO);

        diff = sc_time_stamp ().value () - starttime;
        if (diff)
            _this->add_time_at_fv (diff);

        return ret;
    }

    void
    systemc_qemu_write_memory (qemu_cpu_wrapper_t *_this, unsigned long addr,
        unsigned long data, unsigned char nbytes, int bIO)
    {
        unsigned long long          diff, starttime = sc_time_stamp ().value ();

        _this->systemc_qemu_write_memory (addr, data, nbytes, bIO);

        diff = sc_time_stamp ().value () - starttime;
        if (diff)
        {
            _this->add_time_at_fv (diff);
        }
    }

    unsigned long long
    systemc_qemu_get_time ()
    {
        return sc_time_stamp ().value () / 1000;
    }

    unsigned long
    systemc_qemu_get_crt_thread (qemu_cpu_wrapper_t *_this)
    {
        return _this->m_crt_cpu_thread;
    }

    void
    wait_wb_empty (qemu_cpu_wrapper_t *_this)
    {
        _this->wait_wb_empty ();
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
