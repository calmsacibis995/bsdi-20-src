/*	BSDI mntopts.h,v 2.1 1995/02/03 07:22:07 polk Exp	*/

/*-
 * Copyright (c) 1994
 *      The Regents of the University of California.  All rights reserved.
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
 *
 *	@(#)mntopts.h	8.3 (Berkeley) 3/27/94
 */

struct mntopt {
	const char *m_option;	/* option name */
	int m_inverse;		/* if a negative option, eg "dev" */
	int m_flag;		/* bit to set, eg. MNT_RDONLY */
	char **m_value;		/* where to put the value */
	int *m_flagp;		/* where to put the flag */
};

/* User-visible MNT_ flags. */
#define MOPT_ASYNC		{ "async",	0, MNT_ASYNC, }
#define MOPT_NODEV		{ "dev",	1, MNT_NODEV, }
#define MOPT_NOEXEC		{ "exec",	1, MNT_NOEXEC, }
#define MOPT_NOSUID		{ "suid",	1, MNT_NOSUID, }
#define MOPT_RDONLY		{ "rdonly",	0, MNT_RDONLY, }
#define MOPT_SYNC		{ "sync",	0, MNT_SYNCHRONOUS, }
#define MOPT_UNION		{ "union",	0, MNT_UNION, }

/* Control flags. */
#define MOPT_FORCE		{ "force",	1, MNT_FORCE, }
#define MOPT_UPDATE		{ "update",	0, MNT_UPDATE, }

/* Support for old-style "ro", "rw" flags. */
#define MOPT_RO			{ "ro",		0, MNT_RDONLY, }
#define MOPT_RW			{ "rw",		1, MNT_RDONLY, }

/* Ignored options (used for control in fstab) */
#define MOPT_NOAUTO		{ "na", }
#define MOPT_USERQUOTA		{ "userquota", }
#define MOPT_GROUPQUOTA		{ "groupquota", }

#define MOPT_FSTAB_COMPAT						\
	MOPT_RO,							\
	MOPT_RW

/* Standard options which all mounts can understand. */
#define MOPT_STDOPTS							\
	MOPT_NOAUTO,							\
	MOPT_USERQUOTA,							\
	MOPT_GROUPQUOTA,						\
	MOPT_FSTAB_COMPAT,						\
	MOPT_NODEV,							\
	MOPT_NOEXEC,							\
	MOPT_NOSUID,							\
	MOPT_RDONLY,							\
	MOPT_UNION

void getmntopts __P((const char *, const struct mntopt *, int *));
