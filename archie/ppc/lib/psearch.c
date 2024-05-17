#ifdef __STDC__
# include <stdlib.h>
#else
# include <memory.h>
#endif
#include <string.h>
#include "defs.h"
#include "error.h"
#include "misc.h"
#include "pattrib.h"
#include "ppc_front_end.h"
#include "protos.h"
#include "psearch.h"
#include "str.h"
#include "all.h"


#define MAXHITS "100"


static TOKEN toksubst proto_((TOKEN tok, PATTRIB pat, const char *srch));
static char *varsubst proto_((char *s, PATTRIB pat, const char *srch));
static const char *def_val proto_((const char *name, PATTRIB pa, const char *srch));
static int find_var_in proto_((const char *s, const char **vs, const char **ve, char **vname));
static void db_name_type proto_((const char *s, char **dbname, enum SearchType *type));
static void vsub proto_((int *n, char **mem, char *s, PATTRIB qa, const char *srch));


/*  
 *  Look for a default value for the variable `name'.  If none is found
 *  return the search string (if the type of `name' is a string), otherwise
 *  return "100".  If no information about the variable can be found, return
 *  the search string.
 */  
static const char *def_val(name, pa, srch)
  const char *name;
  PATTRIB pa;
  const char *srch;
{
  if (strstr(name, "string") ||
      strstr(name, "search-words"))
  {
    return srch;
  }
  
  while (pa)
  {
    TOKEN t = pa->value.sequence;

    if (strcmp(t->token, name) == 0)
    {
      const char *dval;
      const char *type;

      if ((dval = nth_token_str(t, 5)))
      {
        return dval;
      }
      else if ((type = nth_token_str(t, 3)))
      {
        if (strstr(type, "%s") || strstr(type, "%[")) return srch;
        else return MAXHITS;
      }
    }
    pa = pa->next;
  }

  return srch;
}


/*  
 *  Variables look like "${foo}".
 *  
 *  Look for the next variable in the string `s'.  If found, return a copy of
 *  its name in `*vname', and set `*vs' and `*ve' to point to the beginning (`$')
 *  and end (`}'), in `s', of the variable.
 */    
static int find_var_in(s, vs, ve, vname)
  const char *s;
  const char **vs;
  const char **ve;
  char **vname;
{
  const char *p;
  const char *start;
  
  for (p = s; (start = strstr(p, "${"));)
  {
    if (start != s && *(start-1) == '$') /* watch out for quoted `$' (`$$') */
    {
      p = start + 2;
    }
    else
    {
      const char *end = strchr(start+2, '}');
      if ( ! end)
      {
        efprintf(stderr, "%s: find_var: malformed string: `%s'.\n", logpfx(), s);
        return 0;
      }
      *vs = start;
      *ve = end;
      *vname = strndup(start+2, end - start - 2);

      return !!*vname;
    }
  }

  return 0;
}


/*  
 *  Return a copy of `tok', but with all variables substituted (barring
 *  errors).
 */  
static TOKEN toksubst(tok, pat, srch)
  TOKEN tok;
  PATTRIB pat;
  const char *srch;
{
  TOKEN ret = 0;
  
  for (; tok; tok = tok->next)
  {
    char *v;

    if ((v = varsubst(tok->token, pat, srch)))
    {
      ret = tkappend(v, ret);
      free(v);
    }
    else
    {
      efprintf(stderr, "%s: toksubst: varsubst() failed on `%s'.\n", logpfx(), tok->token);
      if (ret) tkfree(ret);
      ret = 0;
      break;
    }
  }

  return ret;
}


static char *varsubst(s, pat, srch)
  char *s;
  PATTRIB pat;
  const char *srch;
{
  PATTRIB qa; /* QUERY-ARGUMENT */
  char *mem;
  int len;

  if ( ! (qa = nextAttr("QUERY-ARGUMENT", pat)))
  {
    efprintf(stderr, "%s: varsubst: can't find `QUERY-ARGUMENT' in attribute list.\n",
             logpfx());
    return 0;
  }

  vsub(&len, 0, s, qa, srch);
  if ((mem = malloc((unsigned)len)))
  {
    *mem = '\0';
    vsub(0, &mem, s, qa, srch);
    return mem;
  }
  else
  {
    efprintf(stderr, "%s: varsubst: can't malloc() %d bytes.\n", logpfx(), len);
    return 0;
  }
}


static struct
{
  const char *pfix;
  enum SearchType type;
  const char *dbname;
  const char *dbfmt;  /* only used if dbname is null */
} dbtab[] =
{
  /* bug: what about MOTD? */

  { "FIND(",      SRCH_WAIS,        0,              "FIND(%*d,%*d,%*d, %[^ )])" },
  { "MATCH(",     SRCH_ANONFTP,     "anonftp",      0                           },
  { "GINDEX(",    SRCH_GOPHER,      "gopher",       0                           },
#if 0
  { "HOST/",      SRCH_SITELIST,    "sitelist",     0                           },
#endif
  { "WHATIS/",    SRCH_WHATIS,      "whatis",       0                           },
  { "DOMAINS",    SRCH_DOMAINS,     "domains",      0                           },
  { 0,            SRCH_UNKNOWN,     "<unknown>",    0                           },
};


