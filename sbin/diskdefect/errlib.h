/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*      BSDI errlib.h,v 2.1 1995/02/03 07:13:22 polk Exp */

void    Psetname(const char *name); /* used by Perror and Pwarn */
void    Perror(char *fmt, ...);     /* perror and exit(1) */
void    Pwarn(char *fmt, ...);      /* perror and continue */
void    Perrmsg(char *fmt, ...);    /* print on stderr */
void    __Pmsg(char *fmt, ...);     /* debug printf, enable with -DPDEBUG */

#ifdef PDEBUG
#define Pmsg(x) ( __Pmsg x )
#else
#define Pmsg(x) /*empty*/
#endif
