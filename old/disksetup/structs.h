/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*	BSDI structs.h,v 2.1 1995/02/03 06:40:22 polk Exp	*/

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <stdio.h>

#ifdef __STDC__
#include <stdlib.h>
#include <unistd.h>
#endif

#ifdef sun
extern char *strdup();
extern char *malloc();
extern char *ctime();
extern char *getenv();
extern char *strcpy();
extern time_t time();

#endif

extern char *progname;

#define P_EXISTED	0x1	/* partition already existed in FDISK table */

#define	LINELEN 512
extern char *disktypetostring();
extern char *systemtostring();
extern int everchanged;		/* if changes have been made */
#define TITLINE 0
#define CMDLINE 11
#define DIRLINE 16
#define FIRSTDISKLINE 3		/* first line of disk output goes here */
#define UNASSLINE  9
#define COMMANDLINE 13
#define WARNLINE    14

#define COMMANDLINE2 14
#define WARNLINE2	15
 extern int warnline;

#define FIRSTBSDLINE   3
