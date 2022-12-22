/*	BSDI	extensions.c,v 2.1 1995/02/03 06:40:39 polk Exp	*/

/* Copyright (c) 1993, 1994  Washington University in Saint Louis
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * Washington University in Saint Louis and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASHINGTON UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASHINGTON
 * UNIVERSITY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#include <time.h>
#include <pwd.h>
#include <grp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>

#ifdef HAVE_STATVFS
#include <sys/statvfs.h>
#endif

#include <arpa/ftp.h>

#include "pathnames.h"
#include "extensions.h"
#include "ftw.h"

#ifdef HAVE_REGEX_H
#include <regex.h>
#endif

#if defined(REGEX) && defined(SOLARIS_21)
#include <libgen.h>
#endif

extern int fnmatch(),
  type,
  transflag,
  autospout_free,
  data,
  anonymous,
  guest;
extern char **ftpglob(register char *v),
 *globerr,
  remotehost[],
  hostname[],
 *autospout,
  Shutdown[];

char shuttime[30],
  denytime[30],
  disctime[30];

#ifndef REGEX
char *re_comp();
#elif defined(M_UNIX)
extern char *regcmp(), *regex();
#endif

extern FILE *dataconn(char *name, off_t size, char *mode);
FILE *dout;

time_t newer_time;

int show_fullinfo;

check_newer(char *path, struct stat *st, int flag)
{

    if (st->st_mtime > newer_time) {
        if (show_fullinfo != 0) {
            if (flag == FTW_F || flag == FTW_D) {
                fprintf(dout, "%s %qd %ld %s", flag == FTW_F ? "F" : "D",
                        (quad_t)st->st_size, (long)st->st_mtime, path);
            }
        } else if (flag == FTW_F)
            fprintf(dout, "%s", path);
    }
    return 0;
}

#ifdef HAVE_STATVFS
int getSize(s)
char *s;
{
    int c;
    struct statvfs buf;

    if (( c = statvfs(s, &buf)) != 0)
        return(0);

    return(buf.f_bavail * buf.f_frsize / 1024);
}
#endif

/*************************************************************************/
/* FUNCTION  : msg_massage                                               */
/* PURPOSE   : Scan a message line for magic cookies, replacing them as  */
/*             needed.                                                   */
/* ARGUMENTS : pointer input and output buffers                          */
/*************************************************************************/

int
msg_massage(char *inbuf, char *outbuf)
{
    char *inptr = inbuf;
    char *outptr = outbuf;
    char buffer[MAXPATHLEN];
    time_t curtime;
    int limit;
    extern struct passwd *pw;
    struct aclmember *entry = NULL;

    (void) time(&curtime);
    (void) acl_getclass(buffer);

    limit = acl_getlimit(buffer, NULL);

    while (*inptr) {
        if (*inptr != '%')
            *outptr++ = *inptr;
        else {
            switch (*++inptr) {
            case 'E':
                if ( (getaclentry("email", &entry)) && ARG0 )
                    sprintf(outptr, "%s", ARG0); 
                break;
            case 'N': 
                sprintf(outptr, "%d", acl_countusers(buffer)); 
                break; 
            case 'M':
                sprintf(outptr, "%d", limit);
                break;

            case 'T':
                strncpy(outptr, ctime(&curtime), 24);
                *(outptr + 24) = NULL;
                break;

            case 'F':
#ifdef HAVE_STATVFS
                sprintf(outptr, "%lu", getSize("."));
#endif
                break;

            case 'C':
#ifdef HAVE_GETCWD
                (void) getcwd(outptr, MAXPATHLEN);
#else
                (void) getwd(outptr);
#endif
                break;

            case 'R':
                strcpy(outptr, remotehost);
                break;

            case 'L':
                strcpy(outptr, hostname);
                break;

            case 'U':
                strcpy(outptr, pw->pw_name);
                break;

            case 's':
                strncpy(outptr, shuttime, 24);
                *(outptr + 24) = NULL;
                break;

            case 'd':
                strncpy(outptr, disctime, 24);
                *(outptr + 24) = NULL;
                break;

            case 'r':
                strncpy(outptr, denytime, 24);
                *(outptr + 24) = NULL;
                break;

            case '%':
                *outptr++ = '%';
                *outptr = '\0';
                break;

            default:
                *outptr++ = '%';
                *outptr++ = '?';
                *outptr = '\0';
                break;
            }
            while (*outptr)
                outptr++;
        }
        inptr++;
    }
    *outptr = NULL;
}

