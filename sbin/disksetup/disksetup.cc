/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI disksetup.cc,v 2.1 1995/02/03 07:14:38 polk Exp	*/

#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include "fs.h"
#include <paths.h>
#include <sys/wait.h>
#include "showhelp.h"
#include "screen.h"
#include "help.h"
#include "disk.h"
#include "field.h"
#include "util.h"

Disk disk;

#define	_PATH_BOOTANY	"/sbin/bootany"

char *path_bootany = _PATH_BOOTANY;

char *master = 0;
char *primary = 0;
char *secondary = 0;

char boot0[MAXPATHLEN];
char boot1[MAXPATHLEN];
char boot2[MAXPATHLEN];

void dumptab(char *);


O::Usage(int status)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr,
"    %s disk                                      # Read label\n",
			prog);
    fprintf(stderr,
"    %s [-NW] disk                                # Write Disable/Enable\n",
			prog);
    fprintf(stderr,
"    %s -e [disk [xxboot bootxx [mboot]]]         # Edit disk label\n",
			prog);
    fprintf(stderr,
"    %s -i [disk]                                 # Interactive mode\n",
			prog);
    fprintf(stderr,
"    %s -R disk protofile [xxboot bootxx [mboot]] # Restore Label\n",
			prog);
    fprintf(stderr,
"    %s -B disk [ xxboot bootxx [mboot]]          # Install boot records\n",
			prog);
#if 0
    fprintf(stderr,
"    %s [-m mboot] [-b xxboot bootxx] disk        # Install boot records\n",
			prog);
#endif
    fprintf(stderr, "Additional flags:\n");
    fprintf(stderr, "    -A        Path to the bootany program\n");
    fprintf(stderr, "    -K        Only read the Kernels (internal) label\n");
    fprintf(stderr, "    -D        Only read the On Disk label\n");
    fprintf(stderr, "    -s        Only read/write incore BSD labels\n");
    fprintf(stderr, "    -n        No writes to disk (open read only and non-blocking)\n");
    fprintf(stderr, "    -I        Ignore current BSD label\n");
    fprintf(stderr, "    -I -I     Ignore current BSD label and FDISK table\n");
    fprintf(stderr, "    -F        Edit FDISK partition (implies -i)\n");
    fprintf(stderr, "    -P        Edit BSD partition (implies -i)\n");
    fprintf(stderr, "    -E        Expert mode.  Allows broken tables, etc.\n");
    fprintf(stderr, "    -M mem    Pretend physical memory is mem MB\n");
#if 0
    fprintf(stderr, "    -f        Force the installing of boot blocks (not implemented yet)\n");
    fprintf(stderr, "    -t path   Dump file system names to {path}disk\n");
    fprintf(stderr, "    -x file   Make copes of tables in file\n");
#endif

    exit(status);
}

