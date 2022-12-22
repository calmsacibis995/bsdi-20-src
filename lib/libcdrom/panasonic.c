/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI panasonic.c,v 2.1 1995/02/03 06:56:42 polk Exp
 */

/* libcdrom driver for Panasonic CR-5XX rev 1.0c */

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

static int panasonic_commands[] = { 0xc3, 0xc7, 0xcb, 0xc2, -1 };

struct cdinfo *
panasonic_probe(fd, vendor, product, rev, vers)
	int fd;
	char *vendor;
	char *product;
	char *rev;
	int vers;
{
	int i;
	int val;
	char buf[1000];
	char cdb[16];
	int first_track, last_track;
	int ntracks;
	int track_num;
	struct track_info *tp;
	struct cdinfo *cdinfo;
	char *raw;

	if (strcmp (vendor, "MATSHITA") != 0)
		return (NULL);

	for (i = 0; panasonic_commands[i] >= 0; i++) {
		if (ioctl(fd, SDIOCADDCOMMAND, &panasonic_commands[i]) < 0) {
			perror("add scsi comand");
			return (NULL);
		}
	}

	/* get first and last track numbers */
	bzero(cdb, 10);
	cdb[0] = 0xc3;
	cdb[1] = 2; /* msf format */
	cdb[7] = sizeof buf >> 8;
	cdb[8] = sizeof buf & 0xff;
	if (ioctl(fd, SDIOCSCSICOMMAND, (struct scsi_cdb *)cdb) < 0) {
		perror("scsi command for read toc");
		return (NULL);
	}

	if (read(fd, buf, sizeof buf) < 4) {
		perror("scsi command read for first/last track num");
		return (NULL);
	}

	first_track = buf[2];
	last_track = buf[3];
	ntracks = last_track - first_track + 1;

	if ((cdinfo = make_cdinfo(fd, ntracks)) == NULL)
		return (NULL);
	
	for (track_num = first_track, tp = cdinfo->tracks, raw = buf + 4;
	     track_num <= last_track;
	     track_num++, tp++, raw += 8) {
		tp->track_num = track_num;
		
		tp->start_frame = msf_to_frame(raw[5], raw[6], raw[7]);
		tp->control = raw[1] & 0xff;
	}

	cdinfo->total_frames = msf_to_frame(raw[5], raw[6], raw[7]);
	
	return (cdinfo);
}

int
panasonic_close(cdinfo)
struct cdinfo *cdinfo;
{
	close(cdinfo->fd);
	free(cdinfo->tracks);
	free(cdinfo);
	return (0);
}

int
panasonic_play(cdinfo, start_frame, end_frame)
struct cdinfo *cdinfo;
int start_frame;
int end_frame;

{
	char cdb[16];
	char buf[100];
	int minute, second, frame;

	frame_to_msf(start_frame, &minute, &second, &frame);

	bzero(cdb, 10);
	cdb[0] = 0xc7;
	cdb[3] = minute;
	cdb[4] = second;
	cdb[5] = frame;

	frame_to_msf(end_frame, &minute, &second, &frame);
	cdb[6] = minute;
	cdb[7] = second;
	cdb[8] = frame;

	if (ioctl(cdinfo->fd, SDIOCSCSICOMMAND, (struct scsi_cdb *)cdb) < 0) {
		perror("scsi command: audio search");
		return (-1);
	}

	if (read(cdinfo->fd, buf, 1) != 1) {
		perror ("scsi command read for play msf");
		return (-1);
	}

	return (0);
}

int
panasonic_stop(cdinfo)
struct cdinfo *cdinfo;
{
	char cdb[10];

	bzero(cdb, 10);
	cdb[0] = 0xcb;

	if (scsi_cmd(cdinfo->fd, cdb, NULL, 0) < 0) {
		perror("scsi command for stop");
		return (-1);
	}
	return (0);
}

int
panasonic_status(cdinfo, cdstatus)
struct cdinfo *cdinfo;
struct cdstatus *cdstatus;
{
	char cdb[10];
	char buf[50];

	bzero(cdb, 10);
	cdb[0] = 0xc2;
	cdb[1] = 2;
	cdb[2] = 0x40;
	cdb[3] = 1; /* get current position */
	cdb[7] = sizeof buf >> 8;
	cdb[8] = sizeof buf;

	if (ioctl(cdinfo->fd, SDIOCSCSICOMMAND, (struct scsi_cdb *)cdb) < 0) {
		perror("scsi command get status");
		return (-1);
	}

	if (read(cdinfo->fd, buf, sizeof buf) != sizeof buf) {
		perror("scsi command read for get status");
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
		cdstatus->state = cdstate_stopped;
		break;
	default:
		cdstatus->state = cdstate_unknown;
		break;
	}

	cdstatus->control = buf[5] & 0xff;
	cdstatus->track_num = buf[6];
	cdstatus->index_num = buf[7];

	cdstatus->rel_frame = msf_to_frame(buf[13], buf[14], buf[15]);
	cdstatus->abs_frame = msf_to_frame(buf[9], buf[10], buf[11]);
	
	return (0);
}

int
panasonic_eject(cdinfo)
struct cdinfo *cdinfo;
{
	return (0);
}
	
int
panasonic_volume(cdinfo, volume)
struct cdinfo *cdinfo;
int volume;
{
	return (0);
}
