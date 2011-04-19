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

#ifndef _DBF_DEVICE_
#define _DBF_DEVICE_

#include <unistd.h>
#include <signal.h>

#include <slave_device.h>
#include <fpga.h>

class dbf_device_link;

typedef struct{
  unsigned char p[4];
  unsigned char q[4];
}s_qp;

#define NA             -1

#define Intra_4x4       0
#define Intra_16x16     1
#define Pred_L0         2
#define Pred_L1         3
#define BiPred          4
#define Direct          5

#define P_L0_16x16      0
#define P_L0_L0_16x8    1
#define P_L0_L0_8x16    2
#define P_8x8           3
#define P_8x8ref0       4
#define I_4x4           5
#define I_16x16_0_0_0   6
#define I_16x16_1_0_0   7
#define I_16x16_2_0_0   8
#define I_16x16_3_0_0   9
#define I_16x16_0_1_0  10
#define I_16x16_1_1_0  11
#define I_16x16_2_1_0  12
#define I_16x16_3_1_0  13
#define I_16x16_0_2_0  14
#define I_16x16_1_2_0  15
#define I_16x16_2_2_0  16
#define I_16x16_3_2_0  17
#define I_16x16_0_0_1  18
#define I_16x16_1_0_1  19
#define I_16x16_2_0_1  20
#define I_16x16_3_0_1  21
#define I_16x16_0_1_1  22
#define I_16x16_1_1_1  23
#define I_16x16_2_1_1  24
#define I_16x16_3_1_1  25
#define I_16x16_0_2_1  26
#define I_16x16_1_2_1  27
#define I_16x16_2_2_1  28
#define I_16x16_3_2_1  29
#define I_PCM          30
#define P_Skip       0xFF

#define P_L0_8x8       0
#define P_L0_8x4       1
#define P_L0_4x8       2
#define P_L0_4x4       3
#define B_Direct_8x8   4

#define IsIntra(m)  ((m)>=5 && (m)<=I_PCM)
#define CustomClip(i,min,max) (((i)<min)?min:(((i)>max)?max:(i)))
#define Clip(i) CustomClip(i,0,255)

#define N_SLICE_THREADS         4

#define REG_STATUS_BIT_ALLSLICES    (N_SLICE_THREADS)
#define REG_STATUS_BIT_SRAM         (N_SLICE_THREADS + 1)


const unsigned int MbWidth = 16;

class dbf_device : public sc_module
{
public:
    //constructor & destructor
    SC_HAS_PROCESS (dbf_device);
    dbf_device (sc_module_name module_name, unsigned int master_id/*, unsigned int slave_id*/);
    ~dbf_device ();

public:
    //threads & methods
    void request ();
    void response ();
    void process_slice ();

    void master_request();
    void master_response();

    void irq_update_thread();

public:
    //ports
    sc_port<VCI_GET_REQ_IF>                     get_port;
    sc_port<VCI_PUT_RSP_IF>                     put_port;
    sc_out <bool>                               irq;

    sc_port<VCI_PUT_REQ_IF>                     master_put_port;
    sc_port<VCI_GET_RSP_IF>                     master_get_port;

private:
    //members
    sc_event                                    m_ev_response;
    sc_event                                    m_ev_irqupdate;
    uint32_t                                    m_irq_stat;

    vci_response                                m_response;
    vci_request                                 m_request;

    dbf_device_link                             *m_dbf_link;


    /* For filtering process */
    unsigned char                               slice_number;

    unsigned int filter_slice (int slice);

    int get_bS (int qx, int qy, int px, int py);
    s_qp filter_block (int slice, int qx, int qy, int px, int py,
               int chroma_flag, s_qp qp, int bS);
    
    void filter_vertical_L (int slice, int x, int y);
    void filter_horizontal_L (int slice, int x, int y);
    void filter_vertical_C (int slice, int x, int y, int iCbCr);
    void filter_horizontal_C (int slice, int x, int y, int iCbCr);

    int get_PicWidthInMbs();
    int get_PicHeightInMbs();
    unsigned short get_Offset(unsigned char slice);
    unsigned short get_sliceLength(unsigned char slice);
    unsigned char get_L_pixel(int x, int y);
    void set_L_pixel(int x, int y,
             unsigned char val);
    unsigned char get_C_pixel(unsigned char iCbCr,
                  int x, int y);
    void set_C_pixel(unsigned char iCbCr, int x, int y,
             unsigned char val);
    char get_QP(int x, int y, unsigned char qp_type);
    unsigned char get_alpha(int slice);
    unsigned char get_beta(int slice);
    unsigned char get_mode(int x, int y);
    unsigned char get_TotalCoeffL(int x, int y);
    signed short get_MVx(int x, int y);
    signed short get_MVy(int x, int y);
    unsigned char get_format();
    unsigned char get_nbslices();

    void write_filtered_slice(unsigned char slice, int offset, int count);

    void handle_request ();
    unsigned char get_offset (unsigned char be, unsigned char &cnt);

    sc_mutex                MyMutex;
    unsigned char           nb_processed_slices;
    unsigned char           NB_SLICES;
    unsigned char           filter_initialised;
    unsigned int            m_master_id;
    /* unsigned int            m_slave_id; */
    unsigned long           m_etrace_id;
    int                     m_etrace_crt_mode;
};
    
#endif  /* _DBF_DEVICE_ */

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
