#	$NetBSD: Makefile.inc,v 1.5 1995/02/27 13:28:19 cgd Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# regex sources
.PATH: ${.CURDIR}/regex

CFLAGS+=-DPOSIX_MISTAKE

SRCS+=	regcomp.c regerror.c regexec.c regfree.c

MAN+=	regex.3 re_format.7

MLINKS+=regex.3 regcomp.3 regex.3 regexec.3 regex.3 regerror.3
MLINKS+=regexec.3 regfree.3
