/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*	BSDI bootany.h,v 2.1 1995/02/03 06:39:56 polk Exp	*/

#include <stdio.h>
#include <sys/param.h>
#define MAX_BOOTANY_PARTS  3       /* Can't fit any more           */
#define BOOTANY_TEXT_LEN   15      /* max bytes for partition desc */

/* a BOOTANY partition descriptor */
struct bootany_partdata {
	unsigned char	partnum;	/* 1 origin instead of 0 */
	char		text[BOOTANY_TEXT_LEN];
	unsigned char	term;		/* should be 0x80 in our case */
};

/* all BOOTANY specific data */
struct bootany_data {
    struct bootany_partdata	part_desc[MAX_BOOTANY_PARTS];
    unsigned char		numlockmask;
};
