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

#include <dbf_device.h>
#include <etrace_if.h>

//#define DEBUG_LOG_SLICES
//#define DEBUG_DBF

#ifdef DEBUG_DBF
#define DPRINTF printf
#else
#define DPRINTF if (0) printf
#endif


#ifdef ENERGY_TRACE_ENABLED
#define ENERGY_PROCESS_SLICE    100
#define ENERGY_MASTER_COM       500
static uint64_t        etrace_dbf_class_mode_cost[] =
{0 * ENERGY_PROCESS_SLICE, 1 * ENERGY_PROCESS_SLICE,
 2 * ENERGY_PROCESS_SLICE, 3 * ENERGY_PROCESS_SLICE,
 4 * ENERGY_PROCESS_SLICE,
 0 * ENERGY_PROCESS_SLICE + ENERGY_MASTER_COM, 1 * ENERGY_PROCESS_SLICE + ENERGY_MASTER_COM,
 2 * ENERGY_PROCESS_SLICE + ENERGY_MASTER_COM, 3 * ENERGY_PROCESS_SLICE + ENERGY_MASTER_COM,
 4 * ENERGY_PROCESS_SLICE + ENERGY_MASTER_COM};
static periph_class_t  etrace_dbf_class =
{
    /*.class_name = */"DBF",
    /*.max_energy_ms = */400,
    /*.nmodes = */10,
    /*.nfvsets = */1,
    /*.mode_cost = */etrace_dbf_class_mode_cost
};
#endif

static const char alpha_t[52] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 4, 5, 6, 7, 8, 9, 10, 12, 13, 15, 17, 20, 22, 25, 28,
    32, 36, 40, 45, 50, 56, 63, 71, 80, 90, 101, 113, 127, 144, 162, 182,
    203, 226, 255, 255};

static const char beta_t[52] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 6, 6, 7, 7, 8, 8,
    9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18};

static const char tc0_t[3][52] = {
    /* bS == 1 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 6, 6, 7, 8,
        9, 10, 11, 13},
    /* bS == 2 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
        2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 7, 8, 8, 10, 11,
        12, 13, 15, 17},
    /* bS == 3 */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3,
        3, 3, 4, 4, 4, 5, 6, 6, 7, 8, 9, 10, 11, 13, 14, 16,
        18, 20, 23, 25}
};

static int PicWidthInMbs;

//FILE *tmp_out;

class dbf_device_link
{
public:
    dbf_device_link (sc_event *p, uint32_t *irq_stat);
    ~dbf_device_link ();
    void init ();
    void uninit ();

    sc_event m_ev_slice[N_SLICE_THREADS];
    sc_event m_ev_send_sram;

    unsigned char get_data (unsigned long address);
    void set_data (unsigned long addres, unsigned char data);
    void clear_slice (unsigned char slice);
    void write_out_slice (unsigned char slice);
public:
    unsigned char *m_data;
    sc_event *m_ev_irq;
    uint32_t *m_irq_stat;

};

dbf_device_link::dbf_device_link (sc_event *p, uint32_t *irq_stat)
{
    m_ev_irq   = p;
    m_irq_stat = irq_stat;
    m_data = NULL;
}

dbf_device_link::~dbf_device_link ()
{
    uninit ();
}

void
dbf_device_link::uninit ()
{
    delete [] m_data;
}

void
dbf_device_link::init ()
{
    m_data = new unsigned char[FPGA_CHAR_SIZE];
    memset (m_data, 0, FPGA_CHAR_SIZE);

    m_data[REG_NSLICES] = N_SLICE_THREADS;
    m_data[REG_INT_ENABLE] = 0; // (1 << REG_STATUS_BIT_ALLSLICES) | (1 << REG_STATUS_BIT_SRAM);
}

unsigned char
dbf_device_link::get_data (unsigned long address)
{
    if ((address < 0) || (address > FPGA_CHAR_SIZE))
    {
        cerr << "Error: wrong data access for addr = " << address << endl;
        exit (-1);
    }
    /*
      printf(" -- GET DATA -- data[%x] = %x\n",address,m_data[address]);
     */
    return m_data[address];
}

void
dbf_device_link::set_data (unsigned long address, unsigned char data)
{
    if ((address < 0) || (address > FPGA_CHAR_SIZE))
    {
        cerr << "Error: wrong data access for addr = " << address << endl;
        exit (-1);
    }

    if (address == REG_INT_STATUS || address == REG_NSLICES)
        return;

    if (address == REG_INT_ENABLE)
    {
        unsigned char status = m_data[REG_INT_STATUS];
        unsigned char enable = m_data[REG_INT_ENABLE];

        data &= 0x7F;
        if (!(status & enable) && (status & data))
        {
            *m_irq_stat = 1;
            m_ev_irq->notify();
        }
        else
        {
            if ((status & enable) && !(status & data))
            {
                *m_irq_stat = 0;
                m_ev_irq->notify();
            }
        }

        m_data[REG_INT_ENABLE] = data;

        return;
    }

    m_data[address] = data;

    int i;
    if (address == REG_INT_ACK)
    {
        unsigned char status = m_data[REG_INT_STATUS];
        unsigned char enable = m_data[REG_INT_ENABLE];
        if (data & status & enable)
        {
            status &= (~data | 0x80);

            if (status == 0x80)
                status = 0;

            m_data[REG_INT_STATUS] = status;
            if (!(status & enable))
            {
                *m_irq_stat = 0;
                m_ev_irq->notify();
            }
        }
        return;
    }

    for (i = 0; i < N_SLICE_THREADS; i++)
        if (address == (REG_SLI_FILLOW + SLICE_OFFSET * i))
        {
            #ifdef DEBUG_LOG_SLICES
            write_out_slice (i);
            #endif

            m_ev_slice[i].notify (0, SC_NS);
            return;
        }

    for (i = 0; i < N_SLICE_THREADS; i++)
        if (address == (REG_SLI_CLEAR + SLICE_OFFSET * i))
        {
            clear_slice (i);
            m_data[address] = 0;
            return;
        }

    if (address == REG_FRA_SRAM && data == 1)
    {
        m_ev_send_sram.notify (0, SC_NS);
        return;
    }
}

