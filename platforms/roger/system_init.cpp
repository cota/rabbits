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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <system_init.h>

#define BOARD_ID            2339
#define KERNEL_ARGS_ADDR    0x100
#define KERNEL_LOAD_ADDR    0x00010000
#define INITRD_LOAD_ADDR    0x00800000
#define HAS_ARG             0x0001

#ifndef O_BINARY
#define O_BINARY 0
#endif

extern unsigned long no_frames_to_simulate;

int systemc_load_image (const char *kernel, unsigned long ofs);
unsigned char* systemc_get_sram_mem_addr ();

enum
{
    CMDLINE_OPTION_loops,
    CMDLINE_OPTION_ncpu,
	CMDLINE_OPTION_cpu_family,
    CMDLINE_OPTION_cpu,
    CMDLINE_OPTION_ram,
    CMDLINE_OPTION_sram,
    CMDLINE_OPTION_kernel,
    CMDLINE_OPTION_initrd,
    CMDLINE_OPTION_gdb_port,
	CMDLINE_OPTION_kernel_cmd,
};

typedef struct
{
    const char      *name;
    int             flags;
    int             index;
} cmdline_option;

const cmdline_option cmdline_options[] = 
{
    {"L",       HAS_ARG, CMDLINE_OPTION_loops},
    {"ncpu",    HAS_ARG, CMDLINE_OPTION_ncpu},
    {"M",       HAS_ARG, CMDLINE_OPTION_cpu_family},
    {"cpu",     HAS_ARG, CMDLINE_OPTION_cpu},
    {"ram",     HAS_ARG, CMDLINE_OPTION_ram},
    {"sram",    HAS_ARG, CMDLINE_OPTION_sram},
    {"kernel",  HAS_ARG, CMDLINE_OPTION_kernel},
    {"initrd",  HAS_ARG, CMDLINE_OPTION_initrd},
    {"gdb_port",  HAS_ARG, CMDLINE_OPTION_gdb_port},
	{"append", HAS_ARG, CMDLINE_OPTION_kernel_cmd},
    {NULL},
};

void parse_cmdline (int argc, char **argv, init_struct *is)
{
    int                 optind;
    const char          *r, *optarg;

    optind = 1;
    for (; ;)
    {
        if (optind >= argc)
            break;
        r = argv[optind];

        const cmdline_option *popt;

        optind++;
        /* Treat --foo the same as -foo.  */
        if (r[1] == '-')
            r++;
        popt = cmdline_options;
        for (;;)
        {
            if (!popt->name)
            {
                fprintf (stderr, "%s: invalid option -- '%s'\n", argv[0], r);
                exit (1);
            }
            if (!strcmp (popt->name, r + 1))
                break;
            popt++;
        }
        if (popt->flags & HAS_ARG)
        {
            if (optind >= argc)
            {
                fprintf (stderr, "%s: option '%s' requires an argument\n",
                        argv[0], r);
                exit (1);
            }
            optarg = argv[optind++];
        }
        else
            optarg = NULL;

        switch (popt->index)
        {
        case CMDLINE_OPTION_loops:
            no_frames_to_simulate = atoi (optarg);
            break;
        case CMDLINE_OPTION_ncpu:
            is->no_cpus = atoi (optarg);
            break;
        case CMDLINE_OPTION_cpu_family:
            is->cpu_family = optarg;
            break;
        case CMDLINE_OPTION_cpu:
            is->cpu_model = optarg;
            break;
        case CMDLINE_OPTION_ram:
            is->ramsize = atoi (optarg) * 1024 * 1024;
            break;
        case CMDLINE_OPTION_sram:
            is->sramsize = atoi (optarg) * 1024 * 1024;
            break;
        case CMDLINE_OPTION_kernel:
            is->kernel_filename = optarg;
            break;
        case CMDLINE_OPTION_initrd:
            is->initrd_filename = optarg;
        case CMDLINE_OPTION_gdb_port:
            is->gdb_port = atoi (optarg);
            break;
		case CMDLINE_OPTION_kernel_cmd:
			 is->kernel_cmdline = optarg;
			 break;
        }
    }
}

static unsigned long bootloader[] = 
{
  0xe3a00000, /* mov     r0, #0 */
  0xe3a01000, /* mov     r1, #0x?? */
  0xe3811c00, /* orr     r1, r1, #0x??00 */
  0xe59f2000, /* ldr     r2, [pc, #0] */
  0xe59ff000, /* ldr     pc, [pc, #0] */
  0, /* Address of kernel args.  Set by integratorcp_init.  */
  0  /* Kernel entry point.  Set by integratorcp_init.  */
};

static void set_kernel_args (unsigned long ram_size, int initrd_size,
    const char *kernel_cmdline, unsigned long loader_start)
{
    unsigned long               *p;

    p = (unsigned long *) (systemc_get_sram_mem_addr () + KERNEL_ARGS_ADDR);

    /* ATAG_CORE */
    *p++ = 5;
    *p++ = 0x54410001;
    *p++ = 1;
    *p++ = 0x1000;
    *p++ = 0;
    /* ATAG_MEM */
    *p++ = 4;
    *p++ = 0x54410002;
    *p++ = ram_size;
    *p++ = loader_start;

    if (initrd_size)
    {
        /* ATAG_INITRD2 */
        *p++ = 4;
        *p++ = 0x54420005;
        *p++ = loader_start + INITRD_LOAD_ADDR;
        *p++ = initrd_size;
    }

    if (kernel_cmdline && *kernel_cmdline)
    {
        /* ATAG_CMDLINE */
        int cmdline_size;

        cmdline_size = strlen (kernel_cmdline);
        memcpy (p + 2, kernel_cmdline, cmdline_size + 1);
        cmdline_size = (cmdline_size >> 2) + 1;
        *p++ = cmdline_size + 2;
        *p++ = 0x54410009;
        p += cmdline_size;
    }
    /* ATAG_END */
    *p++ = 0;
    *p++ = 0;
}

void arm_load_kernel (init_struct *is)
{
    systemc_load_image (is->kernel_filename, KERNEL_LOAD_ADDR);

    bootloader[1] |= BOARD_ID & 0xff;
    bootloader[2] |= (BOARD_ID >> 8) & 0xff;
    bootloader[5] = KERNEL_ARGS_ADDR;
    bootloader[6] = KERNEL_LOAD_ADDR;
    memcpy (systemc_get_sram_mem_addr (), bootloader, sizeof (bootloader));

    int     initrd_size = systemc_load_image (is->initrd_filename, INITRD_LOAD_ADDR);
    if (-1 == initrd_size)
        initrd_size = 0;

    set_kernel_args (is->ramsize, initrd_size, is->kernel_cmdline, 0);
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
