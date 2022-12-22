/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI field.cc,v 2.1 1995/02/03 07:14:53 polk Exp	*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "screen.h"
#include "help.h"
#include "showhelp.h"
#include "disk.h"
#include "field.h"


void
Disk::AddNumber(int &value, char *buf, int (Disk::*func)(double))
{
    if (buf[0] == '*' || buf[0] == '/') {
	int mv = func(strtod(buf+1, 0));
    	if (!mv)
	    return;
    	if (buf[0] == '*')
	    value *= mv;
    	else
	    value /= mv;
    	if (func)
	    value = func(value);
    } else if (buf[0] == '+' || buf[0] == '-') {
	value += func(strtod(buf, 0));
    } else if (buf[0] >= '0' && buf[0] <= '9') {
	value = func(strtod(buf, 0));
    }
}

int
Field::NumericEdit(int &value, int max, int (*force)(int), int cyoff, int ev)
{
    help_info hi(full_help);

    char c;
    int cx = 0;
    int tv;

    move(ESTART, 0); clrtobot();
    for (int i = 0; full_qhelp[i]; ++i)
	mvprint(ESTART+i, 4, full_qhelp[i]);

    char buf[length + 1];

    memset(buf, ' ', length);
    buf[length] = '\0';

    tv = force(value);
    standout();
    mvprint(y, x, "%*d", length, tv);
    standend();
    move(y, x);
    refresh();

    c = '\n';
    int lc;

    while ((lc = c), (c = readc())) {
    	int cy = 0;

    	if (check) {
	    int r = (*check)(c);
    	    if (r)
		return(r);
    	}
    	switch (c) {
    	case 'u': case 'U':
	    cx = 0;
	    mvprint(y, x, "%*d", length, value);
	    standend();
	    memset(buf, ' ', length);
	    buf[length] = '\0';
	    if (redisplay)
	    	redisplay(value, ev);
	    move(y, x);
	    refresh();
	    break;
	case 'x': case 'X':
	    if (max >= 0) {
		tv = max;
		sprintf(buf, "%-*ld", length, tv);
		mvprint(y, x, buf);
		cx = 0;
		while (cx < length && buf[cx] != ' ')
		    ++cx;
		if (redisplay)
		    redisplay(tv, ev);
		move(y,x + cx);
		refresh();
		break;
	    }
	    break;
    	case '\r': case '\n':
	    disk.AddNumber(tv, buf);
	    sprintf(buf, "%-*ld", length, tv);
	    mvprint(y, x, buf);
	    cx = 0;
	    while (cx < length && buf[cx] != ' ')
		++cx;
	    if (redisplay)
		redisplay(tv, ev);
	    move(y,x + cx);
	    refresh();
    	    
    	    if (lc == '\n' || lc == '\r') {
		value = force(tv);
		mvprint(y, x, "%*ld", length, value);
		return(2);
    	    }
    	    break;
    	case ('H' & 037): case 0177:
	    if (cx > 0) {
		if (cx != length - 1 || buf[cx] == ' ')
		    --cx;
		buf[cx] = ' ';
    	    	mvprint(y, x, buf);
		move(y, x + cx);
	    	refresh();
    	    }
	    break;
    	case ('W' & 037): case ('U' & 037):
    	    memset(buf, ' ', length);
	    cx = 0;
	    mvprint(y, x, buf);
	    move(y, x);
	    refresh();
	    break;
    	case 's': case 'S': case '=':
	    disk.AddNumber(tv, buf);
	    sprintf(buf, "%-*ld", length, tv);
	    mvprint(y, x, buf);
	    cx = 0;
	    while (cx < length && buf[cx] != ' ')
		++cx;
	    if (redisplay)
		redisplay(tv, ev);
	    move(y,x + cx);
	    refresh();
    	    break;
    	case 'm':
	    disk.AddNumber(tv, buf, &Disk::FromMB);

	    sprintf(buf, "%-*ld", length, tv);
	    mvprint(y, x, buf);
	    cx = 0;
	    while (cx < length && buf[cx] != ' ')
		++cx;
	    if (redisplay)
		redisplay(tv, ev);
	    move(y, x + cx);
	    refresh();
    	    break;
    	case 'M':
	    disk.AddNumber(tv, buf, &Disk::FromMB);
    	    tv = disk.FromCyl(int(disk.ToCyl(tv))) - cyoff;

	    sprintf(buf, "%-*ld", length, tv);
	    mvprint(y, x, buf);
	    cx = 0;
	    while (cx < length && buf[cx] != ' ')
		++cx;
	    if (redisplay)
		redisplay(tv, ev);
	    move(y,x + cx);
	    refresh();
    	    break;
    	case 'C':
	    cy = cyoff;
    	case 'c':
	    disk.AddNumber(tv, buf, &Disk::FromCyl);
    	    tv -= cyoff;

	    sprintf(buf, "%-*ld", length, tv);
	    mvprint(y, x, buf);
	    cx = 0;
	    while (cx < length && buf[cx] != ' ')
		++cx;
	    if (redisplay)
		redisplay(tv, ev);
	    move(y,x + cx);
	    refresh();
    	    break;
    	case 't': case 'T':
	    disk.AddNumber(tv, buf, &Disk::FromTrack);

	    sprintf(buf, "%-*ld", length, tv);
	    mvprint(y, x, buf);
	    cx = 0;
	    while (cx < length && buf[cx] != ' ')
		++cx;
	    if (redisplay)
		redisplay(tv, ev);
	    move(y,x + cx);
	    refresh();
    	    break;
    	case '\t':
	    disk.AddNumber(tv, buf);
    	    value = tv;
	    if (redisplay)
		redisplay(value, ev);
	    mvprint(y, x, "%*ld", length, value);
	    return(1);
    	case '\033':
	    if (redisplay)
		redisplay(value, ev);
	    mvprint(y, x, "%*ld", length, value);
	    return(0);
    	default:
    	    if ((c >= '0' && c <= '9') || (c == '.' && !strchr(buf, '.'))
				       || (c == '+' && cx == 0)
				       || (c == '-' && cx == 0)) {
    	    	buf[cx] = c;
    	    	if (cx < length - 1)
		    ++cx;
    	    	mvprint(y, x, buf);
		if ((c == '+' || c == '-') && redisplay)
		    redisplay(tv, ev);
    	    	move(y, x + cx);
		refresh();
    	    } else if (c == '+' || c == '-' || c == '*' || c == '/') {
		disk.AddNumber(tv, buf);
		buf[0] = c;
		memset(buf+1, ' ', length - 1);
		cx = 1;
    	    	mvprint(y, x, buf);
		if (redisplay)
		    redisplay(tv, ev);
    	    	move(y, x + cx);
		refresh();
	    }
    	    break;
    	}
	    
    }
    return(0);
}

