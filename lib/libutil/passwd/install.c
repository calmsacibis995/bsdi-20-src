/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI install.c,v 2.1 1995/02/03 09:19:14 polk Exp
 */

#include <assert.h>
#include <db.h>
#include <err.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>

#include "libpasswd.h"

static int pw_mv __P((char *, char *));

int
pw_install(pw)
	struct pwinfo *pw;
{
	char *cp;

	assert(pw->pw_flags & (PW_INPLACE | PW_REBUILD));

	/*
	 * We always move the master file last, since it holds the
	 * lock (see pw_lock()).
	 */
	if (pw->pw_flags & PW_MAKEOLD &&
	    pw_mv(pw->pw_old.pf_name, pw_path(pw, _PATH_PASSWD, NULL)))
		return (-1);
	pw->pw_flags &= ~PW_RMOLD;
	if (pw->pw_flags & PW_INPLACE) {
		cp = pw->pw_new.pf_name;
	} else {
		if (pw_mv(pw->pw_idb.pf_name, pw_path(pw, _PATH_MP_DB, NULL)))
			return (-1);
		pw->pw_flags &= ~PW_RMIDB;
		if (pw_mv(pw->pw_sdb.pf_name, pw_path(pw, _PATH_SMP_DB, NULL)))
			return (-1);
		pw->pw_flags &= ~PW_RMSDB;
		cp = pw->pw_master.pf_name;
	}
	if (!(pw->pw_flags & PW_NOLINK)
	    && pw_mv(cp, pw_path(pw, _PATH_MASTERPASSWD, NULL)))
		return (-1);
	pw->pw_flags &= ~PW_RMNEW;
	pw_unlock(pw);
	return (0);
}

static int
pw_mv(from, to)
	char *from, *to;
{

	if (rename(from, to)) {
		warn("rename %s to %s", from, to);
		return (-1);
	}
	return (0);
}
