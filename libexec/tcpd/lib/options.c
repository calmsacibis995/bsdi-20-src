 /*
  * General skeleton for adding options to the access control language. The
  * features offered by this module are documented in the hosts_options(5)
  * manual page (source file: hosts_options.5, "nroff -man" format).
  * 
  * Notes and warnings for those who want to add features:
  * 
  * In case of errors, abort options processing and deny access. There are too
  * many irreversible side effects to make error recovery feasible. For example,
  * it makes no sense to continue after we have already changed the userid.
  * 
  * In case of errors, do not terminate the process: the routines might be
  * called from a long-running daemon that should run forever.
  * 
  * In case of fatal errors, use clean_exit() instead of directly calling
  * exit(), or the inetd may loop on an UDP request.
  * 
  * In verification mode (for example, with the "try" command) the "dry_run"
  * flag is set. In this mode, an option function should just "say" what it
  * is going to do instead of really doing it.
  * 
  * Some option functions do not return (for example, the twist option passes
  * control to another program). In verification mode (dry_run flag is set)
  * such options should clear the "dry_run" flag to inform the caller of this
  * course of action.
  */

#ifndef lint
static char sccsid[] = "@(#) options.c 1.13 94/03/23 16:15:59";
#endif

/* System libraries. */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <setjmp.h>

extern char *strchr();
extern void closelog();

/* Local stuff. */

#include "log_tcp.h"
#include "options.h"

int     dry_run = 0;			/* flag set in verification mode */
jmp_buf options_buf;			/* quick way back to hosts_access() */

/* List of functions that implement the options. Add yours here. */

static void user_option();		/* execute "user=name" option */
static void group_option();		/* execute "group=name" option */
static void umask_option();		/* execute "umask=mask" option */
static void linger_option();		/* execute "linger=time" option */
static void keepalive_option();		/* execute "keepalive" option */
static void spawn_option();		/* execute "spawn=command" option */
static void twist_option();		/* execute "twist=command" option */
static void rfc931_option();		/* execute "rfc931" option */
static void setenv_option();		/* execute "setenv=name value" */
static void nice_option();		/* execute "nice" option */
static void severity_option();		/* execute "severity=value" */
static void allow_option();		/* execute "allow" option */
static void deny_option();		/* execute "deny" option */

static char *get_field();		/* chew :-delimited field off string */
static char *chop_string();		/* strip leading and trailing blanks */

/* Structure of the options table. */

struct option {
    char   *name;			/* keyword name, case is ignored */
    void  (*func) ();			/* function that does the real work */
    int     flags;			/* see below... */
};

#define NEED_ARG	(1<<1)		/* option requires argument */
#define USE_LAST	(1<<2)		/* option must be last */
#define OPT_ARG		(1<<3)		/* option has optional argument */

#define need_arg(o)	((o)->flags & NEED_ARG)
#define opt_arg(o)	((o)->flags & OPT_ARG)
#define use_last(o)	((o)->flags & USE_LAST)

/* List of known keywords. Add yours here. */

static struct option option_table[] = {
    "user", user_option, NEED_ARG,	/* switch user id */
    "group", group_option, NEED_ARG,	/* switch group id */
    "umask", umask_option, NEED_ARG,	/* change umask */
    "linger", linger_option, NEED_ARG,	/* change socket linger time */
    "keepalive", keepalive_option, 0,	/* set socket keepalive option */
    "spawn", spawn_option, NEED_ARG,	/* spawn shell command */
    "twist", twist_option, NEED_ARG | USE_LAST,	/* replace current process */
    "rfc931", rfc931_option, OPT_ARG,	/* do RFC 931 lookup */
    "setenv", setenv_option, NEED_ARG,	/* update environment */
    "nice", nice_option, OPT_ARG,	/* change nice value */
    "severity", severity_option, NEED_ARG,	/* adjust logging level */
    "allow", allow_option, USE_LAST,	/* grant access */
    "deny", deny_option, USE_LAST,	/* deny access */
    0,
};

static char whitespace[] = " \t\r\n";

/* process_options - process access control options */

void    process_options(options, daemon, client)
char   *options;
char   *daemon;
struct client_info *client;
{
    char   *key;
    char   *value;
    char   *curr_opt;
    char   *next_opt;
    struct option *op;

