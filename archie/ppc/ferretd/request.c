#ifdef __STDC__
# include <stdlib.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "file_type.h"
#include "ppc.h"
#include "protos.h"
#include "request.h"
#include "url.h"
#include "all.h"

static StrVal methods[] =
{
  { "CHECKIN",    HTTP_CHECKIN    },
  { "CHECKOUT",   HTTP_CHECKOUT   },
  { "GET",        HTTP_GET        },
  { "HEAD",       HTTP_HEAD       },
  { "POST",       HTTP_POST       },
  { "PUT",        HTTP_PUT        },
  { "REPLY",      HTTP_REPLY      },
  { "SHOWMETHOD", HTTP_SHOWMETHOD },
  { "SPACEJUMP",  HTTP_SPACEJUMP  },
  { "TEXTSEARCH", HTTP_TEXTSEARCH },

  { STRVAL_END,   HTTP_FAIL       }
};


static int accepted_version proto_((const char *s));
static int is_continuation_line proto_((FILE *ifp));
static void parseAccept proto_((char *s));
  

static int accepted_version(s)
  const char *s;
{
  if (strcmp(s, HTTP_VERSION_STR) == 0)
  {
    return 1;
  }
  else
  {
    efprintf(stderr, "%s: accepted_version: version `%s' is not recognized.\n",
             logpfx(), s);
    return 0;
  }
}


/*  
 *  Assume we are processing RFC822 mail headers, and that the next character
 *  to be read is the first on the line.
 *  
 *  Return 1 if the line is a continuation of the previous line, otherwise
 *  return 0.
 */  
static int is_continuation_line(ifp)
  FILE *ifp;
{
  int c;
  int ret = 0;

  c = timed_getc(ifp);
  if (c == ' ' || c == '\t') ret = 1;
  ungetc(c, ifp);
  return ret;
}


static void check_for_arg(req, epath)
  Request *req;
  char *epath;
{
  char *term;
  
  if ((term = strpbrk(epath, TERM_CHARS)))
  {
    switch (*term)
    {
    case PROP_CHAR:
    case ARG_CHAR:
      /* Separate arguments with spaces, as God intended. */
      tr(term+1, '+', ' ');
      req->arg = url_dequote(term+1, (void *(*)())stalloc);
      break;

    default:
      efprintf(stderr, "%s: check_for_arg: `%c' not in switch!\n", logpfx(), *term);
      break;
    }

    *term = '\0';
  }
}


/*  
 *  Handle `Accept: ' lines.  These can be of the form
 *  
 *  Accept: <entry> [, <entry>]*
 *  
 *  where
 *  
 *  <entry> := <content-type> [; <param]*
 *  <param> := <attr> = <float>
 *  <attr> := q | mxs | mxb
 *  <float> := <ANSI-C floating point text representation>
 *
 *  Assume the lines have been stripped of any trailing \r or \n.
 */  
static void parseAccept(s)
  char *s;
{
  char *term;
      
  term = strend(s);
  *term = ',';
  
  do
  {
    char *e; /* end of the <entry> */
    char *t; /* terminates the <content-type> */
    char tc; /* the character at *t */

    s += strspn(s, " \t");
    e = strchr(s, ',');
    *e = '\0';
    
    t = s + strcspn(s, "; \t");
    tc = *t;
    *t = '\0';
    /* bug: hack for NetScrape, which sends `*' rather than `* / *'. */
    if (strcmp(s, "*") == 0)
    {
      client_accepts("*/*");
    }
    else
    {
      client_accepts(s);
    }
    *t = tc;

    *e = ',';
    s = e + 1;
  } while (s < term);

  *term = '\0';
}



int addToRequest(ifp, r, error)
  FILE *ifp;
  Request *r;
  int *error;
{
  char *line;
  char *s;
  int ret = 1;
  
  *error = 0;
  if ( ! (line = timed_readline(ifp, debug)))
  {
    *error = 1;                 /* requests should end with an empty line */
    return 0;
  }

  /*  
   *  bug: Added check for `.' for APS demo. -wheelan (Mon Feb 28 20:27:35 EST 1994)
   */  
  if(*line == '\0' || (strcmp(line, ".") == 0))
  {
    free(line);
    return 0;
  }

  while (is_continuation_line(ifp))
  {
    char *l, *p;

    if ( ! (l = timed_readline(ifp, debug)))
    {
      ret = 0;
      break;
    }

    /* bug: check return codes */
    p = l + strspn(l, "\t ");
    line = realloc(line, strlen(line) + strlen(p) + 1 + 1);
    strcat(line, " "); /* Just to be safe.  Allowed by RFC822. */
    strcat(line, p);
    free(l);
  }

  if ((s = strchr(line, ':')))
  {
    *s = '\0';
    /* bug: ought to move this out into separate function. */
    if (strcmp(line, "Accept") == 0)
    {
      parseAccept(s+1);
    }
    else if (strcasecmp(line, "Content-Length") == 0)
    {
      r->contlen = atoi(s+1);
    }
#ifdef BUNYIP_AUTHENTICATION
    else if (strcmp(line, "Authorization") == 0)
    {
      r->auth = (char*)malloc(sizeof(char) * (strlen(s + 1) + 1));
      strcpy(r->auth, s+1);
    }
#endif
    *s = ':';
  }

  free(line);
  return ret;
}


