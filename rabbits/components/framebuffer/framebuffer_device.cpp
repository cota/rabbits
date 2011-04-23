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

#include <systemc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <framebuffer_device.h>

#if 0
#define DEBUG_DEVICE_FB
#endif

#ifdef DEBUG_DEVICE_FB
#define DPRINTF(fmt, args...)                               \
    do { printf("fb_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

#define EPRINTF(fmt, args...)                               \
    do { fprintf(stderr, "fb_device: " fmt , ##args); } while (0)

static int          pids_ramdac[10];
static uint8_t      *shm_addr[10][2];
static int          shmids[10][2];
static int          nb_fb = 0;

/*
 * FB_DEVICE
 */

void
close_ramdacs ()
{
    int         i, j, status;

    DPRINTF ("close called\n");

    if (nb_fb == 0)
        return;

    for (i = 0; i < nb_fb; i++)
    {
        for (j = 0; j < 2; j++)
        {
            shmdt (shm_addr[i][j]);
            shmctl (shmids[i][j], IPC_RMID, NULL);
        }

        //cout << "Killing RAMDAC " << (int) i << endl;
        kill (pids_ramdac[i], SIGKILL);
        ::waitpid (pids_ramdac[i], &status, 0);
    }

    nb_fb = 0;
}

void
fb_device::close_viewer ()
{
    int         i, status;

    DPRINTF ("close called\n");

    for (i = 0; i < 2; i++)
    {
        shmdt (m_shm_buf[i]);
        shmctl (m_shmid[i], IPC_RMID, NULL);
    }

    kill (m_pid, SIGKILL);
    ::waitpid (m_pid, &status, 0);
}

void
fb_device::init_viewer ()
{
    char            xname[80] = "*** The Screen ***";
    int             ppout[2];
    key_t           key[2];
    int             i, k;

    pipe(ppout);
    if (!(m_pid = fork()))
    {
        setpgrp();
        /* close (STDIN_FILENO); */
        /* dup2 (ppout[0], STDIN_FILENO); */
        close (0);
        dup2 (ppout[0], 0);

        if (execlp ("fbviewer", "fbviewer", (char *) NULL) == -1)
        {
            perror ("viewer: execlp failed");
            _exit (1);
        }
    }

    if(m_pid == -1)
    {
        perror ("viewer: fork failed");
        exit (1);
    }

    DPRINTF ("Viewer child: %d\n", m_pid);

    pids_ramdac[nb_fb] = m_pid;
    m_pout = ppout[1];

    for(i = 0; i < 2; i++)
    {
        for (k = 1;; k++)
            if ((m_shmid[i] = shmget (k, m_buf_size * sizeof (uint8_t),
                                    IPC_CREAT | IPC_EXCL | 0600)) != -1)
                break;

        DPRINTF ("Got shm %x\n", k);
        key[i] = k;

        m_shm_buf[i] = (uint8_t *) shmat (m_shmid[i], 0, 00200);
        if (m_shm_buf[i] == (void *) -1)
        {
            perror ("ERROR: Ramdac.shmat");
            exit (1);
        }

        shmids[nb_fb][i]   = m_shmid[i];
        shm_addr[nb_fb][i] = m_shm_buf[i];
    }

    nb_fb++;
   
    ::write (m_pout, key, sizeof (key));
    ::write (m_pout, &m_width, sizeof (uint32_t));
    ::write (m_pout, &m_height, sizeof (uint32_t));
    ::write (m_pout, &m_mode, sizeof (uint32_t));
    ::write (m_pout, xname, sizeof (xname));

    sleep(1);
}

fb_device::fb_device (sc_module_name _name, uint32_t master_id,
    fb_reset_t *reset_status)
    :sc_module(_name)
{
    char *buf = new char[strlen (_name) + 3];

    m_regs = new fb_regs_t;
    memset (m_regs, 0, sizeof (fb_regs_t));
    m_io_res = new fb_io_resource_t;
    memset (m_io_res, 0, sizeof (fb_io_resource_t));

    m_io_res->regs     = m_regs;
    m_io_res->mem_size = 0;
    m_io_res->mem_idx  = 0;
    
    m_dma_in_use = 0;
    m_regs->m_buf_addr      = 0;
    m_regs->m_irqstat       = 0;
    if(reset_status->fb_start)
    {
        m_regs->m_image_w   = reset_status->fb_w;
        m_regs->m_image_h   = reset_status->fb_h;
        m_regs->m_mode      = reset_status->fb_mode;
        m_regs->m_ctrl      = (1 << 0);
        m_regs->m_status    = FB_RUNNING;
        m_regs->m_disp_wrap = reset_status->fb_display_on_wrap;
    }
    else
    {
        m_regs->m_image_w   = 0;
        m_regs->m_image_h   = 0;
        m_regs->m_mode      = NONE;
        m_regs->m_ctrl      = 0;
        m_regs->m_status    = FB_STOPPED;
    }
    m_regs->m_irq           = 0;

    strcpy (buf, _name);
    buf[strlen (_name)] = '_';
    buf[strlen (_name)+2] = '\0';

    buf[strlen (_name)+1] = 's';
    slave = new fb_device_slave (buf, m_io_res, &m_ev_irq_update,
                                 &m_ev_display, &m_ev_start_stop);

    /* if needed */
    buf[strlen(_name)+1] = 'm';
    master = new fb_device_master (buf, master_id);

    if(reset_status->fb_start)
        init (reset_status->fb_w, reset_status->fb_h, reset_status->fb_mode);

    SC_THREAD (irq_update_thread);
    SC_THREAD (display_thread);
    SC_THREAD (start_stop_thread);
}

fb_device::~fb_device ()
{
    DPRINTF ("destructor called\n");
}

void
fb_device::uninit (void)
{
    DPRINTF ("uninit()\n");

    m_width     = 0;
    m_height    = 0;
    m_mode      = NONE;
    m_buf_size  = 0;
    close_viewer ();
    m_regs->m_status = FB_STOPPED;
}

char *print_mode (int mode)
{
    switch(mode)
    {
    case NONE:
        return (char *) "NONE";
    case GREY:
        return (char *) "GREY";
    case RGB:
        return (char *) "RGB";
    case BGR:
        return (char *) "BGR";
    case ARGB:
        return (char *) "ARGB";
    case BGRA:
        return (char *) "BGRA";
    case YVYU:
        return (char *) "YVYU";
    case YV12:
        return (char *) "YV12";
    case IYUV:
        return (char *) "IYUV";
    case YV16:
        return (char *) "YV16";
    default:
        return (char *) "UNKN";
    }
}

void
fb_device::init (int width, int height, int mode)
{
    m_width  = width;
    m_height = height;
    m_mode   = mode;

    DPRINTF ("init() %d %d %d\n", width, height, mode);

    switch(m_mode)
    {
    case NONE:
        EPRINTF("Mode not set\n");
        break;
    case GREY:
        m_buf_size       = height * width;
        break;
    case RGB:
    case BGR:
        m_buf_size       = 3 * height * width;
        break;
    case ARGB:
    case BGRA:
        m_buf_size       = 4 * height * width;
        break;
    case YVYU: /* YUV 4:2:2 */
    case YV16:
        m_buf_size       = 3 * height * width;
        break;
    case YV12: /* YUV 4:2:0 */
    case IYUV: /* YUV 4:2:0 */
        m_buf_size       = 3 * height * width;
        break;
    default:
        EPRINTF ("Bad mode\n");
    }

    DPRINTF ("new framebuffer: %dx%d mode: %s\n", width, height, print_mode(mode));
    DPRINTF ("                 buf_size: [%dB--0x%xB]\n", m_buf_size, m_buf_size);

    if (nb_fb == 0)
        atexit (close_ramdacs);

    init_viewer ();

    m_io_res->mem      = m_shm_buf;
    m_io_res->mem_size = m_buf_size;
    m_regs->m_status   = FB_RUNNING;
}

master_device *
fb_device::get_master ()
{
    return (master_device *) master;
}

slave_device *
fb_device::get_slave ()
{
    return (slave_device *) slave;
}

void
fb_device::start_stop_thread ()
{

    while(1)
    {
        wait (m_ev_start_stop);

        if ((m_regs->m_status == FB_STOPPED) && (m_regs->m_ctrl & FB_CTRL_START))
        {
            /* starting FB */
            init(m_regs->m_image_w, m_regs->m_image_h, m_regs->m_mode);
        }
        else
        { /* if starting */
            if ((m_regs->m_status == FB_RUNNING) && !(m_regs->m_ctrl & FB_CTRL_START))
            {
                DPRINTF("Stopping ... !!!\n");
                /* stopping FB */
                uninit();
            } /* if stopping */
        }
    } /* while(1) */
}

void 
fb_device::irq_update_thread ()
{
    while(1)
    {
        wait (m_ev_irq_update);

        if (m_regs->m_ctrl & FB_CTRL_IRQEN)
        {
            if (m_regs->m_irqstat & ~FB_IRQ_GLOBAL)
            {
                DPRINTF ("Raising IRQ\n");
                m_regs->m_irqstat |= FB_IRQ_GLOBAL;
                irq = 1;
            }
            else
            {
                DPRINTF ("Clearing IRQ\n");
                m_regs->m_irqstat &= ~FB_IRQ_GLOBAL;
                irq = 0;
            }
        }
        else
        {
            DPRINTF("IRQ dont care\n");
            if(irq != 0)
                irq = 0;
        }
    }
}

void
fb_device::display_thread (void)
{
    int             func_ret = 0;

    while(1)
    {

        wait (m_ev_display);

        int  proc_idx = (m_io_res->mem_idx + 1) % 2;

        DPRINTF ("display()\n");

        /* if DMA in use */
        /* retrieve data */
        if (m_regs->m_dmaen)
        {
            uint32_t    offset;
            uint32_t    addr        = m_regs->m_buf_addr;
            uint8_t     *data       = m_io_res->mem[proc_idx];

            for (offset = 0; offset < m_io_res->mem_size; offset += 4)
            {
                func_ret = master->cmd_read (addr + offset, data + offset, 4);
                if (!func_ret)
                    break;
            }

            if (!func_ret)
                EPRINTF ("Error in Read\n");
            else
                DPRINTF ("Transfer complete\n");
        }

        kill (m_pid, SIGUSR1);

        m_regs->m_irqstat |= (1 << 0);
        m_ev_irq_update.notify ();
    }
}

/*
 * fb_device_master
 */
fb_device_master::fb_device_master (const char *_name, uint32_t node_id)
    : master_device (_name)
{
    m_crt_tid = 0;
    m_node_id = node_id;
}

fb_device_master::~fb_device_master ()
{
}

int
fb_device_master::cmd_read (uint32_t addr, uint8_t *data, uint8_t nbytes)
{
    m_tr_nbytes = nbytes;

    send_req (m_crt_tid, addr, NULL, nbytes, 0);

    wait (ev_cmd_done);

    for (int i = 0; i < nbytes; i++)
        data[i] = ((unsigned char *) &m_tr_rdata)[i];

    if (m_status != FB_MASTER_CMD_SUCCESS)
        return 0;

   return 1;
}

void 
fb_device_master::rcv_rsp (uint8_t tid, uint8_t *data,
                           bool bErr, bool bWrite)
{
    if (tid != m_crt_tid)
        EPRINTF ("Bad tid (%d / %d)\n", tid, m_crt_tid);
    
    if (bErr)
    {
        DPRINTF ("Cmd KO\n");
        m_status = FB_MASTER_CMD_ERROR;
    }
    else
    {
        //DPRINTF("Cmd OK\n");
        m_status = FB_MASTER_CMD_SUCCESS;
    }

    if (!bWrite)
    {
        for (int i = 0; i < m_tr_nbytes; i++)
            ((unsigned char *) &m_tr_rdata)[i] = data[i];
    }
    m_crt_tid++;

    ev_cmd_done.notify ();
}

/*
 * fb_device_slave
 */
fb_device_slave::fb_device_slave (const char *_name, fb_io_resource_t *io_res,
    sc_event *irq_update, sc_event *display, sc_event *start_stop)
    : slave_device (_name)
{
    m_io_res        = io_res;
    m_ev_display    = display;
    m_ev_irqupdate  = irq_update;
    m_ev_start_stop = start_stop;
}

fb_device_slave::~fb_device_slave ()
{
}

static uint32_t s_mask[16] = { 0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF,
                               0x00FF0000, 0x00FF00FF, 0x00FFFF00, 0x00FFFFFF,
                               0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
                               0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF };

void
fb_device_slave::display (void)
{
    m_io_res->mem_idx = (m_io_res->mem_idx + 1) % 2;
    m_ev_display->notify ();
}

void
fb_device_slave::write (unsigned long ofs, unsigned char be,
                        unsigned char *data, bool &bErr)
{
    uint32_t  *val = (uint32_t *) data;
    uint32_t   lofs = ofs;
    uint8_t    lbe  = be;

    uint32_t   mem_ofs = ofs;

    uint32_t  *ptr = NULL;
    uint32_t   tmp = 0;
    uint32_t   mask = 0;

    bErr = false;

    lofs >>= 2;
    if (lbe & 0xF0)
    {
        lofs  += 1;
        lbe  >>= 4;
        val++;
    }

    switch(lofs)
    {
    case FB_DEVICE_WH:
        DPRINTF ("FB_DEVICE_HW write: %x\n", *val);
        m_io_res->regs->m_image_w = *val & 0xFFFF;
        m_io_res->regs->m_image_h = *val >> 16;
        break;
    case FB_DEVICE_MODE:
        DPRINTF ("FB_DEVICE_MODE write: %x\n", *val);
        m_io_res->regs->m_mode = *val;
        break;
    case FB_DEVICE_BUF:
        DPRINTF ("FB_DEVICE_BUF write: %x\n", *val);
        m_io_res->regs->m_buf_addr = *val;
        break;
    case FB_DEVICE_IRQCLR:
        DPRINTF ("FB_DEVICE_IRQCLR write: %x\n", *val);
        m_io_res->regs->m_irqstat &= ~(*val);
        m_ev_irqupdate->notify();
        break;
    case FB_DEVICE_CTRL:
        DPRINTF ("FB_DEVICE_CTRL write: %x\n", *val);
        tmp = m_io_res->regs->m_ctrl;
        m_io_res->regs->m_ctrl = *val;
        /* BIT 0: Start/Stop FB */
        if ((tmp ^ *val) & FB_CTRL_START)
            m_ev_start_stop->notify();

        /* BIT 1: IRQ Enable */
        if ((tmp ^ *val) & FB_CTRL_IRQEN)
            m_ev_irqupdate->notify();

        /* BIT 2: DMA enable */
        if ((tmp ^ *val) & FB_CTRL_DMAEN)
        {
            if(*val & FB_CTRL_DMAEN)
                m_io_res->regs->m_dmaen = 1;
            else
                m_io_res->regs->m_dmaen = 0;
        }

        /* BIT 3: Display */
        if ((tmp ^ *val) & FB_CTRL_DISPLAY)
        {
            m_io_res->regs->m_ctrl &= ~FB_CTRL_DISPLAY;
            display ();
        }
        break;
    default:
        mem_ofs = lofs - FB_DEVICE_MEM;
        if ((mem_ofs >= 0) && (mem_ofs < (m_io_res->mem_size >> 2)))
        {
            if (m_io_res->regs->m_disp_wrap && ((mem_ofs == 0) && (be & 0x1)))
            {
                /* Each time we write at the zero position we update */
                display();
            }

            ptr = (uint32_t *) m_io_res->mem[m_io_res->mem_idx];
            tmp = ptr[mem_ofs];
            mask = s_mask[lbe];

            /* DPRINTF("write at idx: %x, of val: %x\n", lofs, *val); */

            tmp &= ~mask;
            tmp |= (*val & mask);
            ptr[mem_ofs] = tmp;
        }
        else
        {
            EPRINTF ("write outside mem area (%d): %x(%x) / %x\n",
                    m_io_res->mem_idx, lofs<<2, mem_ofs<<2, m_io_res->mem_size);
            EPRINTF ("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X\n",
                    name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                    (unsigned int) *((unsigned long*)data + 0),
                    (unsigned int) *((unsigned long*)data + 1));
            bErr = true;
            exit (EXIT_FAILURE);
        }
    }
}

void fb_device_slave::read (unsigned long ofs, unsigned char be,
                            unsigned char *data, bool &bErr)
{
    uint32_t    *val  = (uint32_t *)data;
    uint32_t    lofs = ofs;
    uint8_t     lbe  = be;
    uint32_t    mem_ofs = ofs;
    uint32_t  *ptr = NULL;
    uint32_t   tmp = 0;
    uint32_t   mask = 0;

    bErr = false;

    lofs >>= 2;
    if(lbe & 0xF0)
    {
        lofs  += 1;
        lbe  >>= 4;
        val++;
    }

    switch(lofs)
    {
    case FB_DEVICE_WH:
        *val = (m_io_res->regs->m_image_h << 16) | (m_io_res->regs->m_image_w);
        DPRINTF ("FB_DEVICE_WH read: %x\n", *val);
        break;
    case FB_DEVICE_MODE:
         *val = m_io_res->regs->m_mode;
        DPRINTF ("FB_DEVICE_MODE read: %x\n", *val);
        break;
    case FB_DEVICE_BUF:
         *val = m_io_res->regs->m_buf_addr;
        DPRINTF ("FB_DEVICE_BUF read: %x\n", *val);
        break;
    case FB_DEVICE_IRQCLR:
         *val = m_io_res->regs->m_irqstat;
         DPRINTF ("FB_DEVICE_IRQCLR read: %x\n", *val);
        break;
    case FB_DEVICE_CTRL:
         *val = m_io_res->regs->m_ctrl;
        DPRINTF ("FB_DEVICE_CTRL read: %x\n", *val);
        break;
    default:
        mem_ofs = lofs - FB_DEVICE_MEM;
        if (mem_ofs < (m_io_res->mem_size >> 2))
        {
            ptr = (uint32_t *)m_io_res->mem[m_io_res->mem_idx];
            tmp = ptr[mem_ofs];
            mask = s_mask[lbe];

            tmp &= ~mask;
            *val = tmp;
            /* DPRINTF("read at idx: %x (lbe: %x), of val: %x\n", mem_ofs, lbe, *val); */
        }
        else
        {
            EPRINTF ("read outside mem area (%d): %x(%x) / %x\n",
                    m_io_res->mem_idx,
                    lofs<<2, mem_ofs<<2,
                    m_io_res->mem_size);
            EPRINTF ("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X\n",
                    name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                    (unsigned int) *((unsigned long*)data + 0),
                    (unsigned int) *((unsigned long*)data + 1));
            bErr = true;
            exit (EXIT_FAILURE);
        }
    }
}

void fb_device_slave::rcv_rqst (unsigned long ofs, unsigned char be,
                                unsigned char *data, bool bWrite)
{
    bool bErr = false;

    if(bWrite)
        this->write(ofs, be, data, bErr);
    else
        this->read(ofs, be, data, bErr);

    send_rsp (bErr);
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
