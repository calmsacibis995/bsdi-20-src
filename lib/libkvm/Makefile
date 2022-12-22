#	BSDI Makefile,v 2.1 1995/02/03 08:32:27 polk Exp
#
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

LIB=	kvm
CFLAGS+=-DLIBC_SCCS -I/sys
SRCS=	kvm.c kvm_${MACHINE}.c kvm_boottime.c kvm_cpustats.c kvm_disks.c \
	kvm_file.c kvm_getloadavg.c kvm_kmem.c kvm_ncache.c kvm_proc.c \
	kvm_stathz.c kvm_ttys.c kvm_ttytotals.c kvm_util.c kvm_vmmeter.c \
	kvm_vmtotal.c kvm_vnodes.c

MAN3=	kvm.0 kvm_geterr.0 kvm_getfiles.0 kvm_getloadavg.0 kvm_getprocs.0 \
	kvm_nlist.0 kvm_open.0 kvm_read.0

MLINKS+=kvm_getprocs.3 kvm_getargv.3 kvm_getprocs.3 kvm_getenvv.3
MLINKS+=kvm_open.3 kvm_openfiles.3 kvm_open.3 kvm_close.3
MLINKS+=kvm_read.3 kvm_write.3

beforeinstall:
	-cd ${.CURDIR}; cmp -s kvm_stat.h \
	    ${DESTDIR}/usr/include/kvm_stat.h >/dev/null 2>&1 || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 kvm_stat.h \
	    ${DESTDIR}/usr/include

.include <bsd.lib.mk>
