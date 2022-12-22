 /*
  * clean_exit() cleans up and terminates the program. It should be called
  * instead of exit when for some reason the real network daemon will not or
  * cannot be run. Reason: in the case of a datagram-oriented service we must
  * discard the not-yet received data from the client. Otherwise, inetd will
  * see the same datagram again and again, and go into a loop.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) clean_exit.c 1.3 93/09/11 20:45:43";
#endif

#include <stdio.h>

extern void exit();

#include "log_tcp.h"

/* clean_exit - clean up and exit */

void    clean_exit(client)
struct client_info *client;
{

    /*
     * In case of unconnected protocols we must eat up the not-yet received
     * data or inetd will loop.
     */

    if (client->sink)
	client->sink(client->fd);

    /*
     * Be kind to the inetd. We already reported the problem via the syslogd,
     * and there is no need for additional garbage in the logfile.
     */

    sleep(1);
    exit(0);
}
