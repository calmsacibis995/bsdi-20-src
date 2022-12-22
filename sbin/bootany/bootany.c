#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <machine/bootblock.h>

static char bootany[512];

#define	_PATH_BOOTANY	"/usr/bootstraps/bootany.sys"
char *bootany_sys = _PATH_BOOTANY;

#define	MAX_BOOTANY		4

int request_yesno(char *, int);
u_char *request_string(char *, char *);

struct bapart {
	u_char	part;
	u_char	drive;
	u_char	shead;
	u_char	ssec;
	u_char	scyl;
	u_char	text[8];
};

#define	MBRSIG		(512 - 2)
#define	PARTADDR	(MBRSIG - sizeof(struct mbpart) * 4)
#define	SIGADDR		(PARTADDR - 2)
#define	ANYADDR		(SIGADDR - (sizeof(struct bapart) * MAX_BOOTANY))

#define	SIGNATURE		0xaa55
#define	BOOTANY_SIGNATURE	0x8a0d

#define	ISBOOTANY(x)	(*((unsigned short *)(&((char *)x)[SIGADDR])) == BOOTANY_SIGNATURE)
#define	ISMBR(x)	(*((unsigned short *)(&((char *)x)[MBRSIG])) == SIGNATURE)

#define	MBS_DOSEXT	MBS_DOS

char *
sysname(int i)
{
    switch (i) {
    case MBS_DOS12:
    case MBS_DOS16:
    case MBS_DOS4:
	return("DOS");
    case MBS_DOSEXT:
	return("DOS-EXT");
    case MBS_BSDI:
	return("BSD/OS");
    case 0x0A:
	return("OS/2 BM");
    default:
	return("Unknown");
    }
}

int
bootable(int i)
{
    switch (i) {
    case MBS_DOS12:
    case MBS_DOS16:
    case MBS_DOS4:
    case MBS_BSDI:
    case 0x0A:
	return(1);
    case MBS_DOSEXT:
    default:
	return(0);
    }
}

char block[512];
char mblock[512];
struct mbpart *mb = (struct mbpart *)(block + PARTADDR);
struct mbpart *mm = 0;
struct bapart *ba = (struct bapart *)(block + ANYADDR);
int bp = 0;

char *
convname(u_char *n)
{
    static char name[9];
    char *c = name;
    n[7] |= 0x80;

    while (*c++ = (*n & 0x7f)) {
    	if (*n++ & 0x80) {
	    *c = 0;
	    break;
    	}
    }
    return(name);
}

printtable()
{
    int i;
    int p;
    struct mbpart *m;

    for (i = 0; i < MAX_BOOTANY && ba[i].part; ++i) {
	printf("%c:%d %-8.8s %d/%d/%d",
		(ba[i].part & 0x80) ? 'D' : 'C',
		 ba[i].part & 0x7F, convname(ba[i].text),
		 ba[i].scyl | (ba[i].ssec & 0xC0) << 2,
		 ba[i].shead,
		 (ba[i].ssec & 0x3F) - 1);

    	m = 0;

    	if (ba[i].part & 0x80) {
	    if ((p = ba[i].part & 0xF) && mm) {
		--p;
		m = &mm[p];
    	    }
	} else {
	    p = ba[i].part - 1;
	    m = &mb[p];
    	}
	if (m &&
	    ((m->system == MBS_DOSEXT && ba[i].shead != m->shead + 1) ||
	     (m->system != MBS_DOSEXT && ba[i].shead != m->shead) ||
	     ba[i].scyl != m->scyl ||
	     ba[i].ssec != m->ssec))
		printf(" no longer valid");
    	if (m && m->active == 0x80)
	    printf(" active");
    	printf("\n");
    }
}

