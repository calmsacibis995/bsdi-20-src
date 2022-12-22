 /*
  * This module implements a simple access control language that is based on
  * host (or domain) names, NIS netgroup names, IP addresses (or network
  * numbers) and daemon process names. When a match is found an optional
  * shell command is executed and the search is terminated.
  *
  * The language supports rule-driven remote username lookup via the RFC931
  * protocol. This feature is supported only for the connection-oriented TCP
  * protocol, and requires that the caller provides sockaddr_in structures
  * that describe both ends of the connection.
  * 
  * Diagnostics are reported through syslog(3).
  * 
  * Compile with -DNETGROUP if your library provides support for netgroups.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) hosts_access.c 1.16 94/02/01 22:12:05";
#endif

/* System libraries. */

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <syslog.h>
#include <ctype.h>
#include <errno.h>

extern char *fgets();
extern char *strchr();
extern char *strtok();
extern int errno;

#ifndef	INADDR_NONE
#define	INADDR_NONE	(-1)		/* XXX should be 0xffffffff */
#endif

/* Local stuff. */

#include "log_tcp.h"

#ifdef PROCESS_OPTIONS
#include <setjmp.h>
#include "options.h"
#endif

/* Delimiters for lists of daemons or clients. */

static char sep[] = ", \t";

/* Constants to be used in assignments only, not in comparisons... */

#define	YES		1
#define	NO		0
#define	FAIL		(-1)

 /*
  * These variables are globally visible so that they can be redirected in
  * verification mode.
  */

char   *hosts_allow_table = HOSTS_ALLOW;
char   *hosts_deny_table = HOSTS_DENY;

/* These are global so they can be consulted for error reports. */

char   *hosts_access_file = 0;		/* current access control table */
int     hosts_access_line;		/* current line (approximately) */

/* Forward declarations. */

static int table_match();
static int list_match();
static int client_match();
static int host_match();
static int string_match();
static int masked_match();
static FILE *xfopen();
static char *xgets();

/* Size of logical line buffer. */

#define	BUFLEN 2048

/* hosts_access - host access control facility */

int     hosts_access(daemon, client)
char   *daemon;
struct client_info *client;		/* host or user name may be empty */
{

#ifdef PROCESS_OPTIONS

    /*
     * After a rule has been matched, the optional language extensions may
     * decide to grant or refuse service anyway. This is done by jumping back
     * into the hosts_access() routine, bypassing the regular return from the
     * table_match() function calls below.
     */

    switch (setjmp(options_buf)) {
    case OPT_ALLOW:
	return (YES);
    case OPT_DENY:
	return (NO);
    }
#endif /* PROCESS_OPTIONS */

    /*
     * If the (daemon, client) pair is matched by an entry in the file
     * /etc/hosts.allow, access is granted. Otherwise, if the (daemon,
     * client) pair is matched by an entry in the file /etc/hosts.deny,
     * access is denied. Otherwise, access is granted. A non-existent
     * access-control file is treated as an empty file.
     */

    if (table_match(hosts_allow_table, daemon, client))
	return (YES);
    if (table_match(hosts_deny_table, daemon, client))
	return (NO);
    return (YES);
}

/* table_match - match table entries with (daemon, client) pair */

static int table_match(table, daemon, client)
char   *table;
char   *daemon;
struct client_info *client;		/* host or user name may be empty */
{
    FILE   *fp;
    char    sv_list[BUFLEN];		/* becomes list of daemons */
    char   *cl_list;			/* becomes list of clients */
    char   *sh_cmd;			/* becomes optional shell command */
    int     match;
    int     end;

    /* The following variables should always be tested together. */

    int     sv_match = NO;		/* daemon matched */
    int     cl_match = NO;		/* client matched */

    /*
     * Process the table one logical line at a time. Lines that begin with a
     * '#' character are ignored. Non-comment lines are broken at the ':'
     * character (we complain if there is none). The first field is matched
     * against the daemon process name (argv[0]), the second field against
     * the host name or address. A non-existing table is treated as if it
     * were an empty table. The search terminates at the first matching rule.
     * When a match is found an optional shell command is executed.
     */

