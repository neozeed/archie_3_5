/*  
 *  Characters that don't need to be quoted.  Supposedly, (according to the
 *  URL BNF) the space character is included, but we'll quote it just to be
 *  safe.
 *  
 *  abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789$-_@.&!*"'():;,
 *
 *  Thu Feb 24 14:55:43 EST 1994 - added double quote (decimal 34) to list of
 *                                 characters to be quoted.
 *  Wed Mar  9 16:39:28 EST 1994 - added ampersand (decimal 38).
 */  

#include <stdio.h>
#include <string.h>
#if !defined(AIX)
#include "defs.h"
#include "error.h"
#endif
#include "quoting.h"
#include "all.h"


#define IS_DHEX(n) ((n) >= '0' && (n) <= '9')
#define IS_LHEX(n) ((n) >= 'a' && (n) <= 'f')
#define IS_UHEX(n) ((n) >= 'A' && (n) <= 'F')

/* HEX() assumes `n' is a valid hex digit */
#define HEX(n) (IS_DHEX(n) ? (n) - '0' : IS_LHEX(n) ? (n) - 'a' + 10 : (n) - 'A' + 10)
#define IS_HEX(n) (IS_DHEX(n) || IS_LHEX(n) || IS_UHEX(n))
#define QCHAR '%'
#define QSTRSZ 3 /* Quoted character string size (%hh) */


/*  
 *
 *
 *                            Internal routines.
 *
 *
 */  


/*  
 *  Return the character corresponding to the quoted character in `s'.
 *
 *  `s' is assumed to point to a valied quoted character.
 */  
static char dequote(s)
  const char *s;
{
  return (HEX(s[1]) << 4) | HEX(s[2]);
}


/*  
 *  Return the number of characters in the decoded version of `s'.  A
 *  terminating nul is _not_ included in the result.
 *  
 *  Return -1 if `s' is not properly encoded.
 */  
static int dequoted_size(s)
  const char *s;
{
  int len = 0;
  int n = 0;

  for (; *s; s++)
  {
    len++;
    if (*s == QCHAR)
    {
      if (IS_HEX(s[1]) && IS_HEX(s[2]))
      {
        n++;
      }
      else
      {
        return -1;
      }
    }
  }

  return len - n * QSTRSZ + n;
}


/*  
 *  Return the number of characters needed to encode `s'.  A terminating
 *  nul is _not_ included in the result.
 */  
static int quoted_size(s, safech)
  const char *s;
  const unsigned char *safech;
{
  int len = 0;
  int n = 0;
  
  for (; *s; s++)
  {
    len++;
    if ( ! safech[(unsigned char)*s]) n++;
  }

  return len - n + n * QSTRSZ;
}


/*  
 *
 *
 *                            External routines.
 *
 *
 */  


char *dequote_string(s, safech, allocfn)
  const char *s;
  const unsigned char *safech;
  void *(*allocfn) proto_((size_t));
{
  char *p = 0;
  int nsz;
  
  if ((nsz = dequoted_size(s, safech)) == -1)
  {
    efprintf(stderr, "%s: url_dequote_string: improperly quoted string, `%s'.\n",
             prog, s);
    return 0;
  }

  if ( ! (p = allocfn(nsz + 1)))
  {
    efprintf(stderr, "%s: url_qedeuote_string: can't allocate %d bytes.\n",
             prog, nsz + 1);
    return 0;
  }
  else
  {
    char *qd;
    const char *qs;

    *p = '\0';
    for (qs = s, qd = p; *qs; qs++, qd++)
    {
      if (*qs != QCHAR)
      {
        *qd = *qs;
      }
      else
      {
        *qd = dequote(qs);
        qs += QSTRSZ - 1;
      }
    }

    *qd = '\0';
    return p;
  }
}


char *quote_string(s, safech, allocfn)
  const char *s;
  const unsigned char *safech;
  void *(*allocfn) proto_((size_t));
{
  char *p = 0;
  int nsz;
  
  if ((nsz = quoted_size(s, safech)) <= strlen(s))
  {
    return strdup(s);
  }

  if ( ! (p = allocfn(nsz + 1)))
  {
    efprintf(stderr, "%s: url_quote_string: can't allocate %d bytes.\n",
             prog, nsz + 1);
    return 0;
  }
  else
  {
    char *qd;
    const char *qs;

    *p = '\0';
    for (qs = s, qd = p; *qs; qs++, qd++)
    {
      if (safech[(unsigned char)*qs])
      {
        *qd = *qs;
      }
      else
      {
        sprintf(qd, "%%%02x", (unsigned char)*qs);
        qd += QSTRSZ - 1;
      }
    }

    *qd = '\0';
    return p;
  }
}
