#include "prsp_vlink.h"


#define TCL_CHECK(interp, exp, failstr, failret) \
do{if(!(exp)){Tcl_SetResult(interp,failstr,TCL_STATIC);return failret;}}while(0)


typedef int VlinkFn(Tcl_Interp *interp, VLINK v, int ac, char **av);


static int vlinkSetAcls(Tcl_Interp *interp, VLINK v, int ac, char **av);
static int vlinkSetAttributes(Tcl_Interp *interp, VLINK v, int ac, char **av);
static int vlinkSetHost(Tcl_Interp *interp, VLINK v, int ac, char **av);
static int vlinkSetHosttype(Tcl_Interp *interp, VLINK v, int ac, char **av);
static int vlinkSetHsoname(Tcl_Interp *interp, VLINK v, int ac, char **av);
static int vlinkSetHsonametype(Tcl_Interp *interp, VLINK v, int ac, char **av);
static int vlinkSetLinktype(Tcl_Interp *interp, VLINK v, int ac, char **av);
static int vlinkSetName(Tcl_Interp *interp, VLINK v, int ac, char **av);
static int vlinkSetTarget(Tcl_Interp *interp, VLINK v, int ac, char **av);


static int vlinkSetName(Tcl_Interp *interp, VLINK v, int ac, char **av)
{
  TCL_CHECK(interp, ac == 1, "one value is required", 0);
  TCL_CHECK(interp, v->name = stcopyr(av[0], v->name), "stcopyr() failed", 0);
  return 1;
}

static int vlinkSetHost(Tcl_Interp *interp, VLINK v, int ac, char **av)
{
  TCL_CHECK(interp, ac == 1, "one value is required", 0);
  TCL_CHECK(interp, v->host = stcopyr(av[0], v->host), "stcopyr() failed", 0);
  return 1;
}


static int vlinkSetHosttype(Tcl_Interp *interp, VLINK v, int ac, char **av)
{
  TCL_CHECK(interp, ac == 1, "one value is required", 0);
  TCL_CHECK(interp, v->hosttype = stcopyr(av[0], v->hosttype), "stcopyr() failed", 0);
  return 1;
}


static int vlinkSetHsoname(Tcl_Interp *interp, VLINK v, int ac, char **av)
{
  TCL_CHECK(interp, ac == 1, "one value is required", 0);
  TCL_CHECK(interp, v->hsoname = stcopyr(av[0], v->hsoname), "stcopyr() failed", 0);
  return 1;
}


static int vlinkSetHsonametype(Tcl_Interp *interp, VLINK v, int ac, char **av)
{
  TCL_CHECK(interp, ac == 1, "one value is required", 0);
  TCL_CHECK(interp, v->hsonametype = stcopyr(av[0], v->hsonametype), "stcopyr() failed", 0);
  return 1;
}


static int vlinkSetLinktype(Tcl_Interp *interp, VLINK v, int ac, char **av)
{
  TCL_CHECK(interp, ac == 1, "one value is required", 0);
  TCL_CHECK(interp, v->linktype = av[0][0], "value is nul!", 0);
  return 1;
}


static int vlinkSetTarget(Tcl_Interp *interp, VLINK v, int ac, char **av)
{
  TCL_CHECK(interp, ac == 1, "one value is required", 0);
  TCL_CHECK(interp, v->target = stcopyr(av[0], v->target), "stcopyr() failed", 0);
  return 1;
}


static int vlinkSetAttributes(Tcl_Interp *interp, VLINK v, int ac, char **av)
{
  TCL_CHECK(interp, ac >= 0, "bad attribute list", 0);
  TCL_CHECK(interp, v->lattrib = atListFromTclArray(interp, ac, av), "", 0);
  return 1;
}


static int vlinkSetAcls(Tcl_Interp *interp, VLINK v, int ac, char **av)
{
  return 1;
}


/*  
 *  A Tcl attribute looks like `{name [value] ...}'.  Convert this to the
 *  corresponding PATTRIB structure.
 */  
PATTRIB atFromTclList(Tcl_Interp *interp, const char *atstr)
{
  PATTRIB pat = 0;
  char **alist;
  int i;
  int n;
  
  if (Tcl_SplitList(interp, atstr, &n, &alist) != TCL_OK)
  {
    return 0;
  }

  if (n < 1)
  {
    free(alist);
    Tcl_SetResult(interp, "bad # of attributes", TCL_STATIC);
    return 0;
  }
  
  if ( ! (pat = atalloc()))
  {
    free(alist);
    Tcl_SetResult(interp, "can't allocate memory for attribute", TCL_STATIC);
    return 0;
  }

  pat->aname = stcopyr(alist[0], pat->aname);
  pat->avtype = ATR_SEQUENCE;
  pat->nature = ATR_NATURE_APPLICATION;
  for (i = 1; i < n; i++)
  {
    /* It's not clear that error checking would help us... */
    pat->value.sequence = tkappend(alist[i], pat->value.sequence);
  }

  free(alist);
  return pat;
}


