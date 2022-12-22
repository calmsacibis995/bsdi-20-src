/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI abort.c,v 2.1 1995/02/03 09:18:49 polk Exp
 */

#include <db.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "libpasswd.h"

void
pw_abort(pw)
	struct pwinfo *pw;
{
	FILE *fp;

	if (pw->pw_flags & PW_RMIDB)
		(void)unlink(pw->pw_idb.pf_name);
	if (pw->pw_flags & PW_RMSDB)
		(void)unlink(pw->pw_sdb.pf_name);
	if (pw->pw_flags & PW_RMOLD)
		(void)unlink(pw->pw_old.pf_name);
	if (pw->pw_flags & PW_RMNEW)
		(void)unlink(pw->pw_new.pf_name);
	pw->pw_flags &= PW__USER | PW_LOCKED;
	fp = pw->pw_master.pf_fp;
	pw->pw_master.pf_fp = NULL;
	if (fp != NULL && fp != pw->pw_lock.pf_fp)
		(void)fclose(fp);
}
