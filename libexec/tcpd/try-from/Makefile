#	BSDI	Makefile,v 2.1 1995/02/03 06:53:09 polk Exp

PROG=	try-from
SRCS=	try-from.c fakelog.c
NOMAN=	noman
CFLAGS+=-I${.CURDIR}/../lib

.include "../Makefile.defs"

LDADD+=	${LIBWRAP}
DPADD+=	${LIBWRAP}
BINDIR=	${TCPD_BINDIR}

.PATH:	${.CURDIR}/../try

.include <bsd.prog.mk>
