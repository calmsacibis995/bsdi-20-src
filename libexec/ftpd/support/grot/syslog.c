/*
**      14.4.1993
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <syslog.h>
#include <varargs.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>

extern int sys_nerr;
extern char *sys_errlist[];
extern int errno;

static int logfd = -1;
static char *logident = NULL;
static int logopt = 0, logfac = 0;

static int openlogfd()
{
        int err;
        struct sockaddr_un ad;
        if (logfd >= 0)
                return 0;
        if ((logfd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0)
                return logfd;
        if (err = fcntl(logfd, F_SETFD, 1))
                return close(logfd), logfd = -1, err;
        ad.sun_family = AF_UNIX;
        strcpy(ad.sun_path, "/dev/log");
        if (err = connect(logfd, &ad, sizeof ad))
                return close(logfd), logfd = -1, err;
        return 0;
}

int openlog(ident, opt, facility)
char *ident;
int opt, facility;
{
        int err;
        char *p;
        if (opt & ~(LOG_PID | LOG_ODELAY | LOG_NDELAY | LOG_NOWAIT))
                return errno = EINVAL, -1;
        if (facility & ~LOG_FACMASK)
                return errno = EINVAL, -1;
        logopt = opt;
        logfac = facility;
        if ((p = malloc(strlen(ident) + 1)) == NULL)
                return -1;
        strcpy(p, ident);
        if (logident != NULL)
                free(logident);
        logident = p;
        if (logopt & LOG_NDELAY && (err = openlogfd()))
                return err;
        return 0;
}

int vsyslog(priority, message, ap)
int priority;
char *message;
va_list ap;
{
        int err;
        char *p, *q, s[4096], format[512];
        time_t tm;
        if ((priority & LOG_FACMASK) == 0)
                if (logfac)
                        priority |= logfac;
                else
                        priority |= LOG_USER;
        sprintf(s, "<%u>", priority & (LOG_FACMASK | LOG_PRIMASK));
        time(&tm);
        strftime(s + strlen(s), 17, "%b %e %T ", localtime(&tm));
        if (logident != NULL)
                strcat(s, logident);
        else
                strcat(s, "syslog");
        if (logopt & LOG_PID)
                sprintf(s + strlen(s), "[%u]", getpid());
        strcat(s, ": ");
        for (p = message, q = format; *p;)
                if (*p == '%')
                        if (p[1] == 'm')
                        {
                                strcpy(q, errno >= 0 && errno < sys_nerr ?
                                        sys_errlist[errno] : "Unknown error");
                                q = format + strlen(format);
                                p += 2;
                        }
                        else
                        {
                                *q++ = *p++;
                                while (strchr("$-+ #*0123456789.l", *p) != NULL)
                                        *q++ = *p++;
                                if (*p)
                                        *q++ = *p++;
                        }
                else
                        *q++ = *p++;
        *q = 0;
        vsprintf(s + strlen(s), format, ap);
        strcat(s, "\n");
        if (err = openlogfd())
                return err;
        if ((err = send(logfd, s, strlen(s), 0)) < 0)
                return err;
        return 0;
}

int syslog(priority, message, va_alist)
int priority;
char *message;
va_dcl
{
        int err;
        va_list ap;
        va_start(ap);
        err = vsyslog(priority, message, ap);
        va_end(ap);
        return err;
}

int closelog()
{
        int err;
        if (logfd < 0)
                return 0;
        if ((err = close(logfd)) == 0)
                logfd = -1;
        return err;
}
