/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI field.h,v 2.1 1995/02/03 07:14:57 polk Exp	*/

class Field {
public:
    int y;
    int x;
    int length;
    char **full_help;
    char **full_qhelp;
    void (*redisplay)(int, int);
    int (*check)(char c);

    int NumericEdit(int &, int = -1, int (*)(int) = force_sector, int = 0, int = 0);
    int NumberEdit(int &);
    int TextEdit(char *, int (*validate)(char *) = 0);
    int LetterEdit(char &);
};

#define ESTART  17
