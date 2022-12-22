/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI partition.cc,v 2.1 1995/02/03 07:15:42 polk Exp	*/

#include <stdlib.h>
#include "screen.h"
#include "disk.h"

int    
Partition::IsDosType()
{
    return(type == MBS_DOS12 || type == MBS_DOS16 || type == MBS_DOS4);
}

int    
Partition::IsBSDType()
{
    return(BSDType(type));
}

void    
Partition::AdjustType()
{
    if (type == MBS_DOS12 || type == MBS_DOS16 || type == MBS_DOS4) {
        if (length < 16 * 1024 * 1024 / disk.secsize) {
            if (type != MBS_DOS12) {
                type = MBS_DOS12;
            }   
        } else if (length < 32 * 1024 * 1024 / disk.secsize) {
            if (type != MBS_DOS16) {
                type = MBS_DOS16;
            }
        } else if (type != MBS_DOS4) {
            type = MBS_DOS4;
        }
    }
}
 
void
Partition::Print(int y, int gap)
{   
    move(y,0); clrtoeol();
    
    mvprint(y, 0, "%c%c | %02X %-10.10s | %8d (%7.1f %7.1f)| %8d | %8d | ",
        number + '0',
        number == disk.active ? '*' : ' ',
        type,
        PTypeFind(type).name,   
        length,
        disk.ToMB(length),      
        disk.ToCyl(length),     
        offset,
        offset + length - 1);   

    if (gap && (number != disk.part[0].number ||
                        (gap != disk.SecPerTrack() && 
                         gap != disk.SecPerCyl())))
        print("%8d", gap);      
}

int    
Partition::BootType()
{
    return(PTypeFind(type).bootable);
}

char *
Partition::TypeName()
{
    return(PTypeFind(type).name);
}

PType ptypes[] = {
        { "BSD/OS",   		0x9f,		force_sector,0, 1, },
        { "DOS", 		MBS_DOS4,	force_track, 1, 1, },
        { "DOS-FAT16",		MBS_DOS16,	force_track, 1, 1, },
        { "DOS-FAT12",		MBS_DOS12,	force_track, 1, 1, },
        { "DOS-EXTEND",		0x05,		force_track, 1, 0, },
    	{ "XENIX",		0x02,		force_track, 1, 1, },
    	{ "XENIX-USR",		0x03,		force_track, 1, 0, },
        { "HPFS", 		0x07,		force_track, 1, 1, },
        { "AIX", 		0x08,		force_track, 1, 1, },
        { "AIX-BOOT", 		0x09,		force_track, 1, 1, },
        { "OS2BOOT", 		0x0A,		force_track, 1, 1, },
        { "VENIX", 		0x40,		force_track, 1, 1, },
        { "NOVEL", 		0x51,		force_track, 1, 1, },
        { "CP/M", 		0x52,		force_track, 1, 1, },
        { "MINIX", 		0x81,		force_track, 1, 1, },
        { "LINUX", 		0x82,		force_track, 1, 1, },
        { "AMOEBA", 		0x93,		force_track, 1, 1, },
        { "<Unknown>",		0x00,		force_sector, 0, 1, }, 
};

void
showtypes()
{
    for (int i = 0; ptypes[i].type; ++i)
	;

    char buf[((i + 4)/5) * 60];
    char *b = buf;

    for (i = 0; ptypes[i].type; ++i) {
    	sprintf(b, "%-10.10s ", ptypes[i].name);
	b += 11;
	if (i % 5 == 4)
	    b[-1] = '\n';
    }
    while (i++ % 5) {
	memset(b, ' ', 11);
	b += 11;
    }
    b[-1] = '\n';
    *b = 0;
    inform("The following types are known by disksetup:\n\n%s\n"
	   "(With DOS partition types, the type is adjusted\n"
	   " according to the length of the partition.)    ", buf);
}

int
PFindType(char *s)
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

    for (int x = 0; ptypes[x].type; ++x) {
	if (strncasecmp(s, ptypes[x].name, e - s) == 0)
	    return(ptypes[x].type);
    }
    int t = strtol(s, &e, 16);

    while (e && *e == ' ')
	++e;
    if (*e)
	t = 0;
    return(t);
}

PType &
PTypeFind(int p)
{
    for (int i = 0; ptypes[i].type; ++i)
	if (ptypes[i].type == p)
	    break;
    return(ptypes[i]);
}
