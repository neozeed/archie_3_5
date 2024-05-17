/*  
 *  
 *  TELNET ARCHIE
 *  
 *  Program to provide telnet interface to the archie system.
 *  
 *  History:
 *
 *    - Oct./92	Modified by Peter Deutsch (peterd@bunyip.com):
 *  
 *  Changes:
 *
 *    - added DOMAINS command (bajan Tue Aug 17 00:42:03 EDT 1993)
 *    - modified FIND to use Prospero interface
 *    - added FIND alias for "PROG" command
 *    - modified LIST to use Prospero interface
 *    - dropped support for SITE command
 *    - Replace SERVERS command
 *    - added MOTD command
 *    - redid HELP subsytem to use recursive code and help tree.
 *    - added support for ".archierc" file to allow specifying server, help
 *      directory, etc (see sample .archierc file for details...)
 *    - added support for logfile (-l, -L) options -- bajan Sat Feb 27 21:58:25 EST 1993
 *  
 *  To come (once Prospero 5.0 ready):
 *
 *    - add multple database capability
 *    - In database <cmd>
 *    - set database = <string>
 *    - add path spec to anonFTP
 *    - add pseudo-domain support
 *    - add support for template model (WHOIS)
 *  
 *  - Original author: Bill Heelan ( wheelan@cs.mcgill.ca )
 *  
 *  
 *  Bugs fixed:
 *  
 *    - no longer tries to chroot() in server mode if euid != 0.
 *      (Wed Jan 27 22:28:53 EST 1993)
 *    - no longer core dumps on "set term", etc. (i.e. numeric or string
 *      variables without arguments.
 *      (Fri Jan 29 01:21:30 EST 1993)
 *  
 *  
 *  To do for production:
 *  
 *    - stty (DONE)
 *    - getenv("TERM") for smart telnets (DONE)
 *    - if no commands in e-mail mode print help on exit (DONE)
 *    - new command: disable-command <cmd> ... # sets mode to M_NONE (DONE)
 *  
 *  To think about:
 *  
 *    - rewrite 1/2 :-)
 *    - put function pointers in command list & make all front-end functions
 *      take the same arguments.
 *    - do pager outside of front-end functions.
 *    - remove_tmp on login?!  Look into it.
 *    - hack less to take fd argument so we can unlink tmp files right away.
 */  

#include <stdio.h>
#include <signal.h>
#ifdef RUTGERS
#include <sys/wait.h>
#endif
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <pwd.h>
#include <errno.h>
#ifdef __STDC__
#  include <stdlib.h> /* for free() */
#  include <unistd.h>
#endif
#include <netdb.h>
#include "alarm.h"
#include "archie.h"
#include "archie_inet.h"
#include "argv.h"
#include "client_defs.h"
#include "commands.h"
/*#include "database.h"*/
#include "debug.h"
#include "defines.h"
#include "domains.h"
#include "error.h"
#include "files.h" /* for tail() in library */
#include "find.h"
#include "fork_wait.h"
#include "generic_find.h"
#include "help.h"
#include "input.h"
#include "lang.h"
#include "list.h"
#include "macros.h"
#include "mail.h"
#include "master.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "mode.h"
#include "pager.h"
#include "prosp.h"
#include "signals.h"
#include "style_lang.h" /*bug: display() should have separate routines*/
#include "terminal.h"
#include "unixcompat.h"
#include "vars.h"
#include "version.h"
#include "whatis.h"

#include "protos.h"

#define ARCHIERC ".archierc"
#define SYSARCHIERC "/usr/lib/.archierc"
#define USERNAME "archie"

#define PRI_LOW    -100
#define PRI_HIGH    100
#define PRI_NORMAL    0

#define NEXTARG if ( ! (av++, --ac)) usage()
#define NSETARG(var) NEXTARG; var = atoi(av[0])
#define SETARG(var) NEXTARG; var = av[0]


