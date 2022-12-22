/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI copy.c,v 2.1 1995/02/03 09:18:52 polk Exp
 */
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

static int
dupfield(char **f)
{
    return(*f == NULL || (*f = strdup(*f)) != NULL);
}

struct passwd *
pw_copy(pwd)
	struct passwd *pwd;
{
	struct passwd *new = (struct passwd *)malloc(sizeof(struct passwd));

    	if (new) {
		*new = *pwd;
		if (!dupfield(&new->pw_name) ||
		    !dupfield(&new->pw_passwd) ||
		    !dupfield(&new->pw_class) ||
		    !dupfield(&new->pw_gecos) ||
		    !dupfield(&new->pw_dir) ||
		    !dupfield(&new->pw_shell))
			new = 0;
	}
	return(new);
}
