/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI subr.c,v 2.1 1995/02/03 06:45:32 polk Exp
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
static char sccsid[] = "@(#)subr.c	8.1 (Berkeley) 6/4/93";
#endif /* not lint */

/*
 * Melbourne getty.
 */
#include <sys/ioctl.h>

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include "gettytab.h"
#include "extern.h"
#include "pathnames.h"

extern	struct termios term;

/*
 * Get a table entry.
 */
void
gettable(name, buf)
	char *name, *buf;
{
	register struct gettystrs *sp;
	register struct gettynums *np;
	register struct gettyflags *fp;
	long n;
	char *dba[2];
	dba[0] = _PATH_GETTYTAB;
	dba[1] = 0;

	if (cgetent(&buf, dba, name) != 0) {
		if (strcmp(name, "default") == 0)
			syslog(LOG_ERR, "missing default entry, check %s",
			    _PATH_GETTYTAB);
		else
			syslog(LOG_ERR, "entry %s missing in %s",
			    name, _PATH_GETTYTAB);
		return;
	}

	for (sp = gettystrs; sp->field; sp++)
		cgetstr(buf, sp->field, &sp->value);
	for (np = gettynums; np->field; np++) {
		if (cgetnum(buf, np->field, &n) == -1)
			np->set = 0;
		else {
			np->set = 1;
			np->value = n;
		}
	}
	for (fp = gettyflags; fp->field; fp++) {
		if (cgetcap(buf, fp->field, ':') == NULL)
			fp->set = 0;
		else {
			fp->set = 1;
			fp->value = 1 ^ fp->invrt;
		}
	}
#ifdef DEBUG
	printf("name=\"%s\", buf=\"%s\"\n", name, buf);
	for (sp = gettystrs; sp->field; sp++)
		printf("cgetstr: %s=%s\n", sp->field, sp->value);
	for (np = gettynums; np->field; np++)
		printf("cgetnum: %s=%d\n", np->field, np->value);
	for (fp = gettyflags; fp->field; fp++)
		printf("cgetflags: %s='%c' set='%c'\n", fp->field, 
		       fp->value + '0', fp->set + '0');
	exit(1);
#endif /* DEBUG */
}

void
gendefaults()
{
	register struct gettystrs *sp;
	register struct gettynums *np;
	register struct gettyflags *fp;

	for (sp = gettystrs; sp->field; sp++)
		if (sp->value)
			sp->defalt = sp->value;
	for (np = gettynums; np->field; np++)
		if (np->set)
			np->defalt = np->value;
	for (fp = gettyflags; fp->field; fp++)
		if (fp->set)
			fp->defalt = fp->value;
		else
			fp->defalt = fp->invrt;
}

void
setdefaults()
{
	register struct gettystrs *sp;
	register struct gettynums *np;
	register struct gettyflags *fp;

	for (sp = gettystrs; sp->field; sp++)
		if (!sp->value)
			sp->value = sp->defalt;
	for (np = gettynums; np->field; np++)
		if (!np->set)
			np->value = np->defalt;
	for (fp = gettyflags; fp->field; fp++)
		if (!fp->set)
			fp->value = fp->defalt;
}

static char **
charnames[] = {
	&ER, &KL, &IN, &QU, &XN, &XF, &ET, &BK,
	&SU, &DS, &RP, &FL, &WE, &LN, 0
};

static cc_t *
charvars[] = {
	&term.c_cc[VERASE], &term.c_cc[VKILL], &term.c_cc[VINTR],
	&term.c_cc[VQUIT], &term.c_cc[VSTART], &term.c_cc[VSTOP],
	&term.c_cc[VEOF], &term.c_cc[VEOL], &term.c_cc[VSUSP],
	&term.c_cc[VDSUSP], &term.c_cc[VREPRINT], &term.c_cc[VDISCARD],
	&term.c_cc[VWERASE], &term.c_cc[VLNEXT], 0
};

