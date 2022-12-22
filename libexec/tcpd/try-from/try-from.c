 /*
  * This program can be called via a remote shell command to find out if the
  * hostname and address are properly recognized, if username lookup works,
  * and (SysV only) if the TLI on top of IP heuristics work.
  * 
  * Example: "rsh host /some/where/try-from".
  * 
  * Diagnostics are reported through syslog(3) and redirected to stderr.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) try-from.c 1.1 93/09/23 22:03:07";
#endif

/* System libraries. */

#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>

#ifdef TLI
#include <sys/tiuser.h>
#include <stropts.h>
#endif

/* Local stuff. */

#include "log_tcp.h"

main(argc, argv)
int     argc;
char  **argv;
{
    struct client_info client;
    char    buf[BUFSIZ];

#ifdef LOG_MAIL
    (void) openlog(argv[0], LOG_PID, FACILITY);
#else
    (void) openlog(argv[0], LOG_PID);
#endif

    /*
     * Turn on the "IP-underneath-TLI" detection heuristics.
     */
#ifdef TLI
    if (ioctl(0, I_FIND, "timod") == 0)
	ioctl(0, I_PUSH, "timod");
#endif						/* TLI */

    /*
     * Look up the remote hostname and address.
     */
    (void) fromhost(&client);

    /*
     * Perform remote username lookups when possible.
     */
    if (client.user[0] == 0 && RFC931_POSSIBLE(&client))
	client.user = rfc931(client.rmt_sin, client.our_sin);

    /*
     * Show some results.
     */

#define EXPAND_X(str) \
	(percent_x(buf, sizeof(buf), str, (char *) 0, &client, 0), buf)

    printf("%s %s\n", "address  (%a):", EXPAND_X("%a"));
    printf("%s %s\n", "hostname (%h):", EXPAND_X("%h"));
    printf("%s %s\n", "username (%u):", EXPAND_X("%u"));
    printf("%s %s\n", "hostsinfo(%c):", EXPAND_X("%c"));

    return (0);
}
