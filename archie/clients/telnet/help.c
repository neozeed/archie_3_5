#ifdef __STDC__
#  include <stdlib.h> /* for free() */
#endif
#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <ctype.h>

/* Including "signals.h" instead... */
#if defined(AIX) || defined(SOLARIS)
#include <signal.h>
#endif

#include "alarm.h"
#include "client_defs.h"
#include "debug.h"
#include "defines.h"
#include "error.h"
#include "extern.h"
#include "help.h"
#include "input.h"
#include "lang.h"
#include "macros.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "mode.h"
#include "pager.h"
#include "signals.h"
#include "vars.h"

#include "protos.h"

/*  
 *  If the first character of the name is a letter it _may_ be a topic.
 */  

#define MAX_SUBTOPICS 128
#define MAXCOLS 3
#define is_topic_name(f) (isascii((int)*f) && isalpha((int)*f))


static char curr_path[MAX_PATH_LEN];
static char curr_prompt[MAXPATHLEN];
static int help_ctxt = -1;
static jmp_buf here_on_intr;


/*
 *
 *                             Internal Routines
 *
 *
 */


static char *get_prompt PROTO((void));
static int get_subtopics PROTO((char **stlist, int stelts));
static int display_help PROTO((void));
static int help_ PROTO((const char *t, int immediate_return));
static int is_topic PROTO((const char *t));
static int list_subtopics PROTO((void));
static int pop_topic PROTO((void));
static int push_topics PROTO((const char *s));
static int strptrcmp PROTO((char **p1, char **p2));
static void catch_help_sigint PROTO((void));
static void handle_help_sigint PROTO((int sig));
static void ignore_sigint PROTO((void));


/*  
 *  Create a string suitable for display as a help prompt, and return a
 *  pointer to it.
 */  

static char *get_prompt()
{
  static char p[MAXPATHLEN+4]; /*bug: inefficient*/

  sprintf(p, curr_lang[122], curr_prompt);
  return p;
}


/*  
 *  Signal handler for SIGINT.  Returns to help().
 */  

static void handle_help_sigint(int sig)
{
  d5fprintf(stderr, "%s (%ld): handle_help_sigint: longjmp()ing.\n",
            prog, (long)getpid());
  longjmp(here_on_intr, 1);
}


/*  
 *  Set-up the signal handler.
 */  

static void catch_help_sigint()
{
  d5fprintf(stderr, "%s (%ld): catch_help_sigint: setting SIGINT handler.\n",
            prog, (long)getpid());
  ppc_signal(SIGINT, handle_help_sigint);
}


/*  
 *  Invoke the pager, if necessary, on the file containing help.
 */  

static int display_help()
{
  char fname[MAXPATHLEN];

  sprintf(fname, curr_lang[123], curr_path);
  if (access(fname, R_OK) == -1)
  {
    error(A_SYSERR, curr_lang[124], curr_lang[125], fname);
    return 0;
  }
  if (is_set(V_PAGER))
  {
    int ret;

    ignore_sigint(); /* so ^C doesn't longjmp() us past the wait()... */
    ret = pager(fname);
    catch_help_sigint();
    return ret;
  }
  else
  {
    return fprint_file(stdout, fname, 0);
  }
}


/*  
 *  Disable trapping of SIGINT.
 */  

static void ignore_sigint()
{
  d5fprintf(stderr, "%s (%ld): ignore_sigint: ignoring SIGINT.\n",
            prog, (long)getpid());
  ppc_signal(SIGINT, SIG_IGN);
}


/*  
 *  Given the path name of a file, return 1 or 0 depending on whether it is a
 *  help topic (in reality, whether it is a directory).
 */  

static int is_topic(t)
  const char *t;
{
  struct stat sb;

  if (stat(t, &sb) == -1)
  {
    return 0;
  }
  return S_ISDIR(sb.st_mode);
}


/*  
 *  Using the current path (in the variable `curr_path') print a list of
 *  subtopics.  (Basically, it prints a list of the subdirectories under the
 *  current directory.)
 *  
 *  Print the list in three columns, assuming a maximun topic string length
 *  of twenty characters.
 */    