static FILE *create_tmp_file PROTO((int email));
static int compress_it PROTO((int ac, char **av));
static int cvt_priority PROTO((int our_pri));
static int disable_command PROTO((int ac, char **av));
static int do_motd PROTO((int ac, char **av, FILE *ofp));
#ifdef MULTIPLE_COLLECTIONS
static int in_it PROTO((int ac, char **av, FILE *ofp));
#endif
static int init_from_file PROTO((const char *file, FILE *ofp, int mode));
static int list_servers PROTO((int ac, char **av, FILE *ofp));
static int man_it PROTO((int ac, char **av, FILE *ofp));
static int set_path PROTO((int ac, char **av));
static void command_loop PROTO((FILE *ifp, FILE *ofp, int mode));
static void display PROTO((FILE *ifp, FILE *ofp));
static void usage PROTO((void));


char homedir[MAXPATHLEN];
char rc_path[MAXPATHLEN];
const char *prog;
int debug = 0;


int main(ac, av)
  int ac;
  char **av;
{
  FILE *ofp;
  const char *hd;               /* home directory */
  const char *logfile = 0;
  pathname_t querylog;
  int email = 0;
  int hctxt;
  int logging = 0;
  int priority = PRI_NORMAL;
  int server = 0;

  curr_lang = english;
  prog = tail(av[0]);
  server = *prog == '-';
  strcpy(rc_path, SYSARCHIERC);

#if defined(VFPROT_VNO) && VFPROT_VNO >= 5
  p_initialize("ggB", 0, (struct p_initialize_st *)0);
#endif

  DBG(uids("pre-initial revoke: "););
  if ( ! revoke_root())
  {
    error(A_SYSERR, "main", "error revoking privileges");
    exit(1);
  }
  DBG(uids("post-initial revoke: "););

  while (av++, --ac)
  {
    if (av[0][0] != '-')
    {
      error(A_ERR, curr_lang[5], curr_lang[6], av[0]);
      usage();
    }
    else
    {
      switch (av[0][1])
      {
      case 'L':
        SETARG(logfile);
        logging = 1;
        break;

      case 'd':
        NEXTARG;
        set_var(V_DEBUG, av[0]); /* bug: arg. check */
        break;
        
      case 'e':
        email = server = 1;
        break;

      case 'i':
        NEXTARG;
        strcpy(rc_path, av[0]);
        break;

      case 'l':
        logging = 1;
        break;

      case 'o':                 /* not used anymore; backward compat. */
        NEXTARG;
        break;
        
      case 'p':
        NSETARG(priority);
        break;
        
      case 's':
        server = 1;
        break;

      default:
        error(A_ERR, curr_lang[5], curr_lang[6], av[0]);
        usage();
      }
    }
  }
  
#define DEFAULT_QUERY_LOG_FILE "query.log"
  
  logging = 1;
  if (logging)
  {
    if(logfile == NULL || logfile[0] == '\0'){
      sprintf(querylog, "%s/%s/%s", get_archie_home(), DEFAULT_LOG_DIR, DEFAULT_QUERY_LOG_FILE);
      logfile = querylog;
    }
      
    if (open_alog(logfile, A_INFO, prog) == ERROR)
    {
      exit(ERROR);
    }
  }

#ifdef TESTING
  putenv("PAGER=");
  putenv("LESS=");
#endif
#ifdef LATIN1
  putenv("LESSCHARSET=latin1");
#endif  

  ardp_priority = cvt_priority(priority);


  /*  
   *  Special requirements for server mode.
   */  
  if (server)
  {
    const char *path = "PATH=bin:pager/bin";

    putenv(path);

    if ((hd = get_home_dir_by_name(USERNAME)))
    {
      strcpy(homedir, hd);
    }
    else
    {
      error(A_INTERR, curr_lang[5], curr_lang[17]);
      exit(1);
    }
    if (chdir(homedir) == -1)
    {
      error(A_SYSERR, curr_lang[5], curr_lang[18], homedir);
      exit(1);
    }
    /*  
     *  Look for .archierc.
     */  
    init_from_file(ARCHIERC, stdout, set_mode(M_SYS_RC));

#ifdef RUTGERS
    /*  
     *  Due a bug at Rutgers we will have, upon startup, a child
     *  that must be disposed of.
     */  
    while (wait((int *)0) > 0);
#endif  
  }
  else                          /* Non-server mode: look for system and user rc files */
  {
    /*  
     *  First look for a system initialization file.
     */  
    init_from_file(rc_path, stdout, set_mode(M_SYS_RC));

    /*  
     *  Next look in the user's home directory (which would be archie's in
     *  server mode).
     */  
    if ((hd = get_home_dir_by_uid(getuid())))
    {
      sprintf(rc_path, curr_lang[22], hd, ARCHIERC);
      init_from_file(rc_path, stdout, set_mode(M_USER_RC));
    }
  }

  if ( ! (ofp = create_tmp_file(email)))
  {
    error(A_ERR, "main", "can't create temporary file -- exiting.\n"); /*FFF*/
    exit(1);
  }

  umask(077);
  nice(atoi(get_var(V_NICENESS)));
  set_mode(email ? M_EMAIL : M_INTERACTIVE);

  /*  
   *  Set up to do command-line editing (if it's linked in...)
   */  
  if ((hctxt = new_hist_context()) != -1)
  {
    set_hist_context(hctxt);
  }
  else
  {
    error(A_INTERR, curr_lang[5], curr_lang[23]);
    exit(1);                    /*bug? switch to non-editing input?*/
  }


#ifdef UQAM
  printf(curr_lang[24]);
#endif

  if (current_mode() != M_EMAIL)
  {
    const char *s = "stty";
    static const char *args[] = { "show", "search", 0 };

    printf(curr_lang[26]);
    init_term();
    set_tty(1, (char **)&s);    /*bug: should fix to do this in .archierc*/
    show_it((int)(sizeof args / sizeof (char *) - 1), (char **)args);
  }

  parent_sigs();
  set_alarm();                  /* boots user off for idling */

  command_loop(stdin            /* used in batch + email mode */,
               ofp, current_mode());

  fclose(ofp);
  exit(0);
  return 0;                     /* keep gcc quiet */
}


