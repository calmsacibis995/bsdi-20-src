#	BSDI Makefile,v 2.1 1995/02/03 07:13:39 polk Exp

PROG=	disksetup
SRCS=	help.c
SRCS+=	disksetup.cc
SRCS+=	curses.c screen.cc showhelp.cc partition.cc disk.cc
SRCS+=	field.cc filesys.cc util.cc dkcksum.c print.cc
SRCS+=	bootblock.cc interactive.cc edit.cc geometry.o choosedisk.o
MAN8=	disksetup.0
CLEANFILES+=	help.c help.h

LDCC= g++

MKDEP=	mkdep -2 -I.

C++FLAGS+=	-I.

LDADD+=	-lcurses -ltermcap -lutil
DPADD+=	${LIBCURSES} ${LIBTERMCAP} ${LIBUTIL}

help.h help.c: help.txt Makefile
	@rm -f help.h
	@awk 'BEGIN { LINE=0; VAR=""; } \
    	    { if (substr($$0, 1, 1) == "#") { \
		if (LINE != 0) printf("0, };\nint %s_lines = %d;\n\n", VAR, LINE); \
		LINE = 0; VAR = $$2; \
		if (VAR != "") { \
		    printf("char *%s[] = {\n", VAR); \
		    printf("extern char *%s[];\nextern int   %s_lines;\n", VAR, VAR) > "help.h" ; \
    	    	} \
	      } else if (substr($$0,1,1) != ":") \
    	      if (VAR != "") { printf("\"%s\",\n", $$0); ++LINE; } \
	    } \
	    END { \
		if (LINE != 0) printf("0, };\nint %s_lines = %d;\n\n", VAR, LINE); \
	    } ' < ${.CURDIR}/help.txt > help.c

.depend: help.h help.c

.include <bsd.prog.mk>
