/*	BSDI startslip.c,v 2.1 1995/02/03 07:39:22 polk Exp	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1990, 1991 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)startslip.c	5.6 (Berkeley) 3/16/92";
#endif /* not lint */

#include <sys/param.h>
#include <sys/termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/slip.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	DEFAULT_BAUD    B9600
#define	MAXTRIES	6	/* w/60 sec and doubling, takes an hour */
#define	PIDFILE		"/var/run/startslip.pid"

int     speed = DEFAULT_BAUD;
char	*pidfile = PIDFILE;
int	wait_time = 60;		/* then back off */

char	*device, *login, *passwd;
int	hup, logged_in;

#define	READ_TIMEOUT	60	/* if nothing from line after this, reject */
volatile sig_atomic_t	dojump;		/* true => alarm should abort read() */
jmp_buf	abortread;

#ifdef	DEBUG
int	debug = 1;
#define	syslog fprintf
#undef	LOG_ERR
#undef	LOG_INFO
#define	LOG_ERR stderr
#define	LOG_INFO stderr
#else
int	debug = 0;
#endif
#define	printd	if (debug) printf

int	getline __P((char *, int, int));
void	usage __P((void));
void	set_disc __P((int, int));

void	sigalrm();	/* XXX signal prototype */
void	sighup();	/* XXX signal prototype */

#if defined(_BSDI_VERSION) && _BSDI_VERSION >= 199312
#define HAS_SETPROCTITLE
#endif

#ifndef HAS_SETPROCTITLE
void setproctitle() { /* empty */; }
#endif

int
main(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	extern int optind;
	int n, ch, pid, unit, gotlogin;
	int fd = -1, first = 1, tries = 0, pausefirst = 0;
	struct termios t;
	FILE *wfd = NULL, *pfd;
	char *dialerstring = 0, buf[BUFSIZ], cmdbuf[BUFSIZ];

	while ((ch = getopt(argc, argv, "b:df:p:s:")) != EOF)
		switch (ch) {
		case 'b':
			speed = atoi(optarg);
			break;
		case 'd':
			debug = 1;
			break;
		case 'f':
			pidfile = optarg;
			break;
		case 'p':
			pausefirst = atoi(optarg);
			break;
		case 's':
			dialerstring = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 3)
		usage();

#if BSD <= 43
	if (debug == 0 && (fd = open("/dev/tty", 0)) >= 0) {
		ioctl(fd, TIOCNOTTY, 0);
		close(fd);
		fd = -1;
	}
#endif

	/* initialize globals from command line arguments */
	device = argv[0];
	login = argv[1];
	passwd = argv[2];

	setproctitle("starting %s", device);

	if (debug)
		setbuf(stdout, NULL);
	else
		openlog("startslip", LOG_PID, LOG_DAEMON);
	
	if (pfd = fopen(pidfile, "r")) {
		pid = 0;
		fscanf(pfd, "%d", &pid);
		if (pid > 0)
			kill(pid, SIGTERM);
		fclose(pfd);
	}
	(void)signal(SIGALRM, sigalrm);

restart:
	logged_in = 0;
	if (++tries > MAXTRIES) {
		syslog(LOG_ERR, "exiting after %d tries\n", tries);
		exit(1);
	}

	/*
	 * We may get a HUP below, when the parent (session leader/
	 * controlling process) exits; ignore HUP until into new session.
	 */
	signal(SIGHUP, SIG_IGN);
	hup = 0;
	if (fork() > 0) {
		if (pausefirst)
			sleep(pausefirst);
		printd("parent exiting\n");
		exit(0);
	}
	pausefirst = 0;

	if (setsid() == -1)
		perror("setsid");

	pid = getpid();
	printd("restart: pid %d: ", pid);
	if ((pfd = fopen(pidfile, "w")) != NULL) {
		printd("pidfile(%s,%d), ", pidfile, pid);
		fprintf(pfd, "%d\n", pid);
		fclose(pfd);
	}
	if (wfd) {
		printd("fclose(wfd), ");
		fclose(wfd);
		wfd = NULL;
	}
	if (fd >= 0) {
		printd("close(fd), ");
		close(fd);
		sleep(5);
	}
	printd("open(%s)=", device);
	if ((fd = open(device, O_RDWR|O_NONBLOCK)) < 0) {
		perror(device);
		syslog(LOG_ERR, "open %s: %m\n", device);
		/*
		 * if the first open fails then give up, else retry
		 */
		if (first)
			exit(1);
		sleep(wait_time * tries);
		goto restart;
	}
	printd("%d", fd);

#ifdef	TIOCSCTTY
	/* set `fd' to be the controlling tty (so we get SIGHUP) */
	if (ioctl(fd, TIOCSCTTY, 0) < 0)
		perror("ioctl (TIOCSCTTY)");
#endif
	signal(SIGHUP, sighup);

	/*
	 * setup the tty for dialing (TTYDISC)
	 */
	set_disc(fd, TTYDISC);
	if (tcgetattr(fd, &t) < 0) {
		perror("tcgetattr");
		syslog(LOG_ERR, "%s: tcgetattr: %m\n", device);
	        exit(2);
	}
	cfmakeraw(&t);
	t.c_iflag &= ~IMAXBEL;
	t.c_cflag |= CRTSCTS|CLOCAL;	/* Need CLOCAL until we have DCD */
	cfsetspeed(&t, speed);
	if (tcsetattr(fd, TCSAFLUSH, &t) < 0) {
		perror("tcsetattr");
		syslog(LOG_ERR, "%s: tcsetattr: %m\n", device);
		/*
		 * give up if the first try fails, else try again
		 */
	        if (first) 
			exit(2);
		sleep(wait_time * tries);
		goto restart;
	}
	/* clear O_NONBLOCK */
	if (fcntl(fd, F_SETFL, 0) < 0) {
		perror("F_SETFL");
		syslog(LOG_ERR, "%s: fcntl(F_SETFL): %m\n", device);
		if (first)
			exit(2);
	}       

	sleep(2);		/* wait for flakey line to settle */
	if (hup)
		goto restart;

	if ((wfd = fdopen(fd, "w+")) == NULL) {
		syslog(LOG_ERR, "can't fdopen slip line\n");
		exit(10);
	}
	/* make `wfd' unbuffered for dialing */
	setbuf(wfd, (char *)0);

	setproctitle("dialing %s", device);

	if (dialerstring) {
		printd(", send dialerstring(%s)", dialerstring);
		fprintf(wfd, "%s\r", dialerstring);
	} else
		putc('\r', wfd);
	printd("\n");

	/*
	 * Log in
	 */
	printd("looking for ``login:'' prompt\n");
	for (gotlogin = 0;;) {
		if ((n = getline(buf, BUFSIZ, fd)) == 0 || hup) {
			sleep(wait_time * tries);
			goto restart;
		}
	        if (n >= 6 && bcmp(&buf[n - 5], "ogin:", 5) == 0) {
			/*
			 * Login prompt means carrier is on, so we can
			 * clear CLOCAL to enable SIGHUP's
			 */
			if (!gotlogin) {
				t.c_cflag &= ~CLOCAL;
				if (tcsetattr(fd, TCSANOW, &t) < 0) {
					perror("tcsetattr");
					syslog(LOG_ERR,
					    "%s: tcsetattr: %m\n", device);
					if (first) 
						exit(2);
					sleep(wait_time * tries);
					goto restart;
				}
			}
			gotlogin = 1;
			fprintf(wfd, "%s\r", login);
			printd("Sent login: %s\n", login);
	                continue;
	        }
	        if (n >= 9 && bcmp(&buf[n - 8], "assword:", 8) == 0) {
			if (!gotlogin)
				syslog(LOG_INFO,
				    "%s: got passwd prompt before login\n",
				    device);
			fprintf(wfd, "%s\r", passwd);
			printd("Sent password: %s\n", passwd);
	                break;
	        }
	}

	/*
	 * Attach
	 */
	printd("Attaching SLIPDISC\n");
	set_disc(fd, SLIPDISC);
	/* find out what unit number we were assigned */
	if (ioctl(fd, SLIOCGUNIT, (caddr_t)&unit) < 0) {
		printd("SLIPDISC failed\n");
		syslog(LOG_ERR, "%s: ioctl (SLIOCGUNIT): %m", device);
		exit(1);
	}

	if (first && debug == 0) {
		close(0);
		close(1);
		close(2);
		(void) open("/dev/null", O_RDWR);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
	}
	(void) sprintf(cmdbuf, "ifconfig sl%d up", unit);
	printd("exec'ing: %s\n", cmdbuf);
	(void) system(cmdbuf);
	if (!first) {
		printd("SLIP reconnected on %s (%d tries).\n", device, tries);
		syslog(LOG_INFO, "SLIP reconnected on %s (%d tries).\n", device, tries);
	} else
		printd("SLIP ready\n");
	first = 0;
	tries = 0;
	logged_in = 1;
	setproctitle("connected %s", device);
	while (hup == 0) {
		sigpause(0L);
		printd("sigpause return\n");
	}
	setproctitle("restarting %s", device);
	goto restart;
}

