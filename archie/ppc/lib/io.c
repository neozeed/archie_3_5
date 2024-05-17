#ifdef __STDC__
# include <stdlib.h>
#else
# include <malloc.h>
#endif
/* These three for file_is_local() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <setjmp.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "class.h"
#include "defs.h"
#include "error.h"
#include "io.h"
#include "misc.h"
#include "os_indep.h"
#include "output.h"
#include "pattrib.h"
#include "ppc_front_end.h"
#include "protos.h"
#include "psearch.h"
#include "str.h"
#include "all.h"


#define BSZ 4096
#define MAX_READ_TIME 600    /* allow 10 minutes to read a request */


/*bug: shouldn't have to call this.*/
extern const char *service_str proto_((void));


static int blat_local_file proto_((FILE *ofp, VLINK v, int is_text));
static int f_binary proto_((FILE *ifp, FILE *ofp, char *buf, int bsz));
static int f_text proto_((FILE *ifp, FILE *ofp, char *buf, int bsz));
  

static int f_binary(ifp, ofp, buf, bsz)
  FILE *ifp;
  FILE *ofp;
  char *buf;
  int bsz;
{
  int n;
    
  while ((n = fread(buf, 1, bsz, ifp)) > 0)
  {
    if (fwrite(buf, 1, n, ofp) != n)
    {
      efprintf(stderr, "%s: f_binary: error fwrite()ing %d bytes: %m.\n", logpfx(), n);
      return 0;
    }
  }

  return feof(ifp);
}


static int f_text(ifp, ofp, buf, bsz)
  FILE *ifp;
  FILE *ofp;
  char *buf;
  int bsz;
{
  while (fgets(buf, bsz, ifp))
  {
    if (buf[0] == '.')
    {
      if (putc('.', ofp) == EOF)
      {
        efprintf(stderr, "%s: f_text: error putc()ing `.': %m.\n", logpfx());
        return 0;
      }
    }
    /*  
     *  Terminate lines with CR LF.
     *  
     *  bug: this will have the effect of folding very long lines.
     */    
    trimright(buf, "\r\n");
    if (fputs(buf, ofp) == EOF || fputs("\r\n", ofp) == EOF)
    {
      efprintf(stderr, "%s: f_text: error from fputs(): %m.\n", logpfx());
      return 0;
    }
  }

  return feof(ifp);
}


static int blat_local_file(ofp, v, is_text)
  FILE *ofp;
  VLINK v;
  int is_text;
{
  FILE *ifp;
  int ret = 0;

  if ( ! (ifp = fopen(v->hsoname, "rb")))
  {
    efprintf(stderr, "%s: blat_local_file: error opening `%s': %m.\n",
             logpfx(), v->hsoname);
    return 0;
  }

  dfprintf(stderr, "%s: blat_local_file: get `%s' locally.\n",
           logpfx(), v->hsoname);
  ret = fcopy(ifp, ofp, is_text);
  fclose(ifp);

  return ret;
}


int fcopy(ifp, ofp, is_text)
  FILE *ifp;
  FILE *ofp;
  int is_text;
{
  static char *buf = 0;

  /*bug: never gets freed*/
  if ( ! buf && ! (buf = malloc(BSZ)))
  {
    efprintf(stderr, "%s: fpcopy: can't malloc() %d bytes: %m.\n", logpfx(), BSZ);
    return 0;
  }

  return (is_text ? f_text : f_binary)(ifp, ofp, buf, BSZ);
}


/*  
 *
 */  
int fcopysize(ifp, ofp, size)
  FILE *ifp;
  FILE *ofp;
  int size;
{
  while (size > 0)
  {
    char buf[1024];
    int n, r;
          
    r = min(size, sizeof buf);
    if ((n = timed_fread(buf, 1, r, ifp)) > 0)
    {
      size -= n;
      if (fwrite(buf, 1, n, ofp) != n) /* bug! assumes `ofp' is file! */
      {
        efprintf(stderr, "%s: fcopysize: can't write %d bytes: %m.\n", logpfx(), n);
        return 0;
      }
    }
    else if (n == 0)            /* unexpected EOF */
    {
      efprintf(stderr, "%s: fcopysize: unexpected end-of-file.\n", logpfx());
      return 0;
    }
    else
    {
      efprintf(stderr, "%s: fcopysize: error reading %ld bytes: %m.\n",
               logpfx(), (long)sizeof buf);
      return 0;
    }
  }

  return 1;
}


int file_is_local(v)
  VLINK v;
{
  char **p;
  char h[MAXHOSTNAMELEN+1];
  int hlen;
  struct hostent *hent;
  
  if (v->hsoname[0] != '/')
  {
    return 0;
  }

  h[0] = '\0';
  gethostname(h, sizeof h);
  if ( ! (hent = gethostbyname(h)))
  {
    efprintf(stderr, "%s: file_is_local: gethostbyname() failed on `%s'.\n",
             logpfx(), h);
    return 0;
  }

  hlen = strcspn(v->host, "(");
  if (strncasecmp(hent->h_name, v->host, hlen) == 0)
  {
    return 1;
  }

  for (p = hent->h_aliases; *p; p++)
  {
    if (strncasecmp(*p, v->host, hlen) == 0)
    {
      return 1;
    }
  }

  return 0;
}


int fpsock(s, ifp, ofp)
  int s;
  FILE **ifp;
  FILE **ofp;
{
  FILE *i;
  FILE *o;

  if ( ! (i = fdopen(s, "r")))
  {
    return 0;
  }
  if ( ! (o = fdopen(s, "w")))
  {
    fclose(i);
    return 0;
  }

  *ifp = i;
  *ofp = o;

  return 1;
}


