#ifdef __STDC__
# include <stdlib.h>
#endif
#include "aprint.h"
#include "http.h"
#include "ppc.h"
#include "search.h"
#include <assert.h> /* to override Prospero's assert() */
#include "all.h"


static void canned_message proto_((Request *req, VLINK v));
static void search_help_message proto_((Request *req, VLINK v));


/*  
 *
 *
 *                             Internal routines.
 *
 *
 */  


static void canned_message(req, v)
  Request *req;
  VLINK v;
{
  setResponse(req, 0, HERR_OK, RES_FILE,
              "<HEAD><ISINDEX><TITLE>%s Search</TITLE></HEAD>\r\n"
              "<BODY><P>Enter a single search item.</BODY>\r\n",
              v->name);
}


static void search_help_message(req, v)
  Request *req;
  VLINK v;
{
  Request r;
  char *rstr;
  const char *msg_file;
  
  if ( ! (msg_file = vlinkAttrStr(SEARCH_HELP_URL, v)))
  {
    canned_message(req, v); /* sets response */
    assert(req->res_type != RES_);
    return;
  }

  /*  
   *  bug: this stuff can be replaced with forwarding.
   */  
  initRequest(&r, req->here);
  rstr = strsdup("GET /", msg_file, " ", req->version, (char *)0);
  if (parse_method(rstr, &r) == HTTP_GET)
  {
    http_get(&r);               /* sets response */
    assert(r.res_type != RES_);
    if (r.errno == HERR_OK)     /* bug: checking is kinda gross */
    {
      setResponse(req, r.res, r.errno, r.res_type, r.mesg);
    }
    else
    {
      canned_message(req, v);
    }
  }
  else
  {
    efprintf(stderr, "%s: search_help_message: parse of `%s' failed.\n",
             logpfx(), rstr);
    canned_message(req, v);
  }

  free(rstr);
  /* Don't call freeRequest()! */
}


/*  
 *
 *
 *                             External routines.
 *
 *
 */  

/*  
 *  This is called whenever the contents of a link, corresponding to a
 *  search, are required.  The link may refer to a simple search or a fake
 *  search (a DIRECTORY link with the BUNYIP-POSE-AS attribute set to
 *  SEARCH), and a string may be supplied.
 *
 *  - fake search, no string
 *
 *  A canned message, containing <ISINDEX>, is printed.
 *
 *  - fake search, string
 *
 *  The contents of the directory are listed.  The URLs corresponding to
 *  the subfile will have a special character, followed by the search
 *  string, appended to them (see `simple search, string').
 *
 *  - simple search, no string
 *
 *  A canned message, containing <ISINDEX>, is printed.
 *
 *  - simple search, string
 *
 *  A search on the given string is performed, and the results are printed.
 *  The format of the results depends on whether or not the BUNYIP-POSE-AS
 *  attribute is present on the search link.  If it is, the results will be
 *  formatted as menu items (not necessarily selectable), or as a text file,
 *  corresponding to the values of DIRECTORY and FILE, repectively.  If the
 *  attribute does not exist, the results will be formatted in some default
 *  manner.
 */  
  
void do_search(v, req)
  VLINK v;
  Request *req;
{
  /*bug: should check for nul searches or propagations*/
  
  if (fake_link_class(v) == B_CLASS_SEARCH)
  {
    /*  
     *  Fake search.
     */  

    if ( ! req->arg)
    {
      search_help_message(req, v); /* sets response */
      assert(req->res_type != RES_);
    }
    else
    {
      http_menu(req, v); /* sets response */
      assert(req->res_type != RES_);
    }
  }
  else
  {
    /*  
     *  Simple search
     */  

    if ( ! req->arg)
    {
      search_help_message(req, v); /* sets response */
      assert(req->res_type != RES_);
    }
    else
    {
      VLINK results;
    
      setParent(req, v); /* so we can write a header for the results */
      if ((results = search(v, req->arg, 0, &req->search_type)))
      {
        setResponse(req, results, HERR_OK, RES_SEARCH, 0);
      }
      else
      {
        setResponse(req, 0, HERR_NOT_FOUND, RES_FILE,
                    "Nothing was found that matched `%s'.\r\n", req->arg);
      }
    }
  }
}
