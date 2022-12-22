#ifndef _G_config_h_
#define _G_config_h_

#define _G_LIB_VERSION "2.6.2"

#define _G_NAMES_HAVE_UNDERSCORE	1

#define	_G_VTABLE_LABEL_HAS_LENGTH	1
#define	_G_VTABLE_LABEL_PREFIX		"__vt$"
#define _G_HAVE_ST_BLKSIZE		1

#ifdef _POSIX_SOURCE		/* XXX */
# undef _POSIX_SOURCE		/* XXX */
# include <sys/types.h>		/* XXX */
# define _POSIX_SOURCE 1	/* XXX */
#else				/* XXX */
#include <sys/types.h>		/* XXX */
#endif				/* XXX */
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

typedef dev_t	_G_dev_t;
typedef clock_t	_G_clock_t;
typedef fpos_t	_G_fpos_t;
typedef gid_t	_G_gid_t;
typedef ino_t	_G_ino_t;
typedef mode_t	_G_mode_t;
typedef nlink_t	_G_nlink_t;
typedef off_t	_G_off_t;
typedef pid_t	_G_pid_t;
typedef ptrdiff_t	_G_ptrdiff_t;
typedef sigset_t	_G_sigset_t;
typedef size_t	_G_size_t;
typedef ssize_t	_G_ssize_t;
typedef time_t	_G_time_t;
typedef uid_t	_G_uid_t;
typedef va_list	_G_va_list;
typedef wchar_t	_G_wchar_t;

typedef	int	_G_wint_t;

#define	_G_signal_return_type	void
#define	_G_sprintf_return_type	int

typedef	int8_t		_G_int8_t;
typedef	u_int8_t	_G_uint8_t;
typedef int16_t		_G_int16_t;
typedef u_int16_t	_G_uint16_t;
typedef int32_t		_G_int32_t;
typedef u_int32_t	_G_uint32_t;
typedef int64_t		_G_int64_t;
typedef u_int64_t	_G_uint64_t;

#define	_G_BUFSIZ	BUFSIZ
#define	_G_FOPEN_MAX	FOPEN_MAX
#define	_G_FILENAME_MAX	FILENAME_MAX
#define	_G_NULL		NULL
#define	_G_ARGS(x)	(...)

#define	_G_HAVE_ATEXIT	1

#define	_G_HAVE_SYS_RESOURCE	1
#define	_G_HAVE_SYS_SOCKET	1
#define	_G_HAVE_SYS_WAIT	1
#define	_G_HAVE_UNISTD		1
#define	_G_HAVE_DIRENT		1
#define	_G_HAVE_CURSES		1
#define	_G_MATH_H_INLINES	0
#define	_G_HAVE_BOOL		1

#endif /* _G_config_h */
