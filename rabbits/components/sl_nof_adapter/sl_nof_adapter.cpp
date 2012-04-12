#include <systemc.h>

#include <sl_nof_adapter.h>

#ifdef DEBUG_SL_NOF_ADAPTER
#define DPRINTF(fmt, args...)						\
	do { printf("sl_nof_adapter: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

#define EPRINTF(fmt, args...)						\
	do { fprintf(stderr, "sl_nof_adapter: " fmt , ##args); } while (0)

sl_nof_adapter::sl_nof_adapter(sc_module_name _name, size_t _wbuf_items,
			       size_t _rbuf_items)
	: slave_device(_name)
{
	m_wbuf_items = _wbuf_items;
	m_rbuf_items = _rbuf_items;

	m_wbuf = new uint32_t[m_wbuf_items];
	m_rbuf = new uint32_t[m_rbuf_items];
	m_regs = new uint32_t[NOF_ADAPTER_NR_REGS];

	m_regs[NOF_ADAPTER_REG_WR_DONE] = 1;

	SC_THREAD(send_packet_thread);
	SC_THREAD(recv_packet_thread);
}

sl_nof_adapter::~sl_nof_adapter()
{
	if (m_wbuf)
		delete[] m_wbuf;
	if (m_rbuf)
		delete[] m_rbuf;
	if (m_regs)
		delete[] m_regs;
}

void sl_nof_adapter::send_packet_thread(void)
{
	size_t len = m_wbuf_items;
	noc_flit flit;

	while (1) {
		wait(m_ev_wbuf_full);
		DPRINTF("send packet\n");

		for (int i = 0; i < len; i++) {
			flit.head = i == 0;
			flit.tail = i == len - 1;
			flit.data = m_wbuf[i];

			to_acc->put(flit);
		}
		m_regs[NOF_ADAPTER_REG_WR_DONE] = 1;
	}
}

void sl_nof_adapter::write_acc_reg(uint32_t offset, uint32_t *val, bool &bErr)
{
	rw_req_transaction_32 req;

	req.address = offset;
	req.read_nwrite = false;
	req.data = *val;
	acc_regs->transport(req);

	DPRINTF("%s:%d offset 0x%x val 0x%x (%d)\n",
	       __func__, __LINE__, offset, *val, *val);

	bErr = false;
}

void sl_nof_adapter::write_reg(uint32_t offset, uint32_t *val, bool &bErr)
{
	bErr = false;

	switch (offset) {
	case NOF_ADAPTER_REG_WR_SEND:
		m_regs[NOF_ADAPTER_REG_WR_DONE] = 0;
		m_ev_wbuf_full.notify();
		break;
	case NOF_ADAPTER_REG_RD_EMPTY:
		m_regs[NOF_ADAPTER_REG_RD_FULL] = 0;
		m_ev_rbuf_empty.notify();
		break;
	default:
		break;
	}
}

void sl_nof_adapter::write(unsigned long ofs, unsigned char be,
			   unsigned char *data, bool &bErr)
{
	uint32_t  *val = (uint32_t *)data;
	uint32_t   lofs = ofs;
	uint8_t    lbe  = be;

	uint32_t  *ptr = NULL;
	uint32_t   tmp = 0;
	uint32_t   mask = 0;
	uint32_t   max_offset = m_wbuf_items - 1;

	lofs >>= 2;
	if (lbe & 0xF0)
	{
		lofs  += 1;
		lbe  >>= 4;
		val++;
	}

	if (lofs >= (NOF_ADAPTER_ACC_REGS_BASE >> 2))
		return write_acc_reg(lofs - (NOF_ADAPTER_ACC_REGS_BASE >> 2), val, bErr);
	if (lofs >= (NOF_ADAPTER_REGS_BASE >> 2))
		return write_reg(lofs - (NOF_ADAPTER_REGS_BASE >> 2), val, bErr);

	bErr = false;

	if (lofs > max_offset) {
		EPRINTF("write outside mem area: %ld / %d\n", ofs, m_wbuf_items);
		EPRINTF("Bad %s::%s ofs=%d, be=0x%X, data=0x%X-%X\n",
			name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
			(unsigned int) *((unsigned long*)data + 0),
			(unsigned int) *((unsigned long*)data + 1));
		bErr = true;
		exit(EXIT_FAILURE);
	}

	m_wbuf[lofs] = *val;

	DPRINTF("write at idx: %d, of val: 0x%x, max_offset=%d\n", lofs, *val, max_offset);
}

void sl_nof_adapter::recv_packet_thread(void)
{
	size_t len = m_wbuf_items;
	noc_flit flit;
	int i;

	while (1) {
		wait(m_ev_rbuf_empty);

		i = 0;
		do {
			from_acc->get(flit);
			m_rbuf[i++] = flit.data;
		} while (!flit.tail);

		if (i != len) {
			EPRINTF("Warning %s::%s recv packet of len=%d, expected %d\n",
				name(), __func__, i, len);
		}
		m_regs[NOF_ADAPTER_REG_RD_FULL] = 1;
	}
}

void sl_nof_adapter::read_acc_reg(uint32_t offset, uint32_t *val, bool &bErr)
{
	rw_req_transaction_32 req;

	bErr = false;
	req.read_nwrite = true;
	req.address = offset;
	req.data = ~0;
	*val = acc_regs->transport(req);
}

void sl_nof_adapter::read_reg(uint32_t offset, uint32_t *val, bool &bErr)
{
	bErr = false;

	switch (offset) {
	case NOF_ADAPTER_REG_WR_DONE:
	case NOF_ADAPTER_REG_RD_FULL:
		*val = m_regs[offset];
		break;
	default:
		*val = ~0;
		break;
	}
}

void sl_nof_adapter::read(unsigned long ofs, unsigned char be,
			  unsigned char *data, bool &bErr)
{
	uint32_t *val = (uint32_t *)data;
	uint32_t lofs = ofs;
	uint8_t lbe  = be;
	uint32_t max_offset = m_wbuf_items - 1;

	bErr = false;

	lofs >>= 2;
	if (lbe & 0xF0)
	{
		lofs  += 1;
		lbe  >>= 4;
		val++;
	}

	if (lofs >= NOF_ADAPTER_ACC_REGS_BASE >> 2)
		return read_acc_reg(lofs - NOF_ADAPTER_ACC_REGS_BASE >> 2, val, bErr);
	if (lofs >= NOF_ADAPTER_REGS_BASE >> 2)
		return read_reg(lofs - NOF_ADAPTER_REGS_BASE >> 2, val, bErr);

	if (lofs > max_offset) {
		EPRINTF("Bad %s::%s ofs=%d, be=0x%X\n", name (), __FUNCTION__,
			(unsigned int) ofs, (unsigned int) be);
	}

	*val = m_rbuf[lofs];
	DPRINTF("read at idx: %d, of val: %x\n", lofs, *val);
}

void
sl_nof_adapter::rcv_rqst(unsigned long ofs, unsigned char be,
			 unsigned char *data, bool bWrite, bool sleep)
{
	bool bErr = false;

	if (bWrite)
		this->write(ofs, be, data, bErr);
	else
		this->read(ofs, be, data, bErr);

	send_rsp(bErr, 0);
}
