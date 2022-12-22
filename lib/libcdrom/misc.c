/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI misc.c,v 2.1 1995/02/03 06:56:40 polk Exp
 */

/*
 * This file contains various helper functions for libcdrom.  It could
 * have gone in common.c, but putting these here avoids error messages
 * about cycles in tsort.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dev/scsi/scsi.h>
#include <dev/scsi/scsi_ioctl.h>

#include <sys/cdrom.h>
#include "devs.h"

/*
 * Convert between bcd values as used in the cdrom table of contents and
 * regular ints.
 */
int
bcd_to_int(val)
	int val;
{
	return (((val >> 4) & 0xf) * 10 + (val & 0xf));
}

int
int_to_bcd(val)
	int val;
{
	return (((val / 10) << 4) | (val % 10));
}

/*
 * Convert between frame (which is essentially a logical block number
 * from the start of the disk), and a (minute, second, frame) tuple.
 * The confusing terminology is inherited from the cd specs.
 */
int
msf_to_frame(minute, second, frame)
	int minute, second, frame;
{
	return (minute * FRAMES_PER_MINUTE + second * FRAMES_PER_SECOND +
	    frame);
}

void
frame_to_msf(frame, minp, secp, framep)
	int frame;
	int *minp;
	int *secp;
	int *framep;
{
	*minp = frame / FRAMES_PER_MINUTE;
	*secp = (frame % FRAMES_PER_MINUTE) / FRAMES_PER_SECOND;
	*framep = frame % FRAMES_PER_SECOND;
}

struct cdinfo *
make_cdinfo(fd, ntracks)
	int fd;
	int ntracks;
{
	struct cdinfo *cdinfo;

	if ((cdinfo = (struct cdinfo *)calloc(1, sizeof *cdinfo)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	cdinfo->fd = fd;
	cdinfo->ntracks = ntracks;
	cdinfo->tracks = (struct track_info *)
		calloc(ntracks, sizeof (struct track_info));

	if (cdinfo->tracks == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	return (cdinfo);
}

static void
scsi_strcpy (dest, src, len)
	char *dest;
	char *src;
	int len;
{
	while (len > 0 && src[len-1] == ' ')
		len--;

	strncpy (dest, src, len);
	dest[len] = 0;
}

int
scsi_inquiry(fd, vendor, ident, rev, vers)
	int fd;
	char *vendor;
	char *ident;
	char *rev;
	int *vers;
{
	char buf[36];
	char cdb[6];

	bzero (cdb, sizeof cdb);
	cdb[0] = 0x12;
	cdb[4] = sizeof buf;

	if (scsi_cmd (fd, cdb, buf, sizeof buf) < 0)
		return (-1);

	scsi_strcpy (vendor, buf + 8, 8);
	scsi_strcpy (ident, buf + 16, 16);
	scsi_strcpy (rev, buf + 32, 4);
	*vers = buf[2] & 7;
	return (0);
}

int
scsi_cmd(fd, cdb, buf, buflen)
	int fd;
	char *cdb;
	char *buf;
	int buflen;
{
	char ch;
	int read_buflen;
	int n;

	if (buflen == 0) {
		buf = &ch;
		read_buflen = 1;
	} else {
		read_buflen = buflen;
	}
		
	if (ioctl(fd, SDIOCSCSICOMMAND, (struct scsi_cdb *)cdb) < 0)
		return (-1);

	n = read(fd, buf, read_buflen);

	if (n != buflen && n != read_buflen)
		return (-1);

	return (0);
}

int
scsi_cmd_write(fd, cdb, buf, buflen)
	int fd;
	char *cdb;
	char *buf;
	int buflen;
{
	char ch;

	if (buflen == 0) {
		buf = &ch;
		buflen = 1;
	}
		
	if (ioctl(fd, SDIOCSCSICOMMAND, (struct scsi_cdb *)cdb) < 0)
		return (-1);

	if (write(fd, buf, buflen) != buflen)
		return (-1);

	return (0);
}

