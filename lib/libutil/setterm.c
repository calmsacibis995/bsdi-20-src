/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI setterm.c,v 2.1 1995/02/03 09:18:35 polk Exp
 */

#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#define	F(s)	s, (sizeof s - 1)

struct flag_table;

static const char *ignore __P((const struct flag_table *, struct termios *,
    const char *, int, long));
static const char *cchar __P((const struct flag_table *, struct termios *,
    const char *, int, long));
static const char *iflags __P((const struct flag_table *, struct termios *,
    const char *, int, long));
static const char *oflags __P((const struct flag_table *, struct termios *,
    const char *, int, long));
static const char *cflags __P((const struct flag_table *, struct termios *,
    const char *, int, long));
static const char *csize __P((const struct flag_table *, struct termios *,
    const char *, int, long));
static const char *lflags __P((const struct flag_table *, struct termios *,
    const char *, int, long));
static const char *speed __P((const struct flag_table *, struct termios *,
    const char *, int, long));
static const char *raw __P((const struct flag_table *, struct termios *,
    const char *, int, long));

static const struct flag_table {
	char		*flag;
	size_t		len;
	const char	*(*function) __P((const struct flag_table *,
			    struct termios *, const char *, int, long));
	u_long		value;
} flag_table[] = {
	F("cchars"),		ignore,		0,
	F("eof"),		cchar,		VEOF,
	F("eol2"),		cchar,		VEOL2,
	F("eol"),		cchar,		VEOL,
	F("erase"),		cchar,		VERASE,
	F("werase"),		cchar,		VWERASE,
	F("kill"),		cchar,		VKILL,
	F("reprint"),		cchar,		VREPRINT,
	F("intr"),		cchar,		VINTR,
	F("quit"),		cchar,		VQUIT,
	F("susp"),		cchar,		VSUSP,
	F("dsusp"),		cchar,		VDSUSP,
	F("start"),		cchar,		VSTART,
	F("stop"),		cchar,		VSTOP,
	F("lnext"),		cchar,		VLNEXT,
	F("discard"),		cchar,		VDISCARD,
	F("min"),		cchar,		VMIN,
	F("time"),		cchar,		VTIME,
	F("status"),		cchar,		VSTATUS,

	F("iflags"),		ignore,		0,
	F("ignbrk"),		iflags,		IGNBRK,
	F("brkint"),		iflags,		BRKINT,
	F("ignpar"),		iflags,		IGNPAR,
	F("parmrk"),		iflags,		PARMRK,
	F("inpck"),		iflags,		INPCK,
	F("istrip"),		iflags,		ISTRIP,
	F("inlcr"),		iflags,		INLCR,
	F("igncr"),		iflags,		IGNCR,
	F("icrnl"),		iflags,		ICRNL,
	F("ixon"),		iflags,		IXON,
	F("ixoff"),		iflags,		IXOFF,
	F("ixany"),		iflags,		IXANY,
	F("imaxbel"),		iflags,		IMAXBEL,

	F("oflags"),		ignore,		0,
	F("opost"),		oflags,		OPOST,
	F("onlcr"),		oflags,		ONLCR,
	F("oxtabs"),		oflags,		OXTABS,
	F("onoeot"),		oflags,		ONOEOT,

	F("cflags"),		ignore,		0,
	F("cignore"),		cflags,		CIGNORE,
	F("cs5"),		csize,		CS5,
	F("cs6"),		csize,		CS6,
	F("cs7"),		csize,		CS7,
	F("cs8"),		csize,		CS8,
	F("cstopb"),		cflags,		CSTOPB,
	F("cread"),		cflags,		CREAD,
	F("parenb"),		cflags,		PARENB,
	F("parodd"),		cflags,		PARODD,
	F("hupcl"),		cflags,		HUPCL,
	F("clocal"),		cflags,		CLOCAL,
	F("cts_oflow"),		cflags,		CCTS_OFLOW,
	F("rtscts"),		cflags,		CRTSCTS,
	F("rts_iflow"),		cflags,		CRTS_IFLOW,
	F("mdmbuf"),		cflags,		MDMBUF,

	F("lflags"),		ignore,		0,
	F("echoke"),		lflags,		ECHOKE,
	F("echoe"),		lflags,		ECHOE,
	F("echok"),		lflags,		ECHOK,
	F("echonl"),		lflags,		ECHONL,
	F("echoprt"),		lflags,		ECHOPRT,
	F("echoctl"),		lflags,		ECHOCTL,
	F("echo"),		lflags,		ECHO,
	F("isig"),		lflags,		ISIG,
	F("icanon"),		lflags,		ICANON,
	F("altwerase"),		lflags,		ALTWERASE,
	F("iexten"),		lflags,		IEXTEN,
	F("extproc"),		lflags,		EXTPROC,
	F("tostop"),		lflags,		TOSTOP,
	F("flusho"),		lflags,		FLUSHO,
	F("nokerninfo"),	lflags,		NOKERNINFO,
	F("pendin"),		lflags,		PENDIN,
	F("noflsh"),		lflags,		NOFLSH,

	F("baud"),		ignore,		0,
	F("speed"),		speed,		0,
	F("ispeed"),		speed,		1,
	F("ospeed"),		speed,		2,
	F("rows"),		ignore,		1,
	F("columns"),		ignore,		1,
	F("raw"),		raw,		0,

	0, 0,			0,		0,
};

