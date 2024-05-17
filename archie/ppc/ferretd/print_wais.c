#ifdef __STDC__
# include <stdlib.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "aprint.h"
#include "contents.h"
#include "ppc.h"
#include "print_results.h"
#include "protos.h"
#include "url.h"
#include "all.h"


static char *wais_hdln proto_((VLINK item));
static int multi_headlines proto_((FILE *ofp, VLINK item, const char *epath, const Where here));
static int single_headline proto_((FILE *ofp, VLINK item, const char *epath, const Where here));


static int multi_headlines(ofp, item, epath, here)
  FILE *ofp;
  VLINK item;
  const char *epath;
  const Where here;
{
  PATTRIB cat;
  PATTRIB hat;
  TOKEN hname;
  int first_attr = 1;
    
  /*  
   *  If the attribute BUNYIP-HEADLINE appears, it will contain a list of
   *  attribute names that should appear as the headline.
   */
  if ( ! (hat = nextAttr(HEADLINE, item->lattrib)))
  {
    return 0;
  }

  if ( ! (cat = get_contents(item)))
  {
    efprintf(stderr, "%s: multi_headlines: can't get CONTENTS attribute for `%s:%s'.\n",
             logpfx(), item->host, item->hsoname);
    return 0;
  }

  /*  
   *  Search the tagged contents for each attribute name appearing in
   *  the BUNYIP-HEADLINE list.
   */  
  for (hname = hat->value.sequence; hname; hname = hname->next)
  {
    PATTRIB a;
          
    for (a = cat; (a = nextAttr(CONTENTS, a)); a = a->next)
    {
      const char *atname;
            
      atname = nth_token_str(a->value.sequence, 1);
      if (strcasecmp(atname, hname->token) == 0)
      {
        const char *atval;
        const char *atpfx;
              
        if ( ! (atpfx = nth_token_str(a->value.sequence, 3)))
        {
          atpfx = "";
        }
        atval = nth_token_str(a->value.sequence, 2);
        if (first_attr)
        {
          efprintf(ofp, "<LI><A HREF=\"http://%s:%s/%s\"><STRONG>%s%s</STRONG></A>\r\n",
                   here.host, here.port, epath, atpfx, atval);
          first_attr = 0;
        }
        else
        {
          efprintf(ofp, "\t<BR>%s%s\r\n", atpfx, atval);
        }
      }
    }
  }

  return 1;
}


static int single_headline(ofp, item, epath, here)
  FILE *ofp;
  VLINK item;
  const char *epath;
  const Where here;
{
  return efprintf(ofp, "<LI><A HREF=\"http://%s:%s/%s\">%s</A>\r\n",
                  here.host, here.port, epath, wais_hdln(item));
}


static char *wais_hdln(item)
  VLINK item;
{
  return nuke_afix_ws(item->name);
}


/*  
 *  bug: what if we're to immediately print the contents, but the link
 *  also has a BUNYIP-REPLACE-WITH-URL attribute?  Good candidate for
 *  Location: header.
 */
int wais_print(ofp, req, print_as_dir)
  FILE *ofp;
  Request *req;
  int print_as_dir;
{
  PATTRIB cat;
  VLINK v;
  const char *epath;
  int ret = 0;
  
  if ( ! print_as_dir)
  {
    ret = efprintf(ofp, "Can't print this way, yet.\r\n");
    return 0;
  }

  v = req->res;
  /* bug: ugh! */
  if (immediatePrint(v, &cat))
  {
    ret = tagprint(ofp, cat);
  }
  else
  {
    print_vllength(ofp, req);
    efprintf(ofp, "<MENU>\r\n");
    for (v = req->res; v; v = v->next)
    {
#if 0
      if ((epath = getTagValue(REPLACE_WITH_URL, v->lattrib)))
#else
      if ((epath = nextAttrStr(REPLACE_WITH_URL, v->lattrib)))
#endif
      {
        ret = (multi_headlines(ofp, v, epath, req->here) ||
               single_headline(ofp, v, epath, req->here));
      }
      else if ((epath = vlink_to_url(v)))
      {
        ret = (multi_headlines(ofp, v, epath, req->here) ||
               single_headline(ofp, v, epath, req->here));
        free((char *)epath);
      }
      else
      {
        ret = 0;
        efprintf(stderr, "%s: wais_print: vlink_to_url() failed on `%s:%s'.\n",
                 v->host, v->hsoname, logpfx());
      }
    }
    efprintf(ofp, "</MENU></BODY>\r\n");
  }

  return ret;
}
