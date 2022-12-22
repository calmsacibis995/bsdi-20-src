/*-
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*	BSDI disksetup.c,v 2.1 1995/02/03 06:40:05 polk Exp	*/

#include <stdio.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <ufs/ffs/fs.h>
#include <string.h>
#define DKTYPENAMES
#include <sys/disklabel.h>
#include <machine/bootblock.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <curses.h>
#include "pathnames.h"
#include "bootany.h"
#include "structs.h"

#define RAWPARTITION	'c'
#define	BOOTDIR		_PATH_BOOTSTRAPS	/* source of boot binaries */

#define MB_MAXSECTOR	63	/* XXX should be in <machine/bootblock.h> */

char *progname;

char *diskname = NULL;
char *savename = NULL;
char specname[MAXPATHLEN];
char namebuf[2 * MAXPATHLEN], *np = namebuf;

char *bootxx, *xxboot, *mbootx;
char boot0[MAXPATHLEN];
char boot1[MAXPATHLEN];
char boot2[MAXPATHLEN];

char tmpfil[] = _PATH_TMP;

int doingdos = 0;		/* we want or will set-up fdisk label */
int hasdoslabel = 0;		/* has existing fdisk label */
int hasbsdlabel = 0;		/* has existing bsd label */
int movedbsd = 0;		/* bsd label moved (must write boot blocks) */
int ignorelabel = 0;		/* ignore existing bsd (+ possibly fdisk) lbl */
int forceboot = 0;		/* install boot blocks regardless */
int bsdoffset;			/* offset of bsd boot partition */

struct mbpart doslabel[MB_MAXPARTS];
struct masterboot mboot;
struct disklabel bsdlabel;
struct disklabel *bootarealp;
char bootarea[BBSIZE];

enum {
	UNSPEC, READ, RESTORE, INTERACTIVE, EDIT, W_ENABLE, W_DISABLE, BOOTBLKS
}    op = UNSPEC;

#define BOOTTYPES	3
char *boottypes[] = {"wd\t(IDE, ESDI, ST506)", "aha\t(ISA SCSI -- AHA154[02][BC[F]], BT542, BT445S)",
"eaha (EISA SCSI -- AHA174[02])", 0};

#define DEFEDITOR	_PATH_VI

usage()
{
	fprintf(stderr, "Usage: \n");
	fprintf(stderr, "    %s disk\t\t\t\t\t(read label)\n", progname);
	fprintf(stderr, 
		"    %s -e [-I [-I]] disk [xxboot bootxx [mboot]]\t(edit label)\n", 
		progname);
	fprintf(stderr, "    %s [-I [-I]] -i disk\t\t\t\t(interactive mode)\n",
		 progname);
	fprintf(stderr, 
    "    %s -R disk protofile [xxboot bootxx [mboot]]\t(restore label)\n",
		progname);
	fprintf(stderr, 
		"    %s [-NW] disk\t\t\t\t(write enable/disable)\n",
		progname);
	fprintf(stderr, 
		"    %s -B disk [xxboot bootxx [mboot]]\t\t(install boot blocks)\n",
		progname);
	exit(1);
}

/*
 * Construct a prototype disklabel from /etc/disktab.  As a side
 * effect, set the names of the primary and secondary boot files
 * if specified. (code stolen from disklabel)
 */
int
makelabel(type, name, lp)
	char *type, *name;
	register struct disklabel *lp;
{
	register struct disklabel *dp;
	char *strcpy();

	dp = getdiskbyname(type);
	if (dp == NULL) {
		fprintf(stderr, "%s: unknown disk type: %s\n", progname, type);
		return (1);
	}
	*lp = *dp;

	/* Check if disktab specifies the bootstraps (b0 or b1). */
	if (!xxboot && lp->d_boot0) {
		if (*lp->d_boot0 != '/')
			(void) sprintf(boot0, "%s/%s", BOOTDIR, lp->d_boot0);
		else
			(void) strcpy(boot0, lp->d_boot0);
		xxboot = boot0;
	}
	if (!bootxx && lp->d_boot1) {
		if (*lp->d_boot1 != '/')
			(void) sprintf(boot1, "%s/%s", BOOTDIR, lp->d_boot1);
		else
			(void) strcpy(boot1, lp->d_boot1);
		bootxx = boot1;
	}

	/* d_packname is union d_boot[01], so zero */
	bzero(lp->d_packname, sizeof(lp->d_packname));
	if (name)
		(void) strncpy(lp->d_packname, name, sizeof(lp->d_packname));

	return (0);
}

/*
 * Write a label (and boot blocks) to the disk at the specified offset.
 */
writelabel(f, boot, lp, offset)
	int f;
	char *boot;
	register struct disklabel *lp;
	long offset;
{
	register int i;
	off_t lseek();

#ifdef d_bsd_startsec
	lp->d_bsd_startsec = offset;
#endif

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);

	/* First set the kernel disk label, then write a label to the raw
	   disk. If the SDINFO ioctl fails because it is unimplemented,
	   keep going; otherwise, the kernel consistency checks may prevent
	   us from changing the current (in-core) label. */
	if (ioctl(f, DIOCSDINFO, lp) < 0 &&
	    errno != ENODEV && errno != ENOTTY) {
		l_perror("ioctl DIOCSDINFO");
		return (1);
	}
	/* write enable label sector before write (if necessary), disable
	   after writing. */
	diocwlabel(f, 1);
	(void) lseek(f, (off_t) offset*DEV_BSIZE, L_SET);
	if ((i = write(f, boot, lp->d_bbsize)) != lp->d_bbsize) {
		if (i == -1)
			perror("write");
		else
			fprintf(stderr,
			    "short write: wrote %d of %d\n",
			    i, lp->d_bbsize);
		return (1);
	}
	diocwlabel(f, 0);
	return (0);
}

diocwlabel(f, flag)
	int f;
	int flag;
{
	if (ioctl(f, DIOCWLABEL, &flag) < 0)
		perror("ioctl DIOCWLABEL");
}

l_perror(s)
	char *s;
{
	int saverrno = errno;

	fprintf(stderr, "%s: %s: ", progname, s);

	switch (saverrno) {

	case ESRCH:
		fprintf(stderr, "No disk label on disk;\n");
		fprintf(stderr,
		    "use \"disklabel -r\" to install initial label\n");
		break;

	case EINVAL:
		fprintf(stderr, "Label magic number or checksum is wrong!\n");
		fprintf(stderr, "(disklabel or kernel is out of date?)\n");
		break;

	case EBUSY:
		fprintf(stderr, "Open partition would move or shrink\n");
		break;

	case EXDEV:
		fprintf(stderr,
		    "Labeled partition or 'a' partition must start at beginning of disk\n");
		break;

	default:
		errno = saverrno;
		perror((char *) NULL);
		break;
	}
}

char *
prompt(s)
	char *s;
{
	static char line[BUFSIZ];

	printf("%s: ", s);
	fflush(stdout);
	fgets(line, sizeof line, stdin);
	line[strlen(line) - 1] = '\0';	/* strip the newline */
	return line;
}

/*
 * Fetch disklabel for disk.
 * Use ioctl to get label unless -r flag is given.
 */
struct disklabel *
readlabel(f, startsector)
	int f;
	long startsector;
{
	register struct disklabel *lp;

	if (lseek(f, startsector * DEV_BSIZE, L_SET) < 0) {
		perror("seek");
		fprintf(stderr, "%s: error reading bsd boot blocks\n",
		    progname);
		exit(1);
	}
	if (read(f, bootarea, BBSIZE) < BBSIZE) {
		fprintf(stderr, "%s: error reading bsd boot blocks\n", progname);
		exit(1);
	}
	for (lp = (struct disklabel *) bootarea;
	    lp <= (struct disklabel *) (bootarea + BBSIZE - sizeof(*lp));
	    lp = (struct disklabel *) ((char *) lp + 16))
		if (lp->d_magic == DISKMAGIC &&
		    lp->d_magic2 == DISKMAGIC)
			break;
	if (lp > (struct disklabel *) (bootarea + BBSIZE - sizeof(*lp)) ||
	    lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC ||
	    dkcksum(lp) != 0) {
		return (NULL);
	}
	return (lp);
}

