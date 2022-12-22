/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
-*/

#ifndef lint
static char sccsid[] = "@(#)diskdefect.c	5.19 (Berkeley) 4/11/91";
static char rcsid[] = "@(#)BSDI diskdefect.c,v 2.1 1995/02/03 07:12:59 polk Exp";
static char copyright[] =
"@(#) Copyright (c) 1980,1986,1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif

/*-
 * Copyright (c) 1980,1986,1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
-*/

/*-
 * diskdefect.c
 *
 * Usage:	diskdefect [-cswnvy] [-b 01234] disk [disk-snum [bad-block ...]]
 *       	diskdefect -a [-cswnvy] [-b 01234] disk [bad-block ...]
 *
 * Function:	This program prints and/or initializes a bad-block record
 *              for a pack, in the format used by the DEC standard 144.
 * 		It can also add bad sector(s) to the record, moving the
 * 		sector replacements as necessary.
 *
 * 		It is preferable to write the bad information with a
 * 		standard formatter, but this program will do.
 *
 * Author:	Unknown
 * Date:	Unknown
 *
 * Remarks:	The -w (write) option should be used with care.
 * See Also:    drck(8), bad144(8)
 * History:	08/03/92 Tony Sanders -- integrated drck style scan
 *         	08/10/92 Tony Sanders -- fixed bugs, updated, removed -S,
 *                       removed vax option, redid verbose display
 *              12/17/92 Tony Sanders -- cleanup and bug fixes
 *		02/19/92 Tony Sanders -- check for existing bad-block tbl
-*/

#include <sys/param.h>
#include <sys/dkbad.h>
#include <sys/ioctl.h>
#include <ufs/ffs/fs.h>
#include <sys/disklabel.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "errlib.h"
#include "diskdefect.h"

int wflag;
int nflag;
int add;
int copy;
int verbose;			/* used by message() among others */
int scan;
int force_yes;

int badfile = -1;		/* copy of badsector table to use */
struct dkbad curbad;
struct dkbad newbad;
struct dkbad replbad;

daddr_t disk_size;		/* size of disk in sectors */
char name[BUFSIZ];
daddr_t newscan[MAXBAD];
int foundscan = 0;

typedef struct disklabel dl_t;

#if defined(DIOCGDINFO)
dl_t dl;
dl_t *dp = &dl;
#else				/* DIOCGDINFO */
char label[BBSIZE];
dl_t *dp;
#endif				/* DIOCGDINFO */

/*
 * used by qsort() to order the bad sector list
 */
int
bynum(a, b)
	register daddr_t *a, *b;
{
	return *a - *b;
}

/*
 * print out a usage message
 */
void
usage()
{
	char *opts = "[-cswnvy] [-b 01234]";

	fprintf(stderr, "usage:  diskdefect %s disk [disk-snum [bad-block ...]]\n", opts);
	fprintf(stderr, "\t\tread or overwrite bad-sector table, e.g.: diskdefect wd0\n");
	fprintf(stderr, "   or:  diskdefect -a %s disk [bad-block ...]\n", opts);
	fprintf(stderr, "\t\tadd to bad-sector table.\n");
	fprintf(stderr, "\noptions are:\n");
	fprintf(stderr, "\tdisk      name of disk (e.g. wd0)\n");
	fprintf(stderr, "\tdisk-snum new disk serial number\n");
	fprintf(stderr, "\tbad-block block numbers to add to bad-block table\n");
	fprintf(stderr, "\t-a        add new bad sectors to the table\n");
	fprintf(stderr, "\t-c        copy original sector to replacement\n");
	fprintf(stderr, "\t-s        scan disk for bad sectors (read only unless -w)\n");
	fprintf(stderr, "\t-w        enable write test in scan (off by default)\n");
	fprintf(stderr, "\t-n        do not write out bad-block table (test mode, enables verbose)\n");
	fprintf(stderr, "\t-v        verbose mode\n");
	fprintf(stderr, "\t-y        don't ask any questions (assume yes)\n");
	fprintf(stderr, "\t-b 0-4    select bad-block header copy (defaults to any)\n");
	exit(1);
}

