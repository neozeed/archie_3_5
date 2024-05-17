#include "error.h"
#include "gopher.h"
#include "misc.h"
#include "pattrib.h"
#include "prosp.h"
#include "protos.h"
#include "strval.h"
#include "all.h"


static int gparse1 proto_((VLINK v, struct gopher_description *gd));
static int gparse2 proto_((VLINK v, struct gopher_description *gd));
static int gparse3 proto_((VLINK v, struct gopher_description *gd));


static void init_gopher_description(gd)
  struct gopher_description *gd;
{
  gd->type = '\0';
  gd->desc[0] = '\0';
  gd->sel[0] = '\0';
  gd->host[0] = '\0';
  gd->port = 0;
}


static int gparse1(v, gd)
  VLINK v;
  struct gopher_description *gd;
{
  init_gopher_description(gd);
  if (sscanf(v->hsoname, "GOPHER-GW/%[^(](%d)/%c/%[^\n]",
             gd->host, &gd->port, &gd->type, gd->sel) < 3)
  {
    return 0;
  }
  strcpy(gd->desc, (v->name ? v->name : "[no description]"));
  return 1;
}


static StrVal interptab[] =
{
  { "DIRECTORY", '1' },
  { "DOCUMENT",  '0' },
  { "PORTAL",    '8' },         /* bug: might be tn3270; we don't know... */
  { "SEARCH",    '7' },

  { STRVAL_END,  '9' }          /* bug: might be a GIF; we don't know... */
};


static int gparse2(v, gd)
  VLINK v;
  struct gopher_description *gd;
{
  PATTRIB at;

  init_gopher_description(gd);
  if (sscanf(v->host, "%[^(](%d)", gd->host, &gd->port) != 2)
  {
    return 0;
  }

  strcpy(gd->desc, v->name);    /* bug? */
  strcpy(gd->sel, v->hsoname);

  if ( ! (at = nextAttr("OBJECT-INTERPRETATION", v->lattrib)))
  {
    return 0;
  }

  gd->type = sval(at->value.sequence->token, interptab);

  return 1;
}


static int gparse3(v, gd)
  VLINK v;
  struct gopher_description *gd;
{
  PATTRIB at;

  init_gopher_description(gd);
  if ( ! (at = nextAttr("GOPHER-MENU-ITEM", v->lattrib)))
  {
    return 0;
  }

  /*  
   *  Return failure here, too, because it's in a format we don't know about.
   */  
  efprintf(stderr, "%s: GOPHER-MENU-ITEM is `%s'.\n",
           logpfx(), at->value.sequence->token);
  return 0;
}


int gopher_parse(v, gd)
  VLINK v;
  struct gopher_description *gd;
{
  if ( ! (gparse3(v, gd) || gparse1(v, gd) || gparse2(v, gd)))
  {
    efprintf(stderr, "%s: gopher_parse: bad Gopher item link, `%s:%s'.\n",
             logpfx(), v->name, v->hsoname);
    return 0;
  }

  return 1;
}
