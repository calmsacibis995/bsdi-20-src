/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_vmtotal.c,v 2.1 1995/02/03 08:34:47 polk Exp
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/sysctl.h>

#include <vm/vm.h>

#include <db.h>
#include <kvm.h>

#include "kvm_private.h"
#include "kvm_stat.h"

/*
 * Read vmtotal (VM_TOTAL, once accidentally called VM_METER) statistics.
 */
int
kvm_vmtotal(kd, v)
	kvm_t *kd;
	struct vmtotal *v;
{
	u_long addr;
	int mib[2];

	if (ISALIVE(kd)) {
		mib[0] = CTL_VM;
		mib[1] = VM_TOTAL;
		return (_kvm_sys2(kd, mib, "VM_TOTAL", v, sizeof *v));
	}

	/*
	 * For now, just fill in with 0 so any program to
	 * show both vmmeter and vmtotal stats "works"		XXX
	 */
	_kvm_err(kd, 0, "cannot compute vmtotal for dead kernel");
	memset(v, 0, sizeof *v);
	return (0);
}
