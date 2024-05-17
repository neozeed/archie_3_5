/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>
#include <pfs.h>
#include <psrv.h>
#include "gopher.h"
#include <mitra_macros.h>

static GLINK lfree = NULL;
int glink_count = 0;
int glink_max = 0;


/* 
  glalloc(): Allocate and initialize GLINK structure. 
  Signal out_of_memory() on failure. 
*/

GLINK 
glalloc(void)
{
    GLINK	gl;
    
    TH_STRUC_ALLOC(glink,GLINK,gl);
    /* Initialize and fill in default values */
    gl->type = '0';
    gl->name = NULL;
    gl->selector = NULL;
    gl->host = NULL;
    gl->port = 0;
    gl->protocol_mesg = NULL;
    return gl;
}


void 
glfree(GLINK gl)
{
    /* Free any bits of memory pointed to by members of the structure. */
    if (gl->name) stfree(gl->name);
    if (gl->host) stfree(gl->host);
    if (gl->selector) stfree(gl->selector);
    if (gl->protocol_mesg) stfree(gl->protocol_mesg);
    /* Free the structure itself. */
    TH_STRUC_FREE(glink,GLINK,gl);
}

void 
gllfree(GLINK gl)
{
    TH_STRUC_LFREE(GLINK,gl,glfree)
}