/*
 * initialize a struct dkbad
 */
void
initbad(bp, csn)
	struct dkbad *bp;
	u_long csn;
{
	int i;

	bp->bt_csn = csn;
	bp->bt_mbz = 0;
	bp->bt_flag = DKBAD_MAGIC;
	for (i = 0; i < MAXBAD; i++) {
		bp->bt_bad[i].bt_cyl = -1;
		bp->bt_bad[i].bt_trksec = -1;
	}

}

/*
 * try and find a valid bad sector map
 */
daddr_t
findbad(f, bad, notify)
	int f;
	struct dkbad *bad;
	int notify;
{
	register int i;
	daddr_t sn;

	if (badfile == -1)
		i = 0;
	else
		i = badfile * 2;
	for (; i < 10 && i < dp->d_nsectors; i += 2) {
		sn = disk_size - dp->d_nsectors + i;
		if (lseek(f, sn * dp->d_secsize, L_SET) < 0)
			Perror("Can't seek to sector %d", sn);
		if (read(f, (char *) bad, dp->d_secsize) == dp->d_secsize) {
			if (bad->bt_flag != DKBAD_MAGIC)
				return -1;
			if (notify && i > 0)
				printf("Using bad-sector file %d\n", i / 2);
			return (sn);
		}
		if (notify)
			fprintf(stderr, "WARNING: error reading bad sector file at sn=%d\n", sn);
		if (badfile != -1)
			break;
	}
	return -1;
}

/*
 * read a struct dkbad off the disk pointed to by f
 */
daddr_t
getbad(f, bad)
	int f;
	struct dkbad *bad;
{
	daddr_t found;

	found = findbad(f, bad, 1);
	if (found == -1)
	    Perror("Unable to locate a valid bad-block table on %s", name);
	return found;
}

/*
 * computes the sector number given a struct bt_bad
 */
daddr_t
badsn(bt)
	register struct bt_bad *bt;
{
	return ((bt->bt_cyl * dp->d_ntracks + (bt->bt_trksec >> 8))
	    * dp->d_nsectors + (bt->bt_trksec & 0xff));
}

/*
 * verifies the data in an array of struct dkbad's
 */
int
checkbad(bp)
	struct dkbad *bp;
{
	register int i;
	register struct bt_bad *bt;
	daddr_t sn, lsn = -1;
	int errors = 0, warned = 0;

	if (bp->bt_flag != DKBAD_MAGIC) {
		fprintf(stderr, "diskdefect: %s: bad flag in bad-sector table\n",
		    name);
		errors++;
	}
	if (bp->bt_mbz != 0) {
		fprintf(stderr, "diskdefect: %s: bad magic number\n", name);
		errors++;
	}
	bt = bp->bt_bad;
	for (i = 0; i < MAXBAD; i++, bt++) {
		if (bt->bt_cyl == 0xffff && bt->bt_trksec == 0xffff)
			break;
		if ((bt->bt_cyl >= dp->d_ncylinders) ||
		    ((bt->bt_trksec >> 8) >= dp->d_ntracks) ||
		    ((bt->bt_trksec & 0xff) >= dp->d_nsectors)) {
			fprintf(stderr, "diskdefect: out of range: ");
			fprintf(stderr, "sn=%d, cyl=%d, trk=%d, sec=%d\n",
			    badsn(bt), bt->bt_cyl, bt->bt_trksec >> 8,
			    bt->bt_trksec & 0xff);
			errors++;
		}
		sn = (bt->bt_cyl * dp->d_ntracks +
		    (bt->bt_trksec >> 8)) *
		    dp->d_nsectors + (bt->bt_trksec & 0xff);
		if (i > 0 && sn < lsn && !warned) {
			fprintf(stderr,
			    "diskdefect: bad sector file is out of order\n");
			errors++;
			warned++;
		}
		if (i > 0 && sn == lsn) {
			fprintf(stderr,
			    "diskdefect: bad sector file contains duplicates (sn %d):%d\n", sn, i);
			errors++;
		}
		lsn = sn;
	}
	if (errors)
		exit(1);
	return (i);
}

