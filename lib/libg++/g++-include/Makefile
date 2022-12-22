#	BSDI Makefile,v 2.1 1995/02/03 08:31:39 polk Exp

# install G++ headers -- built by hand from numerous Makefile.in files
# in the 2.6.2 distribution.

INCDIR=	/usr/include/g++
CXXDIR=	/usr/include/g++/stdc++

# headers changed from the distribution
LIBBSD=	${.CURDIR}/../libg++
LIBBSD_H=_G_config.h curses.h

# libio
.include "../Makefile.libio"

# libstdc++
LIBCXX=	${.CURDIR}/../D/libstdc++
LIBCXX_H=cassert cctype cerrno cfloat ciso646 climits clocale cmath \
	complex csetjmp csignal cstdarg cstddef cstdio cstdlib cstring \
	ctime cwchar cwctype new stddef string exception \
	complex.h new.h typeinfo.h \
	bastring.ccI bastring.hI cinst.hI complext.ccI complext.hI \
	dcomplex.hI ldcomplex.hI sinst.hI straits.hI

# libg++
LIBGXX=	${.CURDIR}/../D/libg++/src
LIBGXX_H=ACG.h AllocRing.h Binomial.h BitSet.h BitString.h Complex.h \
	CursesW.h DLList.h DiscUnif.h Erlang.h Fix.h Fix16.h Fix24.h Geom.h \
	GetOpt.h HypGeom.h Incremental.h Integer.h LogNorm.h MLCG.h \
	NegExp.h Normal.h Obstack.h Pix.h Poisson.h RNG.h Random.h \
	Rational.h Regex.h RndInt.h SLList.h SmplHist.h SmplStat.h \
	String.h Uniform.h Weibull.h bitdo1.h bitdo2.h bitprims.h \
	bool.h builtin.h compare.h complex.h defines.h generic.h \
	getpagesize.h libc.h minmax.h new.h osfcn.h rx.h std.h \
	strclass.h swap.h sysent.h typemacros.h

# gen *P files
GEN=	${.CURDIR}/../D/libg++/src/gen

# gen

all clean cleandir depend lint obj tags:

install:
	-[ -d ${DESTDIR}${INCDIR} ] || mkdir -p ${DESTDIR}${INCDIR}
	-[ -d ${DESTDIR}${CXXDIR} ] || mkdir -p ${DESTDIR}${CXXDIR}
	@echo installing ${LIBBSD_H}
	@-cd ${LIBBSD}; for i in ${LIBBSD_H}; do \
	    cmp -s $$i ${DESTDIR}${INCDIR}/$$i || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}${INCDIR}/$$i; \
	done
	@echo installing ${LIBCXX_H}
	@-cd ${LIBCXX}; for i in ${LIBCXX_H}; do \
	    cmp -s $$i ${DESTDIR}${CXXDIR}/$$i || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}${CXXDIR}/$$i; \
	done
	@echo installing ${LIBIO_H}
	@-cd ${LIBIO}; for i in ${LIBIO_H}; do \
	    cmp -s $$i ${DESTDIR}${INCDIR}/$$i || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}${INCDIR}/$$i; \
	done
	@echo installing ${LIBGXX_H}
	@-cd ${LIBGXX}; for i in ${LIBGXX_H}; do \
	    cmp -s $$i ${DESTDIR}${INCDIR}/$$i || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}${INCDIR}/$$i; \
	done
	@echo installing gen
	@-[ -d ${DESTDIR}${INCDIR}/gen ] || mkdir ${DESTDIR}${INCDIR}/gen
	@-cd ${GEN}; for j in *.*P; do \
	    cmp -s $$j ${DESTDIR}${INCDIR}/gen/$$j || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
		${DESTDIR}${INCDIR}/gen/$$j; \
	done

.include <bsd.prog.mk>
