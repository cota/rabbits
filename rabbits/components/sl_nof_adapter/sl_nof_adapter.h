#ifndef _SL_NOF_ADAPTER_H_
#define _SL_NOF_ADAPTER_H_

#include <mwsr_ta_fifo.h>
#include <slave_device.h>
#include <master_device.h>

#include <systemc.h>
#include <tlm.h>
using namespace tlm;

#include <abstract_noc.h>
#include <mumgr_package.h>

#define NOF_ADAPTER_REGS_BASE		0x1000
#define NOF_ADAPTER_ACC_REGS_BASE	0x2000

#define NOF_ADAPTER_REG_WR_DONE		0 /* R/O */
#define NOF_ADAPTER_REG_WR_SEND		1 /* W/O */
#define NOF_ADAPTER_REG_RD_EMPTY	2 /* W/O */
#define NOF_ADAPTER_REG_RD_FULL		3 /* R/O */
#define NOF_ADAPTER_NR_REGS	4

class sl_nof_adapter : public slave_device
{
 public:
    SC_HAS_PROCESS(sl_nof_adapter);
    sl_nof_adapter(sc_module_name _name, size_t _wbuf_items, size_t _rbuf_items);
    ~sl_nof_adapter();

 public:
    sc_port<tlm_blocking_put_if<noc_flit> > to_acc;
    sc_port<tlm_blocking_get_if<noc_flit> > from_acc;
    sc_port<tlm_transport_if<rw_req_transaction_32, unsigned int> > acc_regs;

 public:
    /*
     *   Obtained from parent:
     *   void send_rsp (bool bErr, uint8_t oob);
     */
    virtual void rcv_rqst(unsigned long ofs, unsigned char be,
                          unsigned char *data, bool bWrite, bool sleep);

 private:
    void write(unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr);
    void read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr);
    void write_reg(uint32_t offset, uint32_t *val, bool &bErr);
    void read_reg (uint32_t offset, uint32_t *val, bool &bErr);
    void write_acc_reg(uint32_t offset, uint32_t *val, bool &bErr);
    void read_acc_reg (uint32_t offset, uint32_t *val, bool &bErr);
    void send_packet_thread(void);
    void recv_packet_thread(void);

 private:
    sc_event	m_ev_wbuf_full;
    sc_event	m_ev_rbuf_empty;
    sc_event	m_ev_rbuf_full;
    uint32_t	*m_wbuf;
    size_t	m_wbuf_items;
    uint32_t	*m_rbuf;
    size_t	m_rbuf_items;
    uint32_t	*m_regs;
};

#endif /* _SL_NOF_ADAPTER_H_ */