/*
 *  Copy disk sector s1 to s2.
 */
int
blkcopy(f, s1, s2)
	int f;
	daddr_t s1, s2;
{
	static char buf[MAXSECSIZE];
	register tries, n;

	for (tries = 0; tries < RETRIES; tries++) {
		if (lseek(f, dp->d_secsize * s1, L_SET) < 0)
			Perror("Can't seek to sector %d", s1);
		if ((n = read(f, buf, dp->d_secsize)) == dp->d_secsize)
			break;
	}
	if (n != dp->d_secsize) {
		fprintf(stderr, "diskdefect: can't read sector, %d: ", s1);
		if (n < 0)
			perror((char *) 0);
		return (0);
	}
	if (lseek(f, dp->d_secsize * s2, L_SET) < 0)
		Perror("Can't seek to sector %d", s2);
	if (nflag == 0 && write(f, buf, dp->d_secsize) != dp->d_secsize) {
		fprintf(stderr,
		    "diskdefect: Can't write replacement sector %d: ", s2);
		perror((char *) 0);
		return (0);
	}
	return (1);
}

/*
 * zero's the sector sn
 */
void
blkzero(f, sn)
	int f;
	daddr_t sn;
{
	static char zbuf[MAXSECSIZE];

	if (lseek(f, dp->d_secsize * sn, L_SET) < 0)
		Perror("Can't seekto sector %d", sn);
	if (nflag == 0 && write(f, zbuf, dp->d_secsize) != dp->d_secsize)
		Perror("Can't write replacement sector %d", sn);
}

/*
 * Compares two struct bt_bad's
 */
int
badcmp(b1, b2)
	register struct bt_bad *b1, *b2;
{
	if (b1->bt_cyl > b2->bt_cyl)
		return (1);
	if (b1->bt_cyl < b2->bt_cyl)
		return (-1);
	return (b1->bt_trksec - b2->bt_trksec);
}

/*
 * checks to make sure we don't overflow the bad sector table
 */
