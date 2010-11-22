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
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h> 

#include <SDL/SDL.h>

#include "conv.h"

#define DEBUG_FBVIEWER

#ifdef DEBUG_FBVIEWER
#define DPRINTF(fmt, args...)                               \
    do { fprintf(stdout, "fbviewer: " fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#endif

#define EPRINTF(fmt, args...)                               \
    do { fprintf(stderr, "fbviewer: " fmt , ##args); } while (0)


enum fb_mode{
    NONE   = 0,
    /* **************************** */
    GREY   = 1,
    /* **************************** */
    RGB    = 2, /* pix[0] = R       *
                 * pix[1] = G       *
                 * pix[2] = B       */
    /* **************************** */
    BGR    = 3, /* pix[0] = B       *
                 * pix[1] = G       *
                 * pix[2] = R       */
    /* **************************** */
    ARGB   = 4, /* pix[0] = A       *
                 * pix[1] = R       *
                 * pix[2] = G       *
                 * pix[3] = B       */
    /* **************************** */
    BGRA   = 5, /* pix[0] = B       *
                 * pix[1] = G       *
                 * pix[2] = R       *
                 * pix[3] = A       */
    /* **************************** */
    YVYU   = 6, /* Packed YUV 4:2:2 */
    /* **************************** */
    YV12   = 7, /* Planar YUV 4:2:0 */
    /* **************************** */
    IYUV   = 8, /* Planar YUV 4:2:0 */
    /* **************************** */
    YV16   = 9, /* Planar YUV 4:2:2 */
};

void refresh(int);
void convert_and_display(void);
static void (*convert)(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h);

static SDL_Surface     *surface;    /* for RGB purposes */
#if 0
static SDL_Overlay     *overlay;    /* for YUV purposes */
#endif
static SDL_PixelFormat *pix_fmt;

static uint8_t    *image[2];
static uint32_t    width;
static uint32_t    height;
static uint32_t    size;
static uint32_t    mode;
static int         shmid[2];

static uint32_t    buf_idx;
static uint8_t    *copy;

void
clean_shm(void){
    int i = 0;
    DPRINTF("XRAMDAC exited\n");
    for(i = 0; i < 2; i++){
        shmdt(image[i]);
    }
}
char *print_mode(int mode) {

    switch(mode) {
    case NONE:
        return (char *)"NONE";
    case RGB:
        return (char *)"RGB";
    case BGR:
        return (char *)"BGR";
    case ARGB:
        return (char *)"ARGB";
    case BGRA:
        return (char *)"BGRA";
    case YVYU:
        return (char *)"YVYU";
    case YV12:
        return (char *)"YV12";
    case IYUV:
        return (char *)"IYUV";
    case YV16:
        return (char *)"YV16";
    default:
        return (char *)"UNKN";
    }
}

int
get_size(uint32_t width, uint32_t height, uint32_t mode){

    int size = 0;
    switch(mode){

    case NONE:
        size = 0;
        break;
    case RGB:
    case BGR:
        size = 3 * width * height;
        break;
    case ARGB:
    case BGRA:
        size = 4 * width * height;
        break;
    case YVYU: /* YUV 4:2:2 */
    case YV16:
        size = 2*width*height;
        break;
    case YV12: /* YUV 4:2:0 */
    case IYUV:
        size = (3*width*height)>>1;
        break;
    default:
        DPRINTF("Unknown mode\n");
    }
    return size;
}

uint32_t
yuv_format(uint32_t mode){

    switch(mode){
        
    case YVYU: /* YUV 4:2:2 */
        return SDL_YVYU_OVERLAY;
    case YV16:
        return SDL_YV12_OVERLAY;

    case YV12: /* YUV 4:2:0 */
        return SDL_YV12_OVERLAY;
    case IYUV: /* YUV 4:2:0 */
        return SDL_IYUV_OVERLAY;

    case NONE:
    case RGB:
    case BGR:
    case ARGB:
    case BGRA:
    default:
        DPRINTF("Unknown mode\n");
        return 0;
    }
}

int
need_an_overlay(int mode){

    int res = 0;
    switch(mode){
    case NONE:
    case YV16:
    case RGB:
    case BGR:
    case ARGB:
    case BGRA:
        res = 0;
        break;
    case YVYU: /* YUV 4:2:2 */
    case YV12: /* YUV 4:2:0 */
    case IYUV: /* YUV 4:2:0 */
        res = 0;
        break;
    default:
        DPRINTF("Unknown mode\n");
    }
    return res;
}

void
set_conversion(uint32_t mode){

    switch(mode){

    case NONE:
        break;

    case RGB:
        /* R : pix[0] */
        /* G : pix[1] */
        /* B : pix[2] */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[0] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[1] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[2] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[3] */
            convert = convert_RGB_ARGB;
        }else{
            convert = convert_RGB_BGRA;
        }
#else /* LITTLE_ENDIAN */
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[3] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[2] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[1] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[0] */
            convert = convert_RGB_BGRA;
        }else{
            convert = convert_RGB_ARGB;
        }
#endif
        break;

    case BGR:
        /* R : pix[2] */
        /* G : pix[1] */
        /* B : pix[0] */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[0] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[1] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[2] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[3] */
            convert = convert_BGR_ARGB;
        }else{
            convert = convert_BGR_BGRA;
        }
#else /* LITTLE_ENDIAN */
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[3] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[2] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[1] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[0] */
            convert = convert_BGR_BGRA;
        }else{
            convert = convert_BGR_ARGB;
        }
#endif
        break;

    case ARGB: 
        /* A : pix[0] */
        /* R : pix[1] */
        /* G : pix[2] */
        /* B : pix[3] */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[0] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[1] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[2] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[3] */
            convert = convert_ARGB_copy_32;
        }else{
            convert = convert_ARGB_swap_32;
        }
#else /* LITTLE_ENDIAN */
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[3] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[2] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[1] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[0] */
            convert = convert_ARGB_swap_32;
        }else{
            convert = convert_ARGB_copy_32;
        }
