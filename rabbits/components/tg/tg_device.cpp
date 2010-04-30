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
#include <tg_device.h>

tg_device::tg_device (const char *_name, const char *filename) : slave_device (_name)
{
    tg_file = fopen (filename, "rb");
    if(tg_file == NULL)
    {
        fprintf(stderr, "Error in %s: file %s can not be opened!\n", name (), filename);
        exit (1);
    }

    tg_bytes_left = 0;
    reset_input ();
}

tg_device::~tg_device ()
{
    if (tg_file)
        fclose (tg_file);
}

void tg_device::reset_input ()
{
    fseek (tg_file, 0, SEEK_END);
    tg_bytes_left += ftell (tg_file);
    fseek (tg_file, 0, SEEK_SET);
}

void tg_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    switch (ofs)
    {
    case 0x480:
        break;
    default:
        printf ("Bad %s:%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n", name (),
                __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned long*)data + 0), (unsigned int) *((unsigned long*)data + 1));
        exit (1);
        break;
    }
    bErr = false;
}

void tg_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int             i;

    *((unsigned long *)data + 0) = 0;
    *((unsigned long *)data + 1) = 0;

    switch (ofs)
    {
    case 0x00:
        for (i = 0; i < 4; i++)
        {
            if (!tg_bytes_left)
                reset_input ();
            fread (data + i, sizeof (unsigned char), 1, tg_file);
            tg_bytes_left --;
        }
        break;
    case 0x80:
        if (!tg_bytes_left)
            reset_input ();
        *((unsigned long *)data + 0) = (tg_bytes_left + 3) / 4;
        break;
    default:
        printf ("Bad %s:%s ofs=0x%X, be=0x%X!\n",  name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
        exit (1);
    }
    bErr = false;
}

void tg_device::rcv_rqst (unsigned long ofs, unsigned char be,
                          unsigned char *data, bool bWrite)
{

    bool bErr = false;

    if(bWrite){
        this->write(ofs, be, data, bErr);
    }else{
        this->read(ofs, be, data, bErr);
    }

    send_rsp(bErr);

    return;
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
