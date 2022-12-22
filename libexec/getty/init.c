/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI init.c,v 2.1 1995/02/03 06:45:26 polk Exp
 */


/*
 * Copyright (c) 1983, 1993
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
static char sccsid[] = "@(#)init.c	8.1 (Berkeley) 6/4/93";
#endif /* not lint */

/*
 * Getty table initializations.
 *
 * Melbourne getty.
 */
#include <termios.h>
#include "gettytab.h"
#include "pathnames.h"

extern	struct termios term;
extern	char hostname[];
#define C (char *)

struct	gettystrs gettystrs[] = {
	{ "nx" },				/* next table */
	{ "cl" },				/* screen clear characters */
						/* initial message */
	{ "im", "\r\n\r\nBSDI BSD/OS (%h) (%t)\r\n\r\n",  },
	{ "lm", "login: " },			/* login message */
	{ "er", C &term.c_cc[VERASE] },		/* erase character */
	{ "kl", C &term.c_cc[VKILL] },		/* kill character */
	{ "et", C &term.c_cc[VEOF] },		/* eof chatacter (eot) */
	{ "pc", "" },				/* pad character */
	{ "tt" },				/* terminal type */
	{ "ev" },				/* enviroment */
	{ "lo", _PATH_LOGIN },			/* login program */
	{ "hn", hostname },			/* host name */
	{ "he" },				/* host name edit */
	{ "in", C &term.c_cc[VINTR] },		/* interrupt char */
	{ "qu", C &term.c_cc[VQUIT] },		/* quit char */
	{ "xn", C &term.c_cc[VSTART] },		/* XON (start) char */
	{ "xf", C &term.c_cc[VSTOP] },		/* XOFF (stop) char */
	{ "bk", C &term.c_cc[VEOL] },		/* brk char (alt \n) */
	{ "su", C &term.c_cc[VSUSP] },		/* suspend char */
	{ "ds", C &term.c_cc[VDSUSP] },		/* delayed suspend */
	{ "rp", C &term.c_cc[VREPRINT] },	/* reprint char */
	{ "fl", C &term.c_cc[VDISCARD] },	/* flush output */
	{ "we", C &term.c_cc[VWERASE] },	/* word erase */
	{ "ln", C &term.c_cc[VLNEXT] },		/* literal next */
	{ "ms" },				/* mode string (stty style) */
	{ "m0" },				/* mode string 0 */
	{ "m1" },				/* mode string 1 */
	{ "m2" },				/* mode string 2 */
	{ 0 }
};

struct	gettynums gettynums[] = {
	{ "is" },			/* input speed */
	{ "os" },			/* output speed */
	{ "sp" },			/* both speeds */
	{ "to" },			/* timeout */
	{ "pf" },			/* delay before flush at 1st prompt */
	{ "de" },			/* delay after open before start */
	{ 0 }
};

struct	gettyflags gettyflags[] = {
	{ "ht",	0 },			/* has tabs */
	{ "nl",	1 },			/* has newline char */
	{ "ep",	0 },			/* even parity */
	{ "op",	0 },			/* odd parity */
	{ "ap",	0 },			/* any parity */
	{ "ec",	1 },			/* no echo */
	{ "co",	0 },			/* console special */
	{ "ck",	0 },			/* crt kill */
	{ "ce",	0 },			/* crt erase */
	{ "pe",	0 },			/* printer erase */
	{ "rw",	1 },			/* don't use raw */
	{ "xc",	1 },			/* don't ^X ctl chars */
	{ "ig",	0 },			/* ignore garbage */
	{ "ps",	0 },			/* do port selector speed select */
	{ "hc",	1 },			/* don't set hangup on close */
	{ "ub", 0 },			/* unbuffered output */
	{ "ab", 0 },			/* auto-baud detect with '\r' */
	{ "dx", 0 },			/* set decctlq */
	{ "np", 0 },			/* no parity at all (8bit chars) */
	{ "bi", 0 },			/* bidirectional modem */
	{ "hw", 0 },			/* hard-wired (no modem control) */
	{ "hf", 0 },			/* hardware flow control */
	{ 0 }
};
