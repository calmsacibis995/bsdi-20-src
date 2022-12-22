 /*
  * sock_host() determines the type of socket (datagram, stream), the name
  * and address of the host at the other end of a socket, the local socket
  * address and port, and the remote username if username lookups are done
  * irrespective of client. All results are in static memory and will be
  * overwritten upon the next call.
  * 
  * The return status is (-1) if the remote host pretends to have someone elses
  * name, or if the remote host name is available but could not be verified;
  * in either case the hostname will be ignored. The return status is zero in
  * all other cases (the hostname is unavailable, or the host name double
  * check succeeds).
  * 
  * Diagnostics are reported through syslog(3).
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) socket.c 1.9 94/02/01 22:12:16";
#endif

/* System libraries. */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>

extern char *inet_ntoa();
extern char *strncpy();
extern char *strcpy();

 /*
  * Some AIX versions advertise a too small MAXHOSTNAMELEN value (32).
  * Result: long hostnames would be truncated, and connections would be
  * dropped because of host name verification failures. Adrian van Bloois
  * (A.vanBloois@info.nic.surfnet.nl) figured out what was the problem.
  */

#if (MAXHOSTNAMELEN < 64)
#undef MAXHOSTNAMELEN
#endif

/* In case not defined in <sys/param.h>. */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	256		/* storage for host name */
#endif

/* Local stuff. */

#include "log_tcp.h"

/* Forward declarations. */

static int sock_match_hostname();
static void sock_sink();

#ifdef APPEND_DOT

 /*
  * Speed up DNS lookups by terminating the host name with a dot. Should be
  * done with care. The speedup can give problems with lookups from sources
  * that lack DNS-style trailing dot magic, such as local files or NIS maps.
  */

static struct hostent *gethostbyname_dot(name)
char   *name;
{
    char    dot_name[MAXHOSTNAMELEN + 1];
    char   *strchr();

    /*
     * Don't append dots to unqualified names. Such names are likely to come
     * from local hosts files or from NIS.
     */

    if (strchr(name, '.') == 0 || strlen(name) >= MAXHOSTNAMELEN - 1) {
	return (gethostbyname(name));
    } else {
	sprintf(dot_name, "%s.", name);
	return (gethostbyname(dot_name));
    }
}

#define gethostbyname gethostbyname_dot
#endif

/* sock_host - determine endpoint info */

int     sock_host(client, fd)
struct client_info *client;
int     fd;
{
    static struct sockaddr rmt_sa;
    static struct sockaddr our_sa;
    int     len;
    char    buf[BUFSIZ];

    /*
     * Initialize the result with suitable defaults.
     */

    init_client(client);
    client->fd = fd;

    /*
     * Look up the remote host address. Hal R. Brand <BRAND@addvax.llnl.gov>
     * suggested how to get the remote host info in case of UDP connections:
     * peek at the first message without actually looking at its contents.
     */

    len = sizeof(rmt_sa);
    if (getpeername(client->fd, &rmt_sa, &len) < 0) {
	len = sizeof(rmt_sa);
	if (recvfrom(client->fd, buf, sizeof(buf), MSG_PEEK, &rmt_sa, &len) < 0) {
	    syslog(LOG_ERR, "error: can't get client address: %m");
	    return (0);				/* address and name unknown */
	}
#ifdef really_paranoid
	memset(buf, 0 sizeof(buf));
#endif
	client->sink = sock_sink;
    }
    client->rmt_sin = (struct sockaddr_in *) & rmt_sa;

    /*
     * Determine the local binding. Right now this information is used only
     * for remote username lookups, but it may become useful to map the local
     * port number to an internet service name, so that services handled by
     * the same daemon program (same argv[0] value) can still be
     * distinguished.
     */

    len = sizeof(our_sa);
    if (getsockname(client->fd, &our_sa, &len) < 0) {
	syslog(LOG_ERR, "error: getsockname: %m");
    } else {
	client->our_sin = (struct sockaddr_in *) & our_sa;
    }
    return (sock_names(client));
}

/* sock_names - map IP address info to readable address and name */

