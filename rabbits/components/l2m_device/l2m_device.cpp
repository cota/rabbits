#include <systemc.h>

#include <l2m_device.h>

#ifdef DEBUG_L2M
#define DPRINTF(fmt, args...)                               \
    do { printf("l2m_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

#define EPRINTF(fmt, args...)                               \
    do { fprintf(stderr, "l2m_device: " fmt , ##args); } while (0)

l2m_device::l2m_device(sc_module_name _name, uint32_t master_id,
                       size_t size_bits, size_t assoc_bits)
    : sc_module(_name)
{
    char *buf = new char[strlen(_name) + 3];

    strcpy (buf, _name);
    buf[strlen (_name)] = '_';
    buf[strlen (_name) + 2] = '\0';

    m_l2_reqs = new mwsr_ta_fifo<l2m_req> (8);
    m_l2_rsps = new mwsr_ta_fifo<l2m_rsp> (8);

    buf[strlen (_name) + 1] = 's';
    slave = new l2m_device_slave(buf, m_l2_reqs, m_l2_rsps);

    buf[strlen(_name) + 1] = 'm';
    master = new l2m_device_master(buf, master_id);

    m_size_bits = size_bits;
    m_assoc_bits = assoc_bits;
    m_nr_lines_bits = size_bits - L2M_LINE_BITS;
    m_lps_bits = m_nr_lines_bits - assoc_bits;
    m_lps = 1 << m_lps_bits;
    m_tag_shift = m_lps_bits + L2M_LINE_BITS;
    m_ways = 1 << assoc_bits;

    m_entries = new struct line_entry *[m_lps];
    for (int i = 0; i < m_lps; i++)
        m_entries[i] = new struct line_entry[m_ways];

    m_mem = new uint8_t [1 << m_size_bits];

    desc = new struct line_entry [m_ways];

    struct line_entry entry;
    entry.valid = false;
    /* invalidate all lines on reset */
    for (int i = 0; i < m_lps; i++) {
        for (int way = 0; way < m_ways; way++) {
            /*
             * Invariant: after every cache operation, the
             * ages in a given set are all different.
             */
            entry.age = way;

            write_entry(entry, i, way);
        }
    }

    SC_THREAD(l2m_thread);
}

l2m_device::~l2m_device()
{
    if (m_l2_reqs)
        delete m_l2_reqs;
    if (m_l2_rsps)
        delete m_l2_rsps;

    for (int i = 0; i < m_lps; i++)
        delete[] m_entries[i];
    delete[] m_entries;

    delete[] m_mem;
    delete[] desc;
}

master_device *
l2m_device::get_master()
{
    return (master_device *)master;
}

slave_device *
l2m_device::get_slave()
{
    return (slave_device *)slave;
}

void l2m_device::l2m_thread()
{
    while (1) {
        int ret;
        l2m_req req = m_l2_reqs->Read();

        int loffs = 0; /* data offset  */
        int lwid  = 0; /* access width */
        int err   = 0;

        bool bErr = false;
        bool hit = false;

        if (req.plen > 1) {
            loffs = 0;
            lwid = 8;
        } else {

            switch (req.be) {
                //byte access
            case 0x01: loffs = 0; lwid = 1; break;
            case 0x02: loffs = 1; lwid = 1; break;
            case 0x04: loffs = 2; lwid = 1; break;
            case 0x08: loffs = 3; lwid = 1; break;
            case 0x10: loffs = 4; lwid = 1; break;
            case 0x20: loffs = 5; lwid = 1; break;
            case 0x40: loffs = 6; lwid = 1; break;
            case 0x80: loffs = 7; lwid = 1; break;
                //word access
            case 0x03: loffs = 0; lwid = 2; break;
            case 0x0C: loffs = 1; lwid = 2; break;
            case 0x30: loffs = 2; lwid = 2; break;
            case 0xC0: loffs = 3; lwid = 2; break;
                //dword access
            case 0x0F: loffs = 0; lwid = 4; break;
            case 0xF0: loffs = 1; lwid = 4; break;
                //qword access
            case 0xFF: loffs = 0; lwid = 8; break;
            default:
                err = 1;
            }
        }

        if (!err) {
            uint32_t data = 0;
            req.ofs += loffs * lwid;

            if (req.bWrite) {
                void *orig = req.data;

                switch (lwid) {
                case 1:
                    data = *(((uint8_t *)orig) + loffs);
                    break;
                case 2:
                    data = *(((uint16_t *)orig) + loffs);
                    break;
                case 4:
                    data = *(((uint32_t *)orig) + loffs);
                    break;
                case 8:
                default: /* cannot fail */
                    data = *(((uint64_t *)orig) + loffs);
                }
            }

            struct line_entry entry;
            int tag = addr_to_tag(req.ofs);
            int idx = addr_to_idx(req.ofs);
            int way;
            int lru_way = 0;
            int i;

            DPRINTF("addr (%s) 0x%08lx tag 0x%x idx 0x%x loffs 0x%x lwid %d\n",
                    req.bWrite ? "w" : "r", req.ofs, tag, idx, loffs, lwid);

            /*
             * Note: in the main loop, we read the necessary entries
             * only at the beginning of an interation, and write
             * them back at the end. By doing so we can easily
             * unroll the corresponding loops and thus access the
             * arrays in parallel during the same clock cycle.
             */
            for (i = 0; i < m_ways; i++)
                desc[i] = read_entry(idx, i);

            for (way = 0; way < m_ways; way++) {
                /* if we miss, desc[lru_way] will be what we need */
                if (desc[way].age > desc[lru_way].age)
                    lru_way = way;

                hit = is_a_hit(desc[way], tag);
                if (hit)
                    break;
            }

            if (!hit) {
                DPRINTF("miss for tag 0x%x, idx 0x%x\n", tag, idx);
                way = lru_way;
            } else {
                DPRINTF("hit for tag 0x%x, idx 0x%x, way %d\n", tag, idx, way);
            }

            uint32_t offset = build_l2_addr(idx, way);
	    uint32_t b_off = offset + ((req.ofs) & L2M_LINE_MASK);

            wait (10, SC_NS);

            if (!req.bWrite) { /* read */
                if (!hit) {
                    uint32_t addr = build_addr(tag, idx);
                    uint32_t mem_addr = 0;
                    ret = master->cmd_read(addr, (uint8_t *)&mem_addr, L2M_LINE_BYTES);
                    if (!ret)
                        err = 1;
		    DPRINTF("offset 0x%x\n", offset);
                    memcpy(&m_mem[offset], (void *)mem_addr, L2M_LINE_BYTES);
		    DPRINTF("update tag 0x%x, idx %d\n", tag, idx);
                    desc[way].tag = tag;
                    desc[way].valid = true;
                }
                if (lwid == 8) {
			*(uint32_t *)req.data = (uint32_t)&m_mem[offset];
                } else {
			DPRINTF("%d: 0x%x 0x%x\n", __LINE__, (int)req.ofs, (int)b_off);
                    memcpy(req.data, &m_mem[b_off], lwid);
                }
            } else { /* write */
		    if (hit) {
			    DPRINTF("%d: 0x%lx 0x%x\n", __LINE__, req.ofs, b_off);
			    memcpy(&m_mem[b_off], &data, lwid);
		    }
                ret = master->cmd_write(req.ofs, data, lwid);
                if (!ret)
                    err = 1;
            }

            if (hit || !req.bWrite) {
                for (i = 0; i < m_ways; i++) {
                    if (desc[i].age < desc[way].age) {
                        desc[i].age++;
                    }
                }
                desc[way].age = 0;
            }

            for (i = 0; i < m_ways; i++)
                write_entry(desc[i], idx, i);
	}

        l2m_rsp rsp;
        if (err) {
            EPRINTF("Error in mem access: ofs 0x%lx loffs 0x%x lwid %d\n",
                    req.ofs, loffs, lwid);
            rsp.bErr = true;
            rsp.oob = OOB_NONE;
            m_l2_rsps->Write(rsp);
        } else {
            rsp.bErr = false;
            rsp.oob = OOB_CACHE_MISS;
            if (!req.bWrite && hit)
                rsp.oob = OOB_CACHE_HIT;
            m_l2_rsps->Write(rsp);
        }
    }
}

l2m_device_slave::l2m_device_slave(const char *_name, mwsr_ta_fifo<l2m_req> *reqs,
                                   mwsr_ta_fifo<l2m_rsp> *rsps)
    : slave_device(_name)
{
    m_reqs = reqs;
    m_rsps = rsps;
}

l2m_device_slave::~l2m_device_slave()
{ }

void
l2m_device_slave::rcv_rqst(unsigned long ofs, unsigned char be,
                     unsigned char *data, bool bWrite, bool sleep)
{
    l2m_req req;

    req.ofs = ofs;
    req.be = be;
    req.plen = this->m_req.plen;
    req.data = data;
    req.bWrite = bWrite;
    m_reqs->Write(req);

    l2m_rsp rsp = m_rsps->Read();

    send_rsp(rsp.bErr, rsp.oob);
}

l2m_device_master::l2m_device_master(const char *_name, uint32_t node_id)
    : master_device(_name)
{
    m_crt_tid = 0;
    m_node_id = node_id;
}

l2m_device_master::~l2m_device_master()
{ }

void
l2m_device_master::rcv_rsp(uint8_t tid, uint8_t *data,
                           bool bErr, bool bWrite, uint8_t oob)
{
    if (tid != m_crt_tid) {
        EPRINTF("Bad tid (%d / %d)\n", tid, m_crt_tid);
    }

    if (!bWrite) {
        for (int i = 0; i < m_tr_nbytes; i++)
            ((unsigned char *) &m_tr_rdata)[i] = data[i];
    }
    m_crt_tid++;

    ev_cmd_done.notify();
}

int
l2m_device_master::cmd_write(uint32_t addr, uint32_t data, uint8_t nbytes)
{
    send_req(m_crt_tid, addr, (unsigned char *)&data, nbytes, 1);
    wait(ev_cmd_done);
    return 1;
}

int
l2m_device_master::cmd_read(uint32_t addr, uint8_t *data, uint8_t nbytes)
{
    m_tr_nbytes = nbytes;
    m_tr_rdata  = 0;

    send_req(m_crt_tid, addr, NULL, nbytes, 0);
    wait(ev_cmd_done);

    if (nbytes > 4)
	    nbytes = 4;
    for (int i = 0; i < nbytes; i++) {
	    data[i] = ((unsigned char *) &m_tr_rdata)[i];
    }

    return 1;
}
