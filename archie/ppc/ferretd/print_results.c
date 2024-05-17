#include <stdio.h>
#include "aprint.h"
#include "contents.h"
#include "file_type.h"
#include "print_results.h"
#include "request.h"
#include "url.h"
#include <assert.h> /* to override Prospero's assert */
#include "all.h"


static void error_print proto_((FILE *ofp, VLINK v, const Where here, int print_as_dir));
static void internalError proto_((FILE *ofp, Request *req));
static void mime_header proto_((FILE *ofp, Request *req, const char *ctype, const char *stype, const char *enc, long size));
static void print_file proto_((FILE *ofp, Request *req));
static void print_head proto_((FILE *ofp, Request *req));
static void print_menu proto_((FILE *ofp, Request *req));
static void print_search_results proto_((FILE *ofp, Request *req));
static void status proto_((FILE *ofp, Request *req));
static void unknown_print proto_((FILE *ofp, VLINK v, const Where here, int print_as_dir));

#define AUTHORIZATION_FILE "/ERRORS/authorization.html"

/*  
 *
 *                             Internal routines
 *
 */  


static void internalError(ofp, req)
  FILE *ofp;
  Request *req;
{
  req->errno = HERR_INTERNAL;

  status(ofp, req);
  mime_header(ofp, req, "text", "html", "7bit", -1);
  efprintf(ofp,
           "<BODY>\r\n"
           "Server error: something _really_ unexpected happened!\r\n"
           "</BODY>\r\n");
}


static void mime_header(ofp, req, ctype, stype, enc, size)
  FILE *ofp;
  Request *req;
  const char *ctype;
  const char *stype;
  const char *enc;
  long size;
{
#if 0
  char date[64];
  time_t t;
#endif

  if (req->version) /* assume non-NULL means 1.0 or greater */
  {
#if 0
    t = time((time_t *)0);
    /*  
     *  bug: use of locale could screw up RFC822 compliance (we could force C
     *  local...)
     */  
    strftime(date, sizeof date, "%a, %d %h %y %T GMT", gmtime(&t));
    efprintf(ofp, "Date: %s\r\n", date);
#endif
    efprintf(ofp, "Server: Ferretd/0.0\r\n");
    efprintf(ofp, "MIME-Version: 1.0\r\n");
    efprintf(ofp, "Content-Type: %s/%s\r\n", ctype, stype);
    efprintf(ofp, "Content-Transfer-Encoding: %s\r\n", enc);

    if (size >= 0)
    {
      efprintf(ofp, "Content-Length: %ld\r\n", size);
    }
    efprintf(ofp, "\r\n");
  }
}


/*  
 *  bug: this should be done by mime_header().
 */
static void redirection_header(ofp, req)
  FILE *ofp;
  Request *req;
{
#if 0
  char date[64];
  time_t t;
#endif

  if (req->version) /* assume non-NULL means 1.0 or greater */
  {
#if 0
    t = time((time_t *)0);
    /*  
     *  bug: use of locale could screw up RFC822 compliance (we could force C
     *  locale...)
     */  
    strftime(date, sizeof date, "%a, %d %h %y %T GMT", gmtime(&t));
    efprintf(ofp, "Date: %s\r\n", date);
#endif
    efprintf(ofp, "Server: Ferretd/0.0\r\n");
    efprintf(ofp, "MIME-Version: 1.0\r\n");
    efprintf(ofp, "Content-Type: %s/%s\r\n", "text", "html");
    efprintf(ofp, "Content-Transfer-Encoding: %s\r\n", "7bit");

    efprintf(ofp, "Content-Length: %ld\r\n", 0L);
    efprintf(ofp, "URI: %s\r\n", req->mesg);
    efprintf(ofp, "Location: %s\r\n", req->mesg);
    efprintf(ofp, "\r\n");
  }
}


/*  
 *  bug: logic should be rearranged so that non-local contents are retrieved
 *  before any headers are printed.  Also, see what can be merged with weaseld.
 *
 *  Currently we can't print both a message and a file.  If this is changed be
 *  careful about not sending headers twice, and getting the content type and
 *  encoding right.
 */  
