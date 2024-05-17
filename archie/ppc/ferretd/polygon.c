#include <math.h>
#include <stdio.h>
#include <string.h>
#include "polygon.h"
#include "protos.h"
#include "all.h"


#define EPSILON 1e-5
#define ODD(n) ((n) % 2 == 1)


static const char *skip_point proto_((const char *p));
static int intersect proto_((double p1x, double p1y, double p2x, double p2y,
                             double p3x, double p3y, double p4x, double p4y));


/*  
 *  Assume `p' points to a string matching the scanf() pattern:
 *  
 *    "%f,%f"
 */  
static const char *skip_point(p)
  const char *p;
{
  p = strchr(p, ',') + 1;
  p += strspn(p, " \t");
  p += strspn(p, ".0123456789");
  return p;
}


static int intersect(p1x, p1y, p2x, p2y, p3x, p3y, p4x, p4y)
  double p1x, p1y;
  double p2x, p2y;
  double p3x, p3y;
  double p4x, p4y;
{
  double ax, ay, bx, by, cx, cy;
  double alpha, beta, denom;

  /* test segment [p1, p2] with [p3, p4] */
  ax = p2x - p1x; ay = p2y - p1y;
  bx = p3x - p4x; by = p3y - p4y;
  cx = p1x - p3x; cy = p1y - p3y;

  denom = ay * bx - ax * by;

  if (fabs(denom) >= EPSILON)
  {
    alpha = (by * cx - bx * cy) / denom;
    if (alpha >= 0.0 && alpha <= 1.0)
    {
      beta = (ax * cy - ay * cx) / denom;
      if (beta >= 0.0 && beta <= 1.0)
      {
        return 1;
      }
    }
  }

  return 0;
}


/*  
 *  Test whether or not the point (x,y) is inside the polygon whose vertices
 *  are given by points.  (farx,fary) is a point assumed to be outside any
 *  polygon.
 *  
 *  Let O be a point known to be outside the polygon and P be the point to be
 *  tested for inclusion.
 *  
 *  P is inside the polygon if and only if the line segment between P and O
 *  intersects an odd number of the polygon's edges.
 *  
 *  Note: this function may return an incorrect result if the line segment
 *  intersects a vertex of the polygon.  Hopefully, (farx,fary) can be chosen
 *  so that this will never happen (or be very rare for most polygons).
 *  [bug: I really ought to add a test for those cases.]
 *  
 *  The line segment intersection test is based on the paper by Franklin
 *  Antonio in Graphics Gems III, but without all the speedups.
 */                

int in_polygon(x, y, farx, fary, points)
  double x, y;
  double farx, fary;
  const char *points;
{
  const char *p = points;
  double firstx, firsty;
  double rx, ry, sx, sy;
  int n;
  int nedges = 0;
  int ninter = 0;

  if (sscanf((char *)p, "%lf,%lf", &firstx, &firsty) != 2)
  {
    efprintf(stderr, "%s: in_polygon: bad points `%s'.\n", logpfx(), points);
    return 0;
  }

  p = skip_point(p);
  
  rx = firstx; ry = firsty;
  while ((n = sscanf((char *)p, "%lf,%lf", &sx, &sy)) == 2)
  {
    if (intersect(x, y, farx, fary, rx, ry, sx, sy))
    {
      ninter++;
    }
    nedges++;
    rx = sx; ry = sy;
    p = skip_point(p);
  }

  if (n != EOF)
  {
    efprintf(stderr, "%s: in_polygon: bad points `%s'.\n", logpfx(), points);
    return 0;
  }
  
  /* Final edge starts with last point, ends with first */
  if (intersect(x, y, farx, fary, rx, ry, firstx, firsty))
  {
    ninter++;
  }
  nedges++;

  if (nedges < 3)
  {
    efprintf(stderr, "%s: in_polygon: not enough vertices in `%s'.\n",
             logpfx(), points);
    return 0;
  }

  return ODD(ninter);
}
