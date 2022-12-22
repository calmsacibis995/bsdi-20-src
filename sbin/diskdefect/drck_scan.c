/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
-*/

#ifndef lint
static char rcsid[] = "@(#)BSDI drck_scan.c,v 2.1 1995/02/03 07:13:10 polk Exp";
#endif

/*-
 * drck_scan.c
 *
 * Usage:	int drck_scan(int fd,
 *                        struct disklabel *dp,
 *			  int verbose,
 *			  int wflag,
 *                        unsigned long int maxbad,
 *                        daddr_t *newscan);    [RETURN]
 *
 * Function:	Read a disk from one end to the other looking for
 *		bad blocks; bad sector numbers are returned in newscan[].
 *		The program reads the disk a cylinder at a time, and requires
 *              sufficient program memory to hold more than one disk cylinder.
 *
 * Returns:	count of bad sectors found
 *              list of sectors in newscan array
 *
 * Remarks:	Dd don't cut it.
 *
 * Author:	Donn Seeley
 * Date:	Sun Dec  7 19:27:24 MST 1986
 *
 * History:	08/03/92 Tony Sanders -- changed to a function.
 *		08/10/92 Tony Sanders -- Fixed bugs and updated.
 *
-*/

/*-
 * COMPILE TIME OPTIONS:
 *
 * -DDEBUG_WRITE	turns off write system call
-*/

/* NOT REENTRANT!!! */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "errlib.h"
#include "diskdefect.h"

/* global data */
static int cyl_buflen;
static off_t pos;
static off_t disk_size;
static char *buf = NULL;
static int foundscan = 0;

#define RAWINDEX (RAWPART - 'a')

/* for fancy busy indication */
#define BACK    ((verbose)?putchar('\b'):0)
#define MODE(c) ((verbose)?putchar(c), fflush(stdout):0)

#ifdef DEBUG_WRITE
static size_t
XWRITE(int d, const char *buf, size_t nbytes)
{
	return nbytes;		/* stubed out write call */
}

#else				/* DEBUG_WRITE */
#define XWRITE write
#endif				/* DEBUG_WRITE */

/*
 * Returns non-zero if it's safe to write the requested position (pos)
 * for size bytes.
 *
 * Just in case something might go horribly wrong, don't write on the
 * first track (disklabel) or the replacement sectors.
 * It's bad enough we might be trashing filesystems...
 */
int
safe_to_write(dp, pos, size)
	struct disklabel *dp;
	off_t pos;
	off_t size;
{
	/* reserve first track */
	u_long top = TRKSIZE;
	/* reserve one track + 126 sectors at end of disk */
	u_long reserve = disk_size - ((dp->d_nsectors + 126) * dp->d_secsize);

	return ((pos <= top || pos + size >= reserve) ? 0 : -1);
}

/*
 * Add sector (bad) to the bad sector list (newscan).
 * Make sure we don't overflow newscan.
 */
static void
mark_bad(bad, dp, verbose, maxbad, newscan)
	daddr_t bad;		/* bad sector number */
	struct disklabel *dp;	/* pointer to filled in disklabel struct */
	int verbose;
	u_long maxbad;		/* sizeof newscan array */
	daddr_t *newscan;	/* RETURN: pointer to array of sector
				 * numbers */
{
	if (!verbose)
		printf("Bad Block: sn=%ld, cyl=%ld, head=%ld, trk=%ld, sec=%ld\n",
		    SN(bad), CYL(bad), HEAD(bad), TRK(bad), SEC(bad));
	if (foundscan >= maxbad) {
		errno = E2BIG;
		Perror("Too many bad blocks");
	}
	newscan[foundscan++] = SN(bad);
}

/*
 * scanby_sector() is called from drck_scan() if it gets a bad read or write
 * to isolate the offending sector(s) and store them in newscan[].
 */