int
Field::NumberEdit(int &value)
{
    help_info hi(full_help);

    char c;
    int cx = 0;
    int tv;

    move(ESTART, 0); clrtobot();
    for (int i = 0; full_qhelp[i]; ++i)
	mvprint(ESTART+i, 4, full_qhelp[i]);

    char buf[length + 1];

    memset(buf, ' ', length);
    buf[length] = '\0';

    tv = value;
    standout();
    mvprint(y, x, "%*d", length, tv);
    standend();
    move(y, x);
    refresh();

    int lc;
    c = '\n';

    while ((lc = c), (c = readc())) {
    	int cy = 0;

    	if (check) {
	    int r = (*check)(c);
    	    if (r)
		return(r);
    	}

    	switch (c) {
    	case 'u': case 'U':
	    cx = 0;
	    mvprint(y, x, "%*d", length, value);
	    standend();
	    memset(buf, ' ', length);
	    buf[length] = '\0';
	    move(y, x);
	    refresh();
	    break;
    	case '\r': case '\n':
	    disk.AddNumber(tv, buf);
	    sprintf(buf, "%-*ld", length, tv);
	    mvprint(y, x, buf);
	    cx = 0;
	    while (cx < length && buf[cx] != ' ')
		++cx;
	    move(y,x + cx);
	    refresh();
    	    if (lc == '\n' || lc == '\r') {
		value = tv;
		mvprint(y, x, "%*ld", length, value);
		return(2);
    	    }
    	    break;
    	case ('H' & 037): case 0177:
	    if (cx > 0) {
		if (cx != length - 1 || buf[cx] == ' ')
		    --cx;
		buf[cx] = ' ';
    	    	mvprint(y, x, buf);
		move(y, x + cx);
	    	refresh();
    	    }
	    break;
    	case ('W' & 037): case ('U' & 037):
    	    memset(buf, ' ', length);
	    cx = 0;
	    mvprint(y, x, buf);
	    move(y, x);
	    refresh();
	    break;
    	case '=':
	    disk.AddNumber(tv, buf);
	    sprintf(buf, "%-*ld", length, tv);
	    mvprint(y, x, buf);
	    cx = 0;
	    while (cx < length && buf[cx] != ' ')
		++cx;
	    move(y,x + cx);
	    refresh();
    	    break;
    	case '\t':
	    disk.AddNumber(tv, buf);
	    value = tv;
	    mvprint(y, x, "%*ld", length, value);
	    return(1);
    	case '\033':
	    mvprint(y, x, "%*ld", length, value);
	    return(0);
    	default:
    	    if ((c >= '0' && c <= '9') || (c == '.' && !strchr(buf, '.'))
				       || (c == '*' && cx == 0)
				       || (c == '/' && cx == 0)
				       || (c == '+' && cx == 0)
				       || (c == '-' && cx == 0)) {
    	    	buf[cx] = c;
    	    	if (cx < length - 1)
		    ++cx;
    	    	mvprint(y, x, buf);
    	    	move(y, x + cx);
		refresh();
    	    } else if (c == '+' || c == '-' || c == '*' || c == '/') {
		disk.AddNumber(tv, buf);
		buf[0] = c;
		memset(buf+1, ' ', length - 1);
		cx = 1;
    	    	mvprint(y, x, buf);
    	    	move(y, x + cx);
		refresh();
	    }
    	    break;
    	}
	    
    }
    return(0);
}

