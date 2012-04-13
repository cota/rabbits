#include <stdio.h>

#include "global.h"
#include "../tb/init.h"


/*
 * Chen IDCT defines
 */
#define LS(r,s) ((r) << (s))
#define RS(r,s) ((r) >> (s))	/* Caution with rounding... */

#define MSCALE(expr)  RS((expr),9)

/* Cos constants */

#define c1d4 362L

#define c1d8 473L
#define c3d8 196L

#define c1d16 502L
#define c3d16 426L
#define c5d16 284L
#define c7d16 100L



/*
 * Initialize Quantization Table
 */
const int izigzag_index[64] = { 0, 1, 8, 16, 9, 2, 3, 10,
  17, 24, 32, 25, 18, 11, 4, 5,
  12, 19, 26, 33, 40, 48, 41, 34,
  27, 20, 13, 6, 7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36,
  29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46,
  53, 60, 61, 54, 47, 55, 62, 63
};

const int zigzag_index[64] =	/* Is zig-zag map for matrix -> scan array */
{ 0, 1, 5, 6, 14, 15, 27, 28,
  2, 4, 7, 13, 16, 26, 29, 42,
  3, 8, 12, 17, 25, 30, 41, 43,
  9, 11, 18, 24, 31, 40, 44, 53,
  10, 19, 23, 32, 39, 45, 52, 54,
  20, 22, 33, 38, 46, 51, 55, 60,
  21, 34, 37, 47, 50, 56, 59, 61,
  35, 36, 48, 49, 57, 58, 62, 63
};

/* Used for sign extensions. */
const static int extend_mask[20] = {
  0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFF8, 0xFFFFFFF0, 0xFFFFFFE0, 0xFFFFFFC0,
  0xFFFFFF80, 0xFFFFFF00, 0xFFFFFE00, 0xFFFFFC00, 0xFFFFF800, 0xFFFFF000,
  0xFFFFE000, 0xFFFFC000, 0xFFFF8000, 0xFFFF0000, 0xFFFE0000, 0xFFFC0000,
  0xFFF80000, 0xFFF00000
};


/* Masks */
const int bit_set_mask[32] = {	/* This is 2^i at ith position */
  0x00000001, 0x00000002, 0x00000004, 0x00000008,
  0x00000010, 0x00000020, 0x00000040, 0x00000080,
  0x00000100, 0x00000200, 0x00000400, 0x00000800,
  0x00001000, 0x00002000, 0x00004000, 0x00008000,
  0x00010000, 0x00020000, 0x00040000, 0x00080000,
  0x00100000, 0x00200000, 0x00400000, 0x00800000,
  0x01000000, 0x02000000, 0x04000000, 0x08000000,
  0x10000000, 0x20000000, 0x40000000, 0x80000000
};

const int lmask[32] = {		/* This is 2^{i+1}-1 */
  0x00000001, 0x00000003, 0x00000007, 0x0000000f,
  0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
  0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
  0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
  0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
  0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
  0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
  0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
};


/*
 * Buffer for reading JPEG file
 */
unsigned char JpegFileBuf[JPEG_FILE_SIZE];

/*
 *  Read Buffer
 */
static unsigned char *ReadBuf;


/*
* Working Pointers for decode
*/
char p_jinfo_data_precision;
short p_jinfo_image_height;
short p_jinfo_image_width;
char p_jinfo_num_components;
int p_jinfo_smp_fact;

char p_jinfo_comps_info_index[NUM_COMPONENT];
char p_jinfo_comps_info_id[NUM_COMPONENT];
char p_jinfo_comps_info_h_samp_factor[NUM_COMPONENT];
char p_jinfo_comps_info_v_samp_factor[NUM_COMPONENT];
char p_jinfo_comps_info_quant_tbl_no[NUM_COMPONENT];
char p_jinfo_comps_info_dc_tbl_no[NUM_COMPONENT];
char p_jinfo_comps_info_ac_tbl_no[NUM_COMPONENT];

unsigned int p_jinfo_quant_tbl_quantval[NUM_QUANT_TBLS][DCTSIZE2];

int p_jinfo_dc_xhuff_tbl_bits[NUM_HUFF_TBLS][36];
int p_jinfo_dc_xhuff_tbl_huffval[NUM_HUFF_TBLS][257];

int p_jinfo_ac_xhuff_tbl_bits[NUM_HUFF_TBLS][36];
int p_jinfo_ac_xhuff_tbl_huffval[NUM_HUFF_TBLS][257];

