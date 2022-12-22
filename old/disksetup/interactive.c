/*-
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*	BSDI interactive.c,v 2.1 1995/02/03 06:40:13 polk Exp	*/

#ifdef __STDC__
#include <unistd.h>
#include <stdlib.h>
#endif

#include <curses.h>
#include <stdio.h>
#include "structs.h"
#include <pwd.h>
#include <ctype.h>

#ifdef CTRL
#undef CTRL
#endif
#define CTRL(x) ((x)&037)

#include <machine/bootblock.h>
#include <sys/disklabel.h>

#define ISDOS(n) ((n) == MBS_DOS12 || (n) == MBS_DOS16 || (n) == MBS_DOS4)

int everchanged;

WINDOW *iowin;
struct report_f *queuebase;
struct mbpart *sortparts[MB_MAXPARTS];	/* sorted partitions */

#define SIZELEN 9
#define TYPELEN 7

#define IGNOREIFEMPTY if (line[0] == '\0') { warn ("Ignored"); continue; }; clearline (warnline);

#define GETORIGNORE(a1,a2) checkget(a1, a2); IGNOREIFEMPTY

#define WARNIF(bool,mesg)  if (bool) { warn(mesg); continue; }

#define WARNIF2(bool,m1,m2)  if (bool) { warn(m1,m2); continue; }

#define CONFIRMOK(d) \
        if (dos_parts[d].active&P_EXISTED) {\
              if (confirm ("Changing original on-disk data -- ok?  y/n") != 'y')\
                   continue;\
              else dos_parts[d].active &= ~P_EXISTED;\
	}

/*
 *  bsdindos[2][0] is first BSD partition in FDISK part 3...
 */
char bsdindos[MB_MAXPARTS][MAXPARTITIONS + 1];
int nbsdindos[MB_MAXPARTS];	/* how many part's assigned */

int disksize;			/* calculated */
int physsecsize;		/* disksetup passes this in */
int secpercyl;			/* disksetup passes this in */
int nsectors;			/* disksetup passes this in */

int warnline;			/* different screens use different lines */
char unusedbsdpart[MAXPARTITIONS];

/*
 * Run the interactive processing for the case when a 
 * disk has an FDISK partition table (is shared between BSD and 
 * at least one other operating system
 */
interactive(dos_parts, bsd_parts)
	struct mbpart dos_parts[MB_MAXPARTS];
	struct disklabel *bsd_parts;
{
	int phase;
	int i, j;

	disksize = bsd_parts->d_secperunit;
	if (bsd_parts->d_type == DTYPE_ST506 || bsd_parts->d_type == DTYPE_ESDI)
		disksize = (disksize - bsd_parts->d_nsectors - 126)
		    / bsd_parts->d_nsectors * bsd_parts->d_nsectors;
	bsd_parts->d_partitions['c' - 'a'].p_size = bsd_parts->d_secperunit;
	bsd_parts->d_partitions['c' - 'a'].p_offset = 0;
	bsd_parts->d_partitions['c' - 'a'].p_fstype = FS_UNUSED;


	physsecsize = bsd_parts->d_secsize;
	secpercyl = bsd_parts->d_secpercyl;
	nsectors = bsd_parts->d_nsectors;

	bzero(bsdindos, (MAXPARTITIONS + 1) * MB_MAXPARTS);
	bzero(nbsdindos, MB_MAXPARTS * sizeof(int));

	bzero(unusedbsdpart, MAXPARTITIONS);

/* see if we can align any BSDI partitions with DOS partitions: */
	for (i = 0; i < MAXPARTITIONS; i++) {
		if (i == 'c' - 'a' || bsd_parts->d_partitions[i].p_size == 0) {
			unusedbsdpart[i] = 1;
			continue;
		}
		for (j = 0; j < MB_MAXPARTS; j++) {
			if (dos_parts[j].start <=
			    bsd_parts->d_partitions[i].p_offset &&
			    bsd_parts->d_partitions[i].p_offset <
			    dos_parts[j].start + dos_parts[j].size) {
				bsdindos[j][nbsdindos[j]++] = i + 'a';
				goto doneassigning;
			}
		}
doneassigning:	;
	}

	phase = 1;
	for (;;) {
		switch (phase) {
		case 1:
			phase = phase1(dos_parts, bsd_parts);
			break;
		case 2:
			phase = phase2(dos_parts, bsd_parts);
			break;
		default:
			return;
		}
	}
}

/*
 * Implement the first screen (FDISK sizing and BSD partition assignment)
 * for the primary interactive processing (when the disk is shared
 * between multiple operating systems
 */
