/*	BSDI	fakelog.c,v 2.1 1995/02/03 06:53:03 polk Exp	*/

 /*
  * This module intercepts syslog() library calls and redirects their output
  * to the standard error stream. For interactive testing. Not for critical
  * applications, because it will happily write beyond the end of buffers.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) fakelog.c 1.2 93/09/25 12:56:20";
#endif

#include <stdio.h>

 /*
  * What follows is an attempt to unify varargs.h and stdarg.h. I'd rather
  * have this than #ifdefs all over the code.
  */

#ifdef __STDC__
#include <stdarg.h>
#define VARARGS(func,type,arg) func(type arg, ...)
#define VASTART(ap,type,name)  va_start(ap,name)
#define VAEND(ap)              va_end(ap)
#else
#include <varargs.h>
#define VARARGS(func,type,arg) func(va_alist) va_dcl
#define VASTART(ap,type,name)  {type name; va_start(ap); name = va_arg(ap, type)
#define VAEND(ap)              va_end(ap);}
#endif

extern int errno;
/* extern char *sys_errlist[]; */
extern int sys_nerr;
extern char *strcpy();

/* percentm - replace %m by error message associated with value in err */

char   *percentm(buf, str, err)
char   *buf;
char   *str;
int     err;
{
    char   *ip = str;
    char   *op = buf;

    while (*ip) {
	switch (*ip) {
	case '%':
	    switch (ip[1]) {
	    case '\0':				/* don't fall off end */
		*op++ = *ip++;
		break;
	    case 'm':				/* replace %m */
		if (err < sys_nerr && err > 0)
		    strcpy(op, sys_errlist[err]);
		else
		    sprintf(op, "Unknown error %d", err);
		op += strlen(op);
		ip += 2;
		break;
	    default:				/* leave %<any> alone */
		*op++ = *ip++, *op++ = *ip++;
		break;
	    }
	default:
	    *op++ = *ip++;
	}
    }
    *op = 0;
    return (buf);
}

/* openlog - dummy */

/* ARGSUSED */

openlog(name, logopt, facility)
char   *name;
int     logopt;
int     facility;
{
    /* void */
}

/* syslog - append record to system log -- not */

/* VARARGS */

VARARGS(syslog, int, severity)
{
    va_list ap;
    char   *fmt;
    int     err = errno;
    char    buf[BUFSIZ];

    VASTART(ap, int, severity);
    fmt = va_arg(ap, char *);
    fprintf(stderr, " ");
    vfprintf(stderr, percentm(buf, fmt, err), ap);
    fprintf(stderr, "\n");
    VAEND(ap);
}

/* closelog - dummy */

closelog()
{
    /* void */
}