/*************************************************************************/
/* FUNCTION  : cwd_beenhere                                              */
/* PURPOSE   : Return 1 if the user has already visited this directory   */
/*             via C_WD.                                                 */
/* ARGUMENTS : a power-of-two directory function code (README, MESSAGE)  */
/*************************************************************************/

int
cwd_beenhere(int dircode)
{
    struct dirlist {
        struct dirlist *next;
        int dircode;
        char dirname[1];
    };

    static struct dirlist *head = NULL;
    struct dirlist *curptr;
    char cwd[MAXPATHLEN];

    (void) realpath(".", cwd);

    for (curptr = head; curptr != NULL; curptr = curptr->next)
        if (strcmp(curptr->dirname, cwd) == NULL) {
            if (!(curptr->dircode & dircode)) {
                curptr->dircode |= dircode;
                return (0);
            }
            return (1);
        }
    curptr = (struct dirlist *) malloc(strlen(cwd) + 1 + sizeof(struct dirlist));

    if (curptr != NULL) {
        curptr->next = head;
        head = curptr;
        curptr->dircode = dircode;
        strcpy(curptr->dirname, cwd);
    }
    return (0);
}

/*************************************************************************/
/* FUNCTION  : show_banner                                               */
/* PURPOSE   : Display a banner on the user's terminal before login      */
/* ARGUMENTS : reply code to use                                         */
/*************************************************************************/

void
show_banner(int msgcode)
{
    char *crptr,
      linebuf[1024],
      outbuf[1024];
    struct aclmember *entry = NULL;
    FILE *infile;

    /* banner <path> */
    while (getaclentry("banner", &entry)) {
        if (ARG0 && strlen(ARG0) > 0) {
            infile = fopen(ARG0, "r");
            if (infile) {
                while (fgets(linebuf, 255, infile) != NULL) {
                    if ((crptr = strchr(linebuf, '\n')) != NULL)
                        *crptr = '\0';
                    msg_massage(linebuf, outbuf);
                    lreply(msgcode, "%s", outbuf);
                }
                fclose(infile);
                lreply(msgcode, "");
            }
        }
    }
}

/*************************************************************************/
/* FUNCTION  : show_message                                              */
/* PURPOSE   : Display a message on the user's terminal if the current   */
/*             conditions are right                                      */
/* ARGUMENTS : reply code to use, LOG_IN|CMD                             */
/*************************************************************************/

void
show_message(int msgcode, int mode)
{
    char *crptr,
      linebuf[1024],
      outbuf[1024],
      class[MAXPATHLEN],
      cwd[MAXPATHLEN];
    int show,
      which;
    struct aclmember *entry = NULL;
    FILE *infile;

    if (cwd_beenhere(1) != 0)
        return;

#ifdef HAVE_GETCWD
    (void) getcwd(cwd,MAXPATHLEN-1);
#else
    (void) getwd(cwd);
#endif
    (void) acl_getclass(class);

    /* message <path> [<when> [<class>]] */
    while (getaclentry("message", &entry)) {
        if (!ARG0)
            continue;
        show = 0;

        if (mode == LOG_IN && (!ARG1 || !strcasecmp(ARG1, "login")))
            if (!ARG2)
                show++;
            else {
                for (which = 2; (which < MAXARGS) && ARG[which]; which++)
                    if (strcasecmp(class, ARG[which]) == NULL)
                        show++;
            }
        if (mode == C_WD && ARG1 && !strncasecmp(ARG1, "cwd=", 4) &&
            (!strcmp((ARG1) + 4, cwd) || *(ARG1 + 4) == '*' ||
            fnmatch((ARG1) + 4, cwd, FNM_PATHNAME) != FNM_NOMATCH))
            if (!ARG2)
                show++;
            else {
                for (which = 2; (which < MAXARGS) && ARG[which]; which++)
                    if (strcasecmp(class, ARG[which]) == NULL)
                        show++;
            }
        if (show && strlen(ARG0) > 0) {
            infile = fopen(ARG0, "r");
            if (infile) {
                while (fgets(linebuf, 255, infile) != NULL) {
                    if ((crptr = strchr(linebuf, '\n')) != NULL)
                        *crptr = '\0';
                    msg_massage(linebuf, outbuf);
                    lreply(msgcode, "%s", outbuf);
                }
                fclose(infile);
                lreply(msgcode, "");
            }
        }
    }
}

