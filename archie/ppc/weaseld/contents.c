#include <ctype.h>
#include <string.h>
#include "contents.h"
#include "ppc.h"
#include "protos.h"
#include "all.h"


struct taglist
{
  const char *aname;
  const char *format;
  int (*printfn) proto_((FILE *ofp, const char *val));
};

struct template
{
  const char *name;
  const struct taglist *ptr;
  int maxatlen;
  int maxlinelen;
};


static int block proto_((FILE *ofp, const char *val));
static int centre proto_((FILE *ofp, const char *val));
static int glossPrint proto_((FILE *ofp, PATTRIB cat, struct template *tp));
static const struct taglist *getTagEntry proto_((const char *tag, const struct taglist *list));
#if 0
static int do_link proto_((FILE *ofp, const char *val, const char *prec[]));
static struct template *getTemplate proto_((const char *tname));
#endif


/*  
 *  The following table contains formatting information for the different
 *  attributes we expect to see in WAIS results.  It consists of the
 *  attribute name and strings to insert before and after the attribute
 *  value.  In the (hopefully) rare case where this is insufficiant the
 *  address of a function can be provided, which can perform more complex
 *  formatting (perhaps based on the contents of the value).
 */  
/*  
 *  bug: this should be changed to reflect the IAFA templates.
 */  
#if 0
static const struct taglist tagstrs[] =
{
  { "title",            "<p><pre><h2>%s</h2>"                       , 0       },
  { "subtitle",         "<h2>%s</h2>"                               , 0       },
  { "publisher",        "<h5>Published by %s</h5>"                  , 0       },
  { "editor",           "<h5>%s</h5>"                               , 0       },
  { "mainblurb",        "</pre>\r\n<blockquote>%s</blockquote>\r\n" , 0       },
  { "frequency",        "%s\r\n"                                    , 0       },
  { "pubmonths",        "%s\r\n"                                    , 0       },
  { "subprice",         "<br>%s\r\n"                                , 0       },

  { 0,                  "<br>\r\n"                                  , 0       }
};
#endif

#if 0
static const struct taglist tags_routledge[] =
{
  { "Title",     "<p><pre><h2>%s</h2>", 0 },
  { "Sub-Title",  "<h2>%s</h2>", 0 },
  { "Short-Title",     "<p><pre><h2>%s</h2>", 0 },
  { "Publisher", "<h5>Published by %s</h5>", 0 },
  { "Editor",    "<h5>%s</h5>", 0 },
  { "Short-Blurb", "</pre>\r\n<blockquote>%s</blockquote>\r\n", 0 },
  { "Long-Blurb", "</pre>\r\n<blockquote>%s</blockquote>\r\n", 0 },
  { "Previous-Title",  "<h5>%s</h5>", 0 },
  { "Series-Title",  "<h5>%s</h5>", 0 },
  { "Frequency", "%s\r\n", 0 },
  { "Pubmonths", "%s\r\n", 0 },
  { "Subprice",  "%s<br>", 0 }
};
#endif

