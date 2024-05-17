#include <stdio.h>
#include <string.h>
#ifdef __STDC__
# include <stdarg.h>
# include <stdlib.h>
# include <unistd.h>
#endif
#include "aprint.h"
#ifdef BUNYIP_AUTHENTICATION
#include "authorization.h"
#endif
#include "contents.h"
#include "http.h"
#include "image_map.h"
#include "ppc.h"
#include "protos.h"
#include "request.h"
#include "search.h"
#include "url.h"
#include <assert.h> /* to override Prospero's assert() */
#include "all.h"


#define HTML_MENU_FILE "default.html"


static VLINK http_req_to_vlink proto_((Request *req));
static void http_head proto_((Request *req));
static void http_post proto_((Request *req, FILE *ifp, FILE *ofp));


/*  
 *
 *
 *                             Internal routines.
 *
 *
 */  


static void http_head(req)
  Request *req;
{
  VLINK v;

  if ((v = http_req_to_vlink(req)))
  {
    setResponse(req, v, HERR_OK, RES_HEAD, 0);
  }
}


static void http_post(req, ifp, ofp)
  Request *req;
  FILE *ifp;
  FILE *ofp;
{
  FILE *tfp;
  PATTRIB at;
  VLINK v;
  char **args;

  if (req->contlen == -1)
  {
    efprintf(stderr, "%s: http_post: content length field was not set.\n", logpfx());
    setResponse(req, 0, HERR_BAD_REQ, RES_FILE,
                "Client error: Content-Length header missing from request.\r\n");
    return;
  }
    
  if ((v = http_req_to_vlink(req)))
  {
    /*  
     *  For security reasons we only execute programs `attached' to local
     *  links.
     */    
    if ( ! file_is_local(v))
    {
      efprintf(stderr, "%s: http_post: `%s:%s' is not local.\n",
               logpfx(), v->host, v->hsoname);
      setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                  "Server error: form processing program cannot be executed.\r\n");
      return;
    }

    if ( ! (at = vlinkAttr(EXECUTABLE, v)))
    {
      efprintf(stderr, "%s: http_post: no %s attribute on `%s:%s'.\n",
               logpfx(), EXECUTABLE, v->host, v->hsoname);
      setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                  "Server error: form processing program cannot be executed.\r\n");
      return;
    }

    if ( ! (args = tkarray(at->value.sequence)))
    {
      efprintf(stderr, "%s: http_post: can't make argument list from `%s' attribute.\n",
               logpfx(), EXECUTABLE);
      setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                  "Server error: form processing program cannot be executed.\r\n");
      return;
    }

    if ( ! args[0])
    {
      free(args);
      efprintf(stderr, "%s: http_post: no value for `%s' attribute.\n",
               logpfx(), EXECUTABLE);
      setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                  "Server error: form processing program cannot be executed.\r\n");
      return;
    }

    if ( ! (tfp = tmpfile()))
    {
      free(args);
      efprintf(stderr, "%s: http_post: error from tmpfile(): %m.\n",
               logpfx());
      setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                  "Server error: can't create temporary space to process form.\r\n");
      return;
    }

    if ( ! fcopysize(ifp, tfp, req->contlen) || fflush(tfp) == EOF)
    {
      free(args);
      fclose(tfp);
      efprintf(stderr, "%s: http_post: can't copy input to temporary file.\n",
               logpfx());
      setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                  "Server error: not enough space available to process form.\r\n");
      return;
    }

    /*  
     *  This is a hack to set up environment variables required by some Perl
     *  CGI scripts.  We only set the ones they use: CONTENT_LENGTH and
     *  REMOTE_ADDR.  (They could use REMOTE_HOST, but we won't bother...)
     */

    {
      char *s;

      if (memsprintf(&s, "CONTENT_LENGTH=%d", req->contlen) == -1 || putenv(s) != 0 ||
          memsprintf(&s, "REMOTE_ADDR=%s", them.host) == -1 || putenv(s) != 0 ||
          putenv("REQUEST_METHOD=POST") != 0)
      {
        free(args);
        fclose(tfp);
        free(s);
        efprintf(stderr, "%s: http_post: can't set GCI environment variables.\n",
                 logpfx());
        setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                    "Server error: not enough memory available to process form.\r\n");
        return;
      }
    }

    /*  
     *  bug: Okay, here's the story...  At this point stderr is likely to be
     *  attached to file descriptor 0 because we closed all files to become a
     *  daemon, and the freopen() on stderr is the first call to allocate a
     *  new one.  For the exec we want to ensure that stderr is back on
     *  descriptor 2 in case the new program (e.g. tclsh) expects it to be
     *  there.  So, we're going to dup() the current stderr into an unused
     *  descriptor, redirect some files into 0 and 1, then dup2() the
     *  temporary stderr descriptor into 2.
     */

    {
      int tfd;
      
      tfd = dup(fileno(stderr));
      fclose(stderr);
      
      rewind(tfp);
      dup2(fileno(tfp), 0);
      fclose(tfp);
      fflush(ofp);
      dup2(fileno(ofp), 1);

      dup2(tfd, 2);
      close(tfd);
    }

    if (args[1])
    {
      execv(args[0], args);
    }
    else
    {
      execl(args[0], tail(args[0]), (char *)0);
    }
    close(0);
    /*  
     *  We have to close this here so the connection won't stay open, in
     *  `-stay' mode, if the exec fails.
     */
    if (fileno(ofp) != 1) close(1);
    efprintf(stderr, "%s: http_post: exec of `%s' failed: %m.\n", logpfx(), args[0]);
    setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                "Server error: form processing program cannot be executed.\r\n");
    free(args);
  }
}