static void command_loop(ifp, ofp, mode)
  FILE *ifp; /* currently only used in batch & e-mail modes */
  FILE *ofp;
  int mode;
{
  char **av;
  char ocommand[INPUT_LINE_LEN];
  char *command;
  int ac;
  int cmd_modes;
  int cmd_token;
  int seen_a_command = 0;

  ocommand[0] = '\0';

  while (1)
  {
    char c[INPUT_LINE_LEN]; /*bug: kludge - fix call to readline*/
    int show_output = 0;

    if (mode == M_INTERACTIVE)
    {
      command = readline(get_var(V_PROMPT));
    }
    else
    {
      if ((command = fgets(c, sizeof c, ifp))) nuke_newline(command);
    }

    if ( ! command)
    {
      if (mode == M_INTERACTIVE) fputs(curr_lang[28], stdout); /* looks nicer */
      cmd_token = BYE;
    }
    else
    {
      unset_alarm();
      cmd_token = get_cmd(command, &ac, &av, &cmd_modes);

      /* Is the command valid in the current mode? */
      if (cmd_modes & mode)
      {
        /* Don't add empty lines to the command history. */
        if (cmd_token != EMPTY) add_history(command);
      }
      else
      {
        if (cmd_modes == M_NONE)
        {
          printf(curr_lang[29]);
        }
        else
        {
          if (av) printf(curr_lang[30], mode_str(mode), av[0]);
          else printf(curr_lang[39], command, mode_str(mode));
        }
        continue;
      }
    }

    /*  
     *  Echo commands in email mode, so the user can distinguish the
     *  different parts of the output.
     */  
    if (mode == M_EMAIL && command && /* bug: ugly */
        cmd_token != BAD && cmd_token != EMPTY &&
        cmd_token != BYE && cmd_token != EXIT && cmd_token != QUIT)
    {
      printf(curr_lang[31], command);
    }

    switch (cmd_token)
    {
    case BYE:
    case EXIT:
    case QUIT:
      /*  
       *  If in e-mail mode and there is stuff to mail back...
       */  
      if (mode == M_INTERACTIVE)
      {
        printf(curr_lang[34]);
      }
      else if (mode == M_EMAIL)
      {
        if ( ! seen_a_command)
        {
          const char *h = "help";
          help(1, (char **)&h);
          mail_it(1, (char **)&h, h, ofp);
        }
        else if ( ! fempty(ofp))
        {
          const char *s = "mail";
          
          mail_it(1, (char **)&s, ocommand, ofp);
        }
      }
      return;

    case COMMENT:
    case EMPTY:		/* input was an empty line */
      break;

    case COMPRESS:
      compress_it(ac, av);
      break;

    case DISABLE:
      disable_command(ac, av);
      break;

    case DOMAINS:
      show_output = domains_it(ac, av, ofp);
      strcpy(ocommand, command);
      seen_a_command = 1;
      break;

    case DONE:
      printf(curr_lang[35]);
      break;

    case FIND:
    case PROG:			/* find a search string */
      show_output = find_it(ac, av, ofp);
      strcpy(ocommand, command);
      seen_a_command = 1;
      break;

    case HELP:
      help(ac, av);
      seen_a_command = 1;
      if (mode == M_EMAIL)
      {
        /*  
         *  if the only command in e-mail mode is "help", mail_it() won't send
         *  anything unless it sees a value for the old command.
         */  
        strcpy(ocommand, command);
      }
      break;

#ifdef MULTIPLE_COLLECTIONS
    case IN:
      seen_a_command = in_it(ac, av, ofp) && ac > 2; /*bug: kludge*/
      strcpy(ocommand, command);
      break;
#endif
      
    case LIST:			/* list the sites we know about */
      show_output = list_it(ac, av, ofp);
      strcpy(ocommand, command);
      seen_a_command = 1;
      break;

    case MAIL:
      mail_it(ac, av, ocommand, ofp);
      if (current_mode() == M_EMAIL)
      {
        truncate_fp(ofp);
      }
      else
      {
        rewind_fp(ofp);
      }
      break;

    case MANPAGE:
      show_output = man_it(ac, av, ofp);
      strcpy(ocommand, command);
      seen_a_command = 1;
      break;

    case MOTD:
      show_output = do_motd(ac, av, ofp);
      strcpy(ocommand, command);
      seen_a_command = 1;
      break;

    case NON_UNIQUE:
      printf(curr_lang[36]);
      break;

    case NOPAGER:
      unset_var(V_PAGER);
      break;

    case PAGER:
      set_var(V_PAGER, (char *)0);
      break;

    case PATH:
      set_path(ac, av); /*bug: error reporting? */
      break;

    case SERVERS:
      show_output = list_servers(ac, av, ofp);
      strcpy(ocommand, command);
      seen_a_command = 1;
      break;

    case SET:
      set_it(ac, av);
      break;

    case SHOW:
      show_it(ac, av);
      seen_a_command = 1;
      break;

    case SITE:
      printf(curr_lang[37]);
      seen_a_command = 1;
      break;

    case STTY:
      set_tty(ac, av);
      break;
      
    case TERM:
      /* Tell us what kind of terminal to use for the pager. */
      install_term(ac, av);
      break;

    case UNSET:
      /* Unset a variable that has been set with the 'set' command. */
      unset_it(ac, av);
      break;

    case VERSION:
      printf(curr_lang[38], version);
      seen_a_command = 1;
      break;

    case WHATIS:
      show_output = whatis_it(ac, av, ofp);
      strcpy(ocommand, command);
      seen_a_command = 1;
      break;

    default:
      /* So signatures don't produce error messages. (bug? I dunno...) */
      if (mode != M_EMAIL) printf(curr_lang[39], command, mode_str(mode));
      break;
    }

    if (av) argvfree(av);
    if (mode == M_INTERACTIVE) free(command);
    d1fprintf(stdout, "show_output is %d\n", show_output); fflush(ofp);
    if (show_output) display(ofp, stdout);
    set_alarm();
  }
}


