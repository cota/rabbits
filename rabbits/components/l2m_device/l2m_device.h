#ifndef _L2M_DEVICE_H_
#define _L2M_DEVICE_H_

#include <mwsr_ta_fifo.h>
#include <slave_device.h>
#include <master_device.h>

struct l2m_req {
    unsigned long ofs;
    unsigned char be;
    uint8_t plen;
    uint8_t *data;
    bool bWrite;
    bool sleep;
};

struct l2m_rsp {
    bool bErr;
    bool sleep;
    uint8_t oob;
};

class l2m_device_slave : public slave_device
{
 public:
    SC_HAS_PROCESS(l2m_device_slave);
    l2m_device_slave(const char *_name, mwsr_ta_fifo<l2m_req> *reqs,
                     mwsr_ta_fifo<l2m_rsp> *rsps);
    ~l2m_device_slave();

 public:
    /*
     *   Obtained from parent:
     *   void send_rsp (bool bErr, uint8_t oob);
     */
    virtual void rcv_rqst(unsigned long ofs, unsigned char be,
                          unsigned char *data, bool bWrite, bool sleep);

 private:
    mwsr_ta_fifo<l2m_req>	*m_reqs;
    mwsr_ta_fifo<l2m_rsp>	*m_rsps;
};

class l2m_device_master : public master_device
{
 public:
    l2m_device_master(const char *_name, uint32_t node_id);
    ~l2m_device_master();

 public:
    /*
     *   Obtained from parent
     *    void send_req(unsigned char tid, unsigned long addr, unsigned char *data,
     *      unsigned char bytes, bool bWrite);
     */
    virtual void rcv_rsp(unsigned char tid, unsigned char *data,
                         bool bErr, bool bWrite, uint8_t oob);
    int cmd_write(uint32_t addr, uint32_t data, uint8_t nbytes, bool sleep);
    int cmd_read (uint32_t addr, uint8_t *data, uint8_t nbytes);

 private:
    uint8_t    m_crt_tid;
    sc_event   ev_cmd_done;
    uint32_t   m_tr_rdata;
    uint8_t    m_tr_nbytes;
};

#define L2M_LINE_BITS	5
#define L2M_LINE_BYTES	(1 << L2M_LINE_BITS)
#define L2M_LINE_MASK	(L2M_LINE_BYTES - 1)
#define L2M_ACCESS_TIME_NS	10

struct line_entry {
    uint32_t	tag;
    uint8_t	age;
    bool	valid;
    bool	dirty;
};

class l2m_device : public sc_module
{
 public:
    SC_HAS_PROCESS(l2m_device);
    l2m_device(sc_module_name _name, uint32_t node_id, size_t size_bits,
               size_t assoc_bits);
    virtual ~l2m_device();

 public:
    slave_device  *get_slave(void);
    master_device *get_master(void);

 private:
    /* Interfaces */
    l2m_device_master *master;
    l2m_device_slave  *slave;

    /* descriptors + mem */
    struct line_entry **m_entries;
    uint8_t *m_mem;
    struct line_entry *desc;

    int m_size_bits;
    int m_assoc_bits;
    int m_nr_lines_bits;
    int m_lps_bits;
    int m_lps;
    int m_tag_shift;
    int m_ways;

 private:
    void l2m_thread(void);
    void evict_line(int tag, int idx, int way);
    void fetch_line(int tag, int idx, int way, uint32_t offset, uint32_t b_off);

 private:
    inline uint32_t addr_to_tag(uint32_t addr)
    {
        return addr >> m_tag_shift;
    }

    inline uint32_t addr_to_idx(uint32_t addr)
    {
	return (addr >> L2M_LINE_BITS) & (m_lps - 1);
    }

    inline uint32_t build_l2_addr(int idx, int way)
    {
        return (way << (m_size_bits - m_assoc_bits)) | (idx << L2M_LINE_BITS);
    }

    inline uint32_t build_addr(int tag, int idx)
    {
	return ((tag) << m_tag_shift) | (idx << L2M_LINE_BITS);
    }

    inline bool is_a_hit(struct line_entry &entry, int &tag)
    {
	return entry.tag == tag && entry.valid;
    }

    inline void __entry_rw(struct line_entry &entry, int idx, int way, bool read)
    {
        if (read)
            entry = m_entries[idx][way];
        else
            m_entries[idx][way] = entry;
    }

    inline struct line_entry read_entry(int idx, int way)
    {
	struct line_entry entry;

	__entry_rw(entry, idx, way, 1);
	return entry;
    }

    inline void write_entry(struct line_entry &entry, int idx, int way)
    {
	__entry_rw(entry, idx, way, 0);
    }

 public:
    mwsr_ta_fifo<l2m_req> *m_l2_reqs;
    mwsr_ta_fifo<l2m_rsp> *m_l2_rsps;
};

#endif /* _L2M_DEVICE_H_ */
