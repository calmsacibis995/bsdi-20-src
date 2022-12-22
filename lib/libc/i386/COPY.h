/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI COPY.h,v 2.1 1995/02/03 06:28:22 polk Exp
 */

/*
 * Get parameters into the right registers for string instructions.
 */
#define	COPY_PROLOGUE() \
	pushl %esi; \
	pushl %edi; \
	movl 12(%esp),%esi;	/* source */ \
	movl 16(%esp),%edi;	/* destination */ \
	movl 20(%esp),%ecx	/* length */

#define	MEM_COPY_PROLOGUE() \
	pushl %esi; \
	pushl %edi; \
	movl 12(%esp),%edi;	/* destination */ \
	movl 16(%esp),%esi;	/* source */ \
	movl 20(%esp),%ecx	/* length */

#define	COPY_EPILOGUE() \
	popl %edi; \
	popl %esi; \
	ret

/*
 * Common code for strcpy() and strcat().
 * We size the source string in addition to loading the arguments.
 */
#define	STR_PROLOGUE() \
	pushl %esi; \
	pushl %edi; \
\
	movl 16(%esp),%edi; \
	xorl %eax,%eax; \
	movl $-1,%ecx; \
\
	cld; \
	repne; scasb; \
\
	negl %ecx; \
	decl %ecx; \
\
	movl 12(%esp),%edi; \
	movl 16(%esp),%esi

/*
 * Common code for strncpy() and strncat().
 */
#define	STRN_PROLOGUE() \
	pushl %esi; \
	pushl %edi; \
\
	movl 16(%esp),%edi; \
	xorl %eax,%eax; \
	movl 20(%esp),%edx; \
	movl %edx,%ecx; \
	testl %edi,%edi; /* clear z bit */ \
\
	cld; \
	repne; scasb; \
	jnz Lstrnpro_notfound; \
\
	incl %ecx; \
	subl %ecx,%edx; \
Lstrnpro_notfound: \
	movl %edx,%ecx; \
\
	movl 12(%esp),%edi; \
	movl 16(%esp),%esi

/*
 * Copy %ecx bytes from (%esi) to (%edi).
 * Initial word alignment brings best performance;
 * we expect that most strings will be aligned
 * and don't try to align ourselves.
 */
#define	COPY() \
	cld; \
	movl %ecx,%eax; \
	andl $3,%eax; \
	shrl $2,%ecx; \
	rep; movsl; \
	movl %eax,%ecx; \
	rep; movsb

/*
 * Like COPY(), but start moving bytes at the end.
 */
#define	COPY_REVERSE() \
	std;		/* evidently reverse copying is sexually transmitted */\
	addl %ecx,%esi; \
	decl %esi; \
	addl %ecx,%edi; \
	decl %edi; \
	movl %ecx,%eax; \
	andl $3,%ecx; \
	shrl $2,%eax; \
	rep; movsb; \
	movl %eax,%ecx; \
	subl $3,%esi;	/* point at first byte in word, not last byte */ \
	subl $3,%edi; \
	rep; movsl; \
	cld		/* XXX necessary? */
