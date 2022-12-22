/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_stathz.c,v 2.1 1995/02/03 08:34:28 polk Exp
 */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <db.h>
#include <kvm.h>

#include "kvm_private.h"
#include "kvm_stat.h"

/*
 * Read stathz.  Statistical samples are taken at this rate.
 */
int
kvm_stathz(kd)
	kvm_t *kd;
{
	u_long addr;
	int hz, mib[2];
	struct clockinfo ci;

	if (ISALIVE(kd)) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_CLOCKRATE;
		if (_kvm_sys2(kd, mib, "KERN_CLOCKRATE", &ci, sizeof ci) < 0)
			return (-1);
		/* clkinfo.stathz = stathz ? stathz : hz; */
		return (ci.stathz);
	}
	/* try stathz first, then hz if stathz == 0 */
	if ((addr = _kvm_symbol(kd, "_stathz")) == 0 ||
	    kvm_read(kd, addr, &hz, sizeof hz) != sizeof hz)
		return (-1);
	if (hz == 0)
		if ((addr = _kvm_symbol(kd, "_hz")) == 0 ||
		    kvm_read(kd, addr, &hz, sizeof hz) != sizeof hz)
			return (-1);
	return (hz);
}
