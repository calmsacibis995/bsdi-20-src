/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI fcvt.c,v 2.1 1995/02/03 06:57:04 polk Exp
 */

#include <stdlib.h>

extern char *__dtoa __P((double, int, int, int *, int *, char **));

char *
fcvt(value, ndigit, decpt, sign)
	double value;
	int ndigit, *decpt, *sign;
{
	char *digitstart, *digitend;
	char *buf, *bufend;
	static char *buf0;

	buf = __dtoa(value, 3, ndigit, decpt, sign, &digitend);

	/* __dtoa() returns a constant "0" for 0.0 */
	if (value == 0) {
		if (buf0)
			free(buf0);
		digitend = buf = buf0 = malloc(ndigit + 1);
		*decpt = 0;
	}

	/* __dtoa() suppresses trailing zeroes */
	if (*buf == '0' && value)
		*decpt = -ndigit + 1;
	bufend = buf + *decpt + ndigit;
	for (; digitend < bufend; *digitend++ = '0')
		;

	*digitend = '\0';

	if (*decpt < 0) {
		/* pad on the left with zeroes */
		if (buf0)
			free(buf0);
		digitstart = buf0 = malloc(ndigit + 1);
		digitend = digitstart - *decpt;
		while (digitstart < digitend)
			*digitstart++ = '0';
		*decpt = 0;
		strcat(buf0, buf);
		buf = buf0;
	}

	return (buf);
}