#endif
        break;

    case BGRA:
        /* A : pix[3] */
        /* R : pix[2] */
        /* G : pix[1] */
        /* B : pix[0] */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[0] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[1] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[2] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[3] */
            convert = convert_ARGB_swap_32;
        }else{
            convert = convert_ARGB_copy_32;
        }
#else /* LITTLE_ENDIAN */
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[3] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[2] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[1] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[0] */
            convert = convert_ARGB_copy_32;
        }else{
            convert = convert_ARGB_swap_32;
        }
#endif
        break;

    case YVYU: /* Packed YUV 4:2:2 */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[0] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[1] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[2] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[3] */
            convert = convert_YVYU_ARGB;
        }else{
            convert = convert_YVYU_BGRA;
        }
#else /* LITTLE_ENDIAN */
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[3] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[2] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[1] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[0] */
            convert = convert_YVYU_BGRA;
        }else{
            convert = convert_YVYU_ARGB;
        }
#endif
        break;

    case YV16: /* Planar YUV 4:2:2 */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[0] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[1] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[2] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[3] */
            convert = convert_YV16_ARGB;
        }else{
            convert = convert_YV16_BGRA;
        }
#else /* LITTLE_ENDIAN */
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[3] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[2] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[1] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[0] */
            DPRINTF("LE -- YV16-BGRA\n");
            convert = convert_YV16_BGRA;
        }else{
            DPRINTF("LE -- YV16-ARGB\n");
            convert = convert_YV16_ARGB;
        }
#endif
        break;

    case YV12: /* Planar YUV 4:2:0 */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[0] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[1] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[2] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[3] */
            convert = convert_YV12_ARGB;
        }else{
            convert = convert_YV12_BGRA;
        }
#else /* LITTLE_ENDIAN */
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[3] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[2] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[1] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[0] */
            convert = convert_YV12_BGRA;
        }else{
            convert = convert_YV12_ARGB;
        }
#endif
        break;

    case IYUV: /* Planar YUV 4:2:0 */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[0] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[1] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[2] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[3] */
            convert = convert_IYUV_ARGB;
        }else{
            convert = convert_IYUV_BGRA;
        }
#else /* LITTLE_ENDIAN */
        if( (pix_fmt->Amask == 0x00000000) && /* A : pix[3] */
            (pix_fmt->Rmask == 0x00FF0000) && /* R : pix[2] */
            (pix_fmt->Gmask == 0x0000FF00) && /* G : pix[1] */
            (pix_fmt->Bmask == 0x000000FF) ){ /* B : pix[0] */
            convert = convert_IYUV_BGRA;
        }else{
            convert = convert_IYUV_ARGB;
        }
