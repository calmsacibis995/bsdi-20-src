/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

static const char rcsid[] = "pppattach.c,v 2.1 1995/02/03 07:31:22 polk Exp";
static const char copyright[] = "Copyright (c) 1993 Berkeley Software Design, Inc.";

/*
 * Attach the ppp interface to the async line
 *
 * pppattach [-i interface#] [-s baud] device
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <net/pppioctl.h>
#include <paths.h>
#include <errno.h>
#include <fcntl.h>

int line;

main(ac, av)
	int ac;
	char **av;
{
	int interface;
	int baudrate;
	char *devname;
	extern char *optarg;
	extern int optind;
	int ld;
	struct termios t;
	int oldld;

	if (ac < 2) {
	usage:  fprintf(stderr, "usage: pppattach [-i interface #] [-s baud] [-l] device\n");
		exit(1);
	}

	interface = -1;
	baudrate = 0;
	for (;;) {
		switch (getopt(ac, av, "i:s:")) {
		case EOF:
			break;

		case 'i':
			interface = atoi(optarg);
			if (interface < 0)
				goto usage;
			if (interface < 0) {
				fprintf(stderr, "pppattach: bad interface #\n");
				exit(1);
			}
			continue;

		case 's':
			if (baudrate != 0)
				goto usage;
			baudrate = atoi(optarg);
			if (baudrate <= 0) {
				fprintf(stderr, "pppattach: bad baud rate\n");
				exit(1);
			}
			continue;

		default:
			goto usage;
		}
		break;
	}
	if (optind != ac - 1)
		goto usage;
	devname = av[optind];

	if (*devname != '/') {
		static char namebuf[128];

		snprintf(namebuf, sizeof namebuf, "%s/%s", _PATH_DEV, devname);
		devname = namebuf;
	}

	line = open(devname, O_RDWR | O_NDELAY);
	if (line < 0) {
		fprintf(stderr, "open: ");
		perror(devname);
		exit(1);
	}

	/*
	 * Set line parameters
	 */
	if (tcgetattr(line, &t) < 0) {
		fprintf(stderr, "tcgetattr: ");
		perror(devname);
		exit(1);
	}
	t.c_iflag &= IXON | IXOFF;
	t.c_iflag |= IGNBRK | IGNPAR;
	t.c_oflag = 0;
	t.c_cflag &= CCTS_OFLOW | CRTS_IFLOW | CLOCAL;
	t.c_cflag |= CS8 | CREAD | HUPCL;
	t.c_lflag = 0;
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	if (baudrate != 0)
		cfsetspeed(&t, baudrate);
	if (tcsetattr(line, TCSADRAIN, &t) < 0) {
		fprintf(stderr, "tcsetattr: ");
		perror(devname);
		exit(1);
	}

	/* Save line discipline */
	if (ioctl(line, TIOCGETD, &oldld) < 0) {
		fprintf(stderr, "ioctl(TIOCGETD): ");
		perror(devname);
		exit(1);
	}

	ld = PPPDISC;
	if (ioctl(line, TIOCSETD, &ld) < 0) {
		fprintf(stderr, "ioctl(TIOCSETD): ");
		perror(devname);
		exit(1);
	}

	if (ioctl(line, PPPIOCSUNIT, &interface) < 0) {
		switch (errno) {
		case ENXIO:
			fprintf(stderr, "pppattach: no interface\n");
			break;
		case EBUSY:
			fprintf(stderr, "pppattach: interface is busy\n");
			break;
		default:
			fprintf(stderr, "ioctl(PPPIOCSUNIT): ");
			perror(devname);
			break;
		}
		exit(1);
	}

	/* Wait until end of session */
	(void) ioctl(line, PPPIOCWEOS);

	/* Restore line discipline */
	(void) ioctl(line, TIOCSETD, &oldld);
	close(line);
	exit(0);
}
