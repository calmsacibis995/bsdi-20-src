/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_util.c,v 2.1 1995/02/03 08:34:39 polk Exp
 */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <db.h>
#include <kvm.h>
#include <stdlib.h>

#include "kvm_private.h"
#include "kvm_stat.h"

/*
 * Execute a two-level sysctl (for live-kernel kvm stat functions).
 */
int
_kvm_sys2(kd, mib, l2name, addr, size)
	kvm_t *kd;
	int *mib;
	char *l2name;
	void *addr;
	size_t size;
{
	size_t s;

	s = size;
	if (sysctl(mib, 2, addr, &s, NULL, 0) < 0) {
		_kvm_syserr(kd, kd->program, "sysctl(%s)", l2name);
		return (-1);
	}
	if (s != size) {
		_kvm_err(kd, kd->program, "sysctl(%s) wrong size", l2name);
		return (-1);
	}
	return (0);
}

/*
 * Execute a 2-level sysctl that needs to malloc space.  The space
 * must be in units of `units'.  The number of such units is stored
 * through `np'.
 */
void *
_kvm_s2alloc(kd, mib, l2name, np, units)
	kvm_t *kd;
	int *mib;
	char *l2name;
	size_t *np, units;
{
	size_t s;
	void *p;

	if (sysctl(mib, 2, NULL, &s, NULL, 0) < 0) {
		_kvm_syserr(kd, kd->program, "sysctl(%s)", l2name);
		return (NULL);
	}
	if (s % units) {
		_kvm_err(kd, kd->program, "sysctl(%s) wrong size (%lu % %lu)",
		    l2name, (u_long)s, (u_long)units);
		return (NULL);
	}
	if ((p = malloc(s)) == NULL) {
		_kvm_syserr(kd, kd->program, "%s: malloc(%lu)",
		    l2name, (u_long)s);
		return (NULL);
	}
	if (sysctl(mib, 2, p, &s, NULL, 0) < 0) {
		_kvm_syserr(kd, kd->program, "sysctl(%s, %lu)",
		    l2name, (u_long)s);
		free(p);
		return (NULL);
	}
	*np = s / units;
	return (p);
}

/*
 * Read a single symbol (for dead-kernel kvm stat functions).
 * Returns an error if not found.
 */
u_long
_kvm_symbol(kd, sym)
	kvm_t *kd;
	char *sym;
{
	int r;
	struct nlist nl[2];

	nl[0].n_name = sym;
	nl[1].n_name = NULL;
	r = kvm_nlist(kd, nl);
	if (r) {
		if (r > 0)
			_kvm_err(kd, kd->program, "%s: not found",
			    *sym == '_' ? sym + 1 : sym);
		return (0);
	}
	return (nl[0].n_value);
}
