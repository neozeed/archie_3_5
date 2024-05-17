/*
 *  A request currently has the form:
 *  
 *  <c-gopher-type>@<s-vlink-host>:<s-vlink-hsoname>['\b'<s-propagate>]
 */  

#ifdef __STDC__
#include <stdlib.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>  /* Requires this to avoid MAXHOSTNAMELEN to have
                           different values between modules */
#include "aprint.h"
#include "contents.h"
#include "defs.h"
#include "ppc.h"
#include "protos.h"
#include "url.h"


static char *do_gopher_portal proto_((FILE *ofp, VLINK v, const char *desc));
static int decode_request proto_((const char *req, char **epath, char **srch));
static int gopher_results proto_((FILE *ofp, VLINK v, int stype, int print_as_dir, const Where here));
static int display_menu proto_((FILE *ofp, VLINK v, const char *propstr, const Where here));
static int handle_search proto_((FILE *ofp, VLINK v, const char *srch, const Where here));
static int handle_request proto_((FILE *ofp, char *req, const Where us));


/*  
 *  Generate a listing for the Prospero directory given by `v'.
 *  
 *  This routine is considered to have worked if there are no directory
 *  entries to print, or if at least one entry is successfully printed.
 */  
static int display_menu(ofp, v, propstr, here)
  FILE *ofp;
  VLINK v;
  const char *propstr;
  const Where here;
{
  int ret = 0;

  if ( ! v)
  {
    efprintf(stderr, "%s: display_menu: VLINK parameter is NULL.\n", logpfx());
  }
  else
  {
    VLINK m;

    if ( ! (m = m_get_menu(v)))
    {
      efprintf(stderr, "%s: display_menu: error getting contents of menu: %s.\n",
               logpfx(), STR(m_error));
    }
    else
    {
      VLINK p;
    
      for (p = m; p; p = p->next)
      {
        char *desc;
        char *epath;
        char *s = 0;

        /*  
         *  Ignore invisible links.
         */
        if (p->linktype == 'I')
        {
          continue;
        }
        
        /*  
         *  Special check.  In some cases we can foist the client off into
         *  Gopher space.
         */    
        if (STRNCMP(p->hsoname, "GOPHER-GW") == 0)
        {
          if ( ! (s = get_gopher_info(p)))
          {
            efprintf(stderr, "%s: display_menu: get_gopher_info() failed on `%s:%s'.\n",
                     logpfx(), p->host, p->hsoname);
          }
          else
          {
            fputs(s, ofp);
#if 1
            fputs("\r\n", ofp);
#endif
            free(s);
          }
          continue;
        }

        if ( ! (desc = m_item_description(p)))
        {
          efprintf(stderr, "%s: display_menu: item `%s:%s' has no description.\n",
                   logpfx(), p->host, p->hsoname);
          continue;
        }

        if ( ! (epath = vlink_to_url(p)))
        {
          efprintf(stderr, "%s: display_menu: can't make path from `%s:%s'.\n",
                   logpfx(), v->host, v->hsoname);
        }
        else
        {
          const char *gtype;
          
          switch (real_link_class(p))
          {
#define PROP(s) (*s ? "&" : ""), s

#if 0
            /*bug: not yet defined in the menu library*/
          case B_CLASS_SOUND:    break;
          case B_CLASS_VOID:     break;
#endif
          case B_CLASS_DATA:     gtype = "9"; goto mkstr;
          case B_CLASS_DOCUMENT: gtype = "0"; goto mkstr;
          mkstr:
            if (strcmp(p->target, "EXTERNAL") == 0)
            {
              /* Assume it is part of the results of an archie search. */
              s = strsdup(gtype, desc, "\t", "ftp:", p->host, "@", p->hsoname,
                          "\t", here.host, "\t", here.port, (char *)0);
            }
            else
            {
              s = strsdup(gtype, desc, "\t", epath, PROP(propstr),
                          "\t", here.host, "\t", here.port, (char *)0);
            }
            break;
            
          case B_CLASS_IMAGE:
            {
              const char *dot;

              gtype = ((dot = strrchr(p->hsoname, '.')) &&
                       strcasecmp(dot+1, "gif") == 0) ? "g" : "I";
              if (strcmp(p->target, "EXTERNAL") == 0)
              {
                /* Assume it is part of the results of an archie search. */
                s = strsdup(gtype, desc, "\t", "ftp:", p->host, "@", p->hsoname,
                            "\t", here.host, "\t", here.port, (char *)0);
              }
              else
              {
                s = strsdup(gtype, desc, "\t", epath, PROP(propstr),
                            "\t", here.host, "\t", here.port, (char *)0);
              }
            }
            break;
            
          case B_CLASS_MENU:
            if (fake_link_class(p) == B_CLASS_UNKNOWN)
            {
              /* It really is a menu. */
              s = strsdup("1", desc, "\t", epath, PROP(propstr),
                          "\t", here.host, "\t", here.port, (char *)0);
            }
            else
            {
              /* Make it look like a search. */
              char *ppath;
              
              if ( ! (ppath = vlink_to_url(v)))
              {
                s = 0;
              }
              else
              {
                s = strsdup("7", desc, "\t", ppath, "/", p->name, PROP(propstr),
                            "\t", here.host, "\t", here.port, (char *)0);
                free(ppath);
              }
            }
            break;

          case B_CLASS_PORTAL:
            /* Must choose between telnet and tn3270. */
            s = do_gopher_portal(ofp, p, desc);
            break;

          case B_CLASS_SEARCH:
            {
              /*  
               *  So we don't end up with, for example, `foo.edu:ARCHIE', we
               *  encode searches as `foo.edu:<parent-path>/<child-name>'.
               */  
              char *ppath;

              if ( ! (ppath = vlink_to_url(v)))
              {
                s = 0;
              }
              else
              {
                char *type;

                switch (fake_link_class(p))
                {
                case B_CLASS_DOCUMENT: type = "0"; break;
                case B_CLASS_MENU:     type = "1"; break;
                default:               type = "7"; break;
                }
                s = strsdup(type, desc, "\t", ppath, "/", p->name, PROP(propstr),
                            "\t", here.host, "\t", here.port, (char *)0);
                free(ppath);
              }
            }
            break;
#undef PROP

          case B_CLASS_UNKNOWN: 
            efprintf(stderr, "%s: display_menu: `%s:%s' is B_CLASS_UNKNOWN!.\n",
                     logpfx(), p->host, p->hsoname);
            s = 0;
            break;

          default:
            efprintf(stderr, "%s: display_menu: unknown class, %d, for `%s:%s'.\n",
                     logpfx(), real_link_class(p), p->host, p->hsoname);
            s = 0;
            break;
          }

          ret |= !!s;
          free(epath);
          if (s)                /*bug: can be reset???*/
          {
            efprintf(ofp, "%s\r\n", s);
            free(s);
          }
        }
      }

      vllfree(m);
    }
  }
  
  return ret;
}