/*************************************************************************/
/* FUNCTION  : show_readme                                               */
/* PURPOSE   : Display a message about a README file to the user if the  */
/*             current conditions are right                              */
/* ARGUMENTS : pointer to ACL buffer, reply code, LOG_IN|C_WD            */
/*************************************************************************/

void
show_readme(int code, int mode)
{
    char **filelist,
      class[MAXPATHLEN],
      cwd[MAXPATHLEN];
    int show,
      which,
      days;
    time_t clock;

    struct stat buf;
    struct tm *tp;
    struct aclmember *entry = NULL;

    if (cwd_beenhere(2) != 0)
        return;

#ifdef HAVE_GETCWD
    (void) getcwd(cwd,MAXPATHLEN-1);
#else
    (void) getwd(cwd);
#endif
    (void) acl_getclass(class);

    /* readme  <path> {<when>} */
    while (getaclentry("readme", &entry)) {
        if (!ARG0)
            continue;
        show = 0;

        if (mode == LOG_IN && (!ARG1 || !strcasecmp(ARG1, "login")))
            if (!ARG2)
                show++;
            else {
                for (which = 2; (which < MAXARGS) && ARG[which]; which++)
                    if (strcasecmp(class, ARG[which]) == NULL)
                        show++;
            }
        if (mode == C_WD && ARG1 && !strncasecmp(ARG1, "cwd=", 4)
            && (!strcmp((ARG1) + 4, cwd) || *(ARG1 + 4) == '*' ||
                fnmatch((ARG1) + 4, cwd, FNM_PATHNAME) != FNM_NOMATCH))
            if (!ARG2)
                show++;
            else {
                for (which = 2; (which < MAXARGS) && ARG[which]; which++)
                    if (strcasecmp(class, ARG[which]) == NULL)
                        show++;
            }
        if (show) {
            globerr = NULL;
            filelist = ftpglob(ARG0);
            if (!globerr) {
                while (filelist && *filelist) {
                   errno = 0;
                   if (!stat(*filelist, &buf)) {
                       lreply(code, "Please read the file %s", *filelist);
                       (void) time(&clock);
                       tp = localtime(&clock);
                       days = 365 * tp->tm_year + tp->tm_yday;
                       tp = localtime(&buf.st_mtime);
                       days -= 365 * tp->tm_year + tp->tm_yday;
/*
                       if (days == 0) {
                         lreply(code, "  it was last modified on %.24s - Today",
                           ctime(&buf.st_mtime));
                       } else {
*/
                         lreply(code, 
                           "  it was last modified on %.24s - %d day%s ago",
                           ctime(&buf.st_mtime), days, days == 1 ? "" : "s");
/*
                       }
*/
                   }
                   filelist++;
                }
            }
        }
    }
}

/*************************************************************************/
/* FUNCTION  : deny_badxfertype                                          */
/* PURPOSE   : If user is in ASCII transfer mode and tries to retrieve a */
/*             binary file, abort transfer and display appropriate error */
/* ARGUMENTS : message code to use for denial, path of file to check for */
/*             binary contents or NULL to assume binary file             */
/*************************************************************************/

int
deny_badasciixfer(int msgcode, char *filepath)
{

    if (type == TYPE_A && !*filepath) {
        reply(msgcode, "This is a BINARY file, using ASCII mode to transfer will corrupt it.");
        return (1);
    }
    /* The hooks are here to prevent transfers of actual binary files, not
     * just TAR or COMPRESS mode files... */
    return (0);
}

/*************************************************************************/
/* FUNCTION  : is_shutdown                                               */
/* PURPOSE   :                                                           */
/* ARGUMENTS :                                                           */
/*************************************************************************/

