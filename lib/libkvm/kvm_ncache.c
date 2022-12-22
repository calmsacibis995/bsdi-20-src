/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_ncache.c,v 2.1 1995/02/03 08:33:49 polk Exp
 */

#include <sys/param.h>
#include <sys/uio.h>
#include <sys/namei.h>
#include <sys/sysctl.h>

#include <db.h>
#include <kvm.h>

#include "kvm_private.h"
#include "kvm_stat.h"

/*
 * Read name-cache-usage statistics.
 */
int
kvm_ncache(kd, np)
	kvm_t *kd;
	struct nchstats *np;
{
	u_long addr;
	int mib[2];

	if (ISALIVE(kd)) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_NCHSTATS;
		return (_kvm_sys2(kd, mib, "KERN_NCHSTATS", np, sizeof *np));
	}
	if ((addr = _kvm_symbol(kd, "_nchstats")) == 0 ||
	    kvm_read(kd, addr, np, sizeof *np) != sizeof *np)
		return (-1);
	return (0);
}