void
setchars()
{
	register int i;
	register char *p;

	for (i = 0; charnames[i]; i++) {
		p = *charnames[i];
		if (p && *p)
			*charvars[i] = *p;
		else
			*charvars[i] = _POSIX_VDISABLE;
	}
}

/*
 * No longer supported: f0, f1, f2, LC, UC, CB, delays
 */
void
settmode(n, t, dflt)
	int n;
	struct termios *t, *dflt;
{
	char *fail;
	char *Mset[3] = { M0, M1, M2 };
	extern char ttyn[];

	/*
	 * In mode 1, we normally clear INPCK and ISTRIP,
	 * so recover them from the default if we might have zapped them.
	 */
	t->c_iflag |= (dflt->c_iflag & (INPCK|ISTRIP));

	/*
	 * If none of NP/AP/EP/OP is set, use the default tty setting.
	 * Otherwise, force as much as necessary to get the requested
	 * behavior.  We assume 7 bits with parity, 8 bits without.
	 */
	if (NP) {
		t->c_cflag &= ~(PARENB|CSIZE);
		t->c_cflag |= CS8;
		t->c_iflag &= ~(INPCK|ISTRIP);
	} else if (AP || OP || EP) {
		t->c_cflag &= ~CSIZE;
		t->c_cflag |= PARENB|CS7;
		if (AP)
			t->c_iflag &= ~INPCK;
		else
			t->c_iflag |= INPCK;
		if (OP)
			t->c_cflag |= PARODD;
		else if (EP)
			t->c_cflag &= ~PARODD;
	}

	if (NL) {
		t->c_iflag |= ICRNL;
		t->c_oflag |= ONLCR|OPOST;
	}

	if (HF)
		t->c_cflag |= CCTS_OFLOW|CRTS_IFLOW;

	if (HC)
		t->c_cflag |= HUPCL;
	else
		t->c_cflag &= ~HUPCL;
	setchars();

	if (n == 1) {		/* read mode flags */
		t->c_lflag &= ~(ICANON|ECHO);
		/*
		 * For "raw" mode, we should in theory switch to 8 bits,
		 * no parity.  However, we ignore the "parity" bit anyway.
		 */
		if (RW) {
			t->c_iflag &= ~(IXON|INPCK|ISTRIP);
			t->c_lflag &= ~ISIG;
		}
		goto msmode;
	} else {
		t->c_iflag |= IXON;			/* force these? */
		t->c_lflag |= ICANON|ISIG;		/* force these? */
	}

	if (!HT)
		t->c_oflag |= OXTABS|OPOST;

	if (n == 0)
		goto msmode;

	/*
	 * The following gettytab options only set the flags.
	 * Users can clear the flags using :ms: if needed.
	 */

	if (CE)
		t->c_lflag |= ECHOE;

	if (CK)
		t->c_lflag |= ECHOKE;

	if (PE)
		t->c_lflag |= ECHOPRT;

	if (EC)
		t->c_lflag |= ECHO;

	if (XC)
		t->c_lflag |= ECHOCTL;

	if (DX)
		t->c_iflag &= ~IXANY;
	else
		t->c_iflag |= IXANY;

msmode:
	if (MS && (cfsetterm(t, MS, &fail) != 0))
		syslog(LOG_ERR, "%s: %s", ttyn, fail);
	if (n < 3 && Mset[n] && (cfsetterm(t, Mset[n], &fail) != 0))
		syslog(LOG_ERR, "%s: %s", ttyn, fail);
}

char	editedhost[32];

void
edithost(pat)
	register char *pat;
{
	register char *host = HN;
	register char *res = editedhost;

	if (!pat)
		pat = "";
	while (*pat) {
		switch (*pat) {

		case '#':
			if (*host)
				host++;
			break;

		case '@':
			if (*host)
				*res++ = *host++;
			break;

		default:
			*res++ = *pat;
			break;

		}
		if (res == &editedhost[sizeof editedhost - 1]) {
			*res = '\0';
			return;
		}
		pat++;
	}
	if (*host)
		strncpy(res, host, sizeof editedhost - (res - editedhost) - 1);
	else
		*res = '\0';
	editedhost[sizeof editedhost - 1] = '\0';
}

