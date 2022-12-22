/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI edit.cc,v 2.1 1995/02/03 07:14:49 polk Exp	*/

#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include "fs.h"
#include <paths.h>
#include <sys/wait.h>
#include "showhelp.h"
#include "screen.h"
#include "help.h"
#include "disk.h"
#include "field.h"
#include "util.h"


char tmpfil[] = _PATH_TMP "disksetup.XXXXXX";
#define DEFEDITOR       "/usr/bin/vi"

int editit();

/*
 * Edit a label using your favorite editor
 */
edit()
{
    FILE *fp;
    char *ans;

    mktemp(tmpfil);
    if ((fp = fopen(tmpfil, "w")) == NULL) {
	unlink(tmpfil);
	err(1, "opening temporary file %s", tmpfil);
    }

    if (!disk.label_original.Valid() || disk.label_original.Soft()) {
	fprintf(fp, "# Missing disk label, default label used.\n");
	fprintf(fp, "# Geometry may be incorrect.\n");
    }   

    disklabel_display(disk.device, fp, &disk.label_original, disk.PartTable());
    fclose(fp);

    for (;;) {
	if (!editit()) {
	    unlink(tmpfil);
	    exit(1);
	}
	if ((fp = fopen(tmpfil, "r")) == NULL) {
	    unlink(tmpfil);
	    err(1, "opening temporary file %s", tmpfil);
	}
	disk.label_new.Clean();
	if (disklabel_getasciilabel(fp, &disk.label_new, 0))
	    break;
	print(edit_invalid);
	if (request_yesno(edit_redit, 1) == 0) {
	    unlink(tmpfil);
	    print(edit_invalid_exit);
	    exit(1);
	}
    }
    unlink(tmpfil);
    return (1);
}

editit()
{
    int pid, xpid;
    int stat, omask;

    omask = sigblock(sigmask(SIGINT) | sigmask(SIGQUIT) | sigmask(SIGHUP));
    while ((pid = fork()) < 0) {
	if (errno == EPROCLIM) {
	    fprintf(stderr, "You have too many processes\n");
	    return (0);
	}
	if (errno != EAGAIN) {
	    warn("creating edit process");
	    return (0);
	}
	sleep(1);
    }

    if (pid == 0) {
	register char *ed;

	sigsetmask(omask);
	setgid(getgid());
	setuid(getuid());
	if ((ed = getenv("EDITOR")) == (char *) 0)
		ed = DEFEDITOR;
	execlp(ed, ed, tmpfil, 0);
	err(1, "Executing editor %s", ed);
    }
    while ((xpid = wait(&stat)) >= 0)
	    if (xpid == pid)
		    break;
    sigsetmask(omask);
    return (!stat);
}
