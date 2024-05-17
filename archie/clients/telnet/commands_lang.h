#include "macros.h"


/*  
 *  Note:
 *  
 *    COMMENT is handled in `get_cmd' since it needn't be followed by a space.
 */  

static Command commands[] =
{
  {
    BAD,    M_ALL,
    {
      {"bad command", FRENCH("commande incorrecte")},
      ENDCMD
    },
    ""
  },
  {
    BYE,     M_ALL,
    {
      {"bye", FRENCH("au_revoir")},
      ENDCMD
    },
    "- exit this program."
  },
  {
    COMMENT, M_ALL,
    {
      {"#"},
      ENDCMD
    },
    "- uninterpreted comment"
  },
  {
    COMPRESS, M_ALL,
    {
      {"compress", FRENCH("condenser")},
      ENDCMD
    },
    "- same as `set compress compress'"
  },
  {
    DISABLE, M_SYS_RC,
    {
      {"disable_command", FRENCH("desactiver_commande")},
      ENDCMD
    },
    "<command|variable> <cmd|var>..."
  },
  {
    DOMAINS, M_ALL,
    {
      {"domains", FRENCH("domaines")},
      ENDCMD
    },
    "- get domains supported by server"
  },
  {
    DONE,    M_HELP,
    {
      {"done", FRENCH("fin")},
      ENDCMD
    },
    "used to exit help system"
  },
  {
    EMPTY,  M_ALL,
    {
      {""},
      ENDCMD
    },
    ""
  },
  {
    EXIT,    M_ALL,
    {
      {"exit", FRENCH("sortir")},
      ENDCMD
    },
    "- exit this program."
  },
  {
    FIND,    M_EMAIL | M_INTERACTIVE,
    {
      {"find", FRENCH("chercher")},
      ENDCMD
    },
    "<ed-regex> - print all files matching the ed(1) regular expression from all sites."
  },
  {
    HELP,    M_EMAIL | M_INTERACTIVE,
    {
      {"help", FRENCH("aide")},
      ENDCMD
    },
    "- print this extremely informative list."
  },
  {
    IN,    M_EMAIL | M_INTERACTIVE,
    {
      {"in", FRENCH("dans")},
      ENDCMD
    },
    "- select a collection"
  },
  {
    LIST,    M_EMAIL | M_INTERACTIVE,
    {
      {"list", FRENCH("lister")},
      ENDCMD
    },
    "[<regex>] - list sites in the database"
  },
  {
    MAIL,    M_EMAIL | M_INTERACTIVE,
    {
      {"mail", FRENCH("poster")},
      ENDCMD
    },
    "[<e-mail-address>] - mail last output to address."
  },
  {
    MANPAGE, M_EMAIL | M_INTERACTIVE,
    {
      {"manpage", FRENCH("manuel")},
      ENDCMD
    },
    "- print the current archie manual page"
  },
  {
    MOTD,    M_ALL,
    {
      {"motd"},
      ENDCMD
    },
    "- print out login banner message"
  },
  {
    NON_UNIQUE, M_ALL,
    {
      {"non-unique command", FRENCH("commande ambigue")},
      ENDCMD
    },
    ""
  },
  {
    NOPAGER, M_ALL,
    {
      {"nopager", FRENCH("pas_paginer")},
      ENDCMD
    },
    "- undoes the effect of \"pager\"."
  },
  {
    PAGER,   M_SYS_RC | M_INTERACTIVE | M_USER_RC,
    {
      {"pager", FRENCH("paginer")},
      ENDCMD
    },
    "- output of \"prog\" and \"site\" is sent through the pager 'less'."
  },
  {
    PATH,    M_EMAIL,
    {
      {"path", FRENCH("chemin")},
      ENDCMD
    },
    "<mail-path> - set the e-mail address to which to mail results."
  },
  {
    PROG,    M_EMAIL | M_INTERACTIVE,
    {
      {"prog"},
      ENDCMD
    },
    "<ed-reg-ex> - alias for 'find'."
  },
  {
    QUIT,    M_ALL,
    {
      {"quit", FRENCH("quitter")},
      ENDCMD
    },
    "- exit this program."
  },
  {
    SERVERS, M_ALL,
    {
      {"servers", FRENCH("serveurs")},
      ENDCMD
    },
    "- list valid archie servers"
  },
  {
    SET,     M_ALL,
    {
      {"set", FRENCH("fixer")},
      ENDCMD
    },
    "<var> [<value>] - set a variable to an optional value."
  },
  {
    SHOW,    M_ALL,
    {
      {"show", FRENCH("afficher")},
      ENDCMD
    },
    "<var> - display the value of a variable."
  },
  {
    SITE,    M_NONE,
    {
      {"site"},
      ENDCMD
    },
    "<name> - print a list of all files at site <name>."
  },
  {
    STTY,   M_SYS_RC | M_INTERACTIVE | M_USER_RC,
    {
      {"stty"},
      ENDCMD
    },
    "<tty-opt> <char> [<tty-opt> <char>] ... - set options controlling the terminal."
  },
  {
    TERM,    M_ALL,
    {
      {"term"},
      ENDCMD
    },
    "<term-type> [<#rows> [<#cols>]] - tell the pager about your terminal."
  },
  {
    UNSET,   M_ALL,
    {
      {"unset", FRENCH("unset")},
      ENDCMD
    },
    "- unset a variable"
  },
  {
    VERSION, M_ALL,
    {
      {"version", FRENCH("version")},
      ENDCMD
    },
    "- print the version number of the software"
  },
  {
    WHATIS,  M_EMAIL | M_INTERACTIVE,
    {
      {"whatis", FRENCH("apropos")},
      ENDCMD
    },
    "<ed-regex>"
  },
  { (enum cmd_e) 0, M_NONE, ENDCMD, (char *)0}
};
