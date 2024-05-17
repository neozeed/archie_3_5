/*  
 *  - cd & ?chroot
 *  - detach
 *  - listen & loop on accept
 *
 *  Bugs:
 *
 *    Doesn't handle text output with a period on a line by itself.
 */

#ifdef __STDC__
# include <stdlib.h>
# include <unistd.h>
#endif
#ifdef NeXT
# include <sys/ioctl.h>
#else
# ifdef AIX
#  include <termios.h>
#  include <sys/ioctl.h>
# else
#  include <sys/termios.h>
# endif
#endif
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <ctype.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "_prog.h"
#include "ansi_compat.h"
#include "defs.h"
#include "error.h"
#include "host_access.h"
#include "io.h"
#include "misc.h"
#include "net.h"
#include "os_indep.h"
#include "ppc_front_end.h"
#include "ppc_tcl.h"
#include "prosp.h"
#include "protos.h"
#include "redirect.h"
#include "vlink.h"
#include "all.h"


#define EXIT(really) if (really) exit(0)
#define FORK(really) (really ? fork() : 0)
#define NEXTARG if ( ! (av++, --ac)) usage()
#define NSETARG(var) NEXTARG; var = atoi(av[0])
#define SETARG(var) NEXTARG; var = av[0]
#define VSDESC_ROOT "/pfs/pfsdat"
#define VSDESC "local_vsystems/bunyip/VS-DESCRIPTION"
#define WAIT(really) do { while (wait((int *)0) != -1); } while (0)
#define dfprintf if (debug) efprintf


static int detach proto_((void));
static int load_acl proto_((const char *file));
static int load_redirects proto_((const char *file));
static int user_id proto_((const char *user, uid_t *uid));
static void child_wait proto_((int sig));
static void log_reset proto_((int sig));
static void usage proto_((void));


Where them;
Where us;
const char *prog;
int debug = 0;
int reuseaddr = 0;
volatile int reset_logfile = 0;


#ifdef BUNYIP_AUTHENTICATION
char *passfile = NULL;
#endif


