/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI misc.c,v 2.1 1995/02/03 09:19:25 polk Exp
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <paths.h>

int
pw_prompt()
{
	int c;

	(void)printf("re-edit the password file? [y]: ");
	(void)fflush(stdout);
	c = getchar();
	if (c != EOF && c != '\n')
		while (getchar() != '\n');
	return (c == 'n');
}

int
pw_edit(file)
	char *file;
{
	int pstat;
	pid_t pid;
	char *p, *editor;

	if (!(editor = getenv("EDITOR")))
		editor = _PATH_VI;
	if (p = strrchr(editor, '/'))
		++p;
	else 
		p = editor;

	if (!(pid = vfork())) {
#if 0
		if (notsetuid) {
			(void)setgid(getgid());
			(void)setuid(getuid());
		}
#endif
		execlp(editor, p, file, NULL);
		_exit(1);
	}
	pid = waitpid(pid, (int *)&pstat, 0);
	if (pid == -1 || !WIFEXITED(pstat) || WEXITSTATUS(pstat) != 0)
		return(1);
	return(0);
}
