#
#	Makefile,v 2.1 1995/02/03 07:23:52 polk Exp
#

PROG=	mount_msdos
SRCS=	mount_msdos.c getmntopts.c
MAN8=	mount_msdos.0

BINOWN=	root
BINMODE= 4555

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
