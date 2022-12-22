/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI index.s,v 2.1 1995/02/03 06:29:27 polk Exp"
#endif

/*
 * Look for a byte c with a particular value in a string s.
 *
 * char *index(const char *s, int c);
 */
ENTRY(index)
	pushl %esi

	movl 8(%esp),%esi
	movl 12(%esp),%edx

	cld

Lagain:
	lodsb
	cmpb %al,%dl
	je Lfound
	testb %al,%al
	jnz Lagain

	xorl %eax,%eax

Lexit:
	popl %esi
	ret

Lfound:
	leal -1(%esi),%eax
	jmp Lexit
