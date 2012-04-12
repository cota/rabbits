#include "acc_nop.h"
#include <flit_check.h>

acc_nop::acc_nop(sc_module_name name, size_t _n_items)
{
	n_items = _n_items;
	buf = new uint32_t[n_items];

	acc_regs(*this);
	from_ni(fifo_from_ni);
	to_ni(fifo_to_ni);
	SC_THREAD(acc_nop_thread);
}

acc_nop::~acc_nop(void)
{
	if (buf)
		delete[] buf;
}

unsigned int acc_nop::transport(const rw_req_transaction_32 & req)
{
	unsigned int offset = req.address >> 2;

	if (offset >= ACC_NOP_NR_REGS) {
		cout << name() << ": offset " << offset
		     << " out of range" << endl;
		return ~0;
	}

	if (req.read_nwrite)
		return regs[offset];

	regs[offset] = req.data;
	return 0;
}

void acc_nop::acc_nop_thread()
{
	while (1) {
		int i = 0;
		noc_flit flit = fifo_from_ni.get();

		flit_check_head(flit);
		buf[i++] = flit.data;

		while (!flit.tail) {
			flit = fifo_from_ni.get();
			buf[i++] = flit.data;
		}

		for (i = 0; i < n_items; i++) {
			flit.head = i == 0;
			flit.tail = i == n_items - 1;
			flit.data = buf[i];
			fifo_to_ni.put(flit);
		}
	}
}