int
is_shutdown(int quiet)
{
    static struct tm tmbuf;
    static struct stat s_last;
    static time_t last = 0,
      shut,
      deny,
      disc;

    static char text[2048];

    struct stat s_cur;

    FILE *fp;

    int deny_off,
      disc_off;

    time_t curtime = time(NULL);

    char buf[1024],
      linebuf[1024];

    if (Shutdown[0] == '\0' || stat(Shutdown, &s_cur))
        return (0);

    if (s_last.st_mtime != s_cur.st_mtime) {
        s_last = s_cur;

        fp = fopen(Shutdown, "r");
        if (fp == NULL)
            return (0);
        fgets(buf, sizeof(buf), fp);
        if (sscanf(buf, "%d %d %d %d %d %d %d", &tmbuf.tm_year, &tmbuf.tm_mon,
        &tmbuf.tm_mday, &tmbuf.tm_hour, &tmbuf.tm_min, &deny, &disc) != 7) {
            return (0);
        }
        deny_off = 3600 * (deny / 100) + 60 * (deny % 100);
        disc_off = 3600 * (disc / 100) + 60 * (disc % 100);

        tmbuf.tm_year -= 1900;
        tmbuf.tm_isdst = -1;
        shut = mktime(&tmbuf);
        strcpy(shuttime, ctime(&shut));

        disc = shut - disc_off;
        strcpy(disctime, ctime(&disc));

        deny = shut - deny_off;
        strcpy(denytime, ctime(&deny));

        text[0] = '\0';

        while (fgets(buf, sizeof(buf), fp) != NULL) {
            msg_massage(buf, linebuf);
            if ((strlen(text) + strlen(linebuf)) < sizeof(text))
                strcat(text, linebuf);
        }

        (void) fclose(fp);
    }
    /* if last == 0, then is_shutdown() only called with quiet == 1 so far */
    if (last == 0 && !quiet) {
        autospout = text;       /* warn them for the first time */
        autospout_free = 0;
        last = curtime;
    }
    /* if past disconnect time, tell caller to drop 'em */
    if (curtime > disc)
        return (1);

    /* if less than 60 seconds to disconnection, warn 'em continuously */
    if (curtime > (disc - 60) && !quiet) {
        autospout = text;
        autospout_free = 0;
        last = curtime;
    }
    /* if less than 15 minutes to disconnection, warn 'em every 5 mins */
    if (curtime > (disc - 60 * 15)) {
        if ((curtime - last) > (60 * 5) && !quiet) {
            autospout = text;
            autospout_free = 0;
            last = curtime;
        }
    }
    /* if less than 24 hours to disconnection, warn 'em every 30 mins */
    if (curtime < (disc - 24 * 60 * 60) && !quiet) {
        if ((curtime - last) > (60 * 30)) {
            autospout = text;
            autospout_free = 0;
            last = curtime;
        }
    }
    /* if more than 24 hours to disconnection, warn 'em every 60 mins */
    if (curtime > (disc - 24 * 60 * 60) && !quiet) {
        if ((curtime - last) >= (24 * 60 * 60)) {
            autospout = text;
            autospout_free = 0;
            last = curtime;
        }
    }
    return (0);
}