/*
 * Make the boot area based on the label and query for the
 * boot block names if necessary.
 */
struct disklabel *
makebsdbootarea(boot, dp, xxbootsec)
	char *boot;
	register struct disklabel *dp;
	int xxbootsec;
{
	struct disklabel *lp;
	register char *p;
	int b, i, choice;
	char *dkbasename;

	lp = (struct disklabel *) (boot + (LABELSECTOR * dp->d_secsize) +
	    LABELOFFSET);

	if (xxboot == NULL || bootxx == NULL) {
		for (;;) {
			printf("\nWhich boot blocks do you wish to install?\n\n");
			for (i = 0; i < BOOTTYPES; i++)
				printf("\t%d: %s\n", i + 1, boottypes[i]);
			choice = atoi(prompt("\nBoot block type")) - 1;
			if (choice >= 0 && choice < BOOTTYPES)
				break;
		}
		dkbasename = strdup(boottypes[choice]);
		for (p = dkbasename; *p && !isspace(*p); p++)
			 /* do nothing */ ;
		*p = '\0';
		if (xxboot == NULL) {
			(void) sprintf(np, "%s/%sboot", BOOTDIR, dkbasename);
			if (access(np, F_OK) < 0 && dkbasename[0] == 'r')
				dkbasename++;
			xxboot = np;
			(void) sprintf(xxboot, "%s/%sboot", BOOTDIR, dkbasename);
			np += strlen(xxboot) + 1;
		}
		if (bootxx == NULL) {
			(void) sprintf(np, "%s/boot%s", BOOTDIR, dkbasename);
			if (access(np, F_OK) < 0 && dkbasename[0] == 'r')
				dkbasename++;
			bootxx = np;
			(void) sprintf(bootxx, "%s/boot%s", BOOTDIR, dkbasename);
			np += strlen(bootxx) + 1;
		}
	}
#ifdef DEBUG
	if (debug)
		fprintf(stderr, "bootstraps: xxboot = %s, bootxx = %s\n",
		    xxboot, bootxx);
#endif

	if ((b = open(bootxx, O_RDONLY)) < 0)
		z(1, "couldn't open %s: %s", bootxx, strerror(errno));
	if (read(b, &boot[dp->d_secsize], (int) (dp->d_bbsize - dp->d_secsize)) < 0)
		z(1, "error reading %s: %s", bootxx, strerror(errno));
	(void) close(b);

	if ((b = open(xxboot, O_RDONLY)) < 0)
		z(1, "couldn't open %s: %s", xxboot, strerror(errno));
	if (read(b, &boot[xxbootsec * dp->d_secsize], (int) dp->d_secsize) < 0)
		z(1, "error reading %s: %s", xxboot, strerror(errno));
	close(b);

	for (p = (char *) lp; p < (char *) lp + sizeof(struct disklabel); p++)
		if (*p) {
			fprintf(stderr,
			    "Bootstrap doesn't leave room for disk label\n");
			exit(2);
		}
	return (lp);
}

/*
 * Check the dos label for errors and fill in derived fields
 * according to values supplied in the bsd label.
 */
checkdoslabel(dlp, lp)
	register struct mbpart *dlp;
	register struct disklabel *lp;
{
	int active = 0;
	int i, errors = 0;
	long scyl, shead, ssec, ecyl, ehead, esec;

	for (i = 0; i < MB_MAXPARTS; i++) {

		if (dlp[i].system == MBS_UNKNOWN) {
			bzero(&dlp[i], sizeof(struct mbpart));
			continue;
		}

		/* turn off previously existed flag if set */
		if (dlp[i].active & P_EXISTED)
			dlp[i].active &= ~P_EXISTED;

		if (dlp[i].active == MBA_ACTIVE) {
			active++;
			dlp[i].active = MBA_NOTACTIVE;	/* XXX */
		}
		else if (dlp[i].active != MBA_NOTACTIVE)
			fprintf(stderr, "%s: fdisk partition %d active flag is 0x%x (not 0x80 or 0x0)\n",
			    progname, i, dlp[i].active);

		if (dlp[i].start < 0 || dlp[i].start > lp->d_secperunit) {
			fprintf(stderr,
			    "%s: invalid start sector in fdisk partition %d (%d)\n",
			    progname, i, dlp[i].start);
			errors++;
		}
		if (dlp[i].size < 0 ||
		    (dlp[i].start + dlp[i].size) > lp->d_secperunit) {
			fprintf(stderr,
			    "%s: invalid size in fdisk partition %d (%d)\n",
			    progname, i, dlp[i].size);
			errors++;
		}

		shead = dlp[i].start / lp->d_nsectors % lp->d_ntracks;
		ssec = dlp[i].start % lp->d_nsectors + 1;
		scyl = dlp[i].start / lp->d_secpercyl;
		if (scyl > MB_MAXCYL) {
			fprintf(stderr,
			    "%s: invalid starting cylinder in fdisk partition %d (%d)\n",
			    progname, i, scyl);
			errors++;
		}
		ehead = (dlp[i].start + dlp[i].size - 1) / lp->d_nsectors % lp->d_ntracks;
		esec = (dlp[i].start + dlp[i].size - 1) % lp->d_nsectors + 1;
		ecyl = (dlp[i].start + dlp[i].size - 1) / lp->d_secpercyl;
		if (ecyl > MB_MAXCYL) {
			fprintf(stderr,
			    "%s: invalid ending cylinder in fdisk partition %d (%d)\n",
			    progname, i, ecyl);
			errors++;
		}

		dlp[i].shead = shead;
		dlp[i].ssec = ssec | ((scyl >> MB_CYLSHIFT) & MB_CYLMASK);
		dlp[i].scyl = (char) scyl;

		dlp[i].ehead = ehead;
		dlp[i].esec = esec | ((ecyl >> MB_CYLSHIFT) & MB_CYLMASK);
		dlp[i].ecyl = (char) ecyl;
	}
	if (active > 1) {
		fprintf(stderr, "%s: more than one active fdisk partition\n",
		    progname);
		errors++;
	}
	return (errors);
}

/*
 * Examine a possible master boot block/FDISK partition table
 * to find the location of the BSD root or other primary partition.
 * Find either the (only) BSD active or first BSD file system partition
 * and return index of this partition.
 * Return -1 if there is no partition table, or it is corrupted.
 */
static
rootpart(mp)
	struct mbpart *mp;
{
	int i, nact, iact, iboot, ibsd;

	iact = -1;
	iboot = -1;
	ibsd = -1;

	/* Scan the partitions looking for the following, listed in order
	   of preference: 1.  active BSD partition 2.  bootable BSD
	   partition (bootany-type) 3.  any other BSD partition If there is
	   more than one of the same preference, other than active, take
	   the first found.  If there is any inconsistency, this may not be
	   a partition table; return -1. */
	for (nact = i = 0; i < MB_MAXPARTS; i++, mp++) {
		switch (mp->active & ~P_EXISTED) {
		case MBA_ACTIVE:
			if (++nact > 1)
				return (-1);
			if (mp->system == MBS_BSDI)
				iact = i;
			break;

		case MBA_NOTACTIVE:
			if (ibsd < 0 && mp->system == MBS_BSDI)
				ibsd = i;
			break;

		default:
			/* bootany marks bootable partitions by changing
			   the active indicator to the system type, and the
			   system value to MBS_BOOTANY. If there is a
			   "bootable" BSD partition and no active BSD
			   partition, take the bootable one. */
			if (mp->system == MBS_BOOTANY) {
				if (mp->active == MBS_BSDI && iboot < 0)
					iboot = i;
				break;
			}
			return (-1);
		}
	}
	if (iact >= 0)
		return (iact);
	if (iboot >= 0)
		return (iboot);
	return (ibsd);
}