/*  
 *  Read from `ifp' until a newline is seen or EOF is encountered.  Return
 *  the line (including newline) in a malloc()ed buffer.
 */
char *memfgets(ifp)
  FILE *ifp;
{
  char *mem = 0, *newmem;
  int c = 0;
  int used = 0;

  while (c != '\n' && c != EOF) {
    char buf[256];
    int nch;

    for (nch = 0; nch < sizeof buf;) {
      buf[nch++] = (char)(c = fgetc(ifp));
      if (c == '\n') break;
      if (c == EOF) { --nch ; break; }
    }

    if (nch == 0) return 0;     /* immediate EOF */

    if (mem) newmem = realloc(mem, used + nch + 1);
    else newmem = malloc(nch + 1);

    if ( ! newmem) goto badmem;

    mem = newmem;
    memcpy(mem + used, buf, nch);
    used += nch;
  }

  mem[used] = '\0';
  return mem;

 badmem:
  if (mem) free(mem);
  return 0;
}


/*  
 *  Get a line.  Ensure it's <CR><LF> terminated.
 */  
char *readline(ofp, debug)
  FILE *ofp;
  int debug;
{
  char *line;

  if ((line = memfgets(ofp)))
  {
    char *cr;
    
    if ((cr = strrchr(line, '\r')))
    {
      *cr = '\0';
      if (debug) efprintf(stderr, "%s: readline: `%s'.\n", logpfx(), line);
      return line;
    }

    efprintf(stderr, "%s: readline: badly terminated line `%s'.\n", logpfx(), line);
    free(line);
  }
  
  return 0;
}


int timed_fread(ptr, size, nitems, stream)
  char *ptr;
  int size;
  int nitems;
  FILE *stream;
{
  int n;

  ppc_signal(SIGALRM, exit);
  alarm(MAX_READ_TIME);
  n = fread(ptr, size, nitems, stream);
  alarm(0);
  return n;
}


/*  
 *  Call readline to get a line.  Also, set a time out for reading the line.
 *  
 *  (As long as we exit on the alarm we're okay, otherwise we have potential
 *  memory leak or corruption problems, since memfgets() calls malloc().  If
 *  necessary we could block SIGALRM around the malloc().)
 */
char *timed_readline(ofp, debug)
  FILE *ofp;
  int debug;
{
  char *line;

  ppc_signal(SIGALRM, exit);
  alarm(MAX_READ_TIME);
  line = readline(ofp, debug);
  alarm(0);
  return line;
}


int timed_getc(ifp)
  FILE *ifp;
{
  int c;

  ppc_signal(SIGALRM, exit);
  alarm(MAX_READ_TIME);
  c = getc(ifp);
  alarm(0);
  return c;
}


/* bug: clean it up. */
int blat_file(ofp, v)
  FILE *ofp;
  VLINK v;
{
  PATTRIB at;  /*bug! doesn't free `at'.*/
  TOKEN t;
  const char *ts;
  int is_text = 0;

  extern int tagprint proto_((FILE *ofp, PATTRIB cat));

  /*bug! KLUDGE! Replace with a sysconf() type call?*/
  if (strcmp(service_str(), "gopher") == 0)
  {
    /* Is this file a text file? */
    if (get_class(v) == B_CLASS_DOCUMENT)
    {
      is_text = 1;
    }
  }

  if (file_is_local(v))
  {
    if (blat_local_file(ofp, v, is_text))
    {
      return 1;
    }
  }
  
  if ( ! (at = get_contents(v)))
  {
    efprintf(stderr, "%s: blat_file: get_contents() failed.\n", logpfx());
    return 0;
  }

  t = at->value.sequence;
  ts = t->token;

  /*  
   *  Assume that `DATA' and `TAGGED' aren't mixed.
   */  
  if (strcmp(ts, "DATA") == 0)
  {
    const char *contents = nth_token_str(t, 1);
    long sz;

    if ((sz = contents_size(v)) == -1)
    {
      efprintf(stderr, "%s: blat_file: can't get size of contents.\n", logpfx());
      return 0;
    }
      
    /*bug: text files with `.' lines?*/
    if (fwrite((char *)contents, 1, (int)sz, ofp) != sz)
    {
      efprintf(stderr, "%s: blat_file: fwrite() failed to write %ld bytes: %m.\n",
               logpfx(), (long)sz);
    }
  }
  else if (strcmp(ts, "TAGGED") == 0)
  {
    tagprint(ofp, at);
  }
  else
  {
    efprintf(stderr, "%s: blat_file: unknown CONTENTS token, `%s', in `%s:%s'.\n",
             logpfx(), ts, v->host, v->hsoname);
    return 0;
  }

  return 1;
}


long contents_size(v)
  VLINK v;
{
#define SZPAT "%ld bytes"

  const char *s;
  long ret = -1;

  if ((s = vlinkAttrStr("SIZE", v)))
  {
    long sz;

    if (sscanf((char *)s, SZPAT, &sz) == 1)
    {
      ret = sz;
    }
    else
    {
      efprintf(stderr, "%s: contents_size: `%s' didn't match `%s'.\n",
               logpfx(), s, SZPAT);
    }
  }

  return ret;
}


long local_file_size(v)
  VLINK v;
{
  struct stat s;

  if (stat(v->hsoname, &s) != -1)
  {
    return s.st_size;
  }
  else
  {
    efprintf(stderr, "%s: local_file_size: can't stat() `%s': %m.\n",
             logpfx(), v->hsoname);
    return -1;
  }
}
