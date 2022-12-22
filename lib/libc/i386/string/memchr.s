/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI memchr.s,v 2.1 1995/02/03 06:29:30 polk Exp"
#endif

/*
 * Locate the first byte with a particular value in a block.
 * Return a pointer to the byte, or 0.
 *
 * void memchr(void *b, int c, size_t len);
 */
ENTRY(memchr)
	pushl %edi

	movl 16(%esp),%ecx
	testl %ecx,%ecx
	jz Lnotfound

	movl 8(%esp),%edi
	movl 12(%esp),%eax

	cld
	repne; scasb

	jnz Lnotfound

	leal -1(%edi),%eax
	jmp Lexit

Lnotfound:
	xorl %eax,%eax

Lexit:
	popl %edi
	ret