static char *do_gopher_portal(ofp, v, desc)
  FILE *ofp;
  VLINK v;
  const char *desc;
{
  TOKEN at;
  char *ret = 0;

  if ( ! v)
  {
    efprintf(stderr, "%s: gopher_portal: VLINK parameter is NULL.\n", logpfx());
  }
  else
  {
    const char *gtype;
    int r;

    if ((r = pget_am(v, &at, P_AM_TELNET)) == P_AM_TELNET)
    {
      gtype = "8";
    }
#if 0
    /*bug: this type not yet defined by Prospero*/
    else if ((r = pget_am(v, &at, P_AM_TN3270)) == P_AM_TN3270)
    {
      gtype = "T";
    }
#endif

    /* bug: what if weird access method? */

    if ( ! r)
    {
      efprintf(stderr, "%s: gopher_portal: no access method: %s\n",
               logpfx(), p_err_string);
    }
    else
    {
      char host[MAXHOSTNAMELEN+1];
      char port[16];

      if ( ! getHostPort(v->host, host, port))
      {
        efprintf(stderr, "%s: gopher_portal: malformed host name, `%s'?\n",
                 logpfx(), v->host);
      }
      else
      {
        ret = strsdup(gtype, desc, "\t\t", host, "\t", port, (char *)0);
      }
    }
  }
  return ret;
}


