/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI showhelp.cc,v 2.1 1995/02/03 07:16:05 polk Exp	*/

#include "showhelp.h"
#include "screen.h"

static help_info *chelp = 0;

help_info::help_info(char **h, int l)
{
    help = h;
    if ((lines  = l) < 0)
	for (lines = 0; help[lines]; ++lines)
	    ;
    last = chelp;
    chelp = this;
}

help_info::~help_info()
{   
    chelp = last;
}

void
showhelp()
{
    static int here = 0;

    if (here || !chelp)
	return;
    here = 1;

    int so = nosusp();
    WINDOW *w = newwin(22, 76, 1, 2);

    int start = 0;
    int ostart = -1;

    for (;;) {
	if (ostart != start) {
	    ostart = start;
	    wclear(w);
	    box(w, BOX_CHAR, BOX_CHAR);

	    for (int i = start; i < chelp->lines && i < start + 17; ++i)
		wmvprint(w, i - start + 2, 2, "%.72s", chelp->help[i]);

	    if (start && start + 17 < chelp->lines)
		wcprint(w, 20, "ESC to exit help"
			       ", SPACE to go forward 1 page"
			       ", - to go backward 1 page ");
	    else if (start + 17 < chelp->lines)
		wcprint(w, 20, "ESC to exit help"
			       ", SPACE to go forward 1 page ");
	    else if (start)
		wcprint(w, 20, "ESC to exit help"
			       ", - to go backward 1 page ");
	    else
		wcprint(w, 20, "ESC to exit help ");

	    touchwin(w);
	    wrefresh(w);
	}

	switch (readc()) {
	case ' ': case '\r': case '\n':
	    if (start + 17 < chelp->lines)
		start += 17;
	    break;
	case '-':
	    if (start)
		start -= 17;
	    break;
	case '\033':
	    delwin(w);
	    oksusp(so);
	    touchwin(stdscr);
	    refresh();
	    here = 0;
	    return;
	}
    }
}
