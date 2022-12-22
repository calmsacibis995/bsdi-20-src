/*	BSDI	ftpd.c,v 2.1 1995/02/03 06:41:00 polk Exp	*/

/* Copyright (c) 1985, 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * University of California, Berkeley and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE. 
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1985, 1988, 1990 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ftpd.c  5.40 (Berkeley) 7/2/91";
#endif /* not lint */

/* FTP server. */
#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/wait.h>

#ifdef AIX
#include <sys/id.h>
#include <sys/priv.h>
#endif

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#define FTP_NAMES
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <setjmp.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <varargs.h>
#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#include <time.h>
#include "conversions.h"
#include "extensions.h"
#include "pathnames.h"

#ifdef M_UNIX
#include <arpa/nameser.h>
#include <resolv.h>
#endif

#if defined(SVR4) || defined(ISC)
#include <fcntl.h>
#endif

#ifdef HAVE_SYSINFO
#include <sys/systeminfo.h>
#endif

#ifdef SHADOW_PASSWORD
#include <shadow.h>
#endif

#ifdef KERBEROS
#include <sys/types.h>
#include <auth.h>
#include <krb.h>
#endif

#ifdef ULTRIX_AUTH
#include <auth.h>
#include <sys/svcinfo.h>
#endif

#ifdef HAVE_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

#ifndef TRUE
#define  TRUE   1
#endif

#ifndef FALSE
#define  FALSE  !TRUE
#endif

/* File containing login names NOT to be used on this machine. Commonly used
 * to disallow uucp. */
extern int errno;
extern int pidfd;
extern char *ctime(const time_t *);
#ifndef NO_CRYPT_PROTO
extern char *crypt(const char *, const char *);
#endif
extern char version[];
extern char *home;              /* pointer to home directory for glob */
extern FILE *ftpd_popen(char *program, char *type, int closestderr),
 *fopen(const char *, const char *),
 *freopen(const char *, const char *, FILE *);
extern int ftpd_pclose(FILE *iop),
  fclose(FILE *);
extern char *getline();
extern char cbuf[];
extern off_t restart_point;

struct sockaddr_in ctrl_addr;
struct sockaddr_in data_source;
struct sockaddr_in data_dest;
struct sockaddr_in his_addr;
struct sockaddr_in pasv_addr;

int data;
jmp_buf errcatch,
  urgcatch;
int logged_in;
struct passwd *pw;
int debug;
int timeout = 900;              /* timeout after 15 minutes of inactivity */
int maxtimeout = 7200;          /* don't allow idle time to be set beyond 2
                                 * hours */
int logging;
int log_commands = 1;
int anonymous;
int guest;
int type;
int form;
int stru;                       /* avoid C keyword */
int mode;
int usedefault = 1;             /* for data transfers */
int pdata = -1;                 /* for passive mode */
int transflag;
off_t file_size;
off_t byte_count;

#if !defined(CMASK) || CMASK == 0
#undef CMASK
#define CMASK 002
#endif
mode_t defumask = CMASK;           /* default umask value */
char tmpline[7];
char hostname[MAXHOSTNAMELEN];
char remotehost[MAXHOSTNAMELEN];
char remoteaddr[MAXHOSTNAMELEN];

/* log failures 	27-apr-93 ehk/bm */
#ifdef LOG_FAILED
#define MAXUSERNAMELEN	32
char the_user[MAXUSERNAMELEN];
#endif

/* Access control and logging passwords */
int use_accessfile = 0;
char guestpw[MAXHOSTNAMELEN];
char privatepw[MAXHOSTNAMELEN];
int nameserved = 0;
extern char authuser[];
extern int authenticated;

/* File transfer logging */
int xferlog = 0;
int log_outbound_xfers = 0;
int log_incoming_xfers = 0;

/* Allow use of lreply(); this is here since some older FTP clients don't
 * support continuation messages.  In violation of the RFCs... */
int dolreplies = 1;

/* Spontaneous reply text.  To be sent along with next reply to user */
char *autospout = NULL;
int autospout_free = 0;

/* allowed on-the-fly file manipulations (compress, tar) */
int mangleopts = 0;

/* number of login failures before attempts are logged and FTP *EXITS* */
int lgi_failure_threshold = 5;

/* Timeout intervals for retrying connections to hosts that don't accept PORT
 * cmds.  This is a kludge, but given the problems with TCP... */
#define SWAITMAX    90          /* wait at most 90 seconds */
#define SWAITINT    5           /* interval between retries */

int swaitmax = SWAITMAX;
int swaitint = SWAITINT;

SIGNAL_TYPE lostconn(int sig);
SIGNAL_TYPE randomsig(int sig);
SIGNAL_TYPE myoob(int sig);
FILE *getdatasock(char *mode),
 *dataconn(char *name, off_t size, char *mode);

#ifdef SETPROCTITLE
char **Argv = NULL;             /* pointer to argument vector */
char *LastArgv = NULL;          /* end of argv */
char proctitle[BUFSIZ];         /* initial part of title */

#endif /* SETPROCTITLE */

#ifdef KERBEROS
void init_krb();
void end_krb();
char krb_ticket_name[100];
#endif /* KERBEROS */

#ifdef ULTRIX_AUTH
int ultrix_check_pass(char *passwd, char *xpasswd);
#endif

/* ls program commands and options for lreplies on and off */
char  ls_long[50];
char  ls_short[50];
struct aclmember *entry = NULL;

main(int argc, char **argv, char **envp)
{
    int addrlen,
      on = 1;
#ifdef IPTOS_LOWDELAY
    int tos;
#endif
    char *cp;

#ifdef SecureWare
    setluid(1);                         /* make sure there is a valid luid */
    set_auth_parameters(argc,argv);
    setreuid(0, 0);
#endif
#if defined(M_UNIX) && !defined(_M_UNIX)
    res_init();                         /* bug in old (1.1.1) resolver     */
    _res.retrans = 20;                  /* because of fake syslog in 3.2.2 */
    setlogmask(LOG_UPTO(LOG_INFO));
#endif

    addrlen = sizeof(his_addr);
    if (getpeername(0, (struct sockaddr *) &his_addr, &addrlen) < 0) {
        syslog(LOG_ERR, "getpeername (%s): %m", argv[0]);
#ifndef DEBUG
        exit(1);
#endif
    }
    addrlen = sizeof(ctrl_addr);
    if (getsockname(0, (struct sockaddr *) &ctrl_addr, &addrlen) < 0) {
        syslog(LOG_ERR, "getsockname (%s): %m", argv[0]);
#ifndef DEBUG
        exit(1);
#endif
    }
#ifdef IPTOS_LOWDELAY
    tos = IPTOS_LOWDELAY;
    if (setsockopt(0, IPPROTO_IP, IP_TOS, (char *) &tos, sizeof(int)) < 0)
          syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif

    data_source.sin_port = htons(ntohs(ctrl_addr.sin_port) - 1);
    debug = 0;

#ifdef FACILITY
    openlog("ftpd", LOG_PID | LOG_NDELAY, FACILITY);
#else
    openlog("ftpd", LOG_PID);
#endif

#ifdef SETPROCTITLE
    /* Save start and extent of argv for setproctitle. */
    Argv = argv;
    while (*envp)
        envp++;
    LastArgv = envp[-1] + strlen(envp[-1]);
#endif /* SETPROCTITLE */

    argc--, argv++;
    while (argc > 0 && *argv[0] == '-') {
        for (cp = &argv[0][1]; *cp; cp++)
            switch (*cp) {

            case 'a':
                use_accessfile = 1;
                break;

            case 'A':
                use_accessfile = 0;
                break;

            case 'v':
                debug = 1;
                break;

            case 'd':
                debug = 1;
                break;

            case 'l':
                logging = 1;
                break;

            case 'L':
                log_commands = 1;
                break;

            case 'i':
                log_incoming_xfers = 1;
                break;

            case 'o':
                log_outbound_xfers = 1;
                break;

            case 't':
                timeout = atoi(++cp);
                if (maxtimeout < timeout)
                    maxtimeout = timeout;
                goto nextopt;

            case 'T':
                maxtimeout = atoi(++cp);
                if (timeout > maxtimeout)
                    timeout = maxtimeout;
                goto nextopt;

            case 'u':
                {
                    int val = 0;

                    while (*++cp && *cp >= '0' && *cp <= '9')
                        val = val * 8 + *cp - '0';
                    if (*cp)
                        fprintf(stderr, "ftpd: Bad value for -u\n");
                    else
                        defumask = val;
                    goto nextopt;
                }

            default:
                fprintf(stderr, "ftpd: Unknown flag -%c ignored.\n",
                        *cp);
                break;
            }
      nextopt:
        argc--, argv++;
    }
    (void) freopen(_PATH_DEVNULL, "w", stderr);

    /* Checking for random signals ... */
#ifdef SIGHUP
    (void) signal(SIGHUP, randomsig);
#endif
#ifdef SIGINT
    (void) signal(SIGINT, randomsig);
#endif
#ifdef SIGQUIT
    (void) signal(SIGQUIT, randomsig);
#endif
#ifdef SIGILL
    (void) signal(SIGILL, randomsig);
#endif
#ifdef SIGTRAP
    (void) signal(SIGTRAP, randomsig);
#endif
#ifdef SIGIOT
    (void) signal(SIGIOT, randomsig);
#endif
#ifdef SIGEMT
    (void) signal(SIGEMT, randomsig);
#endif
#ifdef SIGFPE
    (void) signal(SIGFPE, randomsig);
#endif
#ifdef SIGKILL
    (void) signal(SIGKILL, randomsig);
#endif
#ifdef SIGBUS
    (void) signal(SIGBUS, randomsig);
#endif
#ifdef SIGSEGV
    (void) signal(SIGSEGV, randomsig);
#endif
#ifdef SIGSYS
    (void) signal(SIGSYS, randomsig);
#endif
#ifdef SIGALRM
    (void) signal(SIGALRM, randomsig);
#endif
#ifdef SIGSTOP
    (void) signal(SIGSTOP, randomsig);
#endif
#ifdef SIGTSTP
    (void) signal(SIGTSTP, randomsig);
#endif
#ifdef SIGTTIN
    (void) signal(SIGTTIN, randomsig);
#endif
#ifdef SIGTTOU
    (void) signal(SIGTTOU, randomsig);
#endif
#ifdef SIGIO
    (void) signal(SIGIO, randomsig);
#endif
#ifdef SIGXCPU
    (void) signal(SIGXCPU, randomsig);
#endif
#ifdef SIGXFSZ
    (void) signal(SIGXFSZ, randomsig);
#endif
#ifdef SIGWINCH
    (void) signal(SIGWINCH, randomsig);
#endif
#ifdef SIGVTALRM
    (void) signal(SIGVTALRM, randomsig);
#endif
#ifdef SIGPROF
    (void) signal(SIGPROF, randomsig);
#endif
#ifdef SIGUSR1
    (void) signal(SIGUSR1, randomsig);
#endif
#ifdef SIGUSR2
    (void) signal(SIGUSR2, randomsig);
#endif

#ifdef SIGPIPE
    (void) signal(SIGPIPE, lostconn);
#endif
#ifdef SIGCHLD
    (void) signal(SIGCHLD, SIG_IGN);
#endif

#ifdef SIGURG
    if ((int) signal(SIGURG, myoob) < 0)
        syslog(LOG_ERR, "signal: %m");
#endif

    /* Try to handle urgent data inline */
#ifdef SO_OOBINLINE
    if (setsockopt(0, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(int)) < 0)
        syslog(LOG_ERR, "setsockopt (SO_OOBINLINE): %m");
#endif

#ifdef  F_SETOWN
    if (fcntl(fileno(stdin), F_SETOWN, getpid()) == -1)
        syslog(LOG_ERR, "fcntl F_SETOWN: %m");
#endif
    dolog(&his_addr);
    /* Set up default state */
    data = -1;
    type = TYPE_A;
    form = FORM_N;
    stru = STRU_F;
    mode = MODE_S;
    tmpline[0] = '\0';

#ifdef HAVE_SYSINFO
    sysinfo(SI_HOSTNAME, hostname, sizeof (hostname));
#else
    (void) gethostname(hostname, sizeof (hostname));
#endif

    access_init();
    authenticate();
    conv_init();

    if (is_shutdown(1) != 0) {
        syslog(LOG_INFO, "connection refused (server shut down) from %s [%s]",
               remotehost, remoteaddr);
        reply(500, "%s FTP server shut down -- please try again later.",
              hostname);
        exit(0);
    }
    show_banner(220);

    entry = (struct aclmember *) NULL;
    if (getaclentry("lslong", &entry) && ARG0 && strlen(ARG0) > 0) {
          strcpy(ls_long,ARG0);
      if (ARG1 && strlen(ARG1)) {
             strcat(ls_long," ");
         strcat(ls_long,ARG1);
          }
    } else {
#ifdef SVR4
#ifndef AIX
          strcpy(ls_long,"/bin/ls -la");
#else
          strcpy(ls_long,"/bin/ls -lA");
#endif
#else
          strcpy(ls_long,"/bin/ls -lgA");
#endif
    }
    strcat(ls_long," %s");

    entry = (struct aclmember *) NULL;
    if (getaclentry("lsshort", &entry) && ARG0 && strlen(ARG0) > 0) {
          strcpy(ls_short,ARG0);
      if (ARG1 && strlen(ARG1)) {
             strcat(ls_short," ");
             strcat(ls_short,ARG1);
      }
    } else {
#ifdef SVR4
#ifndef AIX
          strcpy(ls_short,"/bin/ls -la");
#else
          strcpy(ls_short,"/bin/ls -lA");
#endif
#else
          strcpy(ls_short,"/bin/ls -lgA");
#endif
    }
    strcat(ls_short," %s");

    reply(220, "%s FTP server (%s) ready.", hostname, version);
    (void) setjmp(errcatch);

    for (;;)
        (void) yyparse();
    /* NOTREACHED */
}

