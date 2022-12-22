/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_user.c	2.1  2.1 3/18/91";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include <pwd.h>
#include <netinet/in.h>
#include "popper.h"

/* 
 *  user:   Prompt for the user name at the start of a POP session
 */

int pop_user (p)
POP     *   p;
{
    /*  Save the user name */
    (void)strcpy(p->user, p->pop_parm[1]);

    /*  Tell the user that the password is required */
    return (pop_msg(p,POP_SUCCESS,"Password required for %s.",p->user));
}

#ifdef RPOP
/*
 *  rpop:   Use priviledged remote port as a hint that we can trust
 *	    what they say their user name is, and depend on ruserok().
 */

int pop_rpop (p)
POP	*   p;
{
    register struct passwd  *   pw;

    /*  Save the user name */
    (void)strcpy(p->user, p->pop_parm[1]);

    /*  Look for the user in the password file */
    if ((pw = getpwnam(p->user)) == NULL)
        return (pop_msg(p,POP_FAILURE, "User \"%s\" is incorrect.",p->user));

    /*  If they aren't coming from a priviledged port, f*** 'em.  */
    if (p->ipport >= IPPORT_RESERVED)
	return (pop_msg(p,POP_FAILURE,
			"RPOP requires priviledged source port (not %d).",
			(int)p->ipport));

    /*  Check their .rhosts.  */
    if (ruserok(p->client, (pw->pw_uid == 0), p->user, pw->pw_name) != 0)
	return (pop_msg(p,POP_FAILURE,
			"%s@%s not trusted as user %s here.",
			p->user, p->client, pw->pw_name));

    return (pop_setup(p, pw));
}
#endif RPOP
