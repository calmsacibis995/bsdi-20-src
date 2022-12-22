#	BSDI Makefile,v 2.1 1995/02/03 06:50:49 polk Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	rshd
SRCS=	rshd.c
MAN8=	rshd.0

# Kerberized version now gets build in domestic
#CFLAGS+=-DKERBEROS -DCRYPT
#DPADD=	${LIBKRB} ${LIBDES}
#LDADD=	-lkrb -ldes

.PATH: ${.CURDIR}/../../usr.bin/rlogin

.include <bsd.prog.mk>
