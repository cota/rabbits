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

sl_block_device::sl_block_device (sc_module_name _name, uint32_t master_id, const char *fname, uint32_t block_size)
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

    m_fd = ::open(fname, O_RDWR);
    if(m_fd < 0){
        EPRINTF("Impossible to open file : %s\n", fname);
    }
    
    m_cs_regs->m_size = lseek(m_fd, 0, SEEK_END) / block_size;

    SC_THREAD (irq_update_thread);
    SC_THREAD (control_thread);
 
}

sl_block_device::~sl_block_device ()
{

    DPRINTF("destructor called\n");

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

    int func_ret = 0;
    uint32_t offset = 0;
    uint32_t addr = 0;

    while(1){

        switch(m_cs_regs->m_op){

        case BLOCK_DEVICE_NOOP:
            DPRINTF("Got a BLOCK_DEVICE_NOOP\n");
            wait(ev_op_start);
            break;
        case BLOCK_DEVICE_READ:
            m_cs_regs->m_status = BLOCK_DEVICE_BUSY;
            m_transfer_size = m_cs_regs->m_count * m_cs_regs->m_block_size;

            m_data = new uint8_t[m_transfer_size];

            DPRINTF("Got a BLOCK_DEVICE_READ of size %x\n", m_transfer_size);

            /* Read in device */
            lseek(m_fd, m_cs_regs->m_lba*m_cs_regs->m_block_size, SEEK_SET);
            func_ret = ::read(m_fd, m_data, m_transfer_size);
            addr = m_cs_regs->m_buffer;
            if(func_ret < 0){
                EPRINTF("Error in ::read()\n");
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_READ_ERROR;
            }

            /* Send data in memory */
            for(offset = 0; offset < m_transfer_size; offset += 4){
                func_ret = master->cmd_write(addr + offset, m_data + offset, 4);
                if(!func_ret){
                    break;
                }
            }
            if(!func_ret){
                EPRINTF("Error in Read\n");
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_READ_ERROR;
            }

            /* Update everything */
            if(m_cs_regs->m_irqen){
                m_cs_regs->m_irq = 1;
                ev_irq_update.notify();
            }

            m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
            m_cs_regs->m_status = BLOCK_DEVICE_READ_SUCCESS;
            delete m_data;

            break;
        case BLOCK_DEVICE_WRITE:
            DPRINTF("Got a BLOCK_DEVICE_WRITE\n");
            m_cs_regs->m_status = BLOCK_DEVICE_BUSY;
            m_transfer_size = m_cs_regs->m_count * m_cs_regs->m_block_size;

            m_data = new uint8_t[m_transfer_size];
            addr = m_cs_regs->m_buffer;

            /* Read data from memory */
            for(offset = 0; offset < m_transfer_size; offset += 4){
                func_ret = master->cmd_read(addr + offset, m_data + offset, 4);
                if(!func_ret){
                    break;
                }
            }
            if(!func_ret){
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_WRITE_ERROR;
                break;
            }

            /* Write in the device */
            lseek(m_fd, m_cs_regs->m_lba*m_cs_regs->m_block_size, SEEK_SET);
            ::write(m_fd, m_data, m_transfer_size);

            /* Update everything */
            if(m_cs_regs->m_irqen){
                m_cs_regs->m_irq = 1;
                ev_irq_update.notify();
            }
            
            m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
            m_cs_regs->m_status = BLOCK_DEVICE_WRITE_SUCCESS; 
            delete m_data;

            break;
        default:
            EPRINTF("Error in command\n");
        }

    }

}

void sl_block_device::irq_update_thread ()
{
    while(1) {

        wait(ev_irq_update);

        if(m_cs_regs->m_irq == 1){
            DPRINTF("Raising IRQ\n");
            irq = 1;
        }else{
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
    uint8_t tid;

    tid = m_crt_tid;

    DPRINTF("Write to %x [0x%08x]\n", addr, *(uint32_t *)data);

    send_req(tid, addr, data, nbytes, 1);

    wait(ev_cmd_done);

    if(m_status != MASTER_CMD_SUCCESS){
        return 0;
    }else{
        return 1;
    }
}

int
sl_block_device_master::cmd_read (uint32_t addr, uint8_t *data, uint8_t nbytes)
{
    uint8_t  tid;
    int i = 0;

    tid = m_crt_tid;

    m_tr_addr   = addr;
    m_tr_nbytes = nbytes;
    m_tr_rdata  = NULL;

    DPRINTF("Read from %x\n", addr);

    send_req(tid, addr, data, nbytes, 0);

    wait(ev_cmd_done);

    if(m_tr_rdata){
        for( i = 0; i < nbytes; i++){
            data[i] = m_tr_rdata[i];
        }
    }

    if( m_tr_rdata ){
//        DPRINTF("Read val: @0x%08x [0x%08x]\n", (uint32_t *)data, *(uint32_t **)data);
        DPRINTF("Read val: [0x%08x]\n", *(uint32_t *)data);
    }else{
        DPRINTF("Read NULL\n");
    }

    if(m_status != MASTER_CMD_SUCCESS){
        return 0;
    }else{
        return 1;
    }
}

void 
sl_block_device_master::rcv_rsp(uint8_t tid, uint8_t *data,
                                bool bErr, bool bWrite)
{
    int i = 0;

    if(tid != m_crt_tid){
        EPRINTF("Bad tid (%d / %d)\n", tid, m_crt_tid);
    }
    if(bErr){
        DPRINTF("Cmd KO\n");
        m_status = MASTER_CMD_ERROR;
    }else{
        //DPRINTF("Cmd OK\n");
        m_status = MASTER_CMD_SUCCESS;
    }

    if(!bWrite){
        /* DPRINTF("Got data: 0x%08x - 0x%08x\n",  */
        /*         *((uint32_t *)data + 0), *((uint32_t *)data + 1)); */
        m_tr_rdata = *((uint8_t **)data);
    }
    m_crt_tid++;

    ev_cmd_done.notify();

    return;
}

/*
 * sl_block_device_slave
 */
sl_block_device_slave::sl_block_device_slave (const char *_name,
                                              sl_block_device_CSregs_t *cs_regs,
                                              sc_event *op_start, sc_event *irq_update)
: slave_device (_name)
{

    m_cs_regs     = cs_regs;
    ev_op_start   = op_start;
    ev_irq_update = irq_update;

}

sl_block_device_slave::~sl_block_device_slave(){

}

void sl_block_device_slave::write (unsigned long ofs, unsigned char be,
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

    switch(lofs){

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
        if(m_cs_regs->m_status != BLOCK_DEVICE_IDLE){
            EPRINTF("Got a command while executing another one\n");
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
    return;
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

    switch(lofs){

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
        if(m_cs_regs->m_status != BLOCK_DEVICE_BUSY){
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
    return;
}

void sl_block_device_slave::rcv_rqst (unsigned long ofs, unsigned char be,
                                      unsigned char *data, bool bWrite)
{

    bool bErr = false;

    if(bWrite){
        this->write(ofs, be, data, bErr);
    }else{
        this->read(ofs, be, data, bErr);
    }

    send_rsp(bErr);

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
