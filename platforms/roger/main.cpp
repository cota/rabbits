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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <system_init.h>
#include <systemc.h>
#include <abstract_noc.h>
#include <interconnect_master.h>
#include <../../qemu/qemu-0.9.1/qemu_systemc.h>

#include <qemu_wrapper.h>
#include <qemu_cpu_wrapper.h>
#include <qemu_wrapper_cts.h>
#include <interconnect.h>
#include <sram_device.h>
#include <dbf_device.h>
#include <framebuffer_device.h>
#include <timer_device.h>
#include <tty_serial_device.h>
#include <sem_device.h>
#include <mem_device.h>
#include <sl_block_device.h>
#include <qemu_imported.h>
#include <qemu_wrapper_cts.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

unsigned long no_frames_to_simulate = 0;

mem_device          *ram = NULL;
interconnect        *onoc = NULL;
slave_device        *slaves[50];
int                 nslaves = 0;
init_struct         is;

int sc_main (int argc, char ** argv)
{
    int             i;

    memset (&is, 0, sizeof (init_struct));
    is.cpu_family = "arm";
    is.cpu_model = NULL;
    is.kernel_filename = NULL;
    is.initrd_filename = NULL;
    is.no_cpus = 3;
    is.ramsize = 256 * 1024 * 1024;
    parse_cmdline (argc, argv, &is);
    if (check_init (&is) != 0)
        return 1;

    fb_reset_t fb_res_stat = {
        /* .fb_start =           */    0,
        /* .fb_w =               */    0,
        /* .fb_h =               */    0,
        /* .fb_mode =            */ NONE,
        /* .fb_display_on_warp = */    0,
    };

    //slaves
    ram = new mem_device ("dynamic", is.ramsize + 0x1000);
    sram_device       *sram  = new sram_device ("sram", 0x800000);
    tty_serial_device *tty   = new tty_serial_device ("tty");
    sem_device        *sem   = new sem_device ("sem", 0x100000);
    fb_device         *fb    = new fb_device("fb", is.no_cpus, &fb_res_stat); 
    dbf_device        *dbf   = new dbf_device("DBF", is.no_cpus + 1/* , nslaves + 1*/);
    sl_block_device   *bl    = new sl_block_device("block", is.no_cpus + 2, NULL, 1);


    timer_device      *timers[1];
    int                ntimers = sizeof (timers) / sizeof (timer_device *);

    slaves[nslaves++] = ram;             // 0
    slaves[nslaves++] = sram;            // 1
    slaves[nslaves++] = fb->get_slave(); // 2
    slaves[nslaves++] = tty;             // 3
    slaves[nslaves++] = sem;             // 4
    slaves[nslaves++] = bl->get_slave(); // 5

    for (i = 0; i < ntimers; i++){
        char        buf[20];
        sprintf (buf, "timer_%d", i);
        timers[i] = new timer_device (buf);
        slaves[nslaves++] = timers[i];   // 6 + i
    }

    int                         no_irqs = ntimers + 3; /* timers + TTY + FB + DBF */
    int                         int_cpu_mask [] = {1, 1, 1, 1, 0, 0};
    sc_signal<bool>             *wires_irq_qemu = new sc_signal<bool>[no_irqs];
    for (i = 0; i < ntimers; i++)
        timers[i]->irq(wires_irq_qemu[i]);
    tty->irq_line(wires_irq_qemu[ntimers]);
    fb->irq(wires_irq_qemu[ntimers + 1]);
    dbf->irq(wires_irq_qemu[ntimers + 2]);

    //interconnect
    onoc = new interconnect ("interconnect",
                             is.no_cpus + 3,  /* masters: CPUs + FB + DBF + BL*/
                             nslaves + 1);    /* slaves:  ...             */
    for (i = 0; i < nslaves; i++)
        onoc->connect_slave_64 (i, slaves[i]->get_port, slaves[i]->put_port);

    onoc->connect_slave_64(nslaves, dbf->get_port, dbf->put_port);

    arm_load_kernel (&is);

    //masters
    qemu_wrapper qemu1 ("QEMU1", 0, no_irqs, int_cpu_mask, is.no_cpus, 
                        is.cpu_family, is.cpu_model, is.ramsize);
    qemu1.add_map(0xA0000000, 0x20000000); // (base address, size)
    qemu1.set_base_address (QEMU_ADDR_BASE);
    for(i = 0; i < no_irqs; i++)
        qemu1.interrupt_ports[i] (wires_irq_qemu[i]);
    for(i = 0; i < is.no_cpus; i++)
        onoc->connect_master_64 (i, qemu1.get_cpu(i)->put_port, qemu1.get_cpu(i)->get_port);

    if(is.gdb_port > 0){
        qemu1.m_qemu_import.gdb_srv_start_and_wait(qemu1.m_qemu_instance,
            is.gdb_port);
        //qemu1.set_unblocking_write (0);
    }

    /* Master : FrameBuffer */
    onoc->connect_master_64(is.no_cpus, 
                            fb->get_master()->put_port,
                            fb->get_master()->get_port);

    /* Master : H264 DBF */
    onoc->connect_master_64(is.no_cpus + 1,
                            dbf->master_put_port,
                            dbf->master_get_port);

    /* Block device*/
    sc_signal<bool>         bl_irq_wire;
    bl->irq (bl_irq_wire);
    onoc->connect_master_64(is.no_cpus + 2, 
                            bl->get_master()->put_port,
                            bl->get_master()->get_port);

    sc_start ();

    return 0;
}

