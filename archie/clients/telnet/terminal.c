#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifndef AIX
#include <sys/termios.h>
#else
#include <termio.h>
#endif
#ifdef __STDC__
#  include <stdlib.h>
#  ifndef TIOCGWINSZ
#    include <sys/ioctl.h>
#  endif
#endif
#include "client_defs.h"
#include "debug.h"
#include "error.h"
#include "extern.h"
#include "macros.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "terminal.h"
#include "vars.h"
#include "lang.h"

#include "protos.h"

static int cols = 80;
static int rows = 24;


/*  
 *  
 *                                Internal routines
 *  
 */  


static const char *set_term_type PROTO((const char *term_name));
static int set_window_size PROTO((int r, int c));
static char *get_key_str PROTO((unsigned int ch, char *buf, int bufsize));
static char get_key_char PROTO((char *key));
  

static char get_key_char(key)
  char *key;
{
  int len;

  ptr_check(key, char, curr_lang[249], 0);

  dequote(key);
  if ((len = strlen(key)) == 0 || len > 2 || (len == 2 && *key != '^'))
  {
    printf(curr_lang[250], key);
    return 0;
  }

  if (len == 1)
  {
    return *key;
  }
  else
  {
    if (key[1] == '?') /* special case: DEL */
    {
      return '\177';
    }
    else
    {
      int c = CNTRL(toupper(key[1]));

      if (isascii(c) && iscntrl(c) && c != 0)
      {
        return c;
      }
      else
      {
        printf(curr_lang[251], key, c);
        return 0;
      }
    }
  }
}


static char *get_key_str(ch, buf, bufsize)
  unsigned int ch;
  char *buf;
  int bufsize;
{
  static char bad[] = "<unknown>";

  if (bufsize < 5)
  {
    error(A_INTERR, curr_lang[252], curr_lang[253], curr_lang[254]);
    return bad;
  }
  else if (ch > 255)
  {
    error(A_INTERR, curr_lang[252], curr_lang[255], curr_lang[256], (unsigned long)ch);
    return bad;
  }
  else
  {
    if (iscntrl(ch))
    {
      if (ch == '\177') strcpy(buf, curr_lang[257]); /* delete */
      else sprintf(buf, curr_lang[258], LRTNC(ch));
    }
    else if (isprint(ch))
    {
      buf[0] = ch; buf[1] = '\0';
    }
    else
    {
      sprintf(buf, curr_lang[259], ch);
    }
    return buf;
  }
}


static int set_window_size(r, c)
  int r;
  int c;
{
#ifdef __svr4__
  /*
   *  Under Solaris 2.[23] the TIOCGWINSZ ioctl fails when we are started by
   *  telnetd, but works under rlogind.  In any case, we'll just stick to
   *  environment variables.
   */
  static char envcols[64];
  static char envlines[64];

  sprintf(envcols, "COLUMNS=%d", c);
  putenv(envcols);
  cols = c;
  sprintf(envlines, "LINES=%d", r);
  putenv(envlines);
  rows = r;

  return 1;
    
#else
  
  struct winsize wsize;

  if (ioctl(1, TIOCGWINSZ, (caddr_t)&wsize) == -1)
  {
    error(A_SYSERR, curr_lang[260], curr_lang[261]);
    return 0;
  }
  else
  {
    wsize.ws_row = r ? r: rows;
    wsize.ws_col = c ? c: cols;

    if (ioctl(1, TIOCSWINSZ, (caddr_t)&wsize) == -1)
    {
      error(A_SYSERR, curr_lang[260], curr_lang[262]);
      return 0;
    }
    else
    {
      rows = wsize.ws_row;
      cols = wsize.ws_col;
      return 1;
    }
  }
#endif
}


static const char *set_term_type(term_name)
  const char *term_name;
{
  char tc_ent[TC_ENT_LEN];
  static char term_env[16 + MAX_VAR_STR_LEN];

  ptr_check(term_name, const char, curr_lang[263], 0);

  switch (tgetent(tc_ent, term_name))
  {
  case -1:
    printf(curr_lang[264]);
/*    term_name = curr_lang[266];*/
    return (const char *)0;
    break;

  case 0:
    printf(curr_lang[265], term_name);
/*    term_name = curr_lang[266];*/
    return (const char *)0;
    break;

  case 1: /* It worked! */
    break;

  default:
    error(A_ERR, curr_lang[263], curr_lang[267]);
/*    term_name = curr_lang[266];*/
    return (const char *)0;
    break;
  }

  strcpy(term_env, curr_lang[268]);
  strcat(term_env, term_name);

  if (putenv(term_env) != 0) /* 0 means okay -- yeesh! */
  {
    error(A_ERR, curr_lang[263], curr_lang[269]);
  }
  return term_name;
}


