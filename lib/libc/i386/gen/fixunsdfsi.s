/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	.asciz "BSDI fixunsdfsi.s,v 2.1 1995/02/03 06:28:43 polk Exp"
#endif

#define	FCW_CHOP	0x0c00

/*
 * Truncate a double to an unsigned int.
 * See fixdfsi.s for comments about the difficulties with truncation.
 */
	.globl	___fixunsdfsi
___fixunsdfsi:
	/*
	 * Prepare to change the rounding mode.
	 */
	subl $4,%esp
	fstcw (%esp)
	movw (%esp),%ax
	orw $FCW_CHOP,%ax
	movw %ax,2(%esp)

	/*
	 * Is it too big for an int?
	 */
	fldl 8(%esp)
	fcoml intovfl
	fstsw %ax
	sahf
	jb 0f

	/*
	 * It doesn't fit.
	 * Since we don't have an unsigned conversion instruction,
	 * we must adjust into int range, convert and adjust back.
	 */
	fsubl intovfl
	movl $0x80000000,%eax
	jmp 1f

0:
	xorl %eax,%eax		/* null adjustment */

	/*
	 * Peform the conversion from double to int.
	 * Note the little dance with the rounding mode.
	 */
1:
	fldcw 2(%esp)
	fistpl 8(%esp)
	fldcw (%esp)

	/*
	 * Make any final adjustments.
	 */
	addl 8(%esp),%eax
	addl $4,%esp
	ret

	.align 2
intovfl:
	.double 2147483648.

