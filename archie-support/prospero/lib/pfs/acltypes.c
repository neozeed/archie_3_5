/*
 * Copyright (c) 1991 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#define NULL 0

char	*acltypes[] = {
	/*  0 */ "UNINITIALIZED ACL -- You should never see this message.", 
	/*  1 */ "NONE", 
	/*  2 */ "DEFAULT", 
        /*  3 */ "SYSTEM", 
        /*  4 */ "OWNER",
        /*  5 */ "DIRECTORY",
	/*  6 */ "CONTAINER",
        /*  7 */ "ANY",
	/*  8 */ "AUTHENT",
	/*  9 */ "LGROUP", 
        /* 10 */ "GROUP", 
	/* 11 */ "ASRTHOST",
	/* 12 */ "TRSTHOST",
	/* 13 */ "IETF-AAC",
	/* 14 */ "SUBSCRIPTION",
	/* 15 */ "DEFCONT ACL -- You should never see this message.",
	/* 16 */ NULL};