int main(ac, av)
  int ac;
  char **av;
{
  char *logfile = 0;
  char *user = 0;
  char envhost[MAXHOSTNAMELEN+64];
  char envport[64];
  const char *aclfile = 0;
  const char *config = 0;
  const char *emesg = "/dev/console";
  const char *phost = "localhost";
  const char *proot = 0;
  const char *redfile = 0;
  const char *service = service_str();
  char vsdesc[MAXPATHLEN];
  int do_detach = 1;
  int lfd;
  int r;

#ifdef DEBUG
  ppc_signal(SIGINT, exit); /* Exit on SIGINT */
#endif  

  strcpy(vsdesc,VSDESC_ROOT);

  prog = tail(av[0]);
  while (av++, --ac)
  {
    if (strcmp("-acl", av[0]) == 0)
    {
      SETARG(aclfile);
    }
    else if (strcmp("-debug", av[0]) == 0)
    {
      debug = 1;
    }
    else if (strcmp("-config", av[0]) == 0)
    {
      SETARG(config);
    }
    else if (strcmp("-emesg", av[0]) == 0)
    {
      SETARG(emesg);
    }
    else if (strcmp("-log", av[0]) == 0)
    {
      SETARG(logfile);
    }
#ifdef BUNYIP_AUTHENTICATION
    else if (strcmp("-passfile",av[0]) == 0)
    {
      SETARG(passfile);
    }
#endif
    else if (strcmp("-pdebug", av[0]) == 0)
    {
      NSETARG(pfs_debug);
    }
    else if (strcmp("-pfsdat", av[0]) == 0)
    {
      NEXTARG;
      strcpy(vsdesc,av[0]);
    }
    else if (strcmp("-phost", av[0]) == 0)
    {
      SETARG(phost);
    }
    else if (strcmp("-port", av[0]) == 0)
    {
      SETARG(service);
    }
    else if (strcmp("-proot", av[0]) == 0)
    {
      SETARG(proot);
    }
    else if (strcmp("-redirect", av[0]) == 0)
    {
      SETARG(redfile);
    }
    else if (strcmp("-reuseaddr", av[0]) == 0)
    {
      reuseaddr = 1;
    }
    else if (strcmp("-stay", av[0]) == 0)
    {
      do_detach = 0;
    }
    else if (strcmp("-user", av[0]) == 0)
    {
      SETARG(user);
    }
    else
    {
      usage();
    }
  }

  /* Load an ACL file, if necessary. */
  if (aclfile && ! load_acl(aclfile))
  {
    exit(1);
  }
  
  /* Load a redirection file, if necessary. */
  if (redfile && ! load_redirects(redfile))
  {
    exit(1);
  }

  if ( vsdesc[strlen(vsdesc)-1] != '/' )
      strcat(vsdesc,"/");
  strcat(vsdesc,VSDESC);

  ppc_signal(SIGUSR2, log_reset);

  ppc_signal(SIGCHLD, child_wait);
  if (do_detach) detach();

  /* bug: logfile shouldn't be relative to chdir() */
  if (do_detach && ! freopen(logfile ? logfile : "/dev/null", "a+", stderr))
  {
    error(emesg, "%s can't open `%s' as log file: %m -- exiting.\n",
          logpfx(), logfile);
    exit(1);
  }

  setvbuf(stderr, (char *)0, _IOLBF, (size_t)0);

  ppc_p_initialize(prosp_ident(), 0, (struct p_initialize_st *)0);
  while ((r = ppc_vfsetenv((char *)phost, vsdesc, "")) != PSUCCESS)
  {
    efprintf(stderr, "%s: error from vfsetenv(): ", logpfx());
    perrmesg((char *)0, r, (char *)0);
    efprintf(stderr, "%s: sleeping for 60 seconds.\n", logpfx());
    sleep(60);
  }

  if (proot && ! vcd(proot))
  {
    exit(1);
  }

  if ( ! where(&us, service))
  {
    efprintf(stderr, "%s: where() failed -- exiting.\n", logpfx());
    exit(1);
  }

  /*  
   *  bug: Kludge?
   *
   *  Set environment variables BUNYIP_HOST and BUNYIP_PORT for the benefit
   *  of external programs that may be launched by us.
   */
  sprintf(envhost, "BUNYIP_HOST=%s", us.host);
  putenv(envhost);
  sprintf(envport, "BUNYIP_PORT=%s", us.port);
  putenv(envport);

  if ((lfd = passiveTCP(service, 1)) < 0)
  {
    efprintf(stderr, "%s: can't open socket -- exiting.\n", logpfx());
    exit(1);
  }

  if (user)
  {
    uid_t uid;

    if ( ! user_id(user, &uid))
    {
      efprintf(stderr, "%s: can't find uid of `%s'.\n", logpfx(), user);
      exit(1);
    }
    if (setuid(uid) == -1)
    {
      efprintf(stderr, "%s: can't setuid() to %d: %m.\n", logpfx(), (int)uid);
      exit(1);
    }
  }

#ifndef NO_TCL
  tcl_init(config);
#endif
  
  efprintf(stderr, "%s: ready to accept connections.\n", logpfx());

  while (1)
  {
    int alen;
    int fd;
    struct sockaddr_in fsin;

    if (reset_logfile && logfile) {
      reset_logfile = 0;
      fclose(stderr);
      freopen(logfile, "a+", stderr);
    }
    
    alen = sizeof fsin;
    if ((fd = accept(lfd, (struct sockaddr *)&fsin, &alen)) < 0)
    {
      if (errno != EINTR)
      {
        efprintf(stderr, "%s: error accept()ing: %m -- sleeping for 10 seconds.\n",
                 logpfx());
        sleep(10);
      }
      continue;
    }
    
#ifndef NO_TCL
    tcl_reinit_if_needed();
#endif

    switch (FORK(do_detach))
    {
    case -1:
      efprintf(stderr, "%s: error fork()ing: %m -- waiting 10 seconds.\n", logpfx());
      sleep(10);
      break;
      
    case 0:
      {
        FILE *ifp;
        FILE *ofp;

        if ( ! fpsock(fd, &ifp, &ofp))
        {
          efprintf(stderr, "%s: can't create I/O file pointers from socket.\n", logpfx());
          close(fd);
        }
        else
        {
          efprintf(stderr, "%s: connection from %s on remote port %d: ",
                   logpfx(), sockAddr(fd), sockPortNum(fd));
          if (is_host_allowed(net_addr(sockAddr(fd)))) /*bug: clean up */
          {
            efprintf(stderr, "access granted.\n");
            there(&them, sockAddr(fd));
            handle_transaction(ifp, ofp, us, them);
          }
          else
          {
            efprintf(stderr, "access denied.\n");
            access_denied(ifp, ofp, us);
          }
          fclose(ifp); fclose(ofp);
        }
        EXIT(do_detach);
      }
      break;
      
    default:
#if 0
      WAIT(do_detach);
#endif
      break;
    }

    close(fd);
  }

  exit(0);
}


