/*
** kernel/sunos5.c                 SunOS 5 kernel access functions
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Author: Casper Dik <casper@fwi.uva.nl>
**
** Last update: 13 Oct 1994
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/


/*#define DEBUGHASH /**/
#define _KMEMUSER
#define _KERNEL

/* some definition conflicts. but we must define _KERNEL */

#define exit 		kernel_exit
#define strsignal	kernel_strsignal

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <netinet/in.h>

#include <stdio.h>
#include <kvm.h>
#include <nlist.h>
#include <math.h>
#include <sys/fcntl.h>
#include <sys/cred.h>
#include <sys/file.h>
#include <sys/stream.h>
#include <inet/common.h>
#include <inet/ip.h>

#ifdef ipc_tcp_laddr
#define SOLARIS24
/*
 * In Solaris 2.4 there have been a number of changes:
 * - the ipc_s structure had its field names changed
 * - the file global no longer exists in the kernel.
 * (this sort of makes sense for MP machines: having to go through
 * one global lock for all file opens/closes doesn't scale
 * very well)
 */
#endif

#undef exit
#undef strsignal

#include <unistd.h>
#include <string.h>
#include <stddef.h>

#include "identd.h"
#include "error.h"

#define N_FANOUT 0
#ifndef SOLARIS24
#define N_FILE	 1
#endif

struct nlist nl[] = {
	{ "ipc_tcp_fanout" },
#ifndef SOLARIS24
	{ "file" },
#endif
	{ 0 },
};


static kvm_t *kd;


int k_open()
{
  /*
  ** Open the kernel memory device
  */
  if (!(kd = kvm_open(path_unix, path_kmem, NULL, O_RDONLY, NULL)))
    ERROR("main: kvm_open");
  
  /*
  ** Extract offsets to the needed variables in the kernel
  */
  if (kvm_nlist(kd, nl) != 0)
    ERROR("main: kvm_nlist");

  return 0;
}


/*
** Get a piece of kernel memory with error handling.
** Returns 1 if call succeeded, else 0 (zero).
*/
static int getbuf(addr, buf, len, what)
  long addr;
  char *buf;
  int len;
  char *what;
{

  if (kvm_read(kd, addr, buf, len) < 0)
  {
    if (syslog_flag)
      syslog(LOG_ERR, "getbuf: kvm_read(%08x, %d) - %s : %m",
	     addr, len, what);

    return 0;
  }
  
  return 1;
}


