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
#include "print_results.h"
#include "url.h"
#include "all.h"


static int printAnonDirFmt proto_((FILE *ofp, VLINK v, const Where here, const char *lnterm));
static int printAnonFileFmt proto_((FILE *ofp, VLINK v, const Where here, const char *lnterm));


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
static int printAnonDirFmt(ofp, v, here, lnterm)
  FILE *ofp;
  VLINK v;
  const Where here;
  const char *lnterm;
{
  const char *qepath;
  int ret = 0;

  if (strcmp("DIRECTORY", v->target) != 0) /* file */
  {
#if 1
    char *url;
    
    if ((url = UrlFromVlink(v, URL_FTP)))
    {
      ret = efprintf(ofp, "<LI><A HREF=\"%s\">%s:%s</A>\r\n",
                     url, v->host, v->hsoname);
      free(url);
    }
    else
    {
      efprintf(stderr, "%s: printAnonDirFmt: UrlFromVlink() failed.\n",
               logpfx());
    }

#else

    if ( ! (qepath = url_quote(v->hsoname, malloc)))
    {
      efprintf(stderr, "%s: printAnonDirFmt: url_quote() failed.\n",
               logpfx());
    }
    else
    {
      ret = efprintf(ofp, "<LI><A HREF=\"ftp://%s%s\">%s:%s</A>\r\n",
                     v->host, qepath, v->host, v->hsoname);
      free((char *)qepath);
    }
#endif
  }
  else                          /* directory */
  {
    char *epath;
    
    if ( ! (epath = vlink_to_url(v)))
    {
      efprintf(stderr, "%s: printAnonDirFmt: vlink_to_url() failed.\n", logpfx());
    }
    else
    {
      if ( ! (qepath = url_quote(epath, malloc)))
      {
        efprintf(stderr, "%s: printAnonDirFmt: url_quote() failed.\n",
                 logpfx());
      }
      else
      {
        char *host;
        char *sl;

        if ( ! (host = strskip(v->hsoname, "ARCHIE/HOST/")))
        {
          efprintf(stderr, "%s: print_anon_dir_fmt: bad hsoname for directory in `%s:%s'.\n",
                   logpfx(), v->host, v->hsoname);
        }
        else if ( ! (sl = strchr(host+1, '/')))
        {
          /* bug? see bug below concerning no post-hostname slash. */
          efprintf(stderr, "%s: print_anon_dir_fmt: no post-host `/' in hsoname, `%s:%s'.\n",
                   logpfx(), v->host, v->hsoname);
        }
        else
        {
          /*bug: go back to weaseld source & see what's happening */
          *sl = '\0';
          /* Bit of a kludge to get around the trashed hsoname. */
          ret = efprintf(ofp, "<LI><A HREF=\"http://%s:%s/%s\">%s:/%s\t(dir)</A>\r\n",
                         here.host, here.port, qepath, host, sl+1);
          *sl = '/';
        }
        free((char *)qepath);
      }
      free((char *)epath);
    }
  }

  return ret;
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

static int printAnonFileFmt(ofp, v, here, lnterm)
  FILE *ofp;
  VLINK v;
  const Where here;
  const char *lnterm;
{
  char *sl;
  int ret = 1; /*bug: never gets reset */
  
  if ( ! (sl = strrchr(v->hsoname, '/')))
  {
    efprintf(stderr, "%s: printAnonFileFmt: no post-host `/' in hsoname, `%s:%s'.\n",
             logpfx(), v->host, v->hsoname);
  }
  else
  {
    char modstr[64];
    const char *a;
    char *host;
    char *location;
    char *locsl = 0;
    const char *mod = (a = vlinkAttrStr("LAST-MODIFIED", v)) ? a : "[no value]";
    const char *perm = (a = vlinkAttrStr("UNIX-MODES", v)) ? a : "[no value]";
    const char *size = (a = vlinkAttrStr("SIZE", v)) ? a : "[no value]";
    int is_dir;

    *sl = '\0';
    if ((host = strskip(v->hsoname, "ARCHIE/HOST/"))) /* directory */
    {
      is_dir = 1;
      locsl = strchr(host, '/');
      /*  
       *  bug? [Mon Mar 7 18:37:09 EST 1994]
       *  
       *  First time I've seen an hsoname come back as
       *  
       *    ARCHIE/HOST/ftp.virginia.edu
       *  
       *  i.e. without a `/' after the host name.
       */  
      if ( ! locsl)
      {
        location = "";
      }
      else
      {
        *locsl = '\0';
        location = locsl + 1;
      }
    }
    else                        /* file */
    {
      is_dir = 0;
      host = v->host;
      /*bug: empty hsoname*/
      location = v->hsoname + 1; /* be consistent & knock off leading `/' */
    }

    if (strcmp(lasthostname, host))
    {
      /* New host */
      char hupdatestr[64];
      const char *hupdate = (a = vlinkAttrStr("AR_H_LAST_MOD", v)) ? a : "[no value]";
      const char *ip = (a = vlinkAttrStr("AR_H_IP_ADDR", v)) ? a : "[no value]";

#ifdef AIX
      pstrftime(hupdatestr, sizeof hupdatestr, "%H:%M %e %h %Y", hupdate);
#else
      pstrftime(hupdatestr, sizeof hupdatestr, "%R %e %h %Y", hupdate);
#endif
      efprintf(ofp, "\r\n\r\nHost %s    (%s)\r\n", host, ip);
      efprintf(ofp, "Last updated %s\r\n", hupdatestr);

      strcpy(lasthostname, host);
      lastlocation[0] = '\377'; /* force location to be printed */
    }

    if (strcmp(lastlocation, location))
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

  return ret;
}


int anon_print(ofp, req, print_as_dir)
  FILE *ofp;
  Request *req;
  int print_as_dir;
{
  VLINK v;
  int ret = 1;
  
  /* bug? for no slash bug.  See above */
  lastlocation[0] = lasthostname[0] = '\377';
  
  print_vllength(ofp, req);

  efprintf(ofp, "<P>%s\r\n", print_as_dir ? "<MENU>" : "<PRE>");
  for (v = req->res; v; v = v->next)
  {
    if (print_as_dir)
    {
      ret = printAnonDirFmt(ofp, v, req->here, "\r\n") ? ret : 0;
    }
    else
    {
      ret = printAnonFileFmt(ofp, v, req->here, "\r\n") ? ret : 0;
    }
  }
  efprintf(ofp, "%s</BODY>\r\n", print_as_dir ? "</MENU>" : "</PRE>");

  return ret;
}
