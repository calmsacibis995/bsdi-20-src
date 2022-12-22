#	BSDI Makefile,v 2.1 1995/02/03 06:39:33 polk Exp

PROG=	diff3
SRCS=	diff3.c getopt.c getopt1.c version.c
CFLAGS+=-DDIFF_PROGRAM=\"/usr/bin/diff\" 
CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}/../../usr.bin/diff/diff

.PATH:	${.CURDIR}/../../usr.bin/diff/diff

.include <bsd.prog.mk>