phase1(dos_parts, bsd_parts)
	struct mbpart dos_parts[MB_MAXPARTS];
	struct disklabel *bsd_parts;
{
	int i, j;
	int size, start;
	int dosnum;
	char bsdpart;
	struct mbpart *m;

	warnline = WARNLINE;

	erase();
	mvprintw(TITLINE + 0, 12, "FDISK Partition Sizing & BSD Partition Assignment");
	mvprintw(TITLINE + 1, 0, "FDISK");
	mvprintw(TITLINE + 2, 0, "Part#  type        Start  Length    MB BSD Partitions    Warnings");
	mvprintw(CMDLINE, 0, "Commands:  [T]ype  [S]tart  [L]ength  [A]ssign BSD partition  [N]ext phase");
	mvprintw(DIRLINE + 0, 0, "Directions:");
	mvprintw(DIRLINE + 1, 3, "Use `L' and `S' to set the FDISK partition Lengths and Starting sectors");
	mvprintw(DIRLINE + 2, 3, "Use `T' to set the FDISK partition Types");
	mvprintw(DIRLINE + 3, 3, "Use `A' to assign the BSD partitions to cover the FDISK partitions");
	mvprintw(DIRLINE + 4, 3, "-- One BSD partition for non-BSDI FDISK partitions -- One or more for BSDI");
	mvprintw(DIRLINE + 5, 3, "-- Assignment is to the end of the partition list");
	mvprintw(DIRLINE + 6, 3, "Use `N' to move to the next phase:  BSD partition sizing");
	mvprintw(DIRLINE + 7, 3, "Use `X' to abort disksetup");

/**** The grand phase 1 input loop ***/
	for (;;) {
		char c;
		char line[LINELEN];
		int y, x;
		int lastpartendedat;

		sorttheparts(dos_parts, bsd_parts, sortparts);	/* order the FDISK
								   partitions */
		lastpartendedat = 0;
		for (i = 0; i < MB_MAXPARTS; i++) {
			m = sortparts[i];
			dosnum = m - &dos_parts[0];
			clearline(FIRSTDISKLINE + i);
			if (m->size == 0) {
				mvprintw(FIRSTDISKLINE + i, 0, "%3d  [--unused--]",
				    (m - &dos_parts[0]) + 1);
				continue;
			}
			mvprintw(FIRSTDISKLINE + i, 0, "%c%2d  [0x%02X %-5.5s]%7d%8d%6d ",
			    (m->active & P_EXISTED) ? '*' : ' ',
			    (m - &dos_parts[0]) + 1, m->system,
			    systemtostring(m->system), m->start,
			    m->size, m->size * physsecsize / 1024 / 1024);
			move(FIRSTDISKLINE + i, 38);
			if (bsdindos[dosnum][0] == '\0') {
				printw("--none--");
			}
			else {
				for (j = 0; bsdindos[dosnum][j]; j++)
					printw("%c ", bsdindos[dosnum][j]);
			}
			move(FIRSTDISKLINE + i, 55);
			if (lastpartendedat < m->start) {
				printw("Gap: %d  ", m->start - lastpartendedat);
				if (ISDOS(m->system) && m->start == nsectors)
					printw("[OK]");
			}
			if (lastpartendedat > m->start)
				printw("Overlaps by: %d  ", lastpartendedat - m->start);
			lastpartendedat = m->start + m->size;
		}
		clearline(FIRSTDISKLINE + MB_MAXPARTS);
		mvprintw(FIRSTDISKLINE + MB_MAXPARTS, 0, "Last data sector:%7d",
		    disksize - 1);
		move(FIRSTDISKLINE + MB_MAXPARTS, 55);
		if (lastpartendedat < disksize)
			printw("Gap at end: %d  ", disksize - lastpartendedat);
		if (lastpartendedat > disksize)
			printw("Past end: %d  ", lastpartendedat - disksize);
		clearline(UNASSLINE);
		mvprintw(UNASSLINE, 0, "Unassigned BSD partitions:  ");
		for (i = 0; i < MAXPARTITIONS; i++)
			if (unusedbsdpart[i] && i != 'c' - 'a') {
				bsd_parts->d_partitions[i].p_fstype = FS_UNUSED;
				printw("%c ", i + 'a');
			}

		clearline(COMMANDLINE);
		mvprintw(COMMANDLINE, 0, "Command> ");
		refresh();
		c = getch();
		printw("%c", c);
		clearline(WARNLINE);
		switch (c) {
			char partition;
			char dospart;
			int newtype;

		case '\014':
			clearok(stdscr, 1);
			continue;

		case 'S':	/* Start */
		case 's':
			mvprintw(COMMANDLINE, 13,
			    "Change start of partition:  ");
			GETORIGNORE(1, line);
			partition = line[0];
			dosnum = partition - '1';
			WARNIF(partition < '1' || partition > '4',
			    "Must choose partition 1-4");
			WARNIF(dos_parts[dosnum].size == 0,
			    "Must assign length before assigning start");
			warn("num's can be absolute, relative (+ or -) or suffixed with m (MB) or c (cyl)");
			mvprintw(COMMANDLINE, 44, "New start:  ");
			GETORIGNORE(SIZELEN, line);
			CONFIRMOK(dosnum);
			dos_parts[dosnum].start =
			    atosectors(line, dos_parts[dosnum].start);
			adjstart(&dos_parts[dosnum]);
			everchanged = 1;
			continue;

		case 'T':	/* TYPE */
		case 't':
			mvprintw(COMMANDLINE, 13,
			    "Change type of part.:  ");
			GETORIGNORE(1, line);
			partition = line[0];
			WARNIF(partition < '1' || partition > '4',
			    "Must choose partition 1-4");
			dosnum = partition - '1';
			WARNIF(dos_parts[dosnum].size == 0,
			    "Must assign length before assigning type");
			mvprintw(COMMANDLINE, 44, "New type [bsd, msdos, hex]:  ");
			GETORIGNORE(TYPELEN, line);
			tolc(line);
			CONFIRMOK(dosnum);
			if (strcmp(line, "bsd") == 0 || strcmp(line, "bsdi") == 0) {
				dos_parts[dosnum].system = MBS_BSDI;
				adjstart(&dos_parts[dosnum]);
				everchanged = 1;
				continue;
			}
			if (strcmp(line, "dos") == 0 || strcmp(line, "msdos") == 0) {
				WARNIF(nbsdindos[dosnum] > 1,
				    "Can't change to MSDOS until only one or fewer BSDI partitions assigned");
				setdostype(&dos_parts[dosnum]);
				adjstart(&dos_parts[dosnum]);
				everchanged = 1;
				continue;
			}
			sscanf(line, "%x", &newtype);
			WARNIF(nbsdindos[dosnum] > 1 && newtype != MBS_BSDI,
			    "Can't change type until only one or fewer BSDI partitions assigned");
			dos_parts[dosnum].system = newtype;
			adjstart(&dos_parts[dosnum]);
			everchanged = 1;
			continue;

		case 'L':	/* length */
		case 'l':
			mvprintw(COMMANDLINE, 13, "Change length of part.:  ");
			GETORIGNORE(1, line);
			partition = line[0];
			WARNIF(partition < '1' || partition > '4',
			    "Must choose partition 1-4");
			dosnum = partition - '1';
			warn("num's can be absolute, relative (+ or -) or suffixed with m (MB) or c (cyl)");
			mvprintw(COMMANDLINE, 41, "Length (0->unused):  ");
			GETORIGNORE(SIZELEN, line);
			CONFIRMOK(dosnum);
			dos_parts[dosnum].size = atosectors(line, dos_parts[dosnum].size);
			if (ISDOS(dos_parts[dosnum].system))
				setdostype(&dos_parts[dosnum]);
			if (dos_parts[dosnum].size == 0) {
				dos_parts[dosnum].start = 0;
				dos_parts[dosnum].system = MBS_UNKNOWN;
				dos_parts[dosnum].active = MBA_NOTACTIVE;

				/* clean out assigned partitions */
				for (i=0; i<nbsdindos[dosnum]; i++) {
					bsdpart = bsdindos[dosnum][i];
					bsdindos[dosnum][i] = '\0';
					unusedbsdpart[bsdpart - 'a'] = 1;
				}
				nbsdindos[dosnum] = 0;
			}
			everchanged = 1;
			continue;

		case 'A':	/* assign BSD partition */
		case 'a':
			mvprintw(COMMANDLINE, 13, "Assign BSD part.:  ");
			GETORIGNORE(1, line);
			bsdpart = line[0];
			WARNIF(bsdpart < 'a' || bsdpart > 'h',
			    "Must choose partition a-h");
			WARNIF(bsdpart == 'c', "Can't assign BSD partition c");
			mvprintw(COMMANDLINE, 41, "To FDISK part. (0=idle):  ");
			GETORIGNORE(1, line);
			partition = line[0];
			dosnum = partition - '1';
			WARNIF(partition < '0' || partition > '4',
			    "Must choose partition 0-4");
			if (partition > '0') {
				WARNIF(dos_parts[partition - '1'].size == 0,
		"Must assign length before assigning BSD partition");
				WARNIF(nbsdindos[dosnum] > 0 &&
			    		dos_parts[dosnum].system != MBS_BSDI,
		"No more than one BSD partition per non-BSDI FDISK partition");
			}

			/* find old place */
			for (i = 0; i < MB_MAXPARTS; i++) {
				for (j = 0; bsdindos[i][j]; j++) {
					if (bsdpart == bsdindos[i][j]) {
						for (; bsdindos[i][j]; j++)
							bsdindos[i][j] = bsdindos[i][j + 1];
						bsdindos[i][j] = '\0';
						nbsdindos[i]--;
						goto doneremoving;
					}
				}
			}
	doneremoving:	;
			if (partition == '0') {
				unusedbsdpart[bsdpart - 'a'] = 1;
				continue;
			}
			/* assign new place */
			unusedbsdpart[bsdpart - 'a'] = 0;
			bsdindos[dosnum][nbsdindos[dosnum]++] = bsdpart;
			bsdindos[dosnum][nbsdindos[dosnum]] = '\0';
			everchanged = 1;
			continue;

		case 'N':
			{
				struct mbpart *m;
				struct partition *s;

				/* fill in BSD part data for non-bsd FDISK
				   entries */
				for (i = 0; i < MB_MAXPARTS; i++) {
					m = &dos_parts[i];
					if (m->system == MBS_BSDI || m->system == MBS_UNKNOWN ||
					    nbsdindos[i] == 0)
						continue;
					s = &bsd_parts->d_partitions[bsdindos[i][0] - 'a'];
					bzero(s, sizeof(struct partition));
					s->p_offset = m->start;
					s->p_size = m->size;
					if (ISDOS(m->system))
						s->p_fstype = FS_MSDOS;
					else
						s->p_fstype = FS_OTHER;
				}

				/* don't exit if he doesn't have a valid
				   BSDI partition */
				for (i = 0; i < MB_MAXPARTS; i++)
					if (dos_parts[i].system == MBS_BSDI)
						break;
				WARNIF(i == MB_MAXPARTS, "No BSDI FDISK partition");
				for (i = 0; i < MB_MAXPARTS; i++)
					if (dos_parts[i].system == MBS_BSDI && nbsdindos[i] <= 0)
						break;
				WARNIF2(i < MB_MAXPARTS,
				    "No BSD partition letters assigned to BSDI partition %d", i + 1);

				/* don't exit if he has a partition with
				   unknown type */
				for (i = 0; i < MB_MAXPARTS; i++)
					if (dos_parts[i].size > 0 &&
					    dos_parts[i].system == MBS_UNKNOWN)
						break;
				WARNIF2(i < MB_MAXPARTS,
				    "FDISK partition %d has unknown type", i + 1);

				/* don't exit if any partitions extend past
				   end of disk */
				for (i = 0; i < MB_MAXPARTS; i++)
					if (dos_parts[i].start + dos_parts[i].size > disksize)
						break;
				WARNIF2(i < MB_MAXPARTS,
				    "FDISK partition %d extends past end of disk", i + 1);

				/* don't exit if there are overlapping
				   partitions */
				sorttheparts(dos_parts, bsd_parts, sortparts);
				for (i = 0; i < MB_MAXPARTS - 1; i++)
					if (sortparts[i]->size != 0 && sortparts[i + 1]->size != 0
					    && (sortparts[i]->start + sortparts[i]->size) >
					    sortparts[i + 1]->start)
						break;
				WARNIF(i < MB_MAXPARTS - 1, "FDISK partitions overlap");
			}
			return 2;

		case 'X':		/* abort disksetup? */
			interrupt();
			continue;
		default:
			warn("Invalid command");
			printf("");
			fflush(stdout);
		}
	}
}

