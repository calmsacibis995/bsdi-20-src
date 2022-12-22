
/* @(#)s_ilogb.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

/* ilogb(double x)
 * return the binary exponent of non-zero x
 * ilogb(0) = 0x80000001
 * ilogb(inf/NaN) = 0x7fffffff (no signal is raised)
 */

#include "fdlibm.h"

#ifdef __STDC__
static const double one  = 1.0;
#else
static double one  = 1.0;
#endif

#ifdef __STDC__
	int ilogb(double x)
#else
	int ilogb(x)
	double x;
#endif
{
	int n0,hx,lx,ix;

	n0 = ((*(int*)&one)>>29)^1;		/* high word index */
	hx  = (*(n0+(unsigned*)&x))&0x7fffffff;	/* high word of x */
	if(hx<0x00100000) {
	    lx = *(1-n0+(int*)&x);
	    if((hx|lx)==0) 
		return 0x80000001;	/* ilogb(0) = 0x80000001 */
	    else			/* subnormal x */
		if(hx==0) {
		    for (ix = -1043; lx>0; lx<<=1) ix -=1;
		} else {
		    for (ix = -1022,hx<<=11; hx>0; hx<<=1) ix -=1;
		}
	    return ix;
	}
	else if (hx<0x7ff00000) return (hx>>20)-1023;
	else return 0x7fffffff;
}
