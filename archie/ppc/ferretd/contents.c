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
  int (*printfn) proto_((FILE *ofp, const char *val, const char *prec[]));
};

struct template
{
  const char *name;
  const struct taglist *ptr;
  int maxatlen;
  int maxlinelen;
};


static int formattedPrint proto_((FILE *ofp, PATTRIB cat, struct template *tp));
static int glossPrint proto_((FILE *ofp, PATTRIB cat, struct template *tp));
static const struct taglist *getTagEntry proto_((const char *tag, const struct taglist *list));
static int do_link proto_((FILE *ofp, const char *val, const char *prec[]));
static struct template *getTemplate proto_((const char *tname));


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
  { "title",            "<P><PRE><H2>%s</H2>"                       , 0       },
  { "subtitle",         "<H2>%s</H2>"                               , 0       },
  { "publisher",        "<H5>Published by %s</H5>"                  , 0       },
  { "editor",           "<h5>%s</h5>"                               , 0       },
  { "mainblurb",        "</PRE>\r\n<BLOCKQUOTE>%s</BLOCKQUOTE>\r\n" , 0       },
  { "frequency",        "%s\r\n"                                    , 0       },
  { "pubmonths",        "%s\r\n"                                    , 0       },
  { "subprice",         "<BR>%s\r\n"                                , 0       },

  { 0,                  "<BR>\r\n"                                  , 0       }
};
#endif

#if 0
static const struct taglist tags_routledge[] =
{
  { "Title",     "<P><PRE><H2>%s</H2>", 0 },
  { "Sub-Title",  "<H2>%s</H2>", 0 },
  { "Short-Title",     "<P><PRE><H2>%s</H2>", 0 },
  { "Publisher", "<H5>Published by %s</H5>", 0 },
  { "Editor",    "<H5>%s</H5>", 0 },
  { "Short-Blurb", "</PRE>\r\n<BLOCKQUOTE>%s</BLOCKQUOTE>\r\n", 0 },
  { "Long-Blurb", "</PRE>\r\n<BLOCKQUOTE>%s</BLOCKQUOTE>\r\n", 0 },
  { "Previous-Title",  "<H5>%s</H5>", 0 },
  { "Series-Title",  "<H5>%s</H5>", 0 },
  { "Frequency", "%s\r\n", 0 },
  { "Pubmonths", "%s\r\n", 0 },
  { "Subprice",  "%s<BR>", 0 }
};
#endif

#if 0
static const struct taglist tags_user[] =
{
  { 0, 0, 0 }
};
#endif

static const struct taglist tags_isbn[] =
{
  { "title",              "<P><PRE><h2>%s</h2>"                   , 0 },
  { "sub-title",          "<h2>%s</h2>"                           , 0 },
  { "short-title",        "<P><PRE><h2>%s</h2>"                   , 0 },
  { "publisher",          "<H5>Published by %s</H5>"              , 0 },
  { "editor",             "<H5>%s</H5>"                           , 0 },
  { "short-blurb",        "</PRE>\r\n<BLOCKQUOTE>%s</BLOCKQUOTE>\r\n" , 0 },
  { "long-blurb",         "</PRE>\r\n<BLOCKQUOTE>%s</BLOCKQUOTE>\r\n" , 0 },
  { "previous-title",     "<H5>%s</H5>"                           , 0 },
  { "series-title",       "<H5>%s</H5>"                           , 0 },
  { "frequency",          "%s\r\n"                                  , 0 },
  { "pubmonths",          "%s\r\n"                                  , 0 },
  { "subprice",           "<BR>%s\r\n"                              , 0 },

  { 0,                    "%s<BR>\r\n"                              , 0 }
};

static const struct taglist tags_publisher[] =
{
  { 0, 0, 0 }
};

/* for APS demo -wheelan (Sun Mar 27 02:26:08 EST 1994) */
static const struct taglist tags_aps[] =
{
  { "name",                 "Name: %s<BR>\r\n"                 , 0       },
  { "born",                 "Born: %s<BR>\r\n"                 , 0       },
  { "died",                 "Died: %s<BR>\r\n"                 , 0       },
  { "nationality",          "Nationality: %s<BR>\r\n"          , 0       },
  { "keywords",             "Keywords: %s<BR>\r\n"             , 0       },
  { "publications",         "Publications: %s<BR>\r\n"         , 0       },
  { "research-description", "Research-Description: %s<BR>\r\n" , 0       },

  /* aps_pi */

  { "Abstract",                  "<BLOCKQUOTE>\r\n%s</BLOCKQUOTE>\r\n" , 0 },
  { "Author-Affiliation-Name",   "%s<BR>"                              , 0 },
  { "Author-Affiliation-Postal", "%s<BR>"                              , 0 },
  { "Author-Department",         "%s</H5>"                             , 0 },
  { "Author-Email",              "Email: %s<BR>"                       , 0 },
  { "Author-Name",               "<H4>%s</H4>\r\n"                     , 0 },
  { "Author-Organization-Name",  "<H5>%s<BR>"                          , 0 },
  { "Author-Organization-Type",  "%s"                                  , 0 },
  { "Author-Work-Postal",        "%s<BR>"                              , 0 },
  { "Journal-Article-Extent",    "%s"                                  , 0 },
  { "Journal-Article-Pacs",      "PACS numbers: %s<BR>\r\n"            , 0 },
  { "Journal-Article-Pagenum",   "%s"                                  , 0 },
  { "Journal-Article-Received",  "<BR>\r\nReceived: %s<BR>\r\n"        , 0 },
  { "Journal-Article-Title",     "<H2>%s</H2>\r\n"                     , 0 },
  { "Journal-Issue-Number",      ", %s      "                          , 0 },
  { "Journal-Issue-Published",   "      %s</PRE><hr>"                  , 0 },
  { "Journal-Title",             "%s"                                  , 0 },
  { "Journal-Volume-Number",     "<PRE>Volume %s"                      , 0 },

  { 0,                           "%s<BR>\r\n"                          , 0 }
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
  { "mode-code",                 "%s"                  , 0 },
  { "requirements-text",         "%s"                  , 0 },
  { "mrc-def",                   "%s"                  , 0 },
  { "mrc-reply-inst",            "%s"                  , 0 },

  { 0,                           "%s<BR>\r\n"          , 0 }
}; 