/*
 * Implement the second screen for the multi-OS case
 * (bsd partition sizing within FDISK partitions)
 */
phase2(dos_parts, bsd_parts)
	struct mbpart dos_parts[MB_MAXPARTS];
	struct disklabel *bsd_parts;
{
	int i, j;
	int dosnum;
	char bsdpart;
	struct mbpart *m;
	int okbsdparts[MAXPARTITIONS];	/* only BSD partitions marked here */
	int shortfall[MB_MAXPARTS];

	warnline = WARNLINE2;
	bzero(okbsdparts, MAXPARTITIONS * sizeof(int));

	erase();
	mvprintw(0, 27, "BSD Partition Sizing");
	mvprintw(1, 0, "FDISK");
	mvprintw(2, 0, "Part# Start  Length BSD Type    Length   MB  Warnings");
	mvprintw(11, 0, "Commands:  [a-h] Set BSD partition length   [N]ext phase");
	mvprintw(12, 0, "           [A-H] Set BSD partition type     [P]revious page (sizing)");

	mvprintw(17, 0, "Directions:");
	mvprintw(18, 0, "   Use `a' thru `h' to set the BSD partition lengths");
	mvprintw(19, 0, "   Use `A' thru `H' to set the BSD partition types");
	mvprintw(20, 0, "   Use `N' to move to the next phase: Installing boot blocks");
	mvprintw(21, 0, "   Use `P' for previous page  (FDISK sizing and BSD partition assignment)");
	mvprintw(22, 0, "   Use `X' to abort disksetup");

/**** The grand phase 2 input loop ***/
	for (;;) {
		char c;
		char line[LINELEN];
		int y, x;
		int total;
		int nlines;

		sorttheparts(dos_parts, bsd_parts, sortparts);	/* order the FDISK
								   partitions */
		nlines = 0;
		for (i = 0; i < MB_MAXPARTS; i++) {
			int partnum;

			m = sortparts[i];
			dosnum = m - &dos_parts[0];

			if (m->system != MBS_BSDI)
				continue;

			clearline(FIRSTBSDLINE + nlines);
			mvprintw(FIRSTDISKLINE + nlines, 0, "%3d%8d%8d",
			    m - &dos_parts[0] + 1, m->start, m->size);
			total = 0;
			for (j = 0; bsdindos[dosnum][j]; j++) {
				move(FIRSTDISKLINE + nlines, 21);
				clrtoeol();
				partnum = bsdindos[dosnum][j] - 'a';
				okbsdparts[partnum] = 1;
				printw("%c  %-7.7s%7d%5d  ", bsdindos[dosnum][j],
				    disktypetostring(bsd_parts->d_partitions[partnum].p_fstype),
				    bsd_parts->d_partitions[partnum].p_size,
				    bsd_parts->d_partitions[partnum].p_size * physsecsize / 1024 / 1024);
				total += bsd_parts->d_partitions[partnum].p_size;
				nlines++;
			}
			shortfall[dosnum] = m->size - total;
			if (total < m->size)
				printw("BSD partitions %d too short", m->size - total);
			if (total > m->size)
				printw("BSD partitions %d too long", total - m->size);
		}

		clearline(COMMANDLINE2);
		mvprintw(COMMANDLINE2, 0, "Command> ");
		refresh();
		c = getch();
		printw("%c", c);
		clearline(WARNLINE2);
		if (c == '\014') {
			clearok(stdscr, 1);
			continue;
		}
		if (c == 'P')
			return 1;
		if (c == 'X') {
			interrupt();
			continue;
		}
		if (c == 'N') {
			for (i = 0; i < MB_MAXPARTS; i++) {
				int offset = dos_parts[i].start;

				if (dos_parts[i].system != MBS_BSDI)
					continue;
				for (j = 0; bsdindos[i][j]; j++) {
					struct partition *s;

					s = &bsd_parts->d_partitions[bsdindos[i][j] - 'a'];
					s->p_offset = offset;
					offset += s->p_size;

					/* check for running off the end */
					if (offset > dos_parts[i].start + dos_parts[i].size) {
						warn("BSD partitions extend past end of FDISK partition %d",
						    i + 1);
						goto phase2_error;
					}
				}
			}
			return 3;
	phase2_error:	;
			continue;
		}
		if (c >= 'a' && c <= 'h') {	/* new length */
			unsigned long *s;	/* pointer to size */
			int defaultans;

			WARNIF(c == 'c', "Can't change partition 'c'");
			WARNIF(okbsdparts[c - 'a'] == 0, "Not a BSD partition");
			s = &bsd_parts->d_partitions[c - 'a'].p_size;
			for (i = 0; i < MB_MAXPARTS; i++)
				for (j = 0; bsdindos[i][j]; j++)
					if (bsdindos[i][j] == c)
						goto foundbsdpartition;
	foundbsdpartition:;
			defaultans = shortfall[i] + bsd_parts->d_partitions[c - 'a'].p_size;
			if (defaultans > 0) {
				warn("num's can be absolute, relative (+ or -) or suffixed with m (MB) or c (cyl)");
				mvprintw(COMMANDLINE2, 20, "New length for partition %c [%d]: ", c, defaultans);
				checkget(7, line);
				clearline(warnline);
				if (line[0] == '\0')
					sprintf(line, "%d", defaultans);
			}
			else {
				warn("num's can be absolute, relative (+ or -) or suffixed with m (MB) or c (cyl)");
				mvprintw(COMMANDLINE2, 20, "New length for partition %c: ", c);
				GETORIGNORE(7, line);
			}
			*s = atosectors(line, *s);
			continue;
		}
		if (c >= 'A' && c <= 'H') {	/* new type */
			c = c - 'A' + 'a';
			WARNIF(c == 'c', "Can't change partition 'c'");
			WARNIF(okbsdparts[c - 'a'] == 0, "Not a BSD partition");
			mvprintw(COMMANDLINE2, 20, "New type for partition %c -- a=4.2  b=swap  ", c);
			GETORIGNORE(1, line);
			if (line[0] == 'a') {
				bsd_parts->d_partitions[c - 'a'].p_fstype = FS_BSDFFS;
				continue;
			}
			if (line[0] == 'b') {
				bsd_parts->d_partitions[c - 'a'].p_fstype = FS_SWAP;
				continue;
			}
			WARNIF(1, "Must choose a or b");
		}
		warn("Invalid command");
		printf("");
		fflush(stdout);
	}
}