static void print_file(ofp, req)
  FILE *ofp;
  Request *req;
{
  /* if message: print it */
  if (req->mesg)
  {
    status(ofp, req);
    mime_header(ofp, req, "text", "html", "7bit", (long)strlen(req->mesg));
    efprintf(ofp, "%s", req->mesg);
  }
  else if (req->res)
  {
    const char *ctype;
    const char *stype;
    const char *enc;
    long size;
    
    content_type_of(req->res, &ctype, &stype, &enc);
    if ( ! match_content_type(&ctype, &stype, &enc))
    {
      setResponse(req, 0, HERR_BAD_REQ, RES_FILE,
                  "Sorry, but the file you requested has type \"%s/%s\",\r\n"
                  "which is not among the types your client can handle.\r\n",
                  ctype, stype);
      status(ofp, req);
      mime_header(ofp, req, "text", "html", "7bit", (long)strlen(req->mesg));
      efprintf(ofp, "%s", req->mesg);
    }
    else
    {
      if (file_is_local(req->res))
      {
        size = local_file_size(req->res);
      }
      else
      {
        size = contents_size(req->res); /*bug: yet another Prospero request */
      }

      status(ofp, req);
      mime_header(ofp, req, ctype, stype, enc, size);
      blat_file(ofp, req->res);
    }
  }
  else
  {
    internalError(ofp, req);
  }
}

static void print_head(ofp, req)
  FILE *ofp;
  Request *req;
{
  const char *ctype;
  const char *stype;
  const char *enc;
  long size;

  if ( ! req->res)
  {
    size = -1;
    ctype = "application";
    stype = "octet-stream";
    enc = "binary";
  }
  else
  {
    content_type_of(req->res, &ctype, &stype, &enc);
    if (file_is_local(req->res))
    {
      size = local_file_size(req->res);
    }
    else
    {
      size = contents_size(req->res); /*bug: yet another Prospero request */
    }
  }

  status(ofp, req);
  mime_header(ofp, req, ctype, stype, enc, size);
}

#ifdef BUNYIP_AUTHENTICATION
static void print_auth(ofp, req)
  FILE *ofp;
  Request *req;
{
  VLINK v;
  char *e;

  status(ofp, req);
  efprintf(ofp, "WWW-Authenticate: Basic realm=\"Test\"\r\n");
  
  mime_header(ofp, req, "text", "html", "8bit", -1);
  e = AUTHORIZATION_FILE;

  v = url_to_vlink(e);
  if (v)
  {
    blat_file(ofp, v);
  }
}
#endif

