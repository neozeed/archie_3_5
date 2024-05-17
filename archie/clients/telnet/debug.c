#ifdef __STDC__
#include <stdlib.h>
#endif
#include "debug.h"
#include "extern.h"
#include "prosp.h"

#include "protos.h"

static int debug_level;


int dlev()
{
  return debug_level;
}


int set_debug(v)
  const char *v;
{
  debug_level = atoi(v);
  pfs_debug = debug_level;
  return 1;
}


int unset_debug()
{
  debug_level = 0;
  return 1;
}


void uids(s)
  const char *s;
{
  fprintf(stderr, "%s: %seuid: %ld, ruid: %ld -- egid: %ld, rgid: %ld.\n",
          prog, s ? s : "", (long)geteuid(), (long)getuid(), (long)getegid(), (long)getgid());
}
