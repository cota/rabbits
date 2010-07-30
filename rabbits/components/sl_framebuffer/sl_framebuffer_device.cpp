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

#include <systemc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sl_framebuffer_device.h>


//#define DEBUG_DEVICE_FB

#ifdef DEBUG_DEVICE_FB
#define DPRINTF(fmt, args...)                               \
    do { printf("sl_fb_device: " fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

#define EPRINTF(fmt, args...)                               \
    do { fprintf(stderr, "sl_fb_device: " fmt , ##args); } while (0)

static int          pids_ramdac[10];
static uint8_t     *shm_addr[10][2];
static int          shmids[10][2];
static int          nb_fb = 0;

void close_ramdacs ()
{
    int         i, status;

    DPRINTF("close called\n");

    if (nb_fb == 0)
        return;

    for (i = 0; i < nb_fb; i++)
    {
        int j = 0;
        for(j = 0; j < 2; j++){
            shmdt(shm_addr[i][j]);
            shmctl(shmids[i][j], IPC_RMID, NULL);
        }

        //cout << "Killing RAMDAC " << (int) i << endl;
        kill (pids_ramdac[i], SIGKILL);
        ::waitpid (pids_ramdac[i], &status, 0);
    }

    nb_fb = 0;
}

sl_fb_device::sl_fb_device (const char *_name, int fb_w, int fb_h, int fb_mode)
: slave_device (_name)
{
    buf_idx = 0;

    mode   = fb_mode;

    width  = fb_w;
    height = fb_h;

    switch(mode){
    case GREY:
        rgb_components = 1;
        yuv_size       = 0;
        break;
    case RGB32:
        rgb_components = 3;
        yuv_size       = 0;
        break;
    case YVYU: /* YUV 4:2:2 */
    case YV16:
        rgb_components = 3;
        yuv_size       = 2*width*height;
        break;
    case YV12: /* YUV 4:2:0 */
        rgb_components = 3;
        yuv_size       = (3*width*height)>>1;
        break;
    default:
        EPRINTF("Bad mode\n");
    }

    rgb_size = rgb_components*height*width;

    if(yuv_size > 0){
        int i = 0;
        for(i = 0; i < 2; i++){
            yuv_image[i] = (uint8_t *)malloc(sizeof(uint8_t)*yuv_size);
        }

    }

    DPRINTF("new framebuffer: %dx%d [%dB--0x%xB](%dw--0x%xw)\n", fb_w, fb_h,
            rgb_size, rgb_size, rgb_size>>2, rgb_size>>2);

    if (nb_fb == 0)
        atexit (close_ramdacs);

    init_ramdac();

    switch(mode){
    case GREY:
    case RGB32:
        write_buf = rgb_image;
        mem_size  = rgb_size;
        break;
    case YVYU: /* YUV 4:2:2 */
    case YV16:
    case YV12: /* YUV 4:2:0 */
        write_buf = yuv_image;
        mem_size  = yuv_size; 
        break;
    default:
        EPRINTF("Bad mode\n");
    }

}

sl_fb_device::~sl_fb_device ()
{

    DPRINTF("destructor called\n");

}

static uint32_t s_mask[16] = { 0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF,
                               0x00FF0000, 0x00FF00FF, 0x00FFFF00, 0x00FFFFFF,
                               0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,
                               0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF };

void sl_fb_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    uint32_t  *val = (uint32_t *)data;
    uint32_t   lofs = ofs;
    uint8_t    lbe  = be;

    uint32_t  *ptr = NULL;
    uint32_t   tmp = 0;
    uint32_t   mask = 0;

    bErr = false;

    lofs >>= 2;
    if (lbe & 0xF0)
    {
        lofs  += 1;
        lbe  >>= 4;
        val++;
    }

    if(lofs > (mem_size>>2)){
        EPRINTF("write outside mem area: %x / %x\n", ofs, mem_size);
        EPRINTF("Bad %s::%s ofs=0x%X, be=0x%X, data=0x%X-%X\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned long*)data + 0),
                (unsigned int) *((unsigned long*)data + 1));
        bErr = true;
        exit(EXIT_FAILURE);
    }

    if((ofs == 0) && (be & 0x1)){
        /* Each time we write at the zero position we update */
        display();
    }

    ptr = (uint32_t *)write_buf[buf_idx];
    tmp = ptr[lofs];
    mask = s_mask[lbe];

    //DPRINTF("write at idx: %x, of val: %x\n", lofs, *val);

    tmp &= ~mask;
    tmp |= (*val & mask);
    ptr[lofs] = tmp;

}

void sl_fb_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    bErr = false;
    EPRINTF("Bad %s::%s ofs=0x%X, be=0x%X\n", name (), __FUNCTION__,
            (unsigned int) ofs, (unsigned int) be);
    return;
}


void sl_fb_device::init_ramdac ()
{
    char            xname[80] = "*** The Screen ***";
    int             ppout[2];
    key_t           key[2];
    int             i, k;

    pipe(ppout);
    if( !(pid = fork()) ){

        setpgrp();
        /* close (STDIN_FILENO); */
        /* dup2 (ppout[0], STDIN_FILENO); */
        close (0);
        dup2 (ppout[0], 0);

        if (execlp ("xramdac", "xramdac", (char *)NULL) == -1)
        {
            perror ("viewer: execlp failed");
            _exit (1);
        }
    }
    if (pid == -1)
    {
        perror("viewer: fork failed");
        exit(1);
    }

    DPRINTF("Viewer child: %d\n", pid);

    pids_ramdac[nb_fb] = pid;
    pout = ppout[1];

    for (i = 0; i < 2; i++)
    {
        for (k = 1;; k++)
            if ((shmid[i] = shmget (k, rgb_size * sizeof (uint8_t),
                                    IPC_CREAT | IPC_EXCL | 0600)) != -1)
                break;
        DPRINTF("Got shm %x\n", k);
        key[i] = k;

        rgb_image[i] = (uint8_t *) shmat (shmid[i], 0, 00200);
        if (rgb_image[i] == (void *) -1)
        {
            perror ("ERROR: Ramdac.shmat");
            exit (1);
        }

        shmids[nb_fb][i]   = shmid[i];
        shm_addr[nb_fb][i] = rgb_image[i];
    }


    nb_fb++;
    
    ::write (pout, key, sizeof (key));
    ::write (pout, &width, sizeof (int));
    ::write (pout, &height, sizeof (int));
    ::write (pout, &rgb_components, sizeof (unsigned int));
    ::write (pout, xname, sizeof (xname));
}

void sl_fb_device::display (void)
{
    DPRINTF("display()\n");

    switch(mode){
    case GREY:
    case RGB32:
        /* Nothing to do */
        break;
    case YVYU: /* Packed YUV 4:2:2 */
        convert_frame_yvyu();
        break;
    case YV16: /* Planar YUV 4:2:2 */
        convert_frame_yv16();
        break;
    case YV12: /* Planar YUV 4:2:0 */
        convert_frame_yv12();
        break;
    default:
        EPRINTF("Bad mode\n");
    }

    buf_idx = (buf_idx + 1) % 2;
    kill (pid, SIGUSR1);
}

/* ======================================================= */
/* Theory ...                                              */
/* ------------------------------------------------------- */
/* C = Y - 16                                              */
/* D = U - 128                                             */
/* E = V - 128                                             */
/* R = clip(( 298 * C           + 409 * E + 128) >> 8)     */
/* G = clip(( 298 * C - 100 * D - 208 * E + 128) >> 8)     */
/* B = clip(( 298 * C + 516 * D           + 128) >> 8)     */
/* ======================================================= */

/* ======================================================= */
/* Practice ...                                            */
/* ------------------------------------------------------- */
/* C = Y - 16                                              */
/* D = U - 128                                             */
/* E = V - 128                                             */
/*                                                         */
/* tmp_r = 409 * E + 128                                   */
/* tmp_g = -100 * D - 208 * E                              */
/* tmp_b = 516 * D                                         */
/* tmp_c = 298 * C                                         */
/*                                                         */
/* R = clip(( tmp_c + tmp_r ) >> 8)                        */
/* G = clip(( tmp_c + tmp_g ) >> 8)                        */
/* B = clip(( tmp_c + tmp_b ) >> 8)                        */
/* ======================================================= */

void
yuv2rgb(uint8_t y, uint8_t u, uint8_t v, uint8_t *rgb){

    int32_t y_tmp = (int32_t)y - 16;
    int32_t u_tmp = (int32_t)u - 128;
    int32_t v_tmp = (int32_t)v - 128;

    int32_t r_tmp = 0;
    int32_t g_tmp = 0;
    int32_t b_tmp = 0;

    uint8_t r_val = 0;
    uint8_t g_val = 0;
    uint8_t b_val = 0;
        
    y_tmp *= 298;
    
    r_tmp = y_tmp +            0 +    v_tmp*409 + 128;
    g_tmp = y_tmp + u_tmp*(-100) + v_tmp*(-208) + 128;
    b_tmp = y_tmp + u_tmp*516    +            0 + 128;

    r_tmp >>= 8;
    g_tmp >>= 8;
    b_tmp >>= 8;

    r_val = ( (r_tmp < (1<<8)) ? ((r_tmp >= 0)?r_tmp:0) : 0xFF);
    g_val = ( (g_tmp < (1<<8)) ? ((g_tmp >= 0)?g_tmp:0) : 0xFF);
    b_val = ( (b_tmp < (1<<8)) ? ((b_tmp >= 0)?b_tmp:0) : 0xFF);
    
    rgb[0] = (uint8_t)r_val;
    rgb[1] = (uint8_t)g_val;
    rgb[2] = (uint8_t)b_val;
        
    return;
}

void
sl_fb_device::convert_frame_yvyu(void){

    return;
}

void
sl_fb_device::convert_frame_yv16(void){

    int      i = 0, j = 0;
    uint8_t  y, u, v;
    uint8_t *y_buf = yuv_image[buf_idx];
    uint8_t *u_buf = y_buf + (width*height);
    uint8_t *v_buf = u_buf + (width*height)/2;

    uint8_t *rgb   = rgb_image[buf_idx];

    for(i = 0; i < (width*height/2); i++){
        
        y = y_buf[i*2];
        u = u_buf[i];
        v = v_buf[i];
        
        yuv2rgb(y, u, v, rgb);
        rgb += 3;

        y = y_buf[i*2 + 1];

        yuv2rgb(y, u, v, rgb);
        rgb += 3;

    }

    return;
}

void
sl_fb_device::convert_frame_yv12(void){

    

    return;
}


void sl_fb_device::rcv_rqst (unsigned long ofs, unsigned char be,
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
