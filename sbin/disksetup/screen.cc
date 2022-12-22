/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI screen.cc,v 2.1 1995/02/03 07:15:57 polk Exp	*/

#include "screen.h"
#include "showhelp.h"
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#if	_BSDI_VERSION > 199312
#include <sys/time.h>
#endif

int Home::homex = 0;
int Home::homey = 0;

int
truth(char *s, int def)
{   
    while (isspace(*s))
        ++s;
    if (!*s || *s == '\n')
        return(def);
    if (*s == 'Y' || *s == 'y')
        return(1);
    if (*s == 'N' || *s == 'n')
        return(0);
    return(-1);
}       

int
verify(char *fmt, ...)
{
    char buf[1024];
    char *p, *b;
    char *msg = "<Y>es or <N>o?";

    int nlines = 0;
    int ncols = strlen(msg);

    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    b = buf;

    while (p = strchr(b, '\n')) {
    	if (p - b > ncols)
	    ncols = p - b;
    	++nlines;
    	b = p + 1;
    }
    if (*b) {
	if (strlen(b) > ncols)
	    ncols = strlen(b);
	++nlines;
    }

    ncols += 6;
    nlines += 6;

    int x = (80 - ncols) / 2;
    int y = (24 - nlines) / 2;
    int so = nosusp();
    WINDOW *w = newwin(nlines, ncols, y, x);

    wclear(w);
    box(w, BOX_CHAR, BOX_CHAR);

    y = 2;
    b = buf;

    while (p = strchr(b, '\n')) {
    	if (p != b) {
	    x = (ncols - (p - b)) / 2;
	    wmvprint(w, y, x, "%.*s", p - b, b);
    	}
    	++y;
    	b = p + 1;
    }
    if (*b) {
	x = (ncols - strlen(b)) / 2;
	wmvprint(w, y, x, "%s", b);
    }

    x = (ncols - strlen(msg)) / 2;
    wmvprint(w, y + 2, x, "%s ", msg);

    touchwin(w);
    wrefresh(w);

    x = -1;
    while (x == -1) {
	switch(readc()) {
    	case 'y': case 'Y':
	    x = 1;
	    break;
    	case 'n': case 'N':
	    x = 0;
	    break;
    	}
    }

    delwin(w);
    oksusp(so);

    touchwin(stdscr);
    refresh();
    return(x);
}

int
verify(char **lines, ...)
{
    int nlines = 0;
    char buf1[1024];
    char buf2[1024];
    char *p = buf1;

    char *msg = "<Y>es or <N>o?";
    int ncols = strlen(msg);

    for (nlines = 0; lines[nlines]; ++nlines) {
    	int len = strlen(lines[nlines]);
    	if (p != buf1)
	    *p++ = '\n';
    	strcpy(p, lines[nlines]);
    	p += len;
    }

    va_list ap;
    va_start(ap, lines);
    vsprintf(buf2, buf1, ap);
    va_end(ap);

    char *flines[nlines+1];

    p = buf2;
    for (int i = 0; p && i < nlines; ++i) {
    	flines[i] = p;
    	if (p = strchr(p, '\n'))
	    *p++ = '\0';
	if (strlen(flines[i]) > ncols)
	    ncols = strlen(flines[i]);
    }
    flines[nlines = i] = 0;

    ncols += 6;
    nlines += 6;

    int x = (80 - ncols) / 2;
    int y = (24 - nlines) / 2;
    int so = nosusp();
    WINDOW *w = newwin(nlines, ncols, y, x);

    wclear(w);
    box(w, BOX_CHAR, BOX_CHAR);

    y = 2;

    for (i = 0; flines[i]; ++i) {
	x = (ncols - strlen(flines[i])) / 2;
	wmvprint(w, y, x, "%s", flines[i]);
    	++y;
    }

    x = (ncols - strlen(msg)) / 2;
    wmvprint(w, y + 2, x, "%s ", msg);

    touchwin(w);
    wrefresh(w);

    x = -1;
    while (x == -1) {
	switch(readc()) {
    	case 'y': case 'Y':
	    x = 1;
	    break;
    	case 'n': case 'N':
	    x = 0;
	    break;
    	}
    }

    delwin(w);
    oksusp(so);

    touchwin(stdscr);
    refresh();
}

void
cinform(char *fmt)
{
    inform(fmt);
}