/*
 * Bite off a reasonable flag string and return it.
 * Used for error reporting.
 */
static char *
get_flag(flags)
	const char *flags;
{
	static char *format = "mode flag error: unrecognized flag `";
	const char *e;
	char *r;

	for (e = flags; *e && (isalnum(*e) || *e == '_'); ++e)
		;
	if (e == flags)
		return (strdup("mode flag error: end of string or no flags"));
	if ((r = malloc(sizeof (format) + 1 + e - flags)) == 0)
		return (strerror(errno));
	strcpy(r, format);
	strncat(r, flags, e - flags);
	strcat(r, "'");
	return (r);
}

/*
 * Handle a keyword such as 'cchars:' which we want to skip over.
 * Also used to ignore rows/columns values, since termios doesn't cover them.
 */
static const char *
ignore(fp, t, flags, minus, number)
	const struct flag_table *fp;
	struct termios *t;
	const char *flags;
	int minus;
	long number;
{
	char *endp;

	flags += fp->len;

	if (fp->value && number == 0) {
		/* try to eat a number */
		strtol(flags, &endp, 10);
		flags = (const char *)endp;
	}

	return (flags);
}

/*
 * Process special tty characters.
 * The syntax is just like stty(1) output;
 * perhaps we should accept vis(1) output too?
 * This code is intensely prejudiced in favor of ASCII...
 */
static const char *
cchar(fp, t, flags, minus, number)
	const struct flag_table *fp;
	struct termios *t;
	const char *flags;
	int minus;
	long number;
{
	const char *p;
	char c = 0;

	p = flags + fp->len;
	while (isspace(*p))
		++p;
	if (*p == '=')
		++p;
	while (isspace(*p))
		++p;
	if (*p == '<' && memcmp(p, "<undef>", sizeof "<undef>" - 1) == 0) {
		t->c_cc[fp->value] = _POSIX_VDISABLE;
		return (p + sizeof "<undef>" - 1);
	}
	if (*p == 'M') {
		if (*++p == '-')
			++p;
		c |= 0x80;
	}
	if (*p == '^') {
		if (*++p == '?') {
			c |= 0x7f;
			++p;
		} else if (*p >= 0x40)
			c |= *p++ & 0x3f;
		else
			c |= '^';
	} else if (*p == '\0')
		/* no character given -- bad flag */
		return (0);
	else
		c |= *p++;

	t->c_cc[fp->value] = c;

	return (p);
}

static const char *
iflags(fp, t, flags, minus, number)
	const struct flag_table *fp;
	struct termios *t;
	const char *flags;
	int minus;
	long number;
{

	if (minus)
		t->c_iflag &= ~fp->value;
	else
		t->c_iflag |= fp->value;
	return (flags + fp->len);
}

static const char *
oflags(fp, t, flags, minus, number)
	const struct flag_table *fp;
	struct termios *t;
	const char *flags;
	int minus;
	long number;
{

	if (minus)
		t->c_oflag &= ~fp->value;
	else
		t->c_oflag |= fp->value;
	return (flags + fp->len);
}

