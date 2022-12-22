 /*
  * tli_host() determines the type of transport (connected, connectionless),
  * the name and address of the host at the other end of a network link. In
  * case of an IP service, tli_host() also determines the local address and
  * port, and the remote username if username lookups are done irrespective
  * of client. All results are in static memory.
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
static char sccsid[] = "@(#) tli.c 1.9 94/03/23 16:24:47";
#endif

#ifdef TLI

/* System libraries. */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#include <sys/tiuser.h>
#include <sys/timod.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <netconfig.h>
#include <netdir.h>

extern char *strncpy();

 /* Some systems versions advertise a too small MAXHOSTNAMELEN value. */

#if (MAXHOSTNAMELEN < 64)
#undef MAXHOSTNAMELEN
#endif

 /* In case not defined in <sys/param.h>. */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	256		/* storage for host name */
#endif

extern char *nc_sperror();
extern int errno;
extern char *sys_errlist[];
extern int sys_nerr;
extern int t_errno;
extern char *t_errlist[];
extern int t_nerr;

/* Local stuff. */

#include "log_tcp.h"

/* Forward declarations. */

static struct t_unitdata *tli_remote_addr();
static struct t_unitdata *tli_local_addr();
static struct netconfig *tli_transport();
static int tli_names();
static int tli_match_hostname();
static char *tli_error();
static void tli_sink();

/* tli_host - determine endpoint info */

int     tli_host(client, fd)
struct client_info *client;
int     fd;
{
    struct netconfig *config;
    struct t_unitdata *rmt_unit;
    struct t_unitdata *our_unit;
    static struct sockaddr_in rmt_sin;
    static struct sockaddr_in our_sin;
    int     ret = 0;			/* host name/addr unknown */

    /*
     * Initialize the result with suitable defaults.
     */

    init_client(client);
    client->fd = fd;

    /*
     * Find out the client address, find out what type of transport is hidden
     * underneath the TLI programmatic interface, map the transport address
     * to a human-readable form, and determine the host name. Double check
     * the hostname.
     * 
     * If we discover that we are using an IP transport, do DNS lookups and
     * determine the local binding. Otherwise, use the transport-independent
     * method and stick to generic network addresses. XXX hard-coded protocol
     * family name.
     */

#define SINCP(d,s) (((d) = *(struct sockaddr_in *) (s)), &(d))

    if ((rmt_unit = tli_remote_addr(client)) != 0) {
	if ((config = tli_transport(client->fd)) != 0) {
	    if (strcasecmp(config->nc_protofmly, "inet") == 0) {
		if ((our_unit = tli_local_addr(client)) != 0) {
		    client->our_sin = SINCP(our_sin, our_unit->addr.buf);
		    t_free((char *) our_unit, T_UNITDATA);
		}
		client->rmt_sin = SINCP(rmt_sin, rmt_unit->addr.buf);
		ret = sock_names(client);
	    } else {
		ret = tli_names(client, &(rmt_unit->addr), config);
	    }
	    freenetconfigent(config);
	}
	t_free((char *) rmt_unit, T_UNITDATA);
    }
    return (ret);
}

/* tli_remote_addr - determine TLI client address */

static struct t_unitdata *tli_remote_addr(client)
struct client_info *client;
{
    struct t_unitdata *unit;
    int     flags;

    /*
     * Allocate storage for the endpoint address. t_alloc() finds out for us
     * how large the address can be. Address sizes depend on the underlying
     * transport protocol.
     * 
     * Determine the transport-independent client address. With connectionless
     * services, peek at the sender address of the pending datagram without
     * popping it off the receive queue. This trick works because only the
     * address member of the unitdata structure has been allocated.
     */

    if ((unit = (struct t_unitdata *) t_alloc(client->fd, T_UNITDATA, T_ADDR)) == 0) {
	syslog(LOG_ERR, "error: t_alloc: %s", tli_error());
    } else {
	if (ioctl(client->fd, TI_GETPEERNAME, &(unit->addr)) < 0) {
	    if (t_rcvudata(client->fd, unit, &flags) < 0) {
		syslog(LOG_ERR, "error: can't get client address: %s", tli_error());
		t_free((void *) unit, T_UNITDATA);
		unit = 0;
	    } else {
		client->sink = tli_sink;
	    }
	    /* beware: at this point, unit may be null */
	}
    }
    return (unit);
}

/* tli_local_addr - determine TLI local address */

struct t_unitdata *tli_local_addr(client)
struct client_info *client;
{
    struct t_unitdata *unit;

    if ((unit = (struct t_unitdata *) t_alloc(client->fd, T_UNITDATA, T_ADDR)) == 0) {
	syslog(LOG_ERR, "error: t_alloc: %s", tli_error());
    } else {
	if (ioctl(client->fd, TI_GETMYNAME, &(unit->addr)) < 0) {
	    syslog(LOG_ERR, "error: TI_GETMYNAME: %m");
	    t_free((void *) unit, T_UNITDATA);
	    unit = 0;
	}
	/* beware: at this point, unit may be null */
    }
    return (unit);
}

/* tli_transport - find out TLI transport type */

