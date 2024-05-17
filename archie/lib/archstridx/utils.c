#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "defs.h"
#include "utils.h"


int _archAtEOF(FILE *fp)
{
  int c, r;

  r = (c = getc(fp)) == EOF;
  ungetc(c, fp);
  return r;
}


/*  
 *  The returned length doesn't include a terminating nul.
 */  
static int vslen(const char *fmt, va_list ap)
{
  static FILE *nfp = 0;
  int len;

  if ( ! nfp && ! (nfp = fopen("/dev/null", "w")))
  {
    fprintf(stderr, "vslen: can't open `/dev/null' for writing.\n");
    return -1;
  }
  len = vfprintf(nfp, fmt, ap);
  return len;
}


/*  
 *  Like sprintf(), but allocate sufficient memory to contain the result.
 *
 *  Return the the number of characters, not including the terminating nul,
 *  written to *buf.  On error set *buf to 0 and return -1.
 *
 *  Note: on a Sun this is about half the speed of sprintf().
 */  
int _archMemSprintf(char **buf, const char *fmt, ...)
{
  char *mem;
  int len;
  va_list ap;

  va_start(ap, fmt);
  if ((len = vslen(fmt, ap)) < 0)
  {
    goto fail;
  }
  if ( ! (mem = malloc(len + 1)))
  {
    goto fail;
  }
  if ((len = vsprintf(mem, fmt, ap)) < 0)
  {
    free(mem);
    goto fail;
  }
  va_end(ap);
  *buf = mem;
  return len;

 fail:
  *buf = 0;
  va_end(ap);
  return -1;
}


/*  
 *  Open `dir'/`file' with `mode'.  An error is returned if we are unable to
 *  open the file, unless the file does not exist and `must_exist' is false.
 */
int _archFileOpen(const char *dir,
                  const char *file,
                  const char *mode,
                  int must_exist,
                  FILE **fp)
{
  FILE *f;
  char *path;

  if ( ! (path = _archMakePath(dir, "/", file))) {
    fprintf(stderr, "%s: _archFileOpen: can't build path `%s/%s'.\n", prog, dir, file);
    return 0;
  }

  errno = 0;
  if ( ! (f = fopen(path, mode)) && (must_exist || errno != ENOENT)) {
    fprintf(stderr, "%s: _archFileOpen: couldn't open `%s' with mode `%s': ", prog, path, mode);
    perror("fopen"); free(path);
    return 0;
  }

  free(path);
  *fp = f;

  return 1;
}


char *_archMakePath(const char *dir, const char *sep, const char *file)
{
  char *path;
  
  if (_archMemSprintf(&path, "%s%s%s", dir, sep, file) == -1) {
    fprintf(stderr, "%s: _archMakePath: can't create path `%s%s%s': ", prog, dir, sep, file);
    perror("_archMemSprintf");
    return 0;
  }

  return path;
}


/*
 *  Like strchr(), but assume we'll find the character before the end of the
 *  string.
 */
char *_archStrChr(char *s, int c)
{
  while (*s++ != c) {
    continue;
  }
  return s - 1;
}


char *_archStrDup(const char *s)
{
  char *c;

  if ((c = malloc(strlen(s) + 1))) {
    strcpy(c, s);
  }

  return c;
}


char *_archStrNDup(const char *s, size_t n)
{
  char *c;

  if ((c = malloc(n + 1))) {
    strncpy(c, s, n); c[n] = '\0';
  }

  return c;
}


/*
 *  Remove `file' from the directory `dir'.
 */
int _archUnlinkFile(const char *dir, const char *file)
{
  char *path;

  if ( ! (path = _archMakePath(dir, "/", file))) {
    fprintf(stderr, "%s: _archUnlinkFile: can't create path name.\n", prog);
    return 0;
  }

  (void)unlink(path);
  free(path);

  return 1;
}


long _archSystemPageSize(void)
{
#ifdef _SC_PAGESIZE
  return sysconf(_SC_PAGESIZE);
#else
  return getpagesize();
#endif
}


long _archFileSizeFP(FILE *fp)
{
  struct stat s;

  if (fstat(fileno(fp), &s) == -1) {
    fprintf(stderr, "%s: _archFileSizeFP: can't get size of file: ", prog);
    perror("fstat");
    return -1;
  }

  return s.st_size;
}

int identical( char c )
{
  return ((char) c);
}
