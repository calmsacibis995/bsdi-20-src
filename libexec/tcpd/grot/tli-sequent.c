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
  * Warning - this relies heavily on the TLI implementation in PTX 2.X
  * and will probably not work under PTX 4.
  *
  * Author: Tim Wright, Sequent Computer Systems Ltd., UK.
  */

#ifndef lint
static char sccsid[] = "@(#) tli-sequent.c 1.0 94/02/11 10:20:30";
#endif

#ifdef TLI_SEQUENT

/* System libraries. */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/tiuser.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/timod.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>

extern char *strncpy();

 /* Some systems versions advertise a too small MAXHOSTNAMELEN value. */

#if (MAXHOSTNAMELEN < 64)
#undef MAXHOSTNAMELEN
#endif

 /* In case not defined in <sys/param.h>. */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	256		/* storage for host name */
#endif

extern int errno;
extern char *sys_errlist[];
extern int sys_nerr;
extern int t_errno;
extern char *t_errlist[];
extern int t_nerr;

/* Local stuff. */

#include "log_tcp.h"
#include "tli-sequent.h"

/* Forward declarations. */

static char *tli_error();
static void tli_sink();

/* tli_host - determine endpoint info */

int     tli_host(client, fd)
struct client_info *client;
int     fd;
{
    static struct sockaddr_in rmt_sin;
    static struct sockaddr_in our_sin;
    struct _ti_user *tli_state_ptr;
    union  T_primitives *TSI_prim_ptr;
    struct strpeek peek;
    int     len;

    /*
     * Initialize the result with suitable defaults.
     */

    init_client(client);
    client->fd = fd;

    /*
     * Find out the client address using getpeerinaddr(). This call is the
     * TLI equivalent to getpeername() under Dynix/ptx.
     */

    len = sizeof(rmt_sin);
    t_sync(client->fd);
    if (getpeerinaddr(client->fd, &rmt_sin, len) < 0) {
	syslog(LOG_ERR, "error: can't get client address: %s", tli_error());
	return (0);				/* address and name unknown */
    }
    client->rmt_sin = &rmt_sin;

    /* Call TLI utility routine to get information on endpoint */
    if ((tli_state_ptr = _t_checkfd(fd)) == NULL)
	return(0);

    if (tli_state_ptr->ti_servtype == T_CLTS) {
	/* UDP - may need to get address the hard way */
	if (rmt_sin.sin_addr.s_addr == 0) {
	    /* The UDP endpoint is not connected so we didn't get the */
	    /* remote address - get it the hard way ! */

	    /* Look at the control part of the top message on the stream */
	    /* we don't want to remove it from the stream so we use I_PEEK */
	    peek.ctlbuf.maxlen = tli_state_ptr->ti_ctlsize;
	    peek.ctlbuf.len = 0;
	    peek.ctlbuf.buf = tli_state_ptr->ti_ctlbuf;
	    /* Don't even look at the data */
	    peek.databuf.maxlen = -1;
	    peek.databuf.len = 0;
	    peek.databuf.buf = 0;
	    peek.flags = 0;

	    switch (ioctl(client->fd, I_PEEK, &peek)) {
	    case -1:
		syslog(LOG_ERR, "error: can't peek at endpoint: %s", tli_error());
		return(0);
	    case 0:
		/* No control part - we're hosed */
		syslog(LOG_ERR, "error: can't get UDP info: %s", tli_error());
		return(0);
	    default:
		/* FALL THROUGH */
		;
	    }
	    /* Can we even check the PRIM_type ? */
	    if (peek.ctlbuf.len < sizeof(long)) {
		syslog(LOG_ERR, "error: UDP control info garbage");
		return(0);
	    }
	    TSI_prim_ptr = (union T_primitives *) peek.ctlbuf.buf;
	    if (TSI_prim_ptr->type != T_UNITDATA_IND) {
		syslog(LOG_ERR, "error: wrong type for UDP control info");
		return(0);
	    }
	    /* Validate returned unitdata indication packet */
	    if ((peek.ctlbuf.len < sizeof(struct T_unitdata_ind)) ||
		((TSI_prim_ptr->unitdata_ind.OPT_length != 0) &&
		 (peek.ctlbuf.len <
		    TSI_prim_ptr->unitdata_ind.OPT_length +
		    TSI_prim_ptr->unitdata_ind.OPT_offset))) {
		syslog(LOG_ERR, "error: UDP control info garbaged");
		return(0);
	    }
	    /* Extract the address */
	    memcpy(&rmt_sin,
		peek.ctlbuf.buf + TSI_prim_ptr->unitdata_ind.SRC_offset,
		TSI_prim_ptr->unitdata_ind.SRC_length);
	}
	client->sink = tli_sink;
    }

    if (getmyinaddr(client->fd, &our_sin, len) < 0)
	syslog(LOG_ERR, "error: can't get local address: %s", tli_error());
    else
	client->our_sin = &our_sin;
    return (sock_names(client));
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

#endif /* TLI_SEQUENT */