void invalidate_address (unsigned long addr, int slave_id, 
        unsigned long offset_slave, int src_id)
{
    int                 i, first_node_id;
    unsigned long       taddr;
    qemu_wrapper        *qw;

    for  (i = 0; i < qemu_wrapper::s_nwrappers; i++)
    {
        qw = qemu_wrapper::s_wrappers[i];
        first_node_id = qw->m_cpus[0]->m_node_id;

        if (src_id >= first_node_id && src_id < first_node_id + qw->m_ncpu)
        {
            qw->invalidate_address (addr, src_id - first_node_id);
        }
        else
        {
            if (onoc->get_master (first_node_id)->get_liniar_address (
                    slave_id, offset_slave, taddr))
            {
                qw->invalidate_address (taddr, -1);
            }
        }
    }
}

int systemc_load_image (const char *file, unsigned long ofs)
{
    if (file == NULL)
        return -1;

    int     fd, img_size, size;
    fd = open (file, O_RDONLY | O_BINARY);
    if (fd < 0)
        return -1;

    img_size = lseek (fd, 0, SEEK_END);
    if (img_size + ofs >  ram->get_size ())
    {
        printf ("%s - RAM size < %s size + %lx\n", __FUNCTION__, file, ofs);
        close(fd);
        exit (1);
    }

    lseek (fd, 0, SEEK_SET);
    size = img_size;
    if (read (fd, ram->get_mem () + ofs, size) != size)
    {
        printf ("Error reading file (%s) in function %s.\n", file, __FUNCTION__);
        close(fd);
        exit (1);
    }

    close (fd);

    return img_size;
}

unsigned char* systemc_get_sram_mem_addr ()
{
    return ram->get_mem ();
}

extern "C"
{

unsigned char   dummy_for_invalid_address[256];
struct mem_exclusive_t {unsigned long addr; int cpu;} mem_exclusive[100];
int no_mem_exclusive = 0;

void memory_mark_exclusive (int cpu, unsigned long addr)
{
    int             i;

    addr &= 0xFFFFFFFC;

    for (i = 0; i < no_mem_exclusive; i++)
        if (addr == mem_exclusive[i].addr)
            break;
    
    if (i >= no_mem_exclusive)
    {
        mem_exclusive[no_mem_exclusive].addr = addr;
        mem_exclusive[no_mem_exclusive].cpu = cpu;
        no_mem_exclusive++;

        if (no_mem_exclusive > is.no_cpus)
        {
            printf ("Warning: number of elements in the exclusive list (%d) > cpus (%d) (list: ",
                no_mem_exclusive, is.no_cpus);
            for (i = 0; i < no_mem_exclusive; i++)
                printf ("%lx ", mem_exclusive[i].addr);
            printf (")\n");
            if (is.gdb_port > 0)
                kill (0, SIGINT);

        }
    }
}

int memory_test_exclusive (int cpu, unsigned long addr)
{
    int             i;

    addr &= 0xFFFFFFFC;

    for (i = 0; i < no_mem_exclusive; i++)
        if (addr == mem_exclusive[i].addr)
            return (cpu != mem_exclusive[i].cpu);
    
    return 1;
}

void memory_clear_exclusive (int cpu, unsigned long addr)
{
    int             i;

    addr &= 0xFFFFFFFC;

    for (i = 0; i < no_mem_exclusive; i++)
        if (addr == mem_exclusive[i].addr)
        {
            for (; i < no_mem_exclusive - 1; i++)
            {
                mem_exclusive[i].addr = mem_exclusive[i + 1].addr;
                mem_exclusive[i].cpu = mem_exclusive[i + 1].cpu;
            }
            
            no_mem_exclusive--;
            return;
        }

    printf ("Warning in %s: cpu %d not in the exclusive list: ",
        __FUNCTION__, cpu);
    for (i = 0; i < no_mem_exclusive; i++)
        printf ("(%lx, %d) ", mem_exclusive[i].addr, mem_exclusive[i].cpu);
    printf ("\n");
    if (is.gdb_port > 0)
        kill (0, SIGINT);
}

unsigned char   *systemc_get_mem_addr (qemu_cpu_wrapper_t *qw, unsigned long addr)
{
    int slave_id = onoc->get_master (qw->m_node_id)->get_slave_id_for_mem_addr (addr);
    if (slave_id == -1)
        return dummy_for_invalid_address;
    return slaves[slave_id]->get_mem () + addr;
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
