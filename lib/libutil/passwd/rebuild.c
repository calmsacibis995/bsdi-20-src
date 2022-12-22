/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI rebuild.c,v 2.1 1995/02/03 09:19:36 polk Exp
 */

/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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

static int pw_rbend __P((struct pwinfo *, int));
static int pw_rbpass __P((struct pwinfo *, int, u_int));

/*
 * Set up to rebuild the databases from scratch.
 * Need input file name.  No new files are created just yet; see pw_rbpass.
 */
int
pw_rebuild(pw, master, cachesize)
	struct pwinfo *pw;
	char *master;
	u_int cachesize;
{
	int errflag, omask, pass;
	FILE *fp;
	struct passwd pwd;

	assert((pw->pw_flags & (PW_INPLACE | PW_REBUILD)) == 0);
	/*
	 * This is similar to pw_lock, but we do not need to lock the file.
	 * We will go ahead and set close-on-exec, out of sheer paranoia.
	 */
	if ((fp = fopen(master, "r")) == NULL) {
		warn("cannot read %s", master);
		return (-1);
	}
	if (fcntl(fileno(fp), F_SETFD, 1) < 0) {
		warn("set %s close-on-exec", master);
		(void)fclose(fp);
		return (-1);
	}
	(void)fchmod(fileno(fp), S_IRUSR | S_IWUSR);	/* force security */
	pw->pw_master.pf_name = master;
	pw->pw_master.pf_fp = fp;

	/* Just construct the names -- we build the files later. */
	pw->pw_idb.pf_name = pw_path(pw, _PATH_MP_DB, ".tmp");
	pw->pw_sdb.pf_name = pw_path(pw, _PATH_SMP_DB, ".tmp");
	if (pw->pw_flags & PW_MAKEOLD)
		pw->pw_old.pf_name = pw_path(pw, _PATH_PASSWD, ".tmp");
	pw->pw_flags |= PW_REBUILD;

	/* Use exact permissions. */
	omask = umask(0);

	/* Now run two rebuild passes; 0 does insecure, 1 does secure. */
	for (pass = 0; pass < 2; pass++) {
		if (pw_rbpass(pw, pass, cachesize))
			goto bad;
		while (pw_next(pw, &pwd, &errflag))
			if (pw_ent1(pw, NULL, &pwd, pass))
				goto bad;
		if (errflag || pw_rbend(pw, pass))
			goto bad;
	}

	(void)umask(omask);
	return (0);

bad:
	(void)umask(omask);
	return (-1);
}

static HASHINFO openinfo = {
	4096,		/* bsize */
	32,		/* ffactor */
	256,		/* nelem */
	0,		/* cachesize -- set in rbpass */
	NULL,		/* hash() */
	0		/* lorder */
};

/*
 * Set up for a rebuild pass (insecure or secure, as indicated).
 */
static int
pw_rbpass(pw, secure, cachesize)
	struct pwinfo *pw;
	int secure;
	u_int cachesize;
{
	int fd, perm;
	char *cp;
	DB *db;
	FILE *fp;
	struct pwf *pf;

	/* Create the database. */
	openinfo.cachesize = (cachesize ? cachesize : 1024) * 1024;
	if (!secure) {
		/* Create insecure database, and maybe old-style flat file. */
		if (pw->pw_flags & PW_MAKEOLD) {
			cp = pw->pw_old.pf_name;
			pw->pw_flags |= PW_RMOLD;
			fd = open(cp, O_WRONLY | O_CREAT | O_EXCL,
			    PW_PERM_INSECURE);
			if (fd < 0) {
				warn("cannot create %s", cp);
				return (-1);
			}
			if ((fp = fdopen(fd, "w")) == NULL) {
				warn("%s", cp);
				(void)close(fd);
				return (-1);
			}
			pw->pw_old.pf_fp = fp;
		}
		pf = &pw->pw_idb;
		perm = PW_PERM_INSECURE;
		pw->pw_flags |= PW_RMIDB;
	} else {
		/* Create secure database; no flat file. */
		pf = &pw->pw_sdb;
		perm = PW_PERM_SECURE;
		pw->pw_flags |= PW_RMSDB;
	}
	(void)unlink(pf->pf_name);
	db = dbopen(pf->pf_name, O_RDWR | O_CREAT | O_EXCL, perm,
	    DB_HASH, &openinfo);
	if (db == NULL) {
		warn("cannot create %s", pf->pf_name);
		return (-1);
	}
	pf->pf_db = db;

	/* Start at the beginning of the input master file. */
	rewind(pw->pw_master.pf_fp);
	pw->pw_line = 0;

	/* Ready to go. */
	return (0);
}

/*
 * End rebuild pass.
 */
static int
pw_rbend(pw, secure)
	struct pwinfo *pw;
	int secure;
{
	struct pwf *pf;
	DB *db;
	int status = 0;

	if (!secure) {
		if (pw->pw_flags & PW_MAKEOLD) {
			pf = &pw->pw_old;
			if (fflush(pf->pf_fp) || fclose(pf->pf_fp)) {
				warn("%s", pf->pf_name);
				status = -1;
			}
			pf->pf_fp = NULL;
		}
		pf = &pw->pw_idb;
	} else {
		pf = &pw->pw_sdb;
	}
	db = pf->pf_db;
	if (db->close(db)) {
		warn("%s", pf->pf_name);
		status = -1;
	}
	pf->pf_db = NULL;

	return (status);
}
