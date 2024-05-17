#include <string.h>
#include "archie_strings.h"
#include "file_type.h"
#include "ppc.h"
#include "protos.h"
#include "all.h"


enum Field
{
  EXTENSION        = 0,         /* array indices: must start at 0 */
  CONTENT_TYPE,
  CONTENT_SUBTYPE,
  ENCODING
};


static int client_groks proto_((const char *ctype, const char *stype, const char **enc));
static int nextMatch proto_((const char *type, int *idx, enum Field field));


/*  
 *  This table provides a mapping between file extensions and possible fields
 *  of the MIME `Content-type' header.  It is used to determine whether we
 *  can send certain file types to the WWW client.
 *  
 *  The `client_accepts' item of each entry can take on one of three possible
 *  values, -1, 0 and 1, with the following meanings:
 *  
 *   -1    client accepts this type; never change this value
 *    0    client doesn't accept this type
 *    1    client accepts this type; reset this value to 0
 *  
 *  The table initially says that any connecting client accepts a content
 *  type of "text/html" (which is required by the HTTP spec).  Other entries
 *  will be set if the appropriate `Accept: ' header is seen.
 *  
 *  After the request has been serviced, all entries, except those set to -1,
 *  will be reset to 0.
 */        

static struct
{
  const char *flds[4];
  int client_accepts;
} extmap[] =
{
  {{ "au",    "audio",       "basic",                "binary" },  0 }, /* ??? */
  {{ "dvi",   "application", "x-dvi",                "binary" },  0 },
  {{ "gif",   "image",       "gif",                  "binary" },  0 },
  {{ "hdf",   "application", "hdf",                  "binary" },  0 }, /* ??? */
  {{ "hdf",   "application", "x-hdf",                "binary" },  0 }, /* ??? */
  {{ "htm",   "text",        "html",                 "8bit"   },  0 },
  {{ "html",  "text",        "html",                 "8bit"   }, -1 },
  {{ "html",  "text",        "x-html",               "8bit"   },  0 },
  {{ "html",  "application", "html",                 "8bit"   },  0 },
  {{ "html",  "application", "x-html",               "8bit"   },  0 },
  {{ "info",  "application", "x-texinfo",            "8bit"   },  0 }, /* ??? */
  {{ "jpeg",  "image",       "jpeg",                 "binary" },  0 },
  {{ "jpg",   "image",       "jpeg",                 "binary" },  0 },
  {{ "latex", "application", "x-latex",              "7bit"   },  0 },
  {{ "ma",    "application", "x-mathematica",        "8bit"   },  0 }, /* Thomson */
  {{ "man",   "application", "x-troff-man",          "7bit"   },  0 },
  {{ "mcd",   "application", "x-mathcad",            "8bit"   },  0 }, /* Thomson */
  {{ "me",    "application", "x-troff-me",           "7bit"   },  0 },
  {{ "mov",   "video",       "quicktime",            "binary" },  0 },
  {{ "mpeg",  "video",       "mpeg",                 "binary" },  0 },
  {{ "mpg",   "video",       "mpeg",                 "binary" },  0 },
#if 0
  /*  
   *  bug: got rid of this, for now, so Thomson can server Maple files (with
   *  .ms extensions) without having them sent across with troff MIME types.
   *  
   *  In the future we probably ought to give ferretd the ability to read in
   *  a site defined file extension table, and also provide a link attribute
   *  to override any default value.
   */
  {{ "ms",    "application", "x-troff-ms",           "7bit"   },  0 },
#else
  {{ "ms",    "application", "x-maple",              "8bit"   },  0 }, /* Thomson */
#endif
  {{ "pbm",   "image",       "x-portable-bitmap",    "binary" },  0 },
  {{ "pgm",   "image",       "x-portable-graymap",   "binary" },  0 },
  {{ "pnm",   "image",       "x-portable-anymap",    "binary" },  0 },
  {{ "ppm",   "image",       "x-portable-pixmap",    "binary" },  0 },
  {{ "ps",    "application", "postscript",           "7bit"   },  0 },
  {{ "qt",    "video",       "quicktime",            "binary" },  0 },
  {{ "qtw",   "video",       "quicktime",            "binary" },  0 },
  {{ "rgb",   "image",       "rgb",                  "binary" },  0 }, /* ??? */
  {{ "rgb",   "image",       "x-rgb",                "binary" },  0 }, /* ??? */
  {{ "roff",  "application", "x-troff",              "7bit"   },  0 },
  {{ "rtf",   "text",        "richtext",             "8bit"   },  0 },
  {{ "sea",   "application", "octet-stream",         "binary" },  0 },
  {{ "snd",   "audio",       "basic",                "binary" },  0 }, /* ??? */
  {{ "tex",   "application", "x-tex",                "7bit"   },  0 },
  {{ "text",  "text",        "plain",                "8bit"   },  0 },
  {{ "tiff",  "image",       "tiff",                 "binary" },  0 },
  {{ "troff", "application", "x-troff",              "7bit"   },  0 },
  {{ "txt",   "text",        "plain",                "8bit"   },  0 },
  {{ "wav",   "audio",       "x-wav",                "binary" },  0 },
  {{ "xbm",   "image",       "x-xbitmap",            "7bit"   },  0 },
  {{ "xpm",   "image",       "x-xpixmap",            "7bit"   },  0 },
  {{ "xwd",   "image",       "xwd",                  "binary" },  0 },
  {{ "xwd",   "image",       "x-xwd",                "binary" },  0 },
  {{ "xwd",   "image",       "x-xwindowdump",        "binary" },  0 },

  {{ "",      "application", "*",                    "binary" },  0 },
  {{ "",      "audio",       "*",                    "binary" },  0 },
  {{ "",      "image",       "*",                    "binary" },  0 },
  {{ "",      "message",     "*",                    "8bit"   },  0 },
  {{ "",      "text",        "*",                    "8bit"   },  0 },
  {{ "",      "video",       "*",                    "binary" },  0 },
  {{ "",      "*",           "*",                    "binary" },  0 },

#if 0
  /* Not quite sure what ought to be done about the following... */

  {{ "",      "application", "netcdf"                "binary" },  0 },
  {{ "",      "application", "x-netcdf",             "binary" },  0 },
  {{ "",      "audio",       "x-aiff",               "binary" },  0 },
  {{ "",      "message",     "rfc822",               "8bit"   },  0 },
  {{ "",      "text",        "tab-separated-values", "8bit"   },  0 },
  {{ "",      "text",        "x-setext",             "8bit"   },  0 },
#endif

  /* This must be last. */
  {{0,        0,              0,                      0       },  0 }
};


