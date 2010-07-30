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

#ifndef _SL_FB_DEVICE_H_
#define _SL_FB_DEVICE_H_

#include <slave_device.h>

enum fb_mode{
    GREY   = 0,
    RGB32  = 1,
    YVYU   = 2, /* Packed YUV 4:2:2 */
    YV12   = 3, /* Planar YUV 4:2:0 */
    YV16   = 4, /* Planar YUV 4:2:2 */
};

class sl_fb_device : public slave_device
{
public:
    sl_fb_device (const char *_name, int fb_w, int fb_h, int fb_mode);
    virtual ~sl_fb_device ();

public:
    /*
     *   Obtained from father
     *   void send_rsp (bool bErr);
     */
    virtual void rcv_rqst (unsigned long ofs, unsigned char be,
                           unsigned char *data, bool bWrite);

private:

    void write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr);
    void read  (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr);

    void display(void);
    void init_ramdac(void);
    void receive_size (unsigned int data);

    void convert_frame_yvyu(void);
    void convert_frame_yv16(void);
    void convert_frame_yv12(void);

private:
    int             pout;  /* pipe to Xramdac        */
    int             pid;   /* Xramdac PID            */

    int             mode;  /* current mode of the FB */

    int             width;
    int             height;

    /* YUV part ... processed */
    int             yuv_size;
    uint8_t        *yuv_image[2];  /* local buffer for YUV2RGB conversion */

    /* RGB Part ... displayed */
    int             rgb_components;
    int             rgb_size;
    uint8_t        *rgb_image[2];  /* shared memory with the viewer       */

    /* int             w, h; */

    int             buf_idx;       /* Double buffering index */
    uint8_t       **write_buf;     /* addresses of the memory area */
    int             mem_size;      /* size of this area(s) */


    int             shmid[2];


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