/* This is mostly a relic */
struct speedtab {
	int	speed;
	int	uxname;
} speedtab[] = {
	19,	B19200,		/* for people who say 19.2K */
	38,	B38400,
	0
};

int
speed(val)
	int val;
{
	register struct speedtab *sp;

	if (val <= 15)
		return (val);

	for (sp = speedtab; sp->speed; sp++)
		if (sp->speed == val)
			return (sp->uxname);
	
	return (val);		/* Bxxx is xxx */
}

void
makeenv(env)
	char *env[];
{
	static char termbuf[128] = "TERM=";
	register char *p, *q;
	register char **ep;

	ep = env;
	if (TT && *TT) {
		strcat(termbuf, TT);
		*ep++ = termbuf;
	}
	if (p = EV) {
		q = p;
		while (q = strchr(q, ',')) {
			*q++ = '\0';
			*ep++ = p;
			p = q;
		}
		if (*p)
			*ep++ = p;
	}
	*ep = (char *)0;
}

/*
 * This speed select mechanism is written for the Develcon DATASWITCH.
 * The Develcon sends a string of the form "B{speed}\n" at a predefined
 * baud rate. This string indicates the user's actual speed.
 * The routine below returns the terminal type mapped from derived speed.
 */
struct	portselect {
	char	*ps_baud;
	char	*ps_type;
} portspeeds[] = {
	{ "B110",	"std.110" },
	{ "B134",	"std.134" },
	{ "B150",	"std.150" },
	{ "B300",	"std.300" },
	{ "B600",	"std.600" },
	{ "B1200",	"std.1200" },
	{ "B2400",	"std.2400" },
	{ "B4800",	"std.4800" },
	{ "B9600",	"std.9600" },
	{ "B19200",	"std.19200" },
	{ 0 }
};

char *
portselector()
{
	char c, baud[20], *type = "default";
	register struct portselect *ps;
	int len;

	alarm(5*60);
	for (len = 0; len < sizeof (baud) - 1; len++) {
		if (read(STDIN_FILENO, &c, 1) <= 0)
			break;
		c &= 0177;
		if (c == '\n' || c == '\r')
			break;
		if (c == 'B')
			len = 0;	/* in case of leading garbage */
		baud[len] = c;
	}
	baud[len] = '\0';
	for (ps = portspeeds; ps->ps_baud; ps++)
		if (strcmp(ps->ps_baud, baud) == 0) {
			type = ps->ps_type;
			break;
		}
	sleep(2);	/* wait for connection to complete */
	return (type);
}

/*
 * This auto-baud speed select mechanism is written for the Micom 600
 * portselector. Selection is done by looking at how the character '\r'
 * is garbled at the different speeds.
 */
#include <sys/time.h>

char *
autobaud()
{
	int rfds;
	struct timeval timeout;
	char c, *type = "9600-baud";
	int null = 0;

	ioctl(0, TIOCFLUSH, &null);
	rfds = 1 << 0;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	if (select(32, (fd_set *)&rfds, (fd_set *)NULL,
	    (fd_set *)NULL, &timeout) <= 0)
		return (type);
	if (read(STDIN_FILENO, &c, sizeof(char)) != sizeof(char))
		return (type);
	timeout.tv_sec = 0;
	timeout.tv_usec = 20;
	(void) select(32, (fd_set *)NULL, (fd_set *)NULL,
	    (fd_set *)NULL, &timeout);
	ioctl(0, TIOCFLUSH, &null);
	switch (c & 0377) {

	case 0200:		/* 300-baud */
		type = "300-baud";
		break;

	case 0346:		/* 1200-baud */
		type = "1200-baud";
		break;

	case  015:		/* 2400-baud */
	case 0215:
		type = "2400-baud";
		break;

	default:		/* 4800-baud */
		type = "4800-baud";
		break;

	case 0377:		/* 9600-baud */
		type = "9600-baud";
		break;
	}
	return (type);
}
