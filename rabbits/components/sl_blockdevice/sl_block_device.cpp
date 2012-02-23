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
#include <sl_block_device.h>


/* #define DEBUG_DEVICE_BLOCK */

#ifdef DEBUG_DEVICE_BLOCK
#define DPRINTF(fmt, args...)                               \
    do { printf("sl_block_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

#define EPRINTF(fmt, args...)                               \
    do { fprintf(stderr, "sl_block_device: " fmt , ##args); } while (0)

sl_block_device::sl_block_device (sc_module_name _name, uint32_t master_id,
                                  const char *fname, uint32_t block_size)
:sc_module(_name)
{
    char *buf = new char[strlen(_name) + 3];

    m_cs_regs = new sl_block_device_CSregs_t;

    m_cs_regs->m_status = 0;
    m_cs_regs->m_buffer = 0;
    m_cs_regs->m_op = 0;
    m_cs_regs->m_lba = 0;
    m_cs_regs->m_count = 0;
    m_cs_regs->m_size = 0;
    m_cs_regs->m_block_size = block_size;
    m_cs_regs->m_irqen = 0;
    m_cs_regs->m_irq   = 0;

    strcpy(buf, _name);
    buf[strlen(_name)] = '_';
    buf[strlen(_name)+1] = 'm';
    buf[strlen(_name)+2] = '\0';
    master = new sl_block_device_master(buf, master_id);

    buf[strlen(_name)+1] = 's';
    slave = new sl_block_device_slave(buf, m_cs_regs, &ev_op_start, &ev_irq_update);

    m_fd = -1;
    open_host_file (fname);

    SC_THREAD (irq_update_thread);
    SC_THREAD (control_thread);
 
}

sl_block_device::~sl_block_device ()
{

    DPRINTF("destructor called\n");

}


static off_t get_file_size (int fd)
{
    off_t   old_pos = lseek (fd, 0, SEEK_CUR);
    off_t   ret     = lseek (fd, 0, SEEK_END);
    lseek (fd, old_pos, SEEK_SET);

    return ret;
}

void sl_block_device::open_host_file (const char *fname)
{
    if (m_fd != -1)
    {
        ::close (m_fd);
        m_fd = -1;
    }
    if (fname)
    {
        m_fd = ::open(fname, O_RDWR | O_CREAT, 0644);
        if(m_fd < 0)
        {
            EPRINTF("Impossible to open file : %s\n", fname);
            return;
        }

        m_cs_regs->m_size = get_file_size (m_fd) / m_cs_regs->m_block_size;
    }
}

master_device *
sl_block_device::get_master ()
{
    return (master_device *)master;
}

slave_device *
sl_block_device::get_slave ()
{
    return (slave_device *)slave;
}

void
sl_block_device::control_thread ()
{
    uint32_t        offset, addr, transfer_size;
    uint8_t         *data_buff;
    int             func_ret;

    while(1)
    {

        switch(m_cs_regs->m_op)
        {

        case BLOCK_DEVICE_NOOP:
            DPRINTF("Got a BLOCK_DEVICE_NOOP\n");
            wait(ev_op_start);
            break;
        case BLOCK_DEVICE_READ:
            m_cs_regs->m_status = BLOCK_DEVICE_BUSY;
            transfer_size = m_cs_regs->m_count * m_cs_regs->m_block_size;

            data_buff = new uint8_t[transfer_size + 4];

            DPRINTF("Got a BLOCK_DEVICE_READ of size %x\n", transfer_size);

            /* Read in device */
            lseek(m_fd, m_cs_regs->m_lba*m_cs_regs->m_block_size, SEEK_SET);
            func_ret = ::read(m_fd, data_buff, transfer_size);
            addr = m_cs_regs->m_buffer;
            if (func_ret < 0)
            {
                EPRINTF("Error in ::read()\n");
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_READ_ERROR;
                m_cs_regs->m_count = 0;
                break;
            }
            m_cs_regs->m_count = func_ret / m_cs_regs->m_block_size;
            transfer_size = func_ret;

            if (transfer_size)
            {
                /* Send data in memory */
                uint32_t up_4align_limit = transfer_size & ~3;
                for(offset = 0; offset < up_4align_limit; offset += 4)
                {
                    func_ret = master->cmd_write(addr + offset, data_buff + offset, 4);
                    if(!func_ret)
                        break;
                }
                for (; offset < transfer_size; offset++)
                {
                    func_ret = master->cmd_write(addr + offset, data_buff + offset, 1);
                    if(!func_ret)
                        break;
                }

                if(!func_ret)
                {
                    EPRINTF("Error in Read\n");
                    m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                    m_cs_regs->m_status = BLOCK_DEVICE_READ_ERROR;
                    m_cs_regs->m_count = offset / m_cs_regs->m_block_size;
                    break;
                }
            }

            /* Update everything */
            if(m_cs_regs->m_irqen)
            {
                m_cs_regs->m_irq = 1;
                ev_irq_update.notify();
            }

            m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
            m_cs_regs->m_status = BLOCK_DEVICE_READ_SUCCESS;
            delete data_buff;

            break;
        case BLOCK_DEVICE_WRITE:
            DPRINTF("Got a BLOCK_DEVICE_WRITE\n");
            m_cs_regs->m_status = BLOCK_DEVICE_BUSY;
            transfer_size = m_cs_regs->m_count * m_cs_regs->m_block_size;

            data_buff = new uint8_t[transfer_size + 4];
            addr = m_cs_regs->m_buffer;

            /* Read data from memory */
            for(offset = 0; offset < transfer_size; offset += 4)
            {
                func_ret = master->cmd_read(addr + offset, data_buff + offset, 4);
                if(!func_ret)
                    break;
            }

            if(!func_ret)
            {
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_WRITE_ERROR;
                break;
            }

            /* Write in the device */
            lseek(m_fd, m_cs_regs->m_lba*m_cs_regs->m_block_size, SEEK_SET);
            ::write(m_fd, data_buff, transfer_size);

            m_cs_regs->m_size = get_file_size (m_fd) / m_cs_regs->m_block_size;

            /* Update everything */
            if(m_cs_regs->m_irqen)
            {
                m_cs_regs->m_irq = 1;
                ev_irq_update.notify();
            }
            
            m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
            m_cs_regs->m_status = BLOCK_DEVICE_WRITE_SUCCESS; 
            delete data_buff;

            break;
        case BLOCK_DEVICE_FILE_NAME:
            DPRINTF("Got a BLOCK_DEVICE_FILE_NAME\n");
            m_cs_regs->m_status = BLOCK_DEVICE_BUSY;
            transfer_size = m_cs_regs->m_count * m_cs_regs->m_block_size;

            data_buff = new uint8_t[transfer_size + 4];
            addr = m_cs_regs->m_buffer;

            /* Read data from memory */
            for(offset = 0; offset < transfer_size; offset += 4)
            {
                func_ret = master->cmd_read(addr + offset, data_buff + offset, 4);
                if(!func_ret)
                    break;
            }
    
            if (func_ret)
                open_host_file ((const char*) data_buff);

            if(!func_ret || m_fd == -1)
            {
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_WRITE_ERROR;
                break;
            }
            
            m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
            m_cs_regs->m_status = BLOCK_DEVICE_WRITE_SUCCESS; 
            delete data_buff;

            break;
        default:
            EPRINTF("Error in command\n");
        }

    }

}

void sl_block_device::irq_update_thread ()
{
    while(1)
    {

        wait(ev_irq_update);

        if(m_cs_regs->m_irq == 1)
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

/*
 * sl_block_device_master
 */
sl_block_device_master::sl_block_device_master (const char *_name, uint32_t node_id)
: master_device(_name)
{
    m_crt_tid = 0;
    m_status  = MASTER_READY;
    m_node_id = node_id;

}

sl_block_device_master::~sl_block_device_master()
{

}

int
sl_block_device_master::cmd_write (uint32_t addr, uint8_t *data, uint8_t nbytes)
{
    DPRINTF("Write to %x [0x%08x]\n", addr, *(uint32_t *)data);

    send_req (m_crt_tid, addr, data, nbytes, 1);

    wait (ev_cmd_done);

    if(m_status != MASTER_CMD_SUCCESS)
        return 0;
    
    return 1;
}

int
sl_block_device_master::cmd_read (uint32_t addr, uint8_t *data, uint8_t nbytes)
{
    m_tr_nbytes = nbytes;
    m_tr_rdata  = 0;

    DPRINTF ("Read from %x\n", addr);

    send_req (m_crt_tid, addr, NULL, nbytes, 0);

    wait (ev_cmd_done);

    for (int i = 0; i < nbytes; i++)
        data[i] = ((unsigned char *) &m_tr_rdata)[i];

    if (m_status != MASTER_CMD_SUCCESS)
        return 0;

    return 1;
}

void 
sl_block_device_master::rcv_rsp (uint8_t tid, uint8_t *data,
                                 bool bErr, bool bWrite, uint8_t oob)
{
    if (tid != m_crt_tid)
    {
        EPRINTF ("Bad tid (%d / %d)\n", tid, m_crt_tid);
    }

    if (bErr)
    {
        DPRINTF("Cmd KO\n");
        m_status = MASTER_CMD_ERROR;
    }
    else
    {
        //DPRINTF("Cmd OK\n");
        m_status = MASTER_CMD_SUCCESS;
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
 * sl_block_device_slave
 */
sl_block_device_slave::sl_block_device_slave (const char *_name,
    sl_block_device_CSregs_t *cs_regs, sc_event *op_start, sc_event *irq_update)
    : slave_device (_name)
{
    m_cs_regs     = cs_regs;
    ev_op_start   = op_start;
    ev_irq_update = irq_update;
}

sl_block_device_slave::~sl_block_device_slave()
{
}

void sl_block_device_slave::write (unsigned long ofs, unsigned char be,
                                   unsigned char *data, bool &bErr)
{
    uint32_t  *val = (uint32_t *) data;
    uint32_t   lofs = ofs;
    uint8_t    lbe  = be;

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
    case BLOCK_DEVICE_BUFFER     :
        DPRINTF("BLOCK_DEVICE_BUFFER write: %x\n", *val);
        m_cs_regs->m_buffer = *val;
        break;
    case BLOCK_DEVICE_LBA        :
        DPRINTF("BLOCK_DEVICE_LBA write: %x\n", *val);
        m_cs_regs->m_lba = *val;
        break;
    case BLOCK_DEVICE_COUNT      :
        DPRINTF("BLOCK_DEVICE_COUNT write: %x\n", *val);
        m_cs_regs->m_count = *val;
        break;
    case BLOCK_DEVICE_OP         :
        DPRINTF("BLOCK_DEVICE_OP write: %x\n", *val);
        if(m_cs_regs->m_status != BLOCK_DEVICE_IDLE)
        {
            EPRINTF("Got a command while executing another one\n");
            break;
        }
        m_cs_regs->m_op = *val;
        ev_op_start->notify();
        break;
    case BLOCK_DEVICE_IRQ_ENABLE :
        DPRINTF("BLOCK_DEVICE_IRQ_ENABLE write: %x\n", *val);
        m_cs_regs->m_irqen = *val;
        ev_irq_update->notify();
        break;
    case BLOCK_DEVICE_STATUS     :
    case BLOCK_DEVICE_SIZE       :
    case BLOCK_DEVICE_BLOCK_SIZE :
    default:
        EPRINTF("Bad %s::%s ofs=0x%X, be=0x%X\n", name (), __FUNCTION__,
                (unsigned int) ofs, (unsigned int) be);
    }
}

void sl_block_device_slave::read (unsigned long ofs, unsigned char be,
                                  unsigned char *data, bool &bErr)
{

    uint32_t  *val = (uint32_t *)data;
    uint32_t   lofs = ofs;
    uint8_t    lbe  = be;

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
    case BLOCK_DEVICE_BUFFER     :
        *val = m_cs_regs->m_buffer;
        DPRINTF("BLOCK_DEVICE_BUFFER read: %x\n", *val);
        break;
    case BLOCK_DEVICE_LBA        :
        *val = m_cs_regs->m_lba;
        DPRINTF("BLOCK_DEVICE_LBA read: %x\n", *val);
        break;
    case BLOCK_DEVICE_COUNT      :
        *val = m_cs_regs->m_count;
        DPRINTF("BLOCK_DEVICE_COUNT read: %x\n", *val);
        break;
    case BLOCK_DEVICE_OP         :
        *val = m_cs_regs->m_op;
        DPRINTF("BLOCK_DEVICE_OP read: %x\n", *val);
        break;
    case BLOCK_DEVICE_STATUS     :
        *val = m_cs_regs->m_status;
        DPRINTF("BLOCK_DEVICE_STATUS read: %x\n", *val);
        if(m_cs_regs->m_status != BLOCK_DEVICE_BUSY)
        {
            m_cs_regs->m_status = BLOCK_DEVICE_IDLE;
            m_cs_regs->m_irq = 0;
            ev_irq_update->notify();
        }
        break;
    case BLOCK_DEVICE_IRQ_ENABLE :
        *val = m_cs_regs->m_irqen;
        DPRINTF("BLOCK_DEVICE_IRQ_ENABLE read: %x\n", *val);
        break;
    case BLOCK_DEVICE_SIZE       :
        *val = m_cs_regs->m_size;
        DPRINTF("BLOCK_DEVICE_SIZE read: %x\n", *val);
        break;
    case BLOCK_DEVICE_BLOCK_SIZE :
        *val = m_cs_regs->m_block_size;
        DPRINTF("BLOCK_DEVICE_BLOCK_SIZE read: %x\n", *val);
        break;
    default:
        EPRINTF("Bad %s::%s ofs=0x%X, be=0x%X\n", name (), __FUNCTION__,
                (unsigned int) ofs, (unsigned int) be);
    }
}

void sl_block_device_slave::rcv_rqst (unsigned long ofs, unsigned char be,
                                      unsigned char *data, bool bWrite, bool sleep)
{

    bool bErr = false;

    if(bWrite)
        this->write(ofs, be, data, bErr);
    else
        this->read(ofs, be, data, bErr);

    send_rsp(bErr, 0);

    return;
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