void
sighup()
{

	printd("hup\n");
	if (hup == 0 && logged_in)
		syslog(LOG_INFO, "hangup signal\n");
	hup = 1;
}

void
sigalrm()
{

	printd("alarm\n");
	if (dojump)
		longjmp(abortread, 1);
}

int
getline(buf, size, fd)
	char *buf;
	int size, fd;
{
	register int i;
	int ret;

	size--;
	dojump = 0;
	alarm(READ_TIMEOUT);
	for (i = 0; i < size; i++) {
		if (hup) {
			alarm(0);
			dojump = 0;
			return (0);
		}
		if (setjmp(abortread)) {
			dojump = 0;
			return (0);
		}
		dojump = 1;
		ret = read(fd, &buf[i], 1);
		dojump = 0;
	        if (ret == 1) {
	                buf[i] &= 0177;
	                if (buf[i] == '\r' || buf[i] == '\0')
	                        buf[i] = '\n';
	                if (buf[i] != '\n' && buf[i] != ':')
	                        continue;
			alarm(0);
	                buf[i + 1] = '\0';
			printd("Got %d: \"%s\"\n", i + 1, buf);
	                return (i+1);
	        }
		if (ret <= 0) {
			alarm(0);
			if (ret < 0)
				syslog(LOG_ERR, "%s: read: %m", device);
			else
				syslog(LOG_ERR, "%s: read returned 0", device);
			buf[i] = '\0';
			printd("returning 0 after %d: \"%s\"\n", i, buf);
			return (0);
		}
	}
	alarm(0);
	return (0);
}

void
set_disc(fd, disc)
	int fd;
	int disc;
{
	if (ioctl(fd, TIOCSETD, &disc) < 0) {
		perror("ioctl(TIOCSETD)");
		syslog(LOG_ERR, "%s: ioctl (TIOCSETD %d): %m\n",
		device, disc);
		exit(1);
	}
}

void
usage()
{
	fprintf(stderr, "usage: startslip [-b baud] [-d] [-f pidfile] [-p pause] [-s string] device user passwd\n");
	exit(1);
}
