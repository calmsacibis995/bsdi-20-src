/*	BSDI crt0.c,v 2.1 1995/02/03 06:11:39 polk Exp	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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

asm (".text; movl %ebx, ___ps_strings; jmp start");

#ifndef lint
static char sccsid[] = "@(#)crt0.c	8.1 (Berkeley) 6/1/93";
#endif /* not lint */

#include <sys/param.h>
#include <sys/exec.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
 * value of PS_STRINGS in 1.1 release: USRSTACK - sizeof(struct ps_strings)
 */
#define	PS_STRINGS1_1	(0xefc00000 - 2 * NBPG - sizeof(struct ps_strings))

char **environ;
char *__progname;			/* keep this in BSS, for Emacs' sake */
struct ps_strings *__ps_strings;	/* initialized in asm above */

#ifdef MCRT0
extern char etext[];
extern char eprol[] asm ("eprol");
extern void __moncleanup(void);
extern void __monstartup(char *, char *);
#endif

extern void start(void) asm ("start");
extern int main(int, char **, char **);

void
start()
{
	int argc;
	char *p, *s, **argv;
	struct ps_strings *psp;

	/*
	 * On current systems, %ebx points to the ps_strings structure
	 * telling us what we need.  In 1.1, %bxi is 0, and we
	 * use the compiled-in address for that system.
	 * %ebx was stored in __ps_strings by the asm above.
	 */
	if (__ps_strings == 0)
		__ps_strings = (struct ps_strings *)PS_STRINGS1_1;
	psp = __ps_strings;
	environ = psp->ps_envp;
asm("eprol:");

	/* Set up program name for err(3). */
	if ((p = *psp->ps_argv) != NULL) {
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
	(void)close(fd);
    }
#endif

#ifdef MCRT0
	/* Begin profiling, and arrange to write out the data on exit. */
	__monstartup(eprol, etext);
	atexit(__moncleanup);
#endif
	errno = 0;
	exit(main(psp->ps_argc, psp->ps_argv, psp->ps_envp));
}
