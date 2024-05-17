#ifdef __STDC__
# include <stdlib.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "aprint.h"
#include "contents.h"
#include "ppc.h"
#include "protos.h"
#include "url.h"


static char *wais_hdln proto_((VLINK item));
static void printImageHeadlines proto_((FILE *ofp, PATTRIB cat, const Where here));
static void printLinkToHeadlines proto_((FILE *ofp, PATTRIB cat, const Where here));


static char *wais_hdln(item)
  VLINK item;
{
  return nuke_afix_ws(item->name);
}


/*  
 *  Print out multiple headlines, if they appear on the link.
 */  
static int multi_headlines(ofp, item, epath, here)
  FILE *ofp;
  VLINK item;
  const char *epath;
  const Where here;
{
  PATTRIB at;
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

  /*  
   *  We want all the contents, not just for headlines (since we also want to
   *  look for Image and Link-To attributes).
   */
  if ( ! (cat = get_contents_from_server(item)))
  {
    efprintf(stderr, "%s: multi_headlines: can't get CONTENTS attribute for `%s:%s'.\n",
             logpfx(), item->host, item->hsoname);
    return 0;
  }

  /*  
   *  The value of the BUNYIP-HEADLINE attribute is a list of WAIS attribute
   *  names whose values should appear in the headline.
   */
  for (hname = hat->value.sequence; hname; hname = hname->next)
  {
    for (at = cat; (at = nextAttrWithTag(hname->token, at)); at = at->next)
    {
      const char *atname;
      const char *atval;
            
      atname = nth_token_str(at->value.sequence, 1);
      if ( ! (atval = nth_token_str(at->value.sequence, 2)))
      {
        efprintf(stderr, "%s: multi_headlines: no WAIS value name on `%s:%s'.\n",
                 logpfx(), item->host, item->hsoname);
        continue;
      }

      if (first_attr)
      {
        first_attr = 0;
        efprintf(ofp, "0%s\t%s\t%s\t%s\r\n",
                 atval, epath, here.host, here.port);
      }
      else
      {
        efprintf(ofp, "0____%s\t%s\t%s\t%s\r\n",
                 atval, epath, here.host, here.port);
      }
    }
  }

  printImageHeadlines(ofp, cat, here);
  printLinkToHeadlines(ofp, cat, here);

  return 1;
}


static int single_headline(ofp, item, epath, here)
  FILE *ofp;
  VLINK item;
  const char *epath;
  const Where here;
{
  PATTRIB cat;

  efprintf(ofp, "0%s\t%s\t%s\t%s\r\n",
           wais_hdln(item), epath, here.host, here.port);

  /*  
   *  We want all the contents, not just for headlines since we want to look
   *  for Image and Link-To attributes.
   */
  if ( ! (cat = get_contents_from_server(item)))
  {
    efprintf(stderr, "%s: single_headline: can't get CONTENTS attribute for `%s:%s'.\n",
             logpfx(), item->host, item->hsoname);
    return 0;
  }

  printImageHeadlines(ofp, cat, here);
  printLinkToHeadlines(ofp, cat, here);

  return 1;
}


static void printImageHeadlines(ofp, cat, here)
  FILE *ofp;
  PATTRIB cat;
  const Where here;
{
  PATTRIB at;
  
  for (at = cat; (at = nextAttrWithTag("Image", at)); at = at->next)
  {
    const char *atname;
    const char *atval;

    atname = nth_token_str(at->value.sequence, 1);
    if ((atval = nth_token_str(at->value.sequence, 2)))
    {
      efprintf(ofp, "I____Image\t%s\t%s\t%s\r\n", atval, here.host, here.port);
    }
  }
}


static void printLinkToHeadlines(ofp, cat, here)
  FILE *ofp;
  PATTRIB cat;
  const Where here;
{
  PATTRIB at;
  
  for (at = cat; (at = nextAttrWithTag("Link-To", at)); at = at->next)
  {
    char *desc, *epath, *type;
    const char *atname;
    const char *atval;

    atname = nth_token_str(at->value.sequence, 1);
    if ((atval = nth_token_str(at->value.sequence, 2)))
    {                
      if (gopherLinkTo(atval, &type, &epath, &desc))
      {
        efprintf(ofp, "%c____%s\t%s\t%s\t%s\r\n",
                 gopherTypeOf(type), desc, epath, here.host, here.port);
        free(desc); free(epath); free(type);
      }
    }
  }
}


/*  
 *  Print the WAIS headline for this search result.
 */  
void print_wais(ofp, v, print_as_dir, here)
  FILE *ofp;
  VLINK v;
  int print_as_dir;
  const Where here;
{
  if ( ! print_as_dir)
  {
    efprintf(ofp, "3WAIS file -- can't do this yet.\r\n");
    return;
  }

  for (; v; v = v->next)
  {
    char *epath;

    if ( ! (epath = vlink_to_url(v)))
    {
      efprintf(stderr, "%s: print_wais: vlink_to_url() failed on `%s:%s'.\n",
               logpfx(), v->host, v->hsoname);
      continue;
    }

    if ( ! multi_headlines(ofp, v, epath, here))
    {
      single_headline(ofp, v, epath, here);
    }
  }
}
