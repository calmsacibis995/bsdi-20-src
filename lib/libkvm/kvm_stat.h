/*-
 * Copyright (c) 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI kvm_stat.h,v 2.1 1995/02/03 08:34:25 polk Exp
 */

struct cpustats;
struct diskstats;
struct kmembuckets;
struct kmemstats;
struct nchstats;
struct timeval;
struct tty;
struct ttydevice_tmp;
struct ttytotals;
struct vmmeter;
struct vmtotal;

/*
 * kvm systat/vmstat/iostat functions.
 *
 * These use kvm_read on dead kernels and sysctl() on live kernels.
 *
 * Note that statdisks() returns stats in the same order
 * as statdknames()'s names.
 */
int	kvm_boottime __P((kvm_t *, struct timeval *));
int	kvm_cpustats __P((kvm_t *, struct cpustats *));
char  **kvm_dknames __P((kvm_t *, int *));
int	kvm_disks __P((kvm_t *, struct diskstats *dkp, int));
int	kvm_kmemsize __P((kvm_t *, int *, int *));
int	kvm_kmem __P((kvm_t *, struct kmembuckets *, int,
			struct kmemstats *, int));
int	kvm_ncache __P((kvm_t *, struct nchstats *));
int	kvm_stathz __P((kvm_t *));
int	kvm_ttys __P((kvm_t *, struct ttydevice_tmp **, int *,
		struct tty **, int *));
int	kvm_ttytotals __P((kvm_t *, struct ttytotals *));
int	kvm_vmmeter __P((kvm_t *, struct vmmeter *));
int	kvm_vmtotal __P((kvm_t *, struct vmtotal *));
struct e_vnode *
	kvm_vnodes __P((kvm_t *, int *));
