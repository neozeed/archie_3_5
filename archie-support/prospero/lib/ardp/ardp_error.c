/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * Written  by swa 11/93     to handle error reporting consistently in libardp
 */

#include <usc-license.h>

#include <ardp.h>
#include <perrno.h>         /* For perrno */

/* See lib/ardp/usc_lic_str.c */
extern char *usc_license_string;
/* This is used in Prospero in lib/pfs/perrmesg.c.  Change it there too if you
   change it here. */
char *ardp___please_do_not_optimize_me_out_of_existence;

/* perrno is declared in ardp_perrno.c in case some older applications
   have explicitly declared their own perrno variable (a practice we now
   discourage). */

/* This function definition is shadowed in lib/pfs/perrmesg.c.  Change it there
   too if you change it here. */

void
p_clear_errors(void)
{
    ardp___please_do_not_optimize_me_out_of_existence = usc_license_string;
    perrno = 0;
}

