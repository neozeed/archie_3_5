#ifdef __STDC__
# include <stdlib.h>
# include <unistd.h>
#else
# include <memory.h> /* memset(), memcpy() */
#endif
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include "ansi_compat.h"
#include "defs.h"
#include "error.h"
#include "local_attrs.h"
#include "misc.h"
#include "net.h"
#include "pattrib.h"
#include "protos.h"
#include "str.h"
#include "all.h"


void fprint_argv(fp, av)
  FILE *fp;
  char **av;
{
  for (; *av; av++)
  {
    efprintf(fp, "%s\n", *av);
  }
}


int getHostPort(hp, h, p)
  const char *hp;
  char *h;
  char *p;
{
  int ret = 1;

  if (sscanf((char *)hp, "%[^(](%[0-9])", h, p) != 2)
  {
    efprintf(stderr, "%s getHostPort: malformed host/port string `%s'.\n", logpfx(), hp);
    ret = 0;
  }

  return ret;
}


char *head(path, buf, bufsize)
  const char *path;
  char *buf;
  int bufsize;
{
  const char *s = strrchr(path, '/');

  if ( ! s)
  {
    *buf = '\0';
    return buf;
  }
  else
  {
    int l = min(bufsize, s - path);

    if (l == 0)
    {
      strcpy(buf, "/");
    }
    else
    {
      strncpy(buf, path, l);
      buf[l] = '\0';
    }
    return buf;
  }
}


/*  
 *  Return a string suitable for prefixing to log/error messages.
 */  
char *logpfx()
{
  static char s[256];

  sprintf(s, "%s(%ld) [%s]", prog, (long)getpid(), now());
  return s;
}


char *now()
{
  static char t[64];
  time_t clock;

  time(&clock);
  strftime(t, sizeof t, "%e %h %Y %T GMT", gmtime(&clock));
  return t;
}


char *nuke_afix_ws(s)
  char *s;
{
  char *b = s + strspn(s, " \t");
  char *e = strend(s) - strrspn(s, " \t");

  *e = '\0';
  if (b != s)
  {
    memcpy(s, b, e - b + 1);
  }
  return s;
}


/*  
 *  `v' is a list of results from a search.  Return 1 if we should print the
 *  result as a formatted item, or 0 if we should print headlines.
 *
 *  If there is one result and it contains one or more CONTENTS attributes,
 *  but no HEADLINES attribute then we should immediately print the contents.
 *  Otherwise, we should print headlines.
 *
 */
int immediatePrint(v, cat)
  VLINK v;
  PATTRIB *cat;
{
  return ( ! v->next &&
          (*cat = nextAttr(CONTENTS, v->lattrib)) &&
          ! nextAttr(HEADLINE, v->lattrib) &&
          ! nextAttr(REPLACE_WITH_URL, v->lattrib));
}


int there(w, ipstr)
  Where *w;
  const char *ipstr;
{
  strcpy(w->host, ipstr);
  strcpy(w->port, "0");
  return 1;
}

/*  
 *  bug? Maybe this should go in net.c?  (portnumTCP)
 */  
int where(w, service)
  Where *w;
  const char *service;
{
  int port;

  gethostname(w->host, sizeof w->host);
  if ((port = portnumTCP(service)) != 0)
  {
    sprintf(w->port, "%d", port);
    return 1;
  }
  else
  {
    efprintf(stderr, "%s where: portnumTCP() failed.\n", logpfx());
    return 0;
  }
}