static int compress_it(ac, av)
  int ac;
  char **av;
{
  char **argv;
  int argc;

  if (ac != 1)
  {
    printf(curr_lang[315], av[0]);
    return 0;
  }

  argvify("set compress compress", &argc, &argv);
  set_it(argc, argv);
  argvfree(argv);
  return 1;
}


/*  
 *  Scale our priority into the range of Prospero's, keeping in mind that
 *  ARDP_MAX_PRI is the _lowest_ priority, while ARDP_MIN_PRI is the
 *  _highest_.
 */  
static int cvt_priority(our_pri)
  int our_pri;
{
  float pri;
  
  if (our_pri > PRI_HIGH) our_pri = PRI_HIGH;
  else if (our_pri < PRI_LOW) our_pri = PRI_LOW;

  pri = (float)(our_pri - PRI_LOW) / (PRI_HIGH - PRI_LOW);
  return (int)(-pri * (ARDP_MAX_PRI - ARDP_MIN_PRI) + ARDP_MAX_PRI);
}


/*  
 *  Create and unlink a temporary file for output (mail and paging).
 *  
 *  In email mode redirect stdout and stderr to it.
 */  
static FILE *create_tmp_file(email)
  int email;
{
  FILE *fp;
  char *f;
  int fd;

  if ( ! (f = tempnam(get_var(V_TMPDIR), (char *)0)))
  {
    error(A_SYSERR, "create_tmp_file", "can't create tmp file"); /* FFF */
    return 0;
  }

  /*  
   *  I suspect SunOS (4.1.3 at least) doesn't handle append mode correctly
   *  in the presence of multiple processes.
   */  
  if ((fd = open(f, O_RDWR | O_APPEND | O_CREAT | O_EXCL, 0600)) == -1)
  {
    fp = 0;
    error(A_SYSERR, "create_tmp_file", "can't open `%s'", f);
  }
  else if ( ! (fp = fdopen(fd, "a+")))
  {
    fp = 0;
    error(A_ERR, "create_tmp_file", "can't fdopen() fd %d (`%s').", fd, f);
  }
  else if (email)
  {
    /* bug? legal? portable? */
    fflush(stdout);
    fflush(stderr);
    dup2(fd, fileno(stdout));
    dup2(fd, fileno(stderr));

    /*  
     *  bug: an alternative to this is to put flushes in fork_me().
     */  
    setvbuf(fp, (char *)0, _IOLBF, 0); /* so output doesn't get mixed */
    setvbuf(stdout, (char *)0, _IOLBF, 0);
    setvbuf(stderr, (char *)0, _IOLBF, 0);
  }

  d1fprintf(stderr, "%s: create_tmp_file: unlink()ing `%s'.\n", prog, f);
  unlink(f);
  free(f);
  return fp;
}


