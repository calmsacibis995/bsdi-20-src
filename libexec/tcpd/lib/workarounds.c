 /*
  * Workarounds for known system software bugs. This module provides wrappers
  * around library functions and system calls that are known to have problems
  * on some systems. Most of these workarounds won't do any harm on regular
  * systems.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
char    sccsid[] = "@(#) workarounds.c 1.2 94/02/01 22:12:23";
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <syslog.h>

extern int errno;

#include "log_tcp.h"

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
#define MAXHOSTNAMELEN  256             /* storage for host name */
#endif

 /*
  * Some DG/UX inet_addr() versions return a struct/union instead of a long.
  * You have this problem when the compiler complains about illegal lvalues
  * or something like that. The following code fixes this mutant behaviour.
  * It should not be enabled on "normal" systems.
  * 
  * Bug reported by ben@piglet.cr.usgs.gov (Rev. Ben A. Mesander).
  */

#ifdef INET_ADDR_BUG

#undef inet_addr

long    fix_inet_addr(string)
char   *string;
{
    return (inet_addr(string).s_addr);
}

#endif /* INET_ADDR_BUG */

 /*
  * With some System-V versions, the fgets() library function does not
  * account for partial reads from e.g. sockets. The result is that fgets()
  * gives up too soon, causing username lookups to fail. Problem first
  * reported for IRIX 4.0.5, by Steve Kotsopoulos <steve@ecf.toronto.edu>.
  * The following code works around the problem. It does no harm on "normal"
  * systems.
  */

#ifdef BROKEN_FGETS

#undef fgets

char   *fix_fgets(buf, len, fp)
char   *buf;
int     len;
FILE   *fp;
{
    char   *cp = buf;
    int     c;

    /*
     * Copy until the buffer fills up, until EOF, or until a newline is
     * found.
     */
    while (len > 1 && (c = getc(fp)) != EOF) {
	len--;
	*cp++ = c;
	if (c == '\n')
	    break;
    }

    /*
     * Return 0 if nothing was read. This is correct even when a silly buffer
     * length was specified.
     */
    if (cp > buf) {
	*cp = 0;
	return (buf);
    } else {
	return (0);
    }
}

#endif /* BROKEN_FGETS */

 /*
  * With early SunOS 5 versions, recvfrom() does not completely fill in the
  * source address structure when doing a non-destructive read. The following
  * code works around the problem. It does no harm on "normal" systems.
  */

#ifdef RECVFROM_BUG

#undef recvfrom

int     fix_recvfrom(sock, buf, buflen, flags, from, fromlen)
int     sock;
char   *buf;
int     buflen;
int     flags;
struct sockaddr *from;
int    *fromlen;
{
    int     ret;

    /* Assume that both ends of a socket belong to the same address family. */

    if ((ret = recvfrom(sock, buf, buflen, flags, from, fromlen)) >= 0) {
	if (from->sa_family == 0) {
	    struct sockaddr my_addr;
	    int     my_addr_len = sizeof(my_addr);

	    if (getsockname(0, &my_addr, &my_addr_len)) {
		syslog(LOG_ERR, "getsockname: %m");
	    } else {
		from->sa_family = my_addr.sa_family;
	    }
	}
    }
    return (ret);
}

#endif /* RECVFROM_BUG */

 /*
  * The Apollo SR10.3 and some SYSV4 getpeername(2) versions do not return an
  * error in case of a datagram-oriented socket. Instead, they claim that all
  * UDP requests come from address 0.0.0.0. The following code works around
  * the problem. It does no harm on "normal" systems.
  */

#ifdef GETPEERNAME_BUG

#undef getpeername

int     fix_getpeername(sock, sa, len)
int     sock;
struct sockaddr *sa;
int    *len;
{
    int     ret;
    struct sockaddr_in *sin = (struct sockaddr_in *) sa;

    if ((ret = getpeername(sock, sa, len)) >= 0
	&& sa->sa_family == AF_INET
	&& sin->sin_addr.s_addr == 0) {
	errno = ENOTCONN;
	return (-1);
    } else {
	return (ret);
    }
}

#endif /* GETPEERNAME_BUG */

 /*
  * According to Karl Vogel (vogelke@c-17igp.wpafb.af.mil) some Pyramid
  * versions have no yp_default_domain() function. We use getdomainname()
  * instead.
  */

#ifdef USE_GETDOMAIN

int     yp_get_default_domain(ptr)
char  **ptr;
{
    static char mydomain[MAXHOSTNAMELEN];

    *ptr = mydomain;
    return (getdomainname(mydomain, MAXHOSTNAMELEN));
}

#endif /* USE_GETDOMAIN */