/*  
 *  `v' is a VLINK corresponding to an item in Gopher space.  If we have a
 *  complete line, suitable for a Gopher client, in a `GOPHER-MENU-ITEM'
 *  attribute, we can redirect the client into Gopher space.  Otherwise,
 *  construct the line ourselves and point it back to us.
 */  
char *get_gopher_info(v)
  VLINK v;
{
  char *atstr;
  char *ret = 0;
  char *sl;
  
  if ((atstr = vlinkAttrStr("GOPHER-MENU-ITEM", v)))
  {
    ret = strdup(atstr);
  }
  else if ((sl = strchr(v->hsoname, '/')))
  {
    char gtype[2];
    char host[MAXHOSTNAMELEN+1];
    char sel[768]; /* bug!!! consider the size of some of _our_ selector strings!!! */
    char port[16];
    int n;
      
    gtype[1] = '\0'; sel[0] = '\0';
    n = sscanf(sl+1, "%[^(](%[0-9])/%c/%[^\t]", host, port, gtype, sel);
    if (n == 3 || n == 4)
    {
      char *desc;

      if ( ! (desc = m_item_description(v)))
      {
        desc = v->name;
      }
      ret = strsdup(gtype, desc, "\t", sel, "\t", host, "\t", port, (char *)0);
      efprintf(stderr, "%s: get_gopher_info: pointer into Gopher space.\n",
               logpfx());
    }
    else
    {
      efprintf(stderr, "%s: get_gopher_info: malformed hsoname, `%s'.\n",
               logpfx(), v->hsoname);
    }
  }

  return ret;
}


/*  
 *  `req' is the request string sent to us by a Gopher client.
 *  
 *  If a search string or propagated search string is found, allocate memory
 *  for a copy and point *srch at that copy.
 *  
 *  Allocate memory for a copy of the encoded path, pointing *epath to that
 *  memory.  Don't dequote the path, it will be by the caller.
 */  
static int decode_request(req, epath, srch)
  const char *req;
  char **epath;
  char **srch;
{
  char *term;

  if ( ! ((term = strchr(req, '\t')) || (term = strchr(req, '&'))))
  {
    term = strend((char *)req);
  }

  *srch = strdup(*term ? term+1 : term); /* skip the TAB or `&', if necessary */
  *epath = strndup(req, term - req);
  
  return 1;
}


/*  
 *  Guess whether or not the request is an FTP selector string.
 */
static int is_ftp_request(req)
  const char *req;
{
  const char *p;
  
  return (p = strskip(req, "ftp:")) && (p = strchr(p, '@')) && *(p+1) == '/';
}


/*  
 *  bug: Replace this with table of function pointers.
 *
 *  Must handle no results, both as file and directory.
 *  -wheelan (Sun Dec 19 00:44:11 EST 1993)
 */  
