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

#ifndef _SL_BLOCK_DEVICE_H_
#define _SL_BLOCK_DEVICE_H_

#include <slave_device.h>
#include <master_device.h>

enum sl_block_registers {
    BLOCK_DEVICE_BUFFER     = 0,
    BLOCK_DEVICE_LBA        = 1,
    BLOCK_DEVICE_COUNT      = 2,
    BLOCK_DEVICE_OP         = 3,
    BLOCK_DEVICE_STATUS     = 4,
    BLOCK_DEVICE_IRQ_ENABLE = 5,
    BLOCK_DEVICE_SIZE       = 6,
    BLOCK_DEVICE_BLOCK_SIZE = 7,
};

enum sl_block_op {
	BLOCK_DEVICE_NOOP,
	BLOCK_DEVICE_READ,
	BLOCK_DEVICE_WRITE,
};

enum sl_block_status {
	BLOCK_DEVICE_IDLE          = 0,
	BLOCK_DEVICE_BUSY          = 1,
	BLOCK_DEVICE_READ_SUCCESS  = 2,
	BLOCK_DEVICE_WRITE_SUCCESS = 3,
	BLOCK_DEVICE_READ_ERROR    = 4,
	BLOCK_DEVICE_WRITE_ERROR   = 5,
	BLOCK_DEVICE_ERROR         = 6,
};


typedef struct sl_block_device_CSregs sl_block_device_CSregs_t;

struct sl_block_device_CSregs {
    uint32_t m_status;
    uint32_t m_buffer;
    uint32_t m_op;
    uint32_t m_lba;
    uint32_t m_count;
    uint32_t m_size;
    uint32_t m_block_size;
    uint32_t m_irqen;
    uint32_t m_irq;
};

/*
 * SL_BLOCK_DEVICE_SLAVE
 */

class sl_block_device_slave : public slave_device
{
public:
    sl_block_device_slave (const char *_name,
                           sl_block_device_CSregs_t *cs_regs,
                           sc_event *op_start, sc_event *irq_update);
    ~sl_block_device_slave();

public:
    /*
     *   Obtained from father
     *   void send_rsp (bool bErr);
     */
    virtual void rcv_rqst (unsigned long ofs, unsigned char be,
                           unsigned char *data, bool bWrite);

private:
    void write (unsigned long ofs, unsigned char be,
                unsigned char *data, bool &bErr);
    void read (unsigned long ofs, unsigned char be,
               unsigned char *data, bool &bErr);


private:
    sc_event                 *ev_op_start;
    sc_event                 *ev_irq_update;
    sl_block_device_CSregs_t *m_cs_regs;

};

/*
 * SL_BLOCK_DEVICE_MASTER
 */
enum sl_block_master_status {
	MASTER_READY        = 0,
	MASTER_CMD_SUCCESS  = 1,
	MASTER_CMD_ERROR    = 2,
};

class sl_block_device_master : public master_device
{
public:
    sl_block_device_master (const char *_name, uint32_t node_id);
    ~sl_block_device_master();

public:
    /*
     *   Obtained from father
     *    void send_req(unsigned char tid, unsigned long addr, unsigned char *data, 
     * 			  unsigned char bytes, bool bWrite);
     */
    virtual void rcv_rsp (unsigned char tid, unsigned char *data,
                          bool bErr, bool bWrite);

    int cmd_write (uint32_t addr, uint8_t *data, uint8_t nbytes);
    int cmd_read (uint32_t addr, uint8_t *data, uint8_t nbytes);


private:
    sc_event   ev_cmd_done;
    uint32_t   m_status;
    uint8_t    m_crt_tid;

    uint8_t   *m_tr_rdata;
    uint32_t   m_tr_addr;
    uint8_t    m_tr_nbytes;

};

/*
 * SL_BLOCK_DEVICE
 */
class sl_block_device : public sc_module
{
public:
    SC_HAS_PROCESS(sl_block_device);
    sl_block_device (sc_module_name _name, uint32_t master_id, const char *fname,
                     uint32_t block_size);
    virtual ~sl_block_device ();

public:
    slave_device  *get_slave(void);
    master_device *get_master(void);

private:
    void irq_update_thread(void);
    void control_thread(void);

public:
    sc_out<bool>        irq;

private:
    int       m_fd;

    uint32_t  m_transfer_size;
    uint8_t  *m_data;

    /* simulation facilities */
    sc_event  ev_irq_update;
    sc_event  ev_op_start;

    sl_block_device_master *master;
    sl_block_device_slave  *slave;

    /* Control and status register bank */
    sl_block_device_CSregs_t *m_cs_regs;
};

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