static void print_menu(ofp, req)
  FILE *ofp;
  Request *req;
{
  VLINK p;

  if ( ! req->parent && ! req->res)
  {
    efprintf(stderr, "%s: print_menu: parent and result links are null!\n",
             logpfx());
    internalError(ofp, req);
  }

  status(ofp, req);
  mime_header(ofp, req, "text", "html", "7bit", -1);

  if (req->parent)
  {
    char *d;

    d = ppc_m_item_description(req->parent);
    efprintf(ofp,
             "<HEAD><TITLE>%s directory</TITLE></HEAD>\r\n"
             "<BODY><H1>%s</H1><DIR>\r\n",
             d, d);
  }
  
  for (p = req->res; p; p = p->next)
  {
    char *epath;
    
    /*  
     *  We don't need this for B_CLASS_SEARCH, but we'll need it most of the
     *  time, and it seems cleaner here...
     */
    if ( ! (epath = vlink_to_url(p)))
    {
      efprintf(stderr, "%s: print_menu: can't make URL from `%s:%s'.\n",
               logpfx(), p->host, p->hsoname);
      continue;
    }

    switch (real_link_class(p))
    {
    case B_CLASS_DATA:
    case B_CLASS_DOCUMENT:
    case B_CLASS_IMAGE:
      if (req->arg)
      {
        efprintf(ofp, "<LI><A HREF=\"http://%s:%s/%s%s%s\">%s</A>\r\n",
                 req->here.host, req->here.port, epath, PROP_STR, req->arg,
                 ppc_m_item_description(p));
      }
      else if (strcmp(p->target, "EXTERNAL") == 0)
      {
        /*  
         *  Assume a target of EXTERNAL implies a file from an anonftp search
         *  result.
         */  
        char *url;

        if ((url = UrlFromVlink(p, URL_FTP)))
        {
          efprintf(ofp, "<LI><A HREF=\"%s\">%s</A>\r\n", url, ppc_m_item_description(p));
          free(url);
        }
        else
        {
          efprintf(stderr, "%s: print_menu: UrlFromVlink() failed.\n", logpfx());
        }
      }
      else
      {
        efprintf(ofp, "<LI><A HREF=\"http://%s:%s/%s\">%s</A>\r\n",
                 req->here.host, req->here.port, epath, ppc_m_item_description(p));
      }
      break;
                
    case B_CLASS_MENU:
      if (req->arg)
      {
        efprintf(ofp, "<LI><A HREF=\"http://%s:%s/%s%s%s\">%s</A>\r\n",
                 req->here.host, req->here.port, epath, PROP_STR, req->arg,
                 ppc_m_item_description(p));
      }
      else
      {
        efprintf(ofp, "<LI><A HREF=\"http://%s:%s/%s\">%s</A>\r\n",
                 req->here.host, req->here.port, epath, ppc_m_item_description(p));
      }
      break;
                
    case B_CLASS_SEARCH:
      {
        char *pepath;

        if ( ! (pepath = vlink_to_url(req->parent)))
        {
          efprintf(stderr, "%s: print_menu: can't make URL from `%s:%s'.\n",
                   logpfx(), p->host, p->hsoname);
        }
        else if (req->arg)
        {
          efprintf(ofp, "<LI><A HREF=\"http://%s:%s/%s/%s%s%s\">%s</A>\r\n",
                   req->here.host, req->here.port, pepath, p->name, PROP_STR,
                   req->arg, ppc_m_item_description(p));
          free(pepath);
        }
        else
        {
          efprintf(ofp, "<LI><A HREF=\"http://%s:%s/%s/%s\">%s</A>\r\n",
                   req->here.host, req->here.port, pepath, p->name,
                   ppc_m_item_description(p));
          free(pepath);
        }
      }
      break;

    case B_CLASS_PORTAL:
      efprintf(stderr, "%s: print_menu: ignoring portal for now.\n", logpfx());
      break;

    case B_CLASS_UNKNOWN: 
      efprintf(stderr, "%s: print_menu: unknown link class.\n", logpfx());
      break;

#if 0
    case B_CLASS_SOUND:    break;
    case B_CLASS_VOID:     break;
#endif

    default:
      efprintf(stderr, "%s: print_menu: unknown class type, %d.\n",
               logpfx(), real_link_class(p));
      break;
    }

    free(epath);
  }

  /* Menu trailer */
  efprintf(ofp, "</DIR></BODY>\r\n");
}


static void print_redirection(ofp, req)
  FILE *ofp;
  Request *req;
{
  status(ofp, req);
  redirection_header(ofp, req);
}


static void unknown_print(ofp, v, here, print_as_dir)
  FILE *ofp;
  VLINK v;
  const Where here;
  int print_as_dir;
{
  efprintf(stderr, "%s: unknown_print: unknown search type.\n", logpfx());
}


static void error_print(ofp, v, here, print_as_dir)
  FILE *ofp;
  VLINK v;
  const Where here;
  int print_as_dir;
{
  efprintf(stderr, "%s: error_print: erroneous search type?\n", logpfx());
}


static PtrVal prtfns[] =
{
  { anon_print,     SRCH_ANONFTP  },
/*{ domain_print,   SRCH_DOMAINS  },*/
  { error_print,    SRCH_ERROR    },
  { gopher_print,   SRCH_GOPHER   },
/*{ sitelist_print, SRCH_SITELIST },*/
  { sites_print,    SRCH_SITES    },
  { wais_print,     SRCH_WAIS     },
/*{ whatis_print,   SRCH_WHATIS   },*/
  { unknown_print,  SRCH_UNKNOWN  },

  { PTRVAL_END,     0             }
};