/*
** Return the user number for the connection owner
*/
int k_getuid(faddr, fport, laddr, lport, uid)
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
  int *uid;
{
    queue_t *qp, *pq, sqr;
    ipc_t ic, *icp;
    unsigned short uslp, usfp;
    unsigned int offset;
    unsigned long fp;
    file_t tf;
    unsigned long zero = 0;
    u16 *ports;
    u32 *locaddr, *raddr;
#ifdef DEBUGHASH
    int i;
#endif
#ifdef SOLARIS24
    struct proc *procp;
#endif
    
    usfp = fport;
    uslp = lport;

#ifdef SOLARIS24
    /* low byte of local port number not used, low byte of 
       local addres is used
	ip_bind  in the kernel (+ approx 0x4c0)
                srl     %i3, 0x18, %o0
                xor     %i2, %o0, %o0
                srl     %i3, 0x10, %o1
                xor     %o0, %o1, %o0
                xor     %o0, %l0, %o0
                xor     %o0, %i3, %o0
                and     %o0, 0xff, %o0
                sethi   %hi(0xfc1d9c00), %o2
                or      %o2, 0x1c0, %o2          ! ipc_tcp_fanout

     */
    offset = usfp >> 8;
    offset ^= usfp ^ uslp;
    offset ^= faddr->S_un.S_un_b.s_b4;
    if (uslp > 8 || usfp != 0)
	offset ^= 1;
#else
    offset = usfp ^ uslp;
    offset ^= (unsigned) faddr->S_un.S_un_b.s_b4 ^ (offset >> 8);
#endif
    offset &= 0xff;

    if (!getbuf(nl[N_FANOUT].n_value + sizeof(ipc_t *) * offset,
		(char *) &icp,
		sizeof(ipc_t *),
		"ipc_tcp_fanout[offset]"))
	return -1;
    
#ifndef DEBUGHASH
    if (icp == 0) {
	syslog(LOG_INFO, "k_getuid: Hash miss");
	return -1;
    }
#endif

#ifdef SOLARIS24
    locaddr = &ic.ipc_tcp_laddr;
    raddr = &ic.ipc_tcp_faddr;
    ports = (u16*) &ic.ipc_tcp_ports;
#else
    locaddr = (u32*) &ic.ipc_tcp_addr[0];
    raddr = (u32*) &ic.ipc_tcp_addr[2];
    ports = &ic.ipc_tcp_addr[4];
#endif

#ifdef DEBUGHASH
  for (i = 0; i < 256; i++) {
    if (!getbuf(nl[N_FANOUT].n_value + sizeof(ipc_t *) * i,
		(char *) &icp,
		sizeof(ipc_t *),
		"ipc_tcp_fanout[offset]"))
	return -1;
    if (icp == 0)
	continue;
#endif

    while (icp) {
	if (!getbuf((unsigned long) icp,
		    (char *) &ic,
		    sizeof(ic),
		    "hash entry"))
	    return -1;

	if (usfp == ports[0] && /* remote port */
	    uslp == ports[1] && /* local port */
#if 0
	    memcmp(&laddr->s_addr, locaddr, 4) == 0 && /* local */
#else
 	    (memcmp(&laddr->s_addr, locaddr, 4) == 0 ||
 	    /* In SunOS 5.3, the local part can be all zeros */
 	     memcmp(&zero, locaddr, 4) == 0) /* local */ &&
#endif
	    memcmp(&faddr->s_addr, raddr, 4) == 0)
		break;
	icp = ic.ipc_hash_next;
    }
#ifdef DEBUGHASH
    if (icp)
	break;
  } /* for i */
    if (icp)
	printf("found, offset = %x, i = %x, i ^ offset = %x\n", offset,i,
		offset ^ i);
#endif

    if (!icp) {
	syslog(LOG_INFO, "k_getuid: Port not found");
	return -1;
    }
    
    if (!getbuf((unsigned long) ic.ipc_rq+offsetof(queue_t, q_stream),
		(char *) &sqr.q_stream,
		sizeof(sqr.q_stream),
		"queue.q_stream"))
	return -1;

    /* at this point sqr.q_stream holds the pointer to the stream we're
       interested in. Now we're going to find the file pointer
       that refers to the vnode that refers to this stream stream */

#ifdef SOLARIS24
    /* Solaris 2.4 no longer links all file pointers together with
     * f_next, the only way seems to be scrounging them from
     * the proc/user structure, ugh.
     */

    if (kvm_setproc(kd) != 0)
	return -1;

    while (procp = kvm_nextproc(kd)) {
	struct uf_entry files[NFPCHUNK];
	int nfiles = procp->p_user.u_nofiles;
	unsigned long addr = (unsigned long) procp->p_user.u_flist;

	while  (nfiles > 0) {
	    int nread = nfiles > NFPCHUNK ? NFPCHUNK : nfiles;
	    int size = nread * sizeof(struct uf_entry);
	    int i;
	    struct file *last = 0;
	    vnode_t vp;

	    if (!getbuf(addr, (char*) &files[0], size, "ufentries")) {
		return -1;
	    }
	    for (i = 0; i < nread; i++) {
		if (files[i].uf_ofile == 0 || files[i].uf_ofile == last)
		    continue;
		if (!getbuf((unsigned long) (last = files[i].uf_ofile),
			(char*) &tf, sizeof(tf), "file pointer")) {
			    return -1;
		}

		if (!tf.f_vnode)
		    continue;

		if (!getbuf((unsigned long) tf.f_vnode +
				offsetof(vnode_t,v_stream),
				(char *) &vp.v_stream,
				sizeof(vp.v_stream),"vnode.v_stream"))
		    return -1;

		if (vp.v_stream == sqr.q_stream) {
		    cred_t cr;
		    if (!getbuf((unsigned long) tf.f_cred +
				    offsetof(cred_t, cr_ruid),
				(char *) &cr.cr_ruid,
				sizeof(cr.cr_ruid),
				"cred.cr_ruid"))
			return -1;
		    *uid = cr.cr_ruid;
		    return 0;
		}
	    }
	    nfiles -= nread;
	    addr += size;
	}
    }
#else
    fp = nl[N_FILE].n_value;
    for (;fp;fp = (unsigned long) tf.f_next) {
	vnode_t vp;

	if (!getbuf(fp, (char *) &tf, sizeof(file_t),"file pointer"))
	    return -1;

	if (!tf.f_vnode)
	    continue;

	if (!getbuf((unsigned long) tf.f_vnode + offsetof(vnode_t,v_stream),
			(char *) &vp.v_stream,
			sizeof(vp.v_stream),"vnode.v_stream"))
	    return -1;

	if (vp.v_stream == sqr.q_stream) {
	    cred_t cr;
	    if (!getbuf((unsigned long) tf.f_cred + offsetof(cred_t, cr_ruid),
			(char *) &cr.cr_ruid,
			sizeof(cr.cr_ruid),
			"cred.cr_ruid"))
		return -1;
	    *uid = cr.cr_ruid;
	    return 0;
	}
    }
#endif
    return -1;
}