main(int ac, char **av)
{
    int i, j;
    int e;
    char *dn;
    int c;

    char *proto = 0;
    char *output = 0;
    char *tabpath = 0;

    int mfd = -1;
    int pfd = -1;
    int sfd = -1;

    if (dn = strrchr(av[0], '/'))
	O::Program(++dn);
    else
	O::Program(av[0]);

    while ((c = getopt(ac, av, "b:efikm:np:q:st:x:A:BDEFIKM:NPRW?")) != EOF) {
	switch (c) {
	case 'A':
	    path_bootany = optarg;
	    break;
    	case 'M':
	    PhysMem(atoi(optarg) * 1024 * 1024);
	    break;
    	case 'm':	// install master boot record
	    O::Set(O::INSTALL);
	    O::Set(O::IgnoreGeometry);
    	    if (master)
		goto usage;
	    master = optarg;
	    break;
    	case 'p':	// install primary boot strap
	    O::Set(O::INSTALL);
	    O::Set(O::IgnoreGeometry);
	    O::Set(O::DontUpdateLabel);
    	    if (primary)
		goto usage;
	    primary = optarg;
	    break;
    	case 'q':	// install secondary boot strap
	    O::Set(O::INSTALL);
	    O::Set(O::IgnoreGeometry);
	    O::Set(O::DontUpdateLabel);
    	    if (secondary)
		goto usage;
	    secondary = optarg;
	    break;
    	case 'b':	// install boot straps
	    O::Set(O::INSTALL);
	    O::Set(O::IgnoreGeometry);
	    O::Set(O::DontUpdateLabel);
	    O::Set(O::DontUpdateLabel);
    	    if (ac < ++optind)
		goto usage;
    	    if (primary || secondary)
	    	goto usage;
	    primary = optarg;
	    secondary = av[optind-1];
    	    break;
	case 'e':	// Edit
	    O::Set(O::EDIT);
	    O::Set(O::IgnoreGeometry);
	    break;
	case 'F':	// Update FDISK Table
	    O::Set(O::INTERACTIVE);
	    O::Set(O::UpdateFDISK);
	    break;
	case 'P':	// Update BSD Partition Table
	    O::Set(O::INTERACTIVE);
	    O::Set(O::UpdateBSD);
	    break;
	case 'f':	// Force Boot
	    O::Set(O::ForceBoot);
	    break;
	case 's':	// Only save label in core
	    O::Set(O::InCoreOnly);
	    break;
	case 'i':	// Interactive
	    O::Set(O::INTERACTIVE);
	    break;
	case 'n':	// Nowrite
	    O::Set(O::NoWrite);
	    O::Set(O::NonBlock);
	    break;
	case 'B':	// Bootblocks
	    O::Set(O::BOOTBLKS);
	    O::Set(O::ForceBoot);
	    O::Set(O::IgnoreGeometry);
	    O::Set(O::DontUpdateLabel);
    	    if (primary || secondary || master)
	    	goto usage;
	    break;
	case 'E':	// Expert Mode
	    O::Set(O::Expert);
	    break;
	case 'I':	// Ignore label (two -> ignore fdisk label)
	    O::Set(O::Ignore);
	    O::Set(O::NonBlock);
	    break;
	case 'N':	// Make disk label not writeable
	    O::Set(O::W_DISABLE);
	    O::Set(O::NonBlock);
	    O::Set(O::Ignore);
	    O::Set(O::Ignore);
	    O::Set(O::IgnoreGeometry);
	    break;
	case 'R':	// Restore from ascii version
	    O::Set(O::RESTORE);
	    O::Set(O::NonBlock);
	    break;
	case 'W':	// Make disk label writeable
	    O::Set(O::W_ENABLE);
	    O::Set(O::NonBlock);
	    O::Set(O::Ignore);
	    O::Set(O::Ignore);
	    O::Set(O::IgnoreGeometry);
	    break;
	case 'D':	// Only read kernel label
	    O::Set(O::ReadDisk);
	    break;
	case 'K':	// Only read kernel label
	    O::Set(O::ReadKernel);
	    break;
	case 'x':	// Undocumented -- make copies of tables in savename
	    output = optarg;
	    break;
	case 't':	// Undocumented -- output tables for EZconfig
	    tabpath = optarg;
	    break;
    	usage:
	default:
	    O::Usage();
	    break;
	}
    }

    if (O::Flag(O::ReadDisk) && O::Flag(O::ReadKernel)) {
	fprintf(stderr, "Only one of -K and -D may be specified\n");
	exit(1);
    }

    if (O::Cmd() == O::UNSPEC) {
//	if (optind < ac) {
	    O::Set(O::READ);
	    O::Set(O::NoWrite);
//    	} else {
//	    O::Set(O::INTERACTIVE);
//    	}
    }

    switch (O::Cmd()) {
    case O::W_ENABLE:
    case O::W_DISABLE:
    case O::READ:
    case O::INSTALL:
	if (optind != ac - 1)
	    O::Usage();
	break;
    case O::INTERACTIVE:
	if (optind < ac - 1)
	    O::Usage();
	break;
    case O::RESTORE:
	if (optind < ac - 5 || optind > ac - 2)
	    O::Usage();
	proto = av[optind + 1];
	if (primary = av[optind + 2]) {
	    if (!(secondary = av[optind + 3]))
		O::Usage();
	    master =  av[optind + 4];
    	}
	break;
    case O::EDIT:
    case O::BOOTBLKS:
	if (optind < ac - 4 || optind > ac - 1)
	    O::Usage();
	if (primary = av[optind + 1]) {
	    if (!(secondary = av[optind + 2]))
		O::Usage();
	    master =  av[optind + 3];
    	}
	break;
    }

    if (!(dn = av[optind])) {
	if (O::Cmd() != O::INTERACTIVE && O::Cmd() != O::EDIT)
	    O::Usage(2);
	dn = ChooseDisk();
    }

    if (e = disk.Init(dn)) {
	fprintf(stderr, "initializing %s: %s\n", dn, Errors::String(e));
	exit(1);
    }

    if (O::Flag(O::UpdateFDISK) && !O::Flag(O::UpdateBSD)) {
	O::Set(O::DontUpdateLabel);
    }

    switch (O::Cmd()) {
    case O::W_ENABLE:
    	disk.WriteEnable();
	exit(0);
    case O::W_DISABLE:
    	disk.WriteDisable();
	exit(0);
    case O::READ:
    	if (!disk.label_original.Valid() || disk.label_original.Soft()) {
	    printf("# Missing disk label, default label used.\n");
	    printf("# Geometry may be incorrect.\n");
    	}
    	disk.use_fdisk = disk.has_fdisk;
	disklabel_display(disk.device, stdout, &disk.label_original, disk.PartTable());
	exit(0);
    case O::EDIT:
    	disk.use_fdisk = disk.has_fdisk;
    	edit();
    	break;
    case O::INTERACTIVE:
    	interactive();
	break;
    case O::BOOTBLKS:
    	if (!primary && disk.label_original.d_boot0) {
	    if (*disk.label_original.d_boot0 != '/')
		sprintf(boot0, "%s/%s", _PATH_BOOTSTRAPS,
				disk.label_original.d_boot0);
	    else
		strcpy(boot0, disk.label_original.d_boot0);
	    primary = boot0; 
    	}
        if (!secondary && disk.label_original.d_boot1) {
	    if (*disk.label_original.d_boot1 != '/')
		sprintf(boot1, "%s/%s", _PATH_BOOTSTRAPS,
				disk.label_original.d_boot1);
	    else
		strcpy(boot1, disk.label_original.d_boot1);
	    secondary = boot1;
        }
    	O::Set(O::INSTALL, 1);
    	disk.use_fdisk = disk.has_fdisk;
	disk.label_new = disk.label_original;
	if (!primary && !secondary)
	    getbootblocks();
    	break;
    case O::INSTALL:
    	disk.use_fdisk = disk.has_fdisk;
	disk.label_new = disk.label_original;
    	break;
    case O::RESTORE:
    	disk.use_fdisk = disk.has_fdisk;
    	break;
    default:
    	printf("That command is not implemented yet\n");
	exit(0);
    }

    if (proto) {
	FILE *fp = fopen(proto, "r");

    	if (!fp) {
	    err(1, proto);
    	}
    	if (disklabel_getasciilabel(fp, &disk.label_new, 0) == 0) {
	    errx(1, "%s: bad label prototype\n", proto);
    	}
    	fclose(fp);
    }

    BootBlock MBR;

    if (O::Flag(O::InCoreOnly) && (master || primary || secondary)) {
	print(incore_and_bootblocks_long);
	master = primary = secondary = 0;
    }

    if (master && (mfd = open(master, O_RDONLY)) < 0) {
    	if (master[0] != '/') {
	    sprintf(boot2, "%s/%s", _PATH_BOOTSTRAPS, master);
	    master = boot2;
	}
    	if ((mfd = open(master, O_RDONLY)) < 0) {
	    err(1, "MBR: %s", master);
    	}
    	if (e = MBR.Read(mfd)) {
	    errx(1, "reading MBR %s: %s\n", master, Errors::String(e));
    	}
    	close(mfd);
    } else
	MBR = disk.bootblock;

    if (primary && (pfd = open(primary, O_RDONLY)) < 0) {
    	if (primary[0] != '/') {
	    sprintf(boot0, "%s/%s", _PATH_BOOTSTRAPS, primary);
	    primary = boot0;
	}
    	if ((pfd = open(primary, O_RDONLY)) < 0) {
	    err(1, "primary bootstrap %s", primary);
    	}
    }

    if (secondary && (sfd = open(secondary, O_RDONLY)) < 0) {
    	if (secondary[0] != '/') {
	    sprintf(boot1, "%s/%s", _PATH_BOOTSTRAPS, secondary);
	    secondary = boot1;
	}
    	if ((sfd = open(secondary, O_RDONLY)) < 0) {
	    err(1, "secondary bootstrap %s", secondary);
    	}
    }

    mbpart *t;
    if (disk.use_fdisk && (t = disk.PartTable())) {
	MBR.MergePartitions(t);
    } else if (O::Flag(O::RequireBootany)) {
    	request_inform(main_bootany_no_fdisk);
    }

    //
    // If disk.bsdoff has not been set (either to where the old label
    // was, or to where we want to put the new label) we must go out
    // and calculate the value for ourselves.
    //
    if (disk.bsdoff == -1)
	disk.bsdoff = disk.LabelLocation();

    disk.label_new.ComputeSum(disk.bsdoff);

    disk.bsdoff -= LABELSECTOR * disk.secsize + LABELOFFSET;

    if (tabpath)
	dumptab(tabpath);

    if (O::Flag(O::NoWrite)) {
	if (O::Cmd() == O::INTERACTIVE)
	    disklabel_display(disk.device, stdout, &disk.label_new, disk.PartTable());
	exit(0);
    }

    if (output) {
    	disk.dfd = creat(output, 0666);
    	disk.path = output;
    	if (disk.dfd < 0) {
	    err(1, "save file %s", output);
    	}
    }

    if (master || (disk.use_fdisk && disk.PartTable())) {
    	if (O::Flag(O::RequireBootany) ||
	    memcmp(&disk.bootblock, &MBR, sizeof(BootBlock))) {
	    if (disk.has_fdisk)
		for (i = 1; i <= 4; ++i) {
#define	o	    disk.bootblock
		    if (o.Type(i) && o.Length(i)) {
			for (j = 1; j <= 4; ++j) {
			    if (MBR.Type(j) == o.Type(i) &&
				MBR.Offset(j) == o.Offset(i) &&
				MBR.Length(j) == o.Length(i))
				    break;
			}
			if (j > 4)
			    printf("Current FDISK partition %d of type %s"
				   "(0x%02x) will be destroyed.\n",
				    i, PTypeFind(o.Type(i)).name, o.Type(i));
		    }
#undef	o
		}
	    if (request_yesno(okay_to_write_mbr, 1)) {
		if (lseek(disk.dfd, 0, L_SET) == -1) {
		    warn("%s: seeking to MBR (not written)", disk.path);
		} else {
		    disk.WriteEnable();
		    MBR.Write(disk.dfd);
		    fsync(disk.dfd);
		    if (O::Flag(O::RequireBootany)) {
			char cmdbuf[1024];
			sprintf(cmdbuf, "%s -n -i %s", path_bootany, disk.path);
			system(cmdbuf);
		    }
		    fsync(disk.dfd);
		    disk.WriteDisable();
		}
	    }
    	} else {
	    request_inform(mbr_stays_same);
    	}
    }

    if (O::Flag(O::DontUpdateLabel) && sfd < 0 && pfd < 0) {
	//
	// If we are not going to update the label, the primary,
	// or the secondary bootstraps, might as well exit.
	//
	exit(0);
    }

    unsigned char bootarea[BBSIZE];

    memset(bootarea, 0, sizeof(bootarea));

    //
    // We start assuming we will only write the disk label
    // We increase the size as we figure out more to write.
    //
    int start = LABELSECTOR * disk.secsize + LABELOFFSET;
    int stop = LABELSECTOR * disk.secsize + LABELOFFSET + sizeof(DiskLabel);

    if (sfd >= 0) {
	if (read(sfd, &bootarea[disk.secsize], BBSIZE - disk.secsize) < 0) {
    	    fprintf(stderr, "%s: %s while reading\n", secondary, strerror(errno));
	    exit(1);
    	}
    	stop = BBSIZE;
	close(sfd);
    }

    if (pfd >= 0) {
	int o = (disk.bsdoff == 0 && disk.use_fdisk) ? 15 * disk.secsize : 0;
    	if (o == 0) {
	    start = 0;
    	} else if (sfd < 0) {
	    print(need_secondary_bootstrap);
	    exit(1);
    	}
	if (read(pfd, &bootarea[o], disk.secsize) < 0) {
    	    fprintf(stderr, "%s: %s while reading\n", primary, strerror(errno));
	    exit(1);
    	}
	close(pfd);
    }

    //
    // Okay to check space even if not installing label
    //
    unsigned char *p = bootarea + LABELSECTOR * disk.secsize + LABELOFFSET;
    DiskLabel *dl = (DiskLabel *)p;
    while (p < (unsigned char *)(dl+1)) {
	if (*p++) {
	    print(no_room_for_label);
	    exit(1);
    	}
    }

    if (O::Flag(O::DontUpdateLabel) && !output && !proto
				    && (sfd >= 0 || pfd >= 0)) {
	//
	// Okay, we are not supposed to modify the label.  We just read
	// the sector where the label would go and read it up.
	//
    	DiskLabel label;
    	off_t offset;

    	//
    	// This could be a problem on an unlabled disk...
    	//
	int e = label.Read(disk.dfd, disk.bsdoff + LABELSECTOR * disk.secsize
					         + LABELOFFSET, &offset);
    	if (e) {
	    fprintf(stderr, "reading existing disk label from %s: %s\n", disk.path, Errors::String(e));
	    exit(1);
    	}
    	if (offset != disk.bsdoff + LABELSECTOR * disk.secsize + LABELOFFSET) {
    	    request_inform(use_old_label);
    	}
    	label.ComputeSum(disk.bsdoff + LABELSECTOR*disk.secsize + LABELOFFSET);
    	*dl = label;
    } else if (!O::Flag(O::DontUpdateLabel)) {
	*dl = disk.label_new;
    }

    printf("\n");

    if (disk.label_original.Valid() && !disk.label_original.Soft()) {
	for (i = 0; i < 8; ++i) {
#define	o   disk.label_original.d_partitions[i]
	    if (o.p_size && o.p_fstype != FS_SWAP && o.p_fstype != FS_UNUSED) {
		for (j = 0; j < 8; ++j) {
#define	n	dl->d_partitions[j]
		    if (o.p_size == n.p_size &&
			o.p_offset == n.p_offset &&
			o.p_fstype == n.p_fstype)
			    break;
		}
		if (j >= 8)
		    printf("Current BSD partition %c will be destroyed.\n",
			    i + 'a');
	    } 
#undef n
#undef o
	}
    }


    if (O::Flag(O::InCoreOnly) ||
	request_yesno((sfd < 0 && pfd < 0) ? okay_to_write_dl
					   : okay_to_write_dlbb, 1)) {

	//
	// First write the label in core
	//
    	if (dl->WriteInternal(disk.dfd)) {
	    warn("%s: writing internal label", disk.path);
	}

    	if (!O::Flag(O::InCoreOnly)) {
	    disk.WriteEnable();

    	    int o;
    	    char tbuf[disk.secsize];

    	    if (o = (start % disk.secsize)) {
	    	if (lseek(disk.dfd, disk.bsdoff + start - o, L_SET) == -1 ||
		    read(disk.dfd, tbuf, disk.secsize) != disk.secsize) {

		    warn("%s: merging boot blocks", disk.path);
		    fsync(disk.dfd);
		    exit(0);
    	    	}
		memcpy(bootarea + start - o, tbuf, o);
    	    	start -= o;
    	    }

    	    if (o = (stop % disk.secsize)) {
	    	if (lseek(disk.dfd, disk.bsdoff + stop - o, L_SET) == -1 ||
		    read(disk.dfd, tbuf, disk.secsize) != disk.secsize) {

		    warn("%s: merging boot blocks", disk.path);
		    fsync(disk.dfd);
		    exit(0);
    	    	}
		memcpy(bootarea + stop, tbuf + o, disk.secsize - o);
    	    	stop += disk.secsize - o;
    	    }

	    if (lseek(disk.dfd, disk.bsdoff + start, L_SET) == -1 ||
		write(disk.dfd, bootarea + start, stop-start) != stop-start) {
		warn("%s: writing boot blocks", disk.path);
	    }
	    disk.WriteDisable();
    	}
	fsync(disk.dfd);
    }
    exit(0);
}

void
dumptab(char *tabpath)
{
    FILE *fp;
    int i;
    char tabname[strlen(tabpath) + strlen(disk.device) + 1];
    
    strcpy(tabname, tabpath);
    strcat(tabname, disk.device);

    if ((fp = fopen(tabname, "w")) == NULL)
	return;
    for (i = 0; i < 8 && disk.bsd[i].number; ++i) {
	FileSystem &f = disk.bsd[i];
	if (f.number == 3)
	    continue;
	if (f.mount[0] == '\0')
	    strcat(f.mount, "UNKNOWN");
	fprintf(fp, "%s%c %s %s\n", disk.device, f.number + 'a' - 1, f.mount,
				    FTypeFind(f.type).name);
    }
    fclose(fp);
}
