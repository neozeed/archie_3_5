#include <math.h>
#include <stdlib.h>
#include "http.h"
#include "image_map.h"
#include "polygon.h"
#include "ppc.h"
#include "protos.h"
#include <assert.h> /* to override Prospero's assert() */
#include "all.h"


#define FARX 100.0
#define FARY 100.0


static int in_circle proto_((double x, double y, const char *points));
static int point_in_region proto_((int imrows, int imcols, int x, int y, const char *s));
static void get_doc proto_((Request *req, const char *s));


static int in_circle(x, y, points)
  double x;
  double y;
  const char *points;
{
  double a, b;
  double radius, xcentre, ycentre;

  if (sscanf((char *)points, "%lf,%lf %lf", &xcentre, &ycentre, &radius) != 3)
  {
    efprintf(stderr, "%s: in_circle: `%s' not in `%%f,%%f %%f' format.\n",
             logpfx(), points);
    return 0;
  }

  a = x - xcentre; b = y - ycentre;
  return sqrt(a*a + b*b) <= radius;
}


static int point_in_region(imrows, imcols, x, y, s)
  int imrows;
  int imcols;
  int x;
  int y;
  const char *s;
{
  char type[64];
  char points[512];             /*bug: the height of hypocrisy, but... */
  double xpt;
  double ypt;
  
  /*  
   *  Scale (x,y) to be in [0.0, 1.0].
   */

  xpt = x / (double)imcols;
  ypt = y / (double)imrows;
  
  /*  
   *  `s' is something like:
   *  
   *    foo.html POLYGON 0.0,0.0 0.0,0.0 ....
   */  

  switch (sscanf((char *)s, "%*s %s %[^\n]", type, points))
  {
  case 2:
    if (strcasecmp(type, "POLYGON") == 0)
    {
      return in_polygon(xpt, ypt, 100.0, 100.0, points);
    }
    else if (strcasecmp(type, "CIRCLE") == 0)
    {
      return in_circle(xpt, ypt, points);
    }
    else
    {
      efprintf(stderr, "%s: point_in_region: unknown type `%s' in `%s'.\n",
               logpfx(), type, s);
      return 0;
    }
    break;

  case 1:
    if (strcasecmp(type, "OTHERWISE") == 0)
    {
      return 1;
    }
    /* else, fall through... */

  default:
    efprintf(stderr, "%s: point_in_region: malformed region specifier `%s'.\n",
             logpfx(), s);
    return 0;
  }
}


static void get_doc(req, s)
  Request *req;
  const char *s;
{
  char *doc, *url;

  if ( ! (doc = worddup(s, 0)))
  {
    efprintf(stderr, "%s: get_doc: can't get document name from `%s'.\n",
             logpfx(), s);
    setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                "Server error: can't retrieve requested document.\r\n");
    return;
  }
  
  /*  
   *  Convert to a fully qualified URL, if necessary.
   */

  if (strncasecmp(doc, "http://", 7) == 0) {
    url = strdup(doc);
  } else {
    memsprintf(&url, "http://%s:%s%s%s", us.host, us.port, *doc == '/' ? "" : "/", doc);
  }
  
  setResponse(req, 0, HERR_FOUND, RES_REDIRECT, url);

  free(doc);
  free(url);
}


/*  
 *
 *                             External routines.
 *
 */  

/*  
 *  req->arg should be of the form `%d,%d' (`%f,%f' possible?).
 *
 */  
void image_map(v, req)
  VLINK v;
  Request *req;
{
  PATTRIB at;
  TOKEN t;
  int cols;
  int rows;
  int x;
  int y;

  /*  
   *  Attribute values:
   *  
   *  BUNYIP-IMAGE-MAP: 480x360 IMAGE
   *                     a.html POLYGON 0.0,0.0 0.0,0.0 0.0,0.0 ...
   *                     b.html CIRCLE  0.0,0.0 0.0
   *                        .
   *                        .
   *                        .
   *                     x.html OTHERWISE
   */  

  if (sscanf(req->arg, "%d,%d", &x, &y) != 2)
  {
    efprintf(stderr, "%s: image_map: bogus mouse position `%s'.\n",
             logpfx(), req->arg);
    setResponse(req, 0, HERR_BAD_REQ, RES_FILE,
                "Client error?  (Received bogus mouse position, `%s'.)\r\n", req->arg);
    return;
  }
  
  if ( ! (at = vlinkAttr(IMAGE_MAP, v)))
  {
    efprintf(stderr, "%s: image_map: can't get image map attribute for `%s:%s'.\n",
             logpfx(), v->host, v->hsoname);
    setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                "Server error: can't find the regions of the image map.\r\n");
    return;
  }

  /* Get dimensions. */

  t = appendAttrTokens(at, IMAGE_MAP);
  if (sscanf(t->token, "%dx%d IMAGE", &cols, &rows) != 2)
  {
    efprintf(stderr, "%s: image_map: image dimensions not first in `%s'.\n",
             logpfx(), t->token);
    setResponse(req, 0, HERR_INTERNAL, RES_FILE,
                "Server error: can't get image map information.\r\n");
    return;
  }
  
  /* Find the region containing the point. */

  t = t->next;
  while (t)
  {
    if (point_in_region(rows, cols, x, y, t->token))
    {
      get_doc(req, t->token); /* sets response */
      assert(req->res_type != RES_);
      return;
    }
    t = t->next;
  }
  
  setResponse(req, 0, HERR_INTERNAL, RES_FILE,
              "Server error: no document for the selected area.\r\n");
}
