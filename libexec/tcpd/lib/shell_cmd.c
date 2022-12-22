 /*
  * shell_cmd() takes a shell command template and performs %a (host
  * address), %c (client info), %h (host name or address), %d (daemon name),
  * %p (process id) and %u (user name) substitutions. The result is executed
  * by a /bin/sh child process, with standard input, standard output and
  * standard error connected to /dev/null.
  * 
  * Diagnostics are reported through syslog(3).
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) shell_cmd.c 1.4 93/09/11 20:45:12";
#endif

/* System libraries. */

#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <stdio.h>
#include <syslog.h>

extern char *strncpy();
extern void closelog();
extern void exit();

/* Local stuff. */

#include "log_tcp.h"

/* Forward declarations. */

static void do_child();

/* shell_cmd - expand %<char> sequences and execute shell command */

void    shell_cmd(string, daemon, client)
char   *string;
char   *daemon;
struct client_info *client;
{
    char    cmd[BUFSIZ];
    int     child_pid;
    int     wait_pid;
    int     daemon_pid = getpid();

    /*
     * Most of the work is done within the child process, to minimize the
     * risk of damage to the parent.
     */

    switch (child_pid = fork()) {
    case -1:					/* error */
	syslog(LOG_ERR, "fork: %m");
	break;
    case 00:					/* child */
	percent_x(cmd, sizeof(cmd), string, daemon, client, daemon_pid);
	do_child(daemon, cmd);
	/* NOTREACHED */
    default:					/* parent */
	while ((wait_pid = wait((int *) 0)) != -1 && wait_pid != child_pid)
	     /* void */ ;
    }
}

/* do_child - exec command with { stdin, stdout, stderr } to /dev/null */

static void do_child(myname, command)
char   *myname;
char   *command;
{
    char   *error = 0;
    int     tmp_fd;

    /*
     * SunOS 4.x may send a SIGHUP to grandchildren if the child exits first.
     * Sessions and process groups make old and grown-up programmers tear out
     * what little hair is left and run away crying.
     */

    signal(SIGHUP, SIG_IGN);

    /*
     * Close a bunch of file descriptors. The Ultrix inetd only passes stdin,
     * but other inetd implementations set up stdout as well. Ignore errors.
     */

    closelog();
    for (tmp_fd = 0; tmp_fd < 10; tmp_fd++)
	(void) close(tmp_fd);

    /* Set up new stdin, stdout, stderr, and exec the shell command. */

    if (open("/dev/null", 2) != 0) {
	error = "open /dev/null: %m";
    } else if (dup(0) != 1 || dup(0) != 2) {
	error = "dup: %m";
    } else {
	(void) execl("/bin/sh", "sh", "-c", command, (char *) 0);
	error = "execl /bin/sh: %m";
    }

    /* We can reach the following code only if there was an error. */

#ifdef LOG_MAIL
    (void) openlog(myname, LOG_PID, FACILITY);
#else
    (void) openlog(myname, LOG_PID);
#endif
    syslog(LOG_ERR, error);
    exit(0);
}
