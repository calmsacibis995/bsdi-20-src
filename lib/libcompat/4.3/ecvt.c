/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ecvt.c,v 2.1 1995/02/03 06:57:03 polk Exp
 */

#include <stdlib.h>

extern char *__dtoa __P((double, int, int, int *, int *, char **));

char *
ecvt(value, ndigit, decpt, sign)
	double value;
	int ndigit, *decpt, *sign;
{
	char *digitend;
	char *buf, *bufend;
	static char *zero;

	buf = __dtoa(value, 2, ndigit, decpt, sign, &digitend);

	/* __dtoa() returns a constant "0" for 0.0 */
	if (value == 0) {
		if (zero)
			free(zero);
		digitend = buf = zero = malloc(ndigit + 1);
	}

	/* __dtoa() suppresses trailing zeroes */
	for (bufend = buf + ndigit; digitend < bufend; *digitend++ = '0')
		;

	*digitend = '\0';

	return (buf);
}
