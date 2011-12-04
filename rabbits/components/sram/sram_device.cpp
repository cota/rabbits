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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sram_device.h>

//#define DEBUG_SRAM

#ifdef DEBUG_SRAM
#define DPRINTF(fmt, args...)                   \
    do { printf("sram_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

#define EPRINTF(fmt, args...)                               \
    do { fprintf(stderr, "sram_device: " fmt , ##args); } while (0)

sram_device::sram_device(const char *_name, unsigned long _size)
: slave_device(_name)
{
    mem = NULL;
    size = _size;
    mem = new unsigned char [size];
    memset(mem, 0, size);
}

sram_device::~sram_device()
{
    if (mem)
        delete [] mem;
}

void 
sram_device::write (uint32_t ofs, uint8_t be, uint8_t *data, bool &bErr)
{
    int       loffs = 0; /* data offset */
    int       lwid  = 0; /* access width  */
    int       err   = 0;

    bErr = false;
    wait (1, SC_NS);

    if(ofs > size || be == 0)
        err = 1;

    if(!err){
        switch(be){
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

        if (!err)
            switch (lwid)
            {
            case 1:
                *((uint8_t *)(mem + ofs) + loffs) =
                    *((uint8_t *)data + loffs);
                break;
            case 2:
                *((uint16_t *) (mem + ofs) + loffs) =
                    *((uint16_t *) data + loffs);
                break;
            case 4:
                *((uint32_t *) (mem + ofs) + loffs) =
                    *((uint32_t *) data + loffs);
                break;
            case 8:
                *((uint32_t *) (mem + ofs) + 0) = *((uint32_t *) data + 0);
                *((uint32_t *) (mem + ofs) + 1) = *((uint32_t *) data + 1);
                break;
            default:
                err = 1;
            }
    }

    if(err == 1){
        EPRINTF("Bad %s:%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                name(), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                *((uint32_t *)data + 0), *((uint32_t *)data + 1));
        bErr = true;
    }
}

void
sram_device::read (uint32_t ofs, uint8_t be, uint8_t *data, bool &bErr)
{
    int loffs = 0; /* data offset  */
    int lwid  = 0; /* access width */ 
    int err   = 0;

    bErr = false;
    wait (1, SC_NS);

    if (ofs >= size || be == 0)
        err = 1;

    if (!err)
    {
        switch (be)
        {
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

        if (!err)
        {
            switch (lwid)
            {
            case 1:
                *((uint8_t *)data + loffs) =
                    *((uint8_t *)(mem + ofs) + loffs);
                break;
            case 2:
                *((uint16_t *)data + loffs) =
                    *((uint16_t *)(mem + ofs) + loffs);
                break;
            case 4:
                *((uint32_t *)data + loffs) =
                    *((uint32_t *)(mem + ofs) + loffs);
                break;
            case 8:
                *((uint32_t *)data + 0) = *((uint32_t *)(mem + ofs) + 0);
                *((uint32_t *)data + 1) = *((uint32_t *)(mem + ofs) + 1);
                break;
            default:
                err = 1;
            }
        }
    }

    if(err == 1)
    {
        printf("Bad %s:%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                name(), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                *((uint32_t *)data + 0), *((uint32_t *)data + 1));
        bErr = true;
    }
}

void
sram_device::rcv_rqst(unsigned long ofs, unsigned char be,
                      unsigned char *data, bool bWrite)
{

    bool bErr = false;

    if(bWrite)
        this->write(ofs, be, data, bErr);
    else
        this->read(ofs, be, data, bErr);

    send_rsp (bErr, 0);
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
