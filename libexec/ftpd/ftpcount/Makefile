#	BSDI	Makefile,v 2.1 1995/02/03 06:40:01 polk Exp

PROG=	ftpcount
SRCS=	ftpcount.c vers.c
CFLAGS+=-I${.CURDIR}/../ftpd
BINDIR=	/usr/sbin

.PATH:	${.CURDIR}/../ftpd/obj

.include <bsd.prog.mk>
