#include <string.h>
#include "class.h"
#include "defs.h"
#include "pattrib.h"
#include "ppc_front_end.h"
#include "psearch.h"
#include "strval.h"
#include "token.h"
#include "all.h"


/*  
 *
 *
 *                            Internal routines.
 *
 *
 */  


/*  
 *  Map file extensions to link classes.
 */  
static StrVal suftab[] =
{
  { "Z",        B_CLASS_DATA    },
  { "a",        B_CLASS_DATA    },
  { "com",      B_CLASS_DATA    },
  { "exe",      B_CLASS_DATA    },
  { "gif",      B_CLASS_IMAGE   },
  { "gz",       B_CLASS_DATA    },
  { "hqx",      B_CLASS_DATA    },
  { "jpeg",     B_CLASS_IMAGE   },
  { "jpg",      B_CLASS_IMAGE   },
  { "mpeg",     B_CLASS_VIDEO   },
  { "mpg",      B_CLASS_VIDEO   },
  { "o",        B_CLASS_DATA    },
  { "pbm",      B_CLASS_IMAGE   },
  { "sea",      B_CLASS_DATA    }, /* Mac: self extracting archive */
  { "tar",      B_CLASS_DATA    },
  { "tif",      B_CLASS_IMAGE   },
  { "tiff",     B_CLASS_IMAGE   },
  { "xbm",      B_CLASS_IMAGE   },
  { "z",        B_CLASS_DATA    },

  { STRVAL_END, B_CLASS_UNKNOWN }
};


/*  
 *  Return a class based on the file extension.
 */  
static int ext_class(suffix)
  const char *suffix;
{
  int class;
  
  if ((class = sval(suffix, suftab)) == B_CLASS_UNKNOWN)
  {
    /*  
     *  bug: Unknown suffix: assume (dangerously) text.
     */  
    class = B_CLASS_DOCUMENT;
  }

  return class;
}


/*  
 *  Map OBJECT-INTERPRETATION attribute values to link classes.
 */  
static StrVal interptab[] =
{
  { "AGGREGATE",      B_CLASS_DATA     },
  { "DATA",           B_CLASS_DATA     },
  { "DIRECTORY",      B_CLASS_MENU     },
  { "DOCUMENT",       B_CLASS_DOCUMENT },
  { "EMBEDDED",       B_CLASS_DATA     },
  { "EXECUTABLE",     B_CLASS_DATA     },
  { "IMAGE",          B_CLASS_DATA     },
  { "PORTAL",         B_CLASS_PORTAL   },
  { "PROGRAM",        B_CLASS_DOCUMENT },
  { "SEARCH",         B_CLASS_SEARCH   },
  { "SOUND",          B_CLASS_DATA     },
  { "SOURCE-CODE",    B_CLASS_DOCUMENT },
  { "VIDEO",          B_CLASS_DATA     },
  { "VIRTUAL-SYSTEM", B_CLASS_MENU     },

  { STRVAL_END,       B_CLASS_UNKNOWN  }
};


/*  
 *  Return the class based upon the value of the OBJECT-INTERPRETATION
 *  attribute.
 *  
 *  If the type is found to be DOCUMENT, but is neither MIME nor ASCII text,
 *  return DATA (just to be compatible with the equivalent Prospero code).
 */
static int interp_class(type)
  TOKEN type;
{
  int class;

  if ((class = sval(type->token, interptab)) == B_CLASS_DOCUMENT)
  {
    /*  
     *  Special case: we should look at subtype information.
     */
    if ( ! tkmatches(type, "DOCUMENT", "SGML", "HTML", (const char *)0) &&
         ! tkmatches(type, "DOCUMENT", "TEXT", "ASCII", (const char *)0) &&
         ! tkmatches(type, "DOCUMENT", "MIME", (const char *)0))
    {
      class = B_CLASS_DATA;
    }
  }

  return class;
}


/*  
 *
 *
 *                            External routines.
 *
 *
 */  


int get_class(v)
  VLINK v;
{
  TOKEN type;
  int class;
  
  if ( ! v) return B_CLASS_UNKNOWN;

  if ((type = m_interpretation(v)) &&
      (class = interp_class(type)) != B_CLASS_UNKNOWN)
  { 
    return class;
  }

  if (strequal("DIRECTORY", v->target))
  {
    return B_CLASS_MENU;
  }

  if (strequal("FILE",     v->target) ||
      strequal("EXTERNAL", v->target))
  {
    const char *suf;

    if ((suf = strrchr(v->name, '.')))
    {
      return ext_class(suf+1);
    }
    else
    {
      /* bug: Live dangerously; assume no suffix implies text. */  
      return B_CLASS_DOCUMENT;
    }
  }

  return B_CLASS_UNKNOWN;
}


static StrVal faketab[] =
{
  { "DATA",      B_CLASS_DATA     },
  { "DIRECTORY", B_CLASS_MENU     },
  { "DOCUMENT",  B_CLASS_DOCUMENT },
  { "FILE",      B_CLASS_DOCUMENT },
  { "SEARCH",    B_CLASS_SEARCH   },

  { STRVAL_END,  B_CLASS_UNKNOWN  }
};


/*  
 *  This should _only_ get called for a link that may have an overriding
 *  class: currently B_CLASS_MENU and B_CLASS_SEARCH.
 *
 *  Return B_CLASS_UNKNOWN if the link does not have a fake class.
 *
 *  Check for special cases that may let us avoid asking for attributes.
 */    
int fake_link_class(v)
  VLINK v;
{
  const char *at;

  if (STRNCMP(v->hsoname, "ARCHIE/HOST") == 0)
  {
    /* Link is a directory in an anonftp search result. */
    return B_CLASS_UNKNOWN;
  }

  if ( ! (at = vlinkAttrStr(POSE_AS, v)))
  {
    /* Link has no POSE-AS attribute. */
    return B_CLASS_UNKNOWN;
  }

  return sval(at, faketab);
}


/*  
 *  Return the real class of the link.  Check for special cases that would
 *  get past get_class(), such as WAIS documents and directories in the
 *  results of archie searches.
 */    
int real_link_class(v)
  VLINK v;
{
  int class;

  if (STRNCMP(v->hsoname, "ARCHIE/HOST") == 0)
  {
    /* so we can descend into directories in archie search results */
    return B_CLASS_MENU;
  }

  if ((class = get_class(v)) == B_CLASS_DATA)
  {
    /* bug? dirsrv should set proper OBJECT-INTERPRETATION attribute? */
    if (STRNCMP(v->hsoname, "ARCHIE") == 0)
    {
      /* so WAIS headlines (for example) look like documents */
      class = B_CLASS_DOCUMENT;
    }
  }

  return class;
}