/*
 * Convert a filesystem type to a string value
 */
char *
disktypetostring(disktype)
{
	switch (disktype) {
		case FS_UNUSED:
		return "Unused";
	case FS_SWAP:
		return "swap";
	case FS_MSDOS:
		return "DOS";
	case FS_OTHER:
		return "Other";
	case FS_BSDFFS:
		return "4.2";
	default:
		return "Unknown";
	}
}

/*
 * Convert an FDISK `system' type to a string value
 */
char *
systemtostring(sys)
{
	switch (sys) {
		case 0:
		return "????";
	case MBS_BSDI:
		return "BSDI";
	default:
		if (MBS_ISDOS(sys))
			return "DOS";
		else
			return "OTHER";
	}
}

clearline(line)
{
	move(line, 0);
	clrtoeol();
}

/*
 * Create an array of pointers to FDISK partition entries
 * which is sorted primarily by starting offset and 
 * by the partition number as a secondary sort key.
 */
sorttheparts(dos_parts, bsd_parts, sortparts)
	struct mbpart dos_parts[4];
	struct disklabel *bsd_parts;
	struct mbpart *sortparts[4];
{
	int i, j, end;
	struct mbpart *temppart;

	/* sort the FDISK partitions by starting sector: */
	for (i = 0; i < MB_MAXPARTS; i++)
		sortparts[i] = &dos_parts[i];
	/* unused partitions must go to the end */
	for (end = i = MB_MAXPARTS - 1; i >= 0; i--) {
		if (sortparts[i]->size == 0) {
			temppart = sortparts[i];
			sortparts[i] = sortparts[end];
			sortparts[end--] = temppart;
		}
	}
	/* sort non zero items */
	for (i = 0; i < MB_MAXPARTS - 1; i++) {
		if (sortparts[i]->size == 0)
			continue;
		for (j = i + 1; j < MB_MAXPARTS; j++) {
			if (sortparts[j]->size == 0)
				continue;
			if (sortparts[i]->start > sortparts[j]->start) {
				temppart = sortparts[i];
				sortparts[i] = sortparts[j];
				sortparts[j] = temppart;
			}
		}
	}
	/* sort zero items */
	for (i = 0; i < MB_MAXPARTS - 1; i++) {
		if (sortparts[i]->size != 0)
			continue;
		for (j = i + 1; j < MB_MAXPARTS; j++) {
			if (sortparts[j]->size != 0)
				continue;
			if (sortparts[i] - &dos_parts[0] >
			    sortparts[j] - &dos_parts[0]) {
				temppart = sortparts[i];
				sortparts[i] = sortparts[j];
				sortparts[j] = temppart;
			}
		}
	}
}