void
dbf_device_link::write_out_slice (unsigned char slice)
{
    FILE *file_out;
    int i, j = 0;
    int reg_addr = 0;
    char file_name[50];

    sprintf (file_name, "file_out_%d_beforefilter.txt", slice);

    file_out = fopen (file_name, "w");
    if (file_out == NULL)
        printf ("Error while opening output file\n");

    for (j = slice * 198; j < slice * 198 + 198; j++)
    {
        for (i = 0; i < 0x17F; i++)
        {
            reg_addr = j * 0x200 + i;
            fprintf (file_out, "data[%d] = %d\n", reg_addr, m_data[reg_addr]);
        }
    }

    fclose (file_out);
}

void
dbf_device_link::clear_slice (unsigned char slice)
{
    int reg_addr = 0;
    unsigned short offset = 0;
    unsigned short length = 0;

    reg_addr = SLICE_OFFSET * slice + REG_SLI_LGTOFF;
    offset = m_data[reg_addr];
    offset = offset & 0x000F;
    offset = offset << 8;
    reg_addr = SLICE_OFFSET * slice + REG_SLI_OFFSET;
    offset = offset + (m_data[reg_addr] & 0x00FF);

    reg_addr = SLICE_OFFSET * slice + REG_SLI_LGTOFF;
    length = m_data[reg_addr + 1];
    length = length << 4;
    length = length + ((m_data[reg_addr] & 0xF0) >> 4);

    reg_addr = offset * MB_BLOCK_SIZE;

    while (reg_addr < (offset + length) * MB_BLOCK_SIZE)
    {
        if ((reg_addr % MB_BLOCK_SIZE == QUANT_OFFSET) || (reg_addr % MB_BLOCK_SIZE == QUANT_OFFSET + 1))
            m_data[reg_addr] = 0xFF;
        else
            m_data[reg_addr] = 0;
        reg_addr++;
    }
}

