/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI memset.s,v 2.1 1995/02/03 06:29:39 polk Exp"
#endif

/*
 * Initialize all bytes in a block of data b of len bytes to value c.
 *
 * void *memset(void *b, int c, size_t len);
 */
ENTRY(memset)
	pushl %edi

	movl 8(%esp),%edi
	movl 12(%esp),%eax
	movl 16(%esp),%ecx

	/* clone the byte */
	movb %al,%ah
	movl %eax,%edx
	shll $16,%eax
	movw %dx,%ax

	/* copy by words */
	movl %ecx,%edx
	andl $3,%edx
	shrl $2,%ecx
	cld
	rep; stosl

	/* copy by bytes */
	movl %edx,%ecx
	rep; stosb

	movl 8(%esp),%eax

	popl %edi
	ret
