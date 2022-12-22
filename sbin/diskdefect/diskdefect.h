/* how to convert a pos into various disk attributes */
#define CYLSIZE (dp->d_secpercyl * dp->d_secsize)
#define TRKSIZE (dp->d_nsectors * dp->d_secsize)
#define SN(pos) ((u_long)(pos / dp->d_secsize))
#define CYL(pos) ((u_long)(pos / CYLSIZE))
#define HEAD(pos) ((u_long)((pos % CYLSIZE) / (TRKSIZE)))
#define SEC(pos) ((u_long)(SN(pos) % dp->d_nsectors))
#define TRK(pos) ((u_long)(SN(pos) % dp->d_ntracks))

#define RETRIES	10	/* number of retries on reading old sectors */

/* disk partition containing badsector tables */
#ifdef tahoe
#define RAWPART 'a'
#else
#define	RAWPART	'c'
#endif

#define message(args)   ((verbose)?printf args:0)

#define MAXSECSIZE	4096
#define	MAXBAD		126

#ifndef i386
#define	DKBAD_MAGIC	0
#else
#define	DKBAD_MAGIC	0x4321		/* XXX !!! */
#endif

int drck_scan __P((
	int diskf,
	struct disklabel *dp,
	int verbose,
	int wflag,
	u_long maxbad,
	daddr_t *newscan));	/* RETURN */
