/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI doprnt.c,v 2.1 1995/02/03 06:57:00 polk Exp
 */

#include <stdarg.h>
#include <stdio.h>

/*
 * Based on the (unfortunate) description of the interface
 * in the 4.3 BSD UPM, on the printf(3S) manual page.
 */

int
_doprnt(char *format, va_list ap, FILE *stream)
{

	return (vfprintf(stream, format, ap));
}