/*  
 *  
 *                                External routines
 *  
 */  

int get_cols()
{
  return cols;
}


int get_rows()
{
  return rows;
}


int init_term()
{
  char targ[MAX_VAR_STR_LEN];
  const char *term;
  int c = 80;
  int r = 24;
#ifndef __svr4__
  struct winsize wsize;
#endif

  if ( ! (term = getenv(curr_lang[270])))
  {
    term = curr_lang[266];
  }

#ifndef __svr4__
  /* See previous comment w.r.t. TIOCGWINSZ. */
  if (ioctl(1, TIOCGWINSZ, (caddr_t)&wsize) == -1)
  {
    error(A_SYSERR, curr_lang[271], curr_lang[261]);
  }

  if (wsize.ws_row > 0) r = wsize.ws_row;
  if (wsize.ws_col > 0) c = wsize.ws_col;
#endif

  sprintf(targ, curr_lang[272], term, r, c);

  if (we_are_suid())
  {
    /*  
     *  Ensure we use the same file/directory as the pager.
     */  
    static char termcap[32+MAXPATHLEN+1];
    static char terminfo[32+MAXPATHLEN+1];

    sprintf(termcap, "TERMCAP=%s/pager/etc/termcap", homedir);
    putenv(termcap);
    sprintf(terminfo, "TERMINFO=%s/terminfo", homedir);
    putenv(terminfo);
  }

  return set_var(V_TERM, targ);
}


/*  
 *  Set the terminal type to one specified by the user.  Return an error if
 *  the terminal type is unknown to this system.
 *  
 *  Argument should have form: <term-type> [<#rows> [<#cols>]]
 */  

int set_term(term)
  const char *term;
{
  char cols_str[COMMAND_LEN];
  char rows_str[COMMAND_LEN];
  char term_type[INPUT_LINE_LEN];
  const char *tt;
  int win_cols = 80;
  int win_rows = 24;

  switch (sscanf(term, curr_lang[273], term_type, rows_str, cols_str))
  {
  case 3:
    /* Set the terminal type, number of rows and number of columns. */

    if (sscanf(cols_str, curr_lang[274], &win_cols) != 1)
    {
      printf(curr_lang[275], cols_str);
      return 0;
    }

  case 2:
    /* Set the terminal type and number of rows. */

    if (sscanf(rows_str, curr_lang[274], &win_rows) != 1)
    {
      printf(curr_lang[276], rows_str);
      return 0;
    }

  case 1:
    /* Set just the terminal type (handled below). */
    break;

  default:
    printf(curr_lang[277]);
    return 0;
  }

  if ( ! (tt = set_term_type(term_type))) return 0;

  if ( ! set_window_size(win_rows, win_cols))
  {
    error(A_ERR, curr_lang[278], curr_lang[279]);
    return 0;
  }

  printf(curr_lang[280], tt, win_rows, win_cols);
  return 1;
}


#include "terminal_lang.h"


static struct termios keytab;


int set_tty(ac, av)
  int ac;
  char **av;
{
  const char *cmd;
  int key_val;

  ptr_check(av, char *, curr_lang[281], 0);

  cmd = av[0];
  if (ac != 1 && ac % 2 == 0)
  {
    printf(curr_lang[282], cmd, cmd);
    return 0;
  }

  /* for now all we support is `erase' */

#ifdef __STDC__
  if (tcgetattr(0, &keytab) == -1)
#else  
  if (ioctl(0, TCGETS, &keytab) == -1)
#endif
  {
    error(A_SYSERR, curr_lang[281], curr_lang[283]);
    return 0;
  }

  if (ac == 1)                    /* no arguments */
  {
    char kstr[5];

    printf(curr_lang[284],
           get_key_str((unsigned)keytab.c_cc[VERASE], kstr, sizeof kstr));
    return 1;
  }
  else
  {
    while (av++, --ac)
    {
      if ( ! strtoval(av[0], &key_val, keys))
      {
        printf(curr_lang[285], av[0], cmd);
        return 0;
      }
      else
      {
        char c;

        if ( ! (c = get_key_char(*++av)))
        {
          return 0;
        }
        else
        {
          --ac;
          keytab.c_cc[key_val] = c;
        }
      }
    }

#ifdef __STDC__
    if (tcsetattr(0, TCSANOW, &keytab) != -1)
#else
    if (ioctl(0, TCSETS, &keytab) != -1)
#endif
    {
      return 1;
    }
    else
    {
      error(A_SYSERR, curr_lang[281], curr_lang[286]);
      return 0;
    }
  }
}
