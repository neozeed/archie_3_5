#include <string.h>
#include "argv.h"
#include "client_defs.h"
#include "commands.h"
#include "commands_lang.h"
#include "extern.h"
#include "defines.h"
#include "error.h"
#include "input.h"
#include "lang.h"
#include "macros.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "strmap.h"
#include "vars_lang.h"

#include "protos.h"

static Command *find_cmd PROTO((const char *cmd,
                                const char *(*cmp) PROTO((const char *s, const StrMap *map))));
static Command *really_find_cmd PROTO((const char *cmd));


static Command *find_cmd(cmd, cmp)
  const char *cmd;
  const char *(*cmp) PROTO((const char *s, const StrMap *map));
{
  Command *c = commands;
  Command *cmd_is = (Command *)0;
  
  if (cmd[0] == COMMENT_CHAR)
  {
    return &commands[COMMENT];
  }
  
  while ( ! mapEmpty(c->cmd_string))
  {
    if (cmp(cmd, c->cmd_string)) /*bug: requires full command name*/
    {
      if (cmd_is)
      {
        return &commands[NON_UNIQUE];
      }
      else
      {
	cmd_is = c;
      }
    }
    c++;
  }

  return cmd_is ? cmd_is : &commands[BAD];
}


static Command *really_find_cmd(cmd)
  const char *cmd;
{
  Command *c;

  if (strcmp(curr_lang[307], get_var(V_LANGUAGE)) != 0) /*bug: kludge!!!*/
  {
    c = find_cmd(cmd, mapNCaseStrTo);
    if (c->cmd_code != BAD) return c;
  }

  return find_cmd(cmd, mapNCaseStrFrom);
}


int cmd_disable(cmd)
  const char *cmd;
{
  Command *c = really_find_cmd(cmd);

  if (c->cmd_code == BAD || c->cmd_code == NON_UNIQUE)
  {
    error(A_ERR, curr_lang[77], curr_lang[78], cmd, mapFirstStr(c->cmd_string));
    return 0;
  }
  else
  {
    c->mode = M_NONE;
    return 1;
  }
}


/*  
 *  Determine which command the user has entered.  It is insensitive to the
 *  case of the command.
 */  

enum cmd_e get_cmd(line, ac, av, modes)
  const char *line;
  int *ac;
  char ***av;
  int *modes;
{
  Command *cmd;
  const char *end;
  const char *start = line;

  ptr_check(line, const char, curr_lang[79], (enum cmd_e)INTERNAL_ERROR);
  ptr_check(ac, int, curr_lang[79], (enum cmd_e)INTERNAL_ERROR);
  ptr_check(av, char **, curr_lang[79], (enum cmd_e)INTERNAL_ERROR);
  ptr_check(modes, int, curr_lang[79], (enum cmd_e)INTERNAL_ERROR);

  *ac = 0;
  *av = (char **)0;
  bracketstr(line, &start, &end);

  if ( ! *start)
  {
    *modes = commands[EMPTY].mode;
    return EMPTY;
  }
  else if (*start == COMMENT_CHAR)
  {
    /*
      Comments are a special case, since there needn't be a space after
      the initial character.
    */

    *modes = commands[COMMENT].mode;
    return COMMENT;
  }
  else if ( ! argvify(line, ac, av))
  {
    return INTERNAL_ERROR;
  }
  else
  {
    cmd = really_find_cmd((*av)[0]);
    *modes = cmd->mode;
    return cmd->cmd_code;
  }
}
