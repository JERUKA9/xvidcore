#ifndef _IDCT_H_
#define _IDCT_H_

void idct_int32_init();
void idct_ia64_init();

typedef void (idctFunc) (short *const block);
typedef idctFunc *idctFuncPtr;

extern idctFuncPtr idct;

idctFunc idct_int32;

idctFunc idct_mmx;
idctFunc idct_xmm;
idctFunc idct_sse2;

idctFunc idct_altivec;
idctFunc idct_ia64;

#endif							/* _IDCT_H_ */