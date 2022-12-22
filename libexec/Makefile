#	BSDI Makefile,v 2.1 1995/02/03 06:35:55 polk Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

SUBDIR=	bootpd bugfiler comsat diff3 fingerd ftpd getNAME getty identd \
	lfs_cleanerd mail.local makekey popper rexecd rlogind rshd talkd \
	tcpd telnetd tftpd

.if	${MACHINE} == "hp300"
SUBDIR+=rbootd
.elif	${MACHINE} == "luna68k"
SUBDIR+=rbootd
.endif

.include <bsd.subdir.mk>
