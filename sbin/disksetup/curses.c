/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI curses.c,v 2.1 1995/02/03 07:14:02 polk Exp	*/

#include <stdio.h>
#include <termios.h>
#include "curses.h"
#include <stdarg.h>
#include <sys/signal.h>

struct sigaction	act_intr;
struct sigaction	act_bail;

struct sigaction	oact_int;
struct sigaction	oact_quit;
struct sigaction	oact_hup;
struct sigaction	oact_term;
struct sigaction	oact_tstp;

int invisual = 0;
int suspokay = 1;

void endvisual();
void startvisual();

static void
intrvisual()
{
    endvisual();
    exit(1);
}

static void
bailvisual(int sig)
{
    if (invisual) {
	endvisual();
	resetty();
    }
    printf("Aborting due to signal %d\n", sig);
    printf("Please save diksetup.core and contact BSDI support\n");
    abort();
}

void
startvisual()
{
    struct termios tios;
    sigset_t sig_old;

    sigemptyset(&act_intr.sa_mask);
    sigemptyset(&act_bail.sa_mask);

    sigaddset(&act_intr.sa_mask, SIGINT);
    sigaddset(&act_intr.sa_mask, SIGQUIT);
    sigaddset(&act_intr.sa_mask, SIGHUP);
    sigaddset(&act_intr.sa_mask, SIGTERM);

    sigprocmask(SIG_BLOCK, &act_intr.sa_mask, &sig_old);

    act_intr.sa_handler = intrvisual;
    act_bail.sa_handler = bailvisual;

    act_intr.sa_flags = 0;
    act_bail.sa_flags = 0;

    sigaction(SIGINT, &act_intr, &oact_int);
    sigaction(SIGQUIT, &act_intr, &oact_quit);
    sigaction(SIGHUP, &act_intr, &oact_hup);
    sigaction(SIGTERM, &act_intr, &oact_term);

    sigaction(SIGILL, &act_bail, 0);
    sigaction(SIGEMT, &act_bail, 0);
    sigaction(SIGFPE, &act_bail, 0);
    sigaction(SIGBUS, &act_bail, 0);
    sigaction(SIGSEGV, &act_bail, 0);
    sigaction(SIGSYS, &act_bail, 0);

    savetty();
    invisual = 1;
    initscr();

    tcgetattr(0, &tios);
    tios.c_cc[VMIN] = 1;
    tios.c_cc[VTIME] = 0;
    tios.c_lflag &= ~(ECHO|ECHONL|ICANON);
    tcsetattr(0, TCSANOW, &tios);

    sigprocmask(SIG_SETMASK, &sig_old, 0);
}

void
endvisual()
{
    sigset_t sig_old;

    sigprocmask(SIG_BLOCK, &act_intr.sa_mask, &sig_old);

    endwin();
    resetty();
    invisual = 0;
    printf("\n\n");

    sigaction(SIGINT, &oact_int, 0);
    sigaction(SIGQUIT, &oact_quit, 0);
    sigaction(SIGHUP, &oact_hup, 0);
    sigaction(SIGTERM, &oact_term, 0);

    sigprocmask(SIG_SETMASK, &sig_old, 0);
}

void
mvprint(int y, int x, char *fmt, ...)
{
    char buf[128];
    va_list ap;

    va_start(ap, fmt);

    move(y, x);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    printw(buf);

    va_end(ap);
}

void
wmvprint(WINDOW *w, int y, int x, char *fmt, ...)
{
    char buf[128];
    va_list ap;

    va_start(ap, fmt);

    wmove(w, y, x);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    wprintw(w, buf);

    va_end(ap);
}

void
print(char *fmt, ...)
{
    char buf[128];
    va_list ap;

    va_start(ap, fmt);

    vsnprintf(buf, sizeof(buf), fmt, ap);
    printw(buf);

    va_end(ap);
}

#if 0
#undef move
void
move(int y, int x)
{
    wmove(stdscr, y, x);
}

#undef clear
void
clear()
{
    wclear(stdscr);
}

#undef refresh
void
refresh()
{
    wrefresh(stdscr);
}

#undef clrtobot
void
clrtobot()
{
    wclrtobot(stdscr);
}

#undef clrtoeol
void
clrtoeol()
{
    wclrtoeol(stdscr);
}

#undef standout
void
standout()
{
    wstandout(stdscr);
}

#undef standend
void
standend()
{
    wstandend(stdscr);
}
#endif
