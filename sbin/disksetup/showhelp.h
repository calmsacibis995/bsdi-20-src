/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI showhelp.h,v 2.1 1995/02/03 07:16:08 polk Exp	*/

struct help_info {
    char        **help;
    int         lines;
    help_info   *last;

    help_info(char **h, int l = -1);
    ~help_info();
};

extern void showhelp();  