int p_jinfo_dc_dhuff_tbl_ml[NUM_HUFF_TBLS];
int p_jinfo_dc_dhuff_tbl_maxcode[NUM_HUFF_TBLS][36];
int p_jinfo_dc_dhuff_tbl_mincode[NUM_HUFF_TBLS][36];
int p_jinfo_dc_dhuff_tbl_valptr[NUM_HUFF_TBLS][36];

int p_jinfo_ac_dhuff_tbl_ml[NUM_HUFF_TBLS];
int p_jinfo_ac_dhuff_tbl_maxcode[NUM_HUFF_TBLS][36];
int p_jinfo_ac_dhuff_tbl_mincode[NUM_HUFF_TBLS][36];
int p_jinfo_ac_dhuff_tbl_valptr[NUM_HUFF_TBLS][36];

int p_jinfo_MCUWidth;
int p_jinfo_MCUHeight;
int p_jinfo_NumMCU;

unsigned char *p_jinfo_jpeg_data;

int rgb_buf[4][RGB_NUM][DCTSIZE2];


/*
* Working Pointers for Huffman
*/
unsigned char *CurHuffReadBuf;
static unsigned int current_read_byte;
static int read_position = -1;

/*
* Output buffer
**TODO after deadline** Move to testbench and fill 1 block per transaction
*/
int out_data_comp_vpos[RGB_NUM];
int out_data_comp_hpos[RGB_NUM];
unsigned char OutData_comp_buf[RGB_NUM][IMAGE_SIZE];


/*******************************************
CHEN IDCT
********************************************/
/*
 *
 * ChenIDCT() implements the Chen inverse dct. Note that there are two
 * input vectors that represent x=input, and y=output, and must be
 * defined (and storage allocated) before this routine is called.
 */
void
ChenIDct (int *x, int *y)
{
  register int i;
  register int a0, a1, a2, a3;
  register int b0, b1, b2, b3;
  register int c0, c1, c2, c3;

  /* Loop over columns */
  CHENIDCT_COL:
  for (i = 0; i < 8; i++)
    {
      b0 = LS (x[i], 2);
      a0 = LS (x[i+DCTSIZE*1], 2);
      b2 = LS (x[i+DCTSIZE*2], 2);
      a1 = LS (x[i+DCTSIZE*3], 2);
      b1 = LS (x[i+DCTSIZE*4], 2);
      a2 = LS (x[i+DCTSIZE*5], 2);
      b3 = LS (x[i+DCTSIZE*6], 2);
      a3 = LS (x[i+DCTSIZE*7], 2);

      /* Split into even mode  b0 = x0  b1 = x4  b2 = x2  b3 = x6.
         And the odd terms a0 = x1 a1 = x3 a2 = x5 a3 = x7.
       */

      c0 = MSCALE ((c7d16 * a0) - (c1d16 * a3));
      c1 = MSCALE ((c3d16 * a2) - (c5d16 * a1));
      c2 = MSCALE ((c3d16 * a1) + (c5d16 * a2));
      c3 = MSCALE ((c1d16 * a0) + (c7d16 * a3));

      /* First Butterfly on even terms. */

      a0 = MSCALE (c1d4 * (b0 + b1));
      a1 = MSCALE (c1d4 * (b0 - b1));

      a2 = MSCALE ((c3d8 * b2) - (c1d8 * b3));
      a3 = MSCALE ((c1d8 * b2) + (c3d8 * b3));

      b0 = a0 + a3;
      b1 = a1 + a2;
      b2 = a1 - a2;
      b3 = a0 - a3;

      /* Second Butterfly */

      a0 = c0 + c1;
      a1 = c0 - c1;
      a2 = c3 - c2;
      a3 = c3 + c2;

      c0 = a0;
      c1 = MSCALE (c1d4 * (a2 - a1));
      c2 = MSCALE (c1d4 * (a2 + a1));
      c3 = a3;

      y[i] = b0 + c3;
      y[i+DCTSIZE*1] = b1 + c2;
      y[i+DCTSIZE*2] = b2 + c1;
      y[i+DCTSIZE*3] = b3 + c0;
      y[i+DCTSIZE*4] = b3 - c0;
      y[i+DCTSIZE*5] = b2 - c1;
      y[i+DCTSIZE*6] = b1 - c2;
      y[i+DCTSIZE*7] = b0 - c3;
    }

  /* Loop over rows */
  CHENIDCT_ROW:
  for (i = 0; i < 8; i++)
    {
      int j = i<<3;
      b0 = y[j];
      a0 = y[j+1];
      b2 = y[j+2];
      a1 = y[j+3];
      b1 = y[j+4];
      a2 = y[j+5];
      b3 = y[j+6];
      a3 = y[j+7];

      /*
         Split into even mode  b0 = x0  b1 = x4  b2 = x2  b3 = x6.
         And the odd terms a0 = x1 a1 = x3 a2 = x5 a3 = x7.
       */

      c0 = MSCALE ((c7d16 * a0) - (c1d16 * a3));
      c1 = MSCALE ((c3d16 * a2) - (c5d16 * a1));
      c2 = MSCALE ((c3d16 * a1) + (c5d16 * a2));
      c3 = MSCALE ((c1d16 * a0) + (c7d16 * a3));

      /* First Butterfly on even terms. */

      a0 = MSCALE (c1d4 * (b0 + b1));
      a1 = MSCALE (c1d4 * (b0 - b1));

      a2 = MSCALE ((c3d8 * b2) - (c1d8 * b3));
      a3 = MSCALE ((c1d8 * b2) + (c3d8 * b3));

      /* Calculate last set of b's */

      b0 = a0 + a3;
      b1 = a1 + a2;
      b2 = a1 - a2;
      b3 = a0 - a3;

      /* Second Butterfly */

      a0 = c0 + c1;
      a1 = c0 - c1;
      a2 = c3 - c2;
      a3 = c3 + c2;

      c0 = a0;
      c1 = MSCALE (c1d4 * (a2 - a1));
      c2 = MSCALE (c1d4 * (a2 + a1));
      c3 = a3;

      y[j] = b0 + c3;
      y[j+1] = b1 + c2;
      y[j+2] = b2 + c1;
      y[j+3] = b3 + c0;
      y[j+4] = b3 - c0;
      y[j+5] = b2 - c1;
      y[j+6] = b1 - c2;
      y[j+7] = b0 - c3;
    }

  /*
     Retrieve correct accuracy. We have additional factor
     of 16 that must be removed.
   */
  CHENIDC_FACT16:
  for (i = 0; i < 64; i++)
    y[i] = (((y[i] < 0) ? (y[i] - 8) : (y[i] + 8)) / 16);
}




