/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI tmp.c,v 2.1 1995/02/03 09:19:49 polk Exp
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <db.h> 
#include <err.h> 
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
		
#include "libpasswd.h"
		
int
pw_tmp(pw, path, plen)
	struct pwinfo *pw;
	char *path;
	int plen;
{
	int fd;
	char *p; 
	
	if ((pw->pw_flags & PW_STRIPDIR) &&
	    (p = strrchr(_PATH_MASTERPASSWD, '/')) != NULL)
		++p;
	else
		p = _PATH_MASTERPASSWD;

    	strncpy(path, p, plen);

	if (p = strrchr(path, '/'))
		++p;
	else
		p = path;
	strncpy(p, "pw.XXXXXX", plen - (p - path));
	if ((fd = mkstemp(path)) == -1)
		err(1, "%s", path);
    	(void)fchmod(fd, PW_PERM_SECURE);
	return (fd);
}