PATTRIB atListFromTclArray(Tcl_Interp *interp, int ac, char **av)
{
  PATTRIB ats = 0;
  int i;
  
  if (ac < 0)
  {
    return 0;
  }

  for (i = 0; i < ac; i++)
  {
    PATTRIB a;
    
    if ( ! (a = atFromTclList(interp, av[i])))
    {
      if (ats) atlfree(ats);
      return 0;
    }
    APPEND_ITEM(a, ats);
  }

  return ats;
}


Tcl_HashTable vlinkFnHT;
static struct
{
  const char *field;
  VlinkFn *fn;
} vlinkFnTab[] =
{
  { "NAME",        vlinkSetName },
  { "HOST",        vlinkSetHost },
  { "HOSTTYPE",    vlinkSetHosttype },
  { "HSONAME",     vlinkSetHsoname },
  { "HSONAMETYPE", vlinkSetHsonametype },
  { "LINKTYPE",    vlinkSetLinktype },
  { "TARGET",      vlinkSetTarget },
  { "ATTRIBUTES",  vlinkSetAttributes },
  { "ACLS",        vlinkSetAcls },

  { 0, 0 }
};


/* bug: we should allow for ACLs, etc. */

/*  
 *  Create a VLINK from its Tcl string representation.
 *
 *  The Tcl representation is a `keyed list':
 *
 *  {NAME name} {HOST host} ... {ATTRIBUTES {...} ...} {ACLS {...} ...}
 */  
VLINK vlinkFromTclList(Tcl_Interp *interp, const char *vstr)
{
  VLINK v;
  char **vlist;
  int i;
  int n;
  
  if (Tcl_SplitList(interp, (char *)vstr, &n, &vlist) != TCL_OK)
  {
    return 0;
  }

  if ( ! (v = vlalloc()))
  {
    Tcl_SetResult(interp, "can't allocate memory for vlink", TCL_STATIC);
    free(vlist);
    return 0;
  }

  for (i = 0; i < n; i++)
  {
    char **eltlist;
    int m;
    
    eltlist = 0;
    if (Tcl_SplitList(interp, vlist[i], &m, &eltlist) != TCL_OK)
    {
      goto loop_fail;
    }

    if (m == 0)
    {
      Tcl_SetResult(interp, "empty element in VLINK list", TCL_STATIC);
      goto loop_fail;
    }
    else
    {
      Tcl_HashEntry *p;
      VlinkFn *fn;
      
      if ( ! (p = Tcl_FindHashEntry(&vlinkFnHT, eltlist[0])))
      {
        Tcl_AppendResult(interp, "unknown VLINK field: ", eltlist[0], (char *)0);
        goto loop_fail;
      }
      fn = Tcl_GetHashValue(p);
      if ( ! fn(interp, v, m-1, eltlist+1))
      {
        goto loop_fail;
      }
    }
    
    free(eltlist);
    continue;

    /*  
     *  Any failure within the loop requiring clean-up and an error return
     *  comes here.
     */      
  loop_fail:
    free(vlist);
    vlfree(v);
    if (eltlist) free(eltlist);
    return 0;
  }
  return v;
}


int Prsp_VInit(Tcl_Interp *interp)
{
  int i;
  
  /*  
   *  Create a hash table whose keys are the fields of a VLINK structure, and
   *  whose values are pointers to functions to parse a string corresponding to
   *  that field, and set the appropriate value in the VLINK.
   *
   *  Functions have the prototype:
   *
   *  int fn(Tcl_Interp *interp, VLINK v, const char *s)
   *
   *  where v points to an allocated VLINK and s is a string representing the
   *  value of the given field.
   */
  Tcl_InitHashTable(&vlinkFnHT, TCL_STRING_KEYS);
  for (i = 0; vlinkFnTab[i].fn; i++)
  {
    Tcl_HashEntry *p;
    int new; /* junk */
    
    p = Tcl_CreateHashEntry(&vlinkFnHT, vlinkFnTab[i].field, &new);
    Tcl_SetHashValue(p, vlinkFnTab[i].fn);
  }

#if 0
  {
    Tcl_HashEntry *e;
    Tcl_HashSearch s;

    fprintf(stderr, "vlinkFnHT contains: \n\n");
    for (e = Tcl_FirstHashEntry(&vlinkFnHT, &s);
         e;
         e = Tcl_NextHashEntry(&s))
    {
      fprintf(stderr, "\tKey: [%s], value: 0x%p\n",
              Tcl_GetHashKey(&vlinkFnHT, e), (void *)Tcl_GetHashValue(e));
    }
  }
#endif
  return TCL_OK;
}