static int disable_command(ac, av)
  int ac;
  char **av;
{
  if (ac < 2)
  {
    error(A_ERR, curr_lang[40], curr_lang[41], av[0]);
    return 0;
  }
  else
  {
    while (av++, --ac)
    {
      cmd_disable(av[0]);
    }
  }
  return 1;
}


/*  
 *  Print the Prospero `message of the day' for the current server.
 */  
static int do_motd(ac, av, ofp)
  int ac;
  char **av;
  FILE *ofp;
{
  int ret = 0;
  
  if (ac > 1)
  {
    printf(curr_lang[315], av[0]);
  }
  else
  {
    char *motd;

    if ( ! (motd = get_var(V_MOTD_FILE)))
    {
      ret = 1;                  /* succeed if there's no MOTD to print */
    }
    else
    {
      mode_truncate_fp(ofp);
      ret = fprint_file(ofp, motd, 1);
    }
  }

#if 1
  return 0; /* return value indicates whether we should print the output of command? */
#else
  return ret;
#endif
}


#ifdef MULTIPLE_COLLECTIONS
/*  
 *  in db1 [[db2 ...] find str1 [str2 ...]]
 */  
static int in_it(ac, av, ofp)
  int ac;
  char **av;
  FILE *ofp;
{
  int pos;
  
  if (ac < 2)
  {
    printf(curr_lang[317], av[0]);
    return 0;
  }
  else if ((pos = argvFindStrCase(ac, av, 1, ac-1, "find")) < 0) /*bug: french*/
  {
    char *s = argvflatten(ac, av, 1);
    int rv = set_var(V_COLLECTIONS, s);
    free(s);
    return rv;
  }
  else
  {
    if (pos == 1) /* error: no databases specified */
    {
      printf("# One or more databases must be specified.\n"); /*FFF*/
      return 0;
    }
    else if (pos == ac - 1) /* error: no search strings */
    {
      printf("# One or more search strings must be specified.\n"); /*FFF*/
      return 0;
    }
    else
    {
      char **dbs = &av[1];
      char **srchs = &av[pos+1];
      int ndbs = pos - 1;
      int nsrchs = ac - pos - 1;

      /* check for anonymous ftp */
      if (argvFindStrCase(ac, av, 1, pos-1, "anonftp") > 0) /*bug: french*/
      {
        /* only "anonftp" specified? */
        if (ndbs > 1)
        {
          printf("# If `%s' is specified, no other databases may appear in the list.\n",
                 "anonftp"); /*FFF*/
          return 0;
        }
        else
        {
          return anonftp_find(nsrchs+1, &av[pos], ofp);
        }
      }
      else /* do a generic search */
      {
        int maxhdrs = atoi(get_var(V_MAXHITS));
        int maxdocs = 5;
        
        return generic_find(maxhdrs, maxdocs, ndbs, dbs, nsrchs, srchs, ofp);
      }
    }
  }
}
#endif