static int detach()
{
  int fd;

  ppc_signal(SIGTTOU, SIG_IGN);
  ppc_signal(SIGTTIN, SIG_IGN);
  ppc_signal(SIGTSTP, SIG_IGN);
  switch(fork())
  {
  case -1:
    efprintf(stderr, "%s: detach: error from fork().\n", logpfx());
    return 0;

  case 0:
    break;

  default:
    exit(0);
  }

#ifndef SOLARIS  
  setpgrp(0, getpid());
#else
  setpgrp();
#endif

  if((fd = open("/dev/tty", O_RDWR)) >= 0)
  {
    ioctl(fd, TIOCNOTTY, 0) ;   /* Waste the controlling terminal */
    close(fd);
  }

  for(fd = 0 ; fd < max_open_files() ; fd++)
  {
    close(fd);
  }
  umask(0);

  return 1;
}


static int load_acl(file)
  const char *file;
{
  int n;
    
  if (strcmp(file, "-") == 0)
  {
    n = load_host_acl_fp(stdin);
  }
  else
  {
    n = load_host_acl_file(file);
  }

  if (n != -1)
  {
    efprintf(stderr, "%s: loaded %d networks and/or sites from `%s'.\n",
             logpfx(), n, file);
    return 1;
  }
  else
  {
    efprintf(stderr, "%s: error trying to load ACL file, `%s' -- exiting.\n",
             logpfx(), file);
    return 0;
  }
}


static int load_redirects(file)
  const char *file;
{
  int n;
    
  if (strcmp(file, "-") == 0)
  {
    n = load_redirection_fp(stdin);
  }
  else
  {
    n = load_redirection_file(file);
  }

  if (n != -1)
  {
    efprintf(stderr, "%s: loaded %d redirection strings from `%s'.\n",
             logpfx(), n, file);
    return 1;
  }
  else
  {
    efprintf(stderr, "%s: error trying to load redirection file, `%s' -- exiting.\n",
             logpfx(), file);
    return 0;
  }
}


static int user_id(user, uid)
  const char *user;
  uid_t *uid;
{
  int rv;
  struct passwd *pwd;

  if (isdigit((int)*user) || (*user == '-' && isdigit((int)*user)))
  {
    *uid = atoi(user);
    rv = 1;
  }
  else if (isalpha((int)*user))
  {
    if ( ! (pwd = getpwnam(user)))
    {
      rv = 0;
    }
    else
    {
      *uid = pwd->pw_uid;
      rv = 1;
    }
  }
  else
  {
    rv = 0;
  }

  return rv;
}


static void child_wait(sig)
  int sig;
{
  while (waitpid(-1, (int *)0, WNOHANG) > 0)
  { continue; }
}


static void log_reset(sig)
  int sig;
{
  reset_logfile = 1;
}


static void usage()
{
#ifndef NO_TCL
  efprintf(stderr, "Usage: %s [-config <tcl-script>] [-debug] [-log <log-file>] "
           "[-pdebug <#>] [-phost <host>] [-port <#>] [-proot <prosp-root>] "
           "[-stay] [-user <user>]\n",
           prog);
#else
  efprintf(stderr, "Usage: %s [-debug] [-log <log-file>] "
           "[-pdebug <#>] [-phost <host>] [-port <#>] [-proot <prosp-root>] "
           "[-stay] [-user <user>]\n",
           prog);
#endif
  exit(1);
}
