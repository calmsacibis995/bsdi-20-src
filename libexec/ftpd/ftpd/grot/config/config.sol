#define HAVE_SYMLINK
#undef  F_SETOWN
#define HAVE_DIRENT
#define HAVE_D_NAMLEN
#undef  HAVE_FLOCK
#define HAVE_FTW
#define HAVE_GETCWD
#undef  HAVE_GETDTABLESIZE
#undef  HAVE_PSTAT
#define HAVE_STATVFS
#define HAVE_ST_BLKSIZE
#define HAVE_SYSINFO
#undef  HAVE_UT_UT_HOST
#define HAVE_VPRINTF
#define L_INCR	SEEK_CUR
#define REGEX
#define SETPROCTITLE
#define SHADOW_PASSWORD
#define SOLARIS_21
#define SVR4
#define USG

#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef FACILITY
#define FACILITY LOG_DAEMON
#endif

typedef void	SIGNAL_TYPE;

#include "../config.h"
