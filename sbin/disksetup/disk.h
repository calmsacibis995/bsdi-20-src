/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI disk.h,v 2.1 1995/02/03 07:14:28 polk Exp	*/

#include "partition.h"
#include "filesys.h"
#include <machine/bootblock.h>

class Disk;

struct BootBlock {
    unsigned char	data[512];

    int Write(int fd);
    int Read(int fd);
    int Read(char *);
    void MergePartitions(mbpart *);
    void Clean()	{ memset(data, 0, sizeof(data)); }

    int Signature()	{ return((data[511] << 8) | data[510]); }
    unsigned char *Part(int i)
			{ return(data + 512 - (5 - i) * 16 - 2); }
    unsigned long Length(int i)
			{ return(*(unsigned long *)(Part(i)+12)); }
    unsigned long Offset(int i)
			{ return(*(unsigned long *)(Part(i)+8)); }

    unsigned long BTrack(int i) {
			unsigned char *p = Part(i) + 1;
			return(p[0]); }
    unsigned long BCyl(int i) {
			unsigned char *p = Part(i) + 1;
			return(p[2] | (int(p[1] & 0xc0) << 2)); }
    unsigned long BSector(int i) {
			unsigned char *p = Part(i) + 1;
			return(p[1] & 0x3f); }

    unsigned long ETrack(int i) {
			unsigned char *p = Part(i) + 5;
			return(p[0]); }
    unsigned long ECyl(int i) {
			unsigned char *p = Part(i) + 5;
			return(p[2] | (int(p[1] & 0xc0) << 2)); }
    unsigned long ESector(int i) {
			unsigned char *p = Part(i) + 5;
			return(p[1] & 0x3f); }

    unsigned char Type(int i)
			{ return(Part(i)[4]); }
    unsigned char Active(int i)
			{ return(Part(i)[0]); }
};

struct DiskLabel : public disklabel {
    int Read(int fd, off_t, off_t * = 0);
    int Internal(int);
    int WriteInternal(int);
    int ST506(int);
    int SCSI(char *);
    int Disktab(char *, char * = 0);
    void Fixup();
    void Clean();
    int Valid();
    int Soft();
    void ComputeSum(u_long = 0);
};

struct Geometry {
    int			heads;
    int			sectors;
    int			cylinders;
    int			size;
    Geometry()			{ heads = sectors = cylinders = size = 0; }
    int Valid()			{ return(cylinders && heads && sectors); }
    Geometry &operator =(Geometry &i)
				{ heads = i.heads; sectors = i.sectors;
				  cylinders = i.cylinders;
				  if (i.size) size = i.size;
				  return(*this); }
    int operator ==(Geometry &i)
				{ return(heads == i.heads &&
					 sectors == i.sectors &&
					 cylinders == i.cylinders); }
};

struct Disk {
public:
    int			dfd;
    char		device[PATH_MAX];
    char		*path;
    int			d_type;

    off_t		bsdoff;			// Offset to BSD label
    int			bsdbootpart;		// Bootable BSD partition

    int			secsize;		// # Bytes/sector
    int			badblock;

    Geometry		cmos;
    Geometry		bios;
    Geometry		fdisk;
    Geometry		bsdg;
    Geometry		dosg;

    unsigned		use_bsdg : 1;		// Only use bsd geometry
    unsigned		use_bsd : 1;		// if we are doing bsd
    unsigned		has_fdisk : 1;		// if we have an fdisk label
    unsigned		use_fdisk : 1;		// if we want an fdisk label

    unsigned		part_modified : 1;	// Original FDISK part changed
    unsigned		bsd_modified : 1;	// Original BSD part changed

    BootBlock		bootblock;
    DiskLabel		label_original;
    DiskLabel		label_template;		// Filled in by scsicmd
    DiskLabel		label_new;		// The label we will write out
    static DiskLabel	empty_label;
    mbpart		part_table[4];


    Partition		part[4];		// Where we build FDISK parts
    FileSystem		bsd[8];			// Where we build BSD parts
    static Partition	empty_partition;
    static FileSystem	empty_filsys;

    unsigned char	active;			// partition # which is active

    Disk();
    int Init(char *);
    int Type();
    int FDisk();
    int FSys();
    int EditPart(int);
    int EditBSDPart(int);
    void Sort();
    void FSort();
    void BDraw();
    void FDraw();
    int AddPartition(int = 0);
    int DefaultSwap(int = -1);
    int DefaultRoot(int = 0);
    void ComputeBSDBoot();
    FileSystem &RootFilesystem();

    mbpart *PartTable()		{ UpdatePartTable();
				  return((has_fdisk||use_fdisk)?part_table:0); }

    Partition &FindPartition(int);
    FileSystem &FindFileSystem(int);

    int FindPartForBSD(int);
    int FindPartForBSD(FileSystem &);
    off_t LabelLocation();

    int FindFPart(int);
    int FindFSys(int);
    int FindFreeSpace(int pt, int &offset, int &length);

    void SwitchToCMOS()		{ if (!dosg.Valid()) dosg = fdisk;
				  use_bsdg = !dosg.Valid(); }
    void SwitchToBSD()		{ use_bsdg = bsdg.Valid(); }

    int SecPerTrack()		{ return(use_bsdg ? bsdg.sectors
						  : dosg.sectors); }
    int Heads()			{ return(use_bsdg ? bsdg.heads
						  : dosg.heads); }
    int Cyls()			{ return(use_bsdg ? bsdg.cylinders
						  : dosg.cylinders); }
    int Sectors()		{ return(bsdg.size); }
    int UseSectors()		{ return(bsdg.size - badblock); }
    int SecPerCyl()		{ return(SecPerTrack() * Heads()); }
    int MBs()			{ return(int(ToMB(Sectors()))); }
    double ToMB(int i)		{ return(i / (1024.0 * 1024.0 / secsize)); }
    double ToCyl(int i)		{ return(i / double(SecPerCyl())); }
    int FromMB(double i);
    int FromCyl(double i);
    int FromTrack(double i);
    int FromSector(double i);
    int Max(int i)		{ return(i > UseSectors() ? UseSectors() : i); }

    void AddNumber(int &v, char *b)
	{ AddNumber(v, b, FromSector); }
    void AddNumber(int &, char *, int (Disk::*func)(double));

    void WriteEnable();
    void WriteDisable();
    void UpdatePart();
    void UpdatePartTable();
};

inline
BSDType(int t)
{
    return(t == 0x9f);
}

extern Disk disk;

extern "C" void disklabel_display(char *, FILE *, disklabel *, mbpart * = 0);
extern "C" int disklabel_getasciilabel(FILE *, disklabel *, mbpart * = 0);
extern "C" int disklabel_checklabel(disklabel *);
extern "C" u_short dkcksum(struct disklabel *);