#if 0
static const struct taglist tags_user[] =
{
  { "Name",
    "Name = %s<br>", 0 },
  { "Email",
    "Email = %s<br>", 0 },
  { "Host-Name",
    "Host-Name = %s<br>", 0 },
  { "Host-Port",
    "Host-Port = %s<br>", 0 },
  { "Protocol",
    "Protocol = %s<br>", 0 },
  { "Admin-Department",
    "Admin-Department = %s<br>", 0 },
  { "Admin-Email",
    "Admin-Email = %s<br>", 0 },
  { "Admin-Handle",
    "Admin-Handle = %s<br>", 0 },
  { "Admin-Home-Fax",
    "Admin-Home-Fax = %s<br>", 0 },
  { "Admin-Home-Phone",
    "Admin-Home-Phone = %s<br>", 0 },
  { "Admin-Home-Postal",
    "Admin-Home-Postal = %s<br>", 0 },
  { "Admin-Job-Title",
    "Admin-Job-Title = %s<br>", 0 },
  { "Admin-Name",
    "Admin-Name = %s<br>", 0 },
  { "Admin-Organization-Name",
    "Admin-Organization-Name = %s<br>", 0 },
  { "Admin-Organization-Type",
    "Admin-Organization-Type = %s<br>", 0 },
  { "Admin-Work-Fax",
    "Admin-Work-Fax = %s<br>", 0 },
  { "Admin-Work-Phone",
    "Admin-Work-Phone = %s<br>", 0 },
  { "Admin-Work-Postal",
    "Admin-Work-Postal = %s<br>", 0 },
  { "Admin-Organization-Postal",
    "Admin-Organization-Postal = %s<br>", 0 },
  { "Admin-Organization-City",
    "Admin-Organization-City = %s<br>", 0 },
  { "Admin-Organization-State",
    "Admin-Organization-State = %s<br>", 0 },
  { "Admin-Organization-Country",
    "Admin-Organization-Country = %s<br>", 0 },
  { "Admin-Organization-Email",
    "Admin-Organization-Email = %s<br>", 0 },
  { "Admin-Organization-Phone",
    "Admin-Organization-Phone = %s<br>", 0 },
  { "Admin-Organization-Fax",
    "Admin-Organization-Fax = %s<br>", 0 },
  { "Admin-Organization-Handle",
    "Admin-Organization-Handle = %s<br>", 0 },
  { "Sponsoring-Organization-City",
    "Sponsoring-Organization-City = %s<br>", 0 },
  { "Sponsoring-Organization-Country",
    "Sponsoring-Organization-Country = %s<br>", 0 },
  { "Sponsoring-Organization-Email",
    "Sponsoring-Organization-Email = %s<br>", 0 },
  { "Sponsoring-Organization-Fax",
    "Sponsoring-Organization-Fax = %s<br>", 0 },
  { "Sponsoring-Organization-Handle",
    "Sponsoring-Organization-Handle = %s<br>", 0 },
  { "Sponsoring-Organization-Name",
    "Sponsoring-Organization-Name = %s<br>", 0 },
  { "Sponsoring-Organization-Phone",
    "Sponsoring-Organization-Phone = %s<br>", 0 },
  { "Sponsoring-Organization-Postal",
    "Sponsoring-Organization-Postal = %s<br>", 0 },
  { "Sponsoring-Organization-State",
    "Sponsoring-Organization-State = %s<br>", 0 },
  { "Sponsoring-Organization-Type",
    "Sponsoring-Organization-Type = %s<br>", 0 },
  { "Description",
    "Description = %s<br>", 0 },
  { "Authentication",
    "Authentication = %s<br>", 0 },
  { "Registration",
    "Registration = %s<br>", 0 },
  { "Charging-Policy",
    "Charging-Policy = %s<br>", 0 },
  { "Access-Policy",
    "Access-Policy = %s<br>", 0 },
  { "Access-Times",
    "Access-Times = %s<br>", 0 },
  { "Keywords",
    "Keywords = %s<br>", 0 },
  { "Record-Last-Modified-Date",
    "Record-Last-Modified-Date = %s<br>", 0 },
  { "Record-Last-Modified-Department",
    "Record-Last-Modified-Department = %s<br>", 0 },
  { "Record-Last-Modified-Email",
    "Record-Last-Modified-Email = %s<br>", 0 },
  { "Record-Last-Modified-Handle",
    "Record-Last-Modified-Handle = %s<br>", 0 },
  { "Record-Last-Modified-Home-Fax",
    "Record-Last-Modified-Home-Fax = %s<br>", 0 },
  { "Record-Last-Modified-Home-Phone",
    "Record-Last-Modified-Home-Phone = %s<br>", 0 },
  { "Record-Last-Modified-Home-Postal",
    "Record-Last-Modified-Home-Postal = %s<br>", 0 },
  { "Record-Last-Modified-Job-Title",
    "Record-Last-Modified-Job-Title = %s<br>", 0 },
  { "Record-Last-Modified-Name",
    "Record-Last-Modified-Name = %s<br>", 0 },
  { "Record-Last-Modified-Organization-Name",
    "Record-Last-Modified-Organization-Name = %s<br>", 0 },
  { "Record-Last-Modified-Organization-Type",
    "Record-Last-Modified-Organization-Type = %s<br>", 0 },
  { "Record-Last-Modified-Work-Fax",
    "Record-Last-Modified-Work-Fax = %s<br>", 0 },
  { "Record-Last-Modified-Work-Phone",
    "Record-Last-Modified-Work-Phone = %s<br>", 0 },
  { "Record-Last-Modified-Work-Postal",
    "Record-Last-Modified-Work-Postal = %s<br>", 0 },
  { "Record-Last-Verified-Department",
    "Record-Last-Verified-Department = %s<br>", 0 },
  { "Record-Last-Verified-Email",
    "Record-Last-Verified-Email = %s<br>", 0 },
  { "Record-Last-Verified-Handle",
    "Record-Last-Verified-Handle = %s<br>", 0 },
  { "Record-Last-Verified-Home-Fax",
    "Record-Last-Verified-Home-Fax = %s<br>", 0 },
  { "Record-Last-Verified-Home-Phone",
    "Record-Last-Verified-Home-Phone = %s<br>", 0 },
  { "Record-Last-Verified-Home-Postal",
    "Record-Last-Verified-Home-Postal = %s<br>", 0 },
  { "Record-Last-Verified-Job-Title",
    "Record-Last-Verified-Job-Title = %s<br>", 0 },
  { "Record-Last-Verified-Name",
    "Record-Last-Verified-Name = %s<br>", 0 },
  { "Record-Last-Verified-Organization-Name",
    "Record-Last-Verified-Organization-Name = %s<br>", 0 },
  { "Record-Last-Verified-Organization-Type",
    "Record-Last-Verified-Organization-Type = %s<br>", 0 },
  { "Record-Last-Verified-Work-Fax",
    "Record-Last-Verified-Work-Fax = %s<br>", 0 },
  { "Record-Last-Verified-Work-Phone",
    "Record-Last-Verified-Work-Phone = %s<br>", 0 },
  { "Record-Last-Verified-Work-Postal",
    "Record-Last-Verified-Work-Postal = %s<br>", 0 },
  { "Record-Last-Verified-Date",
    "Record-Last-Verified-Date = %s<br>", 0 },

  { 0, 0, 0 }
};
#endif

