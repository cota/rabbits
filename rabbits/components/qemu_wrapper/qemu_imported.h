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

#ifndef _QEMU_IMPORTED_FUNCTIONS_
#define _QEMU_IMPORTED_FUNCTIONS_

#ifdef __cplusplus
extern "C"
{
#endif

    struct qemu_import_t;

    typedef void            *(*qemu_init_fc_t) (int id, int ncpu, int indexfirstcpu,
                                                const char *cpu_model, int _ramsize, 
                                                struct qemu_import_t *qi, void *systemc_fcs);
    typedef void            (*qemu_add_map_fc_t) (void *instance,
                                                  unsigned long base_address, unsigned long size, 
                                                  int type);
    typedef void            (*qemu_release_fc_t) (void *instance);
    typedef void            *(*qemu_get_set_cpu_obj_fc_t) (void *instance,
                                                           unsigned long index, void *sc_obj);
    typedef long            (*qemu_cpu_loop_fc_t) (void* cpuenv);
    typedef void            (*qemu_set_cpu_fv_percent_fc_t) (void* cpuenv,
                                                             unsigned long fvpercent);
    typedef void            (*qemu_irq_update_fc_t) (void* instance, int cpu_mask,
                                                     int level);
    typedef void            (*qemu_get_counters_fc_t) (
        unsigned long long *no_instr,
        unsigned long long *no_dcache_miss,
        unsigned long long *no_write,
        unsigned long long *no_icache_miss,
        unsigned long long *no_uncached);
    typedef void            (*qemu_invalidate_address_fc_t) (void* instance, 
                                                             unsigned long addr, int src_idx);
    typedef int             (*gdb_srv_start_and_wait_fc_t) (int port);

    //imported by QEMU
    struct qemu_import_t
    {
        qemu_init_fc_t                  qemu_init;
        qemu_add_map_fc_t               qemu_add_map;
        qemu_release_fc_t               qemu_release;
        qemu_get_set_cpu_obj_fc_t       qemu_get_set_cpu_obj;
        qemu_cpu_loop_fc_t              qemu_cpu_loop;
        qemu_set_cpu_fv_percent_fc_t    qemu_set_cpu_fv_percent;
        qemu_irq_update_fc_t            qemu_irq_update;
        qemu_get_counters_fc_t          qemu_get_counters;
        qemu_invalidate_address_fc_t    qemu_invalidate_address;
        gdb_srv_start_and_wait_fc_t     gdb_srv_start_and_wait;
    };

#ifdef __cplusplus
}
#endif

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
