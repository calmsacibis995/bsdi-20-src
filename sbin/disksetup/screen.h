/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI screen.h,v 2.1 1995/02/03 07:16:01 polk Exp	*/

#include <string.h>

extern "C" {
#include "curses.h"
};  

#define BOX_CHAR        '#'

#define K_UP    0x101       
#define K_DOWN  0x102
#define K_RIGHT 0x103
#define K_LEFT  0x104

struct Home {
    static int homex;     
    static int homey;
    int ox, oy;
    Home()      { ox = homex; oy = homey; }
    ~Home()     { homex = ox; homey = oy; }
    
    static Set(int y, int x)
                { homex = x; homey = y; }
    static Go() { move(homey, homex); }
};  

extern "C" void wmvprint(WINDOW *, int, int, char *, ...);
extern "C" void mvprint(int, int, char *, ...);
extern "C" void print(char *, ...);
extern "C" void startvisual();
extern "C" void endvisual();

inline void
wcprint(WINDOW *w, int line, char *s)
{
    wmvprint(w, line, (76 - strlen(s))/2, s);
}

extern int suspokay;

inline int
nosusp()
{
    int i = suspokay;
    suspokay = 0;
    return(i);
}

inline void
oksusp(int i)
{
    suspokay = i;
}

extern int truth(char *, int);
extern int readc();
extern int verify(char *, ...);
extern int verify(char **, ...);
extern void inform(char *, ...);
extern void inform(char **, ...);

extern void print (char **);
extern int request_yesno(char **, int);
extern int request_number(char **, int, int = 0, int = 0);
extern int request_inform(char **);
extern char *request_string(char **, char * = 0);
extern "C" void cinform(char *);