static const struct taglist tags_publisher[] =
{
  { "title",              "<p><pre><h2>%s</h2>"                       , 0 },
  { "sub-title",          "<h2>%s</h2>"                               , 0 },
  { "short-title",        "<p><pre><h2>%s</h2>"                       , 0 },
  { "publisher",          "<h5>Published by %s</h5>"                  , 0 },
  { "editor",             "<h5>%s</h5>"                               , 0 },
  { "short-blurb",        "</pre>\r\n<blockquote>%s</blockquote>\r\n" , 0 },
  { "long-blurb",         "</pre>\r\n<blockquote>%s</blockquote>\r\n" , 0 },
  { "previous-title",     "<h5>%s</h5>"                               , 0 },
  { "series-title",       "<h5>%s</h5>"                               , 0 },
  { "frequency",          "%s\r\n"                                    , 0 },
  { "pubmonths",          "%s\r\n"                                    , 0 },
  { "subprice",           "<br>%s\r\n"                                , 0 },

  { 0,                    "%s<br>\r\n"                                , 0 }
};

/* for APS demo -wheelan (Sun Mar 27 02:26:08 EST 1994) */
static const struct taglist tags_aps[] =
{
  { "Name",                 "Name: %s\r\n", 0 },
  { "Born",                 "Born: %s\r\n", 0 },
  { "Died",                 "Died: %s\r\n", 0 },
  { "Nationality",          "Nationality: %s\r\n", 0 },
  { "Keywords",             "Keywords: %s\r\n", 0 },
  { "Publications",         "Publications: %s\r\n", 0 },
  { "Research-Description", "Research-Description: %s\r\n", 0 },

  /* aps_pi */

  { "Abstract",                  "\r\n%s\r\n"                   , block },
  { "Author-Affiliation-Name",   "%s\r\n"                     , centre },
  { "Author-Affiliation-Postal", "%s\r\n"                     , centre },
  { "Author-Department",         0                          , 0 },
  { "Author-Email",              "%s\r\n"                     , centre },
  { "Author-Name",               "\r\n%s\r\n"                   , centre },
  { "Author-Organization-Name",  "%s\r\n"                     , centre },
  { "Author-Organization-Type",  0                          , 0 },
  { "Author-Work-Postal",        "%s\r\n"                     , centre },
  { "Journal-Article-Extent",    0                          , 0 },
  { "Journal-Article-Pacs",      "  PACS numbers: %s\r\n"     , 0 },
  { "Journal-Article-Pagenum",   0                          , 0 },
  { "Journal-Article-Received",  "  Received: %s\r\n"         , 0 },
  { "Journal-Article-Title",     "%s\r\n\r\n"                   , centre },
  { "Journal-Issue-Number",      ", %s               "      , 0 },
  { "Journal-Issue-Published",   "               %s\r\n\r\n\r\n\r\n", 0 },
  { "Journal-Title",             "%s"                       , 0 },
  { "Journal-Volume-Number",     "  Volume %s"              , 0 },
  
  /* This must be last */
  { 0,                           0                          , 0 }
};

static const struct taglist tags_dod[] =
{
  /* For dod-ain.aux */
  { "fiig-number",               "%s"                , 0 },
  { "reprint-date",              "%s"                , 0 },
  { "approved-item-name",        "%s"                , 0 },
  { "item-name-code",            "%s"                , 0 },
  { "applicability-key",         "%s"                , 0 },
  { "ain-desc",                  "%s"                , 0 },

  /* For dod-appkey (those fields not previously listed */
  { "app-key",                   "%s"                , 0 },
  { "mast-req-code",             "%s"                , 0 },
  { "FIIG-Page",                 "%s"                , 0 },
  { "requirement-status",        "%s"                , 0 },

  /* For dod-sect1 (those fields not previously listed */
  { "mode-code",                 "%s"                , 0 },
  { "requirements-text",         "%s"                , 0 },
  { "mrc-def",                   "%s"                , 0 },
  { "mrc-reply-inst",            "%s"                , 0 },

  { 0,                           0                   , 0 }
}; 

