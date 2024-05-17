/*
 * Copyright (c) 1991 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <ardp.h>
#include <pfs.h>
#include <perrno.h>
#include "ansi_compat.h"

#include "protos.h"

static int assign_priority PROTO((RREQ r1));


int arch_prioritize_request(r1, r2)
  RREQ r1;
  RREQ r2;
{
	if ( ! r1->pf_priority) r1->pf_priority = assign_priority(r1);
	if ( ! r2->pf_priority) r2->pf_priority = assign_priority(r2);

	if (r1->pf_priority == r2->pf_priority) return 0;
	else if (r1->pf_priority < r2->pf_priority) return -1;
	else return 1;
}


static int assign_priority(r1)
  RREQ r1;
{
	char stype;
	char *arg_ptr;
	int	maxhit = 0;
	int	maxhitpm = 0;
	int	maxmatch = 0;
	int	offset;
	int	retval;
	int	tmp;

	/* Result is probably cached, use it or lose it */
	if (r1->prcvd_thru > 0) return 2;

	arg_ptr = sindex(r1->rcvd->start, "ARCHIE");
	if( ! arg_ptr) return 1;

	arg_ptr = sindex(arg_ptr, "MATCH");
	if ( ! arg_ptr) return 3;

	tmp = sscanf(arg_ptr, "MATCH(%d,%d,%d,%d,%c", &maxhit, &maxmatch,
               &maxhitpm, &offset, &stype);
	if (tmp != 5) tmp = sscanf(arg_ptr, "MATCH(%d,%d,%c", &maxhit,
                             &offset, &stype);
	if (tmp < 3) return 4;

	if (stype == '=') retval = 0;
	else if ((stype == 'r') || (stype == 'x')) retval = 700;
	else if ((stype == 'R') || (stype == 'X')) retval = 800;
	else retval = 100;

	/* If old format request, then add penalty */
	if (tmp != 5) retval += 100;

	tmp = maxhit;
	if (offset > 0) tmp += offset;

	if (tmp > 10000) retval += 10000;
	else if (tmp > 100) retval+= tmp;
	else retval+= 100;

	if (sindex(arg_ptr, "gif") || sindex(arg_ptr, "GIF")) retval += 20000;

	return retval;
}
