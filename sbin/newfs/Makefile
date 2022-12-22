#	BSDI	Makefile,v 2.1 1995/02/03 07:27:56 polk Exp
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	newfs
SRCS=	getmntopts.c newfs.c mkfs.c
MAN8=	newfs.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+=-DMFS -I${MOUNT}
.PATH:	${MOUNT} 

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	newfs.8 mount_mfs.8 newfs.8 mfs.8

.include <bsd.prog.mk>