static int init_from_file(rcfile, ofp, mode)
  const char *rcfile;
  FILE *ofp;
  int mode;
{
  FILE *rcfp;

  ptr_check(rcfile, const char, curr_lang[43], 0);
  
  if ( ! (rcfp = fopen(rcfile, curr_lang[44])))
  {
    return 0;
  }
  else
  {
    command_loop(rcfp, ofp, mode);
    fclose(rcfp);
    return 1;
  }
}


static int man_it(ac, av, ofp)
  int ac;
  char **av;
  FILE *ofp;
{   
  int ret = 0;
  
  ptr_check(av, char *, curr_lang[45], 0);
  ptr_check(ofp, FILE, curr_lang[45], 0);

  if (ac > 2)
  {
    printf(curr_lang[156], av[0]);
    return 0;
  }

  mode_truncate_fp(ofp);
  switch (fork_me(0, &ret))
  {
  case (enum forkme_e)INTERNAL_ERROR:
    break;

  case CHILD:
    {
      const char *man = get_var(V_MAN_ASCII_FILE);

      if (ac == 2 && strstr(av[1], curr_lang[47]))
      {
        man = get_var(V_MAN_ROFF_FILE);
      }
      ret = fprint_file(ofp, man, 0);
    }
    fork_return(ret);
    break;

  case PARENT:
    set_alarm();
    break;

  default:
    error(A_INTERR, curr_lang[45], curr_lang[48]);
    break;
  }

  return ret;
}


static int set_path(ac, av)
  int ac;
  char **av;
{
  if (ac < 2)
  {
    printf(curr_lang[203], av[0]);
    return 0;
  }
  else
  {
    char *maddr;
    int rv;

    maddr = argvflatten(ac, av, 1);
    if ( ! maddr)
    {
      printf(curr_lang[319]);
      rv = 0;
    }
    else
    {
      rv = set_var(V_MAILTO, maddr);
      free(maddr);
    }
    return rv;
  }
}

static void display(ifp, ofp)
  FILE *ifp;
  FILE *ofp;
{
  if (current_mode() != M_EMAIL)
  {
    int ostyle;
  
    strtoval(get_var(V_OUTPUT_FORMAT), &ostyle, style_list);
    if (ostyle != SILENT) 
    {
      if (is_set(V_PAGER))
      {
        pager_fp(ifp);
      }
      else
      {
        fprint_fp(ofp, ifp);
      }
    }
  }
}


static int list_servers(ac, av, ofp)
  int ac;
  char **av;
  FILE *ofp;
{
  const char *slist;

  mode_truncate_fp(ofp);
  if ((slist = get_var(V_SERVERS_FILE)))
  {
    return fprint_file(ofp, slist, 0);
  }
  else
  {
    printf(curr_lang[63]);
    return 0;
  }
}


static void usage()
{
  fprintf(stderr, curr_lang[64], prog);
  exit(1);
}