SIGNAL_TYPE
randomsig(int sig)
{
    syslog(LOG_ERR, "exiting on signal %d", sig);
    chdir("/etc/tmp");
    signal(SIGIOT, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    abort();
    /* dologout(-1); *//* NOTREACHED */
}

SIGNAL_TYPE
lostconn(int sig)
{
    if (debug)
        syslog(LOG_DEBUG, "lost connection to %s [%s]", remotehost, remoteaddr);
    dologout(-1);
}

static char ttyline[20];

/* Helper function for sgetpwnam(). */
char *
sgetsave(char *s)
{
    char *new;
    
    new = (char *) malloc(strlen(s) + 1);

    if (new == NULL) {
        perror_reply(421, "Local resource failure: malloc");
        dologout(1);
        /* NOTREACHED */
    }
    (void) strcpy(new, s);
    return (new);
}

/* Save the result of a getpwnam.  Used for USER command, since the data
 * returned must not be clobbered by any other command (e.g., globbing). */
struct passwd *
sgetpwnam(char *name)
{
    static struct passwd save;
    register struct passwd *p;
#ifdef M_UNIX
    struct passwd *ret = (struct passwd *) NULL;
#endif
    char *sgetsave(char *s);

#ifdef KERBEROS
    register struct authorization *q;
#endif /* KERBEROS */

#ifdef SecureWare
    struct pr_passwd *pr;
#endif

#ifdef KERBEROS
    init_krb();
    q = getauthuid(p->pw_uid);
    end_krb();
#endif /* KERBEROS */

#ifdef M_UNIX
# ifdef SecureWare
    if ((pr = getprpwnam(name)) == NULL)
        goto DONE;
# endif /* SecureWare */
    if ((p = getpwnam(name)) == NULL)
        goto DONE;
#else   /* M_UNIX */
# ifdef SecureWare
    if ((pr = getprpwnam(name)) == NULL)
        return((struct passwd *) pr);
# endif /* SecureWare */
    if ((p = getpwnam(name)) == NULL)
        return (p);
#endif  /* M_UNIX */

    if (save.pw_name)   free(save.pw_name);
    if (save.pw_gecos)  free(save.pw_gecos);
    if (save.pw_dir)    free(save.pw_dir);
    if (save.pw_shell)  free(save.pw_shell);

    save = *p;

    save.pw_name = sgetsave(p->pw_name);

#ifdef KERBEROS
    save.pw_passwd = sgetsave(q->a_password);
#elif defined(SecureWare)
     if (pr->uflg.fg_encrypt && pr->ufld.fd_encrypt && *pr->ufld.fd_encrypt)
        save.pw_passwd = sgetsave(pr->ufld.fd_encrypt);
     else
        save.pw_passwd = sgetsave("");
#else
    save.pw_passwd = sgetsave(p->pw_passwd);
#endif

    save.pw_gecos = sgetsave(p->pw_gecos);
    save.pw_dir = sgetsave(p->pw_dir);
    save.pw_shell = sgetsave(p->pw_shell);
#ifdef M_UNIX
    ret = &save;
DONE:
    endpwent();
#endif
#ifdef SecureWare
    endprpwent();
#endif
#ifdef M_UNIX
    return(ret);
#else
    return(&save);
#endif
}

int login_attempts;             /* number of failed login attempts */
int askpasswd;                  /* had user command, ask for passwd */

/* USER command. Sets global passwd pointer pw if named account exists and is
 * acceptable; sets askpasswd if a PASS command is expected.  If logged in
 * previously, need to reset state.  If name is "ftp" or "anonymous", the
 * name is not in _PATH_FTPUSERS, and ftp account exists, set anonymous and
 * pw, then just return.  If account doesn't exist, ask for passwd anyway.
 * Otherwise, check user requesting login privileges.  Disallow anyone who
 * does not have a standard shell as returned by getusershell().  Disallow
 * anyone mentioned in the file _PATH_FTPUSERS to allow people such as root
 * and uucp to be avoided. */
user(char *name)
{
    register char *cp;
    char *shell;
    char *getusershell();
    int   why = 0;

#ifdef HOST_ACCESS                     /* 19-Mar-93    BM              */
    if (!rhost_ok(name, remotehost, remoteaddr))
    {
            reply(530, "User %s access denied.", name);
            syslog(LOG_NOTICE,
                    "FTP LOGIN REFUSED (name in %s) FROM %s [%s], %s",
                     _PATH_FTPHOSTS, remotehost, remoteaddr, name);
            return;
    }
#endif

#ifdef LOG_FAILED                       /* 06-Nov-92    EHK             */
    strncpy(the_user, name, MAXUSERNAMELEN - 1);
#endif

    if (logged_in) {
        if (anonymous || guest) {
            reply(530, "Can't change user from guest login.");
            return;
        }
        end_login();
    }

    anonymous = 0;
    acl_remove();

    if (!strcasecmp(name, "ftp") || !strcasecmp(name, "anonymous")) {
      struct aclmember *entry = NULL;
      int machineok=1;
      char guestservername[MAXHOSTNAMELEN];
      guestservername[0]='\0';

      if (checkuser("ftp") || checkuser("anonymous")) {
          reply(530, "User %s access denied.", name);
          syslog(LOG_NOTICE,
	       "FTP LOGIN REFUSED (ftp in /etc/ftpusers) FROM %s [%s], %s",
	       remotehost, remoteaddr, name);
          return;
          
        /*
        ** Algorithm used:
        ** - if no "guestserver" directive is present,
        **     anonymous access is allowed, for backward compatibility.
        ** - if a "guestserver" directive is present,
        **     anonymous access is restricted to the machines listed,
        **     usually the machine whose CNAME on the current domain
        **     is "ftp"...
        **
        ** the format of the "guestserver" line is
        ** guestserver [<machine1> [<machineN>]]
        ** that is, "guestserver" will forbid anonymous access on all machines
        ** while "guestserver ftp inf" will allow anonymous access on
        ** the two machines whose CNAMES are "ftp.enst.fr" and "inf.enst.fr".
        **
        ** if anonymous access is denied on the current machine,
        ** the user will be asked to use the first machine listed (if any)
        ** on the "guestserver" line instead:
        ** 530- Guest login not allowed on this machine,
        **      connect to ftp.enst.fr instead.
        **
        ** -- <Nicolas.Pioch@enst.fr>
        */
      } else if (getaclentry("guestserver", &entry)
                 && ARG0 && strlen(ARG0) > 0) {
        struct hostent *tmphostent;

        /*
        ** if a "guestserver" line is present,
        ** default is not to allow guest logins
        */
        machineok=0;

        if (hostname[0]
            && ((tmphostent=gethostbyname(hostname)))) {

          /*
          ** hostname is the only first part of the FQDN
          ** this may or may not correspond to the h_name value
          ** (machines with more than one IP#, CNAMEs...)
          ** -> need to fix that, calling gethostbyname on hostname
          **
          ** WARNING!
          ** for SunOS 4.x, you need to have a working resolver in the libc
          ** for CNAMES to work properly.
          ** If you don't, add "-lresolv" to the libraries before compiling!
          */
          char dns_localhost[MAXHOSTNAMELEN];
          int machinecount;

          strncpy(dns_localhost,
                  tmphostent->h_name,
                  sizeof(dns_localhost));
          dns_localhost[sizeof(dns_localhost)-1]='\0';

          for (machinecount=0;
               entry->arg[machinecount] && (entry->arg[machinecount])[0];
               machinecount++) {

            if ((tmphostent=gethostbyname(entry->arg[machinecount]))) {
              /*
              ** remember the name of the first machine for redirection
              */

              if ((!machinecount) && tmphostent->h_name) {
                strncpy(guestservername, entry->arg[machinecount],
                        sizeof(guestservername));
                guestservername[sizeof(guestservername)-1]='\0';
              }

              if (!strcasecmp(tmphostent->h_name, dns_localhost)) {
                machineok++;
                break;
              }
            }
          }
        }
      }
      if (!machineok) {
        if (guestservername[0])
          reply(530,
             "Guest login not allowed on this machine, connect to %s instead.",
                guestservername);
        else
          reply(530,
                "Guest login not allowed on this machine.");
        syslog(LOG_NOTICE,
               "FTP LOGIN REFUSED (localhost not in guestservers) FROM %s [%s], %s",
               remotehost, remoteaddr, name);
        /* End of the big patch -- Nap */

        } else if ((pw = sgetpwnam("ftp")) != NULL) {
            anonymous = 1;      /* for the access_ok call */
            if ((why = access_ok(530)) == 1) {
                askpasswd = 1;
                acl_setfunctions();
                reply(331, "Guest login ok, send your complete e-mail address as password.");
            } else if (why == 0) {
                reply(530, "User %s access denied..", name);
                syslog(LOG_NOTICE,
                       "FTP LOGIN REFUSED (access denied) FROM %s [%s], %s",
                       remotehost, remoteaddr, name);
                anonymous = 0;
            } else {
                reply(530, "User %s access denied.", name);
                syslog(LOG_NOTICE,
                       "FTP LOGIN REFUSED (access denied) FROM %s [%s], %s",
                       remotehost, remoteaddr, name);
                dologout(0);
            }
        } else {
            reply(530, "User %s unknown.", name);
            syslog(LOG_NOTICE,
              "FTP LOGIN REFUSED (ftp not in /etc/passwd) FROM %s [%s], %s",
                   remotehost, remoteaddr, name);
        }
        return;
    }
    if ((pw = sgetpwnam(name)) != NULL) {
        if ((shell = pw->pw_shell) == NULL || *shell == 0)
            shell = _PATH_BSHELL;
        while ((cp = getusershell()) != NULL)
            if (strcmp(cp, shell) == 0)
                break;
        endusershell();
        if (cp == NULL || checkuser(name)) {
            reply(530, "User %s access denied...", name);
            if (logging)
                syslog(LOG_NOTICE,
                       "FTP LOGIN REFUSED (bad shell) FROM %s [%s], %s",
                       remotehost, remoteaddr, name);
            pw = (struct passwd *) NULL;
            return;
        }
        /* if user is a member of any of the guestgroups, cause a chroot() */
        /* after they log in successfully                                  */
        guest = acl_guestgroup(pw);
    }
    if (access_ok(530) < 1) {
        reply(530, "User %s access denied....", name);
        syslog(LOG_NOTICE, "FTP LOGIN REFUSED (access denied) FROM %s [%s], %s",
               remotehost, remoteaddr, name);
        return;
    } else
        acl_setfunctions();

    reply(331, "Password required for %s.", name);
    askpasswd = 1;
    /* Delay before reading passwd after first failed attempt to slow down
     * passwd-guessing programs. */
    if (login_attempts)
        sleep((unsigned) login_attempts);
}

/* Check if a user is in the file _PATH_FTPUSERS */
checkuser(char *name)
{
    register FILE *fd;
    register char *p;
    char line[BUFSIZ];

    if ((fd = fopen(_PATH_FTPUSERS, "r")) != NULL) {
        while (fgets(line, sizeof(line), fd) != NULL)
            if ((p = strchr(line, '\n')) != NULL) {
                *p = '\0';
                if (line[0] == '#')
                    continue;
                if (strcmp(line, name) == 0) {
                    (void) fclose(fd);
                    return (1);
                }
            }
        (void) fclose(fd);
    }
    return (0);
}

/* Terminate login as previous user, if any, resetting state; used when USER
 * command is given or login fails. */
end_login(void)
{

    (void) seteuid((uid_t) 0);
    if (logged_in)
        logwtmp(ttyline, "", "");
    pw = NULL;
    logged_in = 0;
    anonymous = 0;
    guest = 0;
}

int
validate_eaddr(char *eaddr)
{
    int i,
      host,
      state;

    for (i = host = state = 0; eaddr[i] != '\0'; i++) {
        switch (eaddr[i]) {
        case '.':
            if (!host)
                return 0;
            if (state == 2)
                state = 3;
            host = 0;
            break;
        case '@':
            if (!host || state > 1 || !strncasecmp("ftp", eaddr + i - host, host))
                return 0;
            state = 2;
            host = 0;
            break;
        case '!':
        case '%':
            if (!host || state > 1)
                return 0;
            state = 1;
            host = 0;
            break;
        case '-':
            break;
        default:
            host++;
        }
    }
    if (((state == 3) && host > 1) || ((state == 2) && !host) ||
        ((state == 1) && host > 1))
        return 1;
    else
        return 0;
}

pass(char *passwd)
{
    struct timeval tp;

    char *xpasswd,
     *salt;

#ifdef ULTRIX_AUTH
    int numfails;
#endif /* ULTRIX_AUTH */

    if (logged_in || askpasswd == 0) {
        reply(503, "Login with USER first.");
        return;
    }
    askpasswd = 0;

    /* Disable lreply() if the first character of the password is '-' since
     * some hosts don't understand continuation messages and hang... */

    if (*passwd == '-')
        dolreplies = 0;
    else
        dolreplies = 1;

    if (!anonymous) {           /* "ftp" is only account allowed no password */
        if (*passwd == '-')
            passwd++;
#ifdef SHADOW_PASSWORD
        if (pw) {
           struct spwd *spw = getspnam( pw->pw_name );
           if( !spw ) { pw->pw_passwd = ""; }
           else { pw->pw_passwd = spw->sp_pwdp; }
        }
#endif

        *guestpw = NULL;
        if (pw == NULL)
            salt = "xx";
        else
            salt = pw->pw_passwd;
#ifdef KERBEROS
        xpasswd = crypt16(passwd, salt);
#else
        xpasswd = crypt(passwd, salt);
#endif

#ifdef ULTRIX_AUTH
        if ((numfails = ultrix_check_pass(passwd, xpasswd)) < 0) {
#else
        /* The strcmp does not catch null passwords! */
        if (pw == NULL || *pw->pw_passwd == '\0' ||
            strcmp(xpasswd, pw->pw_passwd)) {
#endif
            reply(530, "Login incorrect.");

#ifdef LOG_FAILED                       /* 27-Apr-93    EHK/BM             */
            syslog(LOG_INFO, "failed login from %s [%s], %s",
                              remotehost, remoteaddr, the_user);
#endif
            pw = NULL;
        }

	if (pw) {
	    if (pw->pw_change || pw->pw_expire)
		(void)gettimeofday(&tp, (struct timezone *)NULL);
	    if (pw->pw_change && tp.tv_sec >= pw->pw_change) {
		reply(530, "Password has expired.");
#ifdef LOG_FAILED
		syslog(LOG_INFO, "expired password from %s [%s], %s",
		    remotehost, remoteaddr, the_user);
#endif
		pw = NULL;
	    } else if (pw->pw_expire && tp.tv_sec >= pw->pw_expire) {
		reply(530, "Account has expired.");
#ifdef LOG_FAILED
		syslog(LOG_INFO, "expired account from %s [%s], %s",
		    remotehost, remoteaddr, the_user);
#endif
		pw = NULL;
	    }
	}

	if (pw == NULL) {
            acl_remove();
            if (++login_attempts >= lgi_failure_threshold) {
                syslog(LOG_NOTICE, "repeated login failures from %s [%s]",
                       remotehost, remoteaddr);
                exit(0);
            }
            return;
	}
    } else {
        char *pwin,
         *pwout = guestpw;
        struct aclmember *entry = NULL;
        int valid;

        if (getaclentry("passwd-check", &entry) &&
            ARG0 && strcasecmp(ARG0, "none")) {

            if (!strcasecmp(ARG0, "rfc822"))
                valid = validate_eaddr(passwd);
            else if (!strcasecmp(ARG0, "trivial"))
                valid = (strchr(passwd, '@') == NULL) ? 0 : 1;
            else
                valid = 1;

            if (!valid && ARG1 && !strcasecmp(ARG1, "enforce")) {
                lreply(530, "The response '%s' is not valid", passwd);
                lreply(530, "Please use your e-mail address as your password");
                lreply(530, "   for example: %s@%s or %s@",
                       authenticated ? authuser : "joe", remotehost,
                       authenticated ? authuser : "joe");
                lreply(530, "[%s will be added if password ends with @]",
                       remotehost);
                reply(530, "Login incorrect.");
		acl_remove();	
                if (++login_attempts >= lgi_failure_threshold) {
                    syslog(LOG_NOTICE, "repeated login failures from %s [%s]",
                           remotehost, remoteaddr);
                    exit(0);
                }
                return;
            } else if (!valid) {
                lreply(230, "The response '%s' is not valid", passwd);
                lreply(230,
                "Next time please use your e-mail address as your password");
                lreply(230, "        for example: %s@%s",
                       authenticated ? authuser : "joe", remotehost);
            }
        }
        if (!*passwd) {
            strcpy(guestpw, "[none_given]");
        } else {
            int cnt = sizeof(guestpw) - 2;

            for (pwin = passwd; *pwin && cnt--; pwin++)
                if (!isgraph(*pwin))
                    *pwout++ = '_';
                else
                    *pwout++ = *pwin;
        }
    }

    /* if autogroup command applies to user's class change pw->pw_gid */
    if (anonymous)
        (void) acl_autogroup(pw);

    login_attempts = 0;         /* this time successful */
    (void) setegid((gid_t) pw->pw_gid);
    (void) initgroups(pw->pw_name, pw->pw_gid);

    /* open wtmp before chroot */
    (void) sprintf(ttyline, "ftp%d", getpid());
    logwtmp(ttyline, pw->pw_name, remotehost);
    logged_in = 1;

    /* if logging is enabled, open logfile before chroot */
    if (log_outbound_xfers || log_incoming_xfers)
        xferlog = open(_PATH_XFERLOG, O_WRONLY | O_APPEND | O_CREAT, 0660);

    expand_id();

    if (anonymous || guest) {
        /* We MUST do a chdir() after the chroot. Otherwise the old current
         * directory will be accessible as "." outside the new root! */
        if (anonymous) {
            if (chroot(pw->pw_dir) < 0 || chdir("/") < 0) {
                reply(550, "Can't set guest privileges.");
                goto bad;
            }
        } else if (guest) {
            char *sp;

            /* determine root and home directory */

            if ((sp = strstr(pw->pw_dir, "/./")) == NULL) {
                if (chroot(pw->pw_dir) < 0 || chdir("/") < 0) {
                    reply(550, "Can't set guest privileges.");
                    goto bad;
                }
            } else {
                *sp++ = '\0';

                if (chroot(pw->pw_dir) < 0 || chdir(++sp) < 0) {
                    reply(550, "Can't set guest privileges.");
                    goto bad;
                }
            }
        }
    } else {
        if (chdir(pw->pw_dir) < 0) {
            if (chdir("/") < 0) {
                reply(530, "User %s: can't change directory to %s.",
                      pw->pw_name, pw->pw_dir);
                goto bad;
            } else
                lreply(230, "No directory! Logging in with home=/");
        }
    }

#ifdef AIX
    {
       /* AIX 3 lossage.  Don't ask.  It's undocumented.  */
       priv_t priv;

       priv.pv_priv[0] = 0;
       priv.pv_priv[1] = 0;
       setgroups(NULL, NULL);
       if (setpriv(PRIV_SET|PRIV_INHERITED|PRIV_EFFECTIVE|PRIV_BEQUEATH,
                   &priv, sizeof(priv_t)) < 0 ||
           setuidx(ID_REAL|ID_EFFECTIVE, (uid_t)pw->pw_uid) < 0 ||
           seteuid((uid_t)pw->pw_uid) < 0) {
               reply(550, "Can't set uid (AIX3).");
               goto bad;
       }
    }
# ifdef UID_DEBUG
    lreply(230, "ruid=%d, euid=%d, suid=%d, luid=%d", getuidx(ID_REAL),
         getuidx(ID_EFFECTIVE), getuidx(ID_SAVED), getuidx(ID_LOGIN));
    lreply(230, "rgid=%d, egid=%d, sgid=%d, lgid=%d", getgidx(ID_REAL),
         getgidx(ID_EFFECTIVE), getgidx(ID_SAVED), getgidx(ID_LOGIN));
#endif
#else
    if (seteuid((uid_t) pw->pw_uid) < 0) {
        reply(550, "Can't set uid.");
        goto bad;
    }
#endif
    /* * following two lines were inside the next scope... */

    show_message(230, LOG_IN);
    show_readme(230, LOG_IN);

#ifdef ULTRIX_AUTH
    if (!anonymous && numfails > 0) {
        lreply(230,
            "There have been %d unsuccessful login attempts on your account",
            numfails);
    }
#endif /* ULTRIX_AUTH */    

    if (anonymous) {
        (void) is_shutdown(0);  /* display any shutdown messages now */

        reply(230, "Guest login ok, access restrictions apply.");
#ifdef SETPROCTITLE
        sprintf(proctitle, "%s: anonymous/%.*s", remotehost,
                    sizeof(proctitle) - sizeof(remotehost) -
                    sizeof(": anonymous/"), passwd);
        setproctitle("%s", proctitle);
#endif /* SETPROCTITLE */
        if (logging)
            syslog(LOG_INFO, "ANONYMOUS FTP LOGIN FROM %s [%s], %s",
                   remotehost, remoteaddr, passwd);
    } else {
        reply(230, "User %s logged in.%s", pw->pw_name, guest ?
              "  Access restrictions apply." : "");
#ifdef SETPROCTITLE
        sprintf(proctitle, "%s: %s", remotehost, pw->pw_name);
        setproctitle(proctitle);
#endif /* SETPROCTITLE */
        if (logging)
            syslog(LOG_INFO, "FTP LOGIN FROM %s [%s], %s",
                   remotehost, remoteaddr, pw->pw_name);
    }
    home = pw->pw_dir;          /* home dir for globbing */
    (void) umask(defumask);
    return;
  bad:
    /* Forget all about it... */
    xferlog = 0;
    end_login();
}

char *
opt_string(int options)
{
    static char buf[100];
    char *ptr = buf;

    if ((options & O_COMPRESS) != NULL)
        *ptr++ = 'C';
    if ((options & O_TAR) != NULL)
        *ptr++ = 'T';
    if ((options & O_UNCOMPRESS) != NULL)
        *ptr++ = 'U';
    if (options == 0)
        *ptr++ = '_';
    *ptr++ = '\0';
    return (buf);
}

retrieve(char *cmd, char *name)
{
    FILE *fin,
     *dout;
    struct stat st,
      junk;
    int (*closefunc) () = NULL;
    int options = 0;
    time_t start_time = time(NULL);
    static char *logname;
    char namebuf[MAXPATHLEN];
    struct convert *cptr;

    if (!cmd && stat(name, &st)) {
        char fnbuf[MAXPATHLEN],
         *ptr;

        cptr = cvtptr;

        if (cptr == NULL) {
            (void) reply(550, "%s: No such file OR directory.", name);
            return;
        }

        do {
            if (!(mangleopts & O_COMPRESS) && (cptr->options & O_COMPRESS))
                continue;
            if (!(mangleopts & O_UNCOMPRESS) && (cptr->options & O_UNCOMPRESS))
                continue;
            if (!(mangleopts & O_TAR) && (cptr->options & O_TAR))
                continue;

            if ( (cptr->stripfix) && (cptr->postfix) ) {
                int pfxlen = strlen(cptr->postfix);
		int sfxlen = strlen(cptr->stripfix);
                int namelen = strlen(name);
                (void) strcpy(fnbuf, name);

                if (namelen <= pfxlen)
                    continue;
		if ((namelen - pfxlen + sfxlen) >= sizeof(fnbuf))
		    continue;

		if (strcmp(fnbuf + namelen - pfxlen, cptr->postfix))
		    continue;
                *(fnbuf + namelen - pfxlen) = '\0';
                (void) strcat(fnbuf, cptr->stripfix);
                if (stat(fnbuf, &st))
                    continue;
            } else if (cptr->postfix) {
                int pfxlen = strlen(cptr->postfix);
                int namelen = strlen(name);

                if (namelen <= pfxlen)
                    continue;
                (void) strcpy(fnbuf, name);
                if (strcmp(fnbuf + namelen - pfxlen, cptr->postfix))
                    continue;
                *(fnbuf + namelen - pfxlen) = (char) NULL;
                if (stat(fnbuf, &st))
                    continue;
            } else if (cptr->stripfix) {
                (void) strcpy(fnbuf, name);
                (void) strcat(fnbuf, cptr->stripfix);
                if (stat(fnbuf, &st))
                    continue;
            } else {
                (void) reply(550, "%s: No such file OR directory.", name);
                return;
            }

            if (S_ISDIR(st.st_mode)) {
                if (!(cptr->types & T_DIR)) {
                    (void) reply(550, "Cannot %s directories.", cptr->name);
                    return;
                }
                if (cptr->options & O_TAR) {
                    strcpy(namebuf, fnbuf);
                    strcat(namebuf, "/.notar");
                    if (!stat(namebuf, &junk)) {
                        (void) reply(550,
                                  "Sorry, you may not TAR that directory.");
                        return;
                    }
                }
            }

            if (S_ISREG(st.st_mode) && !(cptr->types & T_REG)) {
                (void) reply(550, "Cannot %s plain files.", cptr->name);
                return;
            }
            if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) {
                (void) reply(550, "Cannot %s special files.", cptr->name);
                return;
            }
            if (!(cptr->types & T_ASCII) && deny_badasciixfer(550, ""))
                return;

            logname = fnbuf;
            options |= cptr->options;

            strcpy(namebuf, cptr->external_cmd);
            if ((ptr = strchr(namebuf, ' ')) != NULL)
                *ptr = '\0';
            if (stat(namebuf, &st) != NULL) {
                syslog(LOG_ERR, "external command %s not found",
                       namebuf);
                (void) reply(550,
                "Local error: conversion program not found. Cannot %s file.",
                             cptr->name);
                return;
            }
            (void) retrieve(cptr->external_cmd, fnbuf);

            goto dolog;
        } while ( (cptr = cptr->next) != NULL );

    } else
        logname = (char *) NULL;

    if (cmd == 0) {
        fin = fopen(name, "r"), closefunc = fclose;
        st.st_size = 0;
    } else {
        char line[BUFSIZ];

        (void) sprintf(line, cmd, name), name = line;
        fin = ftpd_popen(line, "r", 1), closefunc = ftpd_pclose;
        st.st_size = -1;
#ifdef HAVE_ST_BLKSIZE
        st.st_blksize = BUFSIZ;
#endif
    }
    if (fin == NULL) {
        if (errno != 0)
            perror_reply(550, name);
        return;
    }
    if (cmd == 0 &&
        (fstat(fileno(fin), &st) < 0 || (st.st_mode & S_IFMT) != S_IFREG)) {
        reply(550, "%s: not a plain file.", name);
        goto done;
    }
    if (restart_point) {
        if (type == TYPE_A) {
            register int i,
              n,
              c;

            n = restart_point;
            i = 0;
            while (i++ < n) {
                if ((c = getc(fin)) == EOF) {
                    perror_reply(550, name);
                    goto done;
                }
                if (c == '\n')
                    i++;
            }
        } else if (lseek(fileno(fin), restart_point, L_SET) < 0) {
            perror_reply(550, name);
            goto done;
        }
    }
    dout = dataconn(name, st.st_size, "w");
    if (dout == NULL)
        goto done;
#ifdef HAVE_ST_BLKSIZE
    send_data(fin, dout, st.st_blksize);
#else
    send_data(fin, dout, BUFSIZ);
#endif
    (void) fclose(dout);

  dolog:
    if (log_outbound_xfers && xferlog && (cmd == 0)) {
        char msg[MAXPATHLEN];
        int xfertime = time(NULL) - start_time;
        time_t curtime = time(NULL);
        int loop;

        if (!xfertime)
            xfertime++;
        if (realpath(logname ? logname : name, namebuf) == NULL)
		/*
		 * if realpath() fails then we don't know where
		 * we are, so at least give the file name in the log.
		 */
		strcpy(namebuf, name);
        for (loop = 0; namebuf[loop]; loop++)
            if (isspace(namebuf[loop]) || iscntrl(namebuf[loop]))
                namebuf[loop] = '_';
        sprintf(msg, "%.24s %d %s %qd %s %c %s %c %c %s ftp %d %s\n",
                ctime(&curtime),
                xfertime,
                remotehost,
                byte_count,
                namebuf,
                (type == TYPE_A) ? 'a' : 'b',
                opt_string(options),
                'o',
                anonymous ? 'a' : 'r',
                anonymous ? guestpw : pw->pw_name,
                authenticated,
                authenticated ? authuser : "*"
            );
        write(xferlog, msg, strlen(msg));
    }
    data = -1;
    pdata = -1;
  done:
    if (closefunc)
        (*closefunc) (fin);
}

store(char *name, char *mode, int unique)
{
    FILE *fout, *din;
    struct stat st;
    int (*closefunc) ();
    char *gunique(char *local);
    time_t start_time = time(NULL);

    struct aclmember *entry = NULL;

    int fdout;

#ifdef OVERWRITE
    int overwrite = 1;

#endif /* OVERWRITE */

#ifdef UPLOAD
    int open_flags = (O_RDWR | O_CREAT |
		      ((mode && *mode == 'a') ? O_APPEND : O_TRUNC));

    mode_t oldmask;
    int f_mode = -1,
      dir_mode,
      match_value = -1;
    uid_t uid;
    gid_t gid;
    uid_t oldid;
    int trunc = O_TRUNC;
    int valid = 0;

#endif /* UPLOAD */

    if (unique && stat(name, &st) == 0 &&
        (name = gunique(name)) == NULL)
        return;

    /*
     * check the filename, is it legal?
     */
    if ( (fn_check(name)) <= 0 )
        return;

#ifdef OVERWRITE
    /* if overwrite permission denied and file exists... then deny the user
     * permission to write the file. */
    while (getaclentry("overwrite", &entry) && ARG0 && ARG1 != NULL) {
        if (type_match(ARG1))
            if (strcmp(ARG0, "yes") != 0) {
                overwrite = 0;
                open_flags |= O_EXCL;
            }
    }

    if (!overwrite && !stat(name, &st)) {
        reply(553, "%s: Permission denied. (Overwrite)", name);
        return;
    }
#endif /* OVERWRITE */

#ifdef UPLOAD
    if ( (match_value = upl_check(name, &uid, &gid, &f_mode, &valid)) < 0 )
        return;

    /* Only truncate on open if the file is not to be appended to. */
    if (mode[0] == 'a' || (mode[0] == 'r' && mode[0] == '+') || restart_point)
        trunc = 0;

    /* if the user has an explicit new file mode, than open the file using
     * that mode.  We must take care to not let the umask affect the file
     * mode.
     * 
     * else open the file and let the default umask determine the file mode. */
    if (f_mode >= 0) {
        oldmask = umask(0000);
        fdout = open(name, open_flags, f_mode);
        umask(oldmask);
    } else
        fdout = open(name, open_flags, 0666);

    if (fdout < 0) {
        perror_reply(553, name);
        return;
    }
    /* if we have a uid and gid, then use them. */

    if (valid > 0) {
        oldid = geteuid();
        (void) seteuid((uid_t) 0);
        if ((fchown(fdout, uid, gid)) < 0) {
            (void) seteuid(oldid);
            perror_reply(550, "fchown");
            return;
        }
        (void) seteuid(oldid);
    }
#endif /* UPLOAD */

    if (restart_point)
        mode = "r+w";

#ifdef UPLOAD
    fout = fdopen(fdout, mode);
#else
    fout = fopen(name, mode);
#endif /* UPLOAD */

    closefunc = fclose;
    if (fout == NULL) {
        perror_reply(553, name);
        return;
    }
    if (restart_point) {
        if (type == TYPE_A) {
            register int i,
              n,
              c;

            n = restart_point;
            i = 0;
            while (i++ < n) {
                if ((c = getc(fout)) == EOF) {
                    perror_reply(550, name);
                    goto done;
                }
                if (c == '\n')
                    i++;
            }
            /* We must do this seek to "current" position because we are
             * changing from reading to writing. */
            if (fseek(fout, 0L, L_INCR) < 0) {
                perror_reply(550, name);
                goto done;
            }
        } else if (lseek(fileno(fout), restart_point, L_SET) < 0) {
            perror_reply(550, name);
            goto done;
        }
    }
    din = dataconn(name, (off_t) - 1, "r");
    if (din == NULL)
        goto done;
    if (receive_data(din, fout) == 0) {
        if (unique)
            reply(226, "Transfer complete (unique file name:%s).",
                  name);
        else
            reply(226, "Transfer complete.");
    }
    (void) fclose(din);

  dolog:
    if (log_incoming_xfers && xferlog) {
        char namebuf[MAXPATHLEN],
          msg[MAXPATHLEN];
        int xfertime = time(NULL) - start_time;
        time_t curtime = time(NULL);
        int loop;

        if (!xfertime)
            xfertime++;
        realpath(name, namebuf);
        for (loop = 0; namebuf[loop]; loop++)
            if (isspace(namebuf[loop]) || iscntrl(namebuf[loop]))
                namebuf[loop] = '_';
        sprintf(msg, "%.24s %d %s %d %s %c %s %c %c %s ftp %d %s\n",
                ctime(&curtime),
                xfertime,
                remotehost,
                byte_count,
                namebuf,
                (type == TYPE_A) ? 'a' : 'b',
                opt_string(0),
                'i',
                anonymous ? 'a' : 'r',
                anonymous ? guestpw : pw->pw_name,
                authenticated,
                authenticated ? authuser : "*"
            );
        write(xferlog, msg, strlen(msg));
    }
    data = -1;
    pdata = -1;
  done:
    (*closefunc) (fout);
}

FILE *
getdatasock(char *mode)
{
    int s,
      on = 1,
      tries;

    if (data >= 0)
        return (fdopen(data, mode));
    (void) seteuid((uid_t) 0);
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        goto bad;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                   (char *) &on, sizeof(on)) < 0)
        goto bad;
    /* anchor socket to avoid multi-homing problems */
    data_source.sin_family = AF_INET;
    data_source.sin_addr = ctrl_addr.sin_addr;
    for (tries = 1;; tries++) {
        if (bind(s, (struct sockaddr *) &data_source,
                 sizeof(data_source)) >= 0)
            break;
        if (errno != EADDRINUSE || tries > 10)
            goto bad;
        sleep(tries);
    }