static const struct taglist tags_generic[] =
{
  { "Bunyip-Sequence",  0                 , 0       },
  { "Image",            0                 , 0       },
  { "Link-To",          0                 , 0 /*do_link*/ },
  { "Replace-With-Url", 0                 , 0       },
  { "Template-Type",    0                 , 0       },

  { 0, 0, 0 }
};


struct template templates[] =
{
  { "publisher", tags_publisher, 14, 80 },
  { "aps", tags_aps, 25, 80 },
#if 0
  { "user", tags_user },
#endif
  { "dod", tags_dod, 18, 50 },
  { 0, tags_generic, 0, 0 }     /* nothing to print (yet) */
};


/*  
 *
 *                            Internal routines
 *
 */  


static int block(ofp, val)
  FILE *ofp;
  const char *val;
{
  fmtprintf(ofp, "      ", val, 80 /*bug*/);
  return 1;
}


static int centre(ofp, val)
  FILE *ofp;
  const char *val;
{
  const int linelen = 80;
  int slen;

  slen = strlen(val);
  if (slen < linelen)
  {
    efprintf(ofp, "%*s%s", (linelen - slen) / 2, " ", val);
  }
  else
  {
    /*bug: ought to calculate an indentation */
    fmtprintf(ofp, "          ", val, 80 /*bug*/);
  }
  return 1;
}


static const struct taglist *getTagEntry(tag, list)
  const char *tag;
  const struct taglist *list;
{
  while (list->aname)
  {
    if (strcasecmp(tag, list->aname) == 0)
    {
      break;
    }
    list++;
  }
  return list;
}


/*  
 *  Normally, this will be called with `tp' set to `tags_generic'.
 */  
static int glossPrint(ofp, cat, tp)
  FILE *ofp;
  PATTRIB cat;
  struct template *tp;
{
  PATTRIB at;
  int anlen = -1;

  /*  
   *  Find the length of the longest attribute name.
   */
  for (at = cat; (at = nextAttr(CONTENTS, at)); at = at->next)
  {
    const char *s;
    int len;

    s = nth_token_str(at->value.sequence, 1);
    len = strlen(s);
    anlen = max(anlen, len);
  }
  
  for (at = cat; (at = nextAttr(CONTENTS, at)); at = at->next)
  {
    const char *aname;
    const char *avalue;
    const struct taglist *l;
    
    aname = nth_token_str(at->value.sequence, 1);
    if (strcasecmp(aname, "Image") == 0)
    {
      continue;                 /* we already handled it above */
    }

    /*  
     *  Assume there will not be a user-supplied format string for attributes
     *  in the "generic" table.
     */  
    avalue = nth_token_str(at->value.sequence, 2);
    l = getTagEntry(aname, tags_generic);
    if (l && l->aname)
    {
      if (l->printfn)
      {
        l->printfn(ofp, avalue);
      }
      else if (l->format)
      {
        efprintf(ofp, l->format, avalue);
      }
    }
    else
    {
      char attr[128];           /* bug: fixed size */

      sprintf(attr, "%s:%*s",
              aname, (int)(anlen - strlen(aname) + 1), " ");
      fmtprintf(ofp, attr, avalue, 80 /*bug?*/);
    }
  }

  return 1;
}


#if 0
static struct template *getTemplate(tname)
  const char *tname;
{
  int i;
    
  for (i = 0; templates[i].name; i++)
  {
    if (strcasecmp(templates[i].name, tname) == 0)
    {
      break;
    }
  }

  return &templates[i];
}
#endif


/*  
 *
 *                            External routines
 *
 */  


int gopherLinkTo(atval, type, epath, desc)
  const char *atval;
  char **type;
  char **epath;
  char **desc;
{
  static const char *prec[] = { "IMAGE", "TEXT", "AUDIO", 0 };

  return link_to(atval, prec, type, epath, desc);
}


static StrVal typetab[] =
{
  { "AUDIO",    '9' },
  { "IMAGE",    'I' },
  { "TEXT",     '0' },

  { "audio",    '9' },
  { "image",    'I' },
  { "text",     '0' },

  { STRVAL_END, '9' }
};


int gopherTypeOf(type)
  const char *type;
{
  return sval(type, typetab);
}


int tagprint(ofp, cat)
  FILE *ofp;
  PATTRIB cat;
{

#ifndef NO_TCL
  if ( ! tcl_tagged_print(ofp, cat))
  {
    return glossPrint(ofp, cat, 0);
  }
  return 1;

#else 

  return glossPrint(ofp, cat, 0);
#endif
}
