/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI filesys.cc,v 2.1 1995/02/03 07:15:03 polk Exp	*/

#include "fs.h"
#include <stdlib.h>
#include <unistd.h>
#include "screen.h"
#include "disk.h"

void
FileSystem::Print(int y, int gap)
{   
    move(y,0); clrtoeol();
    
    mvprint(y, 0, "%c %-5.5s %-10.10s | %8d (%7.1f %7.1f)| %8d | %8d | ",
        number + 'a' - 1,
        FTypeFind(type).name,   
	mount,
        length,
        disk.ToMB(length),      
        disk.ToCyl(length),     
        offset,
        offset + length - 1);   

    if (gap)
        print("%8d", gap);      
}

void
FileSystem::ForceCyl()
{
    double e = disk.ToCyl(End());

    if (e != double(int(e))) {
	int ei = disk.FromCyl(double(int(e + 0.5)));
	length = ei - offset;
    }
}

void
FileSystem::Load(disklabel::partition *p)
{
    length = p->p_size;
    offset = p->p_offset;
    original = 1;
    fixed = 0;
    type = p->p_fstype;

    if (type == FS_BSDFFS) {
    	char tbuf[(sizeof(fs) + DEV_BSIZE -1) & ~(DEV_BSIZE-1)];
	struct fs *fs = (struct fs *)tbuf;
	if (lseek(disk.dfd,off_t(offset)*disk.secsize+SBOFF,L_SET)!=-1
	    && read(disk.dfd, tbuf, sizeof(tbuf)) == sizeof(tbuf)
	    && fs->fs_magic == FS_MAGIC) {
		strcpy(mount, fs->fs_fsmnt);
	}
	fsize = p->p_fsize;
	frag = p->p_frag;
	cpg = p->p_cpg;
    } else
	mount[0] = 0;
}

PType ftypes[] = {
	{ "-----",	FS_UNUSED,	force_sector, 0, },
	{ "swap",	FS_SWAP,	force_sector, 0, },
	{ "v6",		FS_V6,		force_sector, 0, },
	{ "v7",		FS_V7,		force_sector, 0, },
	{ "sysv",	FS_SYSV,	force_sector, 0, },
	{ "v71k",	FS_V71K,	force_sector, 0, },
	{ "v8",		FS_V8,		force_sector, 0, },
	{ "4.2",	FS_BSDFFS,	force_sector, 0, },
	{ "msdos",	FS_MSDOS,	force_sector, 0, },
	{ "lfs",	FS_BSDLFS,	force_sector, 0, },
	{ "hpfs",	FS_HPFS,	force_sector, 0, },
	{ "9660",	FS_ISO9660,	force_sector, 0, },
	{ "other",	FS_OTHER,	force_sector, 0, },
	{ 0, }
};

int
FFindType(char *s)
{
    while (*s == ' ')
	++s;

    char *e = s;
    while (*e)
	++e;
    while (e > s && e[-1] == ' ')
	--e;

    if (e == s)
	return(0);

    for (int x = 0; ftypes[x].name; ++x) {
	if (strncasecmp(s, ftypes[x].name, e - s) == 0)
	    return(ftypes[x].type);
    }
    int t = strtol(s, &e, 16);

    while (e && *e == ' ')
	++e;
    if (*e)
	t = 0;
    return(t);
}

PType &
FTypeFind(int p)
{
    for (int i = 0; ftypes[i].type != FS_OTHER; ++i)
	if (ftypes[i].type == p)
	    break;
    return(ftypes[i]);
}

int
PartToBSDType(int p)
{
    switch (p) {
    case 0x9f:		return(FS_BSDFFS);
    case MBS_DOS4:	return(FS_MSDOS);
    case MBS_DOS16:	return(FS_MSDOS);
    case MBS_DOS12:	return(FS_MSDOS);
    case 0x05:		return(FS_MSDOS);
    default:		return(4);
    }
}
