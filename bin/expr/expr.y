/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

%{

static const char rcsid[] = "expr.y,v 2.1 1995/02/03 05:44:25 polk Exp";
static const char copyright[] = "Copyright (c) 1991 Berkeley Software Design, Inc.";

#include <sys/types.h>
#include <err.h>
#include <regex.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define	null(s)	((s)[0] == '\0' || ((s)[0] == '0' && (s)[1] == '\0'))

#define	relop(result, op, s1, s2) \
{ \
	char *e1, *e2; \
	register long l1, l2, value; \
\
	l1 = strtol((s1), &e1, 10); \
	l2 = strtol((s2), &e2, 10); \
\
	if (e1 == (s1) || e2 == (s2)) \
		value = strcmp((s1), (s2)) op 0; \
	else \
		value = l1 op l2; \
	(result) = value ? "1" : "0"; \
}

#define	arithop(result, op, s1, s2) \
{ \
	char *e1, *e2; \
	register long l1, l2; \
\
	l1 = strtol((s1), &e1, 10); \
	l2 = strtol((s2), &e2, 10); \
\
	if (e1 == (s1)) \
		errx(2, "first operand `%s' to `%s' is not a number", \
		    (s1), #op); \
	else if (e2 == (s2)) \
		errx(2, "second operand `%s' to `%s' is not a number", \
		    (s2), #op); \
	else if (l2 == 0 && (#op[0] == '/' || #op[0] == '%')) \
		errx(2, "second operand to `%s' is 0", #op); \
	if (((result) = malloc(12)) == 0) \
		err(3, 0); \
	sprintf((result), "%ld", (l1 op l2)); \
}

int status = 2;

char *matchop(const char *, char *);

%}

/*
 * This list of operators follows POSIX.2 4.22.7, Table 4-6.
 */

%left '|'
%left '&'
%left '<' LE '=' NE GE '>'
%left '+' '-'
%left '*' '/' '%'
%left ':'
%nonassoc '(' ')'

%union { char *s; }

%token <s> ID
%type <s> expr

%%

start:	expr
		{ puts($1); status = null($1); }
	;

expr:	expr '|' expr
		{ if (null($1)) $$ = $3; else $$ = $1; }
	| expr '&' expr
		{ if (null($1) || null($3)) $$ = "0"; else $$ = $1; }
	| expr '<' expr
		{ relop($$, <, $1, $3); }
	| expr LE expr
		{ relop($$, <=, $1, $3); }
	| expr '=' expr
		{ relop($$, ==, $1, $3); }
	| expr NE expr
		{ relop($$, !=, $1, $3); }
	| expr GE expr
		{ relop($$, >=, $1, $3); }
	| expr '>' expr
		{ relop($$, >, $1, $3); }
	| expr '+' expr
		{ arithop($$, +, $1, $3); }
	| expr '-' expr
		{ arithop($$, -, $1, $3); }
	| expr '*' expr
		{ arithop($$, *, $1, $3); }
	| expr '/' expr
		{ arithop($$, /, $1, $3); }
	| expr '%' expr
		{ arithop($$, %, $1, $3); }
	| expr ':' expr
		{ $$ = matchop($1, $3); }
	| '(' expr ')'
		{ $$ = $2; }
	| ID
	;

%%

char **tokens;

int
main(int argc, char **argv)
{

	tokens = &argv[1];

	yyparse();

	return (status);
}

/*
 * XXX We make no effort to recognize operators as strings in a string context
 * since this is hard and the Unix expr doesn't appear to handle it either.
 */
int
yylex()
{
	register char *s;

	if ((s = yylval.s = *tokens++) == 0)
		return (0);

	if (s[0] == '\0')
		return (ID);

	if (s[1] == '\0') {
		if (strchr("|&<=>+-*/%:()", s[0]))
			return (s[0]);
		return (ID);
	}

	if (s[2] == '\0' && s[1] == '=')
		switch (s[0]) {
		case '<':	return (LE);
		case '>':	return (GE);
		case '!':	return (NE);
		}

	return (ID);
}

char *
matchop(const char *s, char *pattern)
{
	regmatch_t pmatch[2];
	char errbuf[64];
	regex_t r;
	char *p, *q;
	char *anchor;
	size_t len;
	int v;
	int substring = 0;

	/*
	 * Ugliness: the return value depends on whether the BRE
	 * contains a substring match operator ("\(").
	 * If we find such an operator, we return a string,
	 * otherwise we return a number.
	 * We look for an open paren preceded by an odd number of backslashes.
	 */
	for (p = pattern; p = strchr(p, '('); ++p) {
		for (q = p - 1; q >= pattern && *q == '\\'; --q)
			;
		if ((p - ++q & 01) == 1) {
			substring = 1;
			break;
		}
	}

	len = strlen(pattern);
	if ((anchor = malloc(len + 2)) == 0)
		err(3, 0);
	if (*pattern == '^')
		/* unspecified POSIX.2 behavior; we just accept the '^' */
		memcpy(anchor, pattern, len + 1);
	else {
		anchor[0] = '^';
		memcpy(&anchor[1], pattern, len + 1);
	}

	if ((v = regcomp(&r, anchor, REG_BASIC)) != 0) {
		regerror(v, &r, errbuf, sizeof (errbuf));
		errx(2, "invalid basic regular expression `%s': %s",
		    pattern, errbuf);
	}

	if ((v = regexec(&r, s, 2, pmatch, 0)) != 0 &&
	    v != REG_NOMATCH) {
		regerror(v, &r, errbuf, sizeof (errbuf));
		errx(2, "invalid basic regular expression `%s': %s",
		    pattern, errbuf);
	}
	regfree(&r);
	free(anchor);

	if (substring) {
		len = pmatch[1].rm_eo - pmatch[1].rm_so;
		if (v == REG_NOMATCH || len == 0)
			return ("");
		if ((p = malloc(len + 1)) == 0)
			err(3, 0);
		memcpy(p, s + pmatch[1].rm_so, len);
		p[len] = '\0';
		return (p);
	}

	len = pmatch[0].rm_eo - pmatch[0].rm_so;
	if (v == REG_NOMATCH || len == 0)
		return ("0");

	if ((p = malloc(21)) == 0)
		err(3, 0);
	sprintf(p, "%ld", len);
	return (p);
}

void
yyerror(const char *message)
{

	errx(2, "%s, last argument was `%s'", message, *--tokens);
}
