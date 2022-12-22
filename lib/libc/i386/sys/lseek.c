/*	BSDI lseek.c,v 2.1 1995/02/03 06:30:15 polk Exp	*/

#include <sys/param.h>
#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>

extern off_t
__syscall __P((off_t, ...));

off_t
lseek(f, o, w)
	int f;
	off_t o;
	int w;
{
	off_t r;
	int save_errno = errno;

	errno = 0;
	r = __syscall(SYS_lseek, f, 0, o, w);
	if (errno == 0)
		errno = save_errno;
	else
		r = -1;
	return (r);
}