/*
 * Implement the screen oriented interface for the case
 * when BSD does not share the disk with other operating 
 * systems.
 */
interactive2(bsd_parts)
	struct disklabel *bsd_parts;
{
	int i, j;

	warnline = COMMANDLINE + 1;
	disksize = bsd_parts->d_secperunit;
	if (bsd_parts->d_type == DTYPE_ST506 || bsd_parts->d_type == DTYPE_ESDI)
		disksize = (disksize - bsd_parts->d_nsectors - 126)
		    / bsd_parts->d_nsectors * bsd_parts->d_nsectors;


	disksize = bsd_parts->d_secperunit;
	if (bsd_parts->d_type == DTYPE_ST506 || bsd_parts->d_type == DTYPE_ESDI)
		disksize = (disksize - bsd_parts->d_nsectors - 126)
		    / bsd_parts->d_nsectors * bsd_parts->d_nsectors;

	physsecsize = bsd_parts->d_secsize;
	secpercyl = bsd_parts->d_secpercyl;
	nsectors = bsd_parts->d_nsectors;

	erase();
	mvprintw(0, 23, "BSD Partition Sizing");
	mvprintw(1, 0, "BSD Type      Size    MB     Start     End  Warnings");
	mvprintw(12, 0, "Commands:  [T]ype  [S]tart  [L]ength  [N]ext phase");


#define HELPLINE 17
	mvprintw(HELPLINE + 0, 0, "Directions:");
	mvprintw(HELPLINE + 1, 3, "Use `L' and `S' to set the BSD partition Lengths and Starting sectors");
	mvprintw(HELPLINE + 2, 3, "Use `T' to set the BSD partition Types");
	mvprintw(HELPLINE + 3, 3, "Use `N' to move to the next phase:  Installing boot blocks");
	mvprintw(HELPLINE + 4, 3, "Use `X' to abort disksetup");
	mvprintw(HELPLINE + 5, 0, "Numbers can be relative (with + or -) or suffixed with c (cylinders),");
	mvprintw(HELPLINE + 6, 0, "     m (megabytes), or M (megabytes rounded to cylinders)");


