#ifndef GOPHER_H
#define GOPHER_H

#include <sys/param.h>
#include <netdb.h>	/* This is required to avoid bug under Solaris
                           from the prospero code where they redefine 
                           MAXHOSTNAMELEN to 64 if not already defined
                           causing modules to have different definitions
			   for structure below....
                        */
#include "prosp.h"


struct gopher_description
{
  char type;
  char desc[512];
  char sel[2048];
  char host[MAXHOSTNAMELEN + 1];
  int port;
};


extern int gopher_parse proto_((VLINK v, struct gopher_description *gd));

#endif
