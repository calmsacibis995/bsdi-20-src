/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI inplace.c,v 2.1 1995/02/03 09:19:08 polk Exp
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <paths.h>
#include <unistd.h>

#include "libpasswd.h"

static int pw_opendb __P((struct pwinfo *, struct pwf *, char *));

/*
 * Set up to update the databases in place.
 * We open up the databases for modification and make new flat files
 * (because the old flat files may have some lines replaced).
 */
int
pw_inplace(pw)
	struct pwinfo *pw;
{
	int fd, omask;
	char *cp;
	FILE *fp;
	struct stat st;

	assert((pw->pw_flags & (PW_INPLACE | PW_REBUILD)) == 0);

	/* Open input master file, or use lock file if input locked. */
	cp = pw_path(pw, _PATH_MASTERPASSWD, NULL);
	if (pw->pw_flags & PW_LOCKED && strcmp(cp, pw->pw_lock.pf_name) == 0)
		pw->pw_master = pw->pw_lock;
	else {
		if ((fp = fopen(cp, "r")) == NULL) {
			warn("cannot read %s", cp);
			return (-1);
		}
		pw->pw_master.pf_name = cp;
		pw->pw_master.pf_fp = fp;
	}

	/* Create with exact permissions (umask restored at end). */
	omask = umask(0);

	/* These are not removed on error (good, bad, ??? -- use copies?) */
	if (pw_opendb(pw, &pw->pw_idb, _PATH_MP_DB))
		goto bad;
	if (pw_opendb(pw, &pw->pw_sdb, _PATH_SMP_DB))
		goto bad;

	/* Build old-style flat file iff one exists now. */
	cp = pw_path(pw, _PATH_PASSWD, NULL);
	if (stat(cp, &st)) {
		if (errno == ENOENT)
			pw->pw_flags &= ~PW_MAKEOLD;
		else
			goto bad;
	} else
		pw->pw_flags |= PW_MAKEOLD;

	if (pw->pw_flags & PW_MAKEOLD) {
		/* Build new old-style file, to be removed on error. */
		if ((cp = pw_path(pw, _PATH_PASSWD, ".tmp")) == NULL)
			goto bad;
		pw->pw_old.pf_name = cp;
		pw->pw_flags |= PW_RMOLD;
		(void)unlink(cp);
		fd = open(cp, O_WRONLY | O_CREAT | O_EXCL, PW_PERM_INSECURE);
		if (fd < 0) {
			warn("%s", cp);
			goto bad;
		}
		if ((fp = fdopen(fd, "w")) == NULL) {
			warn("%s", cp);
			(void)close(fd);
			goto bad;
		}
		pw->pw_old.pf_fp = fp;
	}

	/* Build new master file, to be removed on error. */
	if ((cp = pw_path(pw, _PATH_MASTERPASSWD, ".tmp")) == NULL)
		goto bad;
	pw->pw_new.pf_name = cp;
	pw->pw_flags |= PW_RMNEW;
	fd = open(cp, O_WRONLY | O_CREAT | O_EXCL, PW_PERM_SECURE);
	if (fd < 0) {
		warn("%s", cp);
		goto bad;
	}
	if ((fp = fdopen(fd, "w")) == NULL) {
		warn("%s", cp);
		(void)close(fd);
		goto bad;
	}
	pw->pw_new.pf_fp = fp;

	/* Phew, made it. */
	(void)umask(omask);
	pw->pw_flags |= PW_INPLACE;
	return (0);

bad:
	/* Something went wrong -- clean up. */
	(void)umask(omask);
	return (-1);
}

static int
pw_opendb(pw, pf, path)
	struct pwinfo *pw;
	struct pwf *pf;
	char *path;
{
	char *cp;

	if ((cp = pw_path(pw, path, NULL)) == NULL)
		return (-1);
	pf->pf_name = cp;
	/* No need for permissions; we just want to open existing db. */
	if ((pf->pf_db = dbopen(cp, O_RDWR, 0, DB_HASH, NULL)) == NULL) {
		warn("%s", cp);
		return (-1);
	}
	return (0);
}

/*
 * End in-place update.
 */
int
pw_ipend(pw)
	struct pwinfo *pw;
{
	struct pwf *pf;
	DB *db;
	int status = 0;

	assert(pw->pw_flags & PW_INPLACE);

	if (pw->pw_flags & PW_MAKEOLD) {
		pf = &pw->pw_old;
		if (fflush(pf->pf_fp) || fclose(pf->pf_fp)) {
			warn("%s", pf->pf_name);
			status = -1;
		}
		pf->pf_fp = NULL;
	}

	pf = &pw->pw_idb;
	db = pf->pf_db;
	if (db->close(db)) {
		warn("%s", pf->pf_name);
		status = -1;
	}
	pf->pf_db = NULL;

	pf = &pw->pw_sdb;
	db = pf->pf_db;
	if (db->close(db)) {
		warn("%s", pf->pf_name);
		status = -1;
	}
	pf->pf_db = NULL;

	/* The fsync is paranoia. */
	pf = &pw->pw_new;
	if (fflush(pf->pf_fp) || fsync(fileno(pf->pf_fp)) ||
	    fclose(pf->pf_fp)) {
		warn("%s", pf->pf_name);
		status = -1;
	}
	pf->pf_fp = NULL;

	pf = &pw->pw_master;
	if (pf->pf_fp != pw->pw_lock.pf_fp) {
		/* Different (or no) lock; close input. */
		(void)fclose(pf->pf_fp);
		pf->pf_fp = NULL;
	}

	return (status);
}
