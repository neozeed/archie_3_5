#ifndef VARS_H
#define VARS_H

#include "ansi_compat.h"


/*
   A variable name (as opposed to value) must fit in an array of this length.
*/

#define MAX_VAR_NAME_LEN   64
#define MAX_VAR_STR_LEN   256

#define BAD_TYPE 10

/*
  Names for variables.
*/

#define V_AUTOLOGOUT             "autologout"
#ifdef MULTIPLE_COLLECTIONS
# define V_COLLECTIONS           "collections"
#endif
#define V_COMPRESS               "compress"
#define V_DEBUG                  "debug"
#define V_EMAIL_HELP_FILE        "email_help_file"
#define V_ENCODE                 "encode"
#define V_HELP_DIR               "help_dir"
#define V_LANGUAGE               "language"
#define V_MAILTO                 "mailto"
#define V_MAIL_FROM              "mail_from"
#define V_MAIL_HOST              "mail_host"
#define V_MAIL_SERVICE           "mail_service"
#define	V_MAN_ASCII_FILE         "man_ascii_file"
#define	V_MAN_ROFF_FILE          "man_roff_file"
#define V_MATCH_DOMAIN           "match_domain"
#define V_MATCH_PATH             "match_path"
#define V_MAX_SPLIT_SIZE         "max_split_size"
#ifdef MULTIPLE_COLLECTIONS
# define V_MAXDOCS               "maxdocs"
# define V_MAXHDRS               "maxhdrs"
#endif
#define V_MAXHITS                "maxhits"
#define V_MAXHITSPM              "maxhitspm"
#define V_MAXMATCH               "maxmatch"
#define V_MOTD_FILE              "motd_file"
#define V_NICENESS               "niceness"
#define V_OUTPUT_FORMAT          "output_format"
#define V_PAGER                  "pager"
#define V_PAGER_HELP_OPTS        "pager_help_opts"
#define V_PAGER_OPTS             "pager_opts"
#define V_PAGER_PATH             "pager_path"
#define V_PROMPT                 "prompt"
#define V_SEARCH                 "search"
#define V_SERVER                 "server"
#define V_SERVERS_FILE           "servers_file"
#define V_SORTBY                 "sortby"
#define V_STATUS                 "status"
#define V_TMPDIR                 "tmpdir"
#define V_TERM                   "term"
#define V_WHATIS_FILE            "whatis_file"

enum var_type_e
{
  VAR_BAD_TYPE = BAD_TYPE,
  BOOLEAN,
  NUMERIC,
  STRING
};


extern int change_lang PROTO((const char *s));

extern const char *get_var PROTO((const char *varname));
extern int is_set PROTO((const char *varname));
extern int set_var PROTO((const char *var_name, const char *var_ptr));
extern int unset_var PROTO((const char *varname));

extern int set_it PROTO((int ac, char **av));
extern int show_it PROTO((int ac, char **av));
extern int unset_it PROTO((int ac, char **av));

#endif