/*******************************************
HUFFMAN
********************************************/
/*
 * pgetc() gets a character onto the stream but it checks to see
 * if there are any marker conflicts.
 */
static int
pgetc ()
{
  int temp;

  if ((temp = *CurHuffReadBuf++) == MARKER_MARKER)
    {				/* If MARKER then */
      if ((temp = *CurHuffReadBuf++))
	{			/* if next is not 0xff, then marker */
	  printf ("Unanticipated marker detected.\n");
	}
      else
	{
	  return (MARKER_MARKER);
	}
    }
  return (temp);
}


/*
 * buf_getb() gets a bit from the read stream.
 */
int
buf_getb ()
{
  if (read_position < 0)
    {
      current_read_byte = pgetc ();
      read_position = 7;
    }

  if (current_read_byte & bit_set_mask[read_position--])
    {
      return (1);
    }
  return (0);
}


/*
 * megetv() gets n bits from the read stream and returns it.
 *
 */
int
buf_getv (int n)
{
  int p, rv;

  n--;
  p = n - read_position;
  while (p > 0)
    {
      if (read_position > 23)
	{			/* If byte buffer contains almost entire word */
	  rv = (current_read_byte << p);	/* Manipulate buffer */
	  current_read_byte = pgetc ();	/* Change read bytes */

	  rv |= (current_read_byte >> (8 - p));
	  read_position = 7 - p;
	  return (rv & lmask[n]);
	  /* Can return pending residual val */
	}
      current_read_byte = (current_read_byte << 8) | pgetc ();
      read_position += 8;	/* else shift in new information */
      p -= 8;
    }

  if (!p)
    {				/* If position is zero */
      read_position = -1;
      /* Can return current byte */
      return (current_read_byte & lmask[n]);
    }

  p = -p;
  /* Else reverse position and shift */
  read_position = p - 1;
  return ((current_read_byte >> p) & lmask[n]);
}






/*
 *  
 */
