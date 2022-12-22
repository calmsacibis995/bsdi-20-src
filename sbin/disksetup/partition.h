/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI partition.h,v 2.1 1995/02/03 07:15:49 polk Exp	*/

struct Partition {
    unsigned char       number;
    unsigned char       type;
    unsigned char       original:1;
    unsigned char       modified:1;
    unsigned char       bootany:1;
    int length;
    int offset; 

    void Zero()         { number = type = original = 0; length = offset = 0; 
			  modified = 0; bootany = 0; }
    void Clean()        { number = type = original = 0; length = offset = 0;
			  bootany = 0; }
    Partition()         { Zero(); }
    operator =(Partition &p2) { number = p2.number;
                                type = p2.type;
                                original = p2.original;
                                length = p2.length;
                                offset = p2.offset; }
    
    int End()           { return(offset + length); }
    int Start()         { return(offset); }
    void AdjustType();
    int IsDosType();
    int IsBSDType();
    int BootType();
    char *TypeName();
    
    int operator <(Partition &);
    int operator == (Partition &p)      { return(number == p.number); }
        
    void Print(int, int);
    int Changed(Partition &);
};  

struct PType {
    char *name;
    int type;
    int (*start)(int);
    int minsector;              /* Sector that this can first exist on */
    int bootable;
};              

inline int 
Partition::operator <(Partition &p)
{
    return(offset + length <= p.offset);
}

inline int
Partition::Changed(Partition &p)   
{   
    return (p.type != type ||
            p.length != length ||
            p.offset != offset);
}   

extern void showtypes();
extern int PFindType(char *);
extern PType & PTypeFind(int);
extern int force_cyl(int);
extern int force_track(int);
extern int force_sector(int);
