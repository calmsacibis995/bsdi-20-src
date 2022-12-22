/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI fs.h,v 1.1 1994/12/05 21:20:09 prb Exp	*/

#include <sys/param.h>
#if	_BSDI_VERSION > 199312
#include <ufs/ffs/fs.h>
#else
#include	<ufs/fs.h>
#endif
