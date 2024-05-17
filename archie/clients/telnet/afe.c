/*  
 *  Simulate archie.doc.ic.ac.uk's `login' program, which runs as root and is
 *  the parent and process group leader of the telnet-client.
 *
 *  This program should be setuid root.
 */    

#ifdef __STDC__
#include <stdlib.h>
#include <unistd.h>
#endif

#define TC "/archie/src/3.0/archie/bin/tc.test"


main(ac, av, envp)
  int ac;
  char **av;
  char **envp;
{
  switch (fork())
  {
  case -1:
    exit(1);
    
  case 0:
    setuid(getuid());
    setgid(getgid());
    execle(TC, "-telnet-client", 0, envp);
    exit(1);
    
  default:
    setuid(0);
    while (wait((int *)0) != -1);
    exit(0);
  }
}