    if (fp = xfopen(table, "r")) {
	while (!(sv_match && cl_match) && xgets(sv_list, sizeof(sv_list), fp)) {
	    if (sv_list[end = strlen(sv_list) - 1] != '\n') {
		syslog(LOG_ERR, "error: %s, line %d: missing newline or line too long",
		       hosts_access_file, hosts_access_line);
		continue;
	    }
	    if (sv_list[0] == '#')		/* skip comments */
		continue;
	    while (end > 0 && isspace(sv_list[end - 1]))
		end--;
	    sv_list[end] = '\0';		/* strip trailing whitespace */
	    if (sv_list[0] == 0)		/* skip blank lines */
		continue;
	    if ((cl_list = strchr(sv_list, ':')) == 0) {
		syslog(LOG_ERR, "error: %s, line %d: malformed entry: \"%s\"",
		       hosts_access_file, hosts_access_line, sv_list);
		continue;
	    }
	    *cl_list++ = '\0';			/* split 1st and 2nd fields */
	    if ((sh_cmd = strchr(cl_list, ':')) != 0)
		*sh_cmd++ = '\0';		/* split 2nd and 3rd fields */
	    if ((sv_match = list_match(sv_list, daemon, string_match)))
		cl_match = list_match(cl_list, (char *) client, client_match);
	}
	(void) fclose(fp);
    } else if (errno != ENOENT) {
	syslog(LOG_ERR, "error: cannot open %s: %m", table);
    }
    match = (sv_match == YES && cl_match == YES);
    if (match && sh_cmd)
#ifdef PROCESS_OPTIONS
	process_options(sh_cmd, daemon, client);
#else
	shell_cmd(sh_cmd, daemon, client);
#endif
    return (match);
}

/* list_match - match an item against a list of tokens with exceptions */

static int list_match(list, item, match_fn)
char   *list;
char   *item;
int   (*match_fn) ();
{
    char   *tok;
    int     match = NO;

    /*
     * Process tokens one at a time. We have exhausted all possible matches
     * when we reach an "EXCEPT" token or the end of the list. If we do find
     * a match, look for an "EXCEPT" list and recurse to determine whether
     * the match is affected by any exceptions.
     */

    for (tok = strtok(list, sep); tok != 0; tok = strtok((char *) 0, sep)) {
	if (strcasecmp(tok, "EXCEPT") == 0)	/* EXCEPT: give up */
	    break;
	if (match = (*match_fn) (tok, item))	/* YES or FAIL */
	    break;
    }
    /* Process exceptions to YES or FAIL matches. */

    if (match != NO) {
	while ((tok = strtok((char *) 0, sep)) && strcasecmp(tok, "EXCEPT"))
	     /* VOID */ ;
	if (tok == 0 || list_match((char *) 0, item, match_fn) == NO)
	    return (match);
    }
    return (NO);
}

/* host_match - match host name and/or address against token */

static int host_match(tok, client)
char   *tok;
struct client_info *client;
{
    int     match;

    /*
     * The KNOWN pattern requires that both name AND address match; all other
     * patterns are satisfied when either the name OR the address match.
     */

    if (strcasecmp(tok, "KNOWN") == 0) {
	(match = string_match(tok, client->name))
	    && (match = string_match(tok, client->addr));
    } else {
	(match = string_match(tok, client->name))
	    || (match = string_match(tok, client->addr));
    }
    return (match);
}


/* client_match - match client information */

static int client_match(tok, item)
char   *tok;
char   *item;
{
    struct client_info *client = (struct client_info *) item;
    int     match = NO;
    char   *at;
    int     host_does_match;
    int     user_does_match;

    /*
     * Perform username lookups when we see user_pat@host_pat, but only when
     * host_pat matches the remote host, and when no other attempt was done
     * to look up the username. Username lookup is possible only with TCP
     * clients.
     */

    if ((at = strchr(tok + 1, '@')) == 0) {	/* host pattern */
	match = host_match(tok, client);
    } else {					/* user@host */
	*at = 0;
	if (host_does_match = host_match(at + 1, client)) {
	    if (client->user[0] == 0 && RFC931_POSSIBLE(client))
		client->user = rfc931(client->rmt_sin, client->our_sin);
	    user_does_match = string_match(tok, client->user);
	    if (user_does_match == NO || user_does_match == FAIL) {
		match = user_does_match;
	    } else {
		match = host_does_match;
	    }
	}
	*at = '@';
    }
    return (match);
}

/* string_match - match string against token */

