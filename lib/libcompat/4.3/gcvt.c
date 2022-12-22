/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI gcvt.c,v 2.1 1995/02/03 06:57:06 polk Exp
 */

#include <stdio.h>

char *
gcvt(value, ndigit, buf)
	double value;
	int ndigit;
	char *buf;
{

	sprintf(buf, "%.*g", ndigit, value);
	return (buf);
}
