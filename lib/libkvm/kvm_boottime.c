/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_boottime.c,v 2.1 1995/02/03 08:32:40 polk Exp
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/sysctl.h>

#include <db.h>
#include <kvm.h>

#include "kvm_private.h"
#include "kvm_stat.h"

/*
 * Read boottime.
 */
int
kvm_boottime(kd, tp)
	kvm_t *kd;
	struct timeval *tp;
{
	u_long addr;
	int mib[2];

	if (ISALIVE(kd)) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_BOOTTIME;
		return (_kvm_sys2(kd, mib, "KERN_BOOTTIME", tp, sizeof *tp));
	}
	if ((addr = _kvm_symbol(kd, "_boottime")) == 0 ||
	    kvm_read(kd, addr, tp, sizeof *tp) != sizeof *tp)
		return (-1);
	return (0);
}
