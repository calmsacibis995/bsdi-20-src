/* @(#) log_tcp.h 1.11 93/12/13 22:23:12 */

/* Structure filled in by the fromhost() routine. */

struct client_info {
    char   *name;			/* host name */
    char   *addr;			/* host address */
    char   *user;			/* user name ("" if not looked up) */
    int     fd;				/* socket handle */
    struct sockaddr_in *rmt_sin;	/* their side of the link */
    struct sockaddr_in *our_sin;	/* our side of the link */
    void  (*sink)();			/* datagram sink function */
};

#define from_host client_info		/* backwards compatibility */

#define FROM_UNKNOWN	"unknown"	/* name or address lookup failed */
#define FROM_HOST(f) \
    (((f)->name[0] && strcmp((f)->name, FROM_UNKNOWN)) ? (f)->name : \
	(f)->addr[0] ? (f)->addr : FROM_UNKNOWN)

/* The following are to be used in assignment context, not in comparisons. */

#define FROM_GOOD	1		/* AIX pre-empts GOOD */
#define FROM_BAD	0		/* AIX pre-empts BAD */

/* Global functions. */

extern int fromhost();			/* get/validate remote host info */
extern int hosts_access();		/* access control */
extern void refuse();			/* refuse request */
extern void shell_cmd();		/* execute shell command */
extern void percent_x();		/* do %<char> expansion */
extern char *rfc931();			/* remote name from RFC 931 daemon */
extern char *hosts_info();		/* show origin of connection */
extern void clean_exit();		/* clean up and exit */
extern void init_client();		/* init a client_info structure */

#define RFC931_POSSIBLE(f) ((f)->sink == 0 && (f)->rmt_sin && (f)->our_sin)

/* Global variables. */

extern int allow_severity;		/* for connection logging */
extern int deny_severity;		/* for connection logging */
extern char *hosts_allow_table;		/* for verification mode redirection */
extern char *hosts_deny_table;		/* for verification mode redirection */
extern char *hosts_access_file;		/* for diagnostics */
extern int hosts_access_line;		/* for diagnostics */
extern int rfc931_timeout;		/* username lookup period */

/* Bug workarounds. */

#ifdef INET_ADDR_BUG			/* inet_addr() returns struct */
#define inet_addr fix_inet_addr
extern long fix_inet_addr();
#endif 	/* INET_ADDR_BUG */

#ifdef BROKEN_FGETS			/* partial reads from sockets */
#define fgets fix_fgets
extern char *fix_fgets();
#endif 	/* BROKEN_FGETS */

#ifdef RECVFROM_BUG			/* no address family info */
#define recvfrom fix_recvfrom
extern int fix_recvfrom();
#endif 	/* RECVFROM_BUG */

#ifdef GETPEERNAME_BUG			/* claims success with UDP */
#define getpeername     fix_getpeername
extern int fix_getpeername();
#endif 	/* GETPEERNAME_BUG */
