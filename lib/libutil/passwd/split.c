/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI split.c,v 2.1 1995/02/03 09:19:45 polk Exp
 */

/*-
 * Copyright (c) 1990, 1993, 1994
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

#include <sys/param.h>

#include <db.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libpasswd.h"

char *
pw_split(bp, pw, checkroot)
	char *bp;
	struct passwd *pw;
	int checkroot;
{
	int isroot;
	long id;
	char *p, *ep, *uidstr, *gidstr, *chgstr, *expstr, *sh;
	static char buf[100];

	/* Split, ensuring exact right number of fields. */
	pw->pw_name = strsep(&bp, ":");
	pw->pw_passwd = strsep(&bp, ":");
	uidstr = strsep(&bp, ":");
	gidstr = strsep(&bp, ":");
	pw->pw_class = strsep(&bp, ":");
	chgstr = strsep(&bp, ":");
	expstr = strsep(&bp, ":");
	pw->pw_gecos = strsep(&bp, ":");
	pw->pw_dir = strsep(&bp, ":");
	pw->pw_shell = strsep(&bp, ":");
	p = strsep(&bp, ":");

	/* Too few fields => pw_shell is NULL; too many => p is not NULL. */
	if (pw->pw_shell == NULL || p != NULL)
		return ("corrupted entry (wrong number of fields)");

	isroot = strcmp(pw->pw_name, "root") == 0;

	id = strtol(uidstr, &ep, 10);
	if (*ep != '\0')
		return ("corrupted entry (bad uid)");
	if (isroot && id)
		return ("root uid should be 0");
	if (id > INT_MAX) {
		(void)snprintf(buf, sizeof buf, "uid %s > max uid %d",
		    uidstr, INT_MAX);
		return (buf);
	}
	pw->pw_uid = id;

	id = strtol(gidstr, &ep, 10);
	if (*ep != '\0')
		return ("corrupted entry (bad gid)");
	if (id > INT_MAX) {
		(void)snprintf(buf, sizeof buf, "gid %s > max gid %d",
		    gidstr, INT_MAX);
		return (buf);
	}
	pw->pw_gid = id;

	pw->pw_change = atol(chgstr);		/* XXX should check */
	pw->pw_expire = atol(expstr);		/* XXX should check */

	p = pw->pw_shell;
	if (isroot && checkroot && *p != '\0') {	/* empty == /bin/sh */
		for (setusershell(); (sh = getusershell()) != NULL;)
			if (strcmp(p, sh) == 0)
				goto shell_ok;
		warnx("warning, unknown root shell");
shell_ok:	;
	}
	return (NULL);
}