long
getnumber(promptstring)
	char *promptstring;
{
	long n;

	for (;;) {
		if ((n = atol(prompt(promptstring))) != 0)
			break;
		printf("Invalid entry\n");
	}
	return n;
}

/*
 * Prompt the user for the necessary values and build a prototype
 * label for use with the interactive label generator.
 */
promptforlabel()
{
	char *ans;
	struct disklabel dl;
	char tmpbuf[BUFSIZ];

	bzero(&dl, sizeof(dl));
	dl.d_magic = dl.d_magic2 = DISKMAGIC;

	for (;;) {
		ans = prompt("Interface type (SCSI, ESDI, IDE, ST506)?");
		if (strcasecmp(ans, "scsi") == 0) {
			dl.d_type = DTYPE_SCSI;
			break;
		}
		else if (strcasecmp(ans, "ide") == 0 ||
		    strcasecmp(ans, "st506") == 0) {
			dl.d_type = DTYPE_ST506;
			break;
		}
		else if (strcasecmp(ans, "esdi") == 0) {
			dl.d_type = DTYPE_ESDI;
			break;
		}
		else
			printf("Invalid type `%s'\n", ans);
	}

	ans = prompt("Vendor and type");
	strncpy(dl.d_typename, ans, sizeof dl.d_typename);

	for (;;) {
		dl.d_secsize = atol(prompt("Bytes/sector [512]"));
		if (dl.d_secsize == 0)
			dl.d_secsize = 512;
		if (dl.d_secsize % DEV_BSIZE == 0)
			break;
		printf("Invalid sector size -- must be a multiple of %d\n",
		    DEV_BSIZE);
	}
	dl.d_ncylinders = getnumber("Number of cylinders");
	dl.d_ntracks = getnumber("Number of heads");
	dl.d_nsectors = getnumber("Number of sectors/track");
	dl.d_rpm = 3600;	/* XXX */
	dl.d_interleave = 1;	/* XXX */
	dl.d_secpercyl = dl.d_nsectors * dl.d_ntracks;
	sprintf(tmpbuf, "Total data sectors [%d]",
	    dl.d_secpercyl * dl.d_ncylinders);
	dl.d_secperunit = atol(prompt(tmpbuf));
	if (dl.d_secperunit == 0)
		dl.d_secperunit = dl.d_secpercyl * dl.d_ncylinders;

	if (makepartitions(&dl) < 0)
		return -1;

	bcopy(&dl, &bsdlabel, sizeof(dl));
	return 0;
}

/*
 * Fill in reasonable partition values for the default label
 */
makepartitions (lp)
	struct disklabel *lp;
{
	long spc;
	long dsize;

	lp->d_npartitions = MAXPARTITIONS;
	lp->d_bbsize = BBSIZE;
	lp->d_sbsize = SBSIZE;

	bzero(lp->d_partitions, sizeof(struct partition) * MAXPARTITIONS);

	/* 'c' partition: whole disk */
	spc = lp->d_secpercyl;
	lp->d_partitions[2].p_offset = 0;
	lp->d_partitions[2].p_size = lp->d_secperunit;
	lp->d_partitions[2].p_fstype = FS_UNUSED;

	dsize = lp->d_secperunit;
	if (lp->d_type == DTYPE_ESDI || lp->d_type == DTYPE_ST506)
		dsize = (dsize - lp->d_nsectors - 126) /
		    lp->d_nsectors * lp->d_nsectors;

	if (dsize > ((79 * 1024 * 1024 / lp->d_secsize + spc - 1) / spc) * spc) {

		/* 'a' partition: ~8 MB */
		lp->d_partitions[0].p_offset = 0;
		lp->d_partitions[0].p_size =
		    ((8 * 1024 * 1024 / lp->d_secsize + spc - 1) / spc) * spc;
		lp->d_partitions[0].p_fsize = 1024;
		lp->d_partitions[0].p_fstype = FS_BSDFFS;
		lp->d_partitions[0].p_frag = 8;
		lp->d_partitions[0].p_cpg = 16;

		/* 'b' partition: ~32 MB */
		lp->d_partitions[1].p_offset = lp->d_partitions[0].p_size;
		lp->d_partitions[1].p_size =
		    ((32 * 1024 * 1024 / lp->d_secsize + spc - 1) / spc) * spc;
		lp->d_partitions[1].p_fstype = FS_SWAP;

		/* 'h' partition: leftovers */
		lp->d_partitions[7].p_offset = lp->d_partitions[1].p_offset +
		    lp->d_partitions[1].p_size;
		lp->d_partitions[7].p_size = dsize - lp->d_partitions[7].p_offset;
		lp->d_partitions[7].p_fsize = 1024;
		lp->d_partitions[7].p_fstype = FS_BSDFFS;
		lp->d_partitions[7].p_frag = 8;
	}

	if (disklabel_checklabel(lp) != 0) {
		fprintf(stderr, "%s: invalid label generated\n", progname);
		return -1;
	}
	return 0;
}

/*
 * Get a first approximation at a label.  It can be entered as a type
 * from disktab, we'll run scsicmd for the user and read in the data,
 * or we'll prompt the user for the appropriate fields
 */
