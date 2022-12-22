#	BSDI	Makefile,v 2.1 1995/02/03 05:47:33 polk Exp
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rcp
SRCS=	rcp.c util.c
# Kerberized version not gets built in domestic
#SRCS+=	krcmd.c kcmd.c 
#CFLAGS+=-DKERBEROS -DCRYPT
#DPADD=	${LIBKRB} ${LIBDES}
#LDADD=	-lkrb -ldes
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg
.PATH:	${.CURDIR}/../../usr.bin/rlogin

.include <bsd.prog.mk>
