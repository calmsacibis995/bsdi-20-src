 /*
  * percent_x() takes a string and performs %a (host address), %c (client
  * info), %h (host name or address), %d (daemon name), %p (process id) and
  * %u (user name) substitutions. It aborts the program when the result of
  * expansion would overflow the output buffer. Because the result of %<char>
  * expansion is typically passed on to a shell process, characters that may
  * confuse the shell are replaced by underscores.
  * 
  * Diagnostics are reported through syslog(3).
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) percent_x.c 1.3 93/09/11 20:45:38";
#endif

/* System libraries. */

#include <stdio.h>
#include <syslog.h>

extern char *strncpy();
extern char *strchr();
extern void exit();

/* Local stuff. */

#include "log_tcp.h"

/* percent_x - do %<char> expansion, abort if result buffer is too small */

void    percent_x(result, result_len, str, daemon, client, pid)
char   *result;
int     result_len;
char   *str;
char   *daemon;
struct client_info *client;
int     pid;
{
    char   *end = result + result_len - 1;	/* end of result buffer */
    char   *expansion;
    int     expansion_len;
    char    pid_buf[10];
    static char ok_chars[] = "1234567890!@%-_=+\\:,./\
abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char   *cp;

    /*
     * %a becomes the client address; %c all user and host information we
     * have about the client; %d the daemon process name; %h the client host
     * name or address; %p the daemon process id; %u the remote user name; %%
     * becomes a %, and %other is ignored. We terminate with a diagnostic if
     * we would overflow the result buffer. Characters that may confuse the
     * shell are mapped to underscores.
     */

    while (*str) {
	if (*str == '%') {
	    str++;
	    expansion =
		*str == 'a' ? (str++, client->addr) :
		*str == 'c' ? (str++, hosts_info(client)) :
		*str == 'd' ? (str++, daemon) :
		*str == 'h' ? (str++, FROM_HOST(client)) :
		*str == 'p' ? (str++, sprintf(pid_buf, "%d", pid), pid_buf) :
		*str == 'u' ? (str++, client->user) :
		*str == '%' ? (str++, "%") :
		*str == 0 ? "" : (str++, "");
	    expansion_len = strlen(expansion);
	    for (cp = expansion; *cp; cp++)
		if (strchr(ok_chars, *cp) == 0)
		    *cp = '_';
	} else {
	    expansion = str++;
	    expansion_len = 1;
	}
	if (result + expansion_len >= end) {
	    syslog(LOG_ERR, "shell command too long: %30s...", result);
	    exit(0);
	}
	strncpy(result, expansion, expansion_len);
	result += expansion_len;
    }
    *result = 0;
}
