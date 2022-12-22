#	BSDI	Makefile,v 2.1 1995/02/03 06:52:49 polk Exp

PROG=	tcpd
SRCS=	tcpd.c fromhost.c
MAN5=	hosts_access.0 hosts_options.0
MAN8=	tcpd.0
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${LIBWRAP}
DPADD+=	${LIBWRAP}

.include "../Makefile.defs"

.PATH:	${.CURDIR}/../lib

.include <bsd.prog.mk>
