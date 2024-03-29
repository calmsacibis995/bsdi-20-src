/*	BSDI	kernel.c,v 2.1 1995/02/03 06:45:59 polk Exp	*/

/*
** kernel/bsdi.c		Low level kernel access functions for BSDI
**
** This program is in the public domain and may be used freely by anyone
** who wants to. 
**
** Last update: 7 April 1994
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#include <paths.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <nlist.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>

#include <kvm.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <sys/socketvar.h>

#include <sys/uio.h>

#define KERNEL

#include <sys/file.h>

#undef KERNEL

#include <fcntl.h>

#include <sys/user.h>

#include <sys/wait.h>

#include <sys/filedesc.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <netinet/in_pcb.h>

#include <netinet/tcp.h>
#include <netinet/ip_var.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <arpa/inet.h>

#include <pwd.h>

#include "identd.h"
#include "error.h"


extern void *calloc();
extern void *malloc();


struct nlist nl[] =
{
#define N_FILE 0
#define N_NFILE 1
#define N_TCB 2
      
  { "_filehead" },
  { "_nfiles" },
  { "_tcb" },
  { "" }
};


static struct file *xfile;
static int nfile;

static struct inpcb tcb;
static kvm_t *kd = NULL;

int k_open()
{
  /*
  ** Open the kernel memory device
  */
  if ((kd = (kvm_t *)kvm_openfiles(path_unix, path_kmem, NULL, O_RDONLY, NULL)) == NULL)
    ERROR("main: kvm_open");
  
  /*
  ** Extract offsets to the needed variables in the kernel
  */
  if (kvm_nlist(kd, nl) < 0)
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
** Traverse the inpcb list until a match is found.
** Returns NULL if no match.
*/
static struct socket *
    getlist(pcbp, faddr, fport, laddr, lport)
  struct inpcb *pcbp;
  struct in_addr *faddr;
  int fport;
  struct in_addr *laddr;
  int lport;
{
  struct inpcb *head;

  if (!pcbp)
    return NULL;

  
  head = pcbp->inp_prev;
  do 
  {
    if ( pcbp->inp_faddr.s_addr == faddr->s_addr &&
	 pcbp->inp_laddr.s_addr == laddr->s_addr &&
	 pcbp->inp_fport        == fport &&
	 pcbp->inp_lport        == lport )
{
      return pcbp->inp_socket;
}
  } while (pcbp->inp_next != head &&
	   getbuf((long) pcbp->inp_next,
		  pcbp,
		  sizeof(struct inpcb),
		  "tcblist"));

  return NULL;
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
  long addr;
  struct socket *sockp;
  int i;
  struct kinfo_proc	*kp;
  int			nentries;

  if ((kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, &nentries)) == NULL)
  {
    ERROR("k_getuid: kvm_getprocs");
    return -1;
  }

  /* -------------------- TCP PCB LIST -------------------- */
  if (!getbuf(nl[N_TCB].n_value, &tcb, sizeof(tcb), "tcb"))
    return -1;
  
  tcb.inp_prev = (struct inpcb *) nl[N_TCB].n_value;
  sockp = getlist(&tcb, faddr, fport, laddr, lport);

  if (!sockp)
    return -1;

  /*
  ** Locate the file descriptor that has the socket in question
  ** open so that we can get the 'ucred' information
  */
  for (i = 0; i < nentries; i++)
  {
    if(kp[i].kp_proc.p_fd != NULL)
    {
      int				j;
      struct filedesc	pfd;
      struct file		**ofiles;
      struct file		ofile;

      if(!getbuf(kp[i].kp_proc.p_fd, &pfd, sizeof(pfd), "pfd"))
        return(-1);

      ofiles = (struct file **) malloc(pfd.fd_nfiles * sizeof(struct file *));

      if(!getbuf(pfd.fd_ofiles, ofiles,
                 pfd.fd_nfiles * sizeof(struct file *), "ofiles"))
      {
        free(ofiles);
        return(-1);
      }

      for(j = 0; j < pfd.fd_nfiles; j ++)
      {
        if(!getbuf(ofiles[j], &ofile, sizeof(struct file), "ofile"))
        {
          free(ofiles);
          return(-1);
        }

        if(ofile.f_count == 0)
          continue;

        if(ofile.f_type == DTYPE_SOCKET &&
           (struct socket *) ofile.f_data == sockp)
        {
          struct pcred	pc;

          if(!getbuf(kp[i].kp_proc.p_cred, &pc, sizeof(pc), "pcred"))
            return(-1);

          *uid = pc.p_ruid;
          free(ofiles);
          return 0;
        }
      }

      free(ofiles);
    }
  }

  return -1;
}

