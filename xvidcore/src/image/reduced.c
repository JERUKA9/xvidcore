/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *   Reduced-Resolution utilities
 *
 *  Copyright(C) 2002 Pascal Massimino <skal@planet-d.net>
 *
 *  This file is part of XviD, a free MPEG-4 video encoder/decoder
 *
 *  XviD is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Under section 8 of the GNU General Public License, the copyright
 *  holders of XVID explicitly forbid distribution in the following
 *  countries:
 *
 *    - Japan
 *    - United States of America
 *
 *  Linking XviD statically or dynamically with other modules is making a
 *  combined work based on XviD.  Thus, the terms and conditions of the
 *  GNU General Public License cover the whole combination.
 *
 *  As a special exception, the copyright holders of XviD give you
 *  permission to link XviD with independent modules that communicate with
 *  XviD solely through the VFW1.1 and DShow interfaces, regardless of the
 *  license terms of these independent modules, and to copy and distribute
 *  the resulting combined work under terms of your choice, provided that
 *  every copy of the combined work is accompanied by a complete copy of
 *  the source code of XviD (the version of XviD used to produce the
 *  combined work), being distributed under the terms of the GNU General
 *  Public License plus this exception.  An independent module is a module
 *  which is not derived from or based on XviD.
 *
 *  Note that people who make modified versions of XviD are not obligated
 *  to grant this special exception for their modified versions; it is
 *  their choice whether to do so.  The GNU General Public License gives
 *  permission to release a modified version without this exception; this
 *  exception also makes it possible to release a modified version which
 *  carries forward this exception.
 *
 * $Id: reduced.c,v 1.1.2.2 2002-12-09 10:47:05 suxen_drol Exp $
 *
 ****************************************************************************/

#include "../portab.h"
#include "reduced.h"

// function pointers
COPY_UPSAMPLED_8X8_16TO8 * copy_upsampled_8x8_16to8;
ADD_UPSAMPLED_8X8_16TO8 * add_upsampled_8x8_16to8;
VFILTER_31 * vfilter_31;
HFILTER_31 * hfilter_31;
FILTER_18X18_TO_8X8 * filter_18x18_to_8x8;
FILTER_DIFF_18X18_TO_8X8 * filter_diff_18x18_to_8x8;

//////////////////////////////////////////////////////////
// Upsampling (1/3/3/1) filter

#define CLIP(x) ((x)<0 ? 0 : (x)>255 ? 255 : (x))
#define ADD(dst,src)  (dst) = CLIP((dst)+(src))

static __inline void Filter_31(uint8_t *Dst1, uint8_t *Dst2,
                             const int16_t *Src1, const int16_t *Src2)
{
    /* Src[] is assumed to be >=0. So we can use ">>2" instead of "/2" */
  int16_t a = (3*Src1[0]+  Src2[0]+2) >> 2;
  int16_t b = (  Src1[0]+3*Src2[0]+2) >> 2;
  Dst1[0] = CLIP(a);
  Dst2[0] = CLIP(b);
}

static __inline void Filter_9331(uint8_t *Dst1, uint8_t *Dst2,
                               const int16_t *Src1, const int16_t *Src2)
{
    /* Src[] is assumed to be >=0. So we can use ">>4" instead of "/16" */
  int16_t a = (9*Src1[0]+  3*Src1[1]+ 3*Src2[0] + 1*Src2[1] + 8) >> 4;
  int16_t b = (3*Src1[0]+  9*Src1[1]+ 1*Src2[0] + 3*Src2[1] + 8) >> 4;
  int16_t c = (3*Src1[0]+  1*Src1[1]+ 9*Src2[0] + 3*Src2[1] + 8) >> 4;
  int16_t d = (1*Src1[0]+  3*Src1[1]+ 3*Src2[0] + 9*Src2[1] + 8) >> 4;
  Dst1[0] = CLIP(a);
  Dst1[1] = CLIP(b);
  Dst2[0] = CLIP(c);
  Dst2[1] = CLIP(d);
}

void xvid_Copy_Upsampled_8x8_16To8_C(uint8_t *Dst, const int16_t *Src, const int BpS)
{
  int x, y;

  Dst[0] = CLIP(Src[0]);
  for(x=0; x<7; ++x) Filter_31(Dst+2*x+1, Dst+2*x+2, Src+x, Src+x+1);
  Dst[15] = CLIP(Src[7]);
  Dst += BpS;
  for(y=0; y<7; ++y) {
    uint8_t *const Dst2 = Dst + BpS;
    Filter_31(Dst, Dst2, Src, Src+8);
    for(x=0; x<7; ++x)
      Filter_9331(Dst+2*x+1, Dst2+2*x+1, Src+x, Src+x+8);
    Filter_31(Dst+15, Dst2+15, Src+7, Src+7+8);
    Src += 8; 
    Dst += 2*BpS;
  }
  Dst[0] = CLIP(Src[0]);
  for(x=0; x<7; ++x) Filter_31(Dst+2*x+1, Dst+2*x+2, Src+x, Src+x+1);
  Dst[15] = CLIP(Src[7]);
}

