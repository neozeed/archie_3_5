#include <stdio.h>
#ifdef SOLARIS
#include <netdb.h>	/* This is required to avoid bug under Solaris
                           from the prospero code where they redefine 
                           MAXHOSTNAMELEN to 64 if not already defined
                           causing modules to have different definitions
			   for structure below....
                        */

#endif
#ifdef AIX
#include <sys/param.h>	/* This is required to avoid bug under AIX
                           from the prospero code where they redefine 
                           MAXHOSTNAMELEN to 64 if not already defined
                           causing modules to have different definitions
			   for structure below....
                        */
#endif
#include "aprint.h"
#include "ppc.h"
#include "protos.h"


static int gopher_dir_item proto_((FILE *ofp, struct gopher_description *gd));
static int gopher_file_item proto_((FILE *ofp, struct gopher_description *gd));


static int gopher_dir_item(ofp, gd)
  FILE *ofp;
  struct gopher_description *gd;
{
  return efprintf(ofp, "%c%s\t%s\t%s\t%d\r\n",
                  gd->type, gd->desc, gd->sel, gd->host, gd->port);
}


static int gopher_file_item(ofp, gd)
  FILE *ofp;
  struct gopher_description *gd;
{
  return efprintf(ofp,
                  "Type:                   %c\r\n"
                  "Menu Description:       %s\r\n"
                  "Gopher Selector String: %s\r\n"
                  "Host:                   %s\r\n"
                  "Port:                   %d\r\n"
                  "\r\n",
                  gd->type, gd->desc, gd->sel, gd->host, gd->port);
}


void gopher_print(ofp, v, print_as_dir, here)
  FILE *ofp;
  VLINK v;
  int print_as_dir;
  const Where here;
{
  struct gopher_description gd;
  
  for (; v; v = v->next)
  {
    if (gopher_parse(v, &gd))
    {
      if (print_as_dir) gopher_dir_item(ofp, &gd);
      else gopher_file_item(ofp, &gd);
    }
  }
}