static int string_match(tok, string)
char   *tok;
char   *string;
{
    int     tok_len;
    int     str_len;
    char   *cut;
#ifdef	NETGROUP
    static char *mydomain = 0;
#endif

    /*
     * Return YES if a token has the magic value "ALL". Return FAIL if the
     * token is "FAIL". If the token starts with a "." (domain name), return
     * YES if it matches the last fields of the string. If the token has the
     * magic value "LOCAL", return YES if the string does not contain a "."
     * character. If the token ends on a "." (network number), return YES if
     * it matches the first fields of the string. If the token begins with a
     * "@" (netgroup name), return YES if the string is a (host) member of
     * the netgroup. Return YES if the token is "KNOWN" and if the string is
     * not empty or equal to FROM_UNKNOWN. Return YES if the token fully
     * matches the string. If the token is a netnumber/netmask pair, return
     * YES if the address is a member of the specified subnet.
     */

    if (string[0] == 0)				/* no info implies unknown */
	string = FROM_UNKNOWN;

    if (tok[0] == '.') {			/* domain: match last fields */
	if ((str_len = strlen(string)) > (tok_len = strlen(tok))
	    && strcasecmp(tok, string + str_len - tok_len) == 0)
	    return (YES);
    } else if (tok[0] == '@') {			/* netgroup: look it up */
#ifdef	NETGROUP
	if (mydomain == 0)
	    yp_get_default_domain(&mydomain);
	if (!isdigit(string[0])
	    && innetgr(tok + 1, string, (char *) 0, mydomain))
	    return (YES);
#else
	syslog(LOG_ERR, "error: %s, line %d: netgroup support is not configured",
	       hosts_access_file, hosts_access_line);
	return (NO);
#endif
    } else if (strcasecmp(tok, "ALL") == 0) {	/* all: match any */
	return (YES);
    } else if (strcasecmp(tok, "FAIL") == 0) {	/* fail: match any */
	return (FAIL);
    } else if (strcasecmp(tok, "LOCAL") == 0) {	/* local: no dots */
	if (strchr(string, '.') == 0 && strcasecmp(string, FROM_UNKNOWN) != 0)
	    return (YES);
    } else if (strcasecmp(tok, "KNOWN") == 0) {	/* not empty or unknown */
	if (strcasecmp(string, FROM_UNKNOWN) != 0)
	    return (YES);
    } else if (!strcasecmp(tok, string)) {	/* match host name or address */
	return (YES);
    } else if (tok[(tok_len = strlen(tok)) - 1] == '.') {	/* network */
	if (strncmp(tok, string, tok_len) == 0)
	    return (YES);
    } else if ((cut = strchr(tok, '/')) != 0) {	/* netnumber/netmask */
	if (isdigit(string[0]) && masked_match(tok, cut, string))
	    return (YES);
    }
    return (NO);
}

/* is_dotted_quad - determine if string looks like dotted quad */

static int is_dotted_quad(str)
char   *str;
{
    int     in_run = 0;
    int     runs = 0;

    /* Count the number of runs of characters between the dots. */

    while (*str) {
	if (*str == '.') {
	    in_run = 0;
	} else if (in_run == 0) {
	    in_run = 1;
	    runs++;
	}
	str++;
    }
    return (runs == 4);
}

/* masked_match - match address against netnumber/netmask */

static int masked_match(tok, slash, string)
char   *tok;
char   *slash;
char   *string;
{
    unsigned long net;
    unsigned long mask;
    unsigned long addr;

    /*
     * Disallow forms other than dotted quad: the treatment that inet_addr()
     * gives to (<4)-quad forms is not consistent with the access control
     * language. John P. Rouillard <rouilj@cs.umb.edu>.
     */

#define DOT_QUAD_ADDR(s) (is_dotted_quad(s) ? inet_addr(s) : INADDR_NONE)

    if ((addr = DOT_QUAD_ADDR(string)) == INADDR_NONE)
	return (NO);
    *slash = 0;
    net = DOT_QUAD_ADDR(tok);
    *slash = '/';
    if (net == INADDR_NONE || (mask = DOT_QUAD_ADDR(slash + 1)) == INADDR_NONE) {
	syslog(LOG_ERR, "error: %s, line %d: bad net/mask access control: %s",
	       hosts_access_file, hosts_access_line, tok);
	return (NO);
    }
    return (((addr & mask) == net) ? YES : NO);
}

/* xfopen - open file and set context for diagnostics */

static FILE *xfopen(path, mode)
char   *path;
char   *mode;
{
    FILE   *fp;

    if ((fp = fopen(path, mode)) != 0) {
	hosts_access_file = path;
	hosts_access_line = 0;
    }
    return (fp);
}

/* xgets - fgets() with backslash-newline stripping */

static char *xgets(buf, len, fp)
char   *buf;
int     len;
FILE   *fp;
{
    int     got;
    char   *start = buf;

    for (;;) {
	if (fgets(buf, len, fp) == 0)
	    return (buf > start ? start : 0);
	got = strlen(buf);
	if (got >= 1 && buf[got - 1] == '\n')
	    hosts_access_line++;		/* XXX */
	if (got >= 2 && buf[got - 2] == '\\' && buf[got - 1] == '\n') {
	    got -= 2;
	    buf += got;
	    len -= got;
	    buf[0] = 0;
	} else {
	    return (start);
	}
    }
}
