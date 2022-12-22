/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_disks.c,v 2.1 1995/02/03 08:32:49 polk Exp
 */

#include <sys/param.h>
#include <sys/diskstats.h>
#include <sys/sysctl.h>

#include <db.h>
#include <kvm.h>
#include <stdlib.h>
#include <string.h>

#include "kvm_private.h"
#include "kvm_stat.h"

/*
 * Read disk names, count drives, split names into array.
 */
char **
kvm_dknames(kd, countp)
	kvm_t *kd;
	int *countp;
{
	register int n;
	register char *p, *buf, **vec;
	size_t size, vsize;
	int mib[2];

	if (ISALIVE(kd)) {
		mib[0] = CTL_HW;
		mib[1] = HW_DISKNAMES;
		buf = _kvm_s2alloc(kd, mib, "HW_DISKNAMES", &size, sizeof *buf);
		if (buf == NULL)
			return (NULL);
		for (n = 0, p = buf; p < buf + size; p += strlen(p) + 1)
			n++;
		vsize = n * sizeof *vec;
		if ((vec = malloc(vsize)) == NULL) {
			_kvm_syserr(kd, kd->program, "malloc(%lu)",
			    (u_long)vsize);
			free(buf);
			return (NULL);
		}
		for (n = 0, p = buf; p < buf + size; p += strlen(p) + 1)
			vec[n++] = p;
		*countp = n;
		return (vec);
	}
	_kvm_err(kd, kd->program,
	    "disk stats for dead kernels not implemented");
	return (NULL);
}

/*
 * Read disk statistics.  If you use:
 *
 *	char **names = kvm_dknames(&count);
 *	struct diskstats *dkp;
 *
 *	dkp = malloc((count + 1) * sizeof *dkp);
 *	n = kvm_disks(dkp, count + 1);
 *
 * then buf[i] will be the statistics for disk names[i].  If n < count,
 * something is wrong; if n > count, a new disk appeared.
 */
int
kvm_disks(kd, dkp, n)
	kvm_t *kd;
	struct diskstats *dkp;
	int n;
{
	size_t size;
	int mib[2];

	if (ISALIVE(kd)) {
		mib[0] = CTL_HW;
		mib[1] = HW_DISKSTATS;
		size = n * sizeof *dkp;
		if (sysctl(mib, 2, dkp, &size, NULL, 0) < 0) {
			_kvm_syserr(kd, kd->program, "sysctl(HW_DISKSTATS)");
			return (-1);
		}
		return (size / sizeof *dkp);
	}
	_kvm_err(kd, kd->program,
	    "disk stats for dead kernels not implemented");
	return (-1);
}