static int list_subtopics()
{
  char *subtops[MAX_SUBTOPICS];
  int i;
  int ncols = MAXCOLS;
  int nelts;
  int nrows;

  if ((nelts = get_subtopics(&subtops[0], MAX_SUBTOPICS)) == 0)
  {
    printf(curr_lang[130]);
    return 0;
  }

  qsort((char *)&subtops[0], nelts, sizeof subtops[0], strptrcmp);

  nrows = nelts / ncols;
  if (nelts % ncols) nrows++;   /* i.e. nrows = ceil(nelts / (double)ncols) */

  printf(curr_lang[128]);
  for (i = 0; i < nrows; i++)
  {
    int j;

    printf("#   %-20s", subtops[i]);
    for (j = 1; j < ncols; j++)
    {
      int idx = i + j * nrows;

      if (idx < nelts) printf("     %-20s", subtops[idx]);
    }
    printf("\n");
  }
  printf("#\n");

  for (i = 0; i < nelts; i++)
  {
    if (subtops[i]) free(subtops[i]);
  }

  return 1;
}


/*  
 *  Return the number of subtopics under the current topic.  (I.e. Count the
 *  number of entries, in the current directory, that correspond to
 *  subtopics.)
 */      
static int get_subtopics(stlist, stelts)
  char **stlist;
  int stelts;
{
  DIR *dir;
  int n = 0;
  struct dirent *d;

  if ( ! (dir = opendir(curr_path)))
  {
    error(A_ERR, "get_subtopics", curr_lang[127], curr_path);
    return 0;
  }
  
  while ((d = readdir(dir)))
  {
    if (is_topic_name(d->d_name))
    {
      char fname[MAXPATHLEN+1];

      sprintf(fname, curr_lang[22], curr_path, d->d_name);
      if (is_topic(fname))
      {
        if (stlist && n < stelts) /* append a topic */
        {
          /* bug: strdup() could fail */
          stlist[n] = strdup(d->d_name);
        }
        n++;
      }
    }
  }

  closedir(dir);
  return n;
}


/*  
 *  Pop a topic from the stack (of topics).  This translates to removing the
 *  last element from the prompt and the current directory strings.
 *  Underflow causes us to return 0, otherwise 1.
 *  
 *  Underflow should be interpreted as "popping out of help" (i.e. returning
 *  to the main prompt).
 */  

static int pop_topic()
{
  char *new_end;

  if ( ! (new_end = strrchr(curr_prompt, ' ')))
  {
    return 0;
  }
  else
  {
    *new_end = '\0';
    if ( ! (new_end = strrchr(curr_path, '/')))
    {
      error(A_INTERR, curr_lang[131], curr_lang[132]);
      return 0;
    }
    else
    {
      *new_end = '\0';
      return 1;
    }
  }
}


/*  
 *  Push zero or more topics on the stack.
 *  
 *  This translates to appending the appropriate subdirectories to the
 *  current directory list.  We check to see whether the resulting help topic
 *  is valid (by looking for the file <curr_dir>/=); if it isn't we restore
 *  the current directory list and return 0; otherwise we append the
 *  subtopics to the prompt and return 1.
 *  
 *  The topics are represented as a string of whitespace separated words.
 */  

static int push_topics(s)
  const char *s;
{
  char *epath;
  struct stat sb;

  ptr_check(s, const char, curr_lang[133], 0);

  if ( ! *s)
  {
    return 1;
  }

  epath = curr_path + strlen(curr_path);

  strcat(curr_path, curr_lang[98]);
  strcat(curr_path, s);
  strxlate(epath, ' ', '/');
  if (stat(curr_path, &sb) == -1)
  {
    *epath = '\0';
    printf(curr_lang[134], s);
    return 0;
  }
  else
  {
    strcat(curr_prompt, curr_lang[57]);
    strcat(curr_prompt, s);
    return 1;
  }
}


static int strptrcmp(p1, p2)
  char **p1;
  char **p2;
{
  return strcmp(*p1, *p2);
}


#define m(x) /*fprintf(stderr, "At %d.\n", x);*/


/*  
 *  The argument is assumed to be a whitespace separated sequence of zero or
 *  more words, which is interpreted as commands to the help interpreter.
 *  
 *  A dot (`.') tells us to pop up one level in the help hierarchy.  If we
 *  are already at the bottom most level, the pop will fail, telling us to
 *  return from help.
 *  
 *  A question mark (`?') tells us to list all subtopics of the current
 *  topic.
 *  
 *  Any line consisting only of whitespace (including newline) is ignored,
 *  and the prompt is redisplayed.
 *  
 *  All other lines are assumed to be one or more successively refined help
 *  topics.  If valid, they are pushed on the prompt and directory stacks,
 *  and the file containing the actual help text is displayed.  If this topic
 *  has no subtopics we pop back to the previous help level.
 *  
 *  Now, we accept another line from the user, and repeat the cycle.
 */  

