/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI interactive.cc,v 2.1 1995/02/03 07:15:36 polk Exp	*/

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

void
interactive()
{
    int redobsd = 0;
    int swaplen = 0;
    int swapoff = 0;

    if (!disk.d_type) {
	print(main_ask_type_long);
    	while (!disk.d_type) {
	    char *type = request_string(main_ask_type);
    	    if (!strcasecmp(type, "scsi"))
		disk.d_type = DTYPE_SCSI;
    	    else if (!strcasecmp(type, "ide"))
		disk.d_type = DTYPE_ST506;
    	    else if (!strcasecmp(type, "mfm"))
		disk.d_type = DTYPE_ST506;
    	    else if (!strcasecmp(type, "rll"))
		disk.d_type = DTYPE_ST506;
    	    else if (!strcasecmp(type, "st506"))
		disk.d_type = DTYPE_ST506;
    	    else if (!strcasecmp(type, "esdi"))
		disk.d_type = DTYPE_ESDI;
    	    else if (!strcasecmp(type, "floppy"))
		disk.d_type = DTYPE_FLOPPY;
    	}
    }

    //
    // Phase 1 -- FDISK Partitioning
    //
    // Check to make sure we have a geometry.  If running in expert
    // mode allow altering the geometry even if found.
    // 
    // Do not complete this phase until there is a BSD partition
    // (if we are installing a BSD partition table)
    //

    if (disk.d_type == DTYPE_FLOPPY && O::Flag(O::UpdateFDISK)) {
	print(main_no_fdisk_on_floppy);
	exit(1);
    }

    disk.SwitchToCMOS();

    if (    !(disk.use_fdisk = O::Flag(O::UpdateFDISK))
	 && ! O::Flag(O::UpdateBSD)) {
    	if (disk.d_type != DTYPE_FLOPPY) {
	    print(disk.has_fdisk ? main_multipleos_long_fdisk
				 : main_multipleos_long);
	    disk.use_fdisk = request_yesno(main_multipleos, disk.has_fdisk);
	    if (disk.use_fdisk != disk.has_fdisk)
		redobsd = 1;
    	} else {
	    disk.use_fdisk = disk.has_fdisk = 0;
    	}
    }
    if (!disk.use_fdisk) {
	for (int i = 0; i < 4; ++i)
	    disk.part[i].Zero();
    }

    print(main_possible_geometries);
    if (disk.use_fdisk) {
	printf("%16s     CMOS     BIOS    FDISK   BSD/OS\n", "");
	printf("%16s %8d %8d %8d %8d\n",
	    "Heads", disk.cmos.heads, disk.bios.heads,
		     disk.fdisk.heads, disk.bsdg.heads);
	printf("%16s %8d %8d %8d %8d\n",
	    "Sectors/Track", disk.cmos.sectors, disk.bios.sectors,
			     disk.fdisk.sectors, disk.bsdg.sectors);
	printf("%16s %8d %8d %8d %8d\n",
	    "Cylinders", disk.cmos.cylinders, disk.bios.cylinders,
			 disk.fdisk.cylinders, disk.bsdg.cylinders);
	printf("%16s %8d %8d %8d %8d\n",
	    "Total Sectors", disk.cmos.size, disk.bios.size,
			 disk.fdisk.size, disk.bsdg.size);
	// XXX - check for fdisk == bios
	if (disk.fdisk.Valid()) {
	    disk.dosg = disk.fdisk;
	    print(will_use_fdisk_geometry);
	} else if (disk.bios.Valid()) {
	    disk.dosg = disk.bios;
	    print(will_use_bios_geometry);
	} else if (disk.cmos.Valid()) {
	    disk.dosg = disk.cmos;
	    print(will_use_cmos_geometry);
	}
    } else {
	printf("%16s     CMOS     BIOS   BSD/OS\n", "");
	printf("%16s %8d %8d %8d\n",
	    "Heads", disk.cmos.heads, disk.bios.heads, disk.bsdg.heads);
	printf("%16s %8d %8d %8d\n",
	    "Sectors/Track", disk.cmos.sectors, disk.bios.sectors,
			     disk.bsdg.sectors);
	printf("%16s %8d %8d %8d\n",
	    "Cylinders", disk.cmos.cylinders, disk.bios.cylinders,
			 disk.bsdg.cylinders);
	printf("%16s %8d %8d %8d\n",
	    "Total Sectors", disk.cmos.size, disk.bios.size,
			 disk.bsdg.size);
    }
    printf("\n");
    request_inform(press_return_to_continue);

    if (!O::Flag(O::UpdateFDISK) || O::Flag(O::UpdateBSD)) {
	disk.use_bsd = 1;
	if (!disk.bsdg.Valid() || request_yesno(main_get_bsd_geom, 0)) {
	    int first = 1;
	    for(;;) {
		if (!first)
		    request_inform(invalid_geometry);
		first = 0;
		SetGeometry(disk.use_fdisk ? main_need_bsd_geom_coex_long
					   : main_need_bsd_geom_long,
			    disk.bsdg);
		if (disk.bsdg.Valid())
		    break;
	    }
	}

	disk.SwitchToBSD();

	switch (disk.d_type) {
	case DTYPE_ST506:
	case DTYPE_ESDI:
	    disk.badblock = disk.SecPerTrack() + 126;
	    break;
	}
    }

    if (disk.use_fdisk) {
	int yn = 0;

	if (disk.bios.Valid() && !(disk.bios == disk.dosg))
		yn = 1;
	print(main_get_dos_geom_long);

    	if (!disk.dosg.Valid() && disk.d_type != DTYPE_SCSI && disk.use_bsd) {
	    disk.dosg = disk.bsdg;
	    request_inform(will_use_bsd_geometry);
	    yn = 1;
	}

	if (!disk.dosg.Valid() || request_yesno(main_get_dos_geom, yn)) {
	    int first = 1;
	    for (;;) {
		if (!first)
		    request_inform(invalid_geometry);
		first = 0;
		SetGeometry(main_need_dos_geom_long, disk.dosg);
		if (disk.dosg.Valid())
		    break;
	    }
    	}
    }

    disk.SwitchToCMOS();

    int sredobsd = redobsd;
    for (;;) {
	int ret = 1;
	swaplen = swapoff = 0;

	if (O::Flag(O::UpdateFDISK)) {
	    ret = disk.FDisk();
	} else if (disk.use_fdisk) {
	    int has_fdisk = disk.has_fdisk;

	    if (has_fdisk) {
		print(main_newfdisk_long);

		if (request_yesno(main_newfdisk, 1) == 0) {
		    redobsd = 1;
		    for (int i = 0; i < 4; ++i)
			disk.part[i].Zero();
		    has_fdisk = 0;
		}
	    }

	    if (!has_fdisk && (print(main_typical_long),
				    request_yesno(main_typical, 1))) {
		redobsd = 1;
		print(main_space_long);
    	    	int p = 0;
	    	printf("\nYou have %dMB of disk space\n", disk.MBs());
		int a = request_number(main_space, 40, 1, disk.MBs());
		int b;

		//
		// Make the size of the partition a multiple of the cylinder
		// size (should this be a multiple of the BSD/OS cylinder size?)
		// less one track (DOS starts on the second track of the disk)
		// and then put BSD/OS after that, giving it all the remaining
		// space.
		//
		a = disk.FromMB(a);
		a /= disk.SecPerCyl();
		a *= disk.SecPerCyl();
		a -= disk.SecPerTrack();
		
		disk.part[p].number = p + 1;
		disk.part[p].type = MBS_DOS16;
		disk.part[p].AdjustType();
		disk.part[p].length = a;
		disk.part[p].offset = disk.SecPerTrack();

		a += disk.SecPerTrack();

    	    	++p;

    	    	print(&main_coswap_long[1]);
	moreswap:
		printf("\n");
    	    	printf(main_coswap_long[0],
		       int(disk.ToMB(disk.DefaultSwap() + disk.SecPerCyl())));
	    	printf("\nYou have %dMB of disk space remaining\n",
		       int(disk.MBs() - disk.ToMB(a)));
    	    	if (b = request_number(main_coswap, 0, 0,
		        int(disk.MBs() - disk.ToMB(a)))) {
		    b = disk.FromMB(b);
		    b /= disk.SecPerCyl();
		    b *= disk.SecPerCyl();

		    if (disk.ToMB(b - disk.SecPerCyl()) < 4.0) {
			print(main_swap_to_small);
			goto moreswap;
		    }
		    disk.part[p].number = p + 1;
		    disk.part[p].type = MBS_DOS;
		    swaplen = disk.part[p].length = b;
		    swapoff = disk.part[p].offset = a;
		    swaplen -= disk.SecPerCyl();
		    swapoff += disk.SecPerCyl();
		    ++p;
    	    	}

		disk.part[p].number = p + 1;
		disk.part[p].type = MBS_BSDI;
		disk.part[p].length = disk.UseSectors() - a - b;
		disk.part[p].offset = a + b;
    	    	disk.active = p + 1;
		if (request_yesno(main_view_fdisk, 0)) {
		    if (disk.FDisk() == -1) {
			print(main_aborted_fdisk_long);
			if (!request_yesno(main_aborted_fdisk, 0))
			    exit(1);
		    }
	    	}
	    } else
		ret = disk.FDisk();
	}

    	if (disk.use_fdisk) {
	    if (ret == 1) {
		if (!O::Flag(O::UpdateFDISK) || O::Flag(O::UpdateBSD)) {
		    for (int i = 0; i < 4 && disk.part[i].number; ++i) {
			if (BSDType(disk.part[i].type))
			    break;
		    }
		    if (i < 4 && disk.part[i].number)
			break;
		    if (request_yesno(main_need_bsd_part, 1) == 0) {
#if 0
			O::Set(O::UpdateFDISK);
			disk.use_bsd = 0;
#endif
		    	break;
		    }
		} else
		    break;
	    } else {
		print(main_redo_fdisk_long);
		if (request_yesno(main_redo_fdisk, 0) == 0) {
		    print(main_need_fdisk_for_cores);
		    exit(1);
		}
	    }
    	} else
	    break;
    }

    if (disk.use_bsd) {
	disk.SwitchToBSD();

	for (;;) {
	    int ret = 1;

	    if (disk.label_original.Valid() && !disk.label_original.Soft()) {
		if (redobsd)
		    print(main_cant_keep_existing_bsd_long);
		else
		    print(main_keep_existing_bsd_long);
    	    }

	    if (!disk.label_original.Valid() ||
		 disk.label_original.Soft() ||
		 request_yesno(main_keep_existing_bsd, redobsd)) {

		for (int i = 0; i < 8; ++i)
		    disk.bsd[i].Zero();
		//
		// Make the c partition be the whole disk.
		//
		disk.bsd[0].number = 3;
		disk.bsd[0].length = disk.Sectors();
		disk.bsd[0].type = FS_UNUSED;
		disk.bsd[0].fixed = 1;
    	    	if (swaplen) {
		    disk.Sort();

		    for (i = 0; i < 8 && disk.bsd[i].number; ++i)
			;
		    disk.bsd[i].number = 2;
		    disk.bsd[i].length = swaplen;
		    disk.bsd[i].offset = swapoff;
		    disk.bsd[i].type = FS_SWAP;
		    disk.Sort();
		}

    	    	int bp = 4;
    	    	for (int j = 0; j < 4 && disk.part[j].number; ++j) {
		    if (disk.part[j].IsDosType()) {
			for (i = 0; i < 8 && disk.bsd[i].number; ++i)
			    ;
			disk.bsd[i].number = bp++;
			disk.bsd[i].length = disk.part[j].length;
			disk.bsd[i].offset = disk.part[j].offset;
			disk.bsd[i].type = FS_MSDOS;
			disk.Sort();
		    }
    	    	}

		print(main_standard_bsd_long);
		if (request_yesno(main_standard_bsd, 1)) {
		    disk.Sort();
		    disk.AddPartition(1);		// Add a root
		    if (!swaplen) {
			disk.Sort();
			disk.AddPartition(2);		// Add a swap
    	    	    }
		    disk.Sort();
		    disk.AddPartition(8);		// Add a /usr
		    //
		    // XXX - complain if something does not work.
		    //
		    if (request_yesno(main_view_new_bsd, 0)) {
			if (disk.FSys() == -1) {
			    print(main_aborted_bsd_long);
			    if (!request_yesno(main_aborted_bsd, 0))
				exit(1);
			}
		    }
		} else
		    ret = disk.FSys();
	    } else {
		for (int i = 0; i < 8; ++i) {
		    disk.bsd[i].Load(disk.label_original.d_partitions + i);
		    disk.bsd[i].number = disk.bsd[i].length ? i + 1 : 0;
		}
		disk.bsd[2].fixed = 1;
	    	disk.Sort();
	    	if (request_yesno(main_view_bsd, 0))
		    ret = disk.FSys();
	    }

	    if (ret == 1)
		break;
	    print(main_redo_bsd_long);
	    if (request_yesno(main_redo_bsd, 0) == 0) {
		print(main_need_bsd);
		exit(1);
	    }
	}

root_again:
	disk.ComputeBSDBoot();
    	if (disk.bsdbootpart < 0) {
    	    do {
		print(main_no_root_long);
		if (request_yesno(main_no_root, 1) == 0)
		    break;
		disk.FSys();
		disk.ComputeBSDBoot();
    	    } while (disk.bsdbootpart < 0);
    	}
	//
    	// Okay, if we have a root partition, make sure it is either
	// at the start of a BSD partition or the start of the disk.
	//
    	if (disk.bsdbootpart >= 0) {
	    Partition &fp = disk.part[disk.bsdbootpart];
    	    FileSystem &rf = disk.RootFilesystem();
	    if (fp.offset != rf.offset) {
		print(fp.number ? bad_root_part_fdisk : bad_root_part);
		if (request_yesno(main_no_root, 1)) {
		    disk.FSys();
		    goto root_again;
    	    	}
    	    } else if (fp.number && !fp.IsBSDType()) {
	    	print(root_on_bsd_only);
		if (request_yesno(main_no_root, 1)) {
		    disk.FSys();
		    goto root_again;
    	    	}
    	    }
    	}
    }


    if (disk.use_fdisk && !O::Flag(O::InCoreOnly)) {
	print(main_new_master_long);
    	if (request_yesno(main_new_master, disk.has_fdisk ? 0 : 1)) {
	    print(main_bsd_master_long);
    	    master = request_string(main_bsd_master, "bootany.sys");
    	    if (!master)
		master = "bootany.sys";
    	    if (!strcmp(master, "bootany.sys")) {
	    	request_inform(bootany_long);
		O::Set(O::RequireBootany);
    	    }
    	}
    }


    if (disk.use_bsd) {
	int no_boot_blocks = O::Flag(O::InCoreOnly);
    	int labelpart = 0;

    	if (disk.use_fdisk) {
	    if (disk.FindPartition(disk.active).IsBSDType()) {
		labelpart = disk.active;
    	    } else {
    	    	labelpart = 0;
		for (int i = 1; i < 5; ++i) {
		    Partition &p = disk.FindPartition(i);
		    if (p.IsBSDType()) {
			if (p.bootany) {
			    labelpart = i;
		    	    break;
    	    	    	}
		    	if (!labelpart)
			    labelpart = i;
    	    	    }
    	    	}
    	    }
	    if (labelpart == 0) {
		print(no_place_to_label);
		if (request_yesno(label_at_0, 0) == 0)
		    exit(1);
		labelpart = -1;
		disk.bsdbootpart = -1;
		no_boot_blocks = 1;
	    } else if (disk.bsdbootpart < 0) {
	    	if (!O::Flag(O::InCoreOnly))
		    request_inform(wont_boot_part);
		no_boot_blocks = 1;
	    } else if (labelpart != disk.part[disk.bsdbootpart].number) {
	    	if (!O::Flag(O::InCoreOnly))
		    request_inform(wrong_boot_part);
		no_boot_blocks = 1;
    	    }
    	}

    	//
    	// The label will be written LABELOFFSET bytes into
    	// the LABELSECTOR.  Since disk.bsdoff was initially set
    	// to where we read our label from, we must
	// set it to where we plan to write the label
	// (checking to see if it changed, of course)
    	//
#define	LO	LABELSECTOR * disk.secsize + LABELOFFSET

	if (disk.bsdoff == -1) {
	    if (!no_boot_blocks) {
		print(main_no_bootblocks_long);
    	    }
    	} else if (!no_boot_blocks) {
	    if ((labelpart < 0 && disk.bsdoff - LO != 0) ||
		(labelpart > 0 && disk.bsdoff - LO !=
		    off_t(disk.FindPartition(labelpart).offset) * disk.secsize))
		print(main_moved_bootblocks_long);
    	}
	if (labelpart < 0) {
	    disk.bsdoff = LO;
	} else {
	    disk.bsdoff = off_t(disk.FindPartition(labelpart).offset)
				* disk.secsize + LO;
    	}

    	if (!no_boot_blocks) {
	    getbootblocks();
    	} else
	    primary = secondary = 0;

	//
	// Build a label.
	//

	//
    	// First copy in the template
    	//

    	disk.label_new = disk.label_template;

    	disk.label_new.d_type = disk.d_type;
	disk.label_new.d_secsize = disk.secsize;
	disk.label_new.d_nsectors = disk.SecPerTrack();
	disk.label_new.d_ntracks = disk.Heads();
	disk.label_new.d_ncylinders = disk.Cyls();
	disk.label_new.d_secpercyl = disk.SecPerCyl();
	disk.label_new.d_secperunit = disk.Sectors();

    	disk.label_new.d_flags &= ~(D_DEFAULT|D_SOFT);
	disk.label_new.d_npartitions = 8;
	memset(disk.label_new.d_partitions, 0,
					sizeof(disk.label_new.d_partitions));
	for (int i = 0; i < 8 && disk.bsd[i].number; ++i) {
	    FileSystem &fs = disk.bsd[i];
#define	p   disk.label_new.d_partitions[disk.bsd[i].number - 1]
    	    p.p_size = fs.length;
    	    p.p_offset = fs.offset;
	    p.p_fstype = fs.type;
	    if (p.p_fstype == FS_BSDFFS) {
	    	p.p_fsize = fs.original ? fs.fsize : 0;
	    	p.p_frag = fs.original ? fs.frag : 0;
	    	p.p_cpg = fs.original ? fs.cpg : 0;
    	    }
#undef	p
    	}
    	disk.label_new.Fixup();
    }
}

