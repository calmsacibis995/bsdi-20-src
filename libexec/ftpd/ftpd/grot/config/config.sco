#define FACILITY LOG_LOCAL7
#define SETPROCTITLE

/* End of configurable parameters for SCO. Leave the below as it is. */

#undef  BSD
#define HAVE_DIRENT
#undef  HAVE_FLOCK
#undef  HAVE_FTW
#define HAVE_GETCWD
#define HAVE_GETDTABLESIZE
#undef  HAVE_PSTAT
#undef  HAVE_ST_BLKSIZE
#undef  HAVE_SYSINFO
#undef  HAVE_UT_UT_HOST
#define HAVE_VPRINTF
#define OVERWRITE
#define REGEX
#define SETPROCTITLE
#undef  SHADOW_PASSWORD
#define UPLOAD
#define USG

#ifdef _M_UNIX
# define HAVE_SYMLINK
#else
# undef  HAVE_SYMLINK
#endif

#undef  HAVE_D_NAMLEN
#define NO_MALLOC_PROTO

#ifdef SETPROCTITLE
# define setproctitle SCOproctitle
#endif

# define MAXPATHLEN 1024

#ifdef _M_UNIX
# define _KR		/* need #define NULL 0 */
# undef __STDC__	/* ugly, but does work :-) */
#else
# define SYSLOGFILE "/usr/adm/ftpd"
# define NULL 0
#endif

#define crypt(k,s) bigcrypt(k,s)
#define d_fileno d_ino
#define ftruncate(fd,size) chsize(fd,size) /* needs -lx */
#define getpagesize() (4096)
#define vfork fork

#define _PATH_WTMP "/etc/wtmp"
#define _PATH_UTMP "/etc/utmp"

#include <stdlib.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/socket.h>     /* eliminate redefinition of _IO
                               in audit.h under 3.2v2.0 */

#define SecureWare
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>

#include <sys/fcntl.h>
#define L_SET SEEK_SET
#define L_INCR SEEK_CUR

typedef void    SIGNAL_TYPE;

#include "../config.h"
