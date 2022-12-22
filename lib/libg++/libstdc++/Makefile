#	BSDI Makefile,v 2.1 1995/02/03 08:32:16 polk Exp

# libstdc++ Makefile -- built by hand from numerous Makefile.in files
# in the 2.6.2 distribution.

CC=	gcc2
INC=	-I${.CURDIR}
CFLAGS=	-O2 ${INC}
C++FLAGS+=-nostdinc++ ${INC}

MKDEP=	mkdep -2 -nostdinc++

LIB=	stdc++

INC+=	-I${.CURDIR}/../libg++

# libio sources.
.include "../Makefile.libio"
.PATH: ${LIBIO}
INC+=	-I${LIBIO}

SRCS+=	${LIBIO_C}

# libstdc++ sources.
.PATH: ${.CURDIR}/../D/libstdc++
INC+=	-I${.CURDIR}/../D/libstdc++

SRCS+=	cstring.cc exception.cc stddef.cc typeinfo.cc

# libstl sources.
.PATH: ${.CURDIR}/../D/libstdc++/stl
INC+=	-I${.CURDIR}/../D/libstdc++/stl

SRCS+=	random.cc tempbuf.cc tree.cc

# "generated" sources:
# string functions (one flavor: plain char)
STR=	REP MAIN TRAITS ADDSS ADDPS ADDCS ADDSP ADDSC \
	EQSS EQPS EQCS EQSP EQSC \
	NESS NEPS NECS NESP NESC EXTRACT INSERT

# complex functions (three flavors: float, double, long double)
CMPLX=	MAIN ADDCC ADDCF ADDFC SUBCC SUBCF SUBFC MULCC MULCF MULFC DIVCC \
	DIVCF DIVFC PLUS MINUS EQCC EQCF EQFC NECC NECF NEFC ABS ARG POLAR \
	CONJ NORM COS COSH EXP LOG POWCC POWCF POWCI POWFC SIN SINH SQRT \
	EXTRACT INSERT

STR_C=	${STR:S/^/c/g:S/$/.cc/g}
CMPLX_F=${CMPLX:S/^/f/g:S/$/.cc/g}
CMPLX_D=${CMPLX:S/^/d/g:S/$/.cc/g}
CMPLX_LD=${CMPLX:S/^/ld/g:S/$/.cc/g}

# the sed's below remove the initial `#line's for `#pragma implementation'
SED=	sed -e 1s,^,//,
${STR_C}: sinst.cc
	${C++C} ${INC} -E -DC -D${.TARGET:R:S/c//} ${.ALLSRC} | \
	    ${SED} > ${.TARGET}
${CMPLX_F}: cinst.cc
	${C++C} ${INC} -E -DF -D${.TARGET:R:S/f//} ${.ALLSRC} | \
	    ${SED} > ${.TARGET}
${CMPLX_D}: cinst.cc
	${C++C} ${INC} -E -DD -D${.TARGET:R:S/d//} ${.ALLSRC} | \
	    ${SED} > ${.TARGET}
${CMPLX_LD}: cinst.cc
	${C++C} ${INC} -E -DLD -D${.TARGET:R:S/ld//} ${.ALLSRC} | \
	    ${SED} > ${.TARGET}

AUTO=	${STR_C} ${CMPLX_F} ${CMPLX_D} ${CMPLX_LD}
SRCS+=	${AUTO}
CLEANFILES+=${AUTO}

.depend: ${AUTO}

NOMAN=		noman

.include <bsd.lib.mk>