static const char *
cflags(fp, t, flags, minus, number)
	const struct flag_table *fp;
	struct termios *t;
	const char *flags;
	int minus;
	long number;
{

	if (minus)
		t->c_cflag &= ~fp->value;
	else
		t->c_cflag |= fp->value;
	return (flags + fp->len);
}

static const char *
csize(fp, t, flags, minus, number)
	const struct flag_table *fp;
	struct termios *t;
	const char *flags;
	int minus;
	long number;
{

	t->c_cflag &= ~CSIZE;
	t->c_cflag |= fp->value;
	return (flags + fp->len);
}

static const char *
lflags(fp, t, flags, minus, number)
	const struct flag_table *fp;
	struct termios *t;
	const char *flags;
	int minus;
	long number;
{

	if (minus)
		t->c_lflag &= ~fp->value;
	else
		t->c_lflag |= fp->value;
	return (flags + fp->len);
}

static const char *
speed(fp, t, flags, minus, number)
	const struct flag_table *fp;
	struct termios *t;
	const char *flags;
	int minus;
	long number;
{
	char *endp;
	const char *p = flags;

	if (fp)
		p += fp->len;

	if (number == 0) {
		number = strtol(p, &endp, 10);
		p = (const char *)endp;
	}

	switch (fp ? fp->flag[0] : 0) {
	default:
		cfsetspeed(t, number);
		break;
	case 'i':
		cfsetispeed(t, number);
		break;
	case 'o':
		cfsetospeed(t, number);
		break;
	}

	return (p);
}

static const char *
raw(fp, t, flags, minus, number)
	const struct flag_table *fp;
	struct termios *t;
	const char *flags;
	int minus;
	long number;
{

	cfmakeraw(t);
	return (flags + fp->len);
}

/*
 * Set the given stty(1)-style flags in the given termios structure.
 * We return 0 for success, -1 for failure.
 * If failp is nonzero, *failp is set to the flag that didn't parse
 * or had an illegal value, otherwise 0.
 */
int
cfsetterm(t, flags, failp)
	struct termios *t;
	const char *flags;
	char **failp;
{
	const struct flag_table *fp;
	const char *p, *end_flags;
	char *endp;
	int minus;
	long number;

	if (failp)
		*failp = 0;

	if (flags == 0)
		return (0);

	end_flags = flags + strlen(flags);

	while (*flags) {
		minus = 0;
		number = 0;

		while (*flags && *flags != '-' && !isalnum(*flags))
			++flags;
		if (!*flags)
			break;

		if (*flags == '-') {
			minus = 1;
			++flags;
		} else
			minus = 0;

		if (isdigit(*flags)) {
			number = strtol(flags, &endp, 10);
			flags = (const char *)endp;
			while (isspace(*flags))
				++flags;
			if (!isalnum(*flags)) {
				p = speed(0, t, flags, 0, number);
				if (p == 0)
					goto bad_flag;
				flags = p;
				continue;
			}
		}

		for (fp = &flag_table[0]; fp->function; ++fp)
			if (fp->len <= end_flags - flags &&
			    memcmp(flags, fp->flag, fp->len) == 0 &&
			    !isalnum(flags[fp->len])) {
				if ((p = (*fp->function)(fp, t, flags, minus,
				    number)) == 0) {
				    	fp = 0;
				    	break;
				}
				flags = p;
				break;
			}

		if (!fp || !fp->flag) {
		bad_flag:
			if (failp)
				*failp = get_flag(flags);
			errno = EINVAL;
			return (-1);
		}
	}

	return (0);
}

/*
 * Similar to cfsetterm() but has an interface more like tcsetattr().
 */
int
setterm(f, action, flags, failp)
	int f, action;
	const char *flags;
	char **failp;
{
	struct termios t;

	if (failp)
		*failp = 0;

	if (tcgetattr(f, &t) == -1) {
		if (failp)
			*failp = strerror(errno);
		return (-1);
	}
	if (cfsetterm(&t, flags, failp) == -1)
		return (-1);
	if (tcsetattr(f, action, &t) == -1) {
		if (failp)
			*failp = strerror(errno);
		return (-1);
	}
	return (0);
}
