/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI lock.c,v 2.1 1995/02/03 09:19:21 polk Exp
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
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <paths.h>

#include "libpasswd.h"

/*
 * Lock against other changes.
 * On successful return, the master file is open for reading.
 */
int
pw_lock(pw, flags)
	struct pwinfo *pw;
	int flags;
{
	int fd, saverr;
	FILE *fp;

	assert((flags & ~O_NONBLOCK) == 0);
	/*
	 * Get an exclusive lock on /etc/master.passwd.  The C library
	 * takes a shared lock, so this coordinates properly.  Set the
	 * close-on-exec bit in the underlying file descriptor, so that
	 * if our caller runs other processes they cannot see the passwords.
	 */
	fd = open(_PATH_MASTERPASSWD, O_RDONLY | O_EXLOCK | flags, 0);
	if (fd < 0)
		return (fd);
	if (fcntl(fd, F_SETFD, 1) == -1 || (fp = fdopen(fd, "r")) == NULL) {
		saverr = errno;
		(void)close(fd);
		errno = saverr;
		fd = -1;
	} else {
		pw->pw_lock.pf_name = _PATH_MASTERPASSWD;
		pw->pw_lock.pf_fp = fp;
		pw->pw_flags |= PW_LOCKED;
	}
	return (fd);
}
