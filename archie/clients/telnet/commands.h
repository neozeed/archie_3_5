#ifndef COMMANDS_H
#define COMMANDS_H

#include "ansi_compat.h"
#include "defines.h"
#include "mode.h"
#include "strmap.h"


#define ENDCMD {(const char *)0, (const char *)0}


/*
    Numeric values for commands that archie knows about.
*/

enum cmd_e
{
  BAD = 0,			/* special: unrecognized command */
  BYE,				/* alias for "quit" */
  COMMENT,                      /* # comment */
  COMPRESS,                     /* same as `set compress compress' */
  DISABLE,
  DOMAINS,			/* get domains */
  DONE,
  EMPTY,			/* special: empty line was entered */
  EXIT,				/* alias for "quit" */
  FIND,
  HELP,
  IN,
  LIST,
  MAIL,
  MANPAGE,
  MOTD,
  NON_UNIQUE,			/* special: command abbreviation is not unique */
  NOPAGER,                      /* same as `unset pager' */
  PAGER,                        /* same as `set pager' */
  PATH,
  PROG,
  QUIT,
  SERVERS,
  SET,
  SHOW,
  SITE,
  STTY,
  TERM,                         /* same as `set term' */
  UNSET,
  VERSION,
  WHATIS
};

typedef struct
{
  enum cmd_e cmd_code;
  int mode;
  StrMap cmd_string[2];
  const char *cmd_help;
} Command;

extern int cmd_disable PROTO((const char *cmd));
extern enum cmd_e get_cmd PROTO((const char *line, int *argc, char ***argv, int *modes));

#ifndef AIX
extern Command commands[];
#endif

#endif