/* bug: have to search type in here... */
static void print_search_results(ofp, req)
  FILE *ofp;
  Request *req;
{
  const void *fn;
  int dir_format = display_as_dir(req->parent);

  assert(req->search_type != SRCH_);
  if ( ! (fn = ptr(req->search_type, prtfns)))
  {
    efprintf(stderr, "%s: print_search_results: couldn't find print function for type %d.\n",
             logpfx(), req->search_type);
    internalError(ofp, req);
  }
  else
  {
#ifndef NO_TCL
    PATTRIB cat;
    char *dbname;
    
    status(ofp, req);
    mime_header(ofp, req, "text", "html", "8bit", -1);
    
    /*  
     *  bug: this is a kludge.  It would be painful to put the equivalent
     *  check in the Tcl scripts.  On the other hand `fn' (for WAIS results)
     *  already calls `immediatePrint'.
     *  
     *  - wheelan (Sun Dec 11 20:59:07 EST 1994)
     *  
     */
    if (immediatePrint(req->res, &cat))
    {
      tagprint(ofp, cat);
    }
    else
    {
      if ( ! (dbname = vlinkAttrStr(HEADLINE_FORMAT, req->parent)))
      {
        dbname = "{}";
      }
      if ( ! tcl_headlines_print(ofp, dbname, req->arg, dir_format, req->res))
      {
        ((int (*) PRT_PARMS)fn)(ofp, req, dir_format);
      }
    }

#else

    status(ofp, req);
    mime_header(ofp, req, "text", "html", "8bit", -1);
    
    ((int (*) PRT_PARMS)fn)(ofp, req, dir_format);
#endif
  }
}


static void status(ofp, req)
  FILE *ofp;
  Request *req;
{
  if (req->version)
  {
    const char *s;
    
    switch (req->errno)
    {
    case HERR_:
      efprintf(stderr, "%s: status: request errno is unset!\n", logpfx());
      internalError(ofp, req);
      return;
      
    case HERR_BAD_REQ:   s = "Bad Request"     ; break;
    case HERR_CREATED:   s = "Created"         ; break;
    case HERR_FORBIDDEN: s = "Forbidden"       ; break;
    case HERR_FOUND:     s = "Found"           ; break;
    case HERR_INTERNAL:  s = "Internal Error"  ; break;
    case HERR_METHOD:    s = "Method"          ; break;
    case HERR_MOVED:     s = "Moved"           ; break;
    case HERR_NOT_FOUND: s = "Not Found"       ; break;
    case HERR_NO_AUTH:   s = "Unauthorized"    ; break;
    case HERR_OK:        s = "OK"              ; break;
    case HERR_PAY_REQ:   s = "Payment Required"; break;
    case HERR_UNIMP:     s = "Not Implemented" ; break;
    default:             s = "Unknown Error"   ; break;
    }

    efprintf(ofp, "%s %03d %s\r\n", req->version, req->errno, s);
  }
}


/*  
 *
 *                             External routines
 *
 */  

void print_results(ofp, req)
  FILE *ofp;
  Request *req;
{
  switch (req->res_type)
  {
  case RES_:
    efprintf(stderr, "%s: print_results: result type unset!\n", logpfx());
    internalError(ofp, req);
    break;
    
  case RES_FILE:
    print_file(ofp, req);
    break;
    
  case RES_HEAD:
    print_head(ofp, req);
    break;
    
  case RES_MENU:
    print_menu(ofp, req);
    break;
    
  case RES_REDIRECT:
    print_redirection(ofp, req);
    break;
    
  case RES_SEARCH:
    print_search_results(ofp, req);
    break;

  case RES_AUTH:
#ifdef BUNYIP_AUTHENTICATION
    print_auth(ofp, req);
#endif
    break;
    
  default:
    efprintf(stderr, "%s: print_results: unknown result type, %d.\n",
             logpfx(), req->res_type);
    internalError(ofp, req);
    break;
  }
}


void print_vllength(ofp, req)
  FILE *ofp;
  Request *req;
{
  VLINK v;
  int nitems;
    
  for (nitems = 0, v = req->res; v; v = v->next)
  {
    nitems++;
  }

  efprintf(ofp, "<HEAD><TITLE>Items matching `%s'</TITLE></HEAD>\r\n", req->arg);
  efprintf(ofp, "<BODY><H2>%d item%s matched `%s'</H2>\r\n",
           nitems, nitems == 1 ? "" : "s", req->arg);
}