#define LAST_IDX ((sizeof extmap / sizeof extmap[0]) - 1)


/*  
 *  Return 1 if the client can accept the given type, otherwise return 0.  If
 *  the type is acceptable, set `*enc' to point to the content encoding of
 *  that type.
 */  
static int client_groks(ctype, stype, enc)
  const char *ctype;
  const char *stype;
  const char **enc;
{
  int i = 0;
  
  for (i = 0; nextMatch(ctype, &i, CONTENT_TYPE); i++)
  {
    if (strcmp(extmap[i].flds[CONTENT_SUBTYPE], stype) == 0 &&
        extmap[i].client_accepts)
    {
      /*  
       *  bug: if a type is found in the table, but was not in the list of
       *  type acceptible to the client (e.g. application/x-mathematica), but
       *  the client accepts * / *, then the encoding from the table will be
       *  overridden by the encoding of * / *.  (E.g. 8bit may be replaced by
       *  binary.)
       */
      *enc = extmap[i].flds[ENCODING];
      return 1;
    }
  }

  return 0;
}


/*  
 *  Starting with position `*idx' look through the table for entries whose
 *  flds[`field'] string matches `s'.
 *  
 *  Only modify `*idx' if a match is found.
 */    
static int nextMatch(s, idx, field)
  const char *s;
  int *idx;
  enum Field field;
{
  int i;
  
  extmap[LAST_IDX].flds[field] = s; /* set a sentinel */
  for (i = *idx; strcasecmp(s, extmap[i].flds[field]) != 0; i++)
  { continue; }
  extmap[LAST_IDX].flds[field] = 0;
  if (i == LAST_IDX)
  {
    return 0;
  }
  else
  {
    *idx = i;
    return 1;
  }
}


/*  
 *  bug: Also, one file type may be under several content types (e.g. xwd),
 *  so we should map file extensions to a list of content types, then check
 *  to see whether the client can handle any in the list.
 */    

/*  
 *  Determine whether the client can accept a document with the given content
 *  type and subtype.
 *  
 *  Reset the types if the client cannot accept the exact type, but will
 *  accept application/octet-stream.
 */          

int match_content_type(ctype, stype, enc)
  const char **ctype;
  const char **stype;
  const char **enc;
{
  if (client_groks(*ctype, *stype, enc) ||
      client_groks(*ctype, "*", enc) ||
      client_groks("*", "*", enc))
  {
    return 1;
  }
  else if (client_groks("application", "octet-stream", enc))
  {
    *ctype = "application";
    *stype = "octet-stream";
    return 1;
  }

  return 0;
}


void clear_acceptance()
{
  int i;
  
  for (i = 0; extmap[i].flds[EXTENSION]; i++)
  {
    if (extmap[i].client_accepts > 0)
    {
      extmap[i].client_accepts = 0;
    }
  }
}


/*
 *  `type' is of the form type/subtype.  E.g. image/gif.
 *
 *  If `type' is found in our list, flag it as accepted.
 */
void client_accepts(type)
  const char *type;
{
  char **t;
  int i, n;
  
  if ( ! strsplit(type, "/", &n, &t) || n != 2)
  {
    efprintf(stderr, "%s: client_accepts: error splitting `%s' on `/'.\n",
             logpfx(), type);
    return;
  }

  for (i = 0; nextMatch(t[0], &i, CONTENT_TYPE); i++)
  {
    if (strcmp(t[1], extmap[i].flds[CONTENT_SUBTYPE]) == 0 &&
        extmap[i].client_accepts == 0)
    {
      extmap[i].client_accepts = 1;
    }
  }

  free(t);
}


/*  
 *  Return the content type and encoding of the "file" specified by `v'.
 *  
 *  If we can't determine the type, use "application/octet-stream".
 */  
void content_type_of(v, ctype, stype, enc)
  VLINK v;
  const char **ctype;
  const char **stype;
  const char **enc;
{
  char *ext;
  
  /* bug: assume WAIS repsonse is in HTML. */
  if (STRNCMP(v->hsoname, "ARCHIE/ITEM/WAIS") == 0)
  {
    *ctype = "text";
    *stype = "html";
    *enc = "8bit";
  }
  else if ( ! ((ext = strrchr(v->name, '.')) ||
               (ext = strrchr(v->hsoname, '.'))))
  {
    /*bug: we assume the lack of an extension implies "text/html" */
    *ctype = "text";
    *stype = "html";
    *enc = "8bit";
  }
  else
  {
    int i = 0;

    if (nextMatch(ext+1, &i, EXTENSION))
    {
      *ctype = extmap[i].flds[CONTENT_TYPE];
      *stype = extmap[i].flds[CONTENT_SUBTYPE];
      *enc = extmap[i].flds[ENCODING];
    }
    else
    {
      *ctype = "application";
      *stype = "octet-stream";
      *enc = "binary";
    }
  }
}
