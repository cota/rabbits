#include "acc_jpeg_adapter.h"

#include <flit_check.h>

#ifdef DEBUG_ACC_JPEG_ADAPTER
#define DPRINTF(fmt, args...)						\
	do { printf("acc_jpeg_adapter: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

#define EPRINTF(fmt, args...)						\
	do { fprintf(stderr, "sl_nof_adapter: " fmt , ##args); } while (0)

#define LEN	(DCTSIZE2 / sizeof(uint32_t))

acc_jpeg_adapter::acc_jpeg_adapter(sc_module_name _name)
{
	to_acc(fifo_to_acc);
	from_acc(fifo_from_acc);

	from_ni(fifo_from_ni);
	to_ni(fifo_to_ni);

	SC_THREAD(push_thread);
	SC_THREAD(pull_thread);
}

acc_jpeg_adapter::~acc_jpeg_adapter()
{ }

void acc_jpeg_adapter::pull_thread()
{
	JpgBlock<unsigned char>	jpg_block;
	noc_flit flit;
	uint32_t *p;
	int i;

	while (1) {
		i = 0;
		do {
			flit = fifo_from_ni.get();
			if (i == 0)
				flit_check_head(flit);

			p = (uint32_t *)&jpg_block.array[i++ << 2];
			*p = flit.data;
		} while (!flit.tail);

		if (i != LEN) {
			EPRINTF("Warning %s::%s rec packet of len=%d, expected %d items\n",
				name(), __func__, i, LEN);
		}

		fifo_to_acc.put(jpg_block);
	}
}

void acc_jpeg_adapter::push_thread()
{
	JpgBlock<unsigned char>	jpg_block;
	noc_flit flit;
	uint32_t *p;

	while (1) {
		jpg_block = fifo_from_acc.get();

		for (int i = 0; i < LEN; i++) {
			flit.head = i == 0;
			flit.tail = i == LEN - 1;

			p = (uint32_t *)&jpg_block.array[i << 2];
			flit.data = *p;
			fifo_to_ni.put(flit);
		}
	}
}