int
DecodeHuffman (int *Xhuff_huffval, int Dhuff_ml,
	       int *Dhuff_maxcode, int *Dhuff_mincode, int *Dhuff_valptr)
{
  int code, l, p;

  code = buf_getb ();
  for (l = 1; code > Dhuff_maxcode[l]; l++)
    {
      code = (code << 1) + buf_getb ();
    }

      p = Dhuff_valptr[l] + code - Dhuff_mincode[l];
      return (Xhuff_huffval[p]);
}


/*
 * Decode one MCU
 */
void
DecodeHuffMCU (int *out_buf, int num_cmp)
{
  int s, diff, tbl_no, *mptr, k, n, r;

  /*
   * Decode DC
   */
  tbl_no = p_jinfo_comps_info_dc_tbl_no[num_cmp];
  s = DecodeHuffman (&p_jinfo_dc_xhuff_tbl_huffval[tbl_no][0],
		     p_jinfo_dc_dhuff_tbl_ml[tbl_no],
		     &p_jinfo_dc_dhuff_tbl_maxcode[tbl_no][0],
		     &p_jinfo_dc_dhuff_tbl_mincode[tbl_no][0],
		     &p_jinfo_dc_dhuff_tbl_valptr[tbl_no][0]);

  if (s)
    {
      diff = buf_getv (s);
      s--;
      if ((diff & bit_set_mask[s]) == 0)
	{
	  diff |= extend_mask[s];
	  diff++;
	}

      diff += *out_buf;		/* Change the last DC */
      *out_buf = diff;
    }

  /*
   * Decode AC
   */
  /* Set all values to zero */
  for (mptr = out_buf + 1; mptr < out_buf + DCTSIZE2; mptr++)
    {
      *mptr = 0;
    }

  for (k = 1; k < DCTSIZE2;)
    {				/* JPEG Mistake */
      r = DecodeHuffman (&p_jinfo_ac_xhuff_tbl_huffval[tbl_no][0],
			 p_jinfo_ac_dhuff_tbl_ml[tbl_no],
			 &p_jinfo_ac_dhuff_tbl_maxcode[tbl_no][0],
			 &p_jinfo_ac_dhuff_tbl_mincode[tbl_no][0],
			 &p_jinfo_ac_dhuff_tbl_valptr[tbl_no][0]);

      s = r & 0xf;		/* Find significant bits */
      n = (r >> 4) & 0xf;	/* n = run-length */

      if (s)
	{
	  if ((k += n) >= DCTSIZE2)	/* JPEG Mistake */
	    break;
	  out_buf[k] = buf_getv (s);	/* Get s bits */
	  s--;			/* Align s */
	  if ((out_buf[k] & bit_set_mask[s]) == 0)
	    {			/* Also (1 << s) */
	      out_buf[k] |= extend_mask[s];	/* Also  (-1 << s) + 1 */
	      out_buf[k]++;	/* Increment 2's c */
	    }
	  k++;			/* Goto next element */
	}
      else if (n == 15)		/* Zero run length code extnd */
	k += 16;
      else
	{
	  break;
	}
    }
}



/*******************************************
DECODE MAIN FUNCTIONS - WRITE BLOCKS
********************************************/
/*
 * IZigzagMatrix() performs an inverse zig-zag translation on the
 * input imatrix and places the output in omatrix.
 */
void
IZigzagMatrix (int *imatrix, int *omatrix)
{
  int i;

  for (i = 0; i < DCTSIZE2; i++)
    
    {
      
*(omatrix++) = imatrix[zigzag_index[i]];
    
}
}


/*
 * IQuantize() takes an input matrix and does an inverse quantization
 * and puts the output int qmatrix.
 */
void
IQuantize (int *matrix, int *qmatrix)
{
  int *mptr;

  for (mptr = matrix; mptr < matrix + DCTSIZE2; mptr++)
    {
      *mptr = *mptr * (*qmatrix);
      qmatrix++;
    }
}


/*
 * PostshiftIDctMatrix() adds 128 (2048) to all 64 elements of an 8x8 matrix.
 * This results in strictly positive values for all pixel coefficients.
 */
void
PostshiftIDctMatrix (int *matrix, int shift)
{
  int *mptr;
  for (mptr = matrix; mptr < matrix + DCTSIZE2; mptr++)
    {
      *mptr += shift;
    }
}


/*
 * BoundIDctMatrix bounds the inverse dct matrix so that no pixel has a
 * value greater than 255 (4095) or less than 0.
 */
void
BoundIDctMatrix (int *matrix, int Bound)
{
  int *mptr;

  for (mptr = matrix; mptr < matrix + DCTSIZE2; mptr++)
    {
      if (*mptr < 0)
	{
	  *mptr = 0;
	}
      else if (*mptr > Bound)
	{
	  *mptr = Bound;
	}
    }
}



