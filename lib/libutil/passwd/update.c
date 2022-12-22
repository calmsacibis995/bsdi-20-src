/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI update.c,v 2.1 1995/02/03 09:20:01 polk Exp
 */

#include <sys/types.h>
#include <db.h>
#include <err.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "libpasswd.h"

static int
comp(s1, s2)
	char *s1;
	char *s2;
{
	if (!s1 && !s2)
		return (0);
	if (!s1 || !s2)
		return (1);
	return (strcmp(s1, s2));
}

int
pw_update(old, new, flags)
	struct passwd *old;
	struct passwd *new;
	int flags;
{
	struct passwd *chk;
    	struct passwd current;
    	struct pwinfo pw;
	int c, t;
	int errflag;

	if (new && !(new = pw_copy(new)))
		err(1, "pw_update");
	warnx("updating passwd database");
	if (new->pw_uid == 0)
		flags |= PW_WARNROOT;
	pw_init(&pw, flags);			/* Initialize datebase */
	if (pw_lock(&pw, O_NONBLOCK) < 0) {
		printf("The password database is currently locked.\n");
		printf("Wait for lock to be released? [y]\n");
		fflush(stdout);
		c = t = getchar();
		while (t != EOF && t != '\n')
			t = getchar();
		if (c == 'n')
		    	errx(0, "entry not changed");
		if (pw_lock(&pw, 0) < 0)
		    	errx(1, "could not aquire lock.  Contact your system administrator");
    	}

	if (getuid() && old && (chk = getpwnam(old->pw_name))) {
		if (strcmp(old->pw_passwd, chk->pw_passwd)) {
			pw_abort(&pw);
			pw_unlock(&pw);
			errx(1, "password changed elsewhere.  no changes made");
		}
		if (old->pw_uid != chk->pw_uid || old->pw_gid != chk->pw_gid) {
			pw_abort(&pw);
			pw_unlock(&pw);
			errx(1, "uid/gid changed elsewhere.  no changes made");
		}
		if (old->pw_expire != chk->pw_expire) {
			pw_abort(&pw);
			pw_unlock(&pw);
			errx(1, "account expiration changed elsewhere.  no changes made");
		}
		if (comp(old->pw_class, chk->pw_class) ||
		    comp(old->pw_gecos, chk->pw_gecos) ||
		    comp(old->pw_dir, chk->pw_dir) ||
		    comp(old->pw_shell, chk->pw_shell)) {
			pw_abort(&pw);
			pw_unlock(&pw);
			errx(1, "account changed elsewhere.  no changes made");
		}
	}

	pw_sigs(NULL, NULL);			/* Block signals */
	pw_unlimit();				/* Crank up all usage limits */

	if (pw_inplace(&pw)) {
		pw_abort(&pw);
		pw_unlock(&pw);
		exit(1);
	}

	while (pw_next(&pw, &current, &errflag)) {
		if (old && current.pw_uid == old->pw_uid &&
	      	    strcmp(current.pw_name, old->pw_name) == 0) {
			c = pw_enter(&pw, &current, new);
		} else {
			c = pw_enter(&pw, &current, &current);
		}
		if (c) {
			pw_abort(&pw);
			pw_unlock(&pw);
			errx(1, "failure rebuilding passwd entry.  contact your system administrator");
		}
    	}
	if (errflag) {
		pw_abort(&pw);
		pw_unlock(&pw);
		errx(1, "failure reading passwd file.  contact your system administrator");
	}

	if (!old && pw_enter(&pw, NULL, new)) {
		pw_abort(&pw);
		pw_unlock(&pw);
		errx(1, "failure in adding new password entry.");
	}

	if (pw_ipend(&pw)) {
		pw_abort(&pw);
		pw_unlock(&pw);
		errx(1, "failure in closing password file.  contact your system administrator");
	}

	if (pw_install(&pw)) {
		pw_abort(&pw);
		pw_unlock(&pw);
		errx(1, "failure in installing password file.  contact your system administrator");
	}
	warnx("done");
	if (new)
		free(new);
	return (0);
}
