/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"
#include "COPY.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI bcopy.s,v 2.1 1995/02/03 06:29:23 polk Exp"
#endif

/*
 * Copy routine for regions that may overlap.
 * Move len bytes of data from src to dst even if src < dst < src + len.
 *
 * void bcopy(const void *src, void *dst, size_t len);
 */
ENTRY(bcopy)
	COPY_PROLOGUE()
	cmpl %esi,%edi
	jbe Lforward	/* if destination precedes source, copy forward */

	COPY_REVERSE()
	jmp Lexit

Lforward:
	COPY()

Lexit:
	COPY_EPILOGUE()
