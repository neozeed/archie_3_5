#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "regexp.h"
#include "defines.h"
#include "parser_file.h"
#include "parse.h"
#include "utils.h"
#include "queue.h"
#include "stack.h"
#include "output.h"
#include "line_type.h"
#include "storage.h"
#include "pars_ent_holder.h"
#include "unix2.h"
#include "error.h"
#include "lang_parsers.h"
#ifdef SOLARIS
#include "protos.h"
#endif

#define S_BLANK	    "^$"
#define S_CONT	    (regexp *)0 ;
#define S_DIR_START "^/?[^/]+(/[^/]+)*:$"
#define S_ERROR	    (regexp *)0 ;
#define S_FILE	    "^[-bcdlpsyDFL][-r][-w][-xsSl][-r][-w][-xsSl][-r][-w][-xtT][ \t]+"
#define S_PARTIAL   (regexp *)0 ;
#define S_TOTAL	    "^total[ \t]+[0-9]+"
#define S_UNREAD    "^[^ \t]+[ \t]+unreadable$"
#define S_UNKNOWN   (regexp *)0 ;

enum /* declared as enum to make debugging easier */
{
  F_PERM = 0,
  F_LINKS,
  F_OWNER,
  F_GROUP,
  F_SIZE,
  F_MONTH,
  F_DAY,
  F_T_Y,                        /* time (16:36) or year (1992) */
  F_NAME,
  UNIX_NUM_FLDS                 /* this must be last */
} ;

static regexp *re[L_NUM_ELTS] ;

static int leading_blanks = -1;

char *S_dup_dir_name(name)
   const char *name ;
{
  return strdup(name) ;
}


/*
  Given a file line (see below for example) produce a core site entry
  and return a pointer to the file name.  Return a null pointer upon
  an error.

  The input string will be overwritten.

  An input line looks something like:

    -rw-------  1 news         1630 Mar 18 16:15 #alt.music.alternative~

  as well, a group name may be present and/or the time may be specified
  differently:

    -rw-r--r--  1 news         12 Jul 11  1991 .rhosts
  or
    -rw-r--r--  1 news     news           12 Jul 11  1991 .rhosts

  Then again, we may have a device with major and minor numbers in place of
  the size:

  crw-rw-rw-  1 0        1          3,  12 Mar 19 17:03 zero
*/

char *S_file_parse(in, pe, is_dir)
  char *in ;
  parser_entry_t *pe ;
  int *is_dir ;
{
  char *field[UNIX_NUM_FLDS] ;
  char *p = in ;
  int is_symlink = 0;
  int num_blanks;
  
  ptr_check(in, char, "S_file_parse", (char *)0) ;
  ptr_check(pe, parser_entry_t, "S_file_parse", (char *)0) ;

#define try_get_dev_field(fld, instr, fldname) \
  do \
  { \
    instr += strspn(instr, DEV_WHITE_STR) ; \
    if((field[fld] = strsep(&instr, DEV_WHITE_STR)) == (char *)0) \
    { \
    /* "Error looking for white space after %s" */\
      error(A_ERR, "S_file_parse", U_S_FILE_PARSE_001, fldname) ; \
      return (char *)0 ; \
    } \
  } while(0)

#define try_get_field(fld, instr, fldname) \
  do \
  { \
    instr += strspn(instr, WHITE_STR) ; \
    if((field[fld] = strsep(&instr, WHITE_STR)) == (char *)0) \
    { \
    /* "Error looking for white space after %s" */\
      error(A_ERR, "S_file_parse", U_S_FILE_PARSE_001, fldname) ; \
      return (char *)0 ; \
    } \
  } while(0)


  try_get_field(F_PERM, p, "permissions") ;
  try_get_field(F_LINKS, p, "links") ;
  try_get_field(F_OWNER, p, "owner") ;

  if(*field[F_PERM] == 'b' || *field[F_PERM] == 'c') /* devices */
  {
    try_get_dev_field(F_GROUP, p, "group or major number") ;
    if(*(field[F_GROUP] + strlen(field[F_GROUP]) - 1) == ',' ) /* is major number */

    {
      /* there is no group -- toss the major and minor numbers */

      try_get_field(F_GROUP, p, "minor number") ;
      field[F_GROUP] = (char *)0 ;
    }
    else                        /* is group */
    {
      /* there is a group -- still toss the major and minor numbers */
#if 0

    p+= strspn(p, DEV_WHITE_STR) ; 
    if((field[F_NAME] = strsep(&p, DEV_WHITE_STR)) == (char *)0) 
    { 
    /* "Error looking for white space after %s" */
      error(A_ERR, "S_file_parse", U_S_FILE_PARSE_001, "major") ; 
      return (char *)0 ; 
    } 

    p += strspn(p, DEV_WHITE_STR) ; 

    if((field[F_NAME] = strsep(&p, DEV_WHITE_STR)) == (char *)0) 
    { 
    /* "Error looking for white space after %s" */
      error(A_ERR, "S_file_parse", U_S_FILE_PARSE_001, "minor") ; 
      return (char *)0 ; 
    } 
#endif
    if ( strchr(field[F_GROUP],',') == NULL )  {
      char *ptr;
      
       try_get_dev_field(F_NAME, p, "major number") ;
       ptr = strchr(field[F_NAME],',');

       if ( ptr == NULL  || ptr-field[F_NAME] == strlen(field[F_NAME])-1 )
         try_get_dev_field(F_NAME, p, "minor number") ;
     }

  }
    field[F_SIZE] = "0" ;
    try_get_field(F_MONTH, p, "month") ;
  }
  else                          /* non-devices */
  {
    is_symlink = *field[F_PERM] == 'l';
    
    try_get_field(F_GROUP, p, "group or size") ; /* may be size */
    try_get_field(F_SIZE, p, "size or month") ; /* may be month */

    if(isdigit(*field[F_SIZE])) /* there was a group field */
    {
      try_get_field(F_MONTH, p, "month") ;
    }
    else                        /* no group field -- group was size, size was month */
    {
      field[F_MONTH] = field[F_SIZE] ;
      field[F_SIZE] = field[F_GROUP] ;
      field[F_GROUP] = (char *)0 ;
    }
  }

  try_get_field(F_DAY, p, "day") ;
  try_get_field(F_T_Y, p, "time or year") ;

  num_blanks = strspn(p, WHITE_STR); /* Assume that there is 1 separating */
  if ( leading_blanks > num_blanks ) {
    leading_blanks = num_blanks;
    error(A_INFO,"S_file_parse", "Confused by leading blank characters, will restart and try to be more intelligent !");
    return 0;
  }
  if ( leading_blanks == -1 ) 
    leading_blanks = num_blanks;

  p+= leading_blanks;



  field[F_NAME] = p ;
  if(*field[F_NAME] == '\0')
  {

    /* "Missing file name" */

    error(A_ERR, "S_file_parse", U_S_FILE_PARSE_002) ;
    return (char *)0 ;
  }

  /*
     If the file is a symbolic link, put a nul at the first occurrence
     of " -> ".
  */
  if (is_symlink)
  {
    char *p;

    if ( ! (p = strstr(field[F_NAME], " -> ")))
    {
      error(A_WARN, "S_file_parse", U_S_FILE_PARSE_006, in, line_num());
    }
    else
    {
      *p = '\0';
    }
  }

#ifdef __STDC__

#define try_cvt_str(s) if( ! (s)) do { error(A_ERR, "S_file_parse", "Error from " #s "\n") ; return (char *)0 ; } while(0)
#else

/* "Error extracting field" */

#define try_cvt_str(s) if( ! (s)) do { error(A_ERR, "S_file_parse", U_S_FILE_PARSE_004) ; return (char *)0 ; } while(0)
#endif

  try_cvt_str(u2db_perm(field[F_PERM], &pe->core)) ;
  try_cvt_str(u2db_owner(field[F_OWNER], field[F_GROUP], &pe->core)) ;
  try_cvt_str(u2db_size(field[F_SIZE], &pe->core)) ;
  try_cvt_str(u2db_time(field[F_MONTH], field[F_DAY], field[F_T_Y], &pe->core)) ;
    
  pe->slen = strlen(field[F_NAME]) ; /* when writing we don't include the nul terminator */

  /* Is it an entry for a directory? */

  if( ! S_file_type(field[F_PERM], is_dir))
  {

  /* "Error from S_file_type()" */

    error(A_ERR, "S_file_parse", U_S_FILE_PARSE_005) ;
    return (char *)0 ;
  }
  return field[F_NAME] ;

#undef try_get_field
#undef try_cvt_str
}