    /*
     * Light-weight parser. Being easy to comprehend is more important than
     * being smart.
     */

    for (curr_opt = get_field(options); curr_opt; curr_opt = next_opt) {
	next_opt = get_field((char *) 0);

	/*
	 * Separate the option into name and value parts.
	 */

	if (value = strchr(curr_opt, '=')) {	/* name=value */
	    *value++ = 0;
	    value = chop_string(value);		/* strip blanks around value */
	    if (*value == 0)
		value = 0;			/* no value left */
	}
	key = chop_string(curr_opt);		/* strip blanks around key */

	/*
	 * Disallow missing option names (and empty option fields).
	 */

	if (*key == 0) {
	    syslog(LOG_ERR, "error: %s, line %d: missing option name",
		   hosts_access_file, hosts_access_line);
	    longjmp(options_buf, OPT_DENY);
	}

	/*
	 * Lookup the option-specific info and do some common error checks.
	 * Delegate option-specific processing to the specific fuctions.
	 */

	for (op = option_table; op->name; op++)	/* find keyword */
	    if (strcasecmp(op->name, key) == 0)
		break;
	if (op->name == 0) {
	    syslog(LOG_ERR, "error: %s, line %d: bad option name or syntax: \"%s\"",
		   hosts_access_file, hosts_access_line, key);
	    longjmp(options_buf, OPT_DENY);
	} else if (!value && need_arg(op)) {
	    syslog(LOG_ERR, "error: %s, line %d: option \"%s\" requires value",
		   hosts_access_file, hosts_access_line, key);
	    longjmp(options_buf, OPT_DENY);
	} else if (value && !need_arg(op) && !opt_arg(op)) {
	    syslog(LOG_ERR, "error: %s, line %d: option \"%s\" cannot have value",
		   hosts_access_file, hosts_access_line, key);
	    longjmp(options_buf, OPT_DENY);
	} else if (next_opt && use_last(op)) {
	    syslog(LOG_ERR, "error: %s, line %d: option \"%s\" must be last option",
		   hosts_access_file, hosts_access_line, key);
	    longjmp(options_buf, OPT_DENY);
	} else {
	    (*(op->func)) (value, daemon, client);
	}
    }
}

/* allow_option - grant access */

/* ARGSUSED */

static void allow_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    if (dry_run)
	syslog(LOG_DEBUG, "option:   allow");
    longjmp(options_buf, OPT_ALLOW);
}

/* deny_option - deny access */

/* ARGSUSED */

static void deny_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    if (dry_run)
	syslog(LOG_DEBUG, "option:   deny");
    longjmp(options_buf, OPT_DENY);
}

/* user_option - switch user id */

/* ARGSUSED */

static void user_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    struct passwd *pwd;
    struct passwd *getpwnam();

    if ((pwd = getpwnam(value)) == 0) {
	syslog(LOG_ERR, "error: %s, line %d: unknown user: \"%s\"",
	       hosts_access_file, hosts_access_line, value);
	longjmp(options_buf, OPT_DENY);
    }
    endpwent();

    if (dry_run) {
	syslog(LOG_DEBUG, "option:   user = %s", value);
	return;
    }
    if (setuid(pwd->pw_uid)) {
	syslog(LOG_ERR, "error: %s, line %d: setuid(%s): %m",
	       hosts_access_file, hosts_access_line, value);
	longjmp(options_buf, OPT_DENY);
    }
}

/* group_option - switch group id */

/* ARGSUSED */

static void group_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    struct group *grp;
    struct group *getgrnam();

    if ((grp = getgrnam(value)) == 0) {
	syslog(LOG_ERR, "error: %s, line %d: unknown group: \"%s\"",
	       hosts_access_file, hosts_access_line, value);
	longjmp(options_buf, OPT_DENY);
    }
    endgrent();

    if (dry_run) {
	syslog(LOG_DEBUG, "option:   group = %s", value);
	return;
    }
    if (setgid(grp->gr_gid)) {
	syslog(LOG_ERR, "error: %s, line %d: setgid(%s): %m",
	       hosts_access_file, hosts_access_line, value);
	longjmp(options_buf, OPT_DENY);
    }
}

/* umask_option - set file creation mask */

/* ARGSUSED */

