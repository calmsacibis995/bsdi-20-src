/*
 * Copyright (C) 1992, BSDI
 *
 * regex.c - Compatibility routines that implement the old and crufty
 *	     re_comp/re_exec interface.
 *
 * Derived from code contributed by: James da Silva
 */
#include <sys/param.h>
#include <sys/types.h>
#include <regex.h>

#define MAX_ERRSTR 80

regex_t _re_regexp;
char    _re_errstr[MAX_ERRSTR];

/* 
 * returns NULL if string compiled correctly, an error message otherwise 
 */
char *
re_comp(s)
	const char *s;
{
	int err;

	if (s && (err = regcomp(&_re_regexp, s, 0))) {
		regerror(err, &_re_regexp, _re_errstr, MAX_ERRSTR);
		return(_re_errstr);
	}
	return (NULL);
}

/* 
 * returns 1 if string matches last regexp, 0 if failed, -1 if error 
 */
int 
re_exec(s)
	const char *s;
{

	switch (regexec(&_re_regexp, s, 0, NULL, 0)) {
	case 0:
		return (1);
	case REG_NOMATCH:
		return (0);
	}
	return (-1);
}
