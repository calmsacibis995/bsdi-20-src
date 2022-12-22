#	BSDI	Makefile,v 2.1 1995/02/03 06:53:01 polk Exp

PROG=	try
SRCS=	try.c fakelog.c
NOMAN=	noman
CFLAGS+=-I${.CURDIR}/../lib

.include "../Makefile.defs"

LDADD+=	${LIBWRAP}
DPADD+=	${LIBWRAP}
BINDIR=	${TCPD_BINDIR}

.include <bsd.prog.mk>
