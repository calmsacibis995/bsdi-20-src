#	BSDI	Makefile,v 2.1 1995/02/03 05:40:22 polk Exp
#	@(#)Makefile	8.1 (Berkeley) 6/19/93

#
#  These directories may be compiled direcly from CDROM
#
SUBDIR=	include lib bin games libexec old sbin share \
	usr.bin usr.sbin

#
#  Some of the directories (those with obj links to cdrom_obj)
#  may also be compiled direcly from CDROM
#
# SUBDIR+= contrib

#
#  The sco directory is shipped on the SCO floppy
#
# .if ${MACHINE} == "i386"
# SUBDIR+=sco
# .endif

#
#  The objsrc directory never leaves BSDI 
#
# SUBDIR+=objsrc

.include <bsd.subdir.mk>