void
WriteOneBlock (int *store, char *out_buf, int width, int height,
	       int voffs, int hoffs)
{
  int i, e;


  /* Find vertical buffer offs. */
  for (i = voffs; i < voffs + DCTSIZE; i++)
    {
      if (i < height)
	{
	  int diff;
	  diff = width * i;
	  for (e = hoffs; e < hoffs + DCTSIZE; e++)
	    {
	      if (e < width)
		{
		  out_buf[diff + e] = (unsigned char) (*(store++));
		}
	      else
		{
		  break;
		}
	    }
	}
      else
	{
	  break;
	}
    }


}

/*
 * Write4Block() writes an array of data in the integer array pointed to
 * by store out to the driver specified by the IOB.  The integer array is
 * stored in row-major form, that is, the first row of (8) elements, the
 * second row of (8) elements....
 * SMPLING FACTOR 4:1:1 ONLY
 */
void
Write4Blocks (int *store1, int *store2, int *store3, int *store4,
	      int *p_out_vpos, int *p_out_hpos, unsigned char *p_out_buf)
{
  int voffs, hoffs;

  /*
   * OX
   * XX
   */
  voffs = *p_out_vpos * DCTSIZE;
  hoffs = *p_out_hpos * DCTSIZE;
  WriteOneBlock (store1, p_out_buf,
		 p_jinfo_image_width, p_jinfo_image_height, voffs, hoffs);

  /*
   * XO
   * XX
   */
  hoffs += DCTSIZE;
  WriteOneBlock (store2, p_out_buf,
		 p_jinfo_image_width, p_jinfo_image_height, voffs, hoffs);

  /*
   * XX
   * OX
   */
  voffs += DCTSIZE;
  hoffs -= DCTSIZE;
  WriteOneBlock (store3, p_out_buf,
		 p_jinfo_image_width, p_jinfo_image_height, voffs, hoffs);


  /*
   * XX
   * XO
   */
  hoffs += DCTSIZE;
  WriteOneBlock (store4,
		 p_out_buf, p_jinfo_image_width, p_jinfo_image_height,
		 voffs, hoffs);

  /*
   * Add positions
   */
  *p_out_hpos = *p_out_hpos + 2;
  *p_out_vpos = *p_out_vpos + 2;


  if (*p_out_hpos < p_jinfo_MCUWidth)
    {
      *p_out_vpos = *p_out_vpos - 2;
    }
  else
    {
      *p_out_hpos = 0;		/* If at end of image (width) */
    }
}


/*
 * Transform from Yuv into RGB
 */
void
YuvToRgb (int p, int *y_buf, int *u_buf, int *v_buf)
{
  int r, g, b;
  int y, u, v;
  int i;

  for (i = 0; i < DCTSIZE2; i++)
    {
      y = y_buf[i];
      u = u_buf[i] - 128;
      v = v_buf[i] - 128;

      r = (y * 256 + v * 359 + 128) >> 8;
      g = (y * 256 - u * 88 - v * 182 + 128) >> 8;
      b = (y * 256 + u * 454 + 128) >> 8;

      if (r < 0)
	r = 0;
      else if (r > 255)
	r = 255;

      if (g < 0)
	g = 0;
      else if (g > 255)
	g = 255;

      if (b < 0)
	b = 0;
      else if (b > 255)
	b = 255;

      rgb_buf[p][0][i] = r;
      rgb_buf[p][1][i] = g;
      rgb_buf[p][2][i] = b;

    }
}


/*
 * Decode one block
 */
void
decode_block (int comp_no, int *out_buf, int *HuffBuff)
{
  int QuantBuff[DCTSIZE2];
  unsigned int *p_quant_tbl;

  DecodeHuffMCU (HuffBuff, comp_no);

  IZigzagMatrix (HuffBuff, QuantBuff);

  p_quant_tbl =
    &p_jinfo_quant_tbl_quantval[(int)p_jinfo_comps_info_quant_tbl_no[comp_no]][0];
  IQuantize (QuantBuff, p_quant_tbl);

  ChenIDct (QuantBuff, out_buf);

  PostshiftIDctMatrix (out_buf, IDCT_SHIFT);

  BoundIDctMatrix (out_buf, IDCT_BOUNT);

}


