#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_cd9660
SRCS=	mount_cd9660.c getmntopts.c
MAN8=	mount_cd9660.0

LINKS=	${BINDIR}/mount_cd9660 ${BINDIR}/mount_iso9660
MLINKS=	mount_cd9660.8 mount_iso9660.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