static VLINK http_req_to_vlink(req)
  Request *req;
{
  VLINK v;
  
  if ( ! (v = url_to_vlink(req->epath)))
  {
    setResponse(req, 0, perrno ? ptoh_errno(perrno) : HERR_INTERNAL, RES_FILE,
                "Server error: can't convert URL `%s' to file name.\r\n", req->epath);
  }

  return v;
}


/*  
 *
 *
 *                             External routines.
 *
 *
 */  


void handle_request(req, ifp, ofp)
  Request *req;
  FILE *ifp;
  FILE *ofp;
{
  switch (req->method)
  {
  case HTTP_GET:
    http_get(req); /* sets response */
    assert(req->res_type != RES_);
    break;

  case HTTP_HEAD:
    http_head(req);
    assert(req->res_type != RES_);
    break;
    
  case HTTP_POST:
    http_post(req, ifp, ofp);
    assert(req->res_type != RES_);
    break;
    
  default:
    setResponse(req, 0, HERR_UNIMP, RES_FILE,
                "The `%s' method is not implemented.\r\n", req->mstr);
    break;
  }
}


void http_get(req)
  Request *req;
{
  VLINK v;
  PATTRIB p;

  if ((v = http_req_to_vlink(req)))
  {
#ifdef BUNYIP_AUTHENTICATION
    if ((p = vlinkAttr(RESTRICTED_ACCESS, v))
        && parseAuthorization(req->auth, p) == 0) 
    {
      setResponse(req, v, HERR_NO_AUTH, RES_AUTH, 0);
      return;
    }
#endif

    switch (real_link_class(v))
    {
    case B_CLASS_DATA:
    case B_CLASS_DOCUMENT:
    case B_CLASS_VIDEO:
      setResponse(req, v, HERR_OK, RES_FILE, 0);
      break;

    case B_CLASS_IMAGE:
      if ( ! req->arg)          /* requesting GIF (i.e. not an image map selection) */
      {
        setResponse(req, v, HERR_OK, RES_FILE, 0);
      }
      else if (vlinkAttrStr(IMAGE_MAP, v)) /* image map selection */
      {
        image_map(v, req);      /* sets response */
        assert(req->res_type != RES_);
      }
      else
      {
        setResponse(req, 0, HERR_BAD_REQ, RES_FILE,
                    "The file does not have an associated image map.\r\n");
      }
      break;
      
    case B_CLASS_MENU:
      /* bug? is this right?! I think so, but do_search() also calls fake_link_class()... */
      if (fake_link_class(v) == B_CLASS_SEARCH)
      {
        do_search(v, req);      /* sets response */
        assert(req->res_type != RES_);
      }
      else
      {
        http_menu(req, v);      /* sets response */
        assert(req->res_type != RES_);
      }
      break;
      
    case B_CLASS_SEARCH:
      do_search(v, req);        /* sets response */
      assert(req->res_type != RES_);
      break;
      
    case B_CLASS_PORTAL:
      efprintf(stderr, "%s: http_get: can't handle portals yet.\n", logpfx());
      setResponse(req, 0, HERR_INTERNAL, RES_FILE, "Can't handle portals, yet.\r\n");
      break;
      
    case B_CLASS_UNKNOWN:
      efprintf(stderr, "%s: http_get: class unknown.\n", logpfx());
      setResponse(req, 0, HERR_INTERNAL, RES_FILE, "Server error: unknown result class.\r\n");
      break;
      
#if 0
    case B_CLASS_SOUND:
    case B_CLASS_VOID:
#endif

    default:
      setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                  "Server error: result class not handled.\r\n");
      efprintf(stderr, "%s: http_get: weird link class `%d'.\n",
               logpfx(), real_link_class(v));
      break;
    }
  }
}


void http_menu(req, v)
  Request *req;
  VLINK v;
{
  VLINK m;

  if ((m = file_in_vdir(v, HTML_MENU_FILE)))
  {
    setResponse(req, m, HERR_OK, RES_FILE, 0);
    return;
  }
  
  if ((m = ppc_m_get_menu(v)))
  {
    setParent(req, v);          /* so we can print a header & get full paths for search links */
    setResponse(req, m, HERR_OK, RES_MENU, 0);
  }
  else
  {
    efprintf(stderr, "%s: http_menu: error getting contents of menu: %s.\n",
             logpfx(), STR(m_error));
    setResponse(req, 0, ptoh_errno(perrno), RES_FILE,
                "Server error: can't get the contents of the menu.\r\n");
    return;
  }
}
