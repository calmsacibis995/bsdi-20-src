/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI bzero.s,v 2.1 1995/02/03 06:29:25 polk Exp"
#endif

/*
 * Zero out a block of data dst of len bytes.
 *
 * void bzero(void *dst, size_t len);
 */
ENTRY(bzero)
	pushl %edi

	movl 8(%esp),%edi
	movl 12(%esp),%ecx

	cld
	movl %ecx,%edx
	andl $3,%edx
	shrl $2,%ecx
	xorl %eax,%eax
	rep; stosl
	movl %edx,%ecx
	rep; stosb

	popl %edi
	ret