getfirstlabel(diskfd)
	int diskfd;
{
	int labelerrs = 0;
	int use_extgeom;
	char *choice;
	char *tmp;
	char tmpbuf[BUFSIZ];
	char cmdbuf[BUFSIZ];
	FILE *fp;
	struct mbpart bogus[MB_MAXPARTS];
	struct disklabel *lp;
	extern char *mktemp();

	for (;;) {
		labelerrs = 1;

		printf("\nDisk geometries can be entered in several ways.  Your choices are:\n\n");
		printf("    `scsi'\tQueries the SCSI disk itself to build a label\n");
		printf("\t\t(use this for all SCSI disks unless you know of some problem).\n\n");
		printf("    `internal'\tQueries the kernel for the current label information.\n\n");
		printf("    `st506'\tQueries an IDE/ESDI controller for parameters\n\n");
		printf("    `prompt'\tQueries you for the necessary parameters directly.\n\n");
		printf("    `file'\tQueries you for a saved label file (of the form\n");
		printf("\t\tcreated by disksetup or disklabel with only a disk argument\n");
		printf("\t\tor by `scsicmd -l').\n\n");
		printf("    Any other string is assumed to be a disk type name for an entry in\n");
		printf("    the /etc/disktab file.\n");

		choice = prompt("\nEnter choice [`scsi', `internal', `st506', `prompt', `file', or type]");
		if (choice[0] == '\0')
			continue;

		bzero(&bsdlabel, sizeof(bsdlabel));
		if (strcmp(choice, "scsi") == 0) {
			(void) sprintf(tmpbuf, _PATH_TMP, progname);
			tmp = mktemp(tmpbuf);
			(void) sprintf(cmdbuf, "%s -f %s -l > %s", _PATH_SCSICMD,
			    specname, tmp);
			printf("Executing scsicmd to build a label:\n");
			if (system(cmdbuf) != 0) {
				fprintf(stderr, "%s: execution of scsicmd failed\n");
				return (-1);
			}
			z((fp = fopen(tmp, "r")) == NULL,
			    "couldn't open label file %s", tmp);
			bzero(bogus, sizeof(bogus));
			z(!disklabel_getasciilabel(fp, &bsdlabel, bogus),
			    "errors parsing label file");
			fclose(fp);
			if (doingdos) {
				/* XXX Here we must do what the adaptec
				   will tell DOS.  Namely, 64
				   heads, 32 sectors, and an appropriate
				   number of cylinders or 255 heads, 
				   63 sectors, and an appropriate number
				   of cyls if they have extended mapped mode */
				bsdlabel.d_secperunit =
					bsdlabel.d_partitions['c'-'a'].p_size;
				use_extgeom=0;

				if (bsdlabel.d_secperunit > (64*32*1024)) {
					for (;;) {
						char *choice;
printf("\n");
printf("The Adaptec BIOS uses one of two mapped geometries for large disks.\n");
printf("If your controller (or BIOS) is new enough to have the `Extended\n");
printf("mapped geometry' option, you should use it to describe your disk\n");
printf("(since it is larger than 1GB).  If your controller doesn't support\n");
printf("the extended geometry mapping, you must use the regular mapping\n");
printf("(which limits the disk to 1GB).  If you specify the wrong geometry,\n");
printf("you will be able to complete the installation without errors, but\n");
printf("you won't be able to boot from the hard drive.\n");

						choice = prompt("\nDo you wish to use `Extended mapped geometry'? [yes]");
						if (choice[0] == 'y' || choice[0] == 'Y' || choice[0] == '\0') {
							use_extgeom = 1;
							break;
						}
						if (choice[0] == 'n' || choice[0] == 'N')
							break;
						printf("Invalid response.\n");
					}
				}

				if (use_extgeom) {
					bsdlabel.d_nsectors = 63;
					bsdlabel.d_ntracks = 255;
					bsdlabel.d_sparespertrack = 0;
					bsdlabel.d_sparespercyl = 0;
					bsdlabel.d_acylinders = 0;
					bsdlabel.d_secpercyl = bsdlabel.d_nsectors * bsdlabel.d_ntracks;
					bsdlabel.d_ncylinders = (bsdlabel.d_secperunit + bsdlabel.d_secpercyl - 1) / bsdlabel.d_secpercyl;
				} 
				else {
					bsdlabel.d_nsectors = 32;
					bsdlabel.d_ntracks = 64;
					bsdlabel.d_sparespertrack = 0;
					bsdlabel.d_sparespercyl = 0;
					bsdlabel.d_acylinders = 0;
					bsdlabel.d_secpercyl = bsdlabel.d_nsectors * bsdlabel.d_ntracks;
					bsdlabel.d_ncylinders = (bsdlabel.d_secperunit + bsdlabel.d_secpercyl - 1) / bsdlabel.d_secpercyl;
				}
			}
			labelerrs = 0;
		}
		else if (strcmp(choice, "internal") == 0) {
			if (ioctl(diskfd, DIOCGDINFO, &bsdlabel) < 0) {
				fprintf(stderr, "%s: ioctl DIOCGDINFO: %s\n",
					progname, strerror(errno));
				labelerrs = 1;
				continue;
			}
			if (bsdlabel.d_flags & D_DEFAULT) {
				fprintf(stderr, "%s: No label information available from the kernel\n",
					progname);
				labelerrs = 1;
				continue;
			}
			if (bsdlabel.d_flags & D_SOFT) {
				bsdlabel.d_rpm = 3600;		/* XXX */
				bsdlabel.d_interleave = 1;	/* XXX */
				bsdlabel.d_secsize = 512;	/* XXX */
				bsdlabel.d_secpercyl = bsdlabel.d_nsectors * 
					bsdlabel.d_ntracks;
				bsdlabel.d_secperunit = bsdlabel.d_ncylinders *
					bsdlabel.d_ntracks *
					bsdlabel.d_nsectors;
				labelerrs = makepartitions(&bsdlabel);
			}
			else 
				labelerrs = 0;
		}
		else if (strcmp(choice, "st506") == 0) {
			if (ioctl(diskfd, DIOCGHWINFO, &bsdlabel) < 0) {
				fprintf(stderr, "%s: ioctl DIOCGHWINFO: %s\n",
					progname, strerror(errno));
				labelerrs = 1;
				continue;
			}
			lp = &bsdlabel;
			if (lp->d_nsectors <= 0 || lp->d_ntracks <= 0 ||
						    lp->d_ncylinders <= 0) {
				fprintf(stderr, "%s: Incomplete info from DIOCGHWINFO\n",
					progname);
				printf("The data returned from the controller was incomplete, so\n");
				printf("you'll have to choose one of the other methods.\n");
				printf("In case it's helpful, the controller geometry was\n");
				printf("\t%d sectors/track, %d heads, %d cylinders.\n",
					lp->d_nsectors, lp->d_ntracks, lp->d_ncylinders);
				labelerrs = 1;
				continue;
			}
			lp->d_type = DTYPE_ST506;
			lp->d_rpm = 3600;	/* XXX */			
			lp->d_interleave = 1;	/* XXX */
			lp->d_secsize = 512;	/* XXX */
			lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks;
			lp->d_secperunit = lp->d_ncylinders * lp->d_ntracks *
						lp->d_nsectors;
			labelerrs = makepartitions(lp);
		}
		else if (strcmp(choice, "prompt") == 0) {
			labelerrs = promptforlabel();
		}
		else if (strcmp(choice, "file") == 0) {
			choice = prompt("Enter the name of the file");
			if ((fp = fopen(choice, "r")) == NULL) {
				fprintf(stderr,
				    "%s: couldn't open label file %s\n",
				    progname, choice);
				labelerrs = 1;
				continue;
			}
			bzero(bogus, sizeof(bogus));
			z(!disklabel_getasciilabel(fp, &bsdlabel, bogus),
			    "errors parsing label file");
			fclose(fp);
			labelerrs = 0;
		}
		else
			labelerrs = makelabel(choice, NULL, &bsdlabel);

		if (labelerrs == 0 && doingdos && 
				bsdlabel.d_ncylinders > MB_MAXCYL) {
			long secdiff;

			secdiff = bsdlabel.d_secperunit - 
					MB_MAXCYL * bsdlabel.d_secpercyl;

			printf("\nThe specified geometry has more than %d cylinders, which\n",
				MB_MAXCYL);
			printf("is more than may be partitioned using the FDISK table\n");
			printf("required for co-residency.  This geometry has been modified\n");
			printf("to fit within the %d cylinder limit imposed on the table.\n",
				MB_MAXCYL);
			printf("Using this modified geometry reduces the number of cylinders\n");
			printf("from %d to %d (thus reducing the usable disk space by %d sectors.\n",
				bsdlabel.d_ncylinders, MB_MAXCYL, secdiff);
			printf("If you wish to change the geometry (to specify an alternate\n");
			printf("mapped geometry), specify `no' when prompted whether the\n");
			printf("current information is correct.\n\n");
			printf("You can also use `disksetup -e' to allocate the unusable\n");
			printf("space for BSD; this requires manual calculations and bypasses\n");
			printf("most consistency checks.\n\n");
			(void) prompt("Press return to continue");

			bsdlabel.d_secperunit = 
				bsdlabel.d_partitions['c'-'a'].p_size =
					MB_MAXCYL * bsdlabel.d_secpercyl;
			bsdlabel.d_ncylinders = MB_MAXCYL;
		}

		if (labelerrs == 0) {
			/* display the label */
			showgeometry(&bsdlabel);
			/* verify it's ok */
			tmp = prompt("\nIs this information correct? [yes]");
			if (tmp[0] != 'n' && tmp[0] != 'N')
				return 0;
		}
	}
}

/*
 *Show current disk geometry information
 */
showgeometry(label)
	struct disklabel *label;
{
	printf("\nCurrent geometry:\n");
	printf("\tType:            %s\n", dktypenames[label->d_type]);
	printf("\tVendor/Type:     %s\n", label->d_typename);
	printf("\tBytes/Sector:    %d\n", label->d_secsize);
	printf("\tCylinders:       %d\n", label->d_ncylinders);
	printf("\tHeads:           %d\n", label->d_ntracks);
	printf("\tSectors/track:   %d\n", label->d_nsectors);
	printf("\tTotal Sectors:   %d\n", label->d_secperunit);
}


/*
 * Read the master boot record and current BSD label from the
 * disk.
 */
