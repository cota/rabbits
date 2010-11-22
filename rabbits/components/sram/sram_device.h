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

#ifndef _SRAM_DEVICE_H_
#define _SRAM_DEVICE_H_

#include <stdint.h>
#include <slave_device.h>

class sram_device : public slave_device
{
public:
    sram_device(const char *_name, unsigned long _size);
    virtual ~sram_device();

public:
    /*
     *   Obtained from father
     *   void send_rsp (bool bErr);
     */
    virtual void rcv_rqst(unsigned long ofs, unsigned char be,
                          unsigned char *data, bool bWrite);


    virtual unsigned char *get_mem() {return mem;}
    virtual unsigned long get_size() {return size;}

private:
    void write(uint32_t ofs, uint8_t be, uint8_t *data, bool &bErr);
    void read (uint32_t ofs, uint8_t be, uint8_t *data, bool &bErr);

private:
    uint32_t  size;
    uint8_t  *mem;
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
