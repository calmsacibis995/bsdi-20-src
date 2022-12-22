/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_cpustats.c,v 2.1 1995/02/03 08:32:45 polk Exp
 */

#include <sys/param.h>
#include <sys/cpustats.h>
#include <sys/sysctl.h>

#include <db.h>
#include <kvm.h>

#include "kvm_private.h"
#include "kvm_stat.h"

/*
 * Read cpu-usage statistics.
 */
int
kvm_cpustats(kd, cp)
	kvm_t *kd;
	struct cpustats *cp;
{
	u_long addr;
	int mib[2];

	if (ISALIVE(kd)) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_CPUSTATS;
		return (_kvm_sys2(kd, mib, "KERN_CPUSTATS", cp, sizeof *cp));
	}
	if ((addr = _kvm_symbol(kd, "_cpustats")) == 0 ||
	    kvm_read(kd, addr, cp, sizeof *cp) != sizeof *cp)
		return (-1);
	return (0);
}
