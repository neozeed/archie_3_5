#ifdef __STDC__
# include <stdlib.h>
#endif
#include <string.h>
#include "defs.h"
#include "epath.h"
#include "error.h"
#include "misc.h"
#include "my_rd_vlink.h"
#include "ppc_front_end.h"
#include "prosp.h"
#include "protos.h"
#include "str.h"
#include "all.h"


/* bug: Prospero defines its own assert(). */
#ifdef assert
# undef assert
#endif
#include <assert.h>


static VLINK get_abs_vlink proto_((const char *host, const char *type, const char *path));
static VLINK get_rel_vlink proto_((const char *host, const char *type, const char *path));
static VLINK get_simple_vlink proto_((const char *host, const char *type, const char *path));
static char *hd proto_((const char *path));
static const char *tl proto_((const char *path));
static int epath_split proto_((const char *epath, char **host, char **type, char **path));


/*  
 *  hd() and tl() should return results such that:
 *  
 *  `hd(path)/tl(path)' is an equivalent path to `path'.
 */  

/*  
 *  Return the head of a path.  Allocate memory for the result.
 *
 *  hd(/) = /
 *  hd(/a) = /
 *  hd(/a/b) = /a
 *  hd(foo) =          <empty>
 *  etc.
 *
 *  If no `/' then always return `.'.
 */  
static char *hd(path)
  const char *path;
{
  const char *sl;

  sl = strrchr(path, '/');
  return sl == path ? strdup("/") : strndup(path, sl - path);
}


/*  
 *  Return the tail of a path.
 *
 *  tl(/) = .
 *  tl(/a) = a
 *  tl(/a/b) = b
 *  tl(foo) = foo
 *  etc.
 *
 */  
static const char *tl(path)
  const char *path;
{
  const char *sl;

  if ( ! (sl = strrchr(path, '/')))
  {
    return path;
  }
  else
  {
    return strcmp(path, "/") == 0 ? "." : strrchr(path, '/') + 1;
  }
}


/*  
 *  Return the VLINK corresponding to the arguments, which represent
 *  an absolute path (i.e. assume path begins with `/').
 *
 *  The path is assumed to be the hsoname of the VLINK.
 *
 *  Special case: if the path begins with ARCHIE use the path for the
 *                hsoname.
 *
 *  The result should be vlfree()ed.
 */  
static VLINK get_abs_vlink(host, type, path)
  const char *host;
  const char *type;
  const char *path;
{
  VDIR d;
  VDIR_ST d_;
  VLINK v;
  char h[MAXHOSTNAMELEN+1];
  char *phead;
  const char *hsot = "ASCII";
  const char *ht = "INTERNET-D";

  if (*type)
  {
    efprintf(stderr, "%s: get_abs_vlink: can't do non-empty types.\n", logpfx());
    return 0;
  }

  if ( ! (v = vlalloc()))
  {
    efprintf(stderr, "%s: get_abs_vlink: vlalloc() failed.\n", logpfx());
    return 0;
  }

  if ( ! *host)
  {
    gethostname(h, sizeof h);
    host = h;
  }

  d = &d_;
  vdir_init(d);
    
  /*  
   *  bug: changed to fix '(null)' name when printing menus (esp. when
   *  selecting a directory from a returned archie search).
   */  
  v->name = stcopy(tl(path));
  v->host = stcopy(host);
  v->hosttype = stcopy(ht);
  v->hsoname = stcopy(phead = hd(path)); free(phead);
  v->hsonametype = stcopy(hsot);

  if (ppc_p_get_dir(v, tl(path), d, GVD_ATTRIB|GVD_EXPAND|GVD_NOSORT, 0)
      != PSUCCESS)
  {
    efprintf(stderr, "%s: get_abs_vlink: p_get_dir() of child failed on `%s:%s/%s': ",
             logpfx(), v->host, v->hsoname, tl(path));
    perrmesg((char *)0, 0, (char *)0);
    return 0;
  }

  vllfree(d->ulinks);
  if ( ! d->links)
  {
    efprintf(stderr, "%s: get_abs_vlink: no links returned for `%s:%s/%s':",
             logpfx(), v->host, v->hsoname, tl(path));
    perrmesg((char *)0, 0, (char *)0);
  }
  else if (d->links->next)
  {
    efprintf(stderr, "%s: get_abs_vlink: multiple links match `%s:%s/%s'.\n",
             logpfx(), v->host, v->hsoname, tl(path));
    vllfree(d->links->next);
    d->links->next = 0;
  }

  vlfree(v);
  return d->links;
}


