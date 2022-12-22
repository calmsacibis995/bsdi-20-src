/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI util.cc,v 2.1 1995/02/03 07:16:12 polk Exp	*/

#include <err.h>
#include <stdio.h>
#include <sys/param.h>
#if	_BSDI_VERSION <= 199312
#include <sys/types.h>
#include <sys/param.h>
#include <sys/kinfo.h>
#include <sys/kinfo_proc.h>
#include <sys/sysinfo.h>
#endif
#include "disk.h"
#include "util.h"
#if	_BSDI_VERSION > 199312
#include <sys/sysctl.h>
#endif

int
PhysMem(int x)
{
    static int physmem = 0;
    if (x)
	return(physmem = x);

    if (!physmem) {
#if	_BSDI_VERSION > 199312
	int mib[] = { CTL_HW, HW_PHYSMEM };
	size_t len = sizeof(physmem);

	if (sysctl(mib, sizeof(mib)/sizeof(int), &physmem, &len, NULL, 0) < 0) {
	    warn("extracting memory size");
#else
	struct sysinfo si;
    	int len = sizeof(si);

	if ((x = getkerninfo(KINFO_SYSINFO, &si, &len, 0)) < 0) {
	    warn("extracting memory size");
#endif
	    physmem = 16*1024*1024;
	}
#if	_BSDI_VERSION <= 199312
	else
	    physmem = si.sys_physmem;
#endif
    }
    return(physmem);
}

int
force_cyl(int s)
{   
    s = (s + disk.SecPerCyl() - 1) / disk.SecPerCyl();
    s *= disk.SecPerCyl(); 
    s = disk.Max(s);
    return(s);
}

int
force_track(int s)
{
    s = (s + disk.SecPerTrack() - 1) / disk.SecPerTrack();
    s *= disk.SecPerTrack();
    s = disk.Max(s);
    return(s < 0 ? 0 : s);
}

int
force_sector(int s)
{
    s = disk.Max(s);
    return(s < 0 ? 0 : s);
}

O::CMD O::op = O::UNSPEC;
int O::flag = O::EMPTY;
char *O::prog;

char *
O::Program(char *p)
{
    if (p)
	prog = p;
    return(prog);
}

void
O::Set(CMD c, int override)
{
    if (!override && (op != UNSPEC && (op != INSTALL || c != INSTALL)))
	Usage();
    op = c;
}

void
O::Set(FLAG c)
{
    if (c == Ignore) {
	if (flag & IgnoreBSD)
	    flag |= int(IgnoreFDISK) | int(NonBlock);
	else
	    flag |= int(IgnoreBSD);
    } else
	flag |= int(c);
}

char *
Errors::String(int i)
{
    if (i > 0)
	return(strerror(i));

    if (i == 0)
	return(0);

    for (Errors *e = root; e; e = e->next)
	if (e->value == i)
	    return(e->string);

    static char buf[64];
    sprintf(buf, "Unkown error %d", i);
    return(buf);
}

Errors *Errors::root = 0;
int Errors::serial = 0;