void
toomanyp(c)
	int c;
{
	if (c > MAXBAD) {
		errno = 0;
		fprintf(stderr, "Not enough room for %d more bad sectors.", c);
		fprintf(stderr, "Limited to %d by information format.", MAXBAD);
		exit(1);
	}
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;
	daddr_t sn;
	char line[256];
	int i, f;
	static const char optstring[] = "acswnvb:";

	Psetname("\ndiskdefect");
	while ((ch = getopt(argc, argv, optstring)) != EOF) {
		switch (ch) {
		    case 'a':
			add++;
			break;
		    case 'c':
			copy++;
			break;
		    case 'w':
			wflag++;
			/* implies scan */
		    case 's':
			scan++;
			break;
		    case 'n':
			nflag++;
			/* implies verbose */
		    case 'v':
			verbose++;
			break;
		    case 'y':
		    	force_yes++;
		    	break;
		    case 'b':
			badfile = atoi(optarg);	/* no error checking */
			if (badfile < 0 || badfile > 4)
				usage();
			break;
		    default:
			usage();
			break;
		}
	}
	argv += optind;
	argc -= optind;

	/*
	 * There must be at least one argument remaining: diskname
	 */
	if (argc < 1)
		usage();
	if (*argv[0] != '/')
		(void) sprintf(name, "%sr%s%c", _PATH_DEV, argv[0], RAWPART);
	else
		strcpy(name, argv[0]);
	argc--, argv++;		/* skip past disk name */

	/*
	 * open readonly if nflag or no args unless wflag or scan
	 */
	i = ((nflag || argc == 0) && !(scan || wflag)) ? O_RDONLY : O_RDWR;
	if ((f = open(name, i)) < 0)
		Perror("Can't open disk %s", name);

	/*
	 * read the disk label, can we get rid of !defined(DIOCGDINFO)
	 * case?
	 */
#if defined(DIOCGDINFO)
	if (ioctl(f, DIOCGDINFO, &dl) == -1)
		Perror("Can't get disklabel, ioctl(DIOCGDINFO)");
	if ((dl.d_flags & D_DEFAULT) == D_DEFAULT) {
	    	Perror("No disklabel on disk");
	}
#else				/* DIOCGDINFO */
#ifdef MANUAL
	if (read(f, label, sizeof(label)) < 0)
		Perror("Can't read disk label");
	for (dp = (dl_t *) (label + LABELOFFSET);
	    dp < (dl_t *) (label + sizeof(label) - sizeof(dl_t));
	    dp = (dl_t *) ((char *) dp + 64)) {
		if (dp->d_magic == DISKMAGIC && dp->d_magic2 == DISKMAGIC)
			break;
	}
#else
#error dont know how to read disklabels without DIOCGDINFO
#endif
#endif				/* DIOCGDINFO */
	if (dp->d_magic != DISKMAGIC || dp->d_magic2 != DISKMAGIC)
		Perror("Bad pack magic number (pack is unlabeled)");
	if (dp->d_secsize > MAXSECSIZE || dp->d_secsize <= 0)
		Perror("Disk sector size too large/small (%d)", dp->d_secsize);
	disk_size = dp->d_nsectors * dp->d_ntracks * dp->d_ncylinders;

	message(("Disk geometry:\n"));
	message(("    secsize:         %ld\n", dp->d_secsize));
	message(("    nsectors/track:  %ld\n", dp->d_nsectors));
	message(("    ntracks/cyl:     %ld\n", dp->d_ntracks));
	message(("    ncylinders/unit: %ld\n", dp->d_ncylinders));
	message(("    secpercyl:       %ld\n", dp->d_secpercyl));
	message(("    secperunit:      %ld\n", dp->d_secperunit));
	message(("\n"));

	/*
	 * If no arguments and not scanning then print out old info and
	 * exit
	 */
	if (argc == 0 && !scan) {
		register struct bt_bad *bt;
		daddr_t sn;
		int i;

		sn = getbad(f, &curbad);
		printf("bad-block information at sector %d in %s:\n", sn, name);
		printf("cartridge serial number: %d(10) ", curbad.bt_csn);
		switch (curbad.bt_flag) {
		    case (u_short) -1:
			printf("alignment cartridge");
			break;
		    case DKBAD_MAGIC:
			break;
		    default:
			printf("type=%#x?", curbad.bt_flag);
			break;
		}
		putchar('\n');
		bt = curbad.bt_bad;
		for (i = 0; i < MAXBAD; i++) {
			if (bt->bt_cyl == 0xffff && bt->bt_trksec == 0xffff)
				break;
			printf("Bad Block: sn=%d, cyl=%d, trk=%d, sec=%d\n", badsn(bt),
			    bt->bt_cyl, bt->bt_trksec >> 8, bt->bt_trksec & 0xff);
			bt++;
		}
		(void) checkbad(&curbad);
		exit(0);
	}
	if (scan) {
		int flag;

		/*
		 * write enable label sector before write (if necessary),
		 * disable after writing.
		 */
		flag = 1;
		if (ioctl(f, DIOCWLABEL, &flag) < 0)
			Perror("ioctl(DIOCWLABEL)");

		foundscan += drck_scan(f, dp, verbose, wflag, MAXBAD, newscan);

		flag = 0;
		(void) ioctl(f, DIOCWLABEL, &flag);
	}
	if (!add) {
		/*
		 * We are creating a new bad sector table from scratch so
		 * the next argument is the csn, or default to time().
		 */
		if (argc > 0) {
			initbad(&newbad, atoi(*argv++)), argc--;
		} else {
			initbad(&newbad, (u_long) time(NULL));
		}
	}
	/* append cmd line arguments to newscan */
	toomanyp(argc + foundscan + 1);
	while (argc > 0)
		newscan[foundscan++] = atoi(*argv++), argc--;
	qsort((char *) newscan, foundscan, sizeof(*newscan), bynum);

	if (foundscan > 0) {
		int count = foundscan;

		while (count > 0) {
			daddr_t bn;

			sn = newscan[--count];
			if (sn < 0 || sn >= disk_size)
				Perror("sector %d out of range [0,%d) for disk %s",
				    sn, disk_size, dp->d_typename);
			else if (verbose) {
				int j = sn * dp->d_secsize;

				printf("New Bad Block: sn=%ld, cyl=%ld, head=%ld, trk=%ld, sec=%ld\n",
				    SN(j), CYL(j), HEAD(j), TRK(j), SEC(j));
			}
			bn = sn % (dp->d_nsectors * dp->d_ntracks);
			newbad.bt_bad[count].bt_cyl = (sn / (dp->d_nsectors * dp->d_ntracks));
			newbad.bt_bad[count].bt_trksec = (bn / dp->d_nsectors) << 8;
			newbad.bt_bad[count].bt_trksec += (bn % dp->d_nsectors);
		}
	}
	/*
	 * Double check with user unless -y option was given.
	 */
	if (!force_yes) {
		struct dkbad test;
		if (!add && findbad(f, &test, 0) != -1)
			printf("WARNING: about to overwrite an existing bad-block table!!!\n");
		printf("%s bad block table on %s (y/n)? ",
		    add ? "Update" : "Replace", name);
		fflush(stdout);
		while (1) {
			if (fgets(line, sizeof line, stdin) == NULL) {
				putchar('\n');
				exit(0);
			}
			if (line[0] == 'n')
				exit(0);
			if (line[0] == 'y')
				break;
			printf("Invalid response: you must type 'y' or 'n': ");
			fflush(stdout);
		}
	}
	if (add) {
		int curbad_count;
		int new_i, cur_i, repl_i;
		int cmp;
		daddr_t sn, lsn;
		enum actions {
			a_noop, a_copy, a_relocate
		}       action[MAXBAD];
		daddr_t rfrom[MAXBAD], rto[MAXBAD];
		int rsize = 0;

		/*
		 * Read in the old badsector table.  Verify that it makes
		 * sense, and the bad sectors are in order.
		 */
		(void) getbad(f, &curbad);
#ifdef DEBUG
{
	int i;
	for (i=0; i<126; i++) {
		if (curbad.bt_bad[i].bt_cyl == 0xffff) {
			fprintf(stderr, "fudging: cyl 0x3ff, trksec = 0x0203\n");
			curbad.bt_bad[i].bt_cyl = 0x3ff;
			curbad.bt_bad[i].bt_trksec = 0x0203;
			break;
		}
	}
}
#endif
		curbad_count = checkbad(&curbad);
		message(("Had %d bad sectors, adding %d new bad sectors\n",
			curbad_count, foundscan));
		toomanyp(curbad_count + foundscan);

		/*
		 * Merge the newbad and curbad into replbad.  Easy right?
		 * Shuffle the replacement sectors such that existing
		 * relocated sectors keep their old replacement data.
		 * 
		 * I gave up on being elegant.  I just keep a table of source
		 * and destination and new or relocated flags.
		 */

		initbad(&replbad, curbad.bt_csn);
		replbad.bt_mbz = curbad.bt_mbz;
		replbad.bt_flag = curbad.bt_flag;

		/*
		 * Computes sector address for replacement sector # secnum
		 * [0-MAXBAD) First replacement is last sector of
		 * second-to-last track.
		 */
#define REPL(x) (dp->d_secperunit - (1 + dp->d_nsectors + (x)))

		lsn = -1;
		repl_i = new_i = cur_i = 0;
		while (foundscan != 0 || curbad_count != 0) {
			if (curbad_count != 0 && foundscan == 0) {
				cmp = -1;
			} else if (curbad_count == 0 && foundscan != 0) {
				cmp = 1;
			} else {
				cmp = badcmp(&curbad.bt_bad[cur_i], &newbad.bt_bad[new_i]);
			}
			sn = (cmp < 0) ? badsn(&curbad.bt_bad[cur_i]) : badsn(&newbad.bt_bad[new_i]);

			if (cmp == 0 || lsn == sn) {
				message(("duplicate sector %d eliminated\n", sn));
				(cmp < 0) ? cur_i++, curbad_count-- : new_i++, foundscan--;
				continue;
			} else if (cmp < 0) {	/* relocate smallest first */
				/* relocate existing bad sector (if needed) */
				message(("reloc exist tbl=%d -> tbl=%d\n", cur_i, repl_i));
				replbad.bt_bad[repl_i] = curbad.bt_bad[cur_i];
				if (cur_i != repl_i) {
					rfrom[rsize] = REPL(cur_i);
					rto[rsize] = REPL(repl_i);
					action[rsize] = a_relocate;
					rsize++;
				}
				repl_i++, cur_i++, curbad_count--;
			} else {
				/* relocate new bad sector */
				message(("reloc new sn=%d -> tbl=%d\n",
					badsn(&newbad.bt_bad[new_i]), repl_i));
				replbad.bt_bad[repl_i] = newbad.bt_bad[new_i];
				rfrom[rsize] = badsn(&newbad.bt_bad[new_i]);
				rto[rsize] = REPL(repl_i);
				action[rsize] = a_copy;
				rsize++, repl_i++, new_i++, foundscan--;
			}
			lsn = sn;
		}

		message(("\n%d relocations to perform:\n\n", rsize));

		/*
		 * perform relocation backwards to preserve existing
		 * relocated sectors
		 */
		while (rsize--) {
			switch (action[rsize]) {
			    case a_copy:
				if (copy) {
					blkzero(f, rto[rsize]);
					if (blkcopy(f, rfrom[rsize], rto[rsize]) == 0)
						message(("copy failed from sn=%d, zero'ed sn=%d\n",
							rfrom[rsize], rto[rsize]));
					else
						message(("copied from sn=%d to sn=%d\n",
							rfrom[rsize], rto[rsize]));
				} else {
					blkzero(f, rto[rsize]);
					message(("zero'ed: sn=%d\n", rto[rsize]));
				}
				break;
			    case a_relocate:
				if (blkcopy(f, rfrom[rsize], rto[rsize]) == 0)
					Perror("Couldn't relocate replacement sector from %d to %d",
					    rfrom[rsize], rto[rsize]);
				message(("relocated sn=%d to sn=%d\n", rfrom[rsize], rto[rsize]));
				break;
			    default:
				break;
			}
		}
	} else {
		/* new found bad sectors is the whole enchilada */
		replbad = newbad;
	}

	message(("Checking replacement bad sector table.\n"));
	(void) checkbad(&replbad);

	/*
	 * write out replbad to disk and ioctl(DIOCSBAD) to update in
	 * kernel table
	 */
	if (badfile == -1)
		i = 0;
	else
		i = badfile * 2;
	for (; i < 10 && i < dp->d_nsectors; i += 2) {
		if (lseek(f, dp->d_secsize * (disk_size - dp->d_nsectors + i), L_SET) < 0)
			Perror("Can't seek to bad sector table");
		message(("write badsect file at %d\n", disk_size - dp->d_nsectors + i));
		if (nflag == 0 && write(f, (caddr_t) &replbad, sizeof(replbad)) != sizeof(replbad))
			Perror("write bad sector file %d", i / 2);
		if (badfile != -1)
			break;
	}

#ifdef DIOCSBAD
	if (nflag == 0 && ioctl(f, DIOCSBAD, (caddr_t) &replbad) < 0)
#endif
		fprintf(stderr, "Can't sync bad-sector file; reboot for changes to take effect\n");

	fprintf(stderr, "Bad sector table written successfully\n");
	return (0);
}
