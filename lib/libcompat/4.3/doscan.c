/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI doscan.c,v 2.1 1995/02/03 06:57:02 polk Exp
 */

#include <stdarg.h>
#include <stdio.h>

/*
 * Modelled after _doprnt(), documented in the 4.3 BSD UPM.
 * Hope it works right.
 */

int
_doscan(char *format, va_list ap, FILE *stream)
{

	return (vfscanf(stream, format, ap));
}