static void umask_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    unsigned mask;
    char    junk;

    if (sscanf(value, "%o%c", &mask, &junk) != 1 || (mask & 0777) != mask) {
	syslog(LOG_ERR, "error: %s, line %d: bad umask value: \"%s\"",
	       hosts_access_file, hosts_access_line, value);
	longjmp(options_buf, OPT_DENY);
    }
    if (dry_run) {
	syslog(LOG_DEBUG, "option:   umask = %o", mask);
	return;
    }
    (void) umask(mask);
}

/* spawn_option - spawn a shell command and wait */

static void spawn_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    char    buf[BUFSIZ];
    int     pid = getpid();

    if (dry_run) {
	percent_x(buf, sizeof(buf), value, daemon, client, pid);
	syslog(LOG_DEBUG, "option:   spawn = %s", buf);
	return;
    }
    shell_cmd(value, daemon, client);
}

/* linger_option - set the socket linger time (Marc Boucher <marc@cam.org>) */

/* ARGSUSED */

static void linger_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
#if defined(SO_LINGER) && !defined(BROKEN_SO_LINGER)	/* broken linux */
    struct linger linger;
    char    junk;

    if (sscanf(value, "%d%c", &linger.l_linger, &junk) != 1
	|| linger.l_linger < 0) {
	syslog(LOG_ERR, "error: %s, line %d: bad linger value: \"%s\"",
	       hosts_access_file, hosts_access_line, value);
	longjmp(options_buf, OPT_DENY);
    }
    if (dry_run) {
	syslog(LOG_DEBUG, "option:   linger = %d", linger.l_linger);
	return;
    }
    linger.l_onoff = (linger.l_linger != 0);
    if (setsockopt(client->fd, SOL_SOCKET, SO_LINGER, (char *) &linger, 
	sizeof(linger)) < 0) {
	syslog(LOG_ERR, "error: %s, line %d: setsockopt SO_LINGER %d: %m",
	       hosts_access_file, hosts_access_line, linger.l_linger);
	longjmp(options_buf, OPT_DENY);
    }
#else
    syslog(LOG_ERR, "error: %s, line %d: SO_LINGER not supported",
	   hosts_access_file, hosts_access_line);
    longjmp(options_buf, OPT_DENY);
#endif
}

/* keepalive_option - set the socket keepalive option */

/* ARGSUSED */

static void keepalive_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
#if defined(SO_KEEPALIVE) && !defined(BROKEN_SO_KEEPALIVE)
    int     on = 1;

    if (dry_run) {
	syslog(LOG_DEBUG, "option:   keepalive");
	return;
    }
    if (setsockopt(client->fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &on,
		   sizeof(on)) < 0)
	syslog(LOG_WARNING, "warning: %s, line %d: setsockopt SO_KEEPALIVE: %m",
	       hosts_access_file, hosts_access_line);
#else
    syslog(LOG_WARNING, "warning: %s, line %d: SO_KEEPALIVE not supported",
	   hosts_access_file, hosts_access_line);
#endif
}

/* nice_option - set nice value */

/* ARGSUSED */

static void nice_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    int     niceval = 10;
    char    junk;

    if (value != 0 && sscanf(value, "%d%c", &niceval, &junk) != 1) {
	syslog(LOG_ERR, "error: %s, line %d: bad nice value: \"%s\"",
	       hosts_access_file, hosts_access_line, value);
	longjmp(options_buf, OPT_DENY);
    }
    if (dry_run) {
	syslog(LOG_DEBUG, "option:   nice = %d", niceval);
	return;
    }
    if (nice(niceval) < 0) {
	syslog(LOG_WARNING, "warning: %s, line %d: nice(%d): %m",
	       hosts_access_file, hosts_access_line, niceval);
    }
}

/* maybe_dup2 - conditional dup2 */

static int maybe_dup2(fd1, fd2)
int fd1;
int fd2;
{
    if (fd1 == fd2) {				/* already OK */
	return (fd2);
    } else {					/* dup new to old */
	close(fd2);
	return (dup(fd1));
    }
}

/* twist_option - replace process by shell command */

