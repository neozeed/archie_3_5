/*  
 *  ToDo:
 *  
 *  Look for default.html as replacement for directory listing.  DONE
 *
 *  In ../lib: do locking on the log file & accounting file when it is
 *             implemented + make sure all writes are appends (NOTE that
 *             stdio append mode under SunOS is probably broken)
 *
 *  In ../lib: don't wait for child; catch SIGCHILD  DONE
 *
 *  Canonicalize URLs; -> in file names causes problems
 *
 *  asks for SIZE twice when getting Abacus (text + little GIF)
 *
 *  Make fake searches work.  Just a problem of setting the correct  DONE
 *  attributes on the link?
 *  
 *  "file access controls", i.e. gopher, !www, etc.
 *  
 *  Rework program structure so that everything returns VLINK & they
 *  get processed only in one location.  This would make writing the
 *  HTTP/1.0 head shit easier.  e.g. struct { VLINK res; int disposition }.
 *
 *  Look into freeing attribute lists & clean up get_at, find_at, etc.
 *  (cleanup DONE, but not freeing)
 */  

#include <stdio.h>
#include <unistd.h>
#include "file_type.h"
#include "http.h"
#include "ppc.h"
#include "print_results.h"
#include <assert.h> /* to override Prospero's assert() */
#include "all.h"

#include "protos.h"



/*  
 *
 *
 *                             External routines.
 *
 *
 */  


const char *prosp_ident()
{
  return "ggBf";
}


void access_denied(ifp, ofp, us)
  FILE *ifp;
  FILE *ofp;
  const Where us;
{
  Request r;

  setResponse(&r, 0, HERR_FORBIDDEN, RES_FILE,
#ifdef UNAUTH_MESG
           UNAUTH_MESG
#else
           "Sorry, you are not authorized to use this service.\r\n"
#endif
           );
  print_results(ofp, &r);
  freeRequest(&r);
}


const char *service_str()
{
  return "www";
}


void handle_transaction(ifp, ofp, us, them)
  FILE *ifp;
  FILE *ofp;
  const Where us;
  const Where them;
{
  Request r;
  char *line;

  initRequest(&r, us);
  
  if ( ! (line = timed_readline(ifp, debug)))
  {
    setResponse(&r, 0, HERR_BAD_REQ, RES_FILE,
               "The request was empty or contained a malformed line.\r\n");
  }
  else
  {
    if ((r.method = parse_method(line, &r)) == HTTP_UNSUPPORTED)
    {
      setResponse(&r, 0, HERR_UNIMP, RES_FILE,
                 "This server does not implement the `%s' method.\r\n", r.mstr);
    }
    else if (r.method == HTTP_FAIL)
    {
      setResponse(&r, 0, HERR_BAD_REQ, RES_FILE,
                 "Badly formatted request.\r\n");
    }
    else
    {
      int error = 0;
      
      /* Three arguments means RFC822 headers may follow. */
      /* bug: kludge: not very clean. */
#if 1
      if (r.version)
#else
      if (r.version && r.method != HTTP_POST)
#endif
      {
        while (addToRequest(ifp, &r, &error))
        { continue; }
      }
    
      if (error)
      {
        efprintf(stderr, "%s: handle_transaction: incorrectly terminated line.\n",
                 logpfx());
        setResponse(&r, 0, HERR_BAD_REQ, RES_FILE,
                   "The request contained a malformed line.\r\n");
      }
      else
      {
        const char *redir;
        
        efprintf(stderr, "%s: handle_transaction: request from `%s' is `%s %s'.\n",
                 logpfx(), them.host, r.mstr, r.epath);
        /* Check for a redirection. */
        if ( ! (redir = redirection(r.epath)))
        {
          handle_request(&r, ifp, ofp);
        }
        else
        {
          setResponse(&r, 0, HERR_FOUND, RES_REDIRECT, redir);
        }
      }
    }
  }

  assert(r.res_type != RES_);
  print_results(ofp, &r);
  clear_acceptance();
  freeRequest(&r);

  fflush(ofp);
  fclose(ifp); fclose(ofp);
}