int     sock_names(client)
struct client_info *client;
{
    static char addr_buf[MAXHOSTNAMELEN];
    static char name_buf[MAXHOSTNAMELEN];
    struct hostent *hp;
    struct in_addr addr;

    /*
     * Some stupid compilers can do struct assignment but cannot do structure
     * initialization.
     */

    addr = client->rmt_sin->sin_addr;

    /*
     * Do username lookups if we do lookups irrespective of client. In a
     * future release we should perhaps use asynchronous I/O so that the
     * handshake with the rfc931 server can proceed while hostname lookups
     * are going on.
     */

#ifdef ALWAYS_RFC931
    if (RFC931_POSSIBLE(client))
	client->user = rfc931(client->rmt_sin, client->our_sin);
#endif

    /*
     * Map the address to human-readable form.
     */

#define STRNCP(d,s,l) { strncpy((d),(s),(l)); ((d)[(l)-1] = 0); }

    STRNCP(addr_buf, inet_ntoa(addr), sizeof(addr_buf));
    client->addr = addr_buf;

    /*
     * Look up the remote host name. Verify that the host name does not
     * belong to someone else. Ignore the hostname if verification fails or
     * if verification is not possible.
     */

    if ((hp = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET)) == 0)
	return (0);				/* hostname unknown */
    STRNCP(name_buf, hp->h_name, sizeof(name_buf));
    if (sock_match_hostname(name_buf, addr)) {
	client->name = name_buf;
	return (0);				/* hostname verified ok */
    } else {
	return (-1);				/* bad or unverified name */
    }
}

/* sock_match_hostname - determine if host name matches IP address */

static int sock_match_hostname(remotehost, addr)
char   *remotehost;
struct in_addr addr;
{
    struct hostent *hp;
    int     i;

    /*
     * Verify that the client address is a member of the address list
     * returned by gethostbyname(remotehost).
     * 
     * Verify also that gethostbyaddr() and gethostbyname() return the same
     * hostname, or rshd and rlogind may still end up being spoofed.
     * 
     * On some sites, gethostbyname("localhost") returns "localhost.my.domain".
     * This is a DNS artefact. We treat it as a special case. When we can't
     * believe the address list from gethostbyname("localhost") we're in big
     * trouble anyway.
     */

    if ((hp = gethostbyname(remotehost)) == 0) {

	/*
	 * Unable to verify that the host name matches the address. This may
	 * be a transient problem or a botched name server setup.
	 */

	syslog(LOG_ERR,
	       "warning: can't verify hostname: gethostbyname(%s) failed",
	       remotehost);
	return (FROM_BAD);

    } else if (strcasecmp(remotehost, hp->h_name)
	       && strcasecmp(remotehost, "localhost")) {

	/*
	 * The gethostbyaddr() and gethostbyname() calls did not return the
	 * same hostname. This could be a nameserver configuration problem.
	 * It could also be that someone is trying to spoof us.
	 */

	syslog(LOG_ERR, "warning: host name/name mismatch: %s != %s",
	       remotehost, hp->h_name);
	return (FROM_BAD);

    } else {

	/*
	 * The client address should be a member of the address list returned
	 * by gethostbyname(). We should first verify that the h_addrtype
	 * field is AF_INET, but this program has already caused too much
	 * grief on systems with broken library code.
	 */

	for (i = 0; hp->h_addr_list[i]; i++) {
	    if (memcmp(hp->h_addr_list[i], (caddr_t) & addr, sizeof(addr)) == 0)
		return (FROM_GOOD);
	}

	/*
	 * The host name does not map to the initial client address. Perhaps
	 * someone has messed up. Perhaps someone compromised a name server.
	 */

	syslog(LOG_ERR, "warning: host name/address mismatch: %s != %s",
	       inet_ntoa(addr), hp->h_name);
	return (FROM_BAD);
    }
}

/* sock_sink - absorb unreceived IP datagram */

static void sock_sink(fd)
int     fd;

{
    char    buf[BUFSIZ];
    struct sockaddr sa;
    int     size = sizeof(sa);

    /*
     * Eat up the not-yet received datagram. Some systems insist on a
     * non-zero source address argument in the recvfrom() call below.
     */

    (void) recvfrom(fd, buf, sizeof(buf), 0, &sa, &size);
}
