/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI util.h,v 2.1 1995/02/03 07:16:18 polk Exp	*/

#include <errno.h>

class O {
public:
    enum CMD {
        UNSPEC,
	READ,
	RESTORE,
	INTERACTIVE,
	EDIT,
	W_ENABLE,
	W_DISABLE,
	BOOTBLKS,
	INSTALL,
    };
    enum FLAG {
        EMPTY = 0,
	ForceBoot	= 0x01,
	NoWrite		= 0x02,
    	Ignore		= 0x04 | 0x08,
    	IgnoreBSD	= 0x04,
    	IgnoreFDISK	= 0x08,
    	NonBlock	= 0x10,
    	Expert		= 0x20,
    	InCoreOnly	= 0x40,
    	UpdateFDISK	= 0x80,
    	UpdateBSD	= 0x100,
    	ReadKernel	= 0x200,
    	ReadDisk	= 0x400,
    	IgnoreGeometry	= 0x800,
    	RequireBootany	= 0x1000,
    	DontUpdateLabel	= 0x2000,
    	NoBootBlocks	= 0x4000,
    };
private:
    static CMD op;
    static int flag;
    static char *prog;
public:
    static Usage(int = 1);
    static void Set(O::CMD, int = 0);
    static void Set(O::FLAG);
    static int Cmd()		{ return(op); }
    static int Flags()		{ return(flag); }
    static int Flag(FLAG f)	{ return((flag & f) ? 1 : 0); }
    static char *Program(char * = 0);
};

class Errors {
    static		serial;
    static Errors	*root;
    Errors		*next;
    int			value;
    char		*string;
public:
    Errors(char *s)		{ string = s; value = --serial;
				  next = root; root = this; }
    Errors(char *s, int v)	{ string = s; value = v;
				  next = root; root = this; }

    operator int()		{ return(value); }
    static char *String(int i);
};

int PhysMem(int = 0);

extern char boot0[];
extern char boot1[];
extern char boot2[];
extern char *primary;
extern char *secondary;
extern char *master;

int edit();
void interactive();
void getbootblocks();
char *ChooseDisk();
void SetGeometry(char **, Geometry &);
