/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*	BSDI z.c,v 2.1 1995/02/03 06:40:24 polk Exp	*/

/*=========================================================================
 *
 * NAME: z checks assertions and prints appropriate messages
 *	 before aborting.
 *
 *	z (checkz, message, ...)
 *	   int bool:	if this is true, print/abort
 *	   char *message:  message to print before abort
 *			z (bool, message, parm1, parm2, ... parm4);
 * HISTORY:
 * 	initial coding -- R. Kolstad -- 2/28/84
 *	generalized -- R. Kolstad -- 10/5/84
 *	vararged -- R. Kolstad -- 2/13/92
 *
 *=========================================================================*/

#include	<varargs.h>
#include 	"structs.h"

extern int curseson;

z(va_alist) va_dcl
{
	va_list pvar;		/* var args traverser */
	int checkz;		/* whether we should quit or not */
	char *format;		/* printf message */
	int i;

	va_start(pvar);
	checkz = va_arg(pvar, int);

	if (!checkz)
		return;

	if (curseson)
		cleanup();

	format = va_arg(pvar, char *);
	(void) fprintf(stdout, "%s Fatal Error: ", progname);
	(void) vfprintf(stdout, format, pvar);
	(void) fprintf(stdout, "\n");
	va_end(pvar);
	exit(1);
}
