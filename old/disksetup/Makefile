#	BSDI	Makefile,v 2.1 1995/02/03 06:39:53 polk Exp
#	@(#)Makefile	5.6 (Berkeley) 4/1/91

PROG=	disksetup
SRCS=	disksetup.c checkget.c z.c interactive.c
MAN8=	disksetup.0
LDFLAGS+=-N
LDADD=	-lcurses -ltermcap -lutil
DPADD=	${LIBCURSES} ${LIBTERMCAP} ${LIBUTIL}
BINGRP=operator
BINMODE=2550

.include <bsd.prog.mk>