void
inform(char *fmt, ...)
{
    char buf[1024];
    char *p, *b;
    char *msg = "Press any key to continue";

    int nlines = 0;
    int ncols = strlen(msg);

    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    b = buf;

    while (p = strchr(b, '\n')) {
    	if (p - b > ncols)
	    ncols = p - b;
    	++nlines;
    	b = p + 1;
    }
    if (*b) {
	if (strlen(b) > ncols)
	    ncols = strlen(b);
	++nlines;
    }

    ncols += 6;
    nlines += 6;

    int x = (80 - ncols) / 2;
    int y = (24 - nlines) / 2;
    int so = nosusp();
    WINDOW *w = newwin(nlines, ncols, y, x);

    wclear(w);
    box(w, BOX_CHAR, BOX_CHAR);

    y = 2;
    b = buf;

    while (p = strchr(b, '\n')) {
    	if (p != b) {
	    x = (ncols - (p - b)) / 2;
	    wmvprint(w, y, x, "%.*s", p - b, b);
    	}
    	++y;
    	b = p + 1;
    }
    if (*b) {
	x = (ncols - strlen(b)) / 2;
	wmvprint(w, y, x, "%s", b);
    }

    x = (ncols - strlen(msg)) / 2;
    wmvprint(w, y + 2, x, "%s ", msg);

    touchwin(w);
    wrefresh(w);

    readc();
    delwin(w);
    oksusp(so);

    touchwin(stdscr);
    refresh();
}

void
inform(char **lines, ...) 
{
    int nlines = 0;
    char buf1[1024];
    char buf2[1024];
    char *p = buf1;

    char *msg = "Press any key to continue";
    int ncols = strlen(msg);

    for (nlines = 0; lines[nlines]; ++nlines) {
    	int len = strlen(lines[nlines]);
    	if (p != buf1)
	    *p++ = '\n';
    	strcpy(p, lines[nlines]);
    	p += len;
    }

    va_list ap;
    va_start(ap, lines);
    vsprintf(buf2, buf1, ap);
    va_end(ap);

    char *flines[nlines+1];

    p = buf2;
    for (int i = 0; p && i < nlines; ++i) {
    	flines[i] = p;
    	if (p = strchr(p, '\n'))
	    *p++ = '\0';
	if (strlen(flines[i]) > ncols)
	    ncols = strlen(flines[i]);
    }
    flines[nlines = i] = 0;

    ncols += 6;
    nlines += 6;

    int x = (80 - ncols) / 2;
    int y = (24 - nlines) / 2;
    int so = nosusp();
    WINDOW *w = newwin(nlines, ncols, y, x);

    wclear(w);
    box(w, BOX_CHAR, BOX_CHAR);

    y = 2;

    for (i = 0; flines[i]; ++i) {
	x = (ncols - strlen(flines[i])) / 2;
	wmvprint(w, y, x, "%s", flines[i]);
    	++y;
    }

    x = (ncols - strlen(msg)) / 2;
    wmvprint(w, y + 2, x, "%s ", msg);

    touchwin(w);
    wrefresh(w);

    readc();
    delwin(w);
    oksusp(so);

    touchwin(stdscr);
    refresh();
}

static int
treadc()
{
    char c;
    fd_set rfd;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 1000000/10;

    FD_ZERO(&rfd);
    FD_SET(0, &rfd);

    if (select(1, &rfd, 0, 0, &tv) == 1) {
	if (read(0, &c, 1) == 1)
	    return(c);
    }
    return(-1);
}

int
readc()
{
    static int lastc = -1;

    char c;

    if (lastc != -1) {
	c = lastc;
	lastc = -1;
	return(c);
    }

    for (;;) {
top:
	if (read(0, &c, 1) != 1)
	    return(0);
	switch(c) {
	case '\033': {
	    int rs = 0;

	    while ((lastc = treadc()) != -1) {
		switch(rs) {
		case 0:
		    switch(lastc) {
		    case '[':
		    	rs = 1;
			break;
		    default:
		    	return(c);
		    }
    	    	case 1:
    	    	    if (lastc >= '0' || lastc <= '9')
		    	rs = 2;
		    else
			rs = 3;
		    break;
    	    	case 2:
    	    	    if (lastc < '0' || lastc > '9') {
		    	lastc = -1;
			goto top;
    	    	    }
		    break;
    	    	case 3:
		    switch (lastc) {
    	    	    case 'A':
			lastc = -1;
			return(K_UP);
    	    	    case 'B':
			lastc = -1;
			return(K_DOWN);
    	    	    case 'C':
			lastc = -1;
			return(K_RIGHT);
    	    	    case 'D':
			lastc = -1;
			return(K_LEFT);
    	    	    default:
			lastc = -1;
			break;
		    }
		    goto top;
    	    	}
    	    }
	    return(c);
	    break;
    	  }
	case 0:
	    break;
	case '?':
	    showhelp();
	    break;
	default:
	    return(c);
	}
    }
}
