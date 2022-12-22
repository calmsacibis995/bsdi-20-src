/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI kvm_vnodes.c,v 2.1 1995/02/03 08:34:52 polk Exp
 */

/*-
 * Copyright (c) 1980, 1991, 1993
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

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>
#define KERNEL
#include <sys/mount.h>
#undef KERNEL

/* should be in <sys/vnode.h> or <sys/sysctl.h>		XXX */
struct e_vnode {
	struct vnode *avnode;
	struct vnode vnode;
};

#include <db.h>
#include <kvm.h>
#include <stdlib.h>
#include <string.h>

#include "kvm_private.h"
#include "kvm_stat.h"

/*
 * Return the (externalized) vnode table.
 */
struct e_vnode *
kvm_vnodes(kd, avnodes)
	kvm_t *kd;
	int *avnodes;
{
	struct mntlist mountlist;
	struct mount *mp, mount;
	struct vnode *vp, vnode;
	char *vbuf, *evbuf, *bp;
	int num, numvnodes;
	u_long addr;
	size_t size;
	int mib[2];

#define VPTRSZ  sizeof(struct vnode *)
#define VNODESZ sizeof(struct vnode)
#define	KGET(addr, var) (kvm_read(kd, addr, &var, sizeof var) != sizeof var)

	if (ISALIVE(kd)) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_VNODE;
		vbuf = _kvm_s2alloc(kd, mib, "KERN_VNODE",
		    &size, sizeof(struct e_vnode));
		if (vbuf == NULL)
			return (NULL);
		*avnodes = size;
		return ((struct e_vnode *)vbuf);
	}

	/*
	 * Simulate the sysctl.
	 */
	if ((addr = _kvm_symbol(kd, "_numvnodes")) == 0 ||
	    KGET(addr, numvnodes) ||
	    (addr = _kvm_symbol(kd, "_mountlist")) == 0 ||
	    KGET(addr, mountlist))
		return (NULL);

	/*
	 * The kernel is dead so the list is static, but maybe numvnodes
	 * is a bit off... add 20 for slop.
	 */
	size = (numvnodes + 20) * (VPTRSZ + VNODESZ);
	if ((vbuf = malloc(size)) == NULL) {
		_kvm_syserr(kd, kd->program, "malloc(%lu)", (u_long)size);
		return (NULL);
	}
	bp = vbuf;
	evbuf = vbuf + size;
	num = 0;
	for (mp = mountlist.tqh_first; mp != NULL;
	    mp = mount.mnt_list.tqe_next) {
		if (KGET((u_long)mp, mount)) {
			free(vbuf);
			return (NULL);
		}
		for (vp = mount.mnt_vnodelist.lh_first; vp != NULL;
		    vp = vnode.v_mntvnodes.le_next) {
			if (KGET((u_long)vp, vnode)) {
				free(vbuf);
				return (NULL);
			}
			if ((bp + VPTRSZ + VNODESZ) > evbuf) {
				/* XXX - should realloc */
				_kvm_err(kd, kd->program,
				    "no more room for vnodes");
				free(vbuf);
				return (NULL);
			}
			memmove(bp, &vp, VPTRSZ);
			bp += VPTRSZ;
			memmove(bp, &vnode, VNODESZ);
			bp += VNODESZ;
			num++;
		}
	}
	*avnodes = num;
	return ((struct e_vnode *)vbuf);
}
