/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sigs.c,v 2.1 1995/02/03 09:19:39 polk Exp
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

#include <db.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>

#include "libpasswd.h"

static int sigs[] = {
	SIGALRM, SIGHUP, SIGINT, SIGPIPE, SIGQUIT, SIGTERM, SIGTSTP, SIGTTOU
};
#define NS (sizeof sigs / sizeof *sigs)

/*
 * Block or catch signals.  If actp is not NULL, defaulted and caught
 * signals are delivered to its handler, otherwise the signals are
 * simply blocked.  In any case, if setp is not NULL, the set of
 * `normally caught' signals is stored through it.
 */
void
pw_sigs(setp, actp)
	sigset_t *setp;
	struct sigaction *actp;
{
	register int i;
	sigset_t set, oset;
	struct sigaction oact;

	(void)sigemptyset(&set);
	for (i = 0; i < NS; i++)
		(void)sigaddset(&set, sigs[i]);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);
	if (setp != NULL)
		*setp = set;
	if (actp != NULL) {
		for (i = 0; i < NS; i++) {
			(void)sigaction(sigs[i], actp, &oact);
			if (oact.sa_handler == SIG_IGN)
				(void)sigaction(sigs[i], &oact, NULL);
		}
		(void)sigprocmask(SIG_SETMASK, &oset, NULL);
	}
}
