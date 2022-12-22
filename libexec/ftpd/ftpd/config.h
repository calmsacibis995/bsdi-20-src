#define HAVE_SYMLINK
#undef  HAVE_DIRENT
#undef  HAVE_D_NAMLEN
#define HAVE_FLOCK
#undef  HAVE_FTW
#undef  HAVE_GETCWD
#define HAVE_GETDTABLESIZE
#undef  HAVE_PSTAT
#define HAVE_REGEX_H
#define HAVE_ST_BLKSIZE
#undef  HAVE_SYSINFO
#define HAVE_UT_UT_HOST
#define HAVE_VPRINTF
#define OVERWRITE
#define REGEXEC
#define HAS_REGEX_H
#define SETPROCTITLE
#undef  SHADOW_PASSWORD
#define UPLOAD
#undef  USG

#include <stdlib.h>
#include <unistd.h>
#include <fnmatch.h>

#ifndef FACILITY
#define FACILITY LOG_DAEMON
#endif

typedef void	SIGNAL_TYPE;

#include "../config.h"
