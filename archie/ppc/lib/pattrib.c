#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include "error.h"
#include "local_attrs.h"
#include "misc.h"
#include "pattrib.h"
#include "ppc_front_end.h"
#include "protos.h"
#include "all.h"


static void attr_append proto_((VLINK v, PATTRIB p));


#define LCAT(l1, l2)                                                              \
  do {                                                                            \
    if ( ! (l1))                                                                  \
    {                                                                             \
      (l1) = (l2);                                                                \
    }                                                                             \
    else if ((l2))                                                                \
    {                                                                             \
      (l1)->previous->next = (l2)->previous; /* use l1->previous->next as temp */ \
      (l2)->previous = (l1)->previous;                                            \
      (l1)->previous = (l1)->previous->next;                                      \
      (l2)->previous->next = (l2);                                                \
    }                                                                             \
  } while (0)


/*  
 *  Append one or more attributes, pointed to by `p', to the end of
 *  `v'->lattrib.
 *  
 *  Note: we don't want to use CONCATENATE_LISTS (a.k.a. APPEND_LISTS)
 *  because we would have to call it as CONCATENATE_LISTS(v->lattrib, p) to
 *  ensure that v->lattrib remains a valid doubly linked list pointer, but
 *  then `p' would no longer be the same list passed to us, as it would now
 *  appear _before_ v->lattrib.
 */      
static void attr_append(v, p)
  VLINK v;
  PATTRIB p;
{
  LCAT(v->lattrib, p);
}


PATTRIB get_contents(v)
  VLINK v;
{
  PATTRIB at = 0;

  if ( ! (at = vlinkAttr(CONTENTS, v)))
  {
    efprintf(stderr,
             "%s: get_contents: vlinkAttr() of CONTENTS failed on `%s:%s'.\n",
             logpfx(), v->host, v->hsoname);
    perrmesg((char *)0, 0, (char *)0);
  }

  /*  
   *  bug: sometimes the first attribute in the list is not CONTENTS.
   */
  return nextAttr(CONTENTS, at);
}


/*  
 *  Ask for the CONTENTS attributes from the server.  We call this if we
 *  don't want to be fooled into thinking we have all the CONTENTS
 *  attributes, just because some of them are on the link.
 */  
PATTRIB get_contents_from_server(v)
  VLINK v;
{
  PATTRIB p = 0;

  if ((p = ppc_pget_at(v, CONTENTS)))
  {
    attr_append(v, p);
  }

  /*  
   *  bug: see get_contents().
   */  
  return nextAttr(CONTENTS, p);
}


PATTRIB nextAttr(name, at)
  const char *name;
  PATTRIB at;
{
  PATTRIB a;
  
  for (a = at; a; a = a->next)
  {
    if (strcmp(a->aname, name) == 0)
    {
      break;
    }
  }

  return a;
}


/*  
 *  Look for an attribute attached to the VLINK, `v'.  If not found on the
 *  existing list of attributes make a Prospero call to try to find it.
 *  
 *  If we have to go back to the server to get the attribute, append it to
 *  the attribute list; we may need it again.
 */      
PATTRIB vlinkAttr(name, v)
  const char *name;
  VLINK v;
{
  PATTRIB p;
  
  if ( ! (p = nextAttr(name, v->lattrib)))
  {
    if ((p = ppc_pget_at(v, name)))
    {
      attr_append(v, p);
    }
  }

  return p;
}


/*  
 *  Create a single list of TOKENs consisting of the token lists from the
 *  named attribute.
 *  
 *  bug: should do error checking.
 */
TOKEN appendAttrTokens(at, name)
  PATTRIB at;
  const char *name;
{
  TOKEN ret = 0;

  while (at) {
    TOKEN t = tkcopy(at->value.sequence);
    LCAT(ret, t);
    at = at->next;
  }

  return ret;
}


/*  
 *  Return the first token from the local attribute.
 */  
char *nextAttrStr(name, attr)
  const char *name;
  PATTRIB attr;
{
  PATTRIB p;
  
  if ((p = nextAttr(name, attr)) &&
      p->value.sequence)
  {
    return p->value.sequence->token;
  }
  else
  {  
    return 0;
  }
}


/*  
 *  Return the first token string of the named attribute.
 */  
char *vlinkAttrStr(name, v)
  const char *name;
  VLINK v;
{
  PATTRIB p;

  if ((p = vlinkAttr(name, v)))
  {
    return nextAttrStr(name, p);
  }

  return 0;
}


#ifdef DEBUG
void aPrint(at)
  PATTRIB at;
{
  while (at)
  {
    TOKEN t;

    fprintf(stderr, "%s: ", at->aname);
    t = at->value.sequence;
    while (t)
    {
      fprintf(stderr, "%s", t->token);
      if ((t = t->next)) fprintf(stderr, " ; ");
    }
    fprintf(stderr, "\n");

    at = at->next;
  }
}
#endif
