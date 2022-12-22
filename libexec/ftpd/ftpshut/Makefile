#	BSDI	Makefile,v 2.1 1995/02/03 06:43:13 polk Exp

PROG=	ftpshut
MAN8=	ftpshut.0
SRCS=	ftpshut.c vers.c
CFLAGS+=-I${.CURDIR}/../ftpd
BINDIR=	/usr/sbin

.PATH:	${.CURDIR}/../ftpd/obj

.include <bsd.prog.mk>
