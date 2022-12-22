#	@(#)Makefile	8.1 (Berkeley) 6/4/93

LIB=	util
CFLAGS+=-DLIBC_SCCS -I/sys
PASSWD=	abort.c ent1.c enter.c init.c inplace.c install.c lock.c next.c \
	path.c rebuild.c sigs.c split.c unlimit.c unlock.c tmp.c update.c \
	copy.c misc.c
SRCS=	devname.c disklabel.c dkcksum.c login.c login_tty.c logout.c \
	logwtmp.c pty.c setproctitle.c setterm.c ${PASSWD}

.PATH: ${.CURDIR}/passwd

afterinstall:
	install -m 444 -o bin -g bin -c ${.CURDIR}/passwd/libpasswd.h \
		${DESTDIR}/usr/include

.include <bsd.lib.mk>
