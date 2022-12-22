#	BSDI	Makefile,v 2.1 1995/02/03 06:48:38 polk Exp

PROG=	popper
SRCS=	pop_dele.c pop_dropcopy.c pop_dropinfo.c \
	pop_get_command.c pop_get_subcommand.c pop_init.c \
	pop_last.c pop_list.c pop_log.c pop_lower.c \
	pop_msg.c pop_parse.c pop_pass.c pop_quit.c \
	pop_rset.c pop_send.c pop_stat.c pop_updt.c \
	pop_user.c pop_xtnd.c pop_xmit.c popper.c
MAN8=	popper.0
CFLAGS+=-DBIND43 -DHAVE_VSPRINTF -DRPOP

.include <bsd.prog.mk>
