 /*
  * These are defined here in case the application uses the pre-6.0 libwrap.a
  * interface with static logging severities.
  */

#ifndef lint
static char sccsid[] = "@(#) 6compat.c 1.1 93/09/29 08:31:33";
#endif

#include <syslog.h>

int     allow_severity = SEVERITY;	/* run-time adjustable */
int     deny_severity = LOG_WARNING;	/* ditto */