	/* the interactive2 input loop */
	for (;;) {
		char c;
		char line[LINELEN];
		int y, x;
		int lines;
		int linenum;
		int i, j;
		int end;
		int start;

		for (i = 2; i < 10; i++)
			clearline(i);

#define LISTSTART 2
		linenum = LISTSTART;


/* print the BSD partitions: */
		for (i = 0; i < MAXPARTITIONS; i++) {
			int start1, size1;
			int seen;	/* controls initial warning
					   printing */
			struct partition *part = &bsd_parts->d_partitions[i];

			if (part->p_size == 0) {
				mvprintw(linenum++, 0, " %c   <unused>", i + 'a');
				continue;
			}
			mvprintw(linenum, 0, " %c  %-6.6s%8d (%4d) %8d%8d ",
			    i + 'a', disktypetostring(part->p_fstype),
			    part->p_size, part->p_size * physsecsize / 1024 / 1024,
			    part->p_offset, part->p_size + part->p_offset - 1);

/* show conflicts: */
			if (i != 'c' - 'a') {
				start1 = part->p_offset;
				size1 = part->p_size;
				if (start1 + size1 > bsd_parts->d_secperunit)
					printw("Past Disk End!  ");
				seen = 0;
				for (j = 0; j < MAXPARTITIONS; j++) {
					int start2 = bsd_parts->d_partitions[j].p_offset;
					int size2 = bsd_parts->d_partitions[j].p_size;

					if (size2 == 0 || j == 'c' - 'a' || j == i)
						continue;
					if (start2 >= start1 + size1 || start2 + size2 <= start1)
						continue;
					if (seen == 0) {
						printw("Overlaps: ");
						seen = 1;
					}
					printw("%c ", j + 'a');
				}
			}
			linenum++;
		}
		mvprintw(linenum++, 16, "Last Data Sector:%9d", disksize - 1);

		clearline(COMMANDLINE);
		mvprintw(COMMANDLINE, 0, "Command> ");
		refresh();
		c = getch();
		printw("%c", c);
		clearline(WARNLINE);
		switch (c) {
			char partition;
			unsigned long *s;
			int newtype;

		case '\014':
			clearok(stdscr, 1);
			continue;

		case 'S':	/* new start addr. */
		case 's':
			mvprintw(COMMANDLINE, 15, "New start for part.:  ");
			GETORIGNORE(1, line);
			partition = tolower(line[0]);
			WARNIF(partition < 'a' || partition > 'h', "Must specify a-h");
			WARNIF(line[0] == 'c', "Can't change size of partition c");
			WARNIF(bsd_parts->d_partitions[partition - 'a'].p_size <= 0,
			    "Can't change start of unused partition; set size first");
			mvprintw(COMMANDLINE, 44, "New start:  ");
			GETORIGNORE(SIZELEN, line);
			s = &bsd_parts->d_partitions[partition - 'a'].p_offset;
			*s = atosectors(line, *s);
			everchanged = 1;
			continue;

		case 'L':	/* SIZE */
		case 'l':	/* SIZE */
			mvprintw(COMMANDLINE, 21, "New size for part.:  ");
			GETORIGNORE(1, line);
			partition = tolower(line[0]);
			WARNIF(partition < 'a' || partition > 'h', "Must specify a-h");
			WARNIF(line[0] == 'c', "Can't change size of partition c");
			mvprintw(COMMANDLINE, 44, "New size (0->unused):  ");
			GETORIGNORE(SIZELEN, line);
			s = &bsd_parts->d_partitions[partition - 'a'].p_size;
			*s = atosectors(line, *s);
			everchanged = 1;
			continue;
		case 'T':	/* TYPE */
		case 't':	/* TYPE */
			mvprintw(COMMANDLINE, 12, "Which part.:  ");
			GETORIGNORE(1, line);
			partition = tolower(line[0]);
			WARNIF(partition < 'a' || partition > 'h', "Must specify a-h");
			WARNIF(partition == 'c' && (bsd_parts->d_type == DTYPE_ESDI ||
				bsd_parts->d_type == DTYPE_ST506),
			    "Can't use the `c' partition on ESDI or ST506 type disks");
			WARNIF(bsd_parts->d_partitions[partition - 'a'].p_size <= 0,
			    "Can't change type of unused partition; assign a size");
			mvprintw(COMMANDLINE, 29,
			    "New type [a=swap b=4.2 c=dos d=other e=unused]: ");
			GETORIGNORE(1, line);
			WARNIF2(line[0] < 'a' || line[0] > 'e',
			    "Invalid type type `%c'", line[0]);
			switch (line[0]) {
			case 'a':
				newtype = FS_SWAP;
				break;
			case 'b':
				newtype = FS_BSDFFS;
				break;
			case 'c':
				newtype = FS_MSDOS;
				break;
			case 'd':
				newtype = FS_OTHER;
				break;
			case 'e':
				newtype = FS_UNUSED;
			}
			everchanged = 1;
			bsd_parts->d_partitions[partition - 'a'].p_fstype = newtype;
			continue;

		case 'N':
			for (i = 0; i < MAXPARTITIONS; i++)
				if ((bsd_parts->d_partitions[i].p_offset +
					bsd_parts->d_partitions[i].p_size) > bsd_parts->d_secperunit)
					break;
			WARNIF(i < MAXPARTITIONS, "Partitions extend past end of unit");
			goto done;
		case 'X':
			interrupt();
			continue;
		default:
			if (c != '\n' && c != '\0')
				warn("Invalid command `%c'", c);
			continue;
		}
	}
done:
	cleanup();
}