static const struct taglist tags_generic[] =
{
  { "Bunyip-Sequence",  0                 , 0       },
  { "Image",            0                 , 0       },
  { "Link-To",          0                 , do_link },
  { "Replace-With-Url", 0                 , 0       },
  { "Template-Type",    0                 , 0       },

  { 0, 0, 0 }
};


struct template templates[] =
{
  { "isbn", tags_isbn, 14, 80 },
  { "publisher", tags_publisher, 14, 80 },
  { "aps", tags_aps, 25, 80 },
#if 0
  { "user", tags_user },
#endif
  { "dod", tags_dod, 18, 50 },
  { 0, tags_generic, 0, 0 }     /* nothing to print (yet) */
};


/*  
 *  Format precedence for ferretd.
 */  
static const char *prec[] =
{ "EMBED-HTML", "VIDEO", "IMAGE", "AUDIO", "HTML", "TEXT", 0 };


/*  
 *
 *                            Internal routines
 *
 */  


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


static int do_link(ofp, val, prec)
  FILE *ofp;
  const char *val;
  const char *prec[];
{
  char *desc;
  char *epath;
  char *type;
  int ret = 0;
  
  if ( ! (link_to(val, prec, &type, &epath, &desc)))
  {
    efprintf(stderr, "%s: do_link: link_to() failed on `%s'.\n",
             prog, val);
  }
  else
  {
    if (strcasecmp(type, "embed-html") == 0) /*bug: kludge.  Have associated HTML with types?*/
    {
      ret = efprintf(ofp, "%s\r\n", desc);
    }
    else
    {
      ret = efprintf(ofp, "<A HREF=\"%s\">%s</A>\r\n", epath, desc);
    }
    free(desc); free(epath); free(type);
  }

  return ret;
}


static int formattedPrint(ofp, cat, tp)
  FILE *ofp;
  PATTRIB cat;
  struct template *tp;
{
  int ret = 1;
  
  for (; (cat = nextAttr(CONTENTS, cat)); cat = cat->next)
  {
    const char *aname;
    const char *avalue;
    const struct taglist *l;

    aname = nth_token_str(cat->value.sequence, 1);
    if (strcasecmp(aname, "Image") == 0)
    {
      continue;                 /* we already handled it above */
    }

    avalue = nth_token_str(cat->value.sequence, 2);

    if ( ! (l = getTagEntry(aname, tp->ptr)) || ! l->aname)
    {
      l = getTagEntry(aname, tags_generic);
    }

    if ( ! l->aname)
    {
      /*  
       *  Uh oh!  We've found an attribute name not on our list.  Switch
       *  to glossary format for the rest of the item.
       */
      return glossPrint(ofp, cat, 0);
    }
    else
    {
      if (l->printfn)
      {
        l->printfn(ofp, avalue, prec);
      }
      else if (l->format)
      {
        ret = efprintf(ofp, l->format, avalue);
      }
    }
  }

  return ret;
}


/*  
 *  Combine formattedPrint and glossPrint?  The latter effect is achieved by
 *  calling the routine with tags_generic?  (But we want the former to check
 *  that too...)
 */  

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
  
  efprintf(ofp, "<PRE>\r\n");

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
        l->printfn(ofp, avalue, prec);
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

  return efprintf(ofp, "</PRE>\r\n");
}


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


/*  
 *
 *                            External routines
 *
 */  


int tagprint(ofp, cat)
  FILE *ofp;
  PATTRIB cat;
{
  const char *image;
  const char *ttype;
  struct template *tp = 0;

  /*  
   *  Is the template type specified?  If so, do we recognize it?
   */  
  if ((ttype = getTagValue("Template-Type", cat)))
  {
    tp = getTemplate(ttype);
  }

  if ( ! tp || ! tp->ptr)
  {
    /* bug? replace "Image" with something more general? */
    if ((image = getTagValue("Image", cat)))
    {
      efprintf(ofp, "<IMG SRC=\"%s\">", image);
    }
    return glossPrint(ofp, cat, 0);
  }
  else
  {
#ifndef NO_TCL
    int ret;

    if ( ! (ret = tcl_tagged_print(ofp, cat)))
    {
      ret = formattedPrint(ofp, cat, tp);
    }
    return ret;

#else

    /* bug: replace "Image" with something more general? */
    if ((image = getTagValue("Image", cat)))
    {
      efprintf(ofp, "<IMG SRC=\"%s\">", image);
    }
    return formattedPrint(ofp, cat, tp);
#endif
  }
}
