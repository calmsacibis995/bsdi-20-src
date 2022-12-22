 /*
  * try - program to try out host access-control tables, including the
  * optional shell commands and the optional language extensions.
  * 
  * usage: try [-d] process_name [user@]host_name_or_address
  * 
  * where process_name is a daemon process name (argv[0] value). If a host name
  * is specified, both name and address will be checked against the address
  * control tables. If a host address is specified, the program pretends that
  * host name lookup failed.
  * 
  * The -d option forces the program to use the access control tables in the
  * current directory.
  * 
  * All errors are written to the standard error stream, including the errors
  * that would normally be reported via the syslog daemon.
  */

#ifndef lint
static char sccsid[] = "@(#) try.c 1.11 94/03/23 17:03:14";
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>
#include <setjmp.h>

extern void exit();
extern char *strchr();
extern char *strcpy();

#ifndef HOSTS_ACCESS

main()
{
    fprintf(stderr, "host access control is not enabled.\n");
    return (1);
}

#else

#ifndef	INADDR_NONE
#define	INADDR_NONE	(-1)		/* XXX should be 0xffffffff */
#endif

#include "log_tcp.h"
#include "options.h"

int     allow_severity = SEVERITY;	/* run-time adjustable */
int     deny_severity = LOG_WARNING;	/* ditto */
int     rfc931_timeout = RFC931_TIMEOUT;/* ditto */

/* usage - explain */

void    usage(myname)
char   *myname;
{
    fprintf(stderr,
	    "usage: %s [-d] process_name [user@]host_name_or_address\n",
	    myname);
    exit(1);
}

/* Try out a (daemon,client) pair */

void    try(daemon, name, addr, user)
char   *daemon;
char   *name;
char   *addr;
char   *user;
{
    int     verdict;

    /*
     * The dry_run flag informs the optional extension language routines that
     * they are being run in verification mode, and that they should not
     * perform any real action. Extension language routines that would not
     * return should inform us of their plan, by clearing the dry_run flag.
     * This is a bit clumsy but we must be able to verify hosts with more
     * than one network address; just terminating the program isn't
     * acceptable.
     */

    dry_run = 1;

    /* Reset other stuff that might be changed by options handlers. */

    rfc931_timeout = RFC931_TIMEOUT;
    allow_severity = SEVERITY;
    deny_severity = LOG_WARNING;

    printf(" Daemon:   %s\n", daemon);
    printf(" Hostname: %s\n", name);
    printf(" Address:  %s\n", addr);

    if (user[0] && strcasecmp(user, FROM_UNKNOWN))
	printf(" Username: %s\n", user);

    verdict = hosts_ctl(daemon, name, addr, user);

    printf(" Access:   %s\n",
	   dry_run == 0 ? "delegated" :
	   verdict ? "granted" : "denied");
}

int     main(argc, argv)
int     argc;
char  **argv;
{
    struct hostent *hp;
    char   *myname = argv[0];
    char   *client;
    char   *server;
    char   *at;
    char   *user;
    char   *host;
    char    reverse_name[BUFSIZ];
    struct in_addr addr;

    /*
     * Parse the JCL.
     */
    while (--argc && *++argv && **argv == '-') {
	if (strcmp(*argv, "-d") == 0) {		/* use tables in . */
	    hosts_allow_table = "hosts.allow";
	    hosts_deny_table = "hosts.deny";
	} else {
	    usage(myname);
	}
    }
    if (argc != 2)
	usage(myname);

    server = argv[0];
    client = argv[1];

    /*
     * Default is to specify just a host name or address. If user@host is
     * specified, separate the two parts.
     */
    if ((at = strchr(client, '@')) != 0) {
	user = client;
	*at = 0;
	host = at + 1;
    } else {
	user = FROM_UNKNOWN;
	host = client;
    }

    /*
     * Note: all syslog stuff will be going to stderr, so the next calls are
     * entirely superfluous.
     */
#ifdef LOG_MAIL
    openlog(argv[0], LOG_PID, FACILITY);
#else
    openlog(argv[0], LOG_PID);
#endif

    /*
     * If a host address is specified, we simulate the effect of host name
     * lookup failures.
     */
    if (inet_addr(host) != INADDR_NONE) {
	try(server, FROM_UNKNOWN, host, user);
	return (0);
    }

    /*
     * Otherwise, assume that a host name is specified, and insist that the
     * address is known. The reason is that in real life, the host address is
     * always available (at least with IP).
     */
    if ((hp = gethostbyname(host)) == 0) {
	fprintf(stderr, "host %s: address lookup failed\n", host);
	return (1);
    }
    if (hp->h_addrtype != 0 && hp->h_addrtype != AF_INET) {
	fprintf(stderr,
		"Sorry, this test program cannot handle address family %d\n",
		hp->h_addrtype);
	return (1);
    }
    memcpy((char *) &addr, hp->h_addr_list[0], sizeof(addr));

    /*
     * Use the hostname that gethostbyaddr() would give us. On systems with
     * NIS this may be an unqualified name, even when an FQDN was given on
     * the command line.
     */
    if ((hp = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET)) == 0) {
	fprintf(stderr, "host %s: address->name lookup failed\n", host);
	return (1);
    }
    strcpy(reverse_name, hp->h_name);
    while ((hp = gethostbyname(reverse_name)) == 0)	/* XXX */
	/* void */ ;

    /*
     * Iterate over all known addresses for this host. This way we find out
     * if different addresses for the same host have different permissions,
     * something that we may not want.
     */
    while (hp->h_addr_list[0]) {
	try(server, reverse_name,
	    inet_ntoa(*(struct in_addr *) * hp->h_addr_list++), user);
	if (hp->h_addr_list[0])
	    putchar('\n');
    }
    return (0);
}

/* dummy function to intercept the real shell_cmd() */

void    shell_cmd(cmd, daemon, client)
char   *cmd;
char   *daemon;
struct client_info *client;
{
    char    buf[BUFSIZ];
    int     pid = getpid();

    percent_x(buf, sizeof(buf), cmd, daemon, client, pid);
    printf(" Command:  %s\n", buf);
}

/* dummy function  to intercept the real clean_exit() */

/* ARGSUSED */

void    clean_exit(client)
struct client_info *client;
{
    exit(0);
}

/* dummy function  to intercept the real rfc931() */

/* ARGSUSED */

char   *rfc931(rmt_sin, our_sin)
struct sockaddr_in *rmt_sin;
struct sockaddr_in *our_sin;
{
    fprintf(stderr, "Oops - cannot do username lookups in verification mode\n");
    return (0);
}

#endif
