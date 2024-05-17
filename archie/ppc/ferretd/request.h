#ifndef REQUEST_H
#define REQUEST_H

#include <stdarg.h>
#include "defs.h"


/*  
 *  Encodings.
 */  

#define TXT_HTML  -10
#define TXT_PLAIN -11


/*  
 *  Methods.
 */  

#define HTTP_FAIL        -1000
#define HTTP_UNSUPPORTED -1001

#define HTTP_CHECKIN     HTTP_UNSUPPORTED
#define HTTP_CHECKOUT    HTTP_UNSUPPORTED
#define HTTP_GET         -1004
#define HTTP_HEAD        -1005
#define HTTP_POST        -1006
#define HTTP_PUT         HTTP_UNSUPPORTED
#define HTTP_REPLY       HTTP_UNSUPPORTED
#define HTTP_SHOWMETHOD  HTTP_UNSUPPORTED
#define HTTP_SPACEJUMP   HTTP_UNSUPPORTED
#define HTTP_TEXTSEARCH  HTTP_UNSUPPORTED

/*  
 *  Errors
 */  

enum Status
{
  HERR_          = -100,        /* initial value only */
  HERR_BAD_REQ   =  400,
  HERR_CREATED   =  201,
  HERR_FORBIDDEN =  403,
  HERR_FOUND     =  302,
  HERR_INTERNAL  =  500,
  HERR_METHOD    =  303,
  HERR_MOVED     =  301,
  HERR_NOT_FOUND =  404,
  HERR_NO_AUTH   =  401,
  HERR_OK        =  200,
  HERR_PAY_REQ   =  402,
  HERR_UNIMP     =  501
};


/*  
 *  Responses
 */  

enum Response
{
  RES_,                         /* initial, unset value */
  RES_FILE,                     /* print message (if any), then links (if any) */
  RES_HEAD,                     /* return a MIME header only */
  RES_MENU,
  RES_REDIRECT,                 /* direct the client to another URL */
  RES_SEARCH,
  RES_AUTH                      /* requires authorization */
};


/*  
 *  Misc.
 */  

#define HTTP_VERSION_STR "HTTP/1.0"

#define PROP_CHAR  '&'
#define PROP_STR   "&"
#define ARG_CHAR   '?'
#define ARG_STR    "?"
#define TERM_CHARS "&?"


typedef struct request_
{
  /* Request part */
  char *mstr;
  char *arg;
  char *epath;
  char *version;
  int method;
  int contlen;                  /* Value of Content-Length header, if it exists */

  /* Response part */
  VLINK parent;
  VLINK res;
  char *mesg;                   /* instead of links; res_type file */
  enum SearchType search_type;  /* e.g. SRCH_WAIS, SRCH_GOPHER, etc. */
  enum Status errno;            /* HERR_* */
  enum Response res_type;       /* e.g. menu, file, result from search */

  /* Misc. */
  Where here;
#ifdef BUNYIP_AUTHENTICATION
  char  *auth;			/* authentification string */
#endif
} Request;


extern int addToRequest proto_((FILE *ifp, Request *r, int *badterm));
extern int parse_method proto_((const char *line, Request *req));
extern int ptoh_errno proto_((int n));
extern void freeRequest proto_((Request *r));
extern void initRequest proto_((Request *r, Where us));
extern void setParent proto_((Request *req, VLINK parent));
extern void setResponse(Request *req, VLINK res, enum Status err, enum Response rtype, const char *mesg, ...);

#endif