#define SC_THREAD_WITH_NAME(func, name)                    \
  declare_thread_process (func ## _handle, name, SC_CURRENT_USER_MODULE, func)

//constructor & destructor

dbf_device::dbf_device (sc_module_name module_name,
    unsigned int master_id /*, unsigned int slave_id*/)
: sc_module (module_name)
{
    m_master_id = master_id;

    // m_slave_id = slave_id;
    m_irq_stat = 0;

    m_dbf_link = new dbf_device_link (&m_ev_irqupdate, &m_irq_stat);

    SC_THREAD (request);
    SC_THREAD (response);

    int i;
    char sname[100];
    for (i = 0; i < N_SLICE_THREADS; i++)
    {
        sprintf (sname, "process_slice_%d", i);
        SC_THREAD_WITH_NAME (process_slice, sname);
    }

    SC_THREAD (master_request);
    SC_THREAD (master_response);

    SC_THREAD (irq_update_thread);

    //tmp_out = fopen("tmp_out.txt","w");

    nb_processed_slices = 0;
    NB_SLICES = 0;
    filter_initialised = 0;

    #ifdef ENERGY_TRACE_ENABLED
    int etrace_group_id = -1;
    m_etrace_id = etrace.add_periph (name (), &etrace_dbf_class,
        etrace_group_id, name ());
    etrace.change_energy_mode (m_etrace_id, 0);
    m_etrace_crt_mode = 0;
    #endif
}

dbf_device::~dbf_device ()
{
    if (m_dbf_link)
        delete m_dbf_link;
    m_dbf_link = NULL;
}

//methods

void dbf_device::request ()
{
    // Internal declarations
    unsigned char be;

    // Initialisations
    m_dbf_link->init ();

    // Process
    while (1)
    {
        get_port->get (m_request);

        wait (3, SC_NS);

        // Get the adress and transform it so as to access the data array
        be = m_request.be;

        // Selects the operation requested
        if (be)
        {
            handle_request ();
            m_ev_response.notify (0, SC_NS);
        }
        else
        {
            printf ("Error: be=0 in %s, addr=%lu\n", __FUNCTION__,
                (unsigned long) m_request.initial_address);
        }
    } // end while(1)
}

void dbf_device::handle_request ()
{
    // Internal declarations
    unsigned long address;
    unsigned char be, cmd, cnt;
    unsigned char offset;
    int i = 0;

    // Get the adress and transform it so as to access the data array
    address = m_request.address;
    cmd = m_request.cmd;
    be = m_request.be;

    switch (cmd)
    {
    case 0x01: //read
        offset = get_offset (be, cnt);
        for (i = offset; i < offset + cnt; i++)
            m_response.rdata[i] = m_dbf_link->get_data (address + i);
        m_response.rerror = false;
        m_response.rbe = be;
        break;

    case 0x02: //write
        offset = get_offset (be, cnt);
        for (i = offset; i < offset + cnt; i++)
            m_dbf_link->set_data (address + i, m_request.wdata[i]);
        m_response.rerror = false;
        m_response.rbe = 0;
        break;

    default:
        cerr << "ACCESS WITH CODE " << hex << cmd << " NOT SUPPPORTED!" << endl;
    }

    m_response.rsrcid = m_request.srcid;
    m_response.rtrdid = m_request.trdid;
    m_response.reop = 1;
}

unsigned char dbf_device::get_offset (unsigned char be, unsigned char &cnt)
{
    unsigned char offs = 0;

    cnt = 0;
    while ((be & 0x01) == 0)
    {
        be = be >> 1;
        offs++;
    }

    while ((be & 0x01) == 1)
    {
        be = be >> 1;
        cnt++;
    }

    return offs;
}

void dbf_device::response ()
{
    while (1)
    {
        wait (m_ev_response);
        //printf("send response\n");
        put_port->put (m_response);
    }
}

void dbf_device::process_slice ()
{
    static int s_cnt_slices = 0;
    int slice_id = s_cnt_slices++;
    unsigned short slice_length = 0;
    unsigned char tmp;
    unsigned int i, curr_mb = 0;

    if (slice_id == 0)
    {
        m_irq_stat = 0;
        m_ev_irqupdate.notify();
    }

    while (1)
    {
        wait (m_dbf_link->m_ev_slice[slice_id]);

        DPRINTF ("-- FILTER -- Start filter for slice %d\n", slice_id);

        NB_SLICES = get_nbslices ();

        curr_mb = filter_slice (slice_id);

        slice_length = get_sliceLength (slice_id);
        tmp = slice_length >> 8;
        m_dbf_link->set_data (REG_SLI_EMPHI + SLICE_OFFSET * slice_id, tmp);
        tmp = slice_length & 0x00FF;
        m_dbf_link->set_data (REG_SLI_EMPLOW + SLICE_OFFSET * slice_id, tmp);

        DPRINTF ("-- FILTER -- End filter for slice %d\n", slice_id);

        m_dbf_link->m_data[REG_INT_STATUS] |= (1 << slice_id);
        tmp = 0;
        for (i = 0; i < N_SLICE_THREADS; i++)
            if (m_dbf_link->m_data[REG_SLI_EMPHI + SLICE_OFFSET * i] != 0 ||
                m_dbf_link->m_data[REG_SLI_EMPLOW + SLICE_OFFSET * i] != 0)
                tmp++;

        if (tmp >= m_dbf_link->m_data[REG_NSLICES_INTR])
        {
            m_dbf_link->m_data[REG_INT_STATUS] |= 1 << REG_STATUS_BIT_ALLSLICES;
        }

        if (m_dbf_link->m_data[REG_INT_STATUS] & m_dbf_link->m_data[REG_INT_ENABLE])
        {
            m_dbf_link->m_data[REG_INT_STATUS] |= 0x80;
            m_irq_stat = 1;
            m_ev_irqupdate.notify();
        }
    }
}

void
dbf_device::irq_update_thread(void)
{

    while(1)
    {
        wait(m_ev_irqupdate);

        if (m_irq_stat)
        {
            DPRINTF("Raising IRQ\n");
            irq = 1;
        }
        else
        {
            DPRINTF("Clearing IRQ\n");
            irq = 0;
        }
    }
    return;
}

unsigned int dbf_device::filter_slice (int slice)
{
    /*************************************
          Internal declarations
     *************************************/
    int CurrMbAddr = 0;
    int MbCount = 0;
    int mb_pos_x, mb_pos_y = 0;
    int OffsetMb = 0;
    int x_first, y_first = 0;
    int PicHeightInMbs = 0;

    #ifdef ENERGY_TRACE_ENABLED
    etrace.change_energy_mode (this->m_etrace_id, ++m_etrace_crt_mode);
    #endif

    /*************************************
              Internal declaration
     *************************************/
    PicWidthInMbs = get_PicWidthInMbs ();
    PicHeightInMbs = get_PicHeightInMbs ();
    MbCount = get_sliceLength (slice);
    /* Selection of boundaries depending on slice number */
    OffsetMb = get_Offset (slice);

    x_first = (OffsetMb % PicWidthInMbs) << 4;
    y_first = (OffsetMb / PicWidthInMbs) << 4;

    /*
      printf("PicWidthInMbs = %d, MbCount = %d, OffsetMb = %d, x_first = %d, y_first = %d\n",
      PicWidthInMbs, MbCount, OffsetMb, x_first, y_first);
     */

    /* Main process */
    for (CurrMbAddr = OffsetMb; CurrMbAddr < (MbCount + OffsetMb); CurrMbAddr++)
    {
        //fprintf(tmp_out,"filter mb = %d\n",CurrMbAddr);

        /* Position of the upper left pixel of the 16x16 MB
       considered here */
        mb_pos_x = (CurrMbAddr % PicWidthInMbs) << 4;
        mb_pos_y = (CurrMbAddr / PicWidthInMbs) << 4;

        /****************/
        /* LUMA SAMPLES */
        /****************/
        /* L vertical edges filtering (from left to right) */
        int vertical_edge;
        for (vertical_edge = 0; vertical_edge < 4; vertical_edge++)
        {
            /* skip left boundary of the slice */
            if (mb_pos_x == 0 || (mb_pos_x == x_first && mb_pos_y == y_first))
                continue;
            /* iterates on the 16 lines of the Mb */
            int lines;
            for (lines = 0; lines < 16; lines++)
            {
                filter_vertical_L (slice, mb_pos_x + (vertical_edge << 2), mb_pos_y + lines);
            }
        }

        /* L horizontal edges filtering (from top to bottom) */
        int horizontal_edge;
        for (horizontal_edge = 0; horizontal_edge < 4; horizontal_edge++)
        {
            /* skip top boundary of the slice */
            if (mb_pos_x < x_first)
            {
                if (mb_pos_y <= (y_first + 16))
                    continue;
            }
            else
                if (mb_pos_y <= y_first)
                continue;

            /* iterates on the 16 columns of the Mb */
            int cols;
            for (cols = 0; cols < 16; cols++)
                filter_horizontal_L (slice, mb_pos_x + cols, mb_pos_y + (horizontal_edge << 2));
        }

        /******************/
        /* CHROMA SAMPLES */
        /******************/
        int iCbCr;
        for (iCbCr = 0; iCbCr < 2; iCbCr++)
        {
            /* C vertical edges filtering */
            int vertical_edge;
            for (vertical_edge = 0; vertical_edge < 2; vertical_edge++)
            {
                /* skip left boundary of the slice */
                if (mb_pos_x == 0 || (mb_pos_x == x_first && mb_pos_y == y_first))
                    continue;
                int lines;
                for (lines = 0; lines < 8; lines++)
                    filter_vertical_C (slice, mb_pos_x + (vertical_edge << 2), mb_pos_y + lines, iCbCr);
            }

            /* C horizontal edges filtering */
            int horizontal_edge;
            for (horizontal_edge = 0; horizontal_edge < 2; horizontal_edge++)
            {
                /* skip top boundary of the slice */
                if (mb_pos_x < x_first)
                {
                    if (mb_pos_y <= (y_first + 16))
                        continue;
                }
                else
                    if (mb_pos_y <= y_first)
                    continue;

                int cols;
                for (cols = 0; cols < 8; cols++)
                    filter_horizontal_C (slice, mb_pos_x + cols, mb_pos_y + (horizontal_edge << 2), iCbCr);
            }
        }
    }

    #ifdef DEBUG_LOG_SLICES
    write_filtered_slice (slice, OffsetMb, MbCount);
    #endif

    //fclose(tmp_out);

    wait (15, SC_MS);

    #ifdef ENERGY_TRACE_ENABLED
    etrace.change_energy_mode (m_etrace_id, --m_etrace_crt_mode);
    #endif

    return CurrMbAddr;
}

//----------------------------------------------------------------
/*!
  Calculation of the boundary strength : 0, 1, 2, 3 or 4.
 */
//----------------------------------------------------------------

int dbf_device::get_bS (int qx, int qy, int px, int py)
{
    /* filtering strength */
    int bS = 0;

    if (qx % 16 == 0 && (IsIntra (get_mode (qx, qy)) || IsIntra (get_mode (px, py)))) /* intra */
        bS = 4;
    else if (IsIntra (get_mode (qx, qy))) /* intra */
        bS = 3;
    else if (get_TotalCoeffL (qx >> 2, qy >> 2) != 0 || get_TotalCoeffL (px >> 2, py >> 2) != 0)
        bS = 2;
    else if (abs (get_MVx (qx >> 2, qy >> 2) - get_MVx (px >> 2, py >> 2)) > 4 ||
        abs (get_MVy (qx >> 2, qy >> 2) - get_MVy (px >> 2, py >> 2)) > 4)
        bS = 1;

    //fprintf(tmp_out,"bs = %d and %d and %d\n",bS,qx,qy);

    return bS;
}

s_qp dbf_device::filter_block (int slice, int qx,
    int qy, int px, int py, int chroma_flag, s_qp qp, int bS)
{
    /* Internal declaration */
    s_qp res = qp;
    int QPq, QPp = 0; /* threshold */
    unsigned short alpha_off, beta_off = 0;

    /* Quantization parameters */
    if (chroma_flag == 0)
    {
        QPq = get_QP (qx >> 4, qy >> 4, 0);
        QPp = get_QP (px >> 4, py >> 4, 0);
        //fprintf(tmp_out,"%d %d\n",QPq, QPp);
        //fprintf(tmp_out,"QPp = %d for %d %d\n",QPq, qx>>4, qy>>4);
    }
    else
    {
        QPq = get_QP (qx >> 4, qy >> 4, 1);
        QPp = get_QP (px >> 4, py >> 4, 1);
        //fprintf(tmp_out,"%d %d\n",QPq, QPp);
        //fprintf(tmp_out,"QPp = %d for %d %d\n",QPq, qx>>4, qy>>4);
    }
    int QPav = (QPq + QPp + 1) >> 1;

    /* Offsets */
    alpha_off = get_alpha (slice);
    beta_off = get_beta (slice);

    int indexA = CustomClip (QPav + (alpha_off << 2), 0, 51);
    int indexB = CustomClip (QPav + (beta_off << 2), 0, 51);
    int alpha = alpha_t[indexA];
    int beta = beta_t[indexB];

    int filterSampleFlag = (bS != 0
        && abs (qp.p[0] - qp.q[0]) < alpha
        && abs (qp.p[1] - qp.p[0]) < beta
        && abs (qp.q[1] - qp.q[0]) < beta);

    if (filterSampleFlag == 1 && bS < 4)
    {
        int tc0, Ap, Aq, tc, delta;

        Ap = abs (qp.p[2] - qp.p[0]);
        Aq = abs (qp.q[2] - qp.q[0]);
        tc0 = tc0_t[bS - 1][indexA];

        if (chroma_flag == 0)
            tc = tc0 + ((Ap < beta) ? 1 : 0) + ((Aq < beta) ? 1 : 0);
        else
            tc = tc0 + 1;

        delta = CustomClip (((((qp.q[0] - qp.p[0]) << 2) + (qp.p[1] - qp.q[1]) + 4) >> 3), -tc, tc);

        res.p[0] = Clip (qp.p[0] + delta);
        res.q[0] = Clip (qp.q[0] - delta);


        if (Ap < beta && chroma_flag == 0)
            res.p[1] += CustomClip ((qp.p[2] + ((qp.p[0] + qp.q[0] + 1) >> 1) - (qp.p[1] << 1)) >> 1, -tc0, tc0);

        if (Aq < beta && chroma_flag == 0)
            res.q[1] += CustomClip ((qp.q[2] + ((qp.p[0] + qp.q[0] + 1) >> 1) - (qp.q[1] << 1)) >> 1, -tc0, tc0);

    }
    else if (filterSampleFlag == 1 && bS == 4)
    {
        int Ap, Aq;

        Ap = abs (qp.p[2] - qp.p[0]);
        Aq = abs (qp.q[2] - qp.q[0]);

        if (chroma_flag == 0 && Ap < beta && abs (qp.p[0] - qp.q[0]) < ((alpha >> 2) + 2))
        {
            res.p[0] = (qp.p[2] + 2 * qp.p[1] + 2 * qp.p[0] + 2 * qp.q[0] + qp.q[1] + 4) >> 3;
            res.p[1] = (qp.p[2] + qp.p[1] + qp.p[0] + qp.q[0] + 2) >> 2;
            res.p[2] = (2 * qp.p[3] + 3 * qp.p[2] + qp.p[1] + qp.p[0] + qp.q[0] + 4) >> 3;
        }
        else
        {
            res.p[0] = (2 * qp.p[1] + qp.p[0] + qp.q[1] + 2) >> 2;
        }
        if (chroma_flag == 0 && Aq < beta && abs (qp.p[0] - qp.q[0]) < ((alpha >> 2) + 2))
        {
            res.q[0] = (qp.p[1] + 2 * qp.p[0] + 2 * qp.q[0] + 2 * qp.q[1] + qp.q[2] + 4) >> 3;
            res.q[1] = (qp.p[0] + qp.q[0] + qp.q[1] + qp.q[2] + 2) >> 2;
            res.q[2] = (2 * qp.q[3] + 3 * qp.q[2] + qp.q[1] + qp.q[0] + qp.p[0] + 4) >> 3;
        }
        else
        {
            res.q[0] = (2 * qp.q[1] + qp.q[0] + qp.p[1] + 2) >> 2;
        }
    }

    return res;
}

//----------------------------------------------------------------
/*!
  Applies the deblocking filter on a maximum set of 8 pixels
  around the edge.
 */
//----------------------------------------------------------------

void dbf_device::filter_vertical_L (int slice, int x, int y)
{
    /* Internal declarations */
    s_qp qp;
    int i = 0;

    /* stores pixels around the edge in specific structure */
    for (i = 0; i < 4; i++)
    {
        qp.q[i] = get_L_pixel (x + i, y);
        //fprintf(tmp_out,"pix = %d at %d %d\n",qp.q[i],x+i,y);
        qp.p[i] = get_L_pixel (x - i - 1, y);
        //fprintf(tmp_out,"pix = %d at %d %d\n",qp.p[i],x-i-1,y);
    }

    int bS = get_bS (x, y, x - 16, y);

    s_qp res = filter_block (slice, x, y, x - 16, y, 0, qp, bS);

    /* writes back filtered pixels into the frame buffer */
    for (i = 0; i < 4; i++)
    {
        set_L_pixel (x + i, y, res.q[i]);
        //fprintf(tmp_out,"set pix = %d at %d %d\n",res.q[i],x+i,y);
        set_L_pixel (x - i - 1, y, res.p[i]);
        //fprintf(tmp_out,"set pix = %d at %d %d\n",res.p[i],x-i-1,y);
    }
}

void dbf_device::filter_horizontal_L (int slice, int x, int y)
{
    /* Internal declaration */
    s_qp qp;
    int i = 0;

    for (i = 0; i < 4; i++)
    {
        qp.q[i] = get_L_pixel (x, y + i);
        qp.p[i] = get_L_pixel (x, y - i - 1);
    }

    int bS = get_bS (x, y, x, y - 16);

    s_qp res = filter_block (slice, x, y, x, y - 16, 0, qp, bS);

    for (i = 0; i < 4; i++)
    {
        set_L_pixel (x, y + i, res.q[i]);
        //printf("pix = %d at %d %d\n",res.q[i],x,y+i);
        set_L_pixel (x, y - i - 1, res.p[i]);
        //printf("pix = %d at %d %d\n",res.p[i],x,y-i-1);
    }
}

/* Chroma */
void dbf_device::filter_vertical_C (int slice, int x, int y, int iCbCr)
{
    /* Internal declaration */
    s_qp qp;
    int i = 0;

    for (i = 0; i < 4; i++)
    {
        qp.q[i] = get_C_pixel (iCbCr, (x >> 1) + i, (y >> 1));
        //printf("pix = %d at %d %d\n",qp.q[i],(x>>1)+i,(y>>1));
        qp.p[i] = get_C_pixel (iCbCr, (x >> 1) - i - 1, (y >> 1));
        //printf("pix = %d at %d %d\n",qp.p[i],(x>>1)-i-1,(y>>1));
    }

    int bS = get_bS (x, y, x - 16, y);

    s_qp res = filter_block (slice, x, y, x - 16, y, 1, qp, bS);

    for (i = 0; i < 4; i++)
    {
        set_C_pixel (iCbCr, (x >> 1) + i, (y >> 1), res.q[i]);
        //printf("pix = %d at %d %d\n",res.q[i],(x>>1)+i,(y>>1));
        set_C_pixel (iCbCr, (x >> 1) - i - 1, (y >> 1), res.p[i]);
        //printf("pix = %d at %d %d\n",res.q[i],(x>>1)-i-1,(y>>1));
    }
}

void dbf_device::filter_horizontal_C (int slice, int x, int y, int iCbCr)
{
    /* Internal declaration */
    s_qp qp;
    int i;

    for (i = 0; i < 4; i++)
    {
        qp.q[i] = get_C_pixel (iCbCr, (x >> 1), (y >> 1) + i);
        qp.p[i] = get_C_pixel (iCbCr, (x >> 1), (y >> 1) - i - 1);
    }

    int bS = get_bS (x, y, x, y - 16);

    s_qp res = filter_block (slice, x, y, x, y - 16, 1, qp, bS);

    for (i = 0; i < 4; i++)
    {
        set_C_pixel (iCbCr, (x >> 1), (y >> 1) + i, res.q[i]);
        set_C_pixel (iCbCr, (x >> 1), (y >> 1) - i - 1, res.p[i]);
    }
}

int dbf_device::get_PicWidthInMbs ()
{
    unsigned char format = get_format ();
    int ret = 0;

    switch (format)
    {
    case 0: /* SQCIF */
        ret = 128 / MbWidth;
        break;
    case 1: /* QCIF */
        ret = 176 / MbWidth;
        break;
    case 2: /* CIF */
        ret = 352 / MbWidth;
        break;
    case 3: /* 4CIF */
        ret = 704 / MbWidth;
        break;
    default:
        printf ("Format of the stream has not been intialised !!\n");
    }

    return ret;
}

int dbf_device::get_PicHeightInMbs ()
{
    unsigned char format = get_format ();
    int ret = 0;

    switch (format)
    {
    case 0: /* SQCIF */
        ret = 96 / MbWidth;
        break;
    case 1: /* QCIF */
        ret = 144 / MbWidth;
        break;
    case 2: /* CIF */
        ret = 288 / MbWidth;
        break;
    case 3: /* 4CIF */
        ret = 576 / MbWidth;
        break;
    default:
        printf ("Format of the stream has not been intialised !!\n");
    }

    return ret;
}

unsigned short dbf_device::get_Offset (unsigned char slice)
{
    int reg_addr = 0;
    unsigned short ret = 0;

    reg_addr = SLICE_OFFSET * slice + REG_SLI_LGTOFF;
    ret = m_dbf_link->get_data (reg_addr);
    ret = ret & 0x000F;
    ret = ret << 8;
    reg_addr = SLICE_OFFSET * slice + REG_SLI_OFFSET;
    ret = ret + (m_dbf_link->get_data (reg_addr) & 0x00FF);

    return ret;
}

unsigned short dbf_device::get_sliceLength (unsigned char slice)
{
    int reg_addr = 0;
    unsigned short ret = 0;

    reg_addr = SLICE_OFFSET * slice + REG_SLI_LGTOFF;
    ret = m_dbf_link->get_data (reg_addr + 1);
    ret = ret << 4;
    ret = ret + ((m_dbf_link->get_data (reg_addr) & 0xF0) >> 4);

    return ret;
}

unsigned char dbf_device::get_L_pixel (int x, int y)
{
    /* Internal declaration */
    unsigned char pix = 0;
    int mb_x, mb_y = 0;
    int pix_x, pix_y = 0;
    int reg_addr = 0;
    int mb_pos, pix_pos = 0;

    /* Calculation */
    mb_x = x / 16; /* position of the mb in image*/
    mb_y = y / 16; /* position of the mb in image*/
    pix_x = x % 16; /* position of the pix in mb */
    pix_y = y % 16; /* position of the pix in mb */
    mb_pos = mb_y * PicWidthInMbs + mb_x;
    pix_pos = pix_y * MbWidth + pix_x;

    reg_addr = MB_BLOCK_SIZE * mb_pos + pix_pos + LUMA_OFFSET;

    pix = m_dbf_link->get_data (reg_addr);

    return pix;
}

void dbf_device::set_L_pixel (int x, int y,
    unsigned char val)
{
    /* Internal declaration */
    int mb_x, mb_y = 0;
    int pix_x, pix_y = 0;
    int reg_addr = 0;
    int mb_pos, pix_pos = 0;

    /* Calculation */
    mb_x = x / 16; /* position of the mb in image*/
    mb_y = y / 16; /* position of the mb in image*/
    pix_x = x % 16; /* position of the pix in mb */
    pix_y = y % 16; /* position of the pix in mb */
    mb_pos = mb_y * PicWidthInMbs + mb_x;
    pix_pos = pix_y * MbWidth + pix_x;

    reg_addr = MB_BLOCK_SIZE * mb_pos + pix_pos + LUMA_OFFSET;

    m_dbf_link->set_data (reg_addr, val);
}

unsigned char dbf_device::get_C_pixel (unsigned char iCbCr,
    int x, int y)

{
    /* Internal declaration */
    unsigned char pix = 0;
    int mb_x, mb_y = 0;
    int pix_x, pix_y = 0;
    int reg_addr = 0;
    int mb_pos, pix_pos = 0;

    /* Calculation */
    mb_x = x / 8; /* position of the mb in image*/
    mb_y = y / 8; /* position of the mb in image*/
    pix_x = x % 8; /* position of the pix in mb */
    pix_y = y % 8; /* position of the pix in mb */
    mb_pos = mb_y * PicWidthInMbs + mb_x;
    pix_pos = pix_y * MbWidth / 2 + pix_x;

    reg_addr = MB_BLOCK_SIZE * mb_pos + CHROMA_OFFSET + 64 * iCbCr + pix_pos;

    pix = m_dbf_link->get_data (reg_addr);

    return pix;
}

void dbf_device::set_C_pixel (unsigned char iCbCr, int x, int y,
    unsigned char val)
{
    /* Internal declaration */
    int mb_x, mb_y = 0;
    int pix_x, pix_y = 0;
    int reg_addr = 0;
    int mb_pos, pix_pos = 0;

    /* Calculation */
    mb_x = x / 8; /* position of the mb in image*/
    mb_y = y / 8; /* position of the mb in image*/
    pix_x = x % 8; /* position of the pix in mb */
    pix_y = y % 8; /* position of the pix in mb */
    mb_pos = mb_y * PicWidthInMbs + mb_x;
    pix_pos = pix_y * MbWidth / 2 + pix_x;

    reg_addr = MB_BLOCK_SIZE * mb_pos + CHROMA_OFFSET + 64 * iCbCr + pix_pos;

    m_dbf_link->set_data (reg_addr, val);
}

char dbf_device::get_QP (int x, int y, unsigned char qp_type)
{
    /* Internal declaration */
    char ret = 0;
    int reg_addr = 0;
    int mb_pos = 0;

    /* Calculation */
    mb_pos = y * PicWidthInMbs + x;

    if (qp_type == 0) /* QPy */
        reg_addr = MB_BLOCK_SIZE * mb_pos + QUANT_OFFSET;
    else /* QPc */
        reg_addr = MB_BLOCK_SIZE * mb_pos + QUANT_OFFSET + 1;

    ret = m_dbf_link->get_data (reg_addr);

    return ret;
}

unsigned char dbf_device::get_alpha (int slice)
{
    /* Internal declaration */
    unsigned char ret;
    int reg_addr;

    reg_addr = SLICE_OFFSET * slice + REG_SLI_ALPHA;
    ret = m_dbf_link->get_data (reg_addr);

    return ret;
}

unsigned char dbf_device::get_beta (int slice)
{
    /* Internal declaration */
    unsigned char ret;
    int reg_addr;

    reg_addr = SLICE_OFFSET * slice + REG_SLI_BETA;
    ret = m_dbf_link->get_data (reg_addr);

    return ret;
}

unsigned char dbf_device::get_mode (int x, int y)
{
    unsigned char ret = 0;
    int reg_addr = 0;
    int mb_pos = 0;

    mb_pos = (y >> 4) * PicWidthInMbs + (x >> 4);
    reg_addr = MB_BLOCK_SIZE * mb_pos + MODE_OFFSET;
    ret = m_dbf_link->get_data (reg_addr);

    return ret;
}

unsigned char dbf_device::get_TotalCoeffL (int x, int y)
{
    /* Internal declaration */
    unsigned char ret = 0;
    int reg_addr = 0;
    int mb_x, mb_y = 0;
    int mb_pos, pos_4x4 = 0;

    /* Calculation */
    mb_x = x / 4;
    mb_y = y / 4;
    mb_pos = mb_y * PicWidthInMbs + mb_x;
    pos_4x4 = (y - (mb_y * 4))*4 + (x - (mb_x * 4));

    reg_addr = MB_BLOCK_SIZE * mb_pos + COEFF_OFFSET + pos_4x4;
    ret = m_dbf_link->get_data (reg_addr);

    return ret;
}

signed short dbf_device::get_MVx (int x, int y)
{
    /* Internal declaration */
    signed short mv = 0;
    int reg_addr = 0;
    int mb_x, mb_y = 0;
    int mb_pos, pos_4x4 = 0;
    unsigned char tmp_c1, tmp_c2 = 0;

    /* Calculation */
    mb_x = x / 4;
    mb_y = y / 4;
    mb_pos = mb_y * PicWidthInMbs + mb_x;
    pos_4x4 = (y - (mb_y * 4))*4 + (x - (mb_x * 4));

    reg_addr = MB_BLOCK_SIZE * mb_pos + MVX_OFFSET + pos_4x4 * 2; /* pos4x4*2 because short */
    tmp_c1 = m_dbf_link->get_data (reg_addr + 1);
    tmp_c2 = m_dbf_link->get_data (reg_addr);
    mv = tmp_c1;
    mv = mv << 8;
    mv = mv + tmp_c2;

    return mv;
}

signed short dbf_device::get_MVy (int x, int y)
{
    /* Internal declaration */
    signed short mv = 0;
    int reg_addr = 0;
    int mb_x, mb_y = 0;
    int mb_pos, pos_4x4 = 0;
    unsigned char tmp_c1, tmp_c2 = 0;

    /* Calculation */
    mb_x = x / 4;
    mb_y = y / 4;
    mb_pos = mb_y * PicWidthInMbs + mb_x;
    pos_4x4 = (y - (mb_y * 4))*4 + (x - (mb_x * 4));

    reg_addr = MB_BLOCK_SIZE * mb_pos + MVY_OFFSET + pos_4x4 * 2; /* pos4x4*2 because short */
    tmp_c1 = m_dbf_link->get_data (reg_addr + 1);
    tmp_c2 = m_dbf_link->get_data (reg_addr);
    mv = tmp_c1;
    mv = mv << 8;
    mv = mv + tmp_c2;

    return mv;
}

unsigned char dbf_device::get_format ()
{
    unsigned char ret;

    ret = m_dbf_link->get_data (REG_GLO_CTRL);
    ret = ret & 0x30;
    ret = ret >> 4;

    return ret;
}

unsigned char dbf_device::get_nbslices ()
{
    unsigned char ret_val;

    ret_val = m_dbf_link->get_data (REG_FRA_CTRL);
    ret_val = ret_val & 0x0F;

    return ret_val;
}

void dbf_device::write_filtered_slice (unsigned char slice, int offset, int count)
{
    // Internal declarations
    FILE *file_out;
    int i, j = 0;
    int reg_addr = 0;
    char file_name[50];

    // Internal initialisations
    sprintf (file_name, "file_out_%d_afterfilter", slice);
    file_out = fopen (file_name, "w");
    if (file_out == NULL)
    {
        printf ("-- DBF -- Error during opening of the file\n");
        exit (0);
    }

    // Copy of data to the file
    for (j = offset; j < offset + count; j++)
    {
        for (i = 0; i < 0x17F; i++)
        {
            reg_addr = j * 0x200 + i;
            fprintf (file_out, "data[%d] = %d\n", reg_addr, m_dbf_link->get_data (reg_addr));
        }
    }

    fclose (file_out);
}

#define SEND_PACK                                   \
  if (b8alg) {                                      \
    req.be = 0x0F;                                  \
    * (unsigned long *) (req.wdata + 0) = * (unsigned long *) (m_dbf_link->m_data + reg_addr); \
  } else {                                          \
    req.be = 0xF0;                                  \
    * (unsigned long *) (req.wdata + 4) = * (unsigned long *) (m_dbf_link->m_data + reg_addr); \
  }                                                 \
  if (bIncrementAddress) {                          \
    req.address = adr_dest & 0xFFFFFFF8;            \
    adr_dest += 4;                                  \
    b8alg = !b8alg;                                 \
  } else                                            \
    req.address = adr_dest;                         \
    req.plen = 1;                                   \
  master_put_port->put (req)

void dbf_device::master_request ()
{
    // Internal declarations
    int nb_mb_height;
    int nb_mb_width;
    int mb_height = 16;
    int mb_width = 16;
    int k, l, x, y = 0;
    int i = 0;
    vci_request req;
    unsigned int reg_addr = 0;
    unsigned char tmp = 0;
    bool b8alg, bIncrementAddress;
    unsigned long adr_dest;

    req.trdid = 1;
    req.srcid = m_master_id;
    req.cmd = 0x02;
    req.eop = 1;

    while (1)
    {
        wait (m_dbf_link->m_ev_send_sram);

        nb_mb_height = get_PicHeightInMbs ();
        nb_mb_width = get_PicWidthInMbs ();

        #ifdef ENERGY_TRACE_ENABLED
        m_etrace_crt_mode += N_SLICE_THREADS + 1;
        etrace.change_energy_mode (m_etrace_id, m_etrace_crt_mode);
        #endif

        adr_dest = *(unsigned long *) (m_dbf_link->m_data + REG_SRAM_ADDRESS);
        bIncrementAddress = (m_dbf_link->m_data[REG_INC_ADDRESS] != 0);
        if ((adr_dest & 0x3) != 0)
        {
            printf ("DBF: Address %lu specified for copy is not 4 byte aligned!\n",
                adr_dest);
            exit (1);
        }
        b8alg = ((adr_dest & 0x07) == 0);

        // Folowing requests : data
        for (k = 0; k < nb_mb_height; k++)
            for (l = 0; l < mb_height; l++)
                for (y = 0; y < nb_mb_width; y++)
                    for (x = 0; x < mb_width; x = x + 4)
                    {
                        reg_addr = (k * nb_mb_width * MB_BLOCK_SIZE) +
                            (l * mb_width) + (y * MB_BLOCK_SIZE) + x;
                        SEND_PACK;
                    }

        for (k = 0; k < nb_mb_height; k++)
            for (l = 0; l < 8; l++)
                for (y = 0; y < nb_mb_width; y++)
                    for (x = 0; x < 8; x = x + 4)
                    {
                        reg_addr = 256 + (k * nb_mb_width * MB_BLOCK_SIZE) +
                            (l * 8) + (y * MB_BLOCK_SIZE) + x;
                        SEND_PACK;
                    }

        for (k = 0; k < nb_mb_height; k++)
            for (l = 0; l < 8; l++)
                for (y = 0; y < nb_mb_width; y++)
                    for (x = 0; x < 8; x = x + 4)
                    {
                        reg_addr = 256 + 64 + (k * nb_mb_width * MB_BLOCK_SIZE) +
                            (l * 8) + (y * MB_BLOCK_SIZE) + x;
                        SEND_PACK;
                    }

        #ifdef ENERGY_TRACE_ENABLED
        m_etrace_crt_mode -= (N_SLICE_THREADS + 1);
        etrace.change_energy_mode (m_etrace_id, m_etrace_crt_mode);
        #endif

        DPRINTF ("-- FILTER -- Data have been sent to sram\n");

        m_dbf_link->set_data (REG_FRA_SRAM, 0);

        //update register & generate interrupt
        m_dbf_link->m_data[REG_INT_STATUS] |= (1 << REG_STATUS_BIT_SRAM);
        if (m_dbf_link->m_data[REG_INT_STATUS] & m_dbf_link->m_data[REG_INT_ENABLE])
        {
            m_dbf_link->m_data[REG_INT_STATUS] |= 0x80;
            m_irq_stat = 1;
            m_ev_irqupdate.notify();
        }
    } // end while(1)
}

void dbf_device::master_response ()
{
    // Internal declarations
    vci_response rsp;

    // Process
    while (1)
    {
        master_get_port->get (rsp);
    } // end while(1)
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