/*  
 *  Return the VLINK corresponding to the arguments, which represent
 *  a relative path (i.e. assume path doesn't begin with `/').
 *
 *  The result should be vlfree()ed.
 *
 *  bug: rd_vlink() doesn't seem to be able to do anything with
 *       host:path (barfs on the host).
 */  
static VLINK get_rel_vlink(host, type, path)
  const char *host;
  const char *type;
  const char *path;
{
  VLINK ret;
  char *fullpath;

  if (*type)
  {
    efprintf(stderr, "%s: get_rel_vlink: can't do non-empty type.\n", logpfx());
    return 0;
  }

  if ( ! (fullpath = strsdup(host, *host ? ":" : "", path, (char *)0)))
  {
    efprintf(stderr, "%s: get_rel_vlink: strsdup() failed trying to create full path.\n",
             logpfx());
    return 0;
  }

  if ( ! (ret = ppc_my_rd_vlink(fullpath)))
  {
    efprintf(stderr, "%s: get_rel_vlink: my_rd_vlink() failed on `%s': ",
             logpfx(), fullpath);
    perrmesg((char *)0, 0, (char *)0);
  }
  free(fullpath);

  return ret;
}


static VLINK get_simple_vlink(host, type, path)
  const char *host;
  const char *type;
  const char *path;
{
  VLINK v;
  char h[MAXHOSTNAMELEN+1];
  const char *hsot = "ASCII";
  const char *ht = "INTERNET-D";

  if (*type)
  {
    efprintf(stderr, "%s: get_simple_vlink: can't do non-empty type.\n", logpfx());
    return 0;
  }

  if ( ! *host)
  {
    gethostname(h, sizeof h);
    host = h;
  }

  if ( ! (v = vlalloc()))
  {
    efprintf(stderr, "%s: get_simple_vlink: vlalloc() failed.\n", logpfx());
    return 0;
  }

  /*  
   *  bug: changed to fix '(null)' name when printing menus (esp. when
   *  selecting a directory from a returned archie search).
   */  
  /* bug: check return values */
  v->name = stcopy(tl(path));
  v->host = stcopy(host);
  v->hosttype = stcopy(ht);
  v->hsoname = stcopy(path);
  v->hsonametype = stcopy(hsot);

  return v;
}


/*  
 *  Split a URL up into its host, type and path components.
 *
 *  Note: if a URL has a leading `/' it is assumed to be an absolute path
 *        from the root of the given host's Prospero tree, otherwise it is
 *        relative to the server's current directory.
 *
 *  Note: it seems as though clients will always put a leading `/' in the
 *        URL, so if we find a `:' we should trash the leading `/'.
 *
 *  bug: error checking on str functions.
 */  
