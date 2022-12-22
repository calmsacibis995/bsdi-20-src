/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI susp.c,v 2.1 1995/02/03 06:56:45 polk Exp
 */

/*
 * This file implements cd_suf, part of the Rock Ridge API.  It allows
 * you to collect all of the Rock Ridge information for a given file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <iso9660/iso9660.h>

#include <sys/cdrom.h>

/* fsec is unused, because we don't support multiple section files */
int
cd_suf(path, fsec, signature, idx, buf, buflen)
	char *path;
	int fsec;
	char signature[2];
	int idx;
	char *buf;
	int buflen;
{
	char dirnamebuf[NAME_MAX + 1];
	char *dirname;
	struct iso9660_getsusp arg;
	int fd;
	int suoffset;
	struct susp *susp;
	struct stat statb;
	int susp_len;
	int len;

	static char rawbuf[10000];
	static int last_valid;
	static dev_t last_dot_dev;
	static ino_t last_dot_ino;
	static char *last_path;
	static int last_path_len;
	static int last_fsec;

	if (strlen(path) > NAME_MAX) {
		errno = ENAMETOOLONG;
		return (-1);
	}

	if (stat(".", &statb) < 0)
		return (-1);

	if (last_valid == 0
	    || statb.st_dev != last_dot_dev
	    || statb.st_ino != last_dot_ino
	    || last_path == NULL
	    || strcmp(path, last_path) != 0
	    || fsec != last_fsec) {
		last_valid = 0;

		last_dot_dev = statb.st_dev;
		last_dot_ino = statb.st_ino;
		last_fsec = fsec;

		if (stat(path, &statb) < 0)
			return (-1);

		if (S_ISDIR(statb.st_mode))
			dirname = path;
		else {
			if (strchr(path, '/')) {
				dirname = dirnamebuf;
				strcpy(dirname, path);
				*strrchr(dirname, '/') = 0;
			} else {
				dirname = ".";
			}
		}

		if ((fd = open(dirname, O_RDONLY)) < 0)
			return (-1);

		arg.name = path;
		arg.buf = rawbuf;
		arg.buflen = sizeof rawbuf;
		
		if (ioctl(fd, ISO9660GETSUSP, &arg) < 0) {
			close(fd);
			return (-1);
		}
	
		close(fd);

		len = strlen(path);
		if (len > last_path_len) {
			if (last_path_len == 0)
				last_path = malloc(len + 1);
			else
				last_path = realloc(last_path, len + 1);
			if (last_path == NULL) {
				errno = 0;
				return (-1);
			}
			last_path_len = len;
		}
		strcpy(last_path, path);
		last_valid = 1;
	}

	for (suoffset = 0; suoffset < arg.buflen; suoffset += susp_len) {
		susp = (struct susp *)(rawbuf + suoffset);
		susp_len = susp->length[0];

		if (signature) {
			if (susp->code[0] != signature[0]
			    || susp->code[1] != signature[1])
				continue;
		}

		if (--idx <= 0)
			break;
	}

	if (suoffset >= arg.buflen)
		return (0);

	len = susp_len;
	if (len > buflen)
		len = buflen;

	bcopy(susp, buf, len);

	return (len);
}

#ifdef TEST
main(argc, argv)
char **argv;
{
	int idx;
	char buf[10000];
	int n;

	idx = 1;

	while ((n = cd_suf(argv[1], 1, NULL, idx, buf, sizeof buf)) > 0) {
		printf("%c%c ", buf[0], buf[1]);
		idx++;
	}
	printf("\n");
}
#endif


