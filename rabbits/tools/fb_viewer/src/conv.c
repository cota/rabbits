#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef DEBUG_CONV
#define DPRINTF(fmt, args...)                                           \
    do { fprintf(stdout, "fbviewer:conv: " fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#endif

#define EPRINTF(fmt, args...)                                           \
    do { fprintf(stderr, "fbviewer:conv: " fmt , ##args); } while (0)


void
yuv2rgb(uint8_t  y, uint8_t  u, uint8_t  v,
		uint8_t *r, uint8_t *g, uint8_t *b){

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
    
    *r = (uint8_t)r_val;
    *g = (uint8_t)g_val;
    *b = (uint8_t)b_val;
        
    return;
}

void
convert_RGB_ARGB(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0;
	uint8_t *p_src = src, *p_dst = dst;

    for(i = 0; i < w*h; i++, p_src += 3, p_dst += 4){
        p_dst[0] = 0;
        p_dst[1] = p_src[0];
        p_dst[2] = p_src[1];
        p_dst[3] = p_src[2];
    }
    return;
}

void
convert_RGB_BGRA(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0;
	uint8_t *p_src = src, *p_dst = dst;

    for(i = 0; i < w*h; i++, p_src += 3, p_dst += 4){
        p_dst[0] = p_src[2];
        p_dst[1] = p_src[1];
        p_dst[2] = p_src[0];
        p_dst[3] = 0;
    }
    return;
}

void
convert_BGR_ARGB(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0;
	uint8_t *p_src = src, *p_dst = dst;

    for(i = 0; i < w*h; i++, p_src += 3, p_dst += 4){
        p_dst[0] = 0;
        p_dst[1] = p_src[2];
        p_dst[2] = p_src[1];
        p_dst[3] = p_src[0];
    }
    return;
}

void
convert_BGR_BGRA(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0;
	uint8_t *p_src = src, *p_dst = dst;

    for(i = 0; i < w*h; i++, p_src += 3, p_dst += 4){
        p_dst[0] = p_src[0];
        p_dst[1] = p_src[1];
        p_dst[2] = p_src[2];
        p_dst[3] = 0;
    }
    return;
}

void
convert_ARGB_copy_32(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    memcpy(dst, src, w*h*4);
    return;
}

void
convert_ARGB_swap_32(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0;
	uint8_t *p_src = src, *p_dst = dst;

    for(i = 0; i < w*h; i++, p_src += 4, p_dst += 4){
        p_dst[0] = p_src[3];
        p_dst[1] = p_src[2];
        p_dst[2] = p_src[1];
        p_dst[3] = p_src[0];
    }

    return;
}

void
convert_YVYU_ARGB(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0;
    uint8_t  y = 0, u = 0, v = 0;
    uint8_t  r = 0, g = 0, b = 0;
    uint8_t *p_src = src;
    uint8_t *rgb   = dst;

	DPRINTF("YVYU-2-ARGB\n");

    /*
     * --------------------- ---------------------
     * | Y0 | V0 | Y1 | U0 | | Y2 | V2 | Y3 | U2 |
     * --------------------- ---------------------
     */
    for(i = 0; i < (w*h/2); i++){
        
        y = p_src[i*4 + 0];
        v = p_src[i*4 + 1];
        u = p_src[i*4 + 3];
        
        yuv2rgb(y, u, v, &r, &g, &b);
		rgb[0] = 0;
		rgb[1] = r;
		rgb[2] = g;
		rgb[3] = b;
        rgb += 4;

        y = p_src[i*4 + 2];

        yuv2rgb(y, u, v, &r, &g, &b);
		rgb[0] = 0;
		rgb[1] = r;
		rgb[2] = g;
		rgb[3] = b;
        rgb += 4;
    }
	return;
}

void
convert_YVYU_BGRA(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0;
    uint8_t  y = 0, u = 0, v = 0;
    uint8_t  r = 0, g = 0, b = 0;
    uint8_t *p_src = src;
    uint8_t *rgb   = dst;

	DPRINTF("YVYU-2-BGRA\n");

    /*
     * --------------------- ---------------------
     * | Y0 | V0 | Y1 | U0 | | Y2 | V2 | Y3 | U2 |
     * --------------------- ---------------------
     */
    for(i = 0; i < (w*h/2); i++){
        
        y = p_src[i*4 + 0];
        v = p_src[i*4 + 1];
        u = p_src[i*4 + 3];
        
        yuv2rgb(y, u, v, &r, &g, &b);
		rgb[0] = b;
		rgb[1] = g;
		rgb[2] = r;
		rgb[3] = 0;
        rgb += 4;

        y = p_src[i*4 + 2];

        yuv2rgb(y, u, v, &r, &g, &b);
		rgb[0] = b;
		rgb[1] = g;
		rgb[2] = r;
		rgb[3] = 0;
        rgb += 4;
    }
	return;

}

void
convert_YV12_ARGB(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0, j = 0;
    uint8_t  y = 0, u = 0, v = 0;
    uint8_t  r = 0, g = 0, b = 0;
    uint8_t *y_buf = src;
    uint8_t *v_buf = y_buf + (w*h);
    uint8_t *u_buf = v_buf + (w*h)/4;
	 
    uint8_t *rgb   = dst;
	 
    DPRINTF("YV12-2-ARGB\n");
	 
    for(j = 0; j < h; j++){
        for(i = 0; i < w/2; i++){
			   
            y = y_buf[i*2 + j*w];
            u = u_buf[i + j/2*w/2];
            v = v_buf[i + j/2*w/2];
			   
            yuv2rgb(y, u, v, &r, &g, &b);
            rgb[0] = 0;
            rgb[1] = r;
            rgb[2] = g;
            rgb[3] = b;
            rgb += 4;
			   
            y = y_buf[i*2 + j*w + 1];
			   
            yuv2rgb(y, u, v, &r, &g, &b);
            rgb[0] = 0;
            rgb[1] = r;
            rgb[2] = g;
            rgb[3] = b;
            rgb += 4;
        }
    }
    return;
}

void
convert_YV12_BGRA(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0, j = 0;
    uint8_t  y = 0, u = 0, v = 0;
    uint8_t  r = 0, g = 0, b = 0;
    uint8_t *y_buf = src;
    uint8_t *v_buf = y_buf + (w*h);
    uint8_t *u_buf = v_buf + (w*h)/4;
	 
    uint8_t *rgb   = dst;
	 
    DPRINTF("YV12-2-BGRA\n");
	 
    for(j = 0; j < h; j++){
        for(i = 0; i < w/2; i++){
			   
            y = y_buf[i*2 + j*w];
            u = u_buf[i + j/2*w/2];
            v = v_buf[i + j/2*w/2];
			   
            yuv2rgb(y, u, v, &r, &g, &b);
            rgb[0] = b;
            rgb[1] = g;
            rgb[2] = r;
            rgb[3] = 0;
            rgb += 4;
			   
            y = y_buf[i*2 + j*w + 1];
			   
            yuv2rgb(y, u, v, &r, &g, &b);
            rgb[0] = b;
            rgb[1] = g;
            rgb[2] = r;
            rgb[3] = 0;
            rgb += 4;
        }
    }
    return;
}

void
convert_YV16_ARGB(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0;
    uint8_t  y = 0, u = 0, v = 0;
    uint8_t  r = 0, g = 0, b = 0;
    uint8_t *y_buf = src;
    uint8_t *u_buf = y_buf + (w*h);
    uint8_t *v_buf = u_buf + (w*h)/2;

    uint8_t *rgb   = dst;

	DPRINTF("YV16-2-ARGB\n");

    for(i = 0; i < (w*h/2); i++){
        
        y = y_buf[i*2];
        u = u_buf[i];
        v = v_buf[i];
        
        yuv2rgb(y, u, v, &r, &g, &b);
		rgb[0] = 0;
		rgb[1] = r;
		rgb[2] = g;
		rgb[3] = b;
        rgb += 4;

        y = y_buf[i*2 + 1];

        yuv2rgb(y, u, v, &r, &g, &b);
		rgb[0] = 0;
		rgb[1] = r;
		rgb[2] = g;
		rgb[3] = b;
        rgb += 4;
    }
	return;
}

void
convert_YV16_BGRA(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0;
    uint8_t  y = 0, u = 0, v = 0;
    uint8_t  r = 0, g = 0, b = 0;
    uint8_t *y_buf = src;
    uint8_t *u_buf = y_buf + (w*h);
    uint8_t *v_buf = u_buf + (w*h)/2;

    uint8_t *rgb   = dst;

	DPRINTF("YV16-2-BGRA\n");

    for(i = 0; i < (w*h/2); i++){
        
        y = y_buf[i*2];
        u = u_buf[i];
        v = v_buf[i];
        
        yuv2rgb(y, u, v, &r, &g, &b);
		rgb[0] = b;
		rgb[1] = g;
		rgb[2] = r;
		rgb[3] = 0;
        rgb += 4;

        y = y_buf[i*2 + 1];

        yuv2rgb(y, u, v, &r, &g, &b);
		rgb[0] = b;
		rgb[1] = g;
		rgb[2] = r;
		rgb[3] = 0;
        rgb += 4;
    }

	return;
}

void
convert_IYUV_ARGB(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0, j = 0;
    uint8_t  y = 0, u = 0, v = 0;
    uint8_t  r = 0, g = 0, b = 0;
    uint8_t *y_buf = src;
    uint8_t *u_buf = y_buf + (w*h);
    uint8_t *v_buf = u_buf + (w*h)/4;
	 
    uint8_t *rgb   = dst;
	 
    DPRINTF("YV12-2-ARGB\n");
	 
    for(j = 0; j < h; j++){
        for(i = 0; i < w/2; i++){
			   
            y = y_buf[i*2 + j*w];
            u = u_buf[i + j/2*w/2];
            v = v_buf[i + j/2*w/2];
			   
            yuv2rgb(y, u, v, &r, &g, &b);
            rgb[0] = 0;
            rgb[1] = r;
            rgb[2] = g;
            rgb[3] = b;
            rgb += 4;
			   
            y = y_buf[i*2 + j*w + 1];
			   
            yuv2rgb(y, u, v, &r, &g, &b);
            rgb[0] = 0;
            rgb[1] = r;
            rgb[2] = g;
            rgb[3] = b;
            rgb += 4;
        }
    }
    return;
}

void
convert_IYUV_BGRA(uint8_t *src, uint8_t *dst, uint32_t w, uint32_t h){

    uint32_t i = 0, j = 0;
    uint8_t  y = 0, u = 0, v = 0;
    uint8_t  r = 0, g = 0, b = 0;
    uint8_t *y_buf = src;
    uint8_t *u_buf = y_buf + (w*h);
    uint8_t *v_buf = u_buf + (w*h)/4;
	 
    uint8_t *rgb   = dst;
	 
    DPRINTF("IYUV-2-BGRA\n");
	 
    for(j = 0; j < h; j++){
        for(i = 0; i < w/2; i++){
			   
            y = y_buf[i*2 + j*w];
            u = u_buf[i + j/2*w/2];
            v = v_buf[i + j/2*w/2];
			   
            yuv2rgb(y, u, v, &r, &g, &b);
            rgb[0] = b;
            rgb[1] = g;
            rgb[2] = r;
            rgb[3] = 0;
            rgb += 4;
			   
            y = y_buf[i*2 + j*w + 1];
			   
            yuv2rgb(y, u, v, &r, &g, &b);
            rgb[0] = b;
            rgb[1] = g;
            rgb[2] = r;
            rgb[3] = 0;
            rgb += 4;
        }
    }
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
