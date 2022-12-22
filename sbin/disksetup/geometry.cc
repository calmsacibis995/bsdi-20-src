/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI geometry.cc,v 2.1 1995/02/03 07:15:18 polk Exp	*/

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

Field GeomFields[] = {
    { 11, 10, 10, geom_head_help, geom_head_qhelp, },
    { 12, 10, 10, geom_sectors_help, geom_sectors_qhelp, },
    { 13, 10, 10, geom_cylinders_help, geom_cylinders_qhelp, },
    { 14, 10, 10, geom_total_help, geom_total_qhelp, },
    { 10,  4, 16, geom_type_help, geom_type_qhelp, },
    { 0, }
};

void
SetGeometry(char **message, Geometry &g)
{
    int h, s, c, t;
    int modified = 0;
    int sh = g.heads;
    int ss = g.sectors;
    int sc = g.cylinders;
    int st = g.size;

    //
    // If we already have the number of cylinders, then this must mean
    // we really want to view the geometry.  Hitting [ESC] will still
    // bring us back to the main menu
    //
    if (g.cylinders) {
	startvisual();
	goto view;
    }

top:
    startvisual();

    for (;;) {
	move(0,0); clrtobot();
	for (h = 0; message[h]; ++h)
	    mvprint(h+1, 2, "%s", message[h]);
	for (t = 0; pick_geometry[t]; ++t)
	    mvprint(h+t+2, 2, "%s", pick_geometry[t]);

	move(10, 0); clrtobot();
	mvprint(11, 10, "[P]robing the disk for the geometry");
	mvprint(12, 10, "[I]nternal label used by the kernel");
	mvprint(13, 10, "[F]ile containing a valid disklabel");
	mvprint(14, 10, "[D]isktab entry in /etc/disktab");
	mvprint(15, 10, "[E]nter the geometry by hand");
	if (disk.cmos.Valid())
	mvprint(16, 10, "[C]MOS geometry should be used");
	if (disk.bios.Valid())
	mvprint(17, 10, "[B]IOS geometry should be used");
	if (disk.fdisk.Valid())
	mvprint(18, 10, "[T]able use by FDISK");
    	if (g.heads && g.sectors && g.cylinders && g.size && modified)
	    mvprint(20, 10, "[ENTER] Use modified geometry");
    	if (sh && ss && sc && st)
	    mvprint(21, 10, "[ESC] Do not change the current geometry");
    	move(22, 10);


	DiskLabel lab;
	lab.Clean();

    	refresh();

	while (c = readc()) {
	    if (islower(c))
	    	c = toupper(c);
	    switch (c) {
	    default:
		continue;
    	    case '\n':
		endvisual();
		return;
	    case '\033':
		if (sh && ss && sc && st) {
		    g.heads = sh;
		    g.sectors = ss;
		    g.cylinders = sc;
		    g.size = st;
		    endvisual();
		    return;
		}
		continue;
	    case 'B':
	    	if (disk.bios.Valid()) {
		    g = disk.bios;
		    modified = 1;
		    goto gotit;
    	    	}
		break;
	    case 'C':
	    	if (disk.cmos.Valid()) {
		    g = disk.cmos;
		    modified = 1;
		    goto gotit;
    	    	}
		break;
	    case 'T':
	    	if (disk.fdisk.Valid()) {
		    g = disk.fdisk;
		    modified = 1;
		    goto gotit;
    	    	}
		break;
	    case 'I':
		if (lab.Internal(disk.dfd))
		    lab.Clean();
		else
		    disk.label_template = lab;
		break;
	    case 'P':
		if (disk.d_type == DTYPE_SCSI) {
		    endvisual();
		    if (lab.SCSI(disk.path))
			lab.Clean();
		    else
			disk.label_template = lab;
		    request_inform(press_return_to_continue);
		    startvisual();
		} else if (disk.d_type == DTYPE_ST506 ||
			   disk.d_type == DTYPE_ESDI ||
			   disk.d_type == DTYPE_FLOPPY) {
		    if (lab.ST506(disk.dfd))
			lab.Clean();
		    else
			disk.label_template = lab;
		}
		break;
	    case 'F': {
	    	endvisual();
	    	char *file = request_string(geometry_file);
	    	startvisual();
    	    	if (file) {
		    FILE *fp = fopen(file, "r");

    	    	    if (!fp) {
		    	inform(no_label_file, file);
			c = -1;
    	    	    } else if (disklabel_getasciilabel(fp, &lab, 0) == 0) {
		    	inform(bad_label_file, file);
			c = -1;
		    	fclose(fp);
    	    	    } else {
			disk.label_template = lab;
		    	fclose(fp);
    	    	    }
    	    	}
		break;
    	      }
	    case 'D': {
	    	endvisual();
	    	char *type = request_string(disktab_entry);
	    	startvisual();
    	    	if (type) {
		    int err = lab.Disktab(type);

    	    	    if (err) {
	    	    	char *msg[3];
    	    	    	msg[0] = type;
    	    	    	msg[1] = Errors::String(err);
    	    	    	msg[2] = 0;
		    	inform(msg);
			c = -1;
    	    	    } else
			disk.label_template = lab;
    	    	}
		break;
    	      }
	    case 'E':
		break;
	    }
    	    if (c != 'E' && c != -1) {
		if (lab.d_ntracks && lab.d_nsectors &&
		    lab.d_ncylinders && lab.d_secperunit) {

		    g.heads = lab.d_ntracks;
		    g.sectors = lab.d_nsectors;
		    g.cylinders = lab.d_ncylinders;
		    g.size = lab.d_secperunit;
		    modified = 1;
		} else {
		    inform(invalid_geometry);
		    c = -1;
		}
    	    }
	    break;
	}
	if (!c) {
	    endvisual();
	    return;
    	}
    	if (c != -1)
	    break;
    }

gotit:
    if (c != 'E') {
	endvisual();
	if (!request_yesno(alter_geometry, 0))
	    return;
	startvisual();
    }

view:

    char typename[sizeof(disk.label_template.d_typename)+1];
    memcpy(typename, disk.label_template.d_typename, sizeof(typename) - 1);
    typename[sizeof(typename)-1] = 0;

    move(0,0); clrtobot();
    for (h = 0; message[h]; ++h)
	mvprint(h+1, 2, "%s", message[h]);
    mvprint(10,  4, "%16s Vendor and type (informational)", typename);
    mvprint(11, 10, "%10d Heads", h = g.heads);
    mvprint(12, 10, "%10d Sectors Per Track", s = g.sectors);
    mvprint(13, 10, "%10d Cylinders", c = g.cylinders);
    mvprint(14, 10, "%10d Total Number of Sectors", t = g.size);

#define	ELINE	16

    int r;

    for (;;) {
	for (;;) {
	    r = GeomFields[4].TextEdit(typename);
	    if (r == 0)
		goto done;
	    if (r != 1)
		break;
	    for (;;) {
		r = GeomFields[0].NumberEdit(h);
		if (r == 0)
		    goto done;
	    	if (h >= 1 && h <= 64)
		    break;
    	    	inform(geom_bad_heads);
	    }
	    if (r != 1)
		break;
	    for (;;) {
		r = GeomFields[1].NumberEdit(s);
		if (r == 0)
		    goto done;
		if (s >= 1 && s <= 255);
		    break;
		inform(geom_bad_sectors);
	    }
	    if (r != 1)
		break;
	    for(;;) {
		r = GeomFields[2].NumberEdit(c);
		if (r == 0)
		    goto done;
		if (c >= 1)
		    break;
		inform(geom_bad_cylinders);
	    }
	    if (r != 1)
		break;
	    if (!t)
		t = c * s * h;
	    for(;;) {
		r = GeomFields[3].NumberEdit(t);
		if (r == 0)
		    goto done;
		if (t >= 1)
		    break;
		inform(geom_bad_size);
	    }
	    if (r != 1)
		break;
	}
	if (!(h < 1 || s < 1 || c < 1 || t < 1)) {
	    memcpy(disk.label_template.d_typename,typename,sizeof(typename)-1);
	    g.heads = h;
	    g.sectors = s;
	    g.cylinders = c;
	    g.size = t;
	    modified = 1;
	    break;
	}
	inform(geom_bad_geometry);
    }
done:
    endvisual();
    if (r == 0)
	goto top;
}
