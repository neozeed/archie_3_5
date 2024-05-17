#ifdef __STDC__
# include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>
#include "aprint.h"
#include "epath.h"
#include "ppc.h"
#include "print_results.h"
#include "url.h"
#include "all.h"


static int gopher_dir_item proto_((FILE *ofp, struct gopher_description *gd));
static int gopher_file_item proto_((FILE *ofp, struct gopher_description *gd));


static int gopher_dir_item(ofp, gd)
  FILE *ofp;
  struct gopher_description *gd;
{
  char *qsel;
  int ret = 0;

  if ( ! (qsel = url_quote(gd->sel, malloc)))
  {
    efprintf(stderr, "%s: print_gopher: error quoting selector string: %m.\n",
             logpfx());
  }
  else
  {
    ret = efprintf(ofp, "<LI><A HREF=\"gopher://%s:%d/%c%s\">%s</A>\r\n",
                   gd->host, gd->port, gd->type, qsel, gd->desc);
    free(qsel);
  }

  return ret;
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


int gopher_print(ofp, req, print_as_dir)
  FILE *ofp;
  Request *req;
  int print_as_dir;
{
  VLINK v;
  struct gopher_description gd;
  
  print_vllength(ofp, req);
  efprintf(ofp, "<P>%s\r\n", print_as_dir ? "<MENU>" : "<PRE>");
  for (v = req->res; v; v = v->next)
  {
    if (gopher_parse(v, &gd))
    {
      if (print_as_dir) gopher_dir_item(ofp, &gd);
      else gopher_file_item(ofp, &gd);
    }
  }
  efprintf(ofp, "%s</BODY>\r\n", print_as_dir ? "</MENU>" : "</PRE>");

  return 1;
}
