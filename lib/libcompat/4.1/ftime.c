/*	BSDI ftime.c,v 2.1 1995/02/03 06:56:52 polk Exp	*/

/*-
 * Copyright (c) 1980, 1991 The Regents of the University of California.
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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <time.h>

int
ftime(tbp)
        struct timeb *tbp;
{
        struct timeval t;
        struct tm *tm;

        if (gettimeofday(&t, NULL) < 0)
                return (-1);
        tbp->millitm = t.tv_usec / 1000;
        tbp->time = t.tv_sec;

        tm = localtime(&t.tv_sec);

        /* convert from seconds east of UTC to minutes west */
        tbp->timezone = -(tm->tm_gmtoff / 60);
        if (tbp->timezone < 0)
        	tbp->timezone += 24*60;

        /* some trickery to get old dstflag semantics */
        tbp->dstflag = tm->tm_isdst;
        if (tm->tm_isdst == 0) {
        	/* see if daylight savings applies at summer solstice */
        	tm->tm_isdst = -1;	/* force lookup */
        	tm->tm_mon = 5;		/* June */
        	tm->tm_mday = 22;
        	t.tv_sec = mktime(tm);
        	tm = localtime(&t.tv_sec);
        	tbp->dstflag = tm->tm_isdst;
        }

	return (0);
}