static int help_(t, immediate_return)
  const char *t;
  int immediate_return;
{
  char *p;
  char topics[MAXPATHLEN + 8];

  curr_prompt[0] = '\0';
  strcpy(topics, t);
  squeeze_whitespace(topics);

  /*
    Special case: if no topic is specified display the "info on help"
    file (the file `=' under the base help directory).
  */

  if ( ! *topics)
  {
    set_alarm();
    if ( ! display_help())
    {
      error(A_INTERR, curr_lang[135], curr_lang[136]);
      return 0;
    }
  }

  while(1)
  {
    if (strcmp(topics, curr_lang[19]) == 0)
    {
      if ( ! pop_topic())
      {
        return 1;
      }
    }
    else if (strcmp(topics, curr_lang[137]) == 0)
    {
      list_subtopics();
    }
    else if (strcmp(topics, curr_lang[138]) == 0)
    {
      return 1;
    }
    else if (*topics && push_topics(topics))
    {
      set_alarm();
      display_help();
      if (get_subtopics((char **)0, 0) == 0)
      {
        pop_topic();
      }
    }
    set_alarm();
    if ( ! immediate_return && (p = readline(get_prompt())))
    {
      strcpy(topics, p);
      squeeze_whitespace(topics);
      if ( ! *topics)
      {
        if ( ! pop_topic())
        {
          return 1;               /* exit on empty line */
        }
      }
      free(p);
    }
    else
    {
      clearerr(stdin);
      putchar('\n');
      return 1;
    }
  }
}


/*
 *
 *                             External Routines
 *
 *
 */

/*  
 *  Set things up so we can call help_().
 *  
 *  Copy the relative path of the help directory into the current directory
 *  variable (`curr_var').
 *  
 *  If we haven't already done so, get a new history context for help.  (For
 *  this to have any effect the command line editing library must be linked
 *  in.)
 *  
 *  Set up a place we can longjmp() to if the user generates a SIGINT
 *  (usually ^C).
 *  
 *  Catch SIGINT, save the old history context and call help_().
 *  
 *  Upon return from help_() we just clean up a few things: ignore SIGINT,
 *  and restore the old history context.
 */  

int help(ac, av)
  int ac;
  char **av;
{
  char old_pager_opts[MAX_VAR_STR_LEN];
  int old_ctxt;
  int whence = 0;

  strcpy(curr_path, get_var(V_HELP_DIR));
  strcpy(old_pager_opts, get_var(V_PAGER_OPTS));
  set_var(V_PAGER_OPTS, get_var(V_PAGER_HELP_OPTS));

  if (current_mode() == M_EMAIL && ac == 1)
  {
    if (fprint_file(stdout, get_var(V_EMAIL_HELP_FILE), 0))
    {
      return 1;
    }
    else
    {
      error(A_INTERR, curr_lang[32], curr_lang[139]);
      return 0;
    }
  }

  if (help_ctxt == -1 && set_hist_context(new_hist_context()) == -1)
  {
    error(A_INTERR, curr_lang[32], curr_lang[140]);
  }
  else
  {
    help_ctxt = 0; /* use the same one as non-help */
  }

  if ((whence = setjmp(here_on_intr)) == 0)
  {
    char topic[256];
    int i;

    topic[0] = topic[1] = '\0'; /*bug: ack!*/
    for(i = 1; i < ac; i++)
    {
      strcat(topic, curr_lang[57]); /*bug: kludge*/
      strcat(topic, av[i]);
    }
      
    catch_help_sigint();
    old_ctxt = set_hist_context(help_ctxt);

    help_(topic + 1, current_mode() == M_EMAIL);
  }

  ignore_sigint(); /* we return here on ^C */

  set_var(V_PAGER_OPTS, old_pager_opts);
  set_hist_context(old_ctxt);

  if (whence)
  {
    putchar('\n'); /* it just looks nicer with this... */
  }
  return 1;
}
