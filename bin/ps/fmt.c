/*	BSDI	fmt.c,v 2.1 1995/02/03 05:47:07 polk Exp	*/

/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)fmt.c	8.4 (Berkeley) 4/15/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vis.h>
#include "ps.h"

static char *cmdpart __P((char *));
static char *shquote __P((char **));

/*
 * Produce an approximation of what one would feed to a shell to
 * build an argv.  Shells generally have no way to express control
 * characters indirectly, so we have to represent those a bit oddly.
 */
static char *
shquote(argv)
	char **argv;
{
	char **p, *dst, *src;
	char *buf;
	int len, q;
#ifdef notdef	/* strvis does not quote double quotes... */
#define needquotes(str) (*(str) == 0 || strpbrk(str, " \t") != NULL)
#else		/* so we will just quote empty arguments for now */
#define needquotes(str) (*(str) == 0)
#endif

	if (*argv == 0) {
		if ((buf = malloc(1)) != NULL)
			buf[0] = 0;
		return (buf);
	}
	len = 0;
	for (p = argv; (src = *p++) != 0;) {
		/* Quotes take two characters. */
		if (needquotes(src))
			len += 2;
		/* strvis() needs at most 4 bytes per source char. */
		len += 4 * strlen(src);
		/* Space or NUL needs one more byte. */
		len++;
	}
	if ((buf = malloc(len)) == NULL)
		return (NULL);
	dst = buf;
	for (p = argv; (src = *p++) != 0;) {
		q = needquotes(src);
		if (q)
			*dst++ = '"';
		dst += strvis(dst, src, VIS_NL | VIS_CSTYLE);
		if (q)
			*dst++ = '"';
		*dst++ = ' ';
	}
	dst[-1] = '\0';
	return (buf);
}

static char *
cmdpart(arg0)
	char *arg0;
{
	char *cp;

	return ((cp = strrchr(arg0, '/')) != NULL ? cp + 1 : arg0);
}

char *
fmt_argv(argv, cmd, maxlen)
	char **argv;
	char *cmd;
	int maxlen;
{
	int len;
	char *ap, *cp;

	if (argv == 0 || argv[0] == 0) {
		if (cmd == NULL)
			return ("");
		ap = NULL;
		len = maxlen + 3;
	} else {
		if ((ap = shquote(argv)) == NULL)
			return (NULL);
		len = strlen(ap) + maxlen + 4;
	}
	if ((cp = malloc(len)) == NULL)
		return (NULL);
	if (ap == NULL)
		sprintf(cp, "(%.*s)", maxlen, cmd);
	else if (strncmp(cmdpart(argv[0]), cmd, maxlen) != 0)
		sprintf(cp, "%s (%.*s)", ap, maxlen, cmd);
	else
		(void) strcpy(cp, ap);
	if (ap)
		free(ap);
	return (cp);
}
