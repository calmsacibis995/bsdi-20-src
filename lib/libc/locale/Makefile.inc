#	@(#)Makefile.inc	5.1 (Berkeley) 2/18/91

# locale sources
.PATH: ${.CURDIR}/${MACHINE}/locale ${.CURDIR}/locale

SRCS+=	euc.c frune.c isctype.c lconv.c localeconv.c mbrune.c multibyte.c \
	rune.c runenone.c runetable.c setlocale.c utf2.c

MAN3+=	ctype.0 isalnum.0 isalpha.0 isascii.0 isblank.0 iscntrl.0 isdigit.0 \
	isgraph.0 islower.0 isprint.0 ispunct.0 isspace.0 isupper.0 \
	isxdigit.0 mbrune.0 multibyte.0 rune.0 setlocale.0 toascii.0 \
	tolower.0 toupper.0
MAN5+=	euc.0 utf2.0

MLINKS+=mbrune.3 mbrrune.3 mbrune.3 mbmb.3
MLINKS+=multibyte.3 mblen.3 multibyte.3 mbstowcs.3 multibyte.3 mbtowc.3 \
	multibyte.3 wcstombs.3 multibyte.3 wctomb.3
MLINKS+=rune.3 setrunelocale.3 rune.3 setinvalidrune.3 rune.3 sgetrune.3 \
	rune.3 sputrune.3 rune.3 fgetrune.3 rune.3 fungetrune.3 \
	rune.3 fputrune.3
MLINKS+=setlocale.3 localeconv.3