#endif
        break;

    default:
        DPRINTF("Unknown mode\n");
    }

}

int
main (int argc, char *argv[])
{
    unsigned int			key[2];
    char					xname[80] = "*** The screen ***";
    unsigned int			i;
    char test;

	const SDL_VideoInfo	*info = NULL;

    if(argc != 1){
        EPRINTF("usage: %s\n", argv[0]);
    }

    DPRINTF("Launched\n");
    atexit(clean_shm);

    DPRINTF("Reading config\n");

    if( (read(0, key,     sizeof(key)     ) != sizeof(key)     ) ||
        (read(0, &width,  sizeof(uint32_t)) != sizeof(uint32_t)) ||
        (read(0, &height, sizeof(uint32_t)) != sizeof(uint32_t)) ||
        (read(0, &mode,   sizeof(uint32_t)) != sizeof(uint32_t)) ||
        (read(0, xname,   sizeof(xname)   ) != sizeof(xname)   ) ){
        EPRINTF("Error during configuration reading\n");
        goto error;
    }

    size = get_size(width, height, mode);

    DPRINTF("%dx%d\n", width, height);
    DPRINTF("size: %ld\n", size*sizeof(uint8_t));

    for( i = 0 ; i < 2 ; i++ ) {
        DPRINTF("getting shm %x\n", key[i]);
        if( (shmid[i] = shmget(key[i], sizeof(uint8_t)*size, 0400)) == -1 ){
            perror("viewerfb: shmget error:");
            exit(1);
        }
        if( (image[i] = (unsigned char*)shmat(shmid[i], 0, 00400)) == (void *)-1 ){
            perror("viewerfb: shmat error:");
            exit(1);
        }
    }
    buf_idx = 0;

    copy = (unsigned char *) malloc(sizeof(uint8_t)*size);
    memset(copy, 0x00, sizeof(char)*size);

    /* 
     * SDL initilization
     */
    if(SDL_Init(SDL_INIT_VIDEO) < 0){
		EPRINTF("Init failed: %s\n", SDL_GetError());
        goto SDL_error;
	}
	atexit(SDL_Quit);

	info = SDL_GetVideoInfo();
	if(!info){
		EPRINTF("Video query failed: %s\n", SDL_GetError());
        goto SDL_error;
	}
    pix_fmt = info->vfmt;

    set_conversion(mode);

    printf("got :\n"
           "   Amask: 0x%08x\n"
           "   Rmask: 0x%08x\n"
           "   Gmask: 0x%08x\n"
           "   Bmask: 0x%08x\n",
           info->vfmt->Amask,
           info->vfmt->Rmask,
           info->vfmt->Gmask,
           info->vfmt->Bmask);

    surface = SDL_SetVideoMode (width, height, pix_fmt->BitsPerPixel, SDL_SWSURFACE);
    if(!surface){
		EPRINTF("Video mode set failed: %s\n", SDL_GetError());
        goto SDL_error;
	}

#if 0
    if(need_an_overlay(mode)){
        overlay = SDL_CreateYUVOverlay (width, height,
                                        yuv_format(mode),
                                        surface);
        if(!overlay){
            EPRINTF("Overlay creation failed: %s\n", SDL_GetError());
            goto SDL_error;
        }
    }
#endif

    DPRINTF("Registering SIG ...\n");

    signal(SIGUSR1, refresh);

    read(0, &test, 1);

    DPRINTF("Reached end ...\n");

    while(1){}

    return EXIT_SUCCESS;

SDL_error:
    SDL_Quit();
error:
    exit(EXIT_FAILURE);
}

void
refresh(int signum __attribute__((__unused__)))
{
    DPRINTF("Refresh\n");
    memcpy(copy, image[buf_idx], size*sizeof(char));
    buf_idx = (buf_idx + 1) % 2;

    convert_and_display();
    DPRINTF("Refresh -- END\n");
    return;
}

void
convert_and_display(void){

    SDL_LockSurface(surface);
    
    convert(copy, surface->pixels, width, height);

    SDL_UnlockSurface(surface);

    SDL_UpdateRect(surface, 0, 0, 0, 0);

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
