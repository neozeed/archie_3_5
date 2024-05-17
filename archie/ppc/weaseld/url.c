/*  
 *  bug: currently these routines done't actually do URLs according to the
 *  spec.  They only encode and decode (`%' quoting) what they are passed.
 *
 *  Also, the function prototypes are bound to change when full URLs are
 *  supported.
 */  
#ifdef __STDC__
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include "ppc.h"
#include "url.h"
#include "all.h"


/*  
 *  Ones signify `safe' characters; that is characters that needn't be
 *  quoted.
 */  
static const char goph_safe[256] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*   0 -  15 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  16 -  31 */
  0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, /*  32 -  47 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, /*  48 -  63 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  64 -  79 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /*  80 -  95 */
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  96 - 111 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 112 - 127 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 128 - 143 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 144 - 159 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 160 - 175 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 176 - 191 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 192 - 207 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 208 - 223 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 224 - 239 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* 240 - 255 */
};


char *url_dequote(s, allocfn)
  const char *s;
  void *(*allocfn) proto_((size_t));
{
  return dequote_string(s, goph_safe, allocfn);
}


char *url_quote(s, allocfn)
  const char *s;
  void *(*allocfn) proto_((size_t));
{
  return quote_string(s, goph_safe, allocfn);
}


char *vlink_to_url(v)
  VLINK v;
{
  char *e;
  char *ret = 0;

  if ( ! (e = vlink_to_epath(v)))
  {
    efprintf(stderr, "%s: vlink_to_url: vlink_to_epath() failed.\n", logpfx());
  }
  else
  {
    ret = quote_string(e, goph_safe, malloc);
    free(e);
  }

  return ret;
}


/*  
 *  The result should be vlfree()ed.
 */  
VLINK url_to_vlink(url)
  const char *url;
{
  VLINK ret = 0;
  char *e;
  
  if ( ! (e = dequote_string(url, goph_safe, malloc)))
  {
    efprintf(stderr, "%s: url_to_vlink: dequote_string() failed.\n", logpfx());
  }
  else
  {
    ret = epath_to_vlink(e);
    free(e);
  }

  return ret;
}
