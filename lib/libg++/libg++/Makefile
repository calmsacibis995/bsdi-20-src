#	BSDI Makefile,v 2.1 1995/02/03 08:32:00 polk Exp

# libg++ Makefile -- built by hand from numerous Makefile.in files
# in the 2.6.2 distribution.

CC=	gcc2
INC=	-I${.CURDIR}
CFLAGS=	-O2 ${INC}
C++FLAGS+=-nostdinc++ ${INC}

MKDEP=	mkdep -2 -nostdinc++

LIB=	g++2

# libio sources.
.include "../Makefile.libio"
.PATH: ${LIBIO}
INC+=	-I${LIBIO}

SRCS+=	${LIBIO_C}

# libg++ sources.
.PATH: ${.CURDIR}/../D/libg++/src
INC+=	-I${.CURDIR}/../D/libg++/src

SRCS+=	ACG.cc AllocRing.cc Binomial.cc BitSet.cc BitString.cc Complex.cc \
	CursesW.cc DLList.cc DiscUnif.cc Erlang.cc Fix.cc Fix16.cc Fix24.cc \
	Geom.cc GetOpt.cc HypGeom.cc Intdouble.cc Integer.cc LogNorm.cc \
	MLCG.cc NegExp.cc Normal.cc Obstack.cc Poisson.cc RNG.cc Random.cc \
	Rational.cc Regex.cc RndInt.cc SLList.cc SmplHist.cc SmplStat.cc \
	String.cc Uniform.cc Weibull.cc bitand.c bitany.c bitblt.c \
	bitclear.c bitcopy.c bitcount.c bitinvert.c bitlcomp.c bitset1.c \
	bitxor.c builtin.cc chr.cc compare.cc error.cc except.c fmtq.cc \
	gcd.cc hash.cc ioob.cc lg.cc new.cc pow.cc sqrt.cc str.cc timer.c
#SRCS+=	math.cc

NOMAN=		noman

afterinstall:
	rm -f ${DESTDIR}${LIBDIR}/libg++.a
	ln ${DESTDIR}${LIBDIR}/libg++2.a ${DESTDIR}${LIBDIR}/libg++.a
	rm -f ${DESTDIR}${LIBDIR}/libg++_p.a
	ln ${DESTDIR}${LIBDIR}/libg++2_p.a ${DESTDIR}${LIBDIR}/libg++_p.a

.include <bsd.lib.mk>