getcurrentlabels(dfd)
	int dfd;
{
	int i, n;
	int bsdpart;
	int bsdstartsector;
	char *choice;
	struct disklabel *lp;

	if (ignorelabel > 1) 
		hasdoslabel=0;
	else {
		/* get the master boot record */
		if ((n = read(dfd, &mboot, sizeof(mboot))) != sizeof(mboot)) {
			if (n < 0)
				perror("read");
			fprintf(stderr, "%s: error reading master boot block\n",
		    	progname);
			exit(1);
		}
		bcopy(mbparts(&mboot), doslabel, sizeof(doslabel));

		/* figure out if we have a dos label and/or a bsd partition */
		if (mboot.signature != MB_SIGNATURE)
			bsdpart = -1;
		else
			bsdpart = rootpart(doslabel);
		if (bsdpart >= 0) {
			/* hack: if the bsd partition starts at the beginning of
		   	the disk, it cannot have an offset of 0 (the partition
		   	table lives there, not our boot block). However, the BSD
		   	boot block may be at the end of the second-stage
		   	bootstrap rather than the beginning.  In this case, the
		   	FDISK paritition table is set to use an offset of 15,
		   	but the BSD partition starts at 0.  "Fix" this here. */
			struct mbpart *mpp = &doslabel[bsdpart];
	
			if (mbpssec(mpp) == 15 && mbpstrk(mpp) == 0 &&
		    	mbpscyl(mpp) == 0)
				bsdstartsector = 0;
			else
				bsdstartsector = mpp->start;
			hasdoslabel = 1;
		}
		else {
			int i;
			int numactive = 0;
			int numempty = 0;
	
			bsdstartsector = 0;
			if (mboot.signature == MB_SIGNATURE)
				hasdoslabel = 1;
			for (i = 0; hasdoslabel && i < MB_MAXPARTS; i++) {
				switch (doslabel[i].active) {
				case MBA_ACTIVE:
					if (numactive++ > 1)
						hasdoslabel = 0;
					break;
				case MBA_NOTACTIVE:
					break;
				default:
					hasdoslabel = 0;
					break;
				}
				if (doslabel[i].system == MBS_UNKNOWN)
					numempty++;
			}
			if (numempty == MB_MAXPARTS)
				hasdoslabel = 0;
		}
	}
	
	if (ignorelabel)
		hasbsdlabel = 0;
	else {
		/* get the boot blocks (and label) */
		lp = readlabel(dfd, bsdstartsector);
		if (lp != NULL) {
			bcopy(lp, &bsdlabel, sizeof(bsdlabel));
			hasbsdlabel = 1;
		}
		else
			hasbsdlabel = 0;
		bootarealp = lp;
	}
	
	if (op != INTERACTIVE && op != EDIT) {
		if (!hasbsdlabel)
			printf("\nNo bsd label found on disk!\n");
		return;
	}

	/* find out if we'll be doing co-residency */
	for (;;) {
		for (;;) {
			printf("\nDo you intend to share this disk between BSD/386\n");
			printf("and another operating system? ");
			if (hasdoslabel)
				choice = prompt("[yes]");
			else
				choice = prompt("[no]");
			if (choice[0] == 'y' || choice[0] == 'Y' ||
			    (hasdoslabel && choice[0] == '\0')) {
				doingdos = 1;
				break;
			}
			else if (choice[0] == 'n' || choice[0] == 'N' ||
			    (!hasdoslabel && choice[0] == '\0')) {
				doingdos = 0;
				break;
			}
			printf("Invalid selection -- please answer yes or no.\n");
		}
		if (!doingdos && hasdoslabel) {
			for (;;) {
				printf("\nYou have elected to ignore the existing FDISK partition\n");
				printf("table on the disk.  You will destroy any data for other\n");
				printf("operating systems already existing on the disk if you proceed.\n");
				choice = prompt("\nAre you sure you want to have BSD/386 take over the entire disk? [no]");
				if (choice[0] == 'n' || choice[0] == 'N' || choice[0] == '\0')
					break;
				if (choice[0] == 'y' || choice[0] == 'Y')
					goto verified;
				printf("Invalid selection -- please answer yes or no.\n");
			}
		}
		else
			break;
	}
verified:;

	if (doingdos && hasdoslabel && bsdpart < 0) {
		printf("This disk appears contain a valid FDISK label, but no BSD label.\n");
		for (;;) {
			choice = prompt("Do you wish to start with this label? [yes]");
			if (choice[0] == 'y' || choice[0] == 'Y' ||
			    choice[0] == '\0')
				break;
			else if (choice[0] == 'n' || choice[0] == 'N') {
				hasdoslabel = 0;
				break;
			}
			else
				printf("Invalid selection -- please answer yes or no.\n");
		}
	}

	if (!hasbsdlabel) {
		while (getfirstlabel(dfd))
			 /* keep trying */ ;
	}
	bsdlabel.d_npartitions = MAXPARTITIONS;	/* XXX */

	while (op != EDIT && doingdos && (bsdlabel.d_ncylinders > MB_MAXCYL ||
		bsdlabel.d_nsectors > MB_MAXSECTOR)) {

		printf("\nWhen you share a disk between operating systems,\n");
		printf("you must use the same geometry specification for\n");
		printf("all of them.\n\n");

		if (bsdlabel.d_ncylinders >= MB_MAXCYL)
			printf("The current geometry has TOO MANY CYLINDERS.\n");

		if (bsdlabel.d_nsectors > MB_MAXSECTOR)
			printf("The current geometry has TOO MANY SECTORS per TRACK.\n");

		printf("\nPlease specify the mapped geometry DOS would use to access the disk.\n");
		while (getfirstlabel(dfd))
			 /* keep trying */ ;
		hasbsdlabel = 0;
	}

	if (!doingdos) {
		hasdoslabel = 0;
		bzero(doslabel, MB_MAXPARTS * sizeof(struct mbpart));
		if (hasbsdlabel && bsdstartsector > 0)
			movedbsd = 1;
	}
	else if (op == INTERACTIVE && hasdoslabel) {
		/* Overload the active flag to remember whether a partition
		   already existed. */
		for (i = 0; i < MB_MAXPARTS; i++) {
			if (doslabel[i].system == MBS_UNKNOWN) {
				doslabel[i].active = 0;
				bzero(&doslabel[i], sizeof(struct mbpart));
			}
			else
				doslabel[i].active |= P_EXISTED;
		}
	}
	else if (!hasdoslabel)	/* doingdos && no pre-existing dos label */
		bzero(doslabel, MB_MAXPARTS * sizeof(struct mbpart));

	/* XXX -- error if the end of the disk belongs to someone else */
	if (op == INTERACTIVE && doingdos && hasdoslabel) {
		int dsize;
		int endsect;

		if (bsdlabel.d_type != DTYPE_ESDI &&
		    bsdlabel.d_type != DTYPE_ST506)
			return;

		dsize = (bsdlabel.d_secperunit - bsdlabel.d_nsectors - 126) /
		    bsdlabel.d_nsectors * bsdlabel.d_nsectors;

		for (i = 0; i < MB_MAXPARTS; i++) {
			endsect = doslabel[i].start + doslabel[i].size;
			if (endsect > dsize && doslabel[i].system != MBS_BSDI) {
				fprintf(stderr, "The current FDISK partition table contains a\n");
				fprintf(stderr, "non-BSDI partition which extends near or to the end of\n");
				fprintf(stderr, "the disk.  BSD/386 requires that the end of the disk\n");
				fprintf(stderr, "contain the BSD/386 bad block table for IDE or ESDI drives.\n");  
				fprintf(stderr, "You cannot use BSD/386 co-residency without moving the offending\n");
				fprintf(stderr, "FDISK partition.\n");
				exit(1);
			}
		}
	}
}


/*
 * Generate an fdisk label with a single entry covering the
 * disk with type BSDI.
 */
