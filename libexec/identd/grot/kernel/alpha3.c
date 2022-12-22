/*
 * May 23, 1994 - Modified by Allan E. Johannesen (aej@wpi.edu) from code
 * kindly provided by Digital during the Beta test of Digital Alpha AXP OSF/1
 * 3.0 when WPI discovered that the file structures had changed.  Prior to 3.0,
 * OSF/1 ident support had only needed 64-bit modifications to the `other.c'
 * kernel routine (those mods done at WPI during the initial OSF/1 Beta tests).
 *
 * NOTE:
 *   This tool is NOT part of DEC OSF/1 and is NOT a supported product.
 *
 * BASED ON code provided by
 *   Aju John, UEG, Digital Equipment Corp. (ZK3) Nashua, NH.
 *
 * The following is an **unsupported** tool. Digital Equipment Corporation
 * makes no representations about the suitability of the software described
 * herein for any purpose. It is provided "as is" without express or implied
 * warranty.
 *
 * BASED ON:
 *  PADS program by Stephen Carpenter, UK UNIX Support, Digital Equipment Corp.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <nlist.h>
#include <sys/types.h>
#define SHOW_UTT
#include <sys/user.h>
#define KERNEL_FILE
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>

#include "identd.h"
#include "error.h"
#include "kvm.h"

struct pid_entry {
        pid_t   pe_pid;                 /* process id */
        int     pe_generation;          /* checks for struct re-use */
        struct proc *pe_proc;           /* pointer to this pid's proc */
        union {
                struct pid_entry *peu_nxt;      /* next entry in free list */
                struct {
                        int     peus_pgrp;      /* pid is pgrp leader? */
                        int     peus_sess;      /* pid is session leader? */
                } peu_s;
        } pe_un;
};

#define proc_to_utask(p) (long)(p+sizeof(struct proc))          /* location of the utask structure in the supertask */

#ifdef NOFILE_IN_U                                              /* more than 64 open files per process ? */
#  define OFILE_EXTEND
#else
#  define NOFILE_IN_U NOFILE
#endif

struct nlist name_list[] = {
#define N_PIDTAB 0
  { "_pidtab" },
#define N_NPID 1
  { "_npid" },
  { 0 }
};

static kvm_t *kd;

int k_open()
{
  if (!(kd = kvm_open("/vmunix", "/dev/kmem" , NULL, O_RDONLY, NULL)))
    ERROR("main: kvm_open");
  
  if (kvm_nlist(kd, name_list) != 0)
    ERROR("main: kvm_nlist");

  return 0;
}

int check_procs (off_t pidtab_base, int npid,struct in_addr *faddr, int fport, struct in_addr *laddr, int lport, int *uid)
{
  struct utask proc_utask;
  struct proc the_proc;
  struct pid_entry the_pid_entry;
  struct file **ofile_table_extension, open_file;
  int index, index1;
  long the_proc_utask, addr_of_proc;
 
#ifdef OFILE_EXTEND
      /* Reserve space for the extended open file table of a process */
  ofile_table_extension = (struct file **) malloc ((getdtablesize () - NOFILE_IN_U) * sizeof (struct file *));
#endif

  for (index = 0; index < npid; index++)
    {
          /* Read in the process structure */

      kvm_read(kd, pidtab_base + (off_t)(index * sizeof (struct pid_entry)), &the_pid_entry, sizeof (the_pid_entry));

      if (the_pid_entry.pe_proc == 0)
        {
          continue;
        };

      kvm_read (kd, (off_t) the_pid_entry.pe_proc, &the_proc, sizeof (the_proc));

          /* Read in the utask struct of the process */

      addr_of_proc = (long) the_pid_entry.pe_proc;
      the_proc_utask = proc_to_utask(addr_of_proc);
      kvm_read (kd, the_proc_utask, &proc_utask, sizeof (proc_utask));

#ifdef OFILE_EXTEND

      if (proc_utask.uu_file_state.uf_lastfile >= NOFILE_IN_U)
        {
          kvm_read (kd, (off_t) proc_utask.uu_file_state.uf_ofile_of,ofile_table_extension,
                    proc_utask.uu_file_state.uf_of_count * sizeof (struct file *));
        };
#endif
      for (index1 = 0; index1 <= proc_utask.uu_file_state.uf_lastfile; index1++)
        {
          if (index1 < NOFILE_IN_U)
            {
              if (proc_utask.uu_file_state.uf_ofile[index1] == (struct file *) NULL)
                {
                  continue;
                };

              if (proc_utask.uu_file_state.uf_ofile[index1] == (struct file *) -1)
                {
                  continue;
                };

              kvm_read (kd, (off_t) proc_utask.uu_file_state.uf_ofile[index1],&open_file, sizeof (open_file));

#ifdef OFILE_EXTEND
            }
          else
            {
              if (*(ofile_table_extension + index1 - NOFILE_IN_U) == (struct file *) NULL)
                {
                  continue;
                };

              kvm_read (kd,(off_t) *(ofile_table_extension + index1 - NOFILE_IN_U),&open_file, sizeof (open_file));
#endif 
            };

          if (open_file.f_type == DTYPE_SOCKET)
            {
              struct socket try_socket;
              struct inpcb try_pcb;
              if (kvm_read(kd,(off_t) open_file.f_data,&try_socket,sizeof(try_socket)) < 0)
                {
                  continue;
                };
              if (try_socket.so_pcb)
                {
                  if (kvm_read (kd, (off_t) try_socket.so_pcb,&try_pcb,sizeof(try_pcb)) < 0)
                    {
                      continue;
                    };
                  if ( try_pcb.inp_faddr.s_addr == faddr->s_addr &&
                       try_pcb.inp_laddr.s_addr == laddr->s_addr &&
                       try_pcb.inp_fport        == fport &&
                       try_pcb.inp_lport        == lport )
                    {
                      *uid = the_proc.p_ruid;
                      return(1);
                    };
                };
              continue;
            };
        };
    };
  return(0);
}

int k_getuid(struct in_addr *faddr, int fport, struct in_addr *laddr, int lport, int *uid)
{
  off_t
    pidtab_base;                                                /* Start address of the process table */
  int
    npid,                                                       /* Number of processes in the process table */
    k;                                                          /* check through nlist for misses */
                                                                /* Find the start of the process table */
  kvm_read (kd, (off_t) name_list[N_PIDTAB].n_value, &pidtab_base, sizeof (pidtab_base));
                                                                /* Find the size of the process table */
  kvm_read (kd, (off_t) name_list[N_NPID].n_value,&npid, sizeof (npid));
                                                                /* search for the circuit */
  return(check_procs (pidtab_base, npid, faddr, fport, laddr, lport, uid));
}

