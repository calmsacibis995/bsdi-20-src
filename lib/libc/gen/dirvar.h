/*	BSDI dirvar.h,v 2.1 1995/02/03 06:20:49 polk Exp	*/

/*
 * Copyright (c) 1983, 1993
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

#define	DIRBUFSIZE		CLBYTES	/* Must be a multiple of DIRBLKSIZ */
#define	DIRBUFSHIFT		CLSHIFT

/*
 * Historically, telldir() returned a value of type long that
 * just happened to be the seek offset of a directory entry.
 * In our brave new world of remote filesystems, however,
 * we don't necessarily know the seek offset of every directory entry.
 * Instead, we know the seek offset of a particular directory entry,
 * and the directory buffer offset from that entry to the one we want.
 * Since the seek offset is 64 bits and the directory buffer offset is
 * 12 bits or so, we can't represent the position of a directory entry
 * in an arithmetic value without losing some precision.
 * Thus we pack the low order bits of the seek offset and the directory
 * buffer offset into a 'cookie', which lets us cover directories up
 * a half megabyte or so in size, and when the cookie isn't sufficient
 * to represent the full offset, we pass an index to a structure that
 * contains the full information, marking it with the high order bit.
 * Unfortunately, memory allocated in this instance is leaked, as there
 * is no programmatic interface to indicate when the structure will no
 * longer be needed.
 */

#define	DD_NOT_COOKIE		(1L << (sizeof (long) * 8 - 1))
#define	DD_SEEK_SIZE		(1L << (sizeof (long) * 8 - 1 - DIRBUFSHIFT))
#define	DD_SEEK(x)		((x) >> DIRBUFSHIFT)
#define	DD_LOC(x)		((x) & (DIRBUFSIZE - 1))
#define	DD_MAKE_COOKIE(x,y)	(((long)(x) << DIRBUFSHIFT) | (y))
#define	DD_COOKIE_OKAY(x,y)	((x) >= 0 && (x) < DD_SEEK_SIZE && \
				 (y) >= 0 && (y) < DIRBUFSIZE)

struct ddloc {
	struct	ddloc *loc_next;/* next structure in list */
	long	loc_index;	/* key associated with structure */
	long	loc_seek;	/* magic cookie returned by getdirentries */
	long	loc_loc;	/* offset of entry in buffer */
};

#define	DD_HASH_SIZE		32	/* Must be a power of 2 */
#define	DD_HASH(x)		((x) & (DD_HASH_SIZE - 1))

extern struct ddloc *_dd_hash[DD_HASH_SIZE];