void
decode_start ()
{
  int i;
  int CurrentMCU = 0;
  int HuffBuff[NUM_COMPONENT][DCTSIZE2];
  int IDCTBuff[6][DCTSIZE2];

  /* Read buffer */
  CurHuffReadBuf = p_jinfo_jpeg_data;

  /*
   * Initial value of DC element is 0
   */
  for (i = 0; i < NUM_COMPONENT; i++)
    {
      HuffBuff[i][0] = 0;
    }

  /*
   * Initialize output buffer
   */
  for (i = 0; i < RGB_NUM; i++)
    {
      out_data_comp_vpos[i] = 0;
      out_data_comp_hpos[i] = 0;
    }

      printf ("Decode 4:1:1 NumMCU = %d\n", p_jinfo_NumMCU);
      /*
       * 4_1_1 ONLY
       */
      while (CurrentMCU < p_jinfo_NumMCU)
	{
	  /*
	   * Decode Y element
	   * Decoding Y, U and V elements should be sequentially conducted for the use of Huffman table
	   */

	  for (i = 0; i < 4; i++)
	    {
	      decode_block (0, IDCTBuff[i], HuffBuff[0]);
	    }

	  /* Decode U */
	  decode_block (1, IDCTBuff[4], HuffBuff[1]);

	  /* Decode V */
	  decode_block (2, IDCTBuff[5], HuffBuff[2]);


	  /* Transform from Yuv into RGB */

	  for (i = 0; i < 4; i++)
	    {
	      YuvToRgb (i, IDCTBuff[i], IDCTBuff[4], IDCTBuff[5]);
	    }


	  for (i = 0; i < RGB_NUM; i++)
	    {
	      Write4Blocks (&rgb_buf[0][i][0],
			    &rgb_buf[1][i][0],
			    &rgb_buf[2][i][0],
			    &rgb_buf[3][i][0],
			    &out_data_comp_vpos[i],
			    &out_data_comp_hpos[i], &OutData_comp_buf[i][0]);
	    }


	  CurrentMCU += 4;
    }
}




/*******************************************
PARSE JPEG MARKERS
********************************************/
/*
 *  Read from Buffer
 */
int
read_byte (void)
{
  return *ReadBuf++;
}


short
read_word (void)
{
  short c;

  c = *ReadBuf++ << 8;
  c |= *ReadBuf++;

  return c;
}


/*
 *  Baseline DCT ( Huffman )  
 */
void
get_sof ()
{
  int ci, c;
  int length;

  length = read_word ();
  p_jinfo_data_precision = read_byte ();
  p_jinfo_image_height = read_word ();
  p_jinfo_image_width = read_word ();
  p_jinfo_num_components = read_byte ();

  length -= 8;

  /* Omit error check */

  /* Check components */
  for (ci = 0; ci < p_jinfo_num_components; ci++)
    {
      p_jinfo_comps_info_index[ci] = ci;
      p_jinfo_comps_info_id[ci] = read_byte ();
      c = read_byte ();
      p_jinfo_comps_info_h_samp_factor[ci] = (c >> 4) & 15;
      p_jinfo_comps_info_v_samp_factor[ci] = (c) & 15;
      p_jinfo_comps_info_quant_tbl_no[ci] = read_byte ();

    }

      p_jinfo_smp_fact = SF4_1_1;
}


void
get_sos ()
{
  int length, num_comp;
  int i, c, cc, ci, j;

  length = read_word ();
  num_comp = read_byte ();

  /* Decode each component */
  for (i = 0; i < num_comp; i++)
    {
      cc = read_byte ();
      c = read_byte ();

      for (ci = 0; ci < p_jinfo_num_components; ci++)
	{
	  if (cc == p_jinfo_comps_info_id[ci])
	    break;
	}

    id_found:
      p_jinfo_comps_info_dc_tbl_no[ci] = (c >> 4) & 15;
      p_jinfo_comps_info_ac_tbl_no[ci] = (c) & 15;

    }

  /* Pass parameters; Ss, Se, Ah and Al for progressive JPEG */
  j = 3;
  while (j--)
    {
      c = read_byte ();
    }

  /*
   * Define the Buffer at this point as the head of data
   */
  p_jinfo_jpeg_data = ReadBuf;

}


/*
 * Get Huffman Table
 */