kludgedos(doslp, bsdlp)
	struct mbpart *doslp;
	struct disklabel *bsdlp;
{
	long dsize;
	long start, end;
	int numparts = 0;
	int numfilled = 0;
	int added_one, i;
	struct partition *partp;
	struct mbpart tmpdos[MB_MAXPARTS];
	struct mbpart *sortparts[MB_MAXPARTS];

	bzero(tmpdos, MB_MAXPARTS * sizeof(struct mbpart));

	/* find existing non-bsdi partitions */
	for (i = 0; i < bsdlp->d_npartitions; i++) {
		if (numfilled >= MB_MAXPARTS)
			break;
		if (i == (RAWPARTITION - 'a'))
			continue;
		partp = &bsdlp->d_partitions[i];
		if (partp->p_fstype == FS_MSDOS || partp->p_fstype == FS_OTHER) {
			if (partp->p_fstype == FS_MSDOS)
				tmpdos[numfilled].system = MBS_DOS12;
			tmpdos[numfilled].start = partp->p_offset;
			tmpdos[numfilled].size = partp->p_size;
			tmpdos[numfilled].active = MBA_NOTACTIVE;
			numfilled++;
		}
	}

	dsize = bsdlp->d_secperunit;
	if (bsdlp->d_type == DTYPE_ESDI || bsdlp->d_type == DTYPE_ST506)
		dsize = (dsize - bsdlp->d_nsectors - 126) /
		    bsdlp->d_nsectors * bsdlp->d_nsectors;

	/* make one big one if there are no non BSDI partitions */
	if (numfilled == 0) {
		tmpdos[0].start = 15;
		tmpdos[0].size = dsize;
		tmpdos[0].system = MBS_BSDI;
		tmpdos[0].active = MBA_NOTACTIVE;
		numfilled++;
	}

	for (;;) {
		/* sort the existing partitions */
		sorttheparts(tmpdos, bsdlp, sortparts);
		for (i = 0; i < MB_MAXPARTS; i++)
			bcopy(sortparts[i], &doslp[i], sizeof(struct mbpart));
		bcopy(doslp, tmpdos, MB_MAXPARTS * sizeof(struct mbpart));

		/* stop if they're all full */
		if (numfilled >= MB_MAXPARTS)
			break;

		/* look for a gap */
		start = 0;
		added_one = 0;
		for (i = 0; i < numfilled; i++) {
			if (start >= tmpdos[i].start) {
				start = tmpdos[i].start + tmpdos[i].size;
				continue;
			}
			tmpdos[numfilled].start = start;
			tmpdos[numfilled].size = tmpdos[i].start - start;
			tmpdos[numfilled].system = MBS_BSDI;
			tmpdos[numfilled].active = MBA_NOTACTIVE;
			numfilled++;
			added_one++;
			break;
		}
		if (!added_one)
			break;
	}

	/* make a partition for the rest of the disk if it's not already
	   done */
	if (tmpdos[numfilled - 1].start + tmpdos[numfilled - 1].size < dsize) {
		tmpdos[numfilled].start = tmpdos[i - 1].start + tmpdos[i - 1].size;
		tmpdos[numfilled].size = dsize - tmpdos[i].start;
		tmpdos[numfilled].system = MBS_BSDI;
		tmpdos[numfilled].active = MBA_NOTACTIVE;
		numfilled++;
	}

	/* copy it into the real one and fill in other fields */
	bcopy(tmpdos, doslp, MB_MAXPARTS * sizeof(struct mbpart));
	checkdoslabel(doslp, bsdlp);
}

/*
 * Make sure an existing FDISK label makes sense with the
 * geometry we have in the bsd label.  If not, we can't use
 * the fdisk label.
 */
checkdosgeom(dlp, dl)
	struct mbpart *dlp;
	struct disklabel *dl;
{
	int i;
	char *ans;
	long start, end, size;

	for (i = 0; i < MB_MAXPARTS; i++) {
		if (dlp[i].system == MBS_UNKNOWN)
			continue;
		start = mbpscyl(&dlp[i]) * dl->d_secpercyl +
		    dlp[i].shead * dl->d_nsectors + mbpssec(&dlp[i]);
		end = mbpecyl(&dlp[i]) * dl->d_secpercyl +
		    dlp[i].ehead * dl->d_nsectors + mbpesec(&dlp[i]);
		size = end - start + 1;
		if (start != dlp[i].start || size != dlp[i].size) {
			printf("\nGeometry mismatch between FDISK\n");
			printf("and BSD labels (fdisk partition %d).\n", i + 1);
			ans = prompt("\nIgnore the existing FDISK label? [no]");
			if (ans[0] == 'y' || ans[0] == 'Y') {
				hasdoslabel = 0;
				bzero(doslabel, MB_MAXPARTS * sizeof(struct mbpart));
				break;
			}
			else {
				printf("The same geometry must be used by all operating systems on the disk\n");
				printf("In this case, there is an existing FDISK table with a different\n");
				printf("geometry than the one you specified for BSD.  You will need to\n");
				printf("either determine the geometry used for existing partitions and use\n");
				printf("that geometry, or ignore the existing FDISK table (ignoring it will\n");
				printf("require you to re-install any other operating systems already \n");
				printf("on the disk).\n\n");
				z(1, "Aborting due to FDISK-BSD geometry mismatch");
			}
		}
	}
}

/*
 * Find out what master boot record the user wants, and set it up
 * if necessary (e.g., bootany).
 */
