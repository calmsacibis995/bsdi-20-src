/*
 * Copyright (c) 1994
 *	Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	.asciz "BSDI alloca.s,v 2.1 1995/02/03 06:38:04 polk Exp"
#endif

/*
 * __builtin_alloca for sparc.
 *
 * gcc normally expands this inline, but some files
 * (e.g., sunGX.o in the MIT X11R5 distribution)
 * call the routine.
 */

#include "DEFS.h"

ENTRY(__builtin_alloca)
	add	%o0, 7, %o0
	andn	%o0, 7, %o0
	sub	%o0, %sp, %sp
	retl
	 add	%sp, 96, %o0
