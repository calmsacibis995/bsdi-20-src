/*
** paths.h		Common path definitions for the in.identd daemon
**
** Last update: 11 Dec 1992
**
** Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
*/

#ifdef SEQUENT
#  define _PATH_UNIX "/dynix"
#endif

#if defined(MIPS) || defined(IRIX) || defined(sco)
#  define _PATH_UNIX "/unix"
#endif

#if defined(hpux) || defined(__hpux)
#  define _PATH_UNIX "/hp-ux"
#endif

#ifdef SOLARIS
#  define _PATH_UNIX "/dev/ksyms"
#else
#  ifdef SVR4
#    define _PATH_UNIX "/stand/unix"
#  endif
#endif

#ifdef BSD43
#  define _PATH_SWAP "/dev/drum"
#  define _PATH_MEM  "/dev/mem"
#endif

#ifdef _AUX_SOURCE
#  define _PATH_UNIX "/unix"
#endif

#ifdef _CRAY
#  define _PATH_UNIX "/unicos"
#  define _PATH_MEM  "/dev/mem"
#endif

#ifdef NeXT
#  define _PATH_UNIX "/mach"
#endif

#ifdef	__bsdi__
#  define _PATH_UNIX "/bsd"
#endif	/* __bsdi__ */


/*
 * Some defaults...
 */
#ifndef _PATH_KMEM
#  define _PATH_KMEM "/dev/kmem"
#endif

#ifndef _PATH_UNIX
#  define _PATH_UNIX "/vmunix"
#endif


#ifndef PATH_CONFIG
#  define PATH_CONFIG "/etc/identd.conf"
#endif

#ifndef PATH_DESKEY
#  define PATH_DESKEY "/etc/identd.key"
#endif
