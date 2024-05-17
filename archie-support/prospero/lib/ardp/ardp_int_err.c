/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * Written  by swa 12/93     to handle error reporting consistently in libardp
 */

#include <usc-license.h>

#include <ardp.h>


/* Data definition.  This may be reset by any client. */
int (*internal_error_handler)(const char file[], int line, const char mesg[]) = 0;