static int epath_split(epath, host, type, path)
  const char *epath;
  char **host;
  char **type;
  char **path;
{
  char *h;
  char *p;
  char *t;
  const char *col1;
  const char *col2;
  const char *start;

  start = epath;
  if ( ! (col1 = strchr(start, ':')))
  {
    /*  
     *  No embedded host name and types.
     */  
    h = strdup("");
    t = strdup("");
  }
  else
  {
    start = *epath == '/' ? epath+1 : epath; /* ignore the leading `/' */
    h = strndup(start, col1 - start);
    if ((col2 = strchr(col1+1, ':')))
    {
      t = strndup(col1+1, col2 - (col1+1));
    }
    else
    {
      efprintf(stderr, "%s: epath_split: no second `:' in `%s'.\n",
               logpfx(), epath);
      free(h);
      return 0;
    }
    start = col2 + 1;
  }

  /*  
   *  If the path is empty or just a `/' we convert it to `.', which is the
   *  server's root.
   *  
   *  At the moment, it's not obvious that, in normal operation, a path could
   *  contain only a single `/'.  But I don't think this will hurt.
   */  
  if ( ! *start || strcmp("/", start) == 0)
  {
    p = strdup(".");
  }
  else
  {
    int last;

    /*  
     *  It seems that Mosaic will always put a leading `/' on its URLs, so
     *  we will strip it to get a relative path.  To allow the absolute paths
     *  we can prepend an extra `/', giveng a leading `//'.
     *
     *  If there is no leading `/', it could be someone type directly to us
     *  (e.g. telnetting to our port), in which case we interpret this as a
     *  relative path, too.
     *
     *  - wheelan (Thu Dec  2 00:37:28 EST 1993)
     */  
    p = strdup(*start == '/' ? start+1 : start);
    last = strlen(p) - 1;
    if (p[last] == '/' && last != 0)
    {
      p[last] = '\0'; /* knock off any trailing `/'. */
    }
  }

  *host = h;
  *type = t;
  *path = p;
  return 1;
}


char *vlink_to_epath(v)
  VLINK v;
{
  char *ret;
  char *t;
  
  if ( ! v)
  {
    efprintf(stderr, "%s: vlink_to_epath: VLINK argument is NULL.\n", logpfx());
    return 0;
  }

  if (strcmp(v->hosttype, "INTERNET-D") == 0 &&
      strcmp(v->hsonametype, "ASCII") == 0)
  {
    t = strdup("");
  }
  else
  {
    /* bug: a type like this won't be recognized at this time */
    t = strsdup(v->hosttype, "/", v->hsonametype, (char *)0);
  }

  /*  
   *  Put an extra / in front of the path so we can blindly
   *  strip it off, when converting URLs to VLINKS, and still distinguish
   *  relative paths from absolute paths by whether or not there is a
   *  leading /.
   *
   *  This should work because _we_ always construct absolute paths, but
   *  we may receive relative paths from references in HTML documents.
   *
   *  -wheelan (Wed Dec  1 23:59:46 EST 1993)
   */  
  ret = strsdup(v->host, ":", t, ":/", v->hsoname, (char *)0);
  free(t);

  return ret;
}


/*  
 *  A URL is a string having one of these formats (enclosed in `'):
 *  
 *    `' equivalent to `/'.
 *    `/[<p-host>:[<type-encoding>]:][<path>]'.
 *  
 *  where the optional <p-host> is a Prospero host name, and the optional
 *  <type-encoding> may specify information about the corresponding Prospero
 *  link.
 *  
 *  If <p-host> is not given, the host is assumed to be the local host (i.e.
 *  the host constructing the VLINK), while a non-existant <type-encoding>
 *  specifies a hosttype of `INTERNET-D' and a hsonametype of `ASCII'.
 *
 *  bug: what about searches, etc.?
 */  
  
/*  
 *  The result should be vlfree()ed.
 */  
VLINK epath_to_vlink(epath)
  const char *epath;
{
  VLINK ret = 0;
  char *host;
  char *type;
  char *path;
  
  if ( ! epath_split(epath, &host, &type, &path))
  {
    efprintf(stderr, "%s: epath_to_vlink: epath_split() failed.\n", logpfx());
    return 0;
  }

  if (*type)
  {
    efprintf(stderr, "%s: epath_to_vlink: non-empty type encodings in `%s' are unsupported.\n",
             logpfx(), epath);
  }
  else
  {
    if (*path == '/')
    {
      ret = get_abs_vlink(host, type, path);
    }
    else if (STRNCMP(path, "ARCHIE") == 0)
    {
      ret = get_simple_vlink(host, type, path);
    }
    else
    {
      ret = get_rel_vlink(host, type, path);
    }
  }

  free(host);
  free(type);
  free(path);

  if ( ! ret)
  {
    efprintf(stderr, "%s: epath_to_vlink: can't get URL `%s'.\n", logpfx(), epath);
  }

  return ret;
}