static void db_name_type(s, dbname, type)
  const char *s;
  char **dbname;
  enum SearchType *type;
{
  char database_name[128] = { '\0' };
  const char *db;
  enum SearchType searchtype;
  int i;

  dbtab[(sizeof dbtab / sizeof dbtab[0]) - 1].pfix = s; /* sentinel */
  for (i = 0;; i++)
  {
    if (strncmp(s, dbtab[i].pfix, strlen(dbtab[i].pfix)) == 0)
    {
      if (dbtab[i].dbname)
      {
        db = dbtab[i].dbname;
      }
      else
      {
        sscanf(s, dbtab[i].dbfmt, database_name);
        db = database_name;
      }

      searchtype = dbtab[i].type;
      break;
    }
  }

  if (dbname) *dbname = strdup(db);
  if (type) *type = searchtype;
}


static void vsub(n, mem, s, qa, srch)
  int *n;
  char **mem;
  char *s;
  PATTRIB qa;
  const char *srch;
{
  char *vn;
  char *ve;
  char *vs;
  int len;

#define PASS_0(x) do{if(!mem){x;}}while(0)
#define PASS_1(x) do{if(mem){x;}}while(0)

  PASS_0((len = 0));
  while (find_var_in(s, (const char **)&vs, (const char **)&ve, &vn))
  {
    PASS_0((len += vs - s + strlen(def_val(vn, qa, srch))));
    PASS_1((*vs = '\0',
            strcat(*mem, s),
            *vs = '$',
            strcat(*mem, def_val(vn, qa, srch))
            ));
    free(vn);
    s = ve + 1;
  }
  PASS_0((len += strlen(s) + 1, /* for terminating '\0' of final strcat() */
          *n = len
          ));
  PASS_1((strcat(*mem, s)));

#undef PASS_0
#undef PASS_1
}


/*  
 *  Get n'th token, starting at 0.
 */  
const char *nth_token_str(t, n)
  TOKEN t;
  int n;
{
  TOKEN tt;
  int i;
  
  for (i = 0, tt = t; i < n && tt; i++, tt = tt->next)
  { continue; }
  return tt ? tt->token : 0;
}


int display_as_dir(v)
  VLINK v;
{
  const char *at;

  if ( ! (at = vlinkAttrStr(POSE_AS, v)))
  {
    return 1;
  }
  else
  {
    return strcmp(at, "DIRECTORY") == 0;
  }
}


/*  
 *  `v' is the search VLINK (containing all attributes).
 *  
 *  bug: (dbname, stype) should be replaced by a pointer to a post-processing
 *  function which takes, as arguments, the database name and type.
 */      
VLINK search(v, srch, dbname, stype)
  VLINK v;
  const char *srch;
  char **dbname;
  enum SearchType *stype;
{
  PATTRIB qm;
  TOKEN tok;
  char *t;

  if ( ! v)
  {
    efprintf(stderr, "%s: search: VLINK argument is NULL.\n", logpfx());
    return 0;
  }

  if ( ! v->lattrib)
  {
    efprintf(stderr, "%s: search: no attributes found on `%s:%s'.\n",
             logpfx(), v->host, v->hsoname);
    return 0;
  }

  if ( ! (qm = nextAttr("QUERY-METHOD", v->lattrib)))
  {
    efprintf(stderr, "%s: search: no `QUERY-METHOD' attribute on `%s:%s'.\n",
             logpfx(), v->host, v->hsoname);
    return 0;
  }

  if ( ! (tok = qm->value.sequence->next))
  {
    efprintf(stderr, "%s: search: no `QUERY-METHOD' token on `%s:%s'.\n",
             logpfx(), v->host, v->hsoname);
    return 0;
  }

  if ( ! (t = varsubst(tok->token, v->lattrib, srch)))
  {
    return 0;
  }
  else
  {
    TOKEN acomp;
    VDIR rd;
    VDIR_ST rd_;
    VLINK ret = 0;
    int flags = GVD_ATTRIB | GVD_NOSORT;

    rd = &rd_;
    vdir_init(rd);
    acomp = toksubst(tok->next, v->lattrib, srch);
    if (ppc_p_get_dir(v, t, rd, flags, &acomp) != PSUCCESS)
    {
      efprintf(stderr, "%s: search: p_get_dir() of `%s/%s' failed: ",
               logpfx(), t, acomp ? acomp->token : "[null]");
      perrmesg((char *)0, 0, (char *)0);
    }
    else
    {
      db_name_type(t, dbname, stype);
      ret = rd->links;
      vllfree(rd->ulinks);
    }
    free(t);
    tklfree(acomp);

    return ret;
  }
}
