#ifdef __STDC__
# include <stdlib.h>
#endif
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include "aprint.h"
#include "ppc.h"
#include "ppc_time.h"
#include "url.h"


static void print_anon_dirfmt proto_((FILE *ofp, VLINK v, const Where here, const char *lnterm));
#if 1
static void print_anon_filefmt proto_((FILE *ofp, VLINK v));
#else
static void print_anon_filefmt proto_((FILE *ofp, VLINK item, int style, const char *lnterm));
#endif


static char lastlocation[1024];
static char lasthostname[1024];


/*  
 *  Directory:
 *  
 *  target: DIRECTORY
 *  hsoname: ARCHIE/HOST/<host>/<path>
 *  
 *  File:
 *  
 *  target: EXTERNAL
 *  host: <host>
 *  hsoname: <path>
 */  
static void print_anon_dirfmt(ofp, v, here, lnterm)
  FILE *ofp;
  VLINK v;
  const Where here;
  const char *lnterm;
{
  if (strcmp("DIRECTORY", v->target) != 0) /* file */
  {
    efprintf(ofp, "0%s:%s\tftp:%s@%s\t%s\t%s%s",
             v->host, v->hsoname, v->host, v->hsoname,
             here.host, here.port, lnterm);
  }
  else                          /* directory */
  {
    char *host;
    char *selstr;
    char *sl;

    if ( ! (selstr = vlink_to_url(v)))
    {
      efprintf(stderr, "%s: print_anon_dirfmt: vlink_to_url() failed on `%s:%s'.",
               logpfx(), v->host, v->hsoname);
      return;
    }

    if ( ! (host = strskip(v->hsoname, "ARCHIE/HOST/")))
    {
      efprintf(stderr, "%s: print_anon_dir_fmt: bad hsoname for directory in `%s:%s'.\n",
               logpfx(), v->host, v->hsoname);
    }
    else if ( ! (sl = strchr(host+1, '/')))
    {
      efprintf(stderr, "%s: print_anon_dir_fmt: no post-host `/' in hsoname, `%s:%s'.\n",
               logpfx(), v->host, v->hsoname);
    }
    else
    {
      *sl = '\0';
      /* Bit of a kludge to get around the trashed hsoname. */
      efprintf(ofp, "1%s:/%s\t%s\t%s\t%s%s",
               host, sl+1, selstr, here.host, here.port, lnterm);
      *sl = '/';
    }

    free(selstr);
  }
}


/*  
 *  Host qiclab.scn.rain.com (147.28.0.97)
 *  Last updated 02:36 2 Jan 1993
 *  
 *  Location: /pub/sysadmin
 *    FILE rw-rw-r-- 25198 Feb 20 1992 lldump.tar.Z
 *  
 *  Host oersted.ltf.dth.dk (129.142.66.16)
 *  Last updated 02:49 31 Dec 1992
 *  
 *  Location: /pub/Utilities
 *    FILE r--r--r-- 27611 Jun 11 1992 lldump.tar.Z
 */  

/* bug: get rid of dependency on aq_lhsoname(), etc.? */

static void print_anon_filefmt(ofp, v)
  FILE *ofp;
  VLINK v;
{
  char *sl;
  
  if ( ! (sl = strrchr(v->hsoname, '/')))
  {
      efprintf(stderr, "%s: print_anon_filefmt: no post-host `/' in hsoname, `%s:%s'.\n",
               logpfx(), v->host, v->hsoname);
  }
  else
  {
    char modstr[64];
    const char *a;
    char *host;
    char *location;
    char *locsl = 0;
    const char *mod = (a = vlinkAttrStr("LAST-MODIFIED", v)) ? a : "<no value>";
    const char *perm = (a = vlinkAttrStr("UNIX-MODES", v)) ? a : "<no value>";
    const char *size = (a = vlinkAttrStr("SIZE", v)) ? a : "<no value>";
    int is_dir;

    *sl = '\0';
    if ((host = strskip(v->hsoname, "ARCHIE/HOST/"))) /* directory */
    {
      is_dir = 1;
      if ( ! (locsl = strchr(host, '/')))
      {
        location = ""; /* bug? shouldn't `/' always appear? (I know it doesn't) */
      }
      else
      {
        *locsl = '\0';
        location = locsl + 1;
      }
    }
    else /* file */
    {
      is_dir = 0;
      host = v->host;
      /*bug: empty hsoname*/
      location = v->hsoname + 1; /* be consistent & knock off leading `/' */
    }

    if (strcmp(lasthostname, host))
    {
      char hupdatestr[64];
      const char *hupdate = (a = vlinkAttrStr("AR_H_LAST_MOD", v)) ? a : "<no value>";
      const char *ip = (a = vlinkAttrStr("AR_H_IP_ADDR", v)) ? a : "<no value>";

#ifdef AIX
      pstrftime(hupdatestr, sizeof hupdatestr, "%H:%M %e %h %Y", hupdate);
#else
      pstrftime(hupdatestr, sizeof hupdatestr, "%R %e %h %Y", hupdate);
#endif
      efprintf(ofp, "\r\n\r\nHost %s    (%s)\r\n", host, ip);
      efprintf(ofp, "Last updated %s\r\n", hupdatestr);

      strcpy(lasthostname, host);

      /* bug: kludge - duplication of code */
      efprintf(ofp, "\r\n    Location: /%s\r\n", location);
      strcpy(lastlocation, location);
    }
    else if (strcmp(lastlocation, location) || str)
    {
      efprintf(ofp, "\r\n    Location: /%s\r\n", location);
      strcpy(lastlocation, location);
    }

#ifdef AIX
    pstrftime(modstr, sizeof modstr, "%H:%M %e %h %Y", mod);
#else
    pstrftime(modstr, sizeof modstr, "%R %e %h %Y", mod);
#endif
    efprintf(ofp, "      %s    %s %7s %12s  %s\r\n",
             is_dir ? "DIRECTORY" : "FILE", perm, size, modstr, v->name);
    *sl = '/';
    if (locsl) *locsl = '/';
  }
}


void anon_print(ofp, v, print_as_dir, here)
  FILE *ofp;
  VLINK v;
  int print_as_dir;
  const Where here;
{
  lastlocation[0] = lasthostname[0] = '\0';
  
  for (; v; v = v->next)
  {
    if (print_as_dir)
    {
      print_anon_dirfmt(ofp, v, here, "\r\n");
    }
    else
    {
      print_anon_filefmt(ofp, v);
    }
  }
}
