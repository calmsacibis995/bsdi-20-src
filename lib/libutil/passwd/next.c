/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI next.c,v 2.1 1995/02/03 09:19:29 polk Exp
 */

/*-
 * Copyright (c) 1991, 1993, 1994
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

#include <assert.h>
#include <db.h>
#include <err.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "libpasswd.h"

/*
 * Obtain the next entry from the input master file.
 * Breaks it down as a side effect.
 */
int
pw_next(pw, pwd, aerr)
	struct pwinfo *pw;
	struct passwd *pwd;
	int *aerr;
{
	char *p;
	static char line[LINE_MAX];

	if (fgets(line, sizeof line, pw->pw_master.pf_fp) == NULL) {
		/* EOF or error. */
		if (ferror(pw->pw_master.pf_fp)) {
			warn("error reading %s (last read line %d)",
			    pw->pw_master.pf_name, pw->pw_line);
			*aerr = 1;
			return (0);
		}
		/* Normal EOF. */
		*aerr = 0;
		return (0);
	}
	pw->pw_line++;
	/*
	 * ``... if I swallow anything evil, put your fingers down my
	 * throat...''
	 *	-- The Who
	 */
	if ((p = strchr(line, '\n')) == NULL) {
		p = "line too long";
		goto bad;
	}
	*p = '\0';
	if ((p = pw_split(line, pwd, pw->pw_flags & PW_WARNROOT)) == NULL)
		return (1);

bad:
	warnx("%s, line %d: %s", pw->pw_master.pf_name, pw->pw_line, p);
	*aerr = 1;
	return (0);
}
