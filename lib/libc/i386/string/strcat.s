/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include "DEFS.h"
#include "COPY.h"

#ifdef LIBC_SCCS
	.asciz	"BSDI strcat.s,v 2.1 1995/02/03 06:29:43 polk Exp"
#endif

/*
 * Concatenate one string with another.
 * Return the address of the destination string.
 *
 * char *strcat(char *dst, const char *src);
 */
ENTRY(strcat)
	STR_PROLOGUE()

	movl %ecx,%edx
	/* xorl %eax,%eax -- done in prologue */
	movl $-1,%ecx
	repne; scasb

	decl %edi
	movl %edx,%ecx

	cmpl %esi,%edi
	jbe Lforward	/* if destination precedes source, copy forward */

	COPY_REVERSE()
	jmp Lexit

Lforward:
	COPY()

Lexit:
	movl 12(%esp),%eax
	COPY_EPILOGUE()