static int
vtrue(char *)
{
    return(1);
}

int
Field::TextEdit(char *string, int (*validate)(char *))
{
    if (!validate)
	validate = vtrue;

    help_info hi(full_help);

    char c;
    int cx = 0;

    move(ESTART, 0); clrtobot();
    for (int i = 0; full_qhelp[i]; ++i)
	mvprint(ESTART+i, 4, full_qhelp[i]);

    char buf[length + 1];

    sprintf(buf, "%-*.*s", length, length, string);

    standout();
    mvprint(y, x, buf);
    for (cx = length; cx > 0 && buf[cx-1] == ' '; --cx)
	;
    if (cx == length)
	--cx;
    move(y, x);
    cx = 0;
    standend();
    refresh();

    int lc;
    c = '\n';
    while ((lc = c), (c = readc())) {
    	if (check) {
	    int r = (*check)(c);
    	    if (r)
		return(r);
    	}
    	switch (c) {
    	case '\r': case '\n':
    	    if (cx && buf[0] != ' ') {
		if (!validate(buf))
		    break;
		if (cx) {
		    strncpy(string, buf, cx);
		    string[cx] = 0;
    	    	}
	    }
	    mvprint(y, x, "%-*.*s", length, length, string);
    	    // if (lc == '\r' || lc == '\n')
		return(2);
    	    break;
    	case ('T' & 037) :
	    showtypes();
	    break;
    	case ('H' & 037): case 0177:
	    if (cx > 0) {
		if (cx != length - 1 || buf[cx] == ' ')
		    --cx;
		buf[cx] = ' ';
    	    	mvprint(y, x, buf);
		move(y, x + cx);
	    	refresh();
    	    } else if (buf[0] != ' ') {
	    	while (cx < length && buf[cx] != ' ')
		    ++cx;
    	    	buf[--cx] = ' ';
    	    	mvprint(y, x, buf);
		move(y, x + cx);
	    	refresh();
    	    }
	    break;
    	case ('U' & 037): case ('W' &037):
	    cx = 0;
	    memset(buf, ' ', length);
	    mvprint(y, x, buf);
	    move(y, x);
	    refresh();

    	case '\t':
    	    if (cx && buf[0] != ' ') {
		if (!validate(buf))
		    break;
		strncpy(string, buf, cx);
	    	string[cx] = 0;
	    }
	    mvprint(y, x, "%-*.*s", length, length, string);
	    return(1);
    	case '\033':
	    mvprint(y, x, "%-*.*s", length, length, buf);
	    return(0);
    	default:
    	    if (cx == 0 && buf[0] != ' ')
		memset(buf, ' ', length);
    	    if (c != ' ' && isascii(c) && isprint(c)) {
    	    	buf[cx] = c;
    	    	if (cx < length - 1)
		    ++cx;
    	    	mvprint(y, x, buf);
    	    	move(y, x + cx);
		refresh();
    	    }
    	    break;
    	}
	    
    }
    return(0);
}

int
Field::LetterEdit(char &letter)
{
    help_info hi(full_help);

    char c;
    char oletter = letter;

    move(ESTART, 0); clrtobot();
    for (int i = 0; full_qhelp[i]; ++i)
	mvprint(ESTART+i, 4, full_qhelp[i]);

    standout();
    mvprint(y, x, "%c", letter);
    move(y,x);
    standend();
    refresh();

    while (c = readc()) {
    	if (check) {
	    int r = (*check)(c);
    	    if (r)
		return(r);
    	}
	if (isupper(c))
	    c = tolower(c);
    	switch (c) {
	case 'a': case 'b': case 'd': case 'e': case 'f': case 'g': case 'h':
    	    for (int i = 0; i < 8 && disk.bsd[i].number; ++i)
	    	if (disk.bsd[i].number == c - 'a' + 1)
	    	    break;
    	    if (c == oletter || i >= 8 || disk.bsd[i].number == 0) {
		letter  = c;
		mvprint(y, x, "%c", letter);
		move(y,x);
		refresh();
    	    }
	    break;

    	case '\r': case '\n':
	    mvprint(y, x, "%c", letter);
	    refresh();
	    return(2);
    	case '\t':
	    mvprint(y, x, "%c", letter);
	    refresh();
	    return(1);
    	case '\033':
	    mvprint(y, x, "%c", letter = oletter);
	    refresh();
	    return(0);
    	default:
    	    break;
    	}
	    
    }
    return(0);
}