newer(char *date, char *path, int showlots)
{
    struct tm tm;

    if (sscanf(date, "%04d%02d%02d%02d%02d%02d",
               &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
               &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) {

        tm.tm_year -= 1900;
        tm.tm_mon--;
        tm.tm_isdst = -1;
        newer_time = mktime(&tm);
        dout = dataconn("file list", (off_t) - 1, "w");
        /* dout = dataconn("file list", (off_t)-1, "w", 0); */
        transflag++;
        if (dout != NULL) {

            show_fullinfo = showlots;
#if defined(HAVE_FTW)
            ftw(path, check_newer, -1);
#else
            treewalk(path, check_newer, -1, NULL);
#endif

            if (ferror(dout) != 0)
                perror_reply(550, "Data connection");
            else
                reply(226, "Transfer complete.");

            (void) fclose(dout);
            data = -1;
        }
    } else
        reply(501, "Bad DATE format");
    transflag = 0;
}

int
type_match(char *typelist)
{
    if (anonymous && strcasestr(typelist, "anonymous"))
        return (1);
    if (guest && strcasestr(typelist, "guest"))
        return (1);
    if (!guest && !anonymous && strcasestr(typelist, "real"))
        return (1);

    return (0);
}

int
path_compare(char *p1, char *p2)
{
    if ( fnmatch(p1, p2, NULL) != FNM_NOMATCH )
        return(strlen(p1));
    else
        return(-1);
}

void
expand_id(void)
{
    struct aclmember *entry = NULL;
    struct passwd *pwent;
    struct group *grent;
    char buf[BUFSIZ];

    while (getaclentry("upload", &entry) && ARG0 && ARG1 && ARG2 != NULL) {
        if (ARG3 && ARG4) {
            pwent = getpwnam(ARG3);
            grent = getgrnam(ARG4);

            if (pwent)  sprintf(buf, "%d", pwent->pw_uid);
            else        sprintf(buf, "%d", 0);
            ARG3 = (char *) malloc(strlen(buf) + 1);
            strcpy(ARG3, buf);

            if (grent)  sprintf(buf, "%d", grent->gr_gid);
            else        sprintf(buf, "%d", 0);
            ARG4 = (char *) malloc(strlen(buf) + 1);
            strcpy(ARG4, buf);
        }
    }
}

int
fn_check(char *name)
{
  /* check to see if this is a valid file name... path-filter <type>
   * <message_file> <allowed_charset> <disallowed> */

  struct aclmember *entry = NULL;
  int   j;
  char *sp;
  char *path;
#ifdef M_UNIX
# ifdef REGEX
  char *regp;
# endif
#endif

#ifdef REGEXEC
  regex_t regexbuf;
  regmatch_t regmatchbuf;
#endif

  while (getaclentry("path-filter", &entry) && ARG0 != NULL) {
      if (type_match(ARG0) && ARG1 && ARG2) {

		  /*
		   * check *only* the basename
		   */

		  if (path = strrchr(name, '/'))  ++path;
		  else	path = name;

          /* is it in the allowed character set? */
#if defined(REGEXEC)
          if (regcomp(&regexbuf, ARG2, REG_EXTENDED|REG_ICASE) != 0) {
              reply(553, "REGEX error");
#elif defined(REGEX)
          if ((sp = regcmp(ARG2, (char *) 0)) == NULL) {
              reply(553, "REGEX error");
#else
          if ((sp = re_comp(ARG2)) != 0) {
              perror_reply(553, sp);
#endif
              return(0);
          }
#if defined(REGEXEC)
          if (regexec(&regexbuf, path, 1, &regmatchbuf, 0) != 0) {
#elif defined(REGEX)
# ifdef M_UNIX
          regp = regex(sp, path);
          free(sp);
          if (regp == NULL) {
# else
          if ((regex(sp, path)) == NULL) {
# endif
#else
          if ((re_exec(path)) != 1) {
#endif
              pr_mesg(553, ARG1);
              reply(553, "%s: Permission denied. (Filename (accept))", name);
              return(0);
          }
          /* is it in any of the disallowed regexps */

          for (j = 3; j < MAXARGS; ++j) {
              /* ARGj == entry->arg[j] */
              if (entry->arg[j]) {
#if defined(REGEXEC)
                  if (regcomp(&regexbuf, entry->arg[j], 
                        REG_EXTENDED|REG_ICASE) !=0) {
                      reply(553, "REGEX error");
#elif defined(REGEX)
                  if ((sp = regcmp(entry->arg[j], (char *) 0)) == NULL) {
                      reply(553, "REGEX error");
#else
                  if ((sp = re_comp(entry->arg[j])) != 0) {
                      perror_reply(553, sp);
#endif
                      return(0);
                  }
#if defined(REGEXEC)
                  if (regexec(&regexbuf, path, 1, &regmatchbuf, 0) == 0) {
#elif defined(REGEX)
# ifdef M_UNIX
                  regp = regex(sp, path);
                  free(sp);
                  if (regp != NULL) {
# else
                  if ((regex(sp, path)) != NULL) {
# endif
#else
                  if ((re_exec(path)) == 1) {
#endif
                      pr_mesg(553, ARG1);
                      reply(553, "%s: Permission denied. (Filename (deny))", name);
                      return(0);
                  }
              }
          }
      }
  }
  return(1);
}

int
dir_check(char *name, uid_t *uid, gid_t *gid, int *valid)
{
  struct aclmember *entry = NULL;

  int i,
    match_value = -1;
  char *ap2 = NULL,
       *ap3 = NULL,
       *ap4 = NULL,
       *ap6 = NULL;
  char cwdir[BUFSIZ];
  char path[BUFSIZ];
  char *sp;
  extern struct passwd *pw;

  *valid = 0;

  strcpy(path, name);
  if (sp = strrchr(path, '/'))  *sp = '\0';
  else strcpy(path, ".");

  if ((realpath(path, cwdir)) == NULL) {
      perror_reply(553, "Could not determine cwdir");
      return(0);
  }

  i = match_value;
  while (getaclentry("upload", &entry) && ARG0 && ARG1 && ARG2 != NULL) {
      if ( (!strcmp(ARG0, pw->pw_dir))  &&
		   ((i = path_compare(ARG1, cwdir)) >= match_value) ) {
          match_value = i;
          ap2 = ARG2;
          if (ARG3)  ap3 = ARG3;
          else       ap3 = NULL;
          if (ARG4)  ap4 = ARG4;
          else       ap4 = NULL;
          if (ARG6)  ap6 = ARG6;
          else       ap6 = NULL;
      }
  }

  if ( ((ap2 && !strcasecmp(ap2, "no")) && (ap3 && strcasecmp(ap3, "dirs"))) || 
        (ap3 && !strcasecmp(ap3, "nodirs")) ||
		(ap6 && !strcasecmp(ap6, "nodirs")) ) {
      reply(530, "%s: Permission denied.  (Upload)", name);
      return(0);
  }

  if (ap3)
     *uid = atoi(ap3);    /* the uid  */
  if (ap4) {
     *gid = atoi(ap4);    /* the gid  */
     *valid = 1;
   }
  return(1);
}

int
upl_check(char *name, uid_t *uid, gid_t *gid, int *f_mode, int *valid)
{
  int  match_value = -1;
  char cwdir[BUFSIZ];
  char path[BUFSIZ];
  char *sp;
  int  i;

  char *ap1 = NULL,
   *ap2 = NULL,
   *ap3 = NULL,
   *ap4 = NULL,
   *ap5 = NULL;

  struct aclmember *entry = NULL;
  extern struct passwd *pw;

  *valid = 0;

      /* what's our current directory? */

      strcpy(path, name);
      if (sp = strrchr(path, '/'))  *sp = '\0';
      else strcpy(path, ".");

      if ((realpath(path, cwdir)) == NULL) {
          perror_reply(553, "Could not determine cwdir");
          return(-1);
      }

      /* we are doing a "best match"... ..so we keep track of what "match
       * value" we have received so far... */

      entry = NULL;
      match_value = -1;
      i = match_value;
      while (getaclentry("upload", &entry) && ARG0 && ARG1 && ARG2 != NULL) {
          if ( (!strcmp(ARG0, pw->pw_dir))  &&
		       ((i = path_compare(ARG1, cwdir)) >= match_value) ) {
              match_value = i;
              ap1 = ARG1;
              ap2 = ARG2;
              if (ARG3) ap3 = ARG3;
              else      ap3 = NULL;
              if (ARG4) ap4 = ARG4;
              else      ap4 = NULL;
              if (ARG5) ap5 = ARG5;
              else      ap5 = NULL;
          }
      }

      if (ap3 && ( (!strcasecmp("dirs",ap3)) || (!strcasecmp("nodirs", ap3)) ))
        ap3 = NULL;

      /* if we did get matches... ..else don't do any of this stuff */
      if (match_value >= 0) {
          if (!strcasecmp(ap2, "yes")) {
              if (ap3)
                  *uid = atoi(ap3);    /* the uid  */
              if (ap4) {
                  *gid = atoi(ap4);    /* the gid  */
		  *valid = 1;
		}
              if (ap5)
                  sscanf(ap5, "%o", f_mode); /* the mode */
          } else {
              reply(553, "%s: Permission denied. (Upload)", name);
              return(-1);
          }
      } else {
          /*
           * upload defaults to "permitted"
           */
          return(1);
      }

  return(match_value);
}

int
del_check(char *name)
{
  int pdelete = 1;
  struct aclmember *entry = NULL;

  while (getaclentry("delete", &entry) && ARG0 && ARG1 != NULL) {
      if (type_match(ARG1))
          if (*ARG0 == 'n')
              pdelete = 0;
  }

  if (!pdelete) {
      reply(553, "%s: Permission denied. (Delete)", name);
      return(0);
  } else {
      return(1);
  }
}
