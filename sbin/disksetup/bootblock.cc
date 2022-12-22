/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI bootblock.cc,v 2.1 1995/02/03 07:13:49 polk Exp	*/

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include "disk.h"
#include "util.h"

Errors ENOBOOTBLOCK("Could not read the boot block for this device");
Errors ESHORTBOOTBLOCK("Unable to write boot block (short write)");

int
BootBlock::Read(char *name)
{
    int fd = open(name, O_RDONLY);

    if (fd < 0)
	return(errno);

    int ret = Read(fd);
    close(fd);
    return(ret);
}

int
BootBlock::Read(int fd)
{
    int e = read(fd, data, sizeof(data));

    if (e >= 0 && e != sizeof(data))
	return(ENOBOOTBLOCK);
    if (e < 0)
	return(errno);
    return(0);
}

int
BootBlock::Write(int fd)
{
    int e = write(fd, data, sizeof(data));

    if (e >= 0 && e != sizeof(data))
	return(ESHORTBOOTBLOCK);
    if (e < 0)
	return(errno);
    return(0);
}

void
BootBlock::MergePartitions(mbpart *p)
{
    memcpy(Part(1), p, 16 * 4);
    data[510] = 0x55;
    data[511] = 0xaa;
}
