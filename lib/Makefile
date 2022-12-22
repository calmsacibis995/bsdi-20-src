#	BSDI Makefile,v 2.1 1995/02/03 06:11:28 polk Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

# missing: libmp libplot
# broken: libcdrom
SUBDIR=	libc libcompat libcurses libedit libg++ libkvm libm \
	liboldcurses libresolv librpc/rpc libterm libutil liby

.if	${MACHINE} == "tahoe"
SUBDIR+=csu/tahoe.pcc
.elif	${MACHINE} == "vax"
SUBDIR+=csu/vax.pcc
.else
SUBDIR+=csu/${MACHINE}
.endif

.include <bsd.subdir.mk>
