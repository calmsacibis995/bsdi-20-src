/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI scsi2.c,v 2.1 1995/02/03 06:56:44 polk Exp
 */

/*
 * This file contains the device specific code for the SCSI-2 audio commands.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dev/scsi/scsi.h>
#include <dev/scsi/scsi_ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/cdrom.h>
#include "devs.h"

/* These are the extra scsi commands we need. */

static int scsi2_commands[] = { 0x43, 0x47, 0x4b, 0x42, 0x48, -1 };

struct cdinfo *
scsi2_probe(fd, vendor, product, rev, vers)
	int fd;
	char *vendor;
	char *product;
	char *rev;
	int vers;
{
	int i;
	char buf[100];
	char cdb[16];
	int first_track, last_track;
	int ntracks;
	int track_num;
	struct track_info *tp;
	struct cdinfo *cdinfo;
	int scsi_toc_size;
	char *scsi_toc;
	char *p;

	if (*vendor == 0)
		return (NULL); /* not a scsi drive */

	/* check for SCSI-2 */
	if (vers < 2)
		return (NULL);

	for (i = 0; scsi2_commands[i] >= 0; i++) {
		if (ioctl(fd, SDIOCADDCOMMAND, &scsi2_commands[i]) < 0) {
			perror("add scsi comand");
			return (NULL);
		}
	}

	/* first, find out how many tracks there are */
	bzero(cdb, 10);
	cdb[0] = 0x43;
	cdb[1] = 2;
	cdb[8] = 4;
	if (scsi_cmd(fd, cdb, buf, 4) < 0)
		/* drive doesn't support scsi2 or no disk */
		return (NULL);

	first_track = buf[2] & 0xff;
	last_track = buf[3] & 0xff;
	ntracks = last_track - first_track + 1;
	
	scsi_toc_size = 4 + (ntracks + 1) * 8;
	scsi_toc = malloc(scsi_toc_size);

	bzero(cdb, 10);
	cdb[0] = 0x43;
	cdb[1] = 2; /* msf format */
	cdb[6] = first_track;
	cdb[7] = scsi_toc_size >> 8;
	cdb[8] = scsi_toc_size;
	if (scsi_cmd(fd, cdb, scsi_toc, scsi_toc_size) < 0) {
		perror("scsi command for toc data");
		return (NULL);
	}
	
	if ((cdinfo = make_cdinfo(fd, ntracks)) == NULL)
		return (NULL);

	for (track_num = first_track, tp = cdinfo->tracks, p = scsi_toc + 4;
	     track_num <= last_track; 
	     track_num++, tp++, p += 8) {
		tp->track_num = track_num;
		tp->start_frame = msf_to_frame(p[5] & 0xff, p[6] & 0xff,
		    p[7] & 0xff);
		tp->control = p[1] & 0xff;
	}

	cdinfo->total_frames = msf_to_frame(p[5] & 0xff, p[6] & 0xff,
	    p[7] & 0xff);

	scsi2_volume(cdinfo, 100);

	return (cdinfo);
}

int
scsi2_close(cdinfo)
	struct cdinfo *cdinfo;
{
	close(cdinfo->fd);
	free(cdinfo->tracks);
	free(cdinfo);
	return (0);
}

int
scsi2_play(cdinfo, start_frame, end_frame)
	struct cdinfo *cdinfo;
	int start_frame;
	int end_frame;
{
	int minute, second, frame;
	char cdb[10];

	bzero(cdb, 10);
	cdb[0] = 0x47;

	frame_to_msf(start_frame, &minute, &second, &frame);
	cdb[3] = minute;
	cdb[4] = second;
	cdb[5] = frame;

	frame_to_msf(end_frame - 1, &minute, &second, &frame);
	cdb[6] = minute;
	cdb[7] = second;
	cdb[8] = frame;

	if (scsi_cmd(cdinfo->fd, cdb, NULL, 0) < 0) {
		perror("scsi command for play");
		return (-1);
	}

	return (0);
}

int
scsi2_stop(cdinfo)
	struct cdinfo *cdinfo;
{
	char cdb[10];

	bzero(cdb, 10);
	cdb[0] = 0x4b;
	if (scsi_cmd(cdinfo->fd, cdb, NULL, 0) < 0) {
		perror("scsi command for stop");
		return (-1);
	}
	return (0);
}

int
scsi2_status(cdinfo, cdstatus)
	struct cdinfo *cdinfo;
	struct cdstatus *cdstatus;
{
	char cdb[10];
	char buf[16];

	bzero(cdb, 10);
	cdb[0] = 0x42;
	cdb[1] = 2; /* msf format */
	cdb[2] = 0x40; /* request q channel */
	cdb[3] = 1; /* current position */
	cdb[7] = sizeof buf >> 8;
	cdb[8] = sizeof buf;

	if (scsi_cmd(cdinfo->fd, cdb, buf, sizeof buf) < 0) {
		perror("scsi command for status");
		return (-1);
	}

	switch (buf[1]) {
	case 0x11:
		cdstatus->state = cdstate_playing;
		break;
	case 0x12:
		cdstatus->state = cdstate_paused;
		break;
	case 0x13:
	case 0x14:
	case 0x15:
	default:
		cdstatus->state = cdstate_stopped;
		break;
	}

	cdstatus->control = buf[5] & 0xff;
	cdstatus->track_num = buf[6] & 0xff;
	cdstatus->index_num = buf[7] & 0xff;
	cdstatus->abs_frame = msf_to_frame(buf[9] & 0xff, buf[10] & 0xff,
	    buf[11] & 0xff);
	cdstatus->rel_frame = msf_to_frame(buf[13] & 0xff, buf[14] & 0xff,
	    buf[15] & 0xff);

	return (0);
}

int
scsi2_eject(cdinfo)
	struct cdinfo *cdinfo;
{
	char cdb[6];

	bzero(cdb, 6);
	cdb[0] = 0x1b;
	cdb[1] = 1;
	cdb[4] = 2;
	if (scsi_cmd(cdinfo->fd, cdb, NULL, 0) < 0) {
		perror("scsi command for eject");
		return (-1);
	}
	return (0);
}
	
/* volume is 0 .. 100 */
int
scsi2_volume(cdinfo, volume)
	struct cdinfo *cdinfo;
	int volume;
{
	char cdb[10];
	char buf[100];
	int val;

	val = (volume * 255) / 100;
	if (val < 0)
		val = 0;
	if (val > 255)
		val = 255;

	/* mode select */
	bzero(cdb, 6);
	cdb[0] = 0x15;
	cdb[1] = 0x10;
	cdb[4] = 20;

	bzero(buf, sizeof buf);
	buf[0] = 0; /* param len for mode sense */
	buf[1] = 0; /* medium type */
	buf[2] = 0; /* dev specific params */
	buf[3] = 0; /* block descriptor length */

	buf[4] = 0xe; /* page code */
	buf[5] = 14;
	buf[6] = 4;
	buf[12] = 1;
	buf[13] = val;
	buf[14] = 2;
	buf[15] = val;

	if (scsi_cmd_write(cdinfo->fd, cdb, buf, 18) < 0) {
		perror("scsi command: mode select");
		return (-1);
	}

	return (0);
}

	
