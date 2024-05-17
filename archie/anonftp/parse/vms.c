/*
    vms.c
*/

#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "regexp.h"
#include "parser_file.h"
#include "parse.h"
#include "utils.h"
#include "queue.h"
#include "stack.h"
#include "output.h"
#include "line_type.h"
#include "storage.h"
#include "pars_ent_holder.h"
#include "error.h"
#include "lang_parsers.h"
#include "vms2.h"
#ifdef SOLARIS
#include "protos.h"
#endif
static regexp *re_cont ;
static regexp *re_dir_start ;
static regexp *re_error ;
static regexp *re_file ;
static regexp *re_partial ;

#define S_BLANK	    "^$"
#define S_CONT	    "^[ \t]+[%0-9]"
#define S_DIR_START "^([^:]+):\\[([^].]+)(\\.[^].]+)*\\]"
#define S_ERROR	    "^[^.]*\\.[^;]*;[0-9]+[ \t]+[%0-9]"
#define S_FILE	    "^[^.]*\\.[^;]*;[0-9]+[ \t]+[0-9]+"
#define S_PARTIAL   "^[^.]*\\.[^;]*;[0-9]+"
#define S_TOTAL	    "^Total of|^Grand total of"
#define S_UNKNOWN   (regexp *)0 ;

static regexp *re[L_NUM_ELTS] ;


#define F_NAME	0
#define F_SIZE	1
#define F_DATE	2
#define F_TIME	3
#define F_OWN	4
#define F_PERM	5
#define VMS_NUM_FLDS	(F_PERM+1)


char *
S_dup_dir_name(name)
  const char *name ;
{
  char *d = strchr(name, '.') ; /* can it only have one '.'? */

  if(d == (char *)0)
  {

  /* "'%S' doesn't contain a '.'" */

    error(A_ERR, "S_dup_dir_name", S_DUP_DIR_NAME_001, name) ;
    return (char *)0 ;
  }
  else
  {
    char *s ;

    *d = '\0' ;
    s = strdup(name) ;
    *d = '.' ;
    return s ;
  }
}

/*
  Given a file line (see below for example) produce a core site entry
  and return a pointer to the file name.  Return a null pointer upon
  an error.

  The input string will be overwritten.

  An input line looks something like:

    ARIZONET.DIS;10             2   8-NOV-1989 17:05 NETINFO (RWED,RWED,RWED,RE)
*/

