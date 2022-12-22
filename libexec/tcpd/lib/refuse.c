 /*
  * refuse() reports a refused connection, and takes the consequences: in
  * case of a datagram-oriented service, the unread datagram is taken from
  * the input queue (or inetd would see the same datagram again and again);
  * the program is terminated.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) refuse.c 1.4 93/09/11 20:45:19";
#endif

/* System libraries. */

#include <syslog.h>

/* Local stuff. */

#include "log_tcp.h"

/* refuse - refuse request from bad host */

void    refuse(client)
struct client_info *client;
{
    syslog(deny_severity, "refused connect from %s", hosts_info(client));
    clean_exit(client);
    /* NOTREACHED */
}
