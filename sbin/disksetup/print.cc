/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI print.cc,v 2.1 1995/02/03 07:15:53 polk Exp	*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "screen.h"

void
print(char **str)
{
    printf("\n");
    while (*str)
	printf("%s\n", *str++);
}

int
request_yesno(char **prompt, int def)
{
    char buf[64];
    int t;

    do {
	char **p = prompt;

    	while (p[1])
	    printf("%s\n", *p++);
	printf("%s [%s] ", *p, def ? "YES" : "NO");
	if (!fgets(buf, sizeof(buf), stdin))
	    return(def);
    } while ((t = truth(buf, def)) == -1);
    return(t);
}

int
request_inform(char **prompt)
{
    char **p = prompt;
    char buf[64];

    while (p[1])
	printf("%s\n", *p++);
    printf("%s ", *p);
    fgets(buf, sizeof(buf), stdin);
    return(1);
}

int
request_number(char **prompt, int def, int min, int max)
{
    char buf[64];
    int t;

    do {
	char **p = prompt;

    	while (p[1])
	    printf("%s\n", *p++);
	printf("%s [%d] ", *p, def);
	if (!fgets(buf, sizeof(buf), stdin))
	    return(def);

	char *e;
	t = int(strtol(buf, &e, 0));
    	if (e == buf) {
	    while (isspace(*e) || *e == '\n')
		++e;
	    if (!*e)
		return(def);
    	}
	if (e) {
	    while (isspace(*e) || *e == '\n')
		++e;
	    if (*e)
		t = min - 1;
    	}
    } while (t < min || (max && t > max));
    return(t);
}

char *
request_string(char **prompt, char *def)
{
    static char buf[1024];
    char *s, *e;

    do {
	char **p = prompt;

    	while (p[1])
	    printf("%s\n", *p++);
    	printf("%s ", *p);
    	if (def)
    	    printf("[%s] ", def);
	if(!fgets(buf, sizeof(buf), stdin))
	    return(0);
	s = buf;
    	while (*s == ' ' || *s == '\t' || *s == '\n')
	    ++s;
    	e = s;
    	while (*e && *e != ' ' && *e != '\t' && *e != '\n')
	    ++e;
	*e = 0;
    	if (*s == '\0')
	    s = def;
    } while (s && *s == '\0');
    return(s);
}
