/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*	BSDI checkget.c,v 2.1 1995/02/03 06:39:58 polk Exp	*/

/* Thanks to Mike Locke: */
#include	<curses.h>
#include	<sgtty.h>
#include	<ctype.h>
#include 	<signal.h>
#include	"structs.h"
#include	<varargs.h>

struct sgttyb tty1;
struct ltchars tty2;

setblank(ly, lx, xstandout, len)
{
	(void) move(ly, lx);
	if (xstandout)
		(void) standout();
	while (len--)
		(void) addch(' ');
	(void) standend();
}

checkget(linelen, buf)
	char buf[];		/* where to put the input */
{
	int j;
	int c;
	char line[256];
	int ncharsin;
	int ly, lx;

	getyx(stdscr, ly, lx);
/* get character information: */
	z(ioctl(1, TIOCGETP, (char *) &tty1) != 0, "checkget: ioctl1");
	z(ioctl(1, TIOCGLTC, (char *) &tty2) != 0, "checkget: ioctl2");

	ncharsin = 0;
	setblank(ly, lx, __STANDOUT, linelen);
	(void) move(ly, lx);
	for (;;) {
		(void) refresh();
		c = getch();
		if (c == tty1.sg_erase) {	/* erase this character? */
			if (ncharsin) {	/* anything to erase? */
				erasechars(1);
				ncharsin--;
			}
			else {
				(void) printf("\007");
				z(fflush(stdout) == EOF, "Checkget: fflush??");
			}
			continue;
		}
		if (c == tty1.sg_kill) {	/* line kill */
			if (ncharsin) {
				erasechars(ncharsin);
				ncharsin = 0;
			}
			continue;
		}
		if (c == tty2.t_werasc) {	/* word erase */
			int nerase = 0;

			while (ncharsin &&
			    (line[ncharsin - 1] == ' ' || line[ncharsin - 1] == '\t')) {
				ncharsin--;
				nerase++;
			}
			while (ncharsin && (line[ncharsin - 1] != ' ' &&
				line[ncharsin - 1] != '\t')) {
				ncharsin--;
				nerase++;
			}
			erasechars(nerase);
			continue;
		}

		switch (c) {
		case '\014':	/* Refresh */
			clearok(stdscr, 1);
			touchwin(stdscr);
			(void) wrefresh(curscr);
			break;
		case '\04':	/* ^D */
		case NULL:
			setblank(ly, lx, 0, linelen);
			buf[0] = '\0';
			clearerr(stdin);
			return;
		case '\n':
		case '\r':	/* we're done now! */
			{
				int lastnonblank;

				/* copy to buffer */
				for (j = 0; j < linelen && j < ncharsin; j++) {
					if (line[j] != ' ')
						lastnonblank = j;
					buf[j] = line[j];
				}
				buf[j] = 0;
				showline(ly, lx, 0, buf);
				return;
			}
		default:
			if (!isprint(c) || ncharsin == linelen) {
				(void) printf("\007");
				break;
			}
			line[ncharsin++] = c;
			(void) standout();
			(void) addch(c);
			(void) standend();
			break;
		}
	}
}

showline(ly, lx, xstandout, line)
	char line[];
{
	int j;

	(void) move(ly, lx);	/* replot w/no standout & w/just */
	if (xstandout)
		(void) standout();
	for (j = 0; line[j]; j++)
/*	(void) addch (line[j] != ' ' ? line[j] : '_'); */
		(void) addch(line[j]);
	(void) standend();
}

erasechars(n)
	int n;			/* erase this many characters */
{
	int y, x;		/* row and column coordinates */

	if (n) {
		getyx(stdscr, y, x);
		setblank(y, x - n, __STANDOUT, n);
		(void) move(y, x - n);
	}
}

int curseson;			/* curses is in use */

/* When done with curses */
cleanup()
{
	if (curseson) {
		nocrmode();
		echo();
		move(LINES - 1, 0);
		refresh();
		endwin();
		curseson = 0;
		signal(SIGCONT, SIG_DFL);
	}
	fflush(stdout);
}

void
cleanupleave()
{
	cleanup();
	exit(0);
}

void
interrupt()
{
	char c;

	if (curseson) {
		if (everchanged == 0)
			cleanupleave();
		c = confirm("Really exit (y/n)?");
		if (c == 'y')
			cleanupleave();
	}
	else {
		printf("%s: Termination due to SIGINT\n", progname);
		exit(1);
	}
}

void
cleanup2()
{
	cleanup();
	(void) printf("%s: Termination due to SIGHUP\n", progname);
	exit(0);
}

void
cleanup3()
{
	cleanup();
	(void) printf("%s: Termination due to SIGSEGV\n", progname);
	abort();
}

/* start curses */
setup()
{
	extern int cleanup();

	(void) signal(SIGINT, interrupt);
	(void) signal(SIGHUP, cleanup2);
	(void) signal(SIGSEGV, cleanup3);
	z(initscr() == ERR, "setup:  initscr");
	crmode();
	noecho();
	scrollok(stdscr, 1);
	curseson = 1;
}


warn(va_alist) va_dcl
{
	va_list pvar;		/* var args traverser */
	char *format;
	char line[LINELEN];

	va_start(pvar);
	format = va_arg(pvar, char *);
	z(vsprintf(line, format, pvar) == EOF, "noteerr: vsprintf");
	va_end(pvar);
	move(warnline, 0);
	(void) clrtoeol();
	mvprintw(warnline, 0, "%.79s", line);
	refresh();
}

confirm(message)
	char *message;
{
	char c;
	WINDOW *iowin;
	int half;

	iowin = newwin(3, 80, 10, 0);
	(void) mvwprintw(iowin, 0, 0, "********************************************************************************");
	(void) mvwprintw(iowin, 1, 0, "********************************************************************************");
	(void) mvwprintw(iowin, 2, 0, "********************************************************************************");
	half = 40 - strlen(message) / 2 - 1;
	(void) mvwprintw(iowin, 1, half, " %s ", message);
	touchwin(iowin);
	(void) wrefresh(iowin);
	for (;;) {
		c = getch();
		if (c == '\014') {	/* referesh */
			clearok(stdscr, 1);
			touchwin(stdscr);
			(void) wrefresh(curscr);
			continue;
		}
		touchwin(stdscr);
		refresh();
		return c;
	}
}
