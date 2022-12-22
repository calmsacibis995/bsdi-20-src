 /*
  * General front end for stream and datagram IP services. This program logs
  * the remote host name and then invokes the real daemon. For example,
  * install as /usr/etc/{tftpd,fingerd,telnetd,ftpd,rlogind,rshd,rexecd},
  * after saving the real daemons in the directory specified with the
  * REAL_DAEMON_DIR macro. This arrangement requires that the network daemons
  * are started by inetd or something similar. Connections and diagnostics
  * are logged through syslog(3).
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) tcpd.c 1.8 93/09/27 18:59:22";
#endif

/* System libraries. */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>

extern char *strrchr();
extern char *strcpy();

#ifndef MAXPATHNAMELEN
#define MAXPATHNAMELEN	BUFSIZ
#endif

/* Local stuff. */

#include "patchlevel.h"
#include "log_tcp.h"

int     allow_severity = SEVERITY;	/* run-time adjustable */
int     deny_severity = LOG_WARNING;	/* ditto */

main(argc, argv)
int     argc;
char  **argv;
{
    struct client_info client;
    int     from_stat;
    char    path[MAXPATHNAMELEN];

    /* Attempt to prevent the creation of world-writable files. */

#ifdef DAEMON_UMASK
    umask(DAEMON_UMASK);
#endif

    /*
     * If argv[0] is an absolute path name, ignore REAL_DAEMON_DIR, and strip
     * argv[0] to its basename.
     */

    if (argv[0][0] == '/') {
	strcpy(path, argv[0]);
	argv[0] = strrchr(argv[0], '/') + 1;
    } else {
	sprintf(path, "%s/%s", REAL_DAEMON_DIR, argv[0]);
    }

    /*
     * Open a channel to the syslog daemon. Older versions of openlog()
     * require only two arguments.
     */

#ifdef LOG_MAIL
    (void) openlog(argv[0], LOG_PID, FACILITY);
#else
    (void) openlog(argv[0], LOG_PID);
#endif

    /*
     * Find out and verify the remote host name. Sites concerned with
     * security may choose to refuse connections from hosts that pretend to
     * have someone elses host name.
     */

    from_stat = fromhost(&client);
#ifdef PARANOID
    if (from_stat == -1)
	refuse(&client);
#endif

    /*
     * The BSD rlogin and rsh daemons that came out after 4.3 BSD disallow
     * socket options at the IP level. They do so for a good reason.
     * Unfortunately, we cannot use this with SunOS 4.1.x because the
     * getsockopt() system call can panic the system.
     */

#ifdef KILL_IP_OPTIONS
    fix_options(&client);
#endif

    /*
     * Check whether this host can access the service in argv[0]. The
     * access-control code invokes optional shell commands as specified in
     * the access-control tables.
     */

#ifdef HOSTS_ACCESS
    if (!hosts_access(argv[0], &client))
	refuse(&client);
#endif

    /* Report remote client and invoke the real daemon program. */

    syslog(allow_severity, "connect from %s", hosts_info(&client));
    (void) execv(path, argv);
    syslog(LOG_ERR, "error: cannot execute %s: %m", path);
    clean_exit(&client);
    /* NOTREACHED */
}