static __inline void Filter_Add_31(uint8_t *Dst1, uint8_t *Dst2,
                             const int16_t *Src1, const int16_t *Src2)
{
    /* Here, we must use "/4", since Src[] is in [-256, 255] */
  int16_t a = (3*Src1[0]+  Src2[0] + 2) / 4;
  int16_t b = (  Src1[0]+3*Src2[0] + 2) / 4;
  ADD(Dst1[0], a);
  ADD(Dst2[0], b);
}

static __inline void Filter_Add_9331(uint8_t *Dst1, uint8_t *Dst2,
                                   const int16_t *Src1, const int16_t *Src2)
{
  int16_t a = (9*Src1[0]+  3*Src1[1]+ 3*Src2[0] + 1*Src2[1] + 8) / 16;
  int16_t b = (3*Src1[0]+  9*Src1[1]+ 1*Src2[0] + 3*Src2[1] + 8) / 16;
  int16_t c = (3*Src1[0]+  1*Src1[1]+ 9*Src2[0] + 3*Src2[1] + 8) / 16;
  int16_t d = (1*Src1[0]+  3*Src1[1]+ 3*Src2[0] + 9*Src2[1] + 8) / 16;
  ADD(Dst1[0], a);
  ADD(Dst1[1], b);
  ADD(Dst2[0], c);
  ADD(Dst2[1], d);
}

void xvid_Add_Upsampled_8x8_16To8_C(uint8_t *Dst, const int16_t *Src, const int BpS)
{
  int x, y;

  ADD(Dst[0], Src[0]);
  for(x=0; x<7; ++x) Filter_Add_31(Dst+2*x+1, Dst+2*x+2, Src+x, Src+x+1);
  ADD(Dst[15], Src[7]);
  Dst += BpS;
  for(y=0; y<7; ++y) {
    uint8_t *const Dst2 = Dst + BpS;
    Filter_Add_31(Dst, Dst2, Src, Src+8);
    for(x=0; x<7; ++x)
      Filter_Add_9331(Dst+2*x+1, Dst2+2*x+1, Src+x, Src+x+8);
    Filter_Add_31(Dst+15, Dst2+15, Src+7, Src+7+8);
    Src += 8; 
    Dst += 2*BpS;
  }
  ADD(Dst[0], Src[0]);
  for(x=0; x<7; ++x) Filter_Add_31(Dst+2*x+1, Dst+2*x+2, Src+x, Src+x+1);
  ADD(Dst[15], Src[7]);
}
#undef CLIP
#undef ADD

//////////////////////////////////////////////////////////
// horizontal and vertical deblocking

void xvid_HFilter_31_C(uint8_t *Src1, uint8_t *Src2, int Nb_Blks)
{
  Nb_Blks *= 8;
  while(Nb_Blks-->0) {
    uint8_t a = ( 3*Src1[0] + 1*Src2[0] + 2 ) >> 2;
    uint8_t b = ( 1*Src1[0] + 3*Src2[0] + 2 ) >> 2;
    *Src1++ = a;
    *Src2++ = b;
  }
}

void xvid_VFilter_31_C(uint8_t *Src1, uint8_t *Src2, const int BpS, int Nb_Blks)
{
  Nb_Blks *= 8;
  while(Nb_Blks-->0) {
    uint8_t a = ( 3*Src1[0] + 1*Src2[0] + 2 ) >> 2;
    uint8_t b = ( 1*Src1[0] + 3*Src2[0] + 2 ) >> 2;
    *Src1 = a;
    *Src2 = b;
    Src1 += BpS;
    Src2 += BpS;
  }
}

//////////////////////////////////////////////////////////
// 16x16 -> 8x8  (1/3/3/1) downsampling
//
// Warning! These read 1 pixel outside of the input 16x16 block!
//
//////////////////////////////////////////////////////////

void xvid_Filter_18x18_To_8x8_C(int16_t *Dst, const uint8_t *Src, const int BpS)
{
  int16_t *T, Tmp[18*8];
  int i, j;

  T = Tmp;
  Src -= BpS;
  for(j=-1; j<17; j++) {
    for(i=0; i<8; ++i)
      T[i] = Src[2*i-1] + 3*Src[2*i+0] + 3*Src[2*i+1] + Src[2*i+2];
    T += 8;
    Src += BpS;
  }
  T = Tmp + 8;
  for(j=0; j<8; j++) {
    for(i=0; i<8; ++i)
      Dst[i] = ( T[-8+i] + 3*T[0+i] + 3*T[8+i] + T[16+i] + 32 ) / 64;
    Dst += 8;
    T += 16;
  }
}

void xvid_Filter_Diff_18x18_To_8x8_C(int16_t *Dst, const uint8_t *Src, const int BpS)
{
  int16_t *T, Tmp[18*8];
  int i, j;

  T = Tmp;
  Src -= BpS;
  for(j=-1; j<17; j++) {
    for(i=0; i<8; ++i)
      T[i] = Src[2*i-1] + 3*Src[2*i+0] + 3*Src[2*i+1] + Src[2*i+2];
    T += 8;
    Src += BpS;
  }
  T = Tmp;
  for(j=0; j<8; j++) {
    for(i=0; i<8; ++i)
      Dst[i] -= ( T[i] + 3*T[8+i] + 3*T[16+i] + T[24+i] + 32 ) / 64;
    Dst += 8;
    T += 16;
  }
}

//////////////////////////////////////////////////////////