void
get_dht ()
{
  int length;
  int index, i, count;

  length = read_word ();
  length -= 2;

  while (length > 16)
    {
      index = read_byte ();
      count = 0;

      if (index & 0x10) 
	{
	  index -= 0x10;
	  for (i = 1; i <= 16; i++)
	    {
	       p_jinfo_ac_xhuff_tbl_bits[index][i] = read_byte ();
	       count += p_jinfo_ac_xhuff_tbl_bits[index][i];
	    }
          for (i = 0; i < count; i++)
	    {
	      p_jinfo_ac_xhuff_tbl_huffval[index][i] = read_byte ();
	    }
	}
      else 
	{
	  for (i = 1; i <= 16; i++)
	    {
	      p_jinfo_dc_xhuff_tbl_bits[index][i] = read_byte ();
	      count += p_jinfo_dc_xhuff_tbl_bits[index][i];
	    }
          for (i = 0; i < count; i++)
	    {
	      p_jinfo_dc_xhuff_tbl_huffval[index][i] = read_byte ();
	    }
	}

      length -= 1 + 16;
      length -= count;
    }
}


void
get_dqt ()
{
  int length;
  int prec, num, i;
  unsigned int tmp;

  length = read_word ();
  length -= 2;

  while (length > 0)
    {
      num = read_byte ();
      /* Precision 0:8bit, 1:16bit */
      prec = num >> 4;
      /* Table Number */
      num &= 0x0f;

      for (i = 0; i < DCTSIZE2; i++)
	{
	  if (prec)
	    tmp = read_word ();
	  else
	    tmp = read_byte ();
	  p_jinfo_quant_tbl_quantval[num][izigzag_index[i]] = (unsigned short) tmp;
	}

      length -= DCTSIZE2 + 1;
      if (prec)
	length -= DCTSIZE2;
    }
}



void
read_markers (unsigned char *buf)
{
  int sow_SOI;
  int sow_SOS;

  ReadBuf = buf;

  sow_SOI = 0; //Use bool in SystemC
  sow_SOS = 0; //Use bool in SystemC

  unsigned char c = 0;

  //Last marker containing infromation for decoding: M_SOS

  /* Parse the markers*/
  while(!sow_SOS)
    {
      if (!sow_SOI)
	{
	  c = read_byte ();
	  c = read_byte ();
	}
      else
	{
  	  do c = read_byte ();
	  while (c != 0xff);
	  do c = read_byte ();
	  while (c == 0xff);
	}


      switch (c)
	{
	case M_SOI:		/* Start of Image */
	  sow_SOI = 1;
	  break;

	case M_SOF0:		/* Baseline DCT ( Huffman ) */
	  get_sof ();
	  break;

	case M_SOS:		/* Start of Scan ( Head of Compressed Data ) */
	  get_sos ();
	  sow_SOS = 1;
	  break;

	case M_DHT:		/* Huffman table */
	  get_dht ();
	  break;

	case M_DQT:		/* Quantization table */
	  get_dqt ();
	  break;

	default:		/* Other markers are ignored */
	  break;
	}
    }
}




/*******************************************
READ JPEG and CALL MARKERS and DECODE
********************************************/
/*
 * Create Table for decoding (From Huffman.c)
 */
