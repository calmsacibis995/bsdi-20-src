/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"
#include "COPY.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI memcpy.s,v 2.1 1995/02/03 06:29:34 polk Exp"
#endif

/*
 * Copy routine for non-overlapping regions.
 * Move len bytes of data from src to dst.
 * Return dst.
 *
 * void *memcpy(void *dst, const void *src, size_t len);
 */
ENTRY(memcpy)
	MEM_COPY_PROLOGUE()
	COPY()
	movl 12(%esp),%eax
	COPY_EPILOGUE()
