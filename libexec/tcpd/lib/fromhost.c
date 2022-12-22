 /*
  * On socket-only systems, fromhost() is nothing but an alias for the
  * socket-specific sock_host() function.
  * 
  * On systems with sockets and TLI, fromhost() determines the type of API
  * (sockets, TLI), then invokes the appropriate API-specific routines.
  * 
  * The API-specific routines determine the nature of the service (datagram,
  * stream), and the name and address of the host at the other end. In case
  * of an IP service, these routines also determine the local address and
  * port, and the remote username if username lookups are done irrespective
  * of client. All results are in static memory.
  * 
  * The return value is (-1) if the remote host pretends to have someone elses
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
static char sccsid[] = "@(#) fromhost.c 1.16 94/03/23 16:23:45";
#endif

/* System libraries. */

#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>

#if defined(TLI) || defined(PTX) || defined(TLI_SEQUENT)
#include <sys/tiuser.h>
#include <stropts.h>
#endif

/* Local stuff. */

#include "log_tcp.h"

#if !defined(TLI) && !defined(PTX) && !defined(TLI_SEQUENT)

/* fromhost - compatibility wrapper for socket-only systems */

int     fromhost(client)
struct client_info *client;
{
    int     client_fd = 0;		/* XXX compatibility */

    return (sock_host(client, client_fd));
}

#endif /* !TLI && !PTX && !TLI_SEQUENT */

#if defined(TLI) || defined(PTX) || defined(TLI_SEQUENT)

/* fromhost - find out what network API we should use */

int     fromhost(client)
struct client_info *client;
{
    int     client_fd = 0;		/* XXX compatibility */

    /*
     * On systems with streams support the IP network protocol family may
     * have more than one programming interface (sockets and TLI).
     * 
     * Thus, we must first find out what programming interface to use: sockets
     * or TLI. On some systems, sockets are not part of the streams system,
     * so if stdin is not a stream we assume sockets.
     */

    if (ioctl(client_fd, I_FIND, "timod") > 0) {
	return (tli_host(client, client_fd));
    } else {
	return (sock_host(client, client_fd));
    }
}

#endif /* TLI || PTX || TLI_SEQUENT */
