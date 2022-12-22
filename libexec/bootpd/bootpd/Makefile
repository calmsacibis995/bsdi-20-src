#	BSDI	Makefile,v 2.1 1995/02/03 06:36:05 polk Exp

PROG=	bootpd
SRCS=	bootpd.c dovend.c readfile.c hash.c dumptab.c \
	lookup.c getif.c hwaddr.c report.c tzone.c ethers.c
CFLAGS+=-DETC_ETHERS -DSYSLOG -DDEBUG -DVEND_CMU

MAN8=	bootpd.0 
MAN5=	bootptab.0

.include <bsd.prog.mk>
