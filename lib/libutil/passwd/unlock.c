/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI unlock.c,v 2.1 1995/02/03 09:19:57 polk Exp
 */

#include <db.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>

#include "libpasswd.h"

/*
 * End lock.
 */
void
pw_unlock(pw)
	struct pwinfo *pw;
{

	if (pw->pw_flags & PW_LOCKED) {
		(void)fclose(pw->pw_lock.pf_fp);
		pw->pw_flags &= ~PW_LOCKED;
	}
}