setupmbr()
{
	int i, fd, bpart;
	int dobootany = 0;
	char *ans, *start, *p, *q;
	struct bootany_data *bdp;
	char tmp[BUFSIZ];
	extern char *systemtostring();

	if (mbootx != NULL) {
		if ((fd = open(mbootx, O_RDONLY)) < 0) {
			fprintf(stderr, "%s: couldn't open %s: %s\n", progname,
			    mbootx, strerror(errno));
		}
		else if (read(fd, &mboot, sizeof(mboot)) < sizeof(mboot) ||
		    mboot.signature != MB_SIGNATURE) {
			fprintf(stderr, "*** %s: invalid MBR (%s)\n", progname,
			    mbootx);
			close(fd);
		}
		else
			return;	/* we succeeded */
	}

	mbootx = boot2;

	printf("\nThe master boot record (MBR) contains the FDISK\n");
	printf("partition table used by all operating systems,\n");
	printf("and loads the bootstrap for the one that is\n");
	printf("marked `active' when the machine boots.\n\n");
	printf("BSD/386 supplies an MBR (bootany) which presents a\n");
	printf("menu of OS choices and enables you select one.\n");
	printf("If no choice is selected within a timeout period,\n");
	printf("the OS booted most recently is booted.\n");

	printf("\nIf you wish to install the bootany MBR, accept the\n");
	printf("default answer and %s will ask you further questions.  If\n",
	    progname);
	printf("you have your own boot block, you can specify the path\n");
	printf("to the MBR image.  %s will only modify the FDISK table\n",
	    progname);
	printf("for user supplied boot blocks\n");

	for (;;) {
		printf("\nPlease enter the path name of the\n");
		ans = prompt("MBR you wish to install [BSD/386 bootany]");

		if (ans[0] == '\0') {
			dobootany++;
			(void) strcpy(mbootx, _PATH_BOOTANY);
		}
		else
			(void) strcpy(mbootx, ans);

		if ((fd = open(mbootx, O_RDONLY)) < 0) {
			fprintf(stderr, "%s: couldn't open %s: %s\n", progname,
			    mbootx, strerror(errno));
			continue;
		}

		if (read(fd, &mboot, sizeof(mboot)) < sizeof(mboot) ||
		    mboot.signature != MB_SIGNATURE) {
			fprintf(stderr, "%s: invalid MBR (%s)\n", progname,
			    mbootx);
			close(fd);
			continue;
		}

		/* got a valid MBR */
		close(fd);
		break;
	}

	if (dobootany) {
		/* fill in the extra bootany data */
		bdp = (struct bootany_data *)
		    & mboot.code[MB_PARTOFF - sizeof(struct bootany_data)];

		printf("\nThe bootany boot block can boot from up to %d\n",
		    MAX_BOOTANY_PARTS);
		printf("different partitions.\n");

		bpart = 0;
		for (i = 0; i < MB_MAXPARTS && bpart < MAX_BOOTANY_PARTS; i++) {

			if (doslabel[i].system == MBS_UNKNOWN)
				continue;

			if (doslabel[i].active == MBA_ACTIVE)
				doslabel[i].active = MBA_NOTACTIVE;	/* XXX */

			for (;;) {
				printf("\nFDISK partition entry %d has type 0x%x (%s)\n",
				    i + 1, doslabel[i].system,
				    systemtostring(doslabel[i].system));
				ans = prompt("Is this partition bootable? [yes]");
				if (ans[0] == 'n' || ans[0] == 'N')
					goto skip;
				if (ans[0] == 'y' || ans[0] == 'Y' ||
				    ans[0] == '\0')
					break;
				printf("Invalid response.\n");
			}

			for (;;) {
				printf("\nWhat string should identify this partition\n");
				(void) sprintf(tmp, "in the menu (%d chars max.)? [%s]",
				    BOOTANY_TEXT_LEN,
				    systemtostring(doslabel[i].system));
				ans = prompt(tmp);
				if (ans[0] == '\0')
					(void) strcpy(ans, systemtostring(doslabel[i].system));
				if (strlen(ans) > BOOTANY_TEXT_LEN) {
					printf("Invalid string `%s'\n", ans);
					continue;
				}
				/* fill in the text (space extended, no
				   '\0') */
				start = bdp->part_desc[bpart].text;
				for (p = start; p < start + BOOTANY_TEXT_LEN; p++)
					*p = ' ';
				for (p = start, q = ans; *q; p++, q++)
					*p = *q;
				bdp->part_desc[bpart].partnum = i + 1;
				bdp->part_desc[bpart].term = MBA_ACTIVE;
				bpart++;
				break;
			}
	skip:		;
		}
		/* fill the rest of the spaces with nulls */
		for (; bpart < MAX_BOOTANY_PARTS; bpart++) {
			start = bdp->part_desc[bpart].text;
			for (p = start; p < start + BOOTANY_TEXT_LEN; p++)
				*p = ' ';
			bdp->part_desc[bpart].partnum = 0;
			bdp->part_desc[bpart].partnum = MBA_NOTACTIVE;
		}
	}
}

/*
 * Edit a label using your favorite editor
 */
edit()
{
	FILE *fd;
	char *ans;
	char *mktemp();

	(void) mktemp(tmpfil);
	fd = fopen(tmpfil, "w");
	z(fd == NULL, "Can't create tmpfile %s", tmpfil);
	if (doingdos)
		disklabel_display(specname, fd, &bsdlabel, doslabel);
	else
		disklabel_display(specname, fd, &bsdlabel, NULL);
	fclose(fd);

	for (;;) {
		if (!editit())
			exit(1);
		fd = fopen(tmpfil, "r");
		z(fd == NULL, "Can't reopen %s for reading", tmpfil);
		bzero(&bsdlabel, sizeof(bsdlabel));
		bzero(doslabel, MB_MAXPARTS * sizeof(struct mbpart));
		if (disklabel_getasciilabel(fd, &bsdlabel, doslabel))
			return;
		for (;;) {
			ans = prompt("\nRe-edit the label? [yes]");
			if (ans[0] == 'y' || ans[0] == 'Y' || ans[0] == '\0')
				break;
			else if (ans[0] == 'n' || ans[0] == 'N')
				z(1, "invalid label after edit complete");
		}
	}
	(void) unlink(tmpfil);
	return (1);
}