static void
scanby_sector(diskf, dp, verbose, wflag, maxbad, newscan)
	int diskf;		/* disk file descriptors */
	struct disklabel *dp;	/* pointer to filled in disklabel struct */
	int verbose;		/* verbose mode */
	int wflag;		/* write test flag */
	u_long maxbad;		/* sizeof newscan array */
	daddr_t *newscan;	/* RETURN: pointer to array of sector
				 * numbers */
{
	off_t cur_pos = pos;
	off_t cyl_end = pos + cyl_buflen;
	int founderr = 0;

	if (errno == ENXIO)
		Perror("Partition shorter than expected");
	if (errno != 0 && errno != EIO)
		Perror("Unknown read error while scanning by sector");

	/*
	 * scan by sectors
	 */
	for (; cur_pos < cyl_end; cur_pos += dp->d_secsize) {
		assert((cur_pos % dp->d_secsize) == 0);	/* sector boundry */
		BACK;
		MODE('/');
		if (lseek(diskf, cur_pos, L_SET) == (off_t) -1)
			Perror("Can't seek to offset %d", cur_pos);
		BACK;
		MODE('-');
		errno = 0;
		BACK;
		MODE('\\');
		if (read(diskf, buf, dp->d_secsize) != dp->d_secsize) {
			if (errno != EIO)
				Perror("Unknown read error scanning by cylinder");
			founderr++;
			mark_bad(cur_pos, dp, verbose, maxbad, newscan);
		} else if (wflag) {
			errno = 0;
			if (lseek(diskf, cur_pos, L_SET) == (off_t) -1)
				Perror("Can't seek to offset %d", cur_pos);
			if (safe_to_write(dp, cur_pos, dp->d_secsize)) {
				if (XWRITE(diskf, buf, dp->d_secsize) != dp->d_secsize) {
					founderr++;
					mark_bad(cur_pos, dp, verbose, maxbad, newscan);
				}
			}
		}
		BACK;
		MODE('|');
	}
	BACK;
	MODE(founderr ? 'x' : '.');
}

/*
 * Returns the size of the disk and prints warnings if things don't agree.
 */
static off_t
check_size(dp)
	struct disklabel *dp;
{
	/* various ways of computing disk size: */
	off_t rawsize = dp->d_partitions[RAWINDEX].p_size;
	off_t diskgeom = dp->d_nsectors * dp->d_ntracks * dp->d_ncylinders;
	off_t disklabel = dp->d_secperunit;

	if (rawsize != diskgeom) {
		printf("WARNING: raw partition size(%ld)", rawsize);
		printf(" does not match disk geometry(%ld)\n", diskgeom);
	}
	if (rawsize != disklabel) {
		printf("WARNING: raw partition size(%ld)", rawsize);
		printf(" does not match disk label(%ld)\n", disklabel);
	}
	return rawsize;
}

int
drck_scan(diskf, dp, verbose, wflag, maxbad, newscan)
	int diskf;		/* disk file descriptors */
	struct disklabel *dp;	/* pointer to filled in disklabel struct */
	int verbose;		/* verbose mode */
	int wflag;		/* write test flag */
	u_long maxbad;		/* sizeof newscan array */
	daddr_t *newscan;	/* RETURN: pointer to array of sectors */
{
	size_t nbytes;

	cyl_buflen = dp->d_secpercyl * dp->d_secsize;
	if (cyl_buflen == 0) {
		errno = EBADF;
		Perror("dp->d_secpercyl * dp->d_secsize == 0");
	}
	if ((buf = malloc((unsigned) cyl_buflen)) == NULL)
		Perror("malloc failed allocing %d bytes", cyl_buflen);

	/*
	 * scan the disk a cylinder at a time
	 */

	disk_size = check_size(dp) * dp->d_secsize;

	printf("Scanning disk for defects:");
	fflush(stdout);

	pos = 0;
	while (pos + cyl_buflen <= disk_size) {
		if (CYL(pos) % 100 == 0)
			printf("\ncylinder %5.5ld:", CYL(pos));
		if (verbose && (CYL(pos) % 50 == 0))
			putchar('\n');
		MODE('r');
		if (lseek(diskf, pos, L_SET) == (off_t) -1)
			Perror("Can't seek to offset %d", pos);
		errno = 0;
		if ((nbytes = read(diskf, buf, cyl_buflen)) != cyl_buflen) {
			/* read error: scan by sectors to find */
			scanby_sector(diskf, dp, verbose, wflag, maxbad, newscan);
		} else if (wflag) {
			if (lseek(diskf, pos, L_SET) == (off_t) -1)
				Perror("Can't seek to offset %d", pos);
			BACK;
			MODE('w');
			errno = 0;
			if (safe_to_write(dp, pos, cyl_buflen)) {
				if (XWRITE(diskf, buf, nbytes) != nbytes) {
					/*
					 * write error: scan by sectors to
					 * find
					 */
					scanby_sector(diskf, dp, verbose, wflag, maxbad, newscan);
				} else {
					BACK;
					MODE('.');
				}
			} else {
				/*
				 * Something wasn't safe to write. Scan
				 * sector by sector.
				 */
				errno = 0;	/* clear any errno just in
						 * case */
				scanby_sector(diskf, dp, verbose, wflag, maxbad, newscan);
			}
		} else {
			BACK;
			MODE('.');
		}
		pos += cyl_buflen;
	}

	free(buf);
	putchar('\n');

	return foundscan;
}