static void twist_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    char    buf[BUFSIZ];
    int     pid = getpid();
    char   *error;

    percent_x(buf, sizeof(buf), value, daemon, client, pid);

    if (dry_run) {
	syslog(LOG_DEBUG, "option:   twist = %s", buf);
	dry_run = 0;
	return;
    }
    syslog(deny_severity, "twist %s to %s", hosts_info(client), buf);
    closelog();

    /* Before switching to the shell, set up stdin, stdout and stderr. */

    if (maybe_dup2(client->fd, 0) != 0 ||
	maybe_dup2(client->fd, 1) != 1 ||
	maybe_dup2(client->fd, 2) != 2) {
	error = "twist_option: dup: %m";
    } else {
	if (client->fd > 2)
	    close(client->fd);
	(void) execl("/bin/sh", "sh", "-c", buf, (char *) 0);
	error = "twist_option: /bin/sh: %m";
    }

    /* Can get here only in case of errors. */

#ifdef LOG_MAIL
    (void) openlog(daemon, LOG_PID, FACILITY);
#else
    (void) openlog(daemon, LOG_PID);
#endif
    syslog(LOG_ERR, error);

    /* We MUST terminate the process. */

    clean_exit(client);
}

/* rfc931_option - look up remote user name */

/* ARGSUSED */

static void rfc931_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    int     timeout;
    char    junk;

    if (value) {
	if (sscanf(value, "%d%c", &timeout, &junk) != 1 || timeout <= 0) {
	    syslog(LOG_ERR, "error: %s, line %d: bad rfc931 timeout: \"%s\"",
		   hosts_access_file, hosts_access_line, value);
	    longjmp(options_buf, OPT_DENY);
	}
	rfc931_timeout = timeout;
    }
    if (dry_run) {
	syslog(LOG_DEBUG, "option:   rfc931 = %d", rfc931_timeout);
	return;
    }
    if (client->user[0] == 0 && RFC931_POSSIBLE(client))
	client->user = rfc931(client->rmt_sin, client->our_sin);
}

/* setenv_option - set environment variable */

/* ARGSUSED */

static void setenv_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    char   *var_name;
    char   *var_value;
    char    buf[BUFSIZ];
    int     pid;

    /*
     * Separate the argument into a name and value part.
     */

    var_value = value + strcspn(value, whitespace);

    if (*var_value == 0) {			/* just a name, that's all */
	var_name = value;
    } else {					/* expand %stuff in value */
	*var_value++ = 0;
	var_name = chop_string(value);
	pid = getpid();
	percent_x(buf, sizeof(buf), var_value, daemon, client, pid);
	var_value = chop_string(buf);
    }
    if (dry_run) {
	syslog(LOG_DEBUG, "option:   setenv = %s %s", var_name, var_value);
	return;
    }
    if (setenv(var_name, var_value, 1)) {
	syslog(LOG_ERR, "error: %s, line %d: memory allocation failure",
	       hosts_access_file, hosts_access_line);
	longjmp(options_buf, OPT_DENY);
    }
}

/* get_field - return pointer to next field in string */

static char *get_field(string)
char   *string;
{
    static char *last = "";
    char   *src;
    char   *dst;
    char   *ret;

    /*
     * This function returns pointers to successive fields within a given
     * string. ":" is the field separator; warn if the rule ends in one. It
     * replaces a "\:" sequence by ":", without treating the result of
     * substitution as field terminator. A null argument means resume search
     * where the previous call terminated. This function destroys its
     * argument.
     */

    /*
     * Work from explicit source or from memory.
     */

    if (string == 0)
	string = last;
    if (string[0] == 0)
	return (0);

    /*
     * While processing \: we overwrite the input. This way we do not have to
     * maintain buffers for copies of input fields.
     */

    src = dst = ret = string;

    for (;;) {
	switch (*src) {
	case '\\':				/* handle escape */
	    switch (src[1]) {
	    case ':':				/* convert \: to : */
		src++;
		/* FALLTHROUGH */
	    case '\0':				/* don't run off end */
		*dst++ = *src++;
		break;
	    default:				/* copy \other verbatim */
		*dst++ = *src++, *dst++ = *src++;
		break;
	    }
	    break;
	case ':':				/* field separator */
	    src++;
	    if (*src == 0)
		syslog(LOG_WARNING, "warning: %s, line %d: rule ends in \":\"",
		       hosts_access_file, hosts_access_line);
	    /* FALLTHROUGH */
	case '\0':				/* end of string */
	    last = src;
	    *dst = 0;
	    return (ret);
	default:				/* anything else */
	    *dst++ = *src++;
	    break;
	}
    }
}

