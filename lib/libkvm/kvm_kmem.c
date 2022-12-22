/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_kmem.c,v 2.1 1995/02/03 08:33:34 polk Exp
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <db.h>
#include <kvm.h>

#include "kvm_private.h"
#include "kvm_stat.h"

/*
 * Find size of kmem bucket and usage statistics.  (If we cannot
 * find them from the running system, we will use compiled-in defaults.)
 */
int
kvm_kmemsize(kd, nbp, nsp)
	kvm_t *kd;
	int *nbp, *nsp;
{
	int mib[2];
	size_t size;

	if (ISALIVE(kd)) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_KMEMBUCKETS;
		size = 0;
		if (sysctl(mib, 2, NULL, &size, NULL, 0) >= 0)
			*nbp = size / sizeof(struct kmembuckets);
		else
			*nbp = MINBUCKET + 16;
		mib[1] = KERN_KMEMSTATS;
		size = 0;
		if (sysctl(mib, 2, NULL, &size, NULL, 0) >= 0)
			*nsp = size / sizeof(struct kmemstats);
		else
			*nsp = M_LAST;
	} else {
		*nbp = MINBUCKET + 16;
		*nsp = M_LAST;
	}
	return (0);
}

/*
 * Read kmem buckets and usage statistics.
 */
int
kvm_kmem(kd, bp, nb, sp, ns)
	kvm_t *kd;
	struct kmembuckets *bp;
	int nb;
	struct kmemstats *sp;
	int ns;
{
	u_long addr;
	int mib[2];

	if (ISALIVE(kd)) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_KMEMBUCKETS;
		if (_kvm_sys2(kd, mib, "KERN_KMEMBUCKETS",
		    bp, nb * sizeof *bp) < 0)
			return (-1);
		mib[1] = KERN_KMEMSTATS;
		return (_kvm_sys2(kd, mib, "KERN_KMEMSTATS",
		    sp, ns * sizeof *sp));
	}
	if ((addr = _kvm_symbol(kd, "_bucket")) == 0 ||
	    kvm_read(kd, addr, bp, nb * sizeof *bp) != nb * sizeof *bp)
		return (-1);
	if ((addr = _kvm_symbol(kd, "_kmemstats")) == 0 ||
	    kvm_read(kd, addr, sp, ns * sizeof *sp) != ns * sizeof *sp)
		return (-1);
	return (0);
}
