/* 
 * Top level config file... you'll probably not need to modify any of this.
 *
 * In the future, a lot more definable features will be here (and this
 * will all make sense...)
 */

/*
 * allow "upload" keyword in ftpaccess
 */

#define UPLOAD

/*
 * allow "overwrite" keyword in ftpaccess.
 */

#define OVERWRITE

/*
 * allow "allow/deny" for individual users.
 */

#define HOST_ACCESS

/*
 * log failed login attempts
 */

#define LOG_FAILED

/*
 * allow use of private file.  (for site group and site gpass)
 */

#undef NO_PRIVATE

/*
 * Try once more on failed DNS lookups (to allow far away connections 
 * which might resolve slowly)
 */

#define	DNS_TRYAGAIN