char *
S_file_parse(in, pe, is_dir)
  char *in ;
  parser_entry_t *pe ;
  int *is_dir ;
{
  char *field[VMS_NUM_FLDS] ;

  ptr_check(in, char, "S_file_parse", (char *)0) ;
  ptr_check(pe, parser_entry_t, "S_file_parse", (char *)0) ;

#define try_get_field(fld, instr, fldname) \
  if((field[fld] = strtok(instr, WHITE_STR)) == (char *)0) \
    do \
    { \
      /* "Error looking for white space after %s" */\
      error(A_ERR, "S_file_parse", V_S_FILE_PARSE_001, fldname) ; \
      return (char *)0 ; \
    } while(0)

  try_get_field(F_NAME, in, "file name") ;
  try_get_field(F_SIZE, (char *)0, "file size") ;
  try_get_field(F_DATE, (char *)0, "date") ;
  try_get_field(F_TIME, (char *)0, "time") ;
  try_get_field(F_OWN, (char *)0, "owner") ;
  try_get_field(F_PERM, (char *)0, "permissions") ;

#ifdef __STDC__
#define try_cvt_str(s) \
  if( ! (s)) \
    do \
    { \
      error(A_ERR, "S_file_parse","Error from " #s "\n") ; return (char *)0 ; \
    } while(0)
#else
#define try_cvt_str(s) \
  if( ! (s)) \
    do \
    { \
    /* "Error extracting field" */\
      error(A_ERR, "S_file_parse", V_S_FILE_PARSE_002) ; return (char *)0 ; \
    } while(0)
#endif

  try_cvt_str(v2db_perm(field[F_PERM], &pe->core)) ;
  try_cvt_str(v2db_owner(field[F_OWN], &pe->core)) ; /* includes group owner */
  try_cvt_str(v2db_size(field[F_SIZE], &pe->core)) ;
  try_cvt_str(v2db_time(field[F_DATE], field[F_TIME], &pe->core)) ;
    
  pe->slen = strlen(field[F_NAME]) ; /* when writing we don't include the nul terinator */

  /* Is it an entry for a directory? */

  if( ! S_file_type(field[F_NAME], is_dir))
  {

  /* "Error from S_file_type()" */

    error(A_ERR, "S_file_parse", V_S_FILE_PARSE_003) ;
    return (char *)0 ;
  }
  return field[F_NAME] ;

#undef try_get_field
#undef try_cvt_str
}

int
S_file_type(s, is_dir)
  const char *s ;
  int *is_dir ;
{
  char *d = strchr(s, '.') ;

  ptr_check(s, char, "S_file_type", 0) ;
  ptr_check(is_dir, int, "S_file_type", 0) ;

  if(d == (char *)0)
  {


    /* "Can't find '.' in file name '%s'" */

    error(A_ERR, "S_file_type", S_FILE_TYPE_001, s) ;
    return 0 ;
  }
  else
  {
    *is_dir = strncmp(d, ".DIR;", 5) == 0 ;
    return 1 ;
  }
}

int
S_init_parser()
{
  re[L_BLANK] = regcomp(S_BLANK) ;
  re[L_CONT] = regcomp(S_CONT) ;
  re[L_DIR_START] = regcomp(S_DIR_START) ;
  re[L_ERROR] = regcomp(S_ERROR) ;
  re[L_FILE] = regcomp(S_FILE) ;
  re[L_PARTIAL] = regcomp(S_PARTIAL) ;
  re[L_TOTAL] = regcomp(S_TOTAL) ;
  re[L_UNKNOWN] = S_UNKNOWN ;
  return 1 ;
}

#ifdef DEBUG
#   define dbg_show(t)	error(A_INFO, "(unknown)", "D: %s '%s'", t, nuke_nl(line)) ;
#else
#   define dbg_show(t)          /* nothing */
#endif

int
S_line_type(line)
  const char *line ;
{
  ptr_check(line, char, "S_line_type", L_INTERN_ERR) ;

  if(regexec(re[L_FILE], line))
  {
    dbg_show("file") ;
    return L_FILE ;
  }
  else if(regexec(re[L_BLANK], line))
  {
    dbg_show("blank") ;
    return L_BLANK ;
  }
  else if(regexec(re[L_DIR_START], line))
  {
    dbg_show("dir_start") ;
    return L_DIR_START ;
  }
  else if(regexec(re[L_ERROR], line))
  {
    dbg_show("error") ;
    return L_ERROR ;
  }
  else if(regexec(re[L_PARTIAL], line)) /* should check length as well */
  {
    dbg_show("partial") ;
    return L_PARTIAL ;
  }
  else if(regexec(re[L_CONT], line))
  {
    dbg_show("cont") ;
    return L_CONT ;
  }
  else if(regexec(re[L_TOTAL], line))
  {
    dbg_show("total") ;
    return L_TOTAL ;
  }
  else
  {
    dbg_show("unknown") ;
    return L_UNKNOWN ;
  }
}


/*
  Split a directory path name into its components.
        
  Return a pointer to an array of pointers into 'str', where each
  element points to the start of a file name in the path.  The
  number of path elements will be returned in 'n'.
        
  If this type of listing has device name prefixes, then '*dev' will
  point to it.
        
  The string will be overwritten.
        
  If an error occurs return a null pointer.
*/

#define DEV_TERM ':'
#define PATH_START '['
#define PATH_SEP '.'
#define PATH_END ']'

int
S_split_dir(str, prep_dir, dev, n, p)
  char *str ;
  char *prep_dir ;
  int *dev ;
  int *n ;
  char ***p ;
{
  static char *path[MAX_PATH_ELTS + 1] ;
  char *s = str ;
  int i = 0 ;

  ptr_check(str, char, "S_split_dir", 0) ;
  ptr_check(dev, int, "S_split_dir", 0) ;
  ptr_check(n, int, "S_split_dir", 0) ;
  ptr_check(p, char **, "S_split_dir", 0) ;

  *dev = 1 ;                    /* VMS sites have a device name prefix */
  path[i++] = s ;
  *(s = strchr(s, DEV_TERM)) = '\0' ;
  s += 2 ;
  do
  {
    path[i++] = s ;
    switch(*(s = strpbrk(s, ".]")))
    {
    case '.':
      *s++ = '\0' ;
      break ;

    case ']':
      *s = '\0' ;               /* terminate the loop */
      break ;
    }
  }
  while(*s != '\0') ;
  *n = i ;
  *p = path ;
  return 1 ;
}


int set_blanks(num)
  int num;
{
/*    if (leading_blanks != -1 )
      leading_blanks -= num;
      */
}