main(int ac, char **av)
{
    int c;
    int i;
    int fd;
    struct mbpart *m;
    struct bapart *b;
    int dflag = 0;
    int fflag = 0;
    int iflag = 0;
    int zflag = 0;
    int nflag = 0;
    int changed = 0;
    char *mbr = 0;
    char *dmbr = 0;
    extern int optind;
    extern char *optarg;
    int disk = 0x00;
    char prompt[64];
    char *save = 0;
    u_char *name;
    u_char *p;
    int active = 0;

    while ((c = getopt(ac, av, "dfins:zA:F:?")) != EOF) {
    	switch (c) {
	case 'F':
	    bootany_sys = optarg;
	    break;
    	case 'A':
	    active = atoi(optarg);
	    if (active < 1 || active > 4) {
		fprintf(stderr, "%s: invalid.  Active partition must in the range of 1 and 4.\n");
		exit(1);
    	    }
	    break;
    	case 's':
	    save = optarg;
	    break;
    	case 'f':
	    ++fflag;
	    break;
    	case 'd':
	    ++dflag;
	    break;
    	case 'i':
	    ++iflag;
	    break;
    	case 'z':
	    ++zflag;
	    break;
    	case 'n':
	    ++nflag;
	    break;
    	usage:
	default:
	    fprintf(stderr, "Usage: bootany [-F bootany.sys] [-A fpart] [-dinz] [-s save] MBR [D:MBR]\n");
	    fprintf(stderr, "   MBR  Master Boot Record to modify (can be /dev/rxd0c)\n");
	    fprintf(stderr, " D:MBR  Second drive Master Boot Record (read only)\n");
	    fprintf(stderr, "    -F  path to find bootany.sys\n");
	    fprintf(stderr, "    -A  Make 'fpart' the active FDISK partition\n");
	    fprintf(stderr, "    -d  Assume D: does not have an FDISK table (D:MBR not requried)\n");
	    fprintf(stderr, "    -f  Print fdisk table as well\n");
	    fprintf(stderr, "    -i  Interactively assign boot partition table\n");
	    fprintf(stderr, "    -n  Install new bootany into MBR\n");
	    fprintf(stderr, "    -z  Zero existing boot partition table\n");
	    fprintf(stderr, "    -s  Save old MBR in file 'save'\n");
	    exit(1);
    	}
    }

    if (optind + 1 != ac && optind + 2 != ac)
	goto usage;

    if ((fd = open(bootany_sys, 0)) < 0) {
	perror(bootany_sys);
	exit(1);
    }
    if (read(fd, bootany, sizeof(bootany)) != sizeof(bootany)) {
	fprintf(stderr, "%s: short file\n", bootany_sys);
	exit(1);
    }
    close(fd);
    if (!ISMBR(bootany)) {
	fprintf(stderr, "%s: invalid FDISK signature\n", bootany_sys);
	exit(1);
    }
    if (!ISBOOTANY(bootany)) {
	fprintf(stderr, "%s: invalid BOOTANY signature\n", bootany_sys);
	exit(1);
    }

    mbr = av[optind];

    if ((fd = open(mbr, 2)) < 0) {
	perror(mbr);
	exit(1);
    }
    if (read(fd, block, sizeof(block)) != sizeof(block)) {
	fprintf(stderr, "%s: short read: %s\n", mbr, strerror(errno));
	exit(1);
    }

    if (!ISMBR(block)) {
	fprintf(stderr, "%s: invalid signature (probably not an MBR)\n", mbr);
	exit(1);
    }

    if (save) {
    	int sd = creat(save, 0666);

    	if (sd < 0) {
	    perror(save);
	    exit(1);
    	}

	if (write(sd, block, sizeof(block)) != sizeof(block)) {
	    fprintf(stderr, "%s: short write: %s\n", save, strerror(errno));
	    exit(1);
	}

    }

    if (nflag) {
	++changed;
	memcpy(block, bootany, PARTADDR);
    }

    if (dmbr = av[optind+1]) {
	int md = open(dmbr, 0);

    	if (md < 0) {
	    perror(dmbr);
	    exit(1);
    	}
	if (read(md, mblock, sizeof(mblock)) != sizeof(mblock)) {
	    fprintf(stderr, "%s: short read: %s\n", dmbr, strerror(errno));
	    exit(1);
	}
    	close(md);
	if (!dflag) {
	    if (!ISMBR(mblock)) {
		fprintf(stderr, "%s: invalid signature (probably not an MBR)\n", dmbr);
		exit(1);
	    }
	    mm = (struct mbpart *)(mblock + PARTADDR);
    	}
    }

    if (iflag || dflag || zflag || dmbr || (!active && !fflag)) {
	if (!ISBOOTANY(block)) {
	    fprintf(stderr, "%s: does not have bootany installed.\n", mbr);
	    exit(1);
	}
    }

    if (active) {
    	if (!mb[active-1].system || !mb[active-1].size) {
	    fprintf(stderr, "%s: not an assigned FDISK partition\n", active);
	    exit(1);
    	}
	++changed;
	for (i = 0; i < 4; ++i)
	    mb[i].active = 0;
    	mb[active-1].active = 0x80;
    }

    if (zflag) {
	++changed;
	memset(ba, 0, sizeof(ba[0]) * MAX_BOOTANY);
    }

    b = ba;

    if (ISBOOTANY(block)) {
	for (bp = 0; bp < MAX_BOOTANY && ba[bp].part; ++bp) {
	    int p;
	    m = 0;
	    if (ba[bp].part & 0x80) {
		if ((p = ba[bp].part & 0xF) && mm) {
		    --p;
		    m = &mm[p];
		}
	    } else {
		p = ba[bp].part - 1;
		m = &mb[p];
	    }
	    if (m &&
		((m->system == MBS_DOSEXT && ba[bp].shead != m->shead + 1) ||
		 (m->system != MBS_DOSEXT && ba[bp].shead != m->shead) ||
		 ba[bp].scyl != m->scyl ||
		 ba[bp].ssec != m->ssec))
		    goto bad;
	    ++b;
	    continue;
    bad:
	    if (bp < MAX_BOOTANY - 1)
		memcpy(b, b + 1, sizeof(ba[0]), MAX_BOOTANY - bp - 1);
	    memset(&ba[MAX_BOOTANY-1], 0, sizeof(ba[0]));
	    --bp;
	}
    }

    m = mb;

    if (fflag) {
    	printf("FDISK Tables:\n");
	for (i = 0; i < 4; ++i) {
	    if (mb[i].system && mb[i].size) {
		printf("C:%d 0x%02x:%-8s %d/%d/%d%s\n",
			 i + 1,
			 mb[i].system,
		    	 sysname(mb[i].system),
			 mb[i].scyl | (mb[i].ssec & 0xC0) << 2,
			 mb[i].shead,
			 (mb[i].ssec & 0x3F) - 1,
		    	 mb[i].active == 0x80 ? " active" : "");
    	    }
    	}
    	if (mm) {
	    for (i = 0; i < 4; ++i) {
		if (mm[i].system && mm[i].size) {
		    printf("D:%d 0x%02x:%-8s %d/%d/%d%s\n",
			     i + 1,
			     mm[i].system,
			     sysname(mm[i].system),
			     mm[i].scyl | (mm[i].ssec & 0xC0) << 2,
			     mm[i].shead,
			     (mm[i].ssec & 0x3F) - 1,
			     mm[i].active == 0x80 ? " active" : "");
		}
	    }
    	}
    }
    if (bp) {
	printf("Bootany Table:\n");
	printtable();
    }

    if (dflag) {
	if (bp >= MAX_BOOTANY) {
	    printf("No more bootany partitions left empty\n");
	    exit(1);
    	} else {
	    ++changed;
	    name = request_string("What do you wish to call D: (8 char max)? ",
		    "D:");
	    if (!name)
		exit(1);
	    memcpy(b->text, name, 8);
	    name = b->text;
	    for (p = name; *p && p < name + 8; ++p)
		;
	    p[-1] |= 0x80;
	    while (p < name + 8)
		*p++ = 0;
	    b->part = 0x80;
	    b->drive = 0x81;
	    b->shead = 0;
	    b->ssec = 1;
	    b->scyl = 0;
	    ++b;
	    ++bp;
    	}
	dmbr = 0;
    }

    if (iflag) {
	int a;
    	int once = 0;

	printf("\n");
again:
	a = 0;
	for (i = 1; i <= 4; ++i, ++m) {
	    if (m->system && m->size) {
	    	if (m->active == 0x80) {
		    if (a++) {
		    	printf("Deactiving extra active partition\n");
			m->active = 0;
	    	    }
		}
		if (bp >= MAX_BOOTANY) {
		    if (!once++)
			printf("No more bootany partitions left empty\n");
		    continue;
		}
		sprintf(prompt, "Is %c:%d of type %s (%02x) bootable? ",
			disk ? 'D' : 'C',
			i, sysname(m->system), m->system);
		if (request_yesno(prompt, bootable(m->system)) == 0)
		    continue;
		name = request_string("What do you wish to call it (8 char max)? ",
			sysname(m->system));
		if (!name)
		    exit(1);
		++changed;
		memcpy(b->text, name, 8);
		name = b->text;
		for (p = name; *p && p < name + 8; ++p)
		    ;
		p[-1] |= 0x80;
		while (p < name + 8)
		    *p++ = 0;
		b->part = i | disk;
		b->drive = disk ? 0x81 : 0x80;
		b->shead = m->shead;
    	    	if (m->system == MBS_DOSEXT) {
		    printf("Warning, allowing boots from extended DOS partition\n");
		    printf("Assuming that the actual partition starts 1 track in\n");
		    b->shead++;
    	    	}
		b->ssec = m->ssec;
		b->scyl = m->scyl;
		++b;
		++bp;
	    }
	}
	if (mm && bp < MAX_BOOTANY) {
	    m = mm;
	    disk = 0x80;
	    mm = 0;
	    goto again;
    	}
    }
    if (changed && request_yesno("Write out new MBR?", 1)) {
	lseek(fd, 0, L_SET);
	if (write(fd, block, sizeof(block)) != sizeof(block)) {
	    fprintf(stderr, "%s: short write: %s\n", mbr, strerror(errno));
	    exit(1);
	}
    }
}

int
truth(char *s, int def)
{   
    while (isspace(*s))
        ++s;
    if (!*s || *s == '\n')
        return(def);
    if (*s == 'Y' || *s == 'y')
        return(1);
    if (*s == 'N' || *s == 'n')
        return(0);
    return(-1);
}       

int
request_yesno(char *prompt, int def)
{
    char buf[64];
    int t;

    do {
	printf("%s [%s] ", prompt, def ? "YES" : "NO");
	if (!fgets(buf, sizeof(buf), stdin))
	    return(def);
    } while ((t = truth(buf, def)) == -1);
    return(t);
}

u_char *
request_string(char *prompt, char *def)
{
    static char buf[1024];
    u_char *s, *e;

    do {
    	printf("%s ", prompt);
    	if (def)
    	    printf("[%s] ", def);
	if(!fgets(buf, sizeof(buf), stdin))
	    return(0);
	s = (u_char *)buf;
    	while (*s == ' ' || *s == '\t' || *s == '\n')
	    ++s;
    	e = s;
	while (*e)
	    ++e;
    	while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n'))
	    --e;
	*e = 0;
    	if (*s == '\0')
	    s = (u_char *)def;
    } while (s && *s == '\0');
    return(s);
}