#if defined(M_UNIX) && !defined(_M_UNIX)  /* bug in old TCP/IP release */
    {
        struct linger li;
        li.l_onoff = 1;
        li.l_linger = 900;
        if (setsockopt(s, SOL_SOCKET, SO_LINGER,
          (char *)&li, sizeof(struct linger)) < 0) {
            syslog(LOG_WARNING, "setsockopt (SO_LINGER): %m");
            goto bad;
        }
    }
#endif
    (void) seteuid((uid_t) pw->pw_uid);

#ifdef IPTOS_THROUGHPUT
    on = IPTOS_THROUGHPUT;
    if (setsockopt(s, IPPROTO_IP, IP_TOS, (char *) &on, sizeof(int)) < 0)
          syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif

    return (fdopen(s, mode));
  bad:
    (void) seteuid((uid_t) pw->pw_uid);
    (void) close(s);
    return (NULL);
}

FILE *
dataconn(char *name, off_t size, char *mode)
{
    char sizebuf[32];
    FILE *file;
    int retry = 0;
#ifdef IPTOS_LOWDELAY
    int tos;
#endif

    file_size = size;
    byte_count = 0;
    if (size != (off_t) -1)
        (void) sprintf(sizebuf, " (%qd bytes)", size);
    else
        (void) strcpy(sizebuf, "");
    if (pdata >= 0) {
        struct sockaddr_in from;
        int s,
          fromlen = sizeof(from);
	fd_set rfds;
	struct timeval tv;

	/* Do not blindly call accept(), since peer might never connect. */
	FD_ZERO(&rfds);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	for (;;) {
	    FD_SET(pdata, &rfds);
	    s = select(pdata + 1, &rfds, NULL, NULL, &tv);
	    if (s > 0)
		break;
	    if (s == 0) {
		reply(421, "Timeout, can't open data connection.");
		(void) close(pdata);
		pdata = -1;
		return (NULL);
	    }
	    if (errno != EINTR) {
		reply(425, "Can't open data connection.");
		(void) close(pdata);
		pdata = -1;
		return (NULL);
	    }
	    /* Should reduce tv.tv_sec here by the amount of time we were
	     * asleep in select, but not worth the effort, and someday the
	     * kernel will do it for us. */
	}
        s = accept(pdata, (struct sockaddr *) &from, &fromlen);
        if (s < 0) {
            reply(425, "Can't open data connection.");
            (void) close(pdata);
            pdata = -1;
            return (NULL);
        }
        (void) close(pdata);
        pdata = s;
#ifdef IPTOS_LOWDELAY
        tos = IPTOS_LOWDELAY;
        (void) setsockopt(s, IPPROTO_IP, IP_TOS, (char *) &tos,
                          sizeof(int));

#endif
        reply(150, "Opening %s mode data connection for %s%s.",
              type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
        return (fdopen(pdata, mode));
    }
    if (data >= 0) {
        reply(125, "Using existing data connection for %s%s.",
              name, sizebuf);
        usedefault = 1;
        return (fdopen(data, mode));
    }
    if (usedefault)
        data_dest = his_addr;
    usedefault = 1;
    file = getdatasock(mode);
    if (file == NULL) {
        reply(425, "Can't create data socket (%s,%d): %s.",
              inet_ntoa(data_source.sin_addr),
              ntohs(data_source.sin_port), strerror(errno));
        return (NULL);
    }
    data = fileno(file);
    while (connect(data, (struct sockaddr *) &data_dest,
                   sizeof(data_dest)) < 0) {
        if ((errno == EADDRINUSE || errno == EINTR) && retry < swaitmax) {
            sleep((unsigned) swaitint);
            retry += swaitint;
            continue;
        }
        perror_reply(425, "Can't build data connection");
        (void) fclose(file);
        data = -1;
        return (NULL);
    }
    reply(150, "Opening %s mode data connection for %s%s.",
          type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
    return (file);
}

/* Tranfer the contents of "instr" to "outstr" peer using the appropriate
 * encapsulation of the data subject to Mode, Structure, and Type.
 *
 * NB: Form isn't handled. */
send_data(FILE *instr, FILE *outstr, off_t blksize)
{
    register int c,
      cnt;
    register char *buf;
    int netfd,
      filefd;

    transflag++;
    if (setjmp(urgcatch)) {
        transflag = 0;
        return;
    }
    switch (type) {

    case TYPE_A:
        while ((c = getc(instr)) != EOF) {
            byte_count++;
            if (c == '\n') {
                if (ferror(outstr))
                    goto data_err;
                (void) putc('\r', outstr);
            }
            (void) putc(c, outstr);
        }
        fflush(outstr);
        transflag = 0;
        if (ferror(instr))
            goto file_err;
        if (ferror(outstr))
            goto data_err;
        reply(226, "Transfer complete.");
        return;

    case TYPE_I:
    case TYPE_L:
        if ((buf = (char *) malloc((u_int) blksize)) == NULL) {
            transflag = 0;
            perror_reply(451, "Local resource failure: malloc");
            return;
        }
        netfd = fileno(outstr);
        filefd = fileno(instr);
        while ((cnt = read(filefd, buf, (u_int) blksize)) > 0 &&
               write(netfd, buf, cnt) == cnt)
            byte_count += cnt;
        transflag = 0;
        (void) free(buf);
        if (cnt != 0) {
            if (cnt < 0)
                goto file_err;
            goto data_err;
        }
        reply(226, "Transfer complete.");
        return;
    default:
        transflag = 0;
        reply(550, "Unimplemented TYPE %d in send_data", type);
        return;
    }

  data_err:
    transflag = 0;
    perror_reply(426, "Data connection");
    return;

  file_err:
    transflag = 0;
    perror_reply(551, "Error on input file");
}

/* Transfer data from peer to "outstr" using the appropriate encapulation of
 * the data subject to Mode, Structure, and Type.
 *
 * N.B.: Form isn't handled. */
receive_data(FILE *instr, FILE *outstr)
{
    register int c;
    int cnt,
      bare_lfs = 0;
    char buf[BUFSIZ];

    transflag++;
    if (setjmp(urgcatch)) {
        transflag = 0;
        return (-1);
    }
    switch (type) {

    case TYPE_I:
    case TYPE_L:
        while ((cnt = read(fileno(instr), buf, sizeof buf)) > 0) {
            if (write(fileno(outstr), buf, cnt) != cnt)
                goto file_err;
            byte_count += cnt;
        }
        if (cnt < 0)
            goto data_err;
        transflag = 0;
        return (0);

    case TYPE_E:
        reply(553, "TYPE E not implemented.");
        transflag = 0;
        return (-1);

    case TYPE_A:
        while ((c = getc(instr)) != EOF) {
            byte_count++;
            if (c == '\n')
                bare_lfs++;
            while (c == '\r') {
                if (ferror(outstr))
                    goto data_err;
                if ((c = getc(instr)) != '\n') {
                    (void) putc('\r', outstr);
                    if (c == '\0' || c == EOF)
                        goto contin2;
                }
            }
            (void) putc(c, outstr);
          contin2:;
        }
        fflush(outstr);
        if (ferror(instr))
            goto data_err;
        if (ferror(outstr))
            goto file_err;
        transflag = 0;
        if (bare_lfs) {
            lreply(226, "WARNING! %d bare linefeeds received in ASCII mode", bare_lfs);
            printf("   File may not have transferred correctly.\r\n");
        }
        return (0);
    default:
        reply(550, "Unimplemented TYPE %d in receive_data", type);
        transflag = 0;
        return (-1);
    }

  data_err:
    transflag = 0;
    perror_reply(426, "Data Connection");
    return (-1);

  file_err:
    transflag = 0;
    perror_reply(452, "Error writing file");
    return (-1);
}

statfilecmd(char *filename)
{
    char line[BUFSIZ];
    FILE *fin;
    int c;

    if (anonymous && dolreplies)
        (void) sprintf(line, ls_long, filename);
    else
        (void) sprintf(line, ls_short, filename);
    fin = ftpd_popen(line, "r", 0);
    lreply(211, "status of %s:", filename);
    while ((c = getc(fin)) != EOF) {
        if (c == '\n') {
            if (ferror(stdout)) {
                perror_reply(421, "control connection");
                (void) ftpd_pclose(fin);
                dologout(1);
                /* NOTREACHED */
            }
            if (ferror(fin)) {
                perror_reply(551, filename);
                (void) ftpd_pclose(fin);
                return;
            }
            (void) putc('\r', stdout);
        }
        (void) putc(c, stdout);
    }
    (void) ftpd_pclose(fin);
    reply(211, "End of Status");
}

statcmd(void)
{
    struct sockaddr_in *sin;
    u_char *a,
     *p;

    lreply(211, "%s FTP server status:", hostname);
    printf("     %s\r\n", version);
    printf("     Connected to %s", remotehost);
    if (!isdigit(remotehost[0]))
        printf(" (%s)", inet_ntoa(his_addr.sin_addr));
    printf("\r\n");
    if (logged_in) {
        if (anonymous)
            printf("     Logged in anonymously\r\n");
        else
            printf("     Logged in as %s\r\n", pw->pw_name);
    } else if (askpasswd)
        printf("     Waiting for password\r\n");
    else
        printf("     Waiting for user name\r\n");
    printf("     TYPE: %s", typenames[type]);
    if (type == TYPE_A || type == TYPE_E)
        printf(", FORM: %s", formnames[form]);
    if (type == TYPE_L)
#if NBBY == 8
        printf(" %d", NBBY);
#else
        printf(" %d", bytesize);/* need definition! */
#endif
    printf("; STRUcture: %s; transfer MODE: %s\r\n",
           strunames[stru], modenames[mode]);
    if (data != -1)
        printf("     Data connection open\r\n");
    else if (pdata != -1) {
        printf("     in Passive mode");
        sin = &pasv_addr;
        goto printaddr;
    } else if (usedefault == 0) {
        printf("     PORT");
        sin = &data_dest;
      printaddr:
        a = (u_char *) & sin->sin_addr;
        p = (u_char *) & sin->sin_port;
#define UC(b) (((int) b) & 0xff)
        printf(" (%d,%d,%d,%d,%d,%d)\r\n", UC(a[0]),
               UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
#undef UC
    } else
        printf("     No data connection\r\n");
    reply(211, "End of status");
}

fatal(char *s)
{
    reply(451, "Error in server: %s\n", s);
    reply(221, "Closing connection due to server error.");
    dologout(0);
    /* NOTREACHED */
}

#if defined (HAVE_VPRINTF)
/* VARARGS2 */
reply(va_alist)
  va_dcl
{
    int n;
    char *fmt;
    va_list ap;
    
    va_start(ap);
    /* first argument is always the return code */
    n = va_arg(ap, int);
    /* second argument is always fmt string     */
    fmt = va_arg(ap, char *);

    if (autospout != NULL) {
        char *ptr = autospout;

        printf("%d-", n);
        while (*ptr) {
            if (*ptr == '\n') {
                fputs("\r\n", stdout);
                if (*(++ptr))
                    printf("%03d-", n);
            } else {
                putchar(*ptr++);
            }
        }
        if (*(--ptr) != '\n')
            printf("\r\n");
        if (autospout_free) {
            (void) free(autospout);
            autospout_free = 0;
        }
        autospout = 0;
    }
    printf("%d ", n);
    vprintf(fmt, ap);
    printf("\r\n");
    (void) fflush(stdout);

    if (debug) {
        char buf[BUFSIZ];
        (void) vsprintf(buf, fmt, ap);

        syslog(LOG_DEBUG, "<--- %d ", n);
        syslog(LOG_DEBUG, buf);
    }

    va_end(ap);
}

/* VARARGS2 */
lreply(va_alist)
  va_dcl
{
    va_list ap;
    int n;
    char *fmt;

    va_start(ap);
    /* first argument is always the return code */
    n = va_arg(ap, int);
    /* second argument is always fmt string     */
    fmt = va_arg(ap, char *);

    if (!dolreplies)
        return;
    printf("%d-", n);
    vprintf(fmt, ap);
    printf("\r\n");
    (void) fflush(stdout);

    if (debug) {
        char buf[BUFSIZ];
        (void) vsprintf(buf, fmt, ap);

        syslog(LOG_DEBUG, "<--- %d- ", n);
        syslog(LOG_DEBUG, buf);
    }

    va_end(ap);
}

#else
/* VARARGS2 */
reply(int n, char *fmt, int p0, int p1, int p2, int p3, int p4, int p5)
{
    if (autospout != NULL) {
        char *ptr = autospout;

        printf("%d-", n);
        while (*ptr) {
            if (*ptr == '\n') {
                printf("\r\n");
                if (*(++ptr))
                    printf("%d-", n);
            } else {
                putchar(*ptr++);
            }
        }
        if (*(--ptr) != '\n')
            printf("\r\n");
        if (autospout_free) {
            (void) free(autospout);
            autospout_free = 0;
        }
        autospout = 0;
    }
    printf("%d ", n);
    printf(fmt, p0, p1, p2, p3, p4, p5);
    printf("\r\n");
    (void) fflush(stdout);
    if (debug) {
        syslog(LOG_DEBUG, "<--- %d ", n);
        syslog(LOG_DEBUG, fmt, p0, p1, p2, p3, p4, p5);
    }
}

/* VARARGS2 */
lreply(int n, char *fmt, int p0, int p1, int p2, int p3, int p4, int p5)
{
    if (!dolreplies)
        return;
    printf("%d-", n);
    printf(fmt, p0, p1, p2, p3, p4, p5);
    printf("\r\n");
    (void) fflush(stdout);
    if (debug) {
        syslog(LOG_DEBUG, "<--- %d- ", n);
        syslog(LOG_DEBUG, fmt, p0, p1, p2, p3, p4, p5);
    }
}
#endif

ack(char *s)
{
    reply(250, "%s command successful.", s);
}

nack(char *s)
{
    reply(502, "%s command not implemented.", s);
}

/* ARGSUSED */
yyerror(char *s)
{
    char *cp;

    if ((cp = strchr(cbuf, '\n')) != NULL)
        *cp = '\0';
    reply(500, "'%s': command not understood.", cbuf);
}

delete(char *name)
{
    struct stat st;

    /*
     * delete permission?
     */

    if ( (del_check(name)) == 0 )
        return;

    if (lstat(name, &st) < 0) {
        perror_reply(550, name);
        return;
    }
    if ((st.st_mode & S_IFMT) == S_IFDIR) {
        if (rmdir(name) < 0) {
            perror_reply(550, name);
            return;
        }
        goto done;
    }
    if (unlink(name) < 0) {
        perror_reply(550, name);
        return;
    }
  done:
    {
        char path[MAXPATHLEN];

        realpath(name, path);

        if (anonymous) {
            syslog(LOG_NOTICE, "%s of %s [%s] deleted %s", guestpw, remotehost,
                   remoteaddr, path);
        } else {
            syslog(LOG_NOTICE, "%s of %s [%s] deleted %s", pw->pw_name,
                   remotehost, remoteaddr, path);
        }
    }

    ack("DELE");
}

cwd(char *path)
{
    struct aclmember *entry = NULL;
    char cdpath[MAXPATHLEN + 1];

    if (chdir(path) < 0) {
        /* alias checking */
        while (getaclentry("alias", &entry) && ARG0 && ARG1 != NULL) {
            if (!strcasecmp(ARG0, path)) {
                if (chdir(ARG1) < 0)
                    perror_reply(550, path);
                else {
                    show_message(250, C_WD);
                    show_readme(250, C_WD);
                    ack("CWD");
                }
                return;
            }
        }
    /* check for "cdpath" directories. */
    entry = (struct aclmember *) NULL;
        while (getaclentry("cdpath", &entry) && ARG0 != NULL) {
        strcpy(cdpath,ARG0);
        strcat(cdpath,"/");
        strcat(cdpath,path);
            if (chdir(cdpath) >= 0) {
                show_message(250, C_WD);
                show_readme(250, C_WD);
                ack("CWD");
                return;
            }
        }
        perror_reply(550,path);
    } else {
        show_message(250, C_WD);
        show_readme(250, C_WD);
        ack("CWD");
    }
}

makedir(char *name)
{
	uid_t uid;
	gid_t gid;
	int   valid = 0;

    /*
     * check the directory, can we mkdir here?
     */
    if ( (dir_check(name, &uid, &gid, &valid)) <= 0 )
        return;

    /*
     * check the filename, is it legal?
     */
    if ( (fn_check(name)) <= 0 )
        return;

    if (mkdir(name, 0777) < 0) {
        perror_reply(550, name);
	return;
    }

    reply(257, "MKD command successful.");
}

removedir(char *name)
{
	int c, d;  /* dummy variables */
        int valid = 0;

    /*
     * check the directory, can we rmdir here?
     */
    if ( (dir_check(name, &c, &d, &valid)) <= 0 )
        return;

    /*
     * delete permission?
     */

    if ( (del_check(name)) == 0 )
        return;

    if (rmdir(name) < 0)
        perror_reply(550, name);
    else
        ack("RMD");
}

pwd(void)
{
    char path[MAXPATHLEN + 1];
#ifdef HAVE_GETCWD
    extern char *getcwd();
#else
    extern char *getwd(char *);
#endif

#ifdef HAVE_GETCWD
    if (getcwd(path,MAXPATHLEN) == (char *) NULL)
#else
    if (getwd(path) == (char *) NULL)
#endif
        reply(550, "%s.", path);
    else
        reply(257, "\"%s\" is current directory.", path);
}

char *
renamefrom(char *name)
{
    struct stat st;

    if (lstat(name, &st) < 0) {
        perror_reply(550, name);
        return ((char *) 0);
    }

    /* if rename permission denied and file exists... then deny the user
     * permission to rename the file. 
     */
    while (getaclentry("rename", &entry) && ARG0 && ARG1 != NULL) {
        if (type_match(ARG1))
            if (strcmp(ARG0, "yes")) {
                reply(553, "%s: Permission denied. (rename)", name);
                return ((char *) 0);
            }
    }

    reply(350, "File exists, ready for destination name");
    return (name);
}

renamecmd(char *from, char *to)
{

    /*
     * check the filename, is it legal?
     */
    if ( (fn_check(to)) == 0 )
        return;

    if (rename(from, to) < 0)
        perror_reply(550, "rename");
    else
        ack("RNTO");
}

dolog(struct sockaddr_in *sin)
{
    struct hostent *hp;
	char *blah;

#ifdef	DNS_TRYAGAIN
    int num_dns_tries = 0;
    /*
     * 27-Apr-93    EHK/BM
     * far away connections might take some time to get their IP address
     * resolved. That's why we try again -- maybe our DNS cache has the
     * PTR-RR now. This code is sloppy. Far better is to check what the
     * resolver returned so that in case of error, there's no need to
     * try again.
     */
dns_again:
     hp = gethostbyaddr((char *) &sin->sin_addr,
                                sizeof (struct in_addr), AF_INET);

     if ( !hp && ++num_dns_tries <= 1 ) {
        sleep(3);
        goto dns_again;         /* try DNS lookup once more     */
     }
#else
    hp = gethostbyaddr((char *)&sin->sin_addr, sizeof(struct in_addr), AF_INET);
#endif

    blah = inet_ntoa(sin->sin_addr);

    (void) strncpy(remoteaddr, blah, sizeof(remoteaddr));

    if (!strcmp(remoteaddr, "0.0.0.0")) {
        nameserved = 1;
        strncpy(remotehost, "localhost", sizeof(remotehost));
    } else {
        if (hp) {
            nameserved = 1;
            (void) strncpy(remotehost, hp->h_name, sizeof(remotehost));
        } else {
            nameserved = 0;
            (void) strncpy(remotehost, remoteaddr, sizeof(remotehost));
        }
    }

#ifdef SETPROCTITLE
    sprintf(proctitle, "%s: connected", remotehost);
    setproctitle(proctitle);
#endif /* SETPROCTITLE */

    if (logging)
        syslog(LOG_INFO, "connection from %s [%s]", remotehost,
               remoteaddr);
}

/* Record logout in wtmp file and exit with supplied status. */
dologout(int status)
{
    if (logged_in) {
        (void) seteuid((uid_t) 0);
        logwtmp(ttyline, "", "");
    }
    syslog(LOG_INFO, "FTP session closed");
    if (xferlog)
        close(xferlog);
    acl_remove();
    /* beware of flushing buffers after a SIGPIPE */
    _exit(status);
}

SIGNAL_TYPE
myoob(int sig)
{
    char *cp;

    /* only process if transfer occurring */
    if (!transflag)
        return;
    cp = tmpline;
    if (getline(cp, 7, stdin) == NULL) {
        reply(221, "You could at least say goodbye.");
        dologout(0);
    }
    upper(cp);
    if (strcmp(cp, "ABOR\r\n") == 0) {
        tmpline[0] = '\0';
        reply(426, "Transfer aborted. Data connection closed.");
        reply(226, "Abort successful");
        longjmp(urgcatch, 1);
    }
    if (strcmp(cp, "STAT\r\n") == 0) {
        if (file_size != (off_t) -1)
            reply(213, "Status: %qd of %qd bytes transferred",
                  byte_count, file_size);
        else
            reply(213, "Status: %qd bytes transferred", byte_count);
    }
}

/* Note: a response of 425 is not mentioned as a possible response to the
 * PASV command in RFC959. However, it has been blessed as a legitimate
 * response by Jon Postel in a telephone conversation with Rick Adams on 25
 * Jan 89. */
passive(void)
{
    int len;
    register char *p,
     *a;

    if (!logged_in) {
	syslog(LOG_NOTICE, "passive but not logged in");
	reply(503, "Login with USER first.");
	return;
    }

    pdata = socket(AF_INET, SOCK_STREAM, 0);
    if (pdata < 0) {
        perror_reply(425, "Can't open passive connection");
        return;
    }
    pasv_addr = ctrl_addr;
    pasv_addr.sin_port = 0;
    (void) seteuid((uid_t) 0);
    if (bind(pdata, (struct sockaddr *) &pasv_addr, sizeof(pasv_addr)) < 0) {
        (void) seteuid((uid_t) pw->pw_uid);
        goto pasv_error;
    }
    (void) seteuid((uid_t) pw->pw_uid);
    len = sizeof(pasv_addr);
    if (getsockname(pdata, (struct sockaddr *) &pasv_addr, &len) < 0)
        goto pasv_error;
    if (listen(pdata, 1) < 0)
        goto pasv_error;
    a = (char *) &pasv_addr.sin_addr;
    p = (char *) &pasv_addr.sin_port;

#define UC(b) (((int) b) & 0xff)

    reply(227, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)", UC(a[0]),
          UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
    return;

  pasv_error:
    (void) close(pdata);
    pdata = -1;
    perror_reply(425, "Can't open passive connection");
    return;
}

/* Generate unique name for file with basename "local". The file named
 * "local" is already known to exist. Generates failure reply on error. */
char *
gunique(char *local)
{
    static char new[MAXPATHLEN];
    struct stat st;
    char *cp = strrchr(local, '/');
    int count = 0;

    if (cp)
        *cp = '\0';
    if (stat(cp ? local : ".", &st) < 0) {
        perror_reply(553, cp ? local : ".");
        return ((char *) 0);
    }
    if (cp)
        *cp = '/';
    (void) strcpy(new, local);
    cp = new + strlen(new);
    *cp++ = '.';
    for (count = 1; count < 100; count++) {
        (void) sprintf(cp, "%d", count);
        if (stat(new, &st) < 0)
            return (new);
    }
    reply(452, "Unique file name cannot be created.");
    return ((char *) 0);
}

/* Format and send reply containing system error number. */
perror_reply(int code, char *string)
{
    reply(code, "%s: %s.", string, strerror(errno));
}

static char *onefile[] =
{"", 0};

send_file_list(char *whichfiles)
{
    struct stat st;
    DIR *dirp = NULL;

#ifdef HAVE_DIRENT
    struct dirent *dir;
#else
    struct direct *dir;
#endif

    FILE *dout = NULL;
    register char **dirlist,
     *dirname;
    int simple = 0;
    char *strpbrk(const char *, const char *);

    if (strpbrk(whichfiles, "~{[*?") != NULL) {
        extern char **ftpglob(register char *v),
         *globerr;

        globerr = NULL;
        dirlist = ftpglob(whichfiles);
        if (globerr != NULL) {
            reply(550, globerr);
            return;
        } else if (dirlist == NULL) {
            errno = ENOENT;
            perror_reply(550, whichfiles);
            return;
        }
    } else {
        onefile[0] = whichfiles;
        dirlist = onefile;
        simple = 1;
    }

    if (setjmp(urgcatch)) {
        transflag = 0;
        return;
    }
    while ((dirname = *dirlist++) != NULL) {
        if (stat(dirname, &st) < 0) {
            /* If user typed "ls -l", etc, and the client used NLST, do what
             * the user meant. */
            if (dirname[0] == '-' && *dirlist == NULL && transflag == 0) {
                retrieve("/bin/ls %s", dirname);
                return;
            }
            perror_reply(550, whichfiles);
            if (dout != NULL) {
                (void) fclose(dout);
                transflag = 0;
                data = -1;
                pdata = -1;
            }
            return;
        }
        if ((st.st_mode & S_IFMT) == S_IFREG) {
            if (dout == NULL) {
                dout = dataconn("file list", (off_t) -1, "w");
                if (dout == NULL)
                    return;
                transflag++;
            }
            fprintf(dout, "%s%s\n", dirname,
                    type == TYPE_A ? "\r" : "");
            byte_count += strlen(dirname) + 1;
            continue;
        } else if ((st.st_mode & S_IFMT) != S_IFDIR)
            continue;

        if ((dirp = opendir(dirname)) == NULL)
            continue;

        while ((dir = readdir(dirp)) != NULL) {
            char nbuf[MAXPATHLEN];

#ifndef HAVE_DIRENT    /* does not have d_namlen */
            if (dir->d_name[0] == '.' && dir->d_namlen == 1)
#else
            if (dir->d_name[0] == '.' && (strlen(dir->d_name) == 1))
#endif
                continue;
#ifndef HAVE_DIRENT    /* does not have d_namlen */
            if (dir->d_namlen == 2 && dir->d_name[0] == '.' &&
                dir->d_name[1] == '.')
#else
            if ((strlen(dir->d_name) == 2) && dir->d_name[0] == '.' &&
                dir->d_name[1] == '.')
#endif
                continue;

            sprintf(nbuf, "%s/%s", dirname, dir->d_name);

            /* We have to do a stat to insure it's not a directory or special
             * file. */
            if (simple || (stat(nbuf, &st) == 0 &&
                           (st.st_mode & S_IFMT) == S_IFREG)) {
                if (dout == NULL) {
                    dout = dataconn("file list", (off_t) -1,
                                    "w");
                    if (dout == NULL)
                        return;
                    transflag++;
                }
                if (nbuf[0] == '.' && nbuf[1] == '/')
                    fprintf(dout, "%s%s\n", &nbuf[2],
                            type == TYPE_A ? "\r" : "");
                else
                    fprintf(dout, "%s%s\n", nbuf,
                            type == TYPE_A ? "\r" : "");
                byte_count += strlen(nbuf) + 1;
            }
        }
        (void) closedir(dirp);
    }

    if (dout == NULL)
        reply(550, "No files found.");
    else if (ferror(dout) != 0)
        perror_reply(550, "Data connection");
    else
        reply(226, "Transfer complete.");

    transflag = 0;
    if (dout != NULL)
        (void) fclose(dout);
    data = -1;
    pdata = -1;
}

#ifndef __bsdi__	/* BSDI has setproctitle in libutil */
#ifdef SETPROCTITLE
# ifndef setproctitle /* because of SCO, we use SCOproctitle instead */
/* clobber argv so ps will show what we're doing. (stolen from sendmail)
 * warning, since this is usually started from inetd.conf, it often doesn't
 * have much of an environment or arglist to overwrite. */

#ifdef HAVE_PSTAT
#include <sys/pstat.h>
#endif

/* VARARGS2 */
setproctitle(va_alist)
  va_dcl
{
    va_list ap;
    char *fmt;

    register char *p,
     *bp,
      ch;
    register int i;
    char buf[BUFSIZ];
#ifdef HAVE_PSTAT
    union pstun un;
#endif

    va_start(ap);
    /* first argument is always the fmt string */
    fmt = va_arg(ap, char *);

    (void) vsprintf(buf, fmt, ap);
    va_end(ap);

    /* make ps print our process name */
    p = Argv[0];
    *p++ = '-';

    i = strlen(buf);
#ifdef HAVE_PSTAT
    un.pst_command = buf;
    pstat(PSTAT_SETCMD, un, i, 0, 0);
#else
    if (i > LastArgv - p - 2) {
        i = LastArgv - p - 2;
        buf[i] = '\0';
    }
    bp = buf;
    while ((ch = *bp++) != (char) NULL)
        if (ch != '\n' && ch != '\r')
            *p++ = ch;
    while (p < LastArgv)
        *p++ = ' ';
#endif
}
#endif /* setproctitle */
#endif /* SETPROCTITLE */
#endif /* !__bsdi__ */

#ifdef KERBEROS
/* thanks to gshapiro@wpi.wpi.edu for the following kerberosities */

void
init_krb()
{
    char hostname[100];

#ifdef HAVE_SYSINFO
    if (sysinfo(SI_HOSTNAME, hostname, sizeof (hostname)) < 0) {
        perror("sysinfo");
#else
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        perror("gethostname");
#endif
        exit(1);
    }
    if (strchr(hostname, '.'))
        *(strchr(hostname, '.')) = 0;

    sprintf(krb_ticket_name, "/var/dss/kerberos/tkt/tkt.%d", getpid());
    krb_set_tkt_string(krb_ticket_name);

    config_auth();

    if (krb_svc_init("hesiod", hostname, (char *) NULL, 0, (char *) NULL,
                     (char *) NULL) != KSUCCESS) {
        fprintf(stderr, "Couldn't initialize Kerberos\n");
        exit(1);
    }
}

void
end_krb()
{
    unlink(krb_ticket_name);
}
#endif /* KERBEROS */

#ifdef ULTRIX_AUTH
static int
ultrix_check_pass(char *passwd, char *xpasswd)
{
    struct svcinfo *svp;
    int auth_status;

    if ((svp = getsvc()) == (struct svcinfo *) NULL) {
        syslog(LOG_WARNING, "getsvc() failed in ultrix_check_pass");
        return -1;
    }
    if (pw == (struct passwd *) NULL) {
        return -1;
    }
    if (((svp->svcauth.seclevel == SEC_UPGRADE) &&
        (!strcmp(pw->pw_passwd, "*")))
        || (svp->svcauth.seclevel == SEC_ENHANCED)) {
        if ((auth_status=authenticate_user(pw, passwd, "/dev/ttypXX")) >= 0) {
            /* Indicate successful validation */
            return auth_status;
        }
        if (auth_status < 0 && errno == EPERM) {
            /* Log some information about the failed login attempt. */
            switch(abs(auth_status)) {
            case A_EBADPASS:
                break;
            case A_ESOFTEXP:
                syslog(LOG_NOTICE, "password will expire soon for user %s",
                    pw->pw_name);
                break;
            case A_EHARDEXP:
                syslog(LOG_NOTICE, "password has expired for user %s",
                    pw->pw_name);
                break;
            case A_ENOLOGIN:
                syslog(LOG_NOTICE, "user %s attempted login to disabled acct",
                    pw->pw_name);
                break;
            }
        }
    }
    else {
        if ((*pw->pw_passwd != '\0') && (!strcmp(xpasswd, pw->pw_passwd))) {
            /* passwd in /etc/passwd isn't empty && encrypted passwd matches */
            return 0;
        }
    }
    return -1;
}
#endif /* ULTRIX_AUTH */
