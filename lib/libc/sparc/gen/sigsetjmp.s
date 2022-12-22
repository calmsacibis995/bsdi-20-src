/*
 * Copyright (c) 1994
 *	Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sigsetjmp.s,v 2.1 1995/02/03 06:38:44 polk Exp
 */

#include "DEFS.h"

#define	_JBLEN	9	/* XXX from setjmp.h */

/*
 * int sigsetjmp(sigjmp_buf jmpbuf, int savemask) {
 *	jmpbuf[_JBLEN] = savemask;
 *	return (savemask ? setjmp(jmpbuf) : _setjmp(jmpbuf));
 * }
 *
 * Note, however, that we must call setjmp/_setjmp from the current
 * frame.  Hence, sigsetjmp() MUST NOT be written in C.
 */
ENTRY(sigsetjmp)
	tst	%o1				! savemask
	be	1f
	st	%o1, [%o0 + (_JBLEN * 4)]	! jmpbuf[_JBLEN] = savemask

	/* `call' setjmp */
	sethi	%hi(_setjmp), %o2		! XXX -- not pc-relative
	jmpl	%o2 + %lo(_setjmp), %g0		! XXX -- not pc-relative
	 nop
	retl
	 nop

1:	/* `call' _setjmp */
	sethi	%hi(__setjmp), %o2		! XXX -- not pc-relative
	jmpl	%o2 + %lo(__setjmp), %g0	! XXX -- not pc-relative
	 nop
	retl
	 nop

/*
 * int siglongjmp(sigjmp_buf jmpbuf, int v) {
 *	return (jmpbuf[_JBLEN] ? longjmp(jmpbuf, v) : _longjmp(jmpbuf, v));
 * }
 *
 * This time we could go ahead and call longjmp (which does not return).
 * Annoyingly, that loses %o7, making traceback difficult, but then
 * longjmp tends to do the same... still, for now, we will use the same
 * technique as above.
 */
ENTRY(siglongjmp)
	ld	[%o0 + (_JBLEN * 4)], %o2	! jmpbuf[_JBLEN]
	tst	%o2
	be	1f
	 nop

	sethi	%hi(_longjmp), %o2
	jmpl	%o2 + %lo(_longjmp), %g0
	 nop
	/* NOTREACHED */
1:
	sethi	%hi(__longjmp), %o2
	jmpl	%o2 + %lo(__longjmp), %g0
	 nop
	/* NOTREACHED */
