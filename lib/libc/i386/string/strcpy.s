/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"
#include "COPY.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI strcpy.s,v 2.1 1995/02/03 06:29:49 polk Exp"
#endif

/*
 * Copy a string into a buffer.
 * Return the address of the buffer.
 *
 * char *strcpy(char *dst, const char *src);
 */
ENTRY(strcpy)
	STR_PROLOGUE()

	cmpl %esi,%edi
	jbe Lforward	/* if destination precedes source, copy forward */

	COPY_REVERSE()
	jmp Lexit

Lforward:
	COPY()

Lexit:
	movl 12(%esp),%eax
	COPY_EPILOGUE()
