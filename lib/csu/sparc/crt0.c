/*	BSDI crt0.c,v 2.1 1995/02/03 06:11:54 polk Exp	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
static char sccsid[] = "@(#)crt0.c	8.1 (Berkeley) 6/1/93";
#endif /* not lint */

/*
 *	C start up routine.
 */

#include <sys/param.h>
#include <sys/exec.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char **environ;
char *__progname;		/* keep this in BSS, for Emacs' sake */
struct ps_strings *__ps_strings;

#ifdef MCRT0
extern char etext[];
extern char eprol[] asm("eprol");
extern void __moncleanup(void);
extern void __monstartup(char *, char *);
#endif

/*
 * The program is entered at `start' with:
 *
 *	<kernel space>
 *	+============+
 *	| ps_strings | <-- %g2
 *	+------------+
 *	| reserved   | (signal trampoline code & similar goodies)
 *	+------------+
 *	| argument   |
 *	| strings    |
 *	+------------+
 *	| envp[N]    |
 *	|   ...      |
 *	| envp[0]    |
 *	| argv[argc] |
 *	|   ...      |
 *	| argv[0]    |
 *	| argc       |
 *	+------------+
 *	| 64 bytes   | (for register window)
 *	+------------+ <-- %sp
 *
 * In older systems, %g2 is NULL rather than pointing to the ps_strings
 * data; in that case we use an address compiled into setproctitle().
 */
struct kframe {
	int	regarea[16];	/* space for %i and %o variables */
	int	kargc;		/* argument count */
	char	*kargv[1];	/* actual size depends on kargc */
};

extern volatile void start() asm("start0");
extern int main(int, char **, char **);

volatile void
start(void)
{
	register struct kframe *sp asm("%sp");
	register struct ps_strings *psp asm("%g2");
	register int argc;
	register char **argv, **envp;
	char *p, *s;

asm(".globl start");
asm("start:");
	argc = sp->kargc;
	argv = &sp->kargv[0];
	environ = envp = &argv[argc + 1];
	sp = (struct kframe *)((int)sp - 16);
	__ps_strings = psp;
asm("eprol:");

	/* Set up program name for err(3). */
	if ((p = *argv) != NULL) {
		s = strrchr(p, '/');
		__progname = s ? s + 1 : p;
	} else
		__progname = "";

#ifdef paranoid
	/*
	 * The standard I/O library assumes that file descriptors 0, 1, and 2
	 * are open. If one of these descriptors is closed prior to the start 
	 * of the process, I/O gets very confused. To avoid this problem, we
	 * insure that the first three file descriptors are open before calling
	 * main(). Normally this is undefined, as it adds two unnecessary
	 * system calls.
	 */
    {
	register int fd;
	do {
		fd = open("/dev/null", 2);
	} while (fd >= 0 && fd < 3);
	close(fd);
    }
#endif

#ifdef MCRT0
	/* Begin profiling, and arrange to write out the data on exit. */
	__monstartup(eprol, etext);
	atexit(__moncleanup);
#endif
	errno = 0;
	exit(main(argc, argv, envp));
}
