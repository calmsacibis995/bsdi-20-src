#	BSDI Makefile,v 2.1 1995/02/03 06:53:15 polk Exp
#
#	@(#)Makefile	8.2 (Berkeley) 12/15/93

PROG=	telnetd
CFLAGS+=-DLINEMODE -DUSE_TERMIO -DDIAGNOSTICS
CFLAGS+=-DOLD_ENVIRON -DENV_HACK
#CFLAGS+=-DAUTHENTICATION -DENCRYPTION -I${.CURDIR}/../../lib
.ifnmake(depend)
CFLAGS+=-DDEFAULT_IM="\"\r\nBSDI BSD/OS 2.0 (%h) (%t)\r\n\r\r\n\r\""
.endif
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c getent.c
DPADD=	${LIBUTIL} ${LIBTERM}
LDADD=	-lutil -ltermcap
#DPADD+=${LIBKRB} ${LIBDES}
#LDADD+=-lkrb -ldes
MAN8=	telnetd.0

# These are the sources that have encryption stuff in them.
CRYPT_SRC= authenc.c ext.h state.c telnetd.c termstat.c
CRYPT_SRC+= utility.c Makefile
NOCRYPT_DIR=${.CURDIR}/Nocrypt

.include <bsd.prog.mk>

nocrypt:
#ifdef	ENCRYPTION
	@for i in ${CRYPT_SRC}; do \
	    if [ ! -d ${NOCRYPT_DIR} ]; then \
		echo Creating subdirectory ${NOCRYPT_DIR}; \
		mkdir ${NOCRYPT_DIR}; \
	    fi; \
	    echo ${NOCRYPT_DIR}/$$i; \
	    unifdef -UENCRYPTION ${.CURDIR}/$$i | \
		sed "s/ || defined(ENCRYPTION)//" > ${NOCRYPT_DIR}/$$i; \
	done

placeholder:
#else	/* ENCRYPTION */
	@echo "Encryption code already removed."
#endif	/* ENCRYPTION */