int S_file_type(s, is_dir)
  const char *s ;
  int *is_dir ;
{
  ptr_check(s, char, "S_file_type", 0) ;
  ptr_check(is_dir, int, "S_file_type", 0) ;

  *is_dir = ((*s == 'd') || (*s == 'D'));
  return 1 ;
}


int S_init_parser()
{
  re[L_BLANK] = regcomp(S_BLANK) ;
  re[L_CONT] = S_CONT ;
  re[L_DIR_START] = regcomp(S_DIR_START) ;
  re[L_ERROR] = S_ERROR ;
  re[L_FILE] = regcomp(S_FILE) ;
  re[L_PARTIAL] = S_PARTIAL ;
  re[L_TOTAL] = regcomp(S_TOTAL) ;
  re[L_UNREAD] = regcomp(S_UNREAD) ;
  re[L_UNKNOWN] = S_UNKNOWN ;

  return 1 ;
}


#ifdef DEBUG
#   define dbg_show(t)	error(A_INFO,"(unknown)", "D: %s '%s'\n", t, line) ;
#else
#   define dbg_show(t)          /* nothing */
#endif

int S_line_type(line)
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
  else if(regexec(re[L_TOTAL], line))
  {
    dbg_show("total") ;
    return L_TOTAL ;
  }
  else if(regexec(re[L_UNREAD], line))
  {
    dbg_show("unreadable") ;
    return L_UNREAD ;
  }
  else
  {
    dbg_show("unknown") ;
    return L_UNKNOWN ;
  }
}


/*
  Split a directory path name into its components.

  Return a pointer to an array of pointers into 'str', where each element
  points to the start of a file name in the path.  The number of path elements
  will be returned in 'n'.  If this type of listing has device name prefixes,
  then '*dev' will point to it.

  The string will be overwritten.

  If an error occurs return a null pointer.
*/

int S_split_dir(str, prep_dir, dev, n, p)
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

  *dev = 0 ;                    /* UNIX sites don't have a device name prefix */
  if(prep_dir != (char *)0)
  {
    path[i++] = prep_dir ;
  }
  if(*s == '/')                 /* any leading / is meaningless to us */
  {
    s++ ;
  }
  path[i++] = s ;
  while(*s != '\0')
  {
    if(*s != '/')
    {
      s++ ;
    }
    else
    {
      *s++ = '\0' ;
      path[i++] = s ;
    }
  }
  if(*--s == ':')               /* knock off the trailing colon from the file name */
  {
    *s = '\0' ;
  }
  else
  {

  /* "Expected ':' at end of path, but found '%c'" */

    error(A_ERR, "S_split_dir", S_SPLIT_DIR_001, *s) ;
    return 0 ;
  }
  *n = i ;
  *p = path ;
  return 1 ;
}


int set_blanks(num)
  int num;
{
    if (leading_blanks != -1 )
      leading_blanks -= num;
}