static int gopher_results(ofp, v, stype, print_as_dir, here)
  FILE *ofp;
  VLINK v;
  int stype;
  int print_as_dir;
  const Where here;
{
  int ret = 1;

  if ( ! v)
  {
    if (print_as_dir)
    {
      efprintf(ofp, "3No results found.\r\n");
    }
    else
    {
      efprintf(ofp, "\r\n    No results found.\r\n\r\n");
    }
  }
  else
  {
    switch (stype)
    {
    case SRCH_ANONFTP:
      anon_print(ofp, v, print_as_dir, here);
      break;

#if 0
    case SRCH_DOMAINS:
      domain_print(ofp, v, print_as_dir, here);
      break;
#endif
    
    case SRCH_ERROR:
      efprintf(stderr, "%s: gopher_results: erroneous search type?\n", logpfx());
      efprintf(ofp, "3Search type error.\r\n");
      break;
    
    case SRCH_GOPHER:
      gopher_print(ofp, v, print_as_dir, here);
      break;
    
#if 0
    case SRCH_SITELIST:            /* not currently supported */
#endif

    case SRCH_SITES:
      print_sites(ofp, v, print_as_dir, here);
      break;

    case SRCH_UNKNOWN:
      efprintf(stderr, "%s: gopher_results: unknown search type.\n", logpfx());
      efprintf(ofp, "3Unknown search type.\r\n");
      break;
    
    case SRCH_WAIS:
      print_wais(ofp, v, print_as_dir, here);
      break;
    
#if 0
    case SRCH_WHATIS:
      whatis_print(ofp, v, print_as_dir, here);
      break;
#endif

    default:
      efprintf(ofp, "3Temporary -- bad search.\r\n");
      break;
    }
  }
  
  return ret;
}


/*  
 *  `req' is a Prospero link encoded as a URL, possibly with a trailing
 *  `&<string>', which is a search string to be propagated.
 *  
 *  If the VLINK corresponding to `req' is a search, then there should either
 *  be a propagated string appended to the URL, or a TAB followed by the
 *  search string.
 */  
static int handle_request(ofp, req, here)
  FILE *ofp;
  char *req;
  const Where here;
{
  VLINK v = 0;
  char *epath = 0;
  char *srch = 0;
  int class;
  int ret = 0;

  dfprintf(stderr, "%s: handle_request: request is `%s'.\n", logpfx(), req);

  if (*req == '\0') req = ".";

  /*  
   *  Check whether we are being asked to fetch an FTPable file.
   */
  if (is_ftp_request(req))
  {
#if 1
    efprintf(ofp,
             "\r\n\tSorry, but this service cannot retrieve files for you, via FTP.\r\n\r\n");

#else

    efprintf(ofp,
             "\r\n\tSorry, this server cannot FTP files for you,"
             "\r\n\talthough there are Gopher clients that can.\r\n\r\n");
#endif
    return 0;
  }
  
  if ( ! decode_request(req, &epath, &srch))
  {
    efprintf(stderr, "%s: handle_request: can't decode request.\n", logpfx());
    return 0;
  }

  if ( ! (v = url_to_vlink(epath)))
  {
    efprintf(stderr, "%s: handle_request: can't get VLINK from path.\n",
             logpfx());
    if (epath) free(epath);
    if (srch) free(srch);
    return 0;
  }

  switch (class = real_link_class(v))
  {
  case B_CLASS_DATA:            /* 4, 5, 6, 9 */
  case B_CLASS_DOCUMENT:        /* 0 */
  case B_CLASS_IMAGE:           /* g, I */
#if 0
  case B_CLASS_SOUND:
  case B_CLASS_VOID:
#endif

    ret = blat_file(ofp, v);
    if (class == B_CLASS_DOCUMENT)
    {
      efprintf(ofp, ".\r\n");
    }
    break;

  case B_CLASS_MENU:            /* 1 */
    ret = display_menu(ofp, v, srch, here);
    efprintf(ofp, ".\r\n");
    break;

  case B_CLASS_PORTAL:          /* 8, T */
    /*  
     *  A valid request should never pick up a portal link.
     */  
    efprintf(stderr, "%s: handle_request: unexpected request for portal link, `%s'.\n",
             logpfx(), req);
    efprintf(ofp, "3`%s' is a portal!.\r\n", req);
    break;

  case B_CLASS_SEARCH:          /* 7 */
    /*  
     *  This includes "fake" searches (i.e. directories disguised as
     *  searches).
     */  
    ret = handle_search(ofp, v, srch, here);
    break;
    
  case B_CLASS_UNKNOWN:         /* error: just ignore? */
    efprintf(stderr, "%s: handle_request: `%s' is B_CLASS_UNKNOWN!.\n",
             logpfx(), req);
    efprintf(stderr, "3Internal error on `%s'.\r\n", req);
    break;
    
  default:
    efprintf(stderr, "%s: handle_request: `%s' is an unrecognized type (%d).\n",
             logpfx(), req, real_link_class(v));
    efprintf(ofp, "3Unknown type for `%s'.\r\n", req);
    break;
  }

  dfprintf(stderr, "%s: handle_request: request %s.\n", logpfx(),
           ret ? "succeeded" : "failed");
  if (v) vllfree(v);
  if (epath) free(epath);
  if (srch) free(srch);
  return ret;
}


