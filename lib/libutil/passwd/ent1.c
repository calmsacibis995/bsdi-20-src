/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ent1.c,v 2.1 1995/02/03 09:18:56 polk Exp
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

#include <assert.h>
#include <db.h>
#include <err.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "libpasswd.h"

/*
 * THE DATA FORMATS USED IN THIS FILE MUST MATCH THOSE IN THE
 * C LIBRARY getpwent.c CODE
 */

static int pw_compress __P((char *, struct passwd *, int));
static int pw_store1 __P((struct pwf *, DBT *, DBT *, int, void *, size_t));

/*
 * Enter a passwd line into the appropriate databases and flat files.
 * If old is NULL, this is a new entry, never been in the db before.
 * If old == new, this is an unchanged entry and is just being copied
 * to the flat files.  Otherwise this is a changed entry.
 */
int
pw_ent1(pw, old, new, secure)
	struct pwinfo *pw;
	struct passwd *old, *new;
	int secure;
{
	struct pwf *pf;
	DBT ndata, odata;
	char dbuf[LINE_MAX * 2], obuf[LINE_MAX * 2];

	/* Only need to work on the db files if new or changed entry. */
	if (old != new) {
		ndata.data = dbuf;
		ndata.size = pw_compress(dbuf, new, secure);
		pf = secure ? &pw->pw_sdb : &pw->pw_idb;

		if (old != NULL) {
#if 1 /* temporary ... */
			if (old->pw_uid != new->pw_uid) {
				warnx("cannot alter uids");
				return (-1);
			}
			if (strcmp(old->pw_name, new->pw_name)) {
				warnx("cannot alter user name");
				return (-1);
			}
#endif
			/*
			 * We assume same pw_name and pw_uid in old & new,
			 * and that all record numbers are unchanged.
			 */
			odata.data = obuf;
			odata.size = pw_compress(obuf, old, secure);
		} else {
			odata.data = NULL;
			odata.size = 0;
		}
		if (pw_store1(pf, &odata, &ndata, _PW_KEYBYNAME,
			    new->pw_name, strlen(new->pw_name)) < 0 ||
		    pw_store1(pf, &odata, &ndata, _PW_KEYBYNUM,
			    &pw->pw_line, sizeof pw->pw_line) < 0 ||
		    pw_store1(pf, &odata, &ndata, _PW_KEYBYUID,
			    &new->pw_uid, sizeof new->pw_uid) < 0)
			return (-1);
	}

	/* Always enter in old-style or master file, if making one. */
	if (secure) {
		pf = &pw->pw_new;
		if (pf->pf_fp != NULL) {
			pf = &pw->pw_new;
			if (fprintf(pf->pf_fp,
			    "%s:%s:%ld:%ld:%s:%ld:%ld:%s:%s:%s\n",
			     new->pw_name, new->pw_passwd,
			     (long)new->pw_uid, (long)new->pw_gid,
			     new->pw_class,
			     (long)new->pw_change, (long)new->pw_expire,
			     new->pw_gecos, new->pw_dir, new->pw_shell) < 0) {
				warn("%s", pf->pf_name);
				return (-1);
			}
		}
	} else {
		pf = &pw->pw_old;
		if (pf->pf_fp != NULL) {
			pf = &pw->pw_old;
			if (fprintf(pf->pf_fp, "%s:*:%ld:%ld:%s:%s:%s\n",
			    new->pw_name, (long)new->pw_uid, (long)new->pw_gid,
			    new->pw_gecos, new->pw_dir, new->pw_shell) < 0) {
				warn("%s", pf->pf_name);
				return (-1);
			}
		}
	}
	return (0);
}

static int
pw_store1(pf, odp, ndp, kcode, kaddr, klen)
	struct pwf *pf;
	DBT *odp, *ndp;
	int kcode;
	void *kaddr;
	size_t klen;
{
	int r;
	DB *db;
	DBT key, data;
	char kbuf[1024];

	/* Build the key and its descriptor. */
	kbuf[0] = kcode;
	memmove(&kbuf[1], kaddr, klen);
	key.data = kbuf;
	key.size = klen + 1;

	db = pf->pf_db;
	if (odp->data) {
		r = db->get(db, &key, &data, 0);
		if (r < 0) {
			warn("%s: get", pf->pf_name);
			return (r);		/* error */
		}
		if (r == 0 && data.size == odp->size &&
		    memcmp(data.data, odp->data, data.size) != 0)
			return (0);		/* do not replace */
		r = db->put(db, &key, ndp, 0);
	} else
		r = db->put(db, &key, ndp, R_NOOVERWRITE);
	if (r < 0)
		warn("%s: put", pf->pf_name);
	return (r);
}

static int
pw_compress(buf, pw, secure)
	char *buf;
	register struct passwd *pw;
	int secure;
{
	register char *p, *t;

#define	COMPACT(e)	t = e; while ((*p++ = *t++) != '\0')
#define	SCALAR(v)	memmove(p, &(v), sizeof v); p += sizeof v
	/*
	 * This compression code must match the corresponding
	 * expansion code in libc.
	 */
	p = buf;
	COMPACT(pw->pw_name);
	COMPACT(secure ? pw->pw_passwd : "*");
	SCALAR(pw->pw_uid);
	SCALAR(pw->pw_gid);
	SCALAR(pw->pw_change);
	COMPACT(pw->pw_class); 
	COMPACT(pw->pw_gecos);
	COMPACT(pw->pw_dir); 
	COMPACT(pw->pw_shell); 
	SCALAR(pw->pw_expire);

	return (p - buf);
}