/* chop_string - strip leading and trailing blanks from string */

static char *chop_string(start)
register char *start;
{
    register char *end;

    while (*start && isspace(*start))
	start++;

    for (end = start + strlen(start); end > start && isspace(end[-1]); end--)
	 /* void */ ;
    *end = 0;

    return (start);
}

 /*
  * The severity option goes last because it comes with a huge amount of ugly
  * #ifdefs and tables.
  */

struct syslog_names {
    char   *name;
    int     value;
};

static struct syslog_names log_facilities[] = {
#ifdef LOG_KERN
    "kern", LOG_KERN,
#endif
#ifdef LOG_USER
    "user", LOG_USER,
#endif
#ifdef LOG_MAIL
    "mail", LOG_MAIL,
#endif
#ifdef LOG_DAEMON
    "daemon", LOG_DAEMON,
#endif
#ifdef LOG_AUTH
    "auth", LOG_AUTH,
#endif
#ifdef LOG_LPR
    "lpr", LOG_LPR,
#endif
#ifdef LOG_NEWS
    "news", LOG_NEWS,
#endif
#ifdef LOG_UUCP
    "uucp", LOG_UUCP,
#endif
#ifdef LOG_CRON
    "cron", LOG_CRON,
#endif
#ifdef LOG_LOCAL0
    "local0", LOG_LOCAL0,
#endif
#ifdef LOG_LOCAL1
    "local1", LOG_LOCAL1,
#endif
#ifdef LOG_LOCAL2
    "local2", LOG_LOCAL2,
#endif
#ifdef LOG_LOCAL3
    "local3", LOG_LOCAL3,
#endif
#ifdef LOG_LOCAL4
    "local4", LOG_LOCAL4,
#endif
#ifdef LOG_LOCAL5
    "local5", LOG_LOCAL5,
#endif
#ifdef LOG_LOCAL6
    "local6", LOG_LOCAL6,
#endif
#ifdef LOG_LOCAL7
    "local7", LOG_LOCAL7,
#endif
    0,
};

static struct syslog_names log_severities[] = {
#ifdef LOG_EMERG
    "emerg", LOG_EMERG,
#endif
#ifdef LOG_ALERT
    "alert", LOG_ALERT,
#endif
#ifdef LOG_CRIT
    "crit", LOG_CRIT,
#endif
#ifdef LOG_ERR
    "err", LOG_ERR,
#endif
#ifdef LOG_WARNING
    "warning", LOG_WARNING,
#endif
#ifdef LOG_NOTICE
    "notice", LOG_NOTICE,
#endif
#ifdef LOG_INFO
    "info", LOG_INFO,
#endif
#ifdef LOG_DEBUG
    "debug", LOG_DEBUG,
#endif
    0,
};

/* severity_map - lookup facility or severity value */

static int severity_map(table, name)
struct syslog_names *table;
char   *name;
{
    struct syslog_names *t;

    for (t = table; t->name; t++) {
	if (strcasecmp(t->name, name) == 0)
	    return (t->value);
    }
    syslog(LOG_ERR,
	   "error: %s, line %d: bad syslog facility or severity: \"%s\"",
	   hosts_access_file, hosts_access_line, name);
    longjmp(options_buf, OPT_DENY);
    /* NOTREACHED */
}

/* severity_option - change logging severity for this event (Dave Mitchell) */

/* ARGSUSED */

static void severity_option(value, daemon, client)
char   *value;
char   *daemon;
struct client_info *client;
{
    int     new_severity;
    char   *dot;

    if (dot = strchr(value, '.')) {		/* facility.level */
	*dot = 0;
	new_severity = severity_map(log_facilities, chop_string(value))
	    | severity_map(log_severities, chop_string(dot + 1));
	*dot = '.';
    } else {					/* no facility, just level */
	new_severity = severity_map(log_severities, chop_string(value));
    }
    if (dry_run) {
	syslog(LOG_DEBUG, "option:   severity = %s", value);
	return;
    }
    allow_severity = deny_severity = new_severity;
}