tolc(p)
	char *p;
{
	for (; *p; p++)
		*p = tolower(*p);
}

/*
 * Set the type of an MSDOS partition to the correct 
 * value depending on the current size of the partition.
 */
setdostype(m)
	struct mbpart *m;
{
	if (m->size < 16 * 1024 * 1024 / physsecsize) {
		if (m->system != MBS_DOS12) {
			m->system = MBS_DOS12;
			warn("Setting type to MBS_DOS12 (FAT12)");
		}
		return;
	}
	if (m->size < 32 * 1024 * 1024 / physsecsize) {
		if (m->system != MBS_DOS16) {
			m->system = MBS_DOS16;
			warn("Setting type to MBS_DOS16 (FAT16)");
		}
		return;
	}
	if (m->system != MBS_DOS4) {
		m->system = MBS_DOS4;
		warn("Setting type to MBS_DOS4 (FAT16 with clusters)");
	}
	return;
}

removefrombsdindos(partition)
	char partition;
{
	int i, j;

	for (i = 0; i < MB_MAXPARTS; i++) {
		for (j = 0; bsdindos[j][i]; j++) {
			if (bsdindos[j][i] == partition) {
				for (; bsdindos[j][i]; j++)
					bsdindos[j][i] = bsdindos[j + 1][i];
				return;
			}
		}
	}
}

