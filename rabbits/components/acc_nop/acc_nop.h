#ifndef _ACC_NOP_H_
#define _ACC_NOP_H_

#include <systemc.h>
#include <tlm.h>
using namespace tlm;

#include <abstract_noc.h>
#include <mwsr_ta_fifo.h>

#include <mumgr_package.h>

#include "acc_nop_regs.h"

class acc_nop: public sc_module, public tlm_transport_if<rw_req_transaction_32, unsigned int>
{
 public:
	SC_HAS_PROCESS(acc_nop);
	acc_nop(sc_module_name name, size_t _n_items);
	~acc_nop(void);
 private:
	unsigned int transport(const rw_req_transaction_32 &);
	void acc_nop_thread();

 public:
	sc_export<tlm_transport_if<rw_req_transaction_32, unsigned int> > acc_regs;
	sc_export<tlm_blocking_put_if<noc_flit> > from_ni;
        sc_export<tlm_blocking_get_if<noc_flit> > to_ni;

 private:
	tlm_fifo<noc_flit> fifo_from_ni;
	tlm_fifo<noc_flit> fifo_to_ni;
 private:
	uint32_t *buf;
	size_t n_items;
	unsigned int regs[ACC_NOP_NR_REGS];
};

#endif /* _ACC_NOP_H_ */
