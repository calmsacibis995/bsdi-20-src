/*
 * Copyright (c) 1993, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI disklabel.c,v 2.1 1995/02/03 09:18:01 polk Exp	*/

/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Symmetric Computer Systems.
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
 */

#if 0	/* not in library version */
#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1987, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)disklabel.c	8.2 (Berkeley) 1/7/94";
/* from static char sccsid[] = "@(#)disklabel.c	1.2 (Symmetric) 11/28/85"; */
#endif /* not lint */
#endif

#include <sys/param.h>
#include <ufs/ffs/fs.h>
#define DKTYPENAMES
#include <sys/disklabel.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __i386__
#include <machine/bootblock.h>
#endif

#define streq(a,b)      (strcmp(a,b) == 0)

/*
 * Display a label in a standard text form.  This output is in
 * form used by disklabel -R and -e, and includes an extended
 * section for the DOS label
 */
void
#ifdef __i386__
disklabel_display(specname, f, lp, dlp)
#else
disklabel_display(specname, f, lp)
#endif
	char *specname;
	FILE *f;
	register struct disklabel *lp;
#ifdef __i386__
	register struct mbpart *dlp;
#endif
{
	register int i, j;
	register struct partition *pp;

	fprintf(f, "# %s:\n", specname);
	if ((unsigned) lp->d_type < DKMAXTYPES)
		fprintf(f, "type: %s\n", dktypenames[lp->d_type]);
	else
		fprintf(f, "type: %d\n", lp->d_type);
	fprintf(f, "disk: %.*s\n", sizeof(lp->d_typename), lp->d_typename);
	fprintf(f, "label: %.*s\n", sizeof(lp->d_packname), lp->d_packname);
	fprintf(f, "flags:");
	if (lp->d_flags & D_REMOVABLE)
		fprintf(f, " removable");
	if (lp->d_flags & D_ECC)
		fprintf(f, " ecc");
	if (lp->d_flags & D_BADSECT)
		fprintf(f, " badsect");
	if (lp->d_flags & D_ZONE)
		fprintf(f, " zone-recorded");
	if (lp->d_flags & D_SOFT)
		fprintf(f, " driver-generated");
	if (lp->d_flags & D_DEFAULT)
		fprintf(f, " default_geometry");
	fprintf(f, "\n");
	fprintf(f, "bytes/sector: %lu\n", lp->d_secsize);
	fprintf(f, "sectors/track: %lu\n", lp->d_nsectors);
	fprintf(f, "tracks/cylinder: %lu\n", lp->d_ntracks);
	fprintf(f, "sectors/cylinder: %lu\n", lp->d_secpercyl);
	fprintf(f, "cylinders: %lu\n", lp->d_ncylinders);
	fprintf(f, "sectors/unit: %lu\n", lp->d_secperunit);
	fprintf(f, "replacement sectors/track: %d\n", lp->d_sparespertrack);
	fprintf(f, "replacement sectors/cylinder: %d\n", lp->d_sparespercyl);
	fprintf(f, "alternate cylinders: %lu\n", lp->d_acylinders);
	fprintf(f, "rpm: %d\n", lp->d_rpm);
	fprintf(f, "interleave: %d\n", lp->d_interleave);
	fprintf(f, "trackskew: %d\n", lp->d_trackskew);
	fprintf(f, "cylinderskew: %d\n", lp->d_cylskew);
	fprintf(f, "headswitch: %lu\t\t# milliseconds\n", lp->d_headswitch);
	fprintf(f, "track-to-track seek: %lu\t# milliseconds\n", lp->d_trkseek);
	fprintf(f, "drivedata: ");
	for (i = NDDATA - 1; i >= 0; i--)
		if (lp->d_drivedata[i])
			break;
	if (i < 0)
		i = 0;
	for (j = 0; j <= i; j++)
		fprintf(f, "%lu ", lp->d_drivedata[j]);
	fprintf(f, "\n\n%d partitions:\n", lp->d_npartitions);
	fprintf(f,
	    "#        size   offset    fstype   [fsize bsize   cpg]\n");
	pp = lp->d_partitions;
	for (i = 0; i < lp->d_npartitions; i++, pp++) {
		if (pp->p_size) {
			fprintf(f, "  %c: %8lu %8lu  ", 'a' + i,
			    pp->p_size, pp->p_offset);
			if ((unsigned) pp->p_fstype < FSMAXTYPES)
				fprintf(f, "%8.8s", fstypenames[pp->p_fstype]);
			else
				fprintf(f, "%8d", pp->p_fstype);
			switch (pp->p_fstype) {

			case FS_UNUSED:	/* XXX */
				fprintf(f, "    %5lu %5lu %5.5s ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag, "");
				break;

			case FS_BSDFFS:
				fprintf(f, "    %5lu %5lu %5d ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag,
				    pp->p_cpg);
				break;

			default:
				fprintf(f, "%20.20s", "");
				break;
			}
			fprintf(f, "\t# (Cyl. %4lu",
			    pp->p_offset / lp->d_secpercyl);
			if (pp->p_offset % lp->d_secpercyl)
				putc('*', f);
			else
				putc(' ', f);
			fprintf(f, "- %lu",
			    (pp->p_offset +
				pp->p_size + lp->d_secpercyl - 1) /
			    lp->d_secpercyl - 1);
			if (pp->p_size % lp->d_secpercyl)
				putc('*', f);
			fprintf(f, ")\n");
		}
	}
#ifdef __i386__
	if (dlp != NULL) {
		fprintf(f, "\nFDISK table:\n");
		fprintf(f, "#        size   offset scyl shd ssc ecyl ehd esc    type\n");
		for (i = 0; i < MB_MAXPARTS; i++) {
			fprintf(f, " %c%d: %8ld %8ld %4d %3d %3d %4d %3d %3d ",
			    dlp[i].active == MBA_ACTIVE ? '*' : ' ',
			    i + 1, dlp[i].size, dlp[i].start,
			    mbpscyl(&dlp[i]), mbpstrk(&dlp[i]), 
			    mbpssec(&dlp[i])+1, mbpecyl(&dlp[i]), 
			    mbpetrk(&dlp[i]), mbpesec(&dlp[i])+1);
			fprintf(f, "  0x%02x", dlp[i].system);
			if (dlp[i].system == MBS_BSDI)
				fprintf(f, "\t# (BSDI)\n");
			else if (MBS_ISDOS(dlp[i].system))
				fprintf(f, "\t# (DOS)\n");
			else if (dlp[i].system == MBS_UNKNOWN)
				fprintf(f, "\t# (Unused)\n");
			else
				fprintf(f, "\t# (OTHER)\n");
		}
	}
#endif /* __i386__ */
	fflush(f);
}

char *
disklabel_skip(cp)
	register char *cp;
{

	while (*cp != '\0' && isspace(*cp))
		cp++;
	if (*cp == '\0' || *cp == '#')
		return ((char *) NULL);
	return (cp);
}

char *
disklabel_word(cp)
	register char *cp;
{
	register char c;

	while (*cp != '\0' && !isspace(*cp) && *cp != '#')
		cp++;
	if ((c = *cp) != '\0') {
		*cp++ = '\0';
		if (c != '#')
			return (disklabel_skip(cp));
	}
	return ((char *) NULL);
}

/*
 * Read an ascii label in from fd f,
 * in the same format as that put out by display(),
 * and fill in lp.
 */
int
#ifdef __i386__
disklabel_getasciilabel(f, lp, dlp)
#else
disklabel_getasciilabel(f, lp)
#endif
	FILE *f;
	register struct disklabel *lp;
#ifdef __i386__
	register struct mbpart *dlp;
#endif
{
	register char **cpp, *cp;
	register struct partition *pp;
	char *tp, *s, line[BUFSIZ], *index();
	int v, lineno = 0, errors = 0, active, tmp;
#ifdef __i386__
	struct mbpart dlpbuf[MB_MAXPARTS];

	if (dlp == NULL)
		dlp = dlpbuf;
#endif

	lp->d_bbsize = BBSIZE;	/* XXX */
	lp->d_sbsize = SBSIZE;	/* XXX */
	while (fgets(line, sizeof(line) - 1, f)) {
		lineno++;
		if (cp = index(line, '\n'))
			*cp = '\0';
		cp = disklabel_skip(line);
		if (cp == NULL)
			continue;
		tp = index(cp, ':');
		if (tp == NULL) {
			warnx("line %d: syntax error", lineno);
			errors++;
			continue;
		}
		*tp++ = '\0', tp = disklabel_skip(tp);
		if (streq(cp, "type")) {
			if (tp == NULL)
				tp = "unknown";
			cpp = dktypenames;
			for (; cpp < &dktypenames[DKMAXTYPES]; cpp++)
				if ((s = *cpp) && streq(s, tp)) {
					lp->d_type = cpp - dktypenames;
					goto next;
				}
			v = atoi(tp);
			if ((unsigned) v >= DKMAXTYPES)
				warnx("line %d: unknown disk type %d",
				    lineno, v);
			lp->d_type = v;
			continue;
		}
		if (streq(cp, "flags")) {
			for (v = 0; (cp = tp) && *cp != '\0';) {
				tp = disklabel_word(cp);
				if (streq(cp, "removable") ||
				    streq(cp, "removeable"))
					v |= D_REMOVABLE;
				else if (streq(cp, "ecc"))
					v |= D_ECC;
				else if (streq(cp, "badsect"))
					v |= D_BADSECT;
				else if (streq(cp, "zone-recorded"))
					v |= D_ZONE;
				else if (streq(cp, "driver-generated"));
				else if (streq(cp, "default_geometry"));
				else {
					warnx("line %d: %s: bad flag",
					    lineno, cp);
					errors++;
				}
			}
			lp->d_flags = v;
			continue;
		}
		if (streq(cp, "drivedata")) {
			register int i;

			for (i = 0; (cp = tp) && *cp != '\0' && i < NDDATA;) {
				lp->d_drivedata[i++] = atoi(cp);
				tp = disklabel_word(cp);
			}
			continue;
		}
		if (sscanf(cp, "%d partitions", &v) == 1) {
			char *tmp;

			tmp = index(cp, 'p');
			if (tmp != NULL && streq(tmp, "partitions")) {
				if (v == 0 || (unsigned) v > MAXPARTITIONS) {
					warnx("line %d: bad # of partitions",
					    lineno);
					lp->d_npartitions = MAXPARTITIONS;
					errors++;
				}
				else
					lp->d_npartitions = v;
				continue;
			}
		}
		if (tp == NULL)
			tp = "";
		if (streq(cp, "disk")) {
			strncpy(lp->d_typename, tp, sizeof(lp->d_typename));
			continue;
		}
		if (streq(cp, "label")) {
			strncpy(lp->d_packname, tp, sizeof(lp->d_packname));
			continue;
		}
		if (streq(cp, "bytes/sector")) {
			v = atoi(tp);
			if (v <= 0 || (v % 512) != 0) {
				warnx("line %d: %s: bad sector size",
				    lineno, tp);
				errors++;
			}
			else
				lp->d_secsize = v;
			continue;
		}
		if (streq(cp, "sectors/track")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_nsectors = v;
			continue;
		}
		if (streq(cp, "sectors/cylinder")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_secpercyl = v;
			continue;
		}
		if (streq(cp, "tracks/cylinder")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_ntracks = v;
			continue;
		}
		if (streq(cp, "cylinders")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_ncylinders = v;
			continue;
		}
		if (streq(cp, "sectors/unit")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_secperunit = v;
			continue;
		}
		if (streq(cp, "replacement sectors/track")) {
			v = atoi(tp);
			if (v < 0 || v >= lp->d_nsectors) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_sparespertrack = v;
			continue;
		}
		if (streq(cp, "replacement sectors/cylinder")) {
			v = atoi(tp);
			if (v != lp->d_nsectors * lp->d_ntracks -
			    lp->d_secpercyl) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_sparespercyl = v;
			continue;
		}
		if (streq(cp, "alternate cylinders")) {
			v = atoi(tp);
			if (v < 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_acylinders = v;
			continue;
		}
		if (streq(cp, "rpm")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_rpm = v;
			continue;
		}
		if (streq(cp, "interleave")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_interleave = v;
			continue;
		}
		if (streq(cp, "trackskew")) {
			v = atoi(tp);
			if (v < 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_trackskew = v;
			continue;
		}
		if (streq(cp, "cylinderskew")) {
			v = atoi(tp);
			if (v < 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_cylskew = v;
			continue;
		}
		if (streq(cp, "headswitch")) {
			v = atoi(tp);
			if (v < 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_headswitch = v;
			continue;
		}
		if (streq(cp, "track-to-track seek")) {
			v = atoi(tp);
			if (v < 0) {
				warnx("line %d: %s: bad %s",
				    lineno, tp, cp);
				errors++;
			}
			else
				lp->d_trkseek = v;
			continue;
		}
		if ('a' <= *cp && *cp <= 'z' && cp[1] == '\0') {
			unsigned part = *cp - 'a';

			if (part > lp->d_npartitions) {
				warnx("line %d: bad partition name", lineno);
				errors++;
				continue;
			}
			pp = &lp->d_partitions[part];
#define NXTNUM(n) { \
	cp = tp, tp = disklabel_word(cp); \
	if (tp == NULL) \
		tp = cp; \
	(n) = atoi(cp); \
     }

			NXTNUM(v);
			if (v < 0) {
				warnx("line %d: %s: bad partition size",
				    lineno, cp);
				errors++;
			}
			else
				pp->p_size = v;
			NXTNUM(v);
			if (v < 0) {
				warnx("line %d: %s: bad partition offset",
				    lineno, cp);
				errors++;
			}
			else
				pp->p_offset = v;
			cp = tp, tp = disklabel_word(cp);
			cpp = fstypenames;
			for (; cpp < &fstypenames[FSMAXTYPES]; cpp++)
				if ((s = *cpp) && streq(s, cp)) {
					pp->p_fstype = cpp - fstypenames;
					goto gottype;
				}
			if (isdigit(*cp))
				v = atoi(cp);
			else
				v = FSMAXTYPES;
			if ((unsigned) v >= FSMAXTYPES) {
				warnx("line %d: %s %s", lineno,
				    "Warning, unknown filesystem type", cp);
				v = FS_UNUSED;
			}
			pp->p_fstype = v;
	gottype:

			switch (pp->p_fstype) {

			case FS_UNUSED:	/* XXX */
				NXTNUM(pp->p_fsize);
				if (pp->p_fsize == 0)
					break;
				NXTNUM(v);
				pp->p_frag = v / pp->p_fsize;
				break;

			case FS_BSDFFS:
				NXTNUM(pp->p_fsize);
				if (pp->p_fsize == 0)
					break;
				NXTNUM(v);
				pp->p_frag = v / pp->p_fsize;
				NXTNUM(pp->p_cpg);
				break;

			default:
				break;
			}
			continue;
		}
#ifdef __i386__
		if (streq(cp, "FDISK table"))
			continue;
		if (*cp == '*' && isdigit(cp[1]) && cp[2] == '\0') {
			cp++;
			active = MBA_ACTIVE;
		}
		else
			active = MBA_NOTACTIVE;
		if (isdigit(*cp) && cp[1] == '\0') {
			unsigned part = atoi(cp) - 1;

			if (part > MB_MAXPARTS) {
				warnx("line %d: bad FDISK partition number",
				    lineno);
				errors++;
				continue;
			}
#define CHECKNUM(n, str, a) { \
	NXTNUM(n); \
	if ((n) < 0) { \
		warnx("line %d: %s: bad FDISK %s",  \
			lineno, cp, (str)); \
		errors++; \
		continue; \
	} \
	else \
		(a) = (n); \
}
			CHECKNUM(v, "partition size", dlp[part].size);
			CHECKNUM(v, "partition offset", dlp[part].start);

			CHECKNUM(v, "partition scyl", tmp);
			CHECKNUM(v, "partition shd", dlp[part].shead);
			CHECKNUM(v, "partition ssc", dlp[part].ssec);
			if ((dlp[part].ssec & MB_SECMASK) != dlp[part].ssec ||
				(dlp[part].size && dlp[part].ssec == 0)) {
				warnx("line %d: %s: bad FDISK partition ssec",
				    lineno, cp);
				errors++;
				continue;
			}
			if (tmp >= MB_MAXCYL) {
				warnx("line %d: %s: bad FDISK partition scyl",
				    lineno, cp);
				errors++;
				continue;
			}
                	dlp[part].ssec |= ((tmp >> MB_CYLSHIFT) & MB_CYLMASK);
                	dlp[part].scyl = (char) tmp;

			CHECKNUM(v, "partition ecyl", tmp);
			CHECKNUM(v, "partition ehd", dlp[part].ehead);
			CHECKNUM(v, "partition esc", dlp[part].esec);
			if ((dlp[part].esec & MB_SECMASK) != dlp[part].esec ||
				(dlp[part].size && dlp[part].esec == 0)) {
				warnx("line %d: %s: bad FDISK partition esec",
				    lineno, cp);
				errors++;
				continue;
			}
			if (tmp >= MB_MAXCYL) {
				warnx("line %d: %s: bad FDISK partition ecyl",
				    lineno, cp);
				errors++;
				continue;
			}
                	dlp[part].esec |= ((tmp >> MB_CYLSHIFT) & MB_CYLMASK);
                	dlp[part].ecyl = (char) tmp;

			cp = tp, tp = disklabel_word(cp);
			if (sscanf(cp, "0x%x", &v) == 1)
				dlp[part].system = v;
			else {
				warnx("line %d: %s: bad FDISK partition type",
				    lineno, cp);
				errors++;
			}
			dlp[part].active = active;
			continue;
		}
#endif /* __i386__ */
		warnx("line %d: %s: Unknown disklabel field",
		    lineno, cp);
		errors++;
next:
		;
	}
	errors += disklabel_checklabel(lp);
	return (errors == 0);
}

/*
 * Check disklabel for errors and fill in
 * derived fields according to supplied values.
 */
int
disklabel_checklabel(lp)
	register struct disklabel *lp;
{
	register struct partition *pp;
	int i, errors = 0;
	char part;

	if (lp->d_secsize == 0) {
		warnx("sector size %lu", lp->d_secsize);
		return (1);
	}
	if (lp->d_nsectors == 0) {
		warnx("sectors/track %lu", lp->d_nsectors);
		return (1);
	}
	if (lp->d_ntracks == 0) {
		warnx("tracks/cylinder %lu", lp->d_ntracks);
		return (1);
	}
	if (lp->d_ncylinders == 0) {
		warnx("cylinders/unit %lu", lp->d_ncylinders);
		errors++;
	}
	if (lp->d_rpm == 0)
		warnx("warning: revolutions/minute %d", lp->d_rpm);
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks -
		    lp->d_sparespercyl;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;
	if (lp->d_bbsize == 0) {
		warnx("boot block size %lu", lp->d_bbsize);
		errors++;
	}
	else if (lp->d_bbsize % lp->d_secsize)
		warnx("warning: boot block size %% sector-size != 0");
	if (lp->d_sbsize == 0) {
		warnx("super block size %lu", lp->d_sbsize);
		errors++;
	}
	else if (lp->d_sbsize % lp->d_secsize)
		warnx("warning: super block size %% sector-size != 0");
	if (lp->d_npartitions > MAXPARTITIONS)
		warnx("warning: number of partitions (%d) > MAXPARTITIONS (%d)",
		    lp->d_npartitions, MAXPARTITIONS);
	for (i = 0; i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size == 0 && pp->p_offset != 0)
			warnx("warning: partition %c: size 0, but offset %d",
			    part, pp->p_offset);
#ifdef notdef
		if (pp->p_size % lp->d_secpercyl)
			warnx("warning: partition %c: size %% cylinder-size != 0",
			    part);
		if (pp->p_offset % lp->d_secpercyl)
			warnx("warning: partition %c: offset %% cylinder-size != 0",
			    part);
#endif
		if (pp->p_offset > lp->d_secperunit) {
			warnx("partition %c: offset past end of unit", part);
			errors++;
		}
		if (pp->p_offset + pp->p_size > lp->d_secperunit) {
			warnx("partition %c: partition extends past end of unit",
			    part);
			errors++;
		}
	}
	for (; i < MAXPARTITIONS; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size || pp->p_offset)
			warnx("warning: unused partition %c: size %d offset %d",
			    'a' + i, pp->p_size, pp->p_offset);
	}
	return (errors);
}