/*
 * Adjust the start of an FDISK partition as necessary depending
 * on the type of the partition.
 *
 * DOS partitions must start on track boundaries and cannot start at 
 * sector 0.
 *
 * BSDI partitions must be cylinder aligned.
 */
adjstart(m)
	struct mbpart *m;
{
	int start = m->start;

	if (m->system == MBS_BSDI) {
		if ((start % secpercyl) != 0) {
			m->start = (start + secpercyl - 1) / secpercyl * secpercyl;
			warn("Adjusted BSDI first sector to cylinder boundary");
		}
		return;
	}
	if (ISDOS(m->system)) {
		if (start % nsectors != 0) {
			m->start = (start + nsectors - 1) / nsectors * nsectors;
			warn("Adjusted DOS start sector to even track");
		}
		else if (start == 0) {
			m->start = nsectors;
			warn("Adjusted DOS start sector past 0");
		}
	}
}

/*
 * Process an input string to modify a numeric sector value.  Allow
 * relative values (+/-) and misc. suffixes.
 */
atosectors(line, n)
	char *line;
{
	char *p;
	int result;

	for (p = line; *p; p++);
	p--;			/* p now points to size indicator like M */
	switch (*p) {
	case 'M':
		result = (atoi(line) * 1024 * 1024 / physsecsize +
				secpercyl - 1) / secpercyl * secpercyl;
		break;
	case 'm':
		result = atoi(line) * 1024 * 1024 / physsecsize;
		break;
	case 'c':
		result = atoi(line) * secpercyl;
		break;
	default:
		result = atoi(line);
		break;
	}

	if (line[0] == '-' || line[0] == '+')
		return n + result;
	else
		return result;
}
