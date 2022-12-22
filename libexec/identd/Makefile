#	BSDI	Makefile,v 2.1 1995/02/03 06:45:39 polk Exp

PROG=	identd
SRCS=	identd.c parse.c kernel.c version.c config.c proxy.c crypto.c
CFLAGS+=-DHAVE_KVM -DINCLUDE_EXTENSIONS
LDADD+=	-lkvm
DPADD+=	${LIBKVM}

.include <bsd.prog.mk>
