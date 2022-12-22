#	BSDI	Makefile,v 2.1 1995/02/03 07:12:24 polk Exp

NOMAN=noman
NOOBJ=noobj

chkconfig clean depend lint tags:

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/chkconfig.sh ${DESTDIR}/sbin/chkconfig

.include <bsd.prog.mk>