editit()
{
	register int pid, xpid;
	int stat, omask;
	extern char *getenv();

	omask = sigblock(sigmask(SIGINT) | sigmask(SIGQUIT) | sigmask(SIGHUP));
	while ((pid = fork()) < 0) {
		extern int errno;

		if (errno == EPROCLIM) {
			fprintf(stderr, "You have too many processes\n");
			return (0);
		}
		if (errno != EAGAIN) {
			perror("fork");
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
		perror(ed);
		exit(1);
	}
	while ((xpid = wait(&stat)) >= 0)
		if (xpid == pid)
			break;
	sigsetmask(omask);
	return (!stat);
}


/*
 * Label a disk
 *
 *	3 modes:
 *		read label
 *		restore label from file
 *		interactive label editor
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	char ch;
	int diskfd;
	int nowrite = 0;
	int oflag = 0;
	char *protofile;
	char *ans;
	extern char *optarg;
	extern int optind;
	extern int errno;

	progname = rindex(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		progname++;
	while ((ch = getopt(argc, argv, "efIiNnRWx:B")) != EOF)
		switch (ch) {
		case 'f':
			forceboot++;
			break;
		case 'I':
			ignorelabel++;
			if (ignorelabel > 1)
				oflag |= O_NONBLOCK;
			break;
		case 'n':
			nowrite++;
			break;
		case 'e':
			if (op != UNSPEC)
				usage();
			op = EDIT;
			break;
		case 'i':
			if (op != UNSPEC)
				usage();
			op = INTERACTIVE;
			break;
		case 'R':
			if (op != UNSPEC)
				usage();
			op = RESTORE;
			oflag |= O_NONBLOCK;
			break;
		case 'x':
			savename = optarg;
			break;
		case 'N':
			if (op != UNSPEC)
				usage();
			op = W_DISABLE;
			oflag |= O_NONBLOCK;
			break;
		case 'W':
			if (op != UNSPEC)
				usage();
			op = W_ENABLE;
			oflag |= O_NONBLOCK;
			break;
		case 'B':
			if (op != UNSPEC)
				usage();
			op = BOOTBLKS;
			forceboot++;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (op == UNSPEC) {
		op = READ;
		nowrite++;
	}

	switch (op) {
	case READ:
	case INTERACTIVE:
	case W_ENABLE:
	case W_DISABLE:
		if (argc < 1) {
			fprintf(stderr, "%s: missing disk specification\n",
			    progname);
			usage();
		}
		diskname = argv[0];
		if (op == READ)
			doingdos++;
		break;
	case EDIT:
	case BOOTBLKS:
		if (argc < 1) {
			fprintf(stderr, "%s: missing disk specification\n",
			    progname);
			usage();
		}
		diskname = argv[0];
		if (argc >= 3) {
			xxboot = argv[1];
			bootxx = argv[2];
		}
		if (argc >= 4)
			mbootx = argv[3];
		break;
	case RESTORE:
		if (argc < 1) {
			fprintf(stderr, "%s: missing disk specification\n",
			    progname);
			usage();
		}
		diskname = argv[0];
		if (argc < 2) {
			fprintf(stderr,
			    "%s: missing prototype file specification\n",
			    progname);
			usage();
		}
		protofile = argv[1];
		if (argc >= 4) {
			xxboot = argv[2];
			bootxx = argv[3];
		}
		if (argc >= 5)
			mbootx = argv[4];
		break;
	default:
		z(1, "Unknown op !?!");
	}

	if (diskname[0] == '/')
		(void) strcpy(specname, diskname);
	else
		(void) sprintf(specname, "/dev/r%s%c", diskname,
		    RAWPARTITION);

	if (op == INTERACTIVE) {
		/* make sure we're happy before wasting time */
		setup();
		z(LINES < 24 || COLS < 80,
		    "You need a window at least 24x80; yours is %dx%d",
		    LINES, COLS);
		cleanup();
	}


	diskfd = open(specname, (nowrite ? O_RDONLY : O_RDWR) | oflag);
	z(diskfd < 0, "open of %s failed: %s", specname, strerror(errno));

	if (op == W_DISABLE || op == W_ENABLE) {
		if (op == W_DISABLE)
			diocwlabel(diskfd, 0);
		else
			diocwlabel(diskfd, 1);
		exit (0);
   	} 

	if (op == READ || op == INTERACTIVE || op == EDIT || op == BOOTBLKS)
		getcurrentlabels(diskfd);

	if (op == READ) {
		if (hasdoslabel)
			disklabel_display(specname, stdout, &bsdlabel, doslabel);
		else
			disklabel_display(specname, stdout, &bsdlabel, NULL);
		exit(0);
	}

	if (op == INTERACTIVE || op == EDIT) {
		/* 
		 * Check the geometry match between BSD and FDISK labels 
		 * NOTE: don't check geometry match if using EDIT mode
		 */
		if (op == INTERACTIVE && hasdoslabel && !hasbsdlabel)
			checkdosgeom(doslabel, &bsdlabel);

		if (!hasbsdlabel && doingdos) {

			int rawpartition = RAWPARTITION - 'a';

			/* clear out all but RAWPARTITION */
			bzero(&bsdlabel.d_partitions[0],
			    rawpartition * sizeof(struct partition));
			bzero(&bsdlabel.d_partitions[rawpartition + 1],
			    (bsdlabel.d_npartitions - (rawpartition + 1)) *
			    sizeof(struct partition));
		}

		if (doingdos && !hasdoslabel && (op == EDIT || hasbsdlabel))
			kludgedos(doslabel, &bsdlabel);
	}

	if (op == INTERACTIVE) {
		setup();
		if (doingdos) {
			int bsdpart;

			/* kludge the BSDI fdisk entry if it's at the start
			   of the disk to reflect a 0 offset and 15 sectors
			   more space */
			bsdpart = rootpart(doslabel);
			if (bsdpart >= 0 && doslabel[bsdpart].start == 15) {
				doslabel[bsdpart].start = 0;
				doslabel[bsdpart].size += 15;
			}

			interactive(doslabel, &bsdlabel);

			/* put the fdisk entry back */
			bsdpart = rootpart(doslabel);
			if (bsdpart >= 0 && doslabel[bsdpart].start == 0) {
				doslabel[bsdpart].start = 15;
				doslabel[bsdpart].size -= 15;
			}
		}
		else
			interactive2(&bsdlabel);
		cleanup();

		z(disklabel_checklabel(&bsdlabel),
		    "errors in disklabel after interactive");
		z(doingdos && checkdoslabel(doslabel, &bsdlabel),
		    "errors in FDISK label after interactive");
		z(doingdos && rootpart(doslabel) < 0,
		    "no BSD partition in FDISK table after interactive");
	}


	if (op == EDIT)
		edit();

	if (op == RESTORE) {
		FILE *fp;

		z((fp = fopen(protofile, "r")) == NULL,
		    "couldn't open label file %s", protofile);
		bzero(&bsdlabel, sizeof(bsdlabel));
		bzero(doslabel, MB_MAXPARTS * sizeof(struct mbpart));
		z(!disklabel_getasciilabel(fp, &bsdlabel, doslabel),
			"errors parsing label file");
		fclose(fp);
	}


	if (op == EDIT || op == RESTORE || op == BOOTBLKS) {
		if (rootpart(doslabel) >= 0)
			doingdos++;
		else
			doingdos = 0;
	}

	if (savename != NULL) {
		FILE *fp;
		char tmp[MAXPATHLEN];

		(void) sprintf(tmp, "%s.label", savename);
		fp = fopen(tmp, "w");
		if (doingdos)
			disklabel_display(specname, fp, &bsdlabel, doslabel);
		else
			disklabel_display(specname, fp, &bsdlabel, NULL);
		fclose(fp);
	}

	/* guess we'll start at the beginning of the disk */
	bsdoffset = 0;

	/* XXX - The interactive stuff doesn't currently keep track of
	   whether or not an existing BSD label was moved or not, so we'll
	   force writing the boot blocks for now */
	forceboot = 1;

	/* if moving bsd label, no bsd label before, or force -- install
	   boot */
	if (!hasbsdlabel || movedbsd || forceboot) {
		if (doingdos) {
			int bsdpart;
			struct mbpart *mpp;

			bsdpart = rootpart(doslabel);
			z(bsdpart < 0, "No BSD partition in fdisk table");
			mpp = &doslabel[bsdpart];
			if (mbpssec(mpp) == 15 && mbpstrk(mpp) == 0 &&
			    mbpscyl(mpp) == 0)
				bsdoffset = 0;
			else
				bsdoffset = mpp->start;
			mpp->active = MBA_ACTIVE;	/* XXX */
		}
		bootarealp = makebsdbootarea(bootarea, &bsdlabel,
		    doingdos && bsdoffset == 0 ? 15 : 0);
	}

	/* we have a valid boot area and a label, so copy it into place */
	bcopy(&bsdlabel, bootarealp, sizeof(bsdlabel));

	if (doingdos) {
		if (hasdoslabel) {
			printf("\nThere was a previously existing master boot record on the\n");
			printf("disk.  You can either keep using the exitising MBR (with an\n");
			printf("updated FDISK partition table), or you can replace it with a\n");
			printf("different or updated MBR.\n");
			for (;;) {
				ans = prompt("\nDo you wish to keep the existing MBR (yes/no)? [yes]");
				if (ans[0] == 'N' || ans[0] == 'n') {
					setupmbr();
					hasdoslabel = 0;
					break;
				}
				if (ans[0] == 'y' || ans[0] == 'Y' || ans[0] == '\0')
					break;
				printf("Invalid response.\n");
			}
		}
		else
			setupmbr();

		/* we have an mbr and an fdisk table so copy it into place */
		bcopy(doslabel, mbparts(&mboot), sizeof(doslabel));

		if (savename != NULL) {
			int fd;
			char tmp[MAXPATHLEN];

			(void) sprintf(tmp, "%s.mbr", savename);
			if ((fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC)) >= 0) {
				write(fd, &mboot, sizeof(mboot));
				close(fd);
			}
			else
				fprintf(stderr, "%s: couldn't open %s: %s\n",
				    progname, tmp, strerror(errno));
		}

	}

	if (nowrite) {
		fprintf(stderr, "%s: Writing disabled -- Exiting!\n", progname);
		exit(0);
	}

	/* write the BSD label */
	for (;;) {
		int n;

		printf("\nReady to write BSD label");
		if (forceboot || movedbsd || !hasbsdlabel)
			printf(" and boot blocks.\n");
		else
			printf(".\n");
		ans = prompt("Proceed? [yes]");
		if (ans[0] == 'y' || ans[0] == 'Y' || ans[0] == '\0') {
			n = writelabel(diskfd, bootarea, bootarealp, bsdoffset);
			if (n == 0)
			    printf("BSD boot blocks written successfully.\n");
			break;
		}
		if (n != 0 || ans[0] == 'n' || ans[0] == 'N') {
			printf("WARNING: BSD LABEL NOT WRITTEN!\n");
			break;
		}

	}

	/* write the MBR (and fdisk table) */
	if (doingdos) {
		for (;;) {
			printf("\nReady to write FDISK label");
			if (!hasdoslabel)
				printf(" and MBR.\n");
			else
				printf(".\n");
			ans = prompt("Proceed? [yes]");
			if (ans[0] == 'y' || ans[0] == 'Y' || ans[0] == '\0') {
				z(lseek(diskfd, 0L, L_SET),
				    "seek failed -- failed to write FDISK label");
				z(write(diskfd, &mboot, sizeof(mboot)) < sizeof(mboot),
				    "error writing FDISK label");
				printf("FDISK label written successfully.\n");
				break;
			}
			else if (ans[0] == 'n' || ans[0] == 'N') {
				printf("WARNING: FDISK LABEL NOT WRITTEN!\n");
				break;
			}
		}
	}

	close(diskfd);
	exit(0);
}
