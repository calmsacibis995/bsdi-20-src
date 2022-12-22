/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI enter.c,v 2.1 1995/02/03 09:19:00 polk Exp
 */

#include <db.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>

#include "libpasswd.h"

/*
 * Enter a passwd line in both databases.  (Hides shadowing from caller.)
 */
int
pw_enter(pw, old, new)
	struct pwinfo *pw;
	struct passwd *old, *new;
{

	return (pw_ent1(pw, old, new, 0) || pw_ent1(pw, old, new, 1) ? -1 : 0);
}
