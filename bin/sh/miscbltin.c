/*	BSDI miscbltin.c,v 2.1 1995/02/03 05:49:20 polk Exp	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
static char sccsid[] = "@(#)miscbltin.c	8.2 (Berkeley) 4/16/94";
#endif /* not lint */

/*
 * Miscelaneous builtins.
 */

#include "shell.h"
#include "options.h"
#include "var.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"

#undef eflag

extern char **argptr;		/* argument list for builtin command */


/*
 * The read builtin.  The -e option causes backslashes to escape the
 * following character.
 *
 * This uses unbuffered input, which may be avoidable in some cases.
 */

readcmd(argc, argv)  char **argv; {
	char **ap;
	int backslash;
	char c;
	int eflag;
	char *prompt;
	char *ifs;
	char *p;
	int startword;
	int status;
	int i;

	eflag = 0;
	prompt = NULL;
	while ((i = nextopt("ep:")) != '\0') {
		if (i == 'p')
			prompt = optarg;
		else
			eflag = 1;
	}
	if (prompt && isatty(0)) {
		out2str(prompt);
		flushall();
	}
	if (*(ap = argptr) == NULL)
		error("arg count");
	if ((ifs = bltinlookup("IFS", 1)) == NULL)
		ifs = nullstr;
	status = 0;
	startword = 1;
	backslash = 0;
	STARTSTACKSTR(p);
	for (;;) {
		if (read(0, &c, 1) != 1) {
			status = 1;
			break;
		}
		if (c == '\0')
			continue;
		if (backslash) {
			backslash = 0;
			if (c != '\n')
				STPUTC(c, p);
			continue;
		}
		if (eflag && c == '\\') {
			backslash++;
			continue;
		}
		if (c == '\n')
			break;
		if (startword && *ifs == ' ' && strchr(ifs, c)) {
			continue;
		}
		startword = 0;
		if (backslash && c == '\\') {
			if (read(0, &c, 1) != 1) {
				status = 1;
				break;
			}
			STPUTC(c, p);
		} else if (ap[1] != NULL && strchr(ifs, c) != NULL) {
			STACKSTRNUL(p);
			setvar(*ap, stackblock(), vexport);
			ap++;
			startword = 1;
			STARTSTACKSTR(p);
		} else {
			STPUTC(c, p);
		}
	}
	STACKSTRNUL(p);
	setvar(*ap, stackblock(), vexport);
	while (*++ap != NULL)
		setvar(*ap, nullstr, vexport);
	return status;
}



umaskcmd(argc, argv)  char **argv; {
	int mask;
	char *p;
	int i;

	if ((p = argv[1]) == NULL) {
		INTOFF;
		mask = umask(0);
		umask(mask);
		INTON;
		out1fmt("%.4o\n", mask);	/* %#o might be better */
	} else {
		mask = 0;
		do {
			if ((unsigned)(i = *p - '0') >= 8)
				error("Illegal number: %s", argv[1]);
			mask = (mask << 3) + i;
		} while (*++p != '\0');
		umask(mask);
	}
	return 0;
}

#ifdef LIMITS

#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdlib.h>

struct resource_tab {
	const char	*name;
	size_t		sig_bytes;
	int		resource;
	int		alias;
	enum { COUNT, SIZE, TIME }
			type;
} resource_tab[] = {
	{ "coredumpsize", 2, RLIMIT_CORE, 0, SIZE },
	{ "coresize", 2, RLIMIT_CORE, 1, SIZE },
	{ "cputime", 2, RLIMIT_CPU, 0, TIME },
	{ "datasize", 1, RLIMIT_DATA, 0, SIZE },
	{ "filesize", 1, RLIMIT_FSIZE, 0, SIZE },
	{ "fsize", 1, RLIMIT_FSIZE, 1, SIZE },
	{ "maxproc", 2, RLIMIT_NPROC, 0, COUNT },
	{ "memoryuse", 2, RLIMIT_RSS, 0, SIZE },
	{ "nofile", 2, RLIMIT_NOFILE, 1, COUNT },
	{ "nproc", 2, RLIMIT_NPROC, 1, COUNT },
	{ "openfiles", 2, RLIMIT_NOFILE, 0, COUNT },
	{ "proc", 1, RLIMIT_NPROC, 1, COUNT },
	{ "rss", 1, RLIMIT_RSS, 1, SIZE },
	{ "stacksize", 1, RLIMIT_STACK, 0, SIZE },
	{ 0 }
};

static struct resource_tab *
find_resource(arg)
	char *arg;
{
	struct resource_tab *rp;
	size_t len = strlen(arg);

	for (rp = &resource_tab[0]; rp->name; ++rp) {
		if (len > strlen(rp->name))
			continue;
		if (bcmp(arg, rp->name, len))
			continue;
		if (len < rp->sig_bytes)
			error("ambiguous resource limit prefix `%s'", arg);
		return (rp);
	}
	return (0);
}