/*  
 *  Re: www protocol.
 *  
 *  I assume that no additional data can follow a 2-arg GET.  3-arg commands
 *  may take zero or more mail header lines and are always terminated by
 *  empty lines.  (I don't see how the optional data can follow this, but
 *  what the hell... [Content-Length header.]).
 */  
/* bug: probably ought to save the request line as one string. */
int parse_method(line, req)
  const char *line;
  Request *req;
{
  char **av;
  int ac;
  int ret = HTTP_FAIL;
  
  req->errno = HERR_OK;

  if ( ! splitwhite(line, &ac, &av))
  {
    efprintf(stderr, "%s: parse_method: error from splitwhite().\n", logpfx());
    req->errno = HERR_INTERNAL;
    req->errno = HTTP_FAIL;
  }
  
  switch (ac)
  {
  case 0:
  case 1:
    req->errno = HERR_BAD_REQ;
    req->method = HTTP_FAIL;
    efprintf(stderr, "%s: parse_method: too few arguments in `%s'.\n",
             logpfx(), line);
    break;

  case 2:
    if (sval(av[0], methods) != HTTP_GET)
    {
      req->errno = HERR_BAD_REQ;
      req->method = HTTP_FAIL;
      efprintf(stderr, "%s: parse_method: non-GET method requires three arguments.\n",
               logpfx());
    }
    else
    {
      req->mstr = stcopy(av[0]);
      req->epath = stcopy(av[1]);
      check_for_arg(req, req->epath);
      ret = req->method = HTTP_GET;
    }
    break;

  case 3:
    req->mstr = stcopy(av[0]);
    req->epath = stcopy(av[1]);
    check_for_arg(req, req->epath);
    req->version = stcopy(av[2]);
    if ( ! accepted_version(req->version))
    {
      req->errno = HERR_BAD_REQ;
      req->method = HTTP_FAIL;
    }
    else
    {
      ret = req->method = sval(av[0], methods);
    }
    break;

  default:
    req->errno = HERR_BAD_REQ;
    req->method = HTTP_FAIL;
    efprintf(stderr, "%s: parse_method: too many arguments in `%s'.\n",
             logpfx(), line);
    break;
  }

  free(av);

  return ret;
}


/*  
 *  Convert a Prospero error number to the equivalent HTTP
 *  error number.
 *
 *  Normally, `n' is the value of `perrno'.
 */  
int ptoh_errno(n)
  int n;
{
  int ret;
  
  switch (n)
  {
  case PSUCCESS:
    ret = HERR_OK;
    break;

  case DIRSRV_NOT_DIRECTORY:
  case DIRSRV_NOT_FOUND:
  case PFS_FILE_NOT_FOUND:
  case PFS_DIR_NOT_FOUND:
    ret = HERR_NOT_FOUND;
    break;
    
  case DIRSRV_AUTHENT_REQ:
  case DIRSRV_NOT_AUTHORIZED:
    ret = HERR_NO_AUTH;
    break;
    
  default:
    ret = HERR_INTERNAL;
    break;
  }

  return ret;
}


void freeRequest(r) /* freeRequest? */
  Request *r;
{
  /*bug? clear Where structure?*/
  if (r->mstr) stfree(r->mstr);       r->mstr    = 0;
  if (r->arg) stfree(r->arg);         r->arg     = 0;
  if (r->epath) stfree(r->epath);     r->epath   = 0;
  if (r->version) stfree(r->version); r->version = 0;

  r->contlen = -1;

  if (r->parent) vllfree(r->parent);  r->parent = (VLINK)0;
  if (r->res) vllfree(r->res);        r->res    = (VLINK)0;
  if (r->mesg) stfree(r->mesg);       r->mesg   = 0;
#ifdef BUNYIP_AUTHENTIFICATION
  if (r->auth) free(r->auth);         r->auth   = 0;
#endif
  r->search_type = SRCH_;
  r->errno = HERR_OK;
  r->res_type = RES_;
}


void initRequest(r, us)
  Request *r;
  const Where us;
{
  r->mstr = (char *)0;
  r->arg = (char *)0;
  r->epath = (char *)0;
  r->version = (char *)0;
  r->method = HTTP_FAIL;

  r->contlen = -1;

  r->parent = (VLINK)0;
  r->res = (VLINK)0;
  r->mesg = (char *)0;
  r->search_type = SRCH_;
  r->errno = HERR_OK;           /* bug? should this start off unset, too? */
  r->res_type = RES_;           /* unset */

  r->here = us;
#ifdef BUNYIP_AUTHENTIFICATION
  r->auth = (char*)0;
#endif
}


void setParent(req, parent)
  Request *req;
  VLINK parent;
{
  req->parent = parent;
}


void setResponse(Request *req, VLINK res, enum Status err, enum Response rtype, const char *mesg, ...)
{
  va_list ap;

  if (req->res) vllfree(req->res);
  req->res = res;

  req->errno = err;
  req->res_type = rtype;

  if (req->mesg)
  {
    stfree(req->mesg);
    req->mesg = 0;
  }
  if (mesg)
  {
    int len;

    va_start(ap, mesg);
    if ((len = vslen(mesg, ap)) < 0)
    {
      efprintf(stderr, "%s: setResponse: vslen() failed on `%s', ...\n",
               logpfx(), mesg);
    }
    else
    {
      if ((req->mesg = stalloc(len + 1)))
      {
        vsprintf(req->mesg, mesg, ap);
      }
    }
    va_end(ap);
  }
}
