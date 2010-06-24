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

#ifndef _QEMU_WRAPPER_CONSTANTS_
#define _QEMU_WRAPPER_CONSTANTS_

#define QEMU_ADDR_BASE								0x82000000
#define	SIZE_QEMU_WRAPPER_MEMORY					0x1000

//get
#define GET_SYSTEMC_NO_CPUS							0x0000
#define GET_SYSTEMC_CRT_CPU_ID						0x0004
#define GET_SYSTEMC_TIME_LOW						0x0008
#define GET_SYSTEMC_TIME_HIGH						0x000C
#define GET_SYSTEMC_CRT_CPU_FV_LEVEL				0x0010
#define GET_SYSTEMC_CPU1_FV_LEVEL					0x0014
#define GET_SYSTEMC_CPU2_FV_LEVEL					0x0018
#define GET_SYSTEMC_CPU3_FV_LEVEL					0x001C
#define GET_SYSTEMC_CPU4_FV_LEVEL					0x0020
#define GET_SYSTEMC_CPU1_NCYCLES					0x0024
#define GET_SYSTEMC_CPU2_NCYCLES					0x0028
#define GET_SYSTEMC_CPU3_NCYCLES					0x002C
#define GET_SYSTEMC_CPU4_NCYCLES					0x0030
#define GET_SYSTEMC_INT_ENABLE  					0x0040
#define GET_SYSTEMC_INT_STATUS  					0x0044
#define GET_ALL_CPUS_NO_CYCLES                      0x0050
#define GET_NO_CYCLES_CPU1                          0x0054
#define GET_NO_CYCLES_CPU2                          0x0058
#define GET_NO_CYCLES_CPU3                          0x005C
#define GET_NO_CYCLES_CPU4                          0x0060
#define GET_SYSTEMC_MAX_INT_PENDING				    0x0080
#define GET_SECONDARY_STARTUP_ADDRESS               0x0084

//set
#define TEST_WRITE_SYSTEMC							0x0000
#define SYSTEMC_SHUTDOWN							0x0004
#define SET_SYSTEMC_CRT_CPU_FV_LEVEL				0x0010
#define SET_SYSTEMC_CPU1_FV_LEVEL					0x0014
#define SET_SYSTEMC_CPU2_FV_LEVEL					0x0018
#define SET_SYSTEMC_CPU3_FV_LEVEL					0x001C
#define SET_SYSTEMC_CPU4_FV_LEVEL					0x0020
#define SET_SYSTEMC_ALL_FV_LEVEL                    0x0024
#define SET_SYSTEMC_INT_ENABLE                      0x0040
#define LOG_END_OF_IMAGE                            0x0050
#define TEST1_WRITE_SYSTEMC                         0x0060
#define TEST2_WRITE_SYSTEMC                         0x0068
#define TEST3_WRITE_SYSTEMC                         0x0070
#define LOG_SET_THREAD_CPU                          0x0080
#define SET_SECONDARY_STARTUP_ADDRESS               0x0084
#define GENERATE_SWI                                0x0088
#define SWI_ACK                                     0x008C

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
