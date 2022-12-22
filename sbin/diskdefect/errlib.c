/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
-*/

#ifndef lint
static char rcsid[] = "@(#)BSDI errlib.c,v 2.1 1995/02/03 07:13:17 polk Exp";
#endif

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "errlib.h"

static const char *progname = NULL;

void
Psetname(const char *name)
{
	progname = name;
}

void
Perror(char *fmt,...)
{
	int errcode = errno;
	va_list ap;

	va_start(ap, fmt);
	if (progname != NULL)
		(void) fprintf(stderr, "%s: ", progname);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (errcode != 0)
		(void) fprintf(stderr, ": %s", strerror(errcode));
	(void) fprintf(stderr, "\n");
	exit(1);
}

void
Pwarn(char *fmt,...)
{
	int errcode = errno;
	va_list ap;

	va_start(ap, fmt);
	if (progname != NULL)
		(void) fprintf(stderr, "%s: ", progname);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (errcode != 0)
		(void) fprintf(stderr, ": %s", strerror(errcode));
	(void) fprintf(stderr, "\n");
}

void
Perrmsg(char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void
__Pmsg(char *fmt,...)
{
	va_list ap;
	va_start(ap, fmt);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
}
