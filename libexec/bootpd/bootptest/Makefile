#	BSDI	Makefile,v 2.1 1995/02/03 06:37:24 polk Exp

PROG=	bootptest
SRCS=	bootptest.c print-bootp.c getif.c report.c getether.c
CFLAGS+=-I${.CURDIR}/../bootpd
MAN8=	bootptest.0

BINDIR=	/usr/sbin

.PATH:	${.CURDIR}/../bootpd

.include <bsd.prog.mk>
