/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI strncmp.s,v 2.1 1995/02/03 06:29:55 polk Exp"
#endif

/*
 * Compare two strings and return >0/0/<0 depending on string order.
 * Compare at most len bytes.
 *
 * int strncmp(const char *s1, const char *s2, size_t len);
 */
ENTRY(strncmp)
	pushl %esi
	pushl %edi

	movl 12(%esp),%esi
	movl 16(%esp),%edi
	movl 20(%esp),%ecx

	testl %ecx,%ecx
	jz Lwin

	cld

Lagain:
	cmpsb
	jne Lfound
	cmpb $0,-1(%edi)
	loopne Lagain

Lwin:
	xorl %eax,%eax

Lexit:
	popl %edi
	popl %esi
	ret

Lfound:
	movzbl -1(%esi),%eax
	movzbl -1(%edi),%ecx
	subl %ecx,%eax
	jmp Lexit
