#	BSDI	Makefile,v 2.1 1995/02/03 07:28:31 polk Exp
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

PROG=	newlfs
SRCS=	lfs.c lfs_cksum.c misc.c newfs.c
MAN8=	newlfs.0

CFLAGS+=-I/sys/ufs/lfs
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.PATH:	/sys/ufs/lfs

.include <bsd.prog.mk>
