 /*
  * hosts_ctl() combines the most common applications of the host access
  * control library. routine. It bundles its arguments into a client_info
  * structure, then calls the hosts_access() access control checker. The host
  * name and user name arguments should be empty strings, "unknown" or real
  * data. If a match is found, the optional shell command is executed.
  * 
  * Restriction: this interface does not pass enough information to support
  * selective remote username lookups.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) hosts_ctl.c 1.3 93/09/24 19:48:26";
#endif

#include <stdio.h>

#include "log_tcp.h"

/* hosts_ctl - general interface for the hosts_access() routine */

int     hosts_ctl(daemon, name, addr, user)
char   *daemon;
char   *name;
char   *addr;
char   *user;
{
    struct client_info client;

    init_client (&client);
    client.name = name;
    client.addr = addr;
    client.user = user;

    return (hosts_access(daemon, &client));
}