void
huff_make_dhuff_tb ()
{
 int t;
  //This for allows to build tables in parallel (if unrolled)
 for(t=0;t<4;t++) { 

  int *p_xhtbl_bits;
  int *p_dhtbl_ml;
  int *p_dhtbl_maxcode;
  int *p_dhtbl_mincode;
  int *p_dhtbl_valptr;

  switch(t) {
    case 0:
      p_xhtbl_bits = &p_jinfo_dc_xhuff_tbl_bits[0][0];
      p_dhtbl_ml = &p_jinfo_dc_dhuff_tbl_ml[0];
      p_dhtbl_maxcode = &p_jinfo_dc_dhuff_tbl_maxcode[0][0];
      p_dhtbl_mincode = &p_jinfo_dc_dhuff_tbl_mincode[0][0];
      p_dhtbl_valptr = &p_jinfo_dc_dhuff_tbl_valptr[0][0];
      break;
    case 1:
      p_xhtbl_bits = &p_jinfo_dc_xhuff_tbl_bits[1][0];
      p_dhtbl_ml = &p_jinfo_dc_dhuff_tbl_ml[1];
      p_dhtbl_maxcode = &p_jinfo_dc_dhuff_tbl_maxcode[1][0];
      p_dhtbl_mincode = &p_jinfo_dc_dhuff_tbl_mincode[1][0];
      p_dhtbl_valptr = &p_jinfo_dc_dhuff_tbl_valptr[1][0];
      break;
    case 2:
      p_xhtbl_bits = &p_jinfo_ac_xhuff_tbl_bits[0][0];
      p_dhtbl_ml = &p_jinfo_ac_dhuff_tbl_ml[0];
      p_dhtbl_maxcode = &p_jinfo_ac_dhuff_tbl_maxcode[0][0];
      p_dhtbl_mincode = &p_jinfo_ac_dhuff_tbl_mincode[0][0];
      p_dhtbl_valptr = &p_jinfo_ac_dhuff_tbl_valptr[0][0];
      break;
    case 3:
      p_xhtbl_bits = &p_jinfo_ac_xhuff_tbl_bits[1][0];
      p_dhtbl_ml = &p_jinfo_ac_dhuff_tbl_ml[1];
      p_dhtbl_maxcode = &p_jinfo_ac_dhuff_tbl_maxcode[1][0];
      p_dhtbl_mincode = &p_jinfo_ac_dhuff_tbl_mincode[1][0];
      p_dhtbl_valptr = &p_jinfo_ac_dhuff_tbl_valptr[1][0];
      break;
  }

  int i, j, p, code, size, l;
  int huffsize[257];
  int huffcode[257];
  int lastp;

  /*
   * Get size
   */
  for (p = 0, i = 1; i < 17; i++)
    {
      for (j = 1; j <= p_xhtbl_bits[i]; j++)
	{
	  huffsize[p++] = i;
	}
    }

  huffsize[p] = 0;
  lastp = p;

  p = 0;
  code = 0;
  size = huffsize[0];
  while (1)
    {
      do
	{
	  huffcode[p++] = code++;
	}
      while ((huffsize[p] == size) && (p < 257));
      /* Overflow Detection */
      if (!huffsize[p])
	{			/* All finished. */
	  break;
	}
      do
	{
	  /* Shift next code to expand prefix. */
	  code <<= 1;
	  size++;
	}
      while (huffsize[p] != size);
    }

  *p_dhtbl_ml = 1;
  for (p = 0, l = 1; l <= 16; l++)
    {
      if (p_xhtbl_bits[l] == 0)
	{
          
	  p_dhtbl_maxcode[l] = -1;	/* Watch out JPEG is wrong here */
	}
      else
	{			/* We use -1 to indicate skipping. */
	  p_dhtbl_valptr[l] = p;
	  p_dhtbl_mincode[l] = huffcode[p];
	  p += p_xhtbl_bits[l] - 1;
	  p_dhtbl_maxcode[l] = huffcode[p];
	  *p_dhtbl_ml = l;
	  p++;
	}
    }
  p_dhtbl_maxcode[*p_dhtbl_ml]++;
 }
}


/*
 * Initialize after reading markers
 */
void
jpeg_init_decompress ()
{
  int tmp;
  /*
   * Get MCU number
   */
  p_jinfo_MCUHeight = (p_jinfo_image_height - 1) / 8 + 1;
  p_jinfo_MCUWidth = (p_jinfo_image_width - 1) / 8 + 1;
  p_jinfo_NumMCU = p_jinfo_MCUHeight * p_jinfo_MCUWidth;

  /*
   * Create Huffman Table for decoding
   * 
   * TABLES COULD BE CREATED IN PARALLEL!!!
   */
  huff_make_dhuff_tb ();
}



void
jpeg_read (unsigned char *read_buf)
{

  /*
   * Read markers
   */
  read_markers (read_buf);


  /*
   * Initialize the information used for decoding
   */
  jpeg_init_decompress ();

  /*
   * Start decoding
   */
  decode_start ();

}



/*******************************************
JPEG MAIN
********************************************/

int
jpeg2bmp_main ()
{
  int ci;
  unsigned char *c;
  int i, j;

  /*
   * Store input data in buffer
   */
  c = JpegFileBuf;
  for (i = 0; i < JPEG_FILE_SIZE; i++)
    
    {
        ci = input_jpg[i];
        *c++ = ci;
    }

  jpeg_read (JpegFileBuf);


#if 0
FILE *jpgin;
jpgin = fopen ("c_jpgin.jpg","w+");
for (i = 0; i < JPEG_FILE_SIZE; i++){
    buf[0] = input_jpg[i];
    fwrite(buf,sizeof(char),1,jpgin);
}
fclose(jpgin);
#endif

return 0;

}



/*****************************************************
C MAIN
******************************************************/
int
main ()
{
  jpeg2bmp_main ();

  return 0;
}
