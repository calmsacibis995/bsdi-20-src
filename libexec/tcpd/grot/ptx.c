 /*
  * The Dynix/PTX TLI implementation is not quite compatible with System V
  * Release 4. Some important functions are not present so we are limited to
  * IP-based services.
  * 
  * This module takes a TLI endpoint, and tries to determine the local IP
  * address, the client IP address, and the remote username if username
  * lookups are done irrespective of client. All results are in static memory
  * and will be overwritten upon the next call.
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
static char sccsid[] = "@(#) ptx.c 1.2 94/03/23 16:51:56";
#endif

#ifdef PTX

/* System libraries. */

#include <sys/types.h>
#include <sys/tiuser.h>
#include <sys/socket.h>
#include <stropts.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>

/* Local stuff. */

#include "log_tcp.h"

/* Forward declarations. */

static void ptx_sink();

/* tli_host - determine TLI endpoint info, PTX version */

int     tli_host(client, fd)
struct client_info *client;
int     fd;
{
    static struct sockaddr_in rmt_sin;
    static struct sockaddr_in our_sin;
    int     ret;

    /*
     * Initialize the result with suitable defaults.
     */

    init_client(client);
    client->fd = fd;

    /*
     * getpeerinaddr() was suggested by someone at Sequent. It seems to work
     * with connection-oriented (TCP) services such as rlogind and telnetd,
     * but it returns 0.0.0.0 with datagram (UDP) services. No problem: UDP
     * needs special treatment anyway, in case we must refuse service.
     */

    if (getpeerinaddr(client->fd, &rmt_sin, sizeof(rmt_sin)) == 0
	&& rmt_sin.sin_addr.s_addr != 0) {
	client->rmt_sin = &rmt_sin;
	if (getmyinaddr(client->fd, &our_sin, sizeof(our_sin)) == 0) {
	    client->our_sin = &our_sin;
	} else {
	    syslog(LOG_ERR, "getmyinaddr: %m");
	}
	return (sock_names(client));
    }

    /*
     * Another suggestion was to temporarily switch to the socket interface,
     * identify the client name/address with socket calls, then to switch
     * back to TLI. This seems to works OK with UDP services, but utterly
     * messes up rlogind and telnetd. No problem, rlogind and telnetd are
     * taken care of by the code above.
     */

#define SWAP_MODULE(f, old, new) (ioctl(f, I_POP, old), ioctl(f, I_PUSH, new))

    if (SWAP_MODULE(client->fd, "timod", "sockmod") != 0)
	syslog(LOG_ERR, "replace timod by sockmod: %m");
    ret = sock_host(client, client->fd);
    if (SWAP_MODULE(client->fd, "sockmod", "timod") != 0)
	syslog(LOG_ERR, "replace sockmod by timod: %m");
    if (client->sink != 0)
	client->sink = ptx_sink;
    return (ret);
}

/* ptx_sink - absorb unreceived IP datagram */

static void ptx_sink(fd)
int     fd;
{
    char    buf[BUFSIZ];
    struct sockaddr sa;
    int     size = sizeof(sa);

    /*
     * Eat up the not-yet received datagram. Where needed, switch to the
     * socket programming interface.
     */

    if (ioctl(fd, I_FIND, "timod") != 0)
	ioctl(fd, I_POP, "timod");
    if (ioctl(fd, I_FIND, "sockmod") == 0)
	ioctl(fd, I_PUSH, "sockmod");
    (void) recvfrom(fd, buf, sizeof(buf), 0, &sa, &size);
}

#endif /* PTX */
