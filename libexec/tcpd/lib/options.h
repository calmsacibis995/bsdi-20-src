/* @(#) options.h 1.1 93/08/26 23:49:36 */

 /*
  * Unusual returns from the options module are done by jumping back into the
  * hosts_access() routine. This is cleaner than checking te return value of
  * each and every silly little function.
  */

extern jmp_buf options_buf;

 /*
  * In verification mode an option function should just say what it would do,
  * instead of really doing it. An option function that would not return
  * should clear the dry_run flag to inform the caller of this unusual
  * behavior.
  */

extern int dry_run;

 /*
  * Information that is passed on when jumping back into hosts_access(). The
  * value 0 cannot be used because it is already taken by setjmp().
  */

#define	OPT_ALLOW	2		/* grant access */
#define	OPT_DENY	3		/* deny access */

extern void process_options();
