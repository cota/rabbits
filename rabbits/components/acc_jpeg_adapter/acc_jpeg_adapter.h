#ifndef _ACC_JPEG_ADAPTER_H_
#define _ACC_JPEG_ADAPTER_H_

#include <systemc.h>
#include <tlm.h>
using namespace tlm;

#include <mumgr_package.h>

#include <jpgblock.h>

class acc_jpeg_adapter : public sc_module
{
 public:
	SC_HAS_PROCESS(acc_jpeg_adapter);
	acc_jpeg_adapter(sc_module_name _name);
	~acc_jpeg_adapter(void);

 private:
	void pull_thread();
	void push_thread();

 public:
	sc_export<tlm_blocking_put_if<noc_flit> > from_ni;
	sc_export<tlm_blocking_get_if<noc_flit> > to_ni;

	sc_export<tlm_blocking_put_if<JpgBlock<unsigned char> > > from_acc;
	sc_export<tlm_blocking_get_if<JpgBlock<unsigned char> > > to_acc;

 private:
	tlm_fifo<noc_flit>			fifo_from_ni;
	tlm_fifo<noc_flit>			fifo_to_ni;
	tlm_fifo<JpgBlock<unsigned char> >	fifo_to_acc;
	tlm_fifo<JpgBlock<unsigned char> >	fifo_from_acc;
};

#endif /* _ACC_JPEG_ADAPTER_H_ */
