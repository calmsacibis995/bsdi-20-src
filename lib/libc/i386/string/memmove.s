/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"
#include "COPY.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI memmove.s,v 2.1 1995/02/03 06:29:37 polk Exp"
#endif

/*
 * Copy routine for regions that may overlap.
 * Move len bytes of data from src to dst even if src < dst < src + len.
 * Return dst.
 *
 * void *memmove(void *dst, const void *src, size_t len);
 */
ENTRY(memmove)
	MEM_COPY_PROLOGUE()
	cmpl %esi,%edi
	jbe Lforward	/* if destination precedes source, copy forward */

	COPY_REVERSE()
	jmp Lexit

Lforward:
	COPY()

Lexit:
	movl 12(%esp),%eax
	COPY_EPILOGUE()
