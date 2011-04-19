/*
 *  Copyright (c) 2009 Thales Communication - AAL
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

#ifndef _FPGA_
#define _FPGA_

#define FPGA_CHAR_SIZE 0x40000
#define MB_BLOCK_SIZE  0x00200
#define FRAME_OFFSET   0x40000
#define SLICE_OFFSET   0x10

#define LUMA_OFFSET    0x0      /* from 0x00000 to 0x000FF */
#define CHROMA_OFFSET  0x00100  /* from 0x00100 to 0x0017F */
#define MVX_OFFSET     0x00180  /* from 0x00180 to 0x0019F */
#define MVY_OFFSET     0x001A0  /* from 0x001A0 to 0x001BF */
#define COEFF_OFFSET   0x001C0  /* from 0x001C0 to 0x001CF */
#define QUANT_OFFSET   0x001D0  /* from 00x01D0 to 0x001D1 */
#define MODE_OFFSET    0x001D2

#define REG_GLO_VER    0x34000
#define REG_GLO_REV    0x34001
#define REG_GLO_DBG    0x34002
#define REG_GLO_STA    0x34003
#define REG_GLO_CTRL   0x34004

#define REG_FRA_STA    0x38000
#define REG_FRA_CTRL   0x38004
#define REG_FRA_SRAM   0x38008

#define REG_SLI_OFFSET 0x38010
#define REG_SLI_LGTOFF 0x38011
#define REG_SLI_LENGTH 0x38012
#define REG_SLI_CTRL   0x38013
#define REG_SLI_QPC    0x38014
#define REG_SLI_QPL    0x38015
#define REG_SLI_BETA   0x38016
#define REG_SLI_ALPHA  0x38017
#define REG_SLI_FILLOW 0x38018
#define REG_SLI_FILLHI 0x38019
#define REG_SLI_EMPLOW 0x3801A
#define REG_SLI_EMPHI  0x3801B
#define REG_SLI_CLEAR  0x3801C

#define REG_NSLICES                 0x39000 //read only
#define REG_INT_ACK                 0x39004
#define REG_INT_STATUS              0x39008 //read only
// REG_INT_ENABLE register:
//bit 0 .. N_SLICE_THREADS-1 = slice intr
//bit N_SLICE_THREADS = intr when all slices are finished
//bit N_SLICE_THREADS + 1 = intr when inf was copied to sram
//bit 7 = intr generated
#define REG_INT_ENABLE              0x3900C
#define REG_SRAM_ADDRESS            0x39010
#define REG_INC_ADDRESS             0x39014
#define REG_NSLICES_INTR            0x39018

#endif /* _FPGA_ */

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