static struct netconfig *tli_transport(fd)
int     fd;
{
    struct stat from_client;
    struct stat from_config;
    void   *handlep;
    struct netconfig *config;

    /*
     * Assuming that the network device is a clone device, we must compare
     * the major device number of stdin to the minor device number of the
     * devices listed in the netconfig table.
     */

    if (fstat(fd, &from_client) != 0) {
	syslog(LOG_ERR, "error: fstat(fd %d): %m", fd);
	return (0);
    }
    if ((handlep = setnetconfig()) == 0) {
	syslog(LOG_ERR, "error: setnetconfig: %m");
	return (0);
    }
    while (config = getnetconfig(handlep)) {
	if (stat(config->nc_device, &from_config) == 0) {
	    if (minor(from_config.st_rdev) == major(from_client.st_rdev))
		break;
	}
    }
    if (config == 0) {
	syslog(LOG_ERR, "error: unable to identify transport protocol");
	return (0);
    }

    /*
     * Something else may clobber our getnetconfig() result, so we'd better
     * acquire our private copy.
     */

    if ((config = getnetconfigent(config->nc_netid)) == 0) {
	syslog(LOG_ERR, "error: getnetconfigent(%s): %s",
	       config->nc_netid, nc_sperror());
	return (0);
    }
    return (config);
}

/* tli_names - map TLI transport address to readable address and name */

static int tli_names(client, taddr, config)
struct client_info *client;
struct netbuf *taddr;
struct netconfig *config;
{
    struct nd_hostservlist *service;
    static char host_name[MAXHOSTNAMELEN];
    static char host_addr[MAXHOSTNAMELEN];
    char   *uaddr;
    int     ret = 0;			/* hostname unknown */

    /*
     * Translate the transport address to (well, more-or-less human-readable)
     * universal form and clean up. Some transports may not have a universal
     * address representation and will lose. XXX uaddr form includes blanks?!
     */

#define STRNCP(d,s,l) { strncpy((d),(s),(l)); (d)[(l)-1] = 0; }

    if ((uaddr = taddr2uaddr(config, taddr)) != 0) {
	STRNCP(host_addr, uaddr, sizeof(host_addr));
	client->addr = host_addr;
	free(uaddr);
    }

    /*
     * Map the transport address to a human-readable hostname. Try to verify
     * that the host name does not belong to someone else. If host name
     * verification fails, ignore the host name.
     */

    if (netdir_getbyaddr(config, &service, taddr) == ND_OK) {
	if (tli_match_hostname(config, service->h_hostservs, client->addr)) {
	    STRNCP(host_name, service->h_hostservs->h_host, sizeof(host_name));
	    client->name = host_name;
	    ret = 0;				/* hostname ok */
	} else {
	    ret = (-1);				/* bad or unverified name */
	}
	netdir_free((void *) service, ND_HOSTSERVLIST);
    }
    return (ret);
}

/* tli_match_hostname - determine if host name matches transport address */

static int tli_match_hostname(config, service, uaddr)
struct netconfig *config;
struct nd_hostserv *service;
char   *uaddr;
{
    struct nd_addrlist *addr_list;

    if (netdir_getbyname(config, service, &addr_list) != ND_OK) {

	/*
	 * Unable to verify that the name matches the address. This may be a
	 * transient problem or a botched name server setup. We decide to
	 * play safe.
	 */

	syslog(LOG_ERR,
	       "warning: can't verify hostname: netdir_getbyname(%s) failed",
	       service->h_host);
	return (FROM_BAD);

    } else {

	/*
	 * Look up the host address in the address list we just got. The
	 * comparison is done on the textual representation, because the
	 * transport address is an opaque structure that may have holes with
	 * uninitialized garbage. This approach obviously loses when the
	 * address does not have a textual representation.
	 */

	char   *ua;
	int     i;
	int     found = 0;

	for (i = 0; found == 0 && i < addr_list->n_cnt; i++) {
	    if ((ua = taddr2uaddr(config, &(addr_list->n_addrs[i]))) != 0) {
		found = !strcmp(ua, uaddr);
		free(ua);
	    }
	}
	netdir_free((void *) addr_list, ND_ADDRLIST);

	/*
	 * When the host name does not map to the client address, assume
	 * someone has compromised a name server. More likely someone botched
	 * it, but that could be dangerous, too.
	 */

	if (found) {
	    return (FROM_GOOD);
	} else {
	    syslog(LOG_ERR, "warning: host name/address mismatch: %s != %s",
		   uaddr, service->h_host);
	    return (FROM_BAD);
	}
    }
}

/* tli_error - convert tli error number to text */

static char *tli_error()
{
    static char buf[40];

    if (t_errno != TSYSERR) {
	if (t_errno < 0 || t_errno >= t_nerr) {
	    sprintf(buf, "Unknown TLI error %d", t_errno);
	    return (buf);
	} else {
	    return (t_errlist[t_errno]);
	}
    } else {
	if (errno < 0 || errno >= sys_nerr) {
	    sprintf(buf, "Unknown UNIX error %d", errno);
	    return (buf);
	} else {
	    return (sys_errlist[errno]);
	}
    }
}

/* tli_sink - absorb unreceived datagram */

static void tli_sink(fd)
int     fd;
{
    struct t_unitdata *unit;
    int     flags;

    /*
     * Something went wrong. Absorb the datagram to keep inetd from looping.
     * Allocate storage for address, control and data. If that fails, sleep
     * for a couple of seconds in an attempt to keep inetd from looping too
     * fast.
     */

    if ((unit = (struct t_unitdata *) t_alloc(fd, T_UNITDATA, T_ALL)) == 0) {
	syslog(LOG_ERR, "error: t_alloc: %s", tli_error());
	sleep(5);
    } else {
	(void) t_rcvudata(fd, unit, &flags);
	t_free((void *) unit, T_UNITDATA);
    }
}

#endif /* TLI */
