/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	kvm_i386.c,v 2.1 1995/02/03 08:33:29 polk Exp
 *
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering group at Lawrence Berkeley Laboratory.
 *
 * 4. The name of the Laboratory may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)kvm_hp300.c	5.23 (Berkeley) 4/3/92";
#endif /* LIBC_SCCS and not lint */

/*
 * Intel 386/486 kvm routines.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <nlist.h>
#include <kvm.h>

#include <vm/vm.h>
#include <vm/vm_param.h>

#include <limits.h>
#include <db.h>

#include "kvm_private.h"

#include <machine/pmap.h>

#include <i386/isa/isa.h>

struct vmstate {
	u_long kernbase;
	struct pde *PTD;
};

static unsigned long basemem = IOM_BEGIN;

#define KREAD(kd, addr, p)\
	(kvm_read(kd, addr, (char *)(p), sizeof(*(p))) != sizeof(*(p)))

void
_kvm_freevtop(kd)
	kvm_t *kd;
{
	if (kd->vmst != 0)
		free(kd->vmst);
}

int
_kvm_initvtop(kd)
	kvm_t *kd;
{
	struct vmstate *vm;
	struct nlist nlist[4];

	vm = (struct vmstate *)_kvm_malloc(kd, sizeof(*vm));
	if (vm == 0)
		return (-1);
	kd->vmst = vm;

	nlist[0].n_name = "SYSTEM";
	nlist[1].n_name = "_IdlePTD";
	nlist[2].n_name = "_basemem";
	nlist[3].n_name = 0;

	switch (kvm_nlist(kd, nlist)) {
	case 0:
		break;
	case 1:
		if (nlist[2].n_type == 0)
			break;
	default:
		_kvm_err(kd, kd->program, "bad namelist");
		return (-1);
	}
	vm->kernbase = nlist[0].n_value & ~PDROFSET;
	vm->PTD = 0;
	if (nlist[2].n_type && KREAD(kd, (u_long)nlist[2].n_value, &basemem))
		basemem = IOM_BEGIN;
	if (KREAD(kd, (u_long)nlist[1].n_value, &vm->PTD)) {
		_kvm_err(kd, kd->program, "cannot read page directory table");
		return (-1);
	}
	return (0);
}

static int
_kvm_vatop(kd, pda, va, pa)
	kvm_t *kd;
	struct pde *pda;
	u_long va;
	u_long *pa;
{
	register struct vmstate *vm;
	register u_long addr;
	int p, pde, pte;
	int offset;

	vm = kd->vmst;
	/*
	 * If processing a post-mortem and we are initializing
	 * (kernel segment table pointer not yet set) then return
	 * pa == va - kernbase to avoid infinite recursion.
	 */
	if (!ISALIVE(kd) && vm->PTD == 0) {
		*pa = va - vm->kernbase;
		if (*pa >= basemem)
			*pa += IOM_END - basemem;
		return (NBPG - (va & PGOFSET));
	}
	addr = (u_long)&pda[va >> PD_SHIFT];
	/*
	 * Can't use KREAD to read kernel pdir entries.
	 * Fortunately it is 1-to-1 mapped so we don't have to. 
	 * XXX not all of the kernel is 1-to-1 mapped...
	 */
	if (!ISALIVE(kd) && pda == vm->PTD) {
		if (lseek(kd->pmfd, (off_t)addr, 0) == -1 ||
		    read(kd->pmfd, (char *)&pde, sizeof(pde)) < 0)
			goto invalid;
	} else if (KREAD(kd, addr, &pde))
		goto invalid;
	if ((pde & PG_V) == 0) {
		_kvm_err(kd, 0, "invalid page table page (%x)", pde);
		return((off_t)0);
	}
	p = i386_btop(va & PT_MASK);
	addr = (pde & PG_FRAME) + (p * sizeof(struct pte));

	/*
	 * Address from PDE is a physical address so don't use kvm_read.
	 */
	if (lseek(kd->pmfd, (off_t)addr, 0) == -1 || 
	    read(kd->pmfd, (char *)&pte, sizeof(pte)) < 0)
		goto invalid;
	addr = pte & PG_FRAME;
	if ((pte & PG_V) == 0) {
		_kvm_err(kd, 0, "page not valid");
		return (0);
	}
	offset = va & PGOFSET;
	*pa = addr + offset;
	
	return (NBPG - offset);
invalid:
	_kvm_err(kd, 0, "invalid address (%x)", va);
	return (0);
}

int
_kvm_kvatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	struct vmstate *vm = kd->vmst;

	if (vm->kernbase && va >= vm->kernbase)
		return (_kvm_vatop(kd, (u_long)kd->vmst->PTD, va, pa));
	_kvm_err(kd, 0, "invalid address (%x)", va);
	return (0);
}

/*
 * Translate a user virtual address to a physical address.
 */
int
_kvm_uvatop(kd, p, va, pa)
	kvm_t *kd;
	const struct proc *p;
	u_long va;
	u_long *pa;
{
	register struct vmspace *vms = p->p_vmspace;
	int kva;

	kva = (int)&vms->vm_pmap.pm_pdir;
	if (kvm_read(kd, kva, (char *)&kva, 4) != 4) {
		_kvm_err(kd, 0, "invalid address (%x)", va);
		return (0);
	}
	return (_kvm_vatop(kd, kva, va, pa));
}
