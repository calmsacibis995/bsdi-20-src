#	BSDI Makefile,v 1.3 1992/01/04 19:13:06 kolstad Exp
#	from @(#)Makefile	5.3 (Berkeley) 5/11/90

PROG=	diskdefect
SRCS=	diskdefect.c drck_scan.c errlib.c

.if defined(DEBUG)
# CFLAGS+=-DDEBUG_WRITE
CFLAGS+=-g -DDEBUG
.endif

.if defined(WARN)
CFLAGS+=-Wall -Dlint
.endif

MAN8=	diskdefect.0
.if ${MACHINE} == "vax"
MANSUBDIR=/vax
.elif ${MACHINE} == "i386"
MANSUBDIR=/i386
.endif

.include <bsd.prog.mk>
