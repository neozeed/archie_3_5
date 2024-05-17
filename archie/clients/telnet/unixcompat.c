#include <stdio.h>
#ifdef __svr4__
# include <sys/types.h>
# include <poll.h>
# include <stropts.h>
#else
#endif
#include <unistd.h>
#include "protos.h"
#include "unixcompat.h"

int regain_root()
{
#ifdef __svr4__

  return seteuid(0) != -1;
  
#else

  return setreuid(geteuid(), getuid()) != -1;

#endif
}


int revoke_root()
{
#ifdef __svr4__

  return seteuid(getuid()) != -1;
  
#else

  return setreuid(geteuid(), getuid()) != -1;

#endif
}


void u_sleep(usecs)
  unsigned int usecs;
{
#ifdef __svr4__
  struct pollfd dummy;
  int timeout;

  if ((timeout = usecs / 1000) <= 0)
  {
    timeout = 1;
  }
  poll(&dummy, 0, timeout);

#else

  (void)usleep(usecs);
#endif
}
