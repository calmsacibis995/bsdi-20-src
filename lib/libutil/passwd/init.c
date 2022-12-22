/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI init.c,v 2.1 1995/02/03 09:19:04 polk Exp
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <db.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>

#include "libpasswd.h"

/*
 * Produce a clean pwinfo, with MAKEOLD and/or STRIPDIR.
 */
void
pw_init(pw, flags)
	struct pwinfo *pw;
	int flags;
{

	assert((flags & ~PW__USER) == 0);
	pw->pw_flags = flags;
	pw->pw_master.pf_name = NULL;
	pw->pw_master.pf_fp = NULL;
	pw->pw_new.pf_name = NULL;
	pw->pw_new.pf_fp = NULL;
	pw->pw_idb.pf_name = NULL;
	pw->pw_idb.pf_db = NULL;
	pw->pw_sdb.pf_name = NULL;
	pw->pw_sdb.pf_db = NULL;
	pw->pw_old.pf_name = NULL;
	pw->pw_old.pf_fp = NULL;
    	(void)umask(0);
}