void
getbootblocks()
{
    print(main_new_bootblocks_long);
    if (request_yesno(main_new_bootblocks, 1)) {
	char *def = "bios";
	char *type;
	if (disk.d_type == DTYPE_SCSI)
	    def = "aha";
	else if (disk.d_type == DTYPE_ST506 || disk.d_type == DTYPE_ESDI) {
	    if (disk.use_fdisk && disk.dosg.Valid() && disk.dosg.heads > 16) {
		request_inform(must_use_bios_boot);
	    } else
		def = "wd";
	} else if (disk.d_type == DTYPE_FLOPPY)
	    def = "fd";
	print(main_new_bootblocks_choice_long);
	for (;;) {
	    if (type = request_string(main_new_bootblocks_choice, def)) {
		if (!strcasecmp(type, "bios")) {
		    primary = "biosboot";
		    secondary = "bootbios";
		    break;
		} else if (!strcasecmp(type, "wd")) {
		    primary = "wdboot";
		    secondary = "bootwd";
		    break;
		} else if (!strcasecmp(type, "fd")) {
		    primary = "fdboot";
		    secondary = "bootfd";
		    break;
		} else if (!strcasecmp(type, "aha")) {
		    primary = "ahaboot";
		    secondary = "bootaha";
		    break;
		} else if (!strcasecmp(type, "eaha")) {
		    primary = "eahaboot";
		    secondary = "booteaha";
		    break;
		}
	    }
	}
    }
}
