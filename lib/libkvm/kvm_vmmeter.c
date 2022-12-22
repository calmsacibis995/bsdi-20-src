/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_vmmeter.c,v 2.1 1995/02/03 08:34:42 polk Exp
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
 * Read vmmeter (VM_CNT) statistics.
 */
int
kvm_vmmeter(kd, v)
	kvm_t *kd;
	struct vmmeter *v;
{
	u_long addr;
	int mib[2];

	if (ISALIVE(kd)) {
		mib[0] = CTL_VM;
		mib[1] = VM_CNT;
		return (_kvm_sys2(kd, mib, "VM_CNT", v, sizeof *v));
	}
	if ((addr = _kvm_symbol(kd, "_cnt")) == 0 ||
	    kvm_read(kd, addr, v, sizeof *v) != sizeof *v)
		return (-1);
	return (0);
}
