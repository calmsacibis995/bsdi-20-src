/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI mitsumi.c,v 2.1 1995/02/03 06:56:41 polk Exp
 */

/*
 * This file contains device specific code for the Mitsumi CRMC cd rom drive.
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

#include <i386/isa/mcdioctl.h>

#include <sys/cdrom.h>
#include "devs.h"

static int
mitsumi_cmd(fd, cmdbytes, cmdlen, results, resultslen)
	int fd;
	char *cmdbytes;
	int cmdlen;
	char *results;
	int resultslen;
{
	struct mcd_cmd cmd;
	bcopy(cmdbytes, cmd.args, cmdlen);
	cmd.nargs = cmdlen;
	cmd.results = results;
	cmd.nresults = resultslen;

	if (ioctl(fd, MCDIOCCMD, &cmd) < 0)
		return (-1);

	return (0);
}

struct cdinfo *
mitsumi_probe(fd, vendor, product, rev, vers)
	int fd;
	char *vendor;
	char *product;
	char *rev;
	int vers;
{
	int first_track_num, last_track_num;
	int ntracks;
	int track_num;
	struct track_info *tp;
	char args[20];
	char buf[20];
	int i;
	struct cdinfo *cdinfo;

	args[0] = 0x40; /* request status */
	if (mitsumi_cmd(fd, args, 1, buf, 1) < 0)
		return (NULL);

	if ((buf[0] & 0x08) == 0) {
		fprintf(stderr,"can't read table of contents for data disk\n");
		return (NULL);
	}

	args[0] = 0x50;	/* mode set */
	args[1] = 5;	/* enable toc */
	if (mitsumi_cmd(fd, args, 2, buf, 1) < 0)
		return (NULL);

	/* get first and last track numbers */
	args[0] = 0x10;
	mitsumi_cmd(fd, args, 1, buf, 9);

	first_track_num = bcd_to_int(buf[1]);
	last_track_num = bcd_to_int(buf[2]);
	ntracks = last_track_num - first_track_num + 1;

	if ((cdinfo = make_cdinfo(fd, ntracks)) == NULL)
		return (NULL);

	cdinfo->total_frames = msf_to_frame(bcd_to_int(buf[3]),
	    bcd_to_int(buf[4]), bcd_to_int(buf[5]));

	/* start reading toc area */
	bzero(args, 7);
	args[0] = 0xc0;
	args[5] = 2;
	mitsumi_cmd(fd, args, 7, buf, 1);

	while (1) {
		/* get current sub-q info */
		args[0] = 0x20;
		bzero(buf, 11);
		mitsumi_cmd(fd, args, 1, buf, 11);

		track_num = bcd_to_int(buf[3]);

		if (track_num < first_track_num || track_num > last_track_num)
			continue;

		if ((buf[1] & 0xf) != 1)
			continue;

		tp = &cdinfo->tracks[track_num - first_track_num];
		
		if (tp->initialized)
			continue;

		tp->initialized = 1;
		tp->track_num = track_num;
		tp->start_frame = msf_to_frame(bcd_to_int(buf[8]),
		    bcd_to_int(buf[9]), bcd_to_int(buf[10]));
		tp->control = buf[1];

		/* break when we've seen an entry for every track */
		for (i = 0, tp = cdinfo->tracks; i < ntracks; i++, tp++) 
			if (tp->initialized == 0)
				break;

		if (i == ntracks)
			break;
	}

	args[0] = 0x50;	/* mode set */
	args[1] = 1;	/* disable toc mode */
	mitsumi_cmd(fd, args, 2, buf, 1);

	return (cdinfo);
}

int
mitsumi_close(cdinfo)
	struct cdinfo *cdinfo;
{
	close(cdinfo->fd);
	free(cdinfo->tracks);
	free(cdinfo);
	return (0);
}

int
mitsumi_play(cdinfo, start_frame, end_frame)
	struct cdinfo *cdinfo;
	int start_frame;
	int end_frame;
{
	char args[20];
	char buf[20];
	int minute, second, frame;

	args[0] = 0xc0;
	
	frame_to_msf(start_frame, &minute, &second, &frame);
	args[1] = int_to_bcd(minute);
	args[2] = int_to_bcd(second);
	args[3] = int_to_bcd(frame);

	frame_to_msf(end_frame, &minute, &second, &frame);
	args[4] = int_to_bcd(minute);
	args[5] = int_to_bcd(second);
	args[6] = int_to_bcd(frame);

	mitsumi_cmd(cdinfo->fd, args, 7, buf, 1);
	return (0);
}

int
mitsumi_stop(cdinfo)
	struct cdinfo *cdinfo;
{
	char args[20];
	char buf[20];

	args[0] = 0xf0;
	mitsumi_cmd(cdinfo->fd, args, 1, buf, 1);
	return (0);
}

int
mitsumi_status(cdinfo, cdstatus)
	struct cdinfo *cdinfo;
	struct cdstatus *cdstatus;
{
	char args[20];
	unsigned char buf[20];

	args[0] = 0x20;
	mitsumi_cmd(cdinfo->fd, args, 1, (char *)buf, 11);

	if (buf[0] & 2)
		cdstatus->state = cdstate_playing;
	else
		cdstatus->state = cdstate_stopped;

	cdstatus->control = buf[1] & 0xff;
	cdstatus->track_num = bcd_to_int(buf[2]);
	cdstatus->index_num = bcd_to_int(buf[3]);
	cdstatus->rel_frame = msf_to_frame(bcd_to_int(buf[4]),
	    bcd_to_int(buf[5]), bcd_to_int(buf[6]));
	cdstatus->abs_frame = msf_to_frame(bcd_to_int(buf[8]),
	    bcd_to_int(buf[9]), bcd_to_int(buf[10]));
	return (0);
}

int
mitsumi_eject(cdinfo)
struct cdinfo *cdinfo;
{
	return (0);
}

/*
 * The command is implemented in the drive, but it seems to have no effect.
 * I can read back the values I write, but the audio level stays the same.
 * The DOS program does not provide a way to execute this command.
 */
int
mitsumi_volume(cdinfo, volume)
	struct cdinfo *cdinfo;
	int volume;
{
	char args[20];
	char buf[20];
	int val;

	val = 255 - (volume * 255 / 100);
	if (val <= 0)
		val = 1;
	if (val > 255)
		val = 255;

	bzero(args, sizeof args);
	args[0] = 0xae;
	args[1] = val;
	args[3] = val;
	mitsumi_cmd(cdinfo->fd, args, 5, (char *)buf, 5);
	if (buf[0] & 1)
		fprintf(stderr, "error setting volume: %x\n", buf[0] & 0xff);

	return (0);
}