void
print_resource(rp, hard)
	struct resource_tab *rp;
	int hard;
{
	struct rlimit rl;
	u_quad_t value;

	if (getrlimit(rp->resource, &rl) == -1)
		error("`%s' resource limit not available", rp->name);
	if (hard)
		value = rl.rlim_max;
	else
		value = rl.rlim_cur;
	if (value == RLIM_INFINITY)
		out1fmt("%-15s %s\n", rp->name, "unlimited");
	else
		switch (rp->type) {
		case COUNT:
			out1fmt("%-15s %u\n", rp->name, (unsigned long)value);
			break;
		case SIZE:
			if (value % 1024 == 0) {
				out1fmt("%-15s %u kbytes\n",
				    rp->name, (unsigned long)value / 1024);
				break;
			}
			out1fmt("%-15s %u bytes\n", rp->name, (unsigned long)value);
			break;
		case TIME:
			if (value % 3600 == 0)
				out1fmt("%-15s %u hours\n",
				    rp->name, (unsigned long)value / 3600);
			else if (value % 60 == 0)
				out1fmt("%-15s %u minutes\n",
				    rp->name, (unsigned long)value / 60);
			else
				out1fmt("%-15s %u seconds\n",
				    rp->name, (unsigned long)value);
			break;
		}
}

static void
dump_resources(hard)
	int hard;
{
	struct resource_tab *rp;

	for (rp = &resource_tab[0]; rp->name; ++rp) {
		if (rp->alias)
			continue;
		print_resource(rp, hard);
	}
}

static int
set_resource(rp, hard, argv)
	struct resource_tab *rp;
	int hard;
	char **argv;
{
	struct rlimit rl;
	char *scale_name;
	int scale = 1;
	double dvalue;
	u_quad_t value;
	int is_float = 0;
	size_t len;

	if (getrlimit(rp->resource, &rl) == -1)
		error("`%s' resource limit not available", rp->name);

	if (strcmp(argv[0], "unlimited") == 0 && argv[1] == 0) {
		if (geteuid() == 0)
			value = RLIM_INFINITY;
		else
			value = rl.rlim_max;
		if (hard)
			rl.rlim_max = value;
		else
			rl.rlim_cur = value;
		if (setrlimit(rp->resource, &rl) == -1)
			error("can't unlimit resource `%s'", rp->name);
		return (0);
	}

	value = strtol(argv[0], &scale_name, 10);
	if (*scale_name == '.') {
		dvalue = strtod(argv[0], &scale_name);
		is_float = 1;
	}
	if (argv[0] == scale_name)
		error("can't parse resource limit value `%s'", argv[0]);

	if (*scale_name == '\0' && argv[1])
		scale_name = argv[1];
	len = strlen(scale_name);

#define	prefix_match(s, cs, len) \
	(len < sizeof cs && !bcmp(s, cs, len))

	if (*scale_name)
		switch (rp->type) {
		case SIZE:
			if (prefix_match(scale_name, "mbytes", len) ||
			    prefix_match(scale_name, "megabytes", len))
				scale = 1024 * 1024;
			else if (prefix_match(scale_name, "kbytes", len) ||
			    prefix_match(scale_name, "kilobytes", len))
				scale = 1024;
			else if (prefix_match(scale_name, "bytes", len))
				scale = 1;
			else
				goto bad_scale;
			break;

		case TIME:
			if (prefix_match(scale_name, "hours", len))
				scale = 60 * 60;
			else if (prefix_match(scale_name, "minutes", len))
				scale = 60;
			else if (prefix_match(scale_name, "seconds", len))
				scale = 1;
			else
				goto bad_scale;
			break;

		default:
		bad_scale:
			error("unrecognized resource limit scale factor `%s'",
			    scale_name);
		}

	if (is_float)
		value = dvalue * scale;
	else
		value *= scale;

	if (hard)
		rl.rlim_max = value;
	else
		rl.rlim_cur = value;

	if (setrlimit(rp->resource, &rl) == -1)
		error("can't set `%s' resource limit", rp->name);

	return (0);
}

/*
 * Implement a C-shell-like resource limit command.
 */
int
limitcmd(argc, argv)
	int argc;
	char **argv;
{
	struct resource_tab *rp;
	int hard = 0;

	if (argc >= 2 && strcmp(argv[1], "-h") == 0) {
		hard = 1;
		--argc, ++argv;
	}

	if (argc == 1) {
		dump_resources(hard);
		return (0);
	}

	if (argv[1][0] == '-' || argc > 4)
		error("usage: limit [-h] [resource [maximum-use]]");

	if ((rp = find_resource(argv[1])) == 0)
		error("unknown resource limit `%s'", argv[1]);

	if (argc == 2) {
		print_resource(rp, hard);
		return (0);
	}

	return (set_resource(rp, hard, &argv[2]));
}

static char *unargv[2] = { "unlimited", 0 };

static int
unlimit_resources(hard)
	int hard;
{
	struct resource_tab *rp;
	int result = 0;

	for (rp = &resource_tab[0]; rp->name; ++rp) {
		if (rp->alias)
			continue;
		result |= set_resource(rp, hard, unargv);
	}
	return (result);
}

/*
 * Implement a C-shell-like unlimit command.
 */
int
unlimitcmd(argc, argv)
	int argc;
	char **argv;
{
	struct resource_tab *rp;
	int hard = 0;

	if (argc >= 2 && strcmp(argv[1], "-h") == 0) {
		hard = 1;
		--argc, ++argv;
	}

	if (argc == 1)
		return (unlimit_resources(hard));

	if (argv[1][0] == '-' || argc > 2)
		error("usage: unlimit [-h] [resource]");

	if ((rp = find_resource(argv[1])) == 0)
		error("unknown resource limit `%s'", argv[1]);

	return (set_resource(rp, hard, unargv));
}

#endif /* LIMITS */