/*  
 *  This is called if `v' is _really_ a search.  In particular, it won't be a
 *  directory disguised as a search.
 */  
static int handle_search(ofp, v, srch, here)
  FILE *ofp;
  VLINK v;
  const char *srch;
  const Where here;
{
  VLINK results;
  int print_as_dir;
  int ret = 0;
  enum SearchType stype;
  
  if ( ! srch)
  {
    efprintf(stderr, "%s: handle_search: no search string for `%s:%s'.\n",
             logpfx(), v->host, v->hsoname);
    efprintf(ofp, "3No search string given!\r\n");
  }
  else
  {
#ifndef NO_TCL
    char *dbname;
#endif
    
    print_as_dir = display_as_dir(v);
    results = search(v, srch, 0, &stype);

#ifndef NO_TCL
    if ( ! (dbname = vlinkAttrStr(HEADLINE_FORMAT, v)))
    {
      dbname = "{}";
    }
    if ( ! (ret = tcl_headlines_print(ofp, dbname, "{}", print_as_dir, results)))
    {
      ret = gopher_results(ofp, results, stype, print_as_dir, here);
    }

#else

    ret = gopher_results(ofp, results, stype, print_as_dir, here);
#endif
    vllfree(results);

#if 1
    /*  
     *  Search results weren't being terminated by a period.
     *  
     *  - wheelan (Thu Mar 7 18:01:41 EST 1996)
     */  

    efprintf(ofp, ".\r\n");

#else

    if ( ! display_as_dir(v))
    {
      efprintf(stderr, ".\r\n");
    }
#endif
  }

  return ret;
}


/*  
 *
 *
 *                             External routines.
 *
 *
 */  

void access_denied(ifp, ofp, us)
  FILE *ifp;
  FILE *ofp;
  const Where us;
{
  int c;

  while ((c = fgetc(ifp)) != '\n' && c != EOF) /* drain input */
  { continue; }

  efprintf(ofp, "1%s\t3\t%s\t%s\r\n",
#ifdef UNAUTH_MESG
           UNAUTH_MESG,
#else
           "Sorry, you are not authorized to use this service",
#endif
           us.host, us.port);
}


const char *prosp_ident()
{
  return "ggBw";
}


const char *service_str()
{
  return "gopher";
}


void handle_transaction(ifp, ofp, us, them)
  FILE *ifp;
  FILE *ofp;
  const Where us;
  const Where them;
{
  char *line;

  if ( ! (line = timed_readline(ifp, debug)))
  {
    efprintf(stderr, "%s: handle_transaction: timed_readline() failed.\n", logpfx());
  }
  else
  {
    efprintf(stderr, "%s: handle_transaction: request from `%s' is `%s'.\n",
             logpfx(), them.host, line);
    handle_request(ofp, line, us);
    efprintf(stderr, "%s: handle_transaction: handle_request() finished.\n",
             logpfx());
    free(line);
  }
  fflush(ofp);
}
