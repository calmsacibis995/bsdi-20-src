/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI path.c,v 2.1 1995/02/03 09:19:32 polk Exp
 */

#include <db.h>
#include <err.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libpasswd.h"

/*
 * Construct a string holding the given path name, with directory
 * prefix stripped if appropriate, and with the given suffix appended.
 * We special case the empty suffix (which cannot fail).
 */
char *
pw_path(pw, name, suf)
	struct pwinfo *pw;
	char *name, *suf;
{
	char *p;
	size_t ln, ls;

	if (pw->pw_flags & PW_STRIPDIR) {
		if ((p = strrchr(name, '/')) != NULL)
			name = p + 1;
	}
	if (suf == NULL || (ls = strlen(suf)) == 0)
		return (name);
	ln = strlen(name);
	p = malloc(ln + ls + 1);
	if (p == NULL) {
		warn("cannot save name %s%s", name, suf);
		return (NULL);
	}
	memmove(p, name, ln);
	memmove(p + ln, suf, ls + 1);
	return (p);
}
