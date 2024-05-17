#include "ansi_compat.h"


const char *english[] =
{

/* ==================== alarm.c ==================== */

	/*     0 */	"", /* avail */
	/*     1 */	"get_var() returned a null pointer.",

/* ==================== archie.c ==================== */

	/*     2 */	"", /* avail */
	/*     3 */	"", /* avail */
	/*     4 */	"", /* avail */
	/*     5 */	"main",
	/*     6 */	"unexpected argument `%s'.",
	/*     7 */	"argument required after `%s'.",
	/*     8 */	"can't run in both e-mail and server modes.",
	/*     9 */	"`-e' option requires `-o'.",
	/*    10 */	"w+",
	/*    11 */	"can't open e-mail output file `%s' with mode \"w+\"",
	/*    12 */	"can't freopen() `%s' onto stdout",
	/*    13 */	"can't freopen() `%s' onto stderr",
	/*    14 */	"", /* avail */
	/*    15 */	"", /* avail */
	/*    16 */	"", /* avail */
	/*    17 */	"can't find home directory!",
	/*    18 */	"can't chdir() to home directory, `%s'",
	/*    19 */	".",
	/*    20 */	"can't chroot() to home directory",
	/*    21 */	"setuid() failed",
	/*    22 */	"%s/%s",
	/*    23 */	"can't get new history context.",
        /*    24 */ "# Version fran\347aise r\351alis\351e en collaboration avec le\n# Service des T\351l\351communications, Universit\351 du Qu\351bec \340 Montr\351al.\n\n",
	/*    25 */	"stty",
	/*    26 */	"# Bunyip Information Systems, Inc., 1993, 1994, 1995\n\n",
	/*    27 */	"",
	/*    28 */	"\n",
	/*    29 */	"# Sorry, this command has been disabled by the administrator.\n",
	/*    30 */	"# Wrong mode, `%s', for the `%s' command.\n",
	/*    31 */	"\n>> %s\n",
	/*    32 */	"help",
	/*    33 */	"mail",
	/*    34 */	"# Bye.\n",
	/*    35 */	"# This command works only within the `help' subsystem.\n",
	/*    36 */	"# Non-unique command prefix.\n",
	/*    37 */	"# Sorry, the `site' command is currently unavailable.\n",
	/*    38 */	"# %s\n",
	/*    39 */	"# Unrecognized command `%s' in %s mode.\n",
	/*    40 */	"disable_command",
	/*    41 */	"`%s' requires at least one argument.",
	/*    42 */	"# Pager failed; printing directly to the screen.\n",
	/*    43 */	"init_from_file",
	/*    44 */	"r",
	/*    45 */	"man_it",
	/*    46 */	"# `%s' requires a single argument.\n",
	/*    47 */	"roff",
	/*    48 */	"unknown return code from fork_me().",
	/*    49 */	"whatis_it",
	/*    50 */	"new_tmp_file",
	/*    51 */	"error opening tmp file `%s'",
	/*    52 */	"tempnam() failed.",
	/*    53 */	"# `%s' requires at least one argument.\n",
	/*    54 */	"# boolean variables do not take arguments.\n",
	/*    55 */	"# `numeric' variables require a single argument.\n",
	/*    56 */	"# `string' variables require an argument.\n",
	/*    57 */	" ",
	/*    58 */	"# `%s' is not a known variable.\n",
	/*    59 */	"set_it",
	/*    60 */	"unknown variable type `%d'.",
	/*    61 */	"remove_tmp_file",
	/*    62 */	"error unlinking `%s'",
	/*    63 */	"# Sorry, I can't find a list of servers.\n",
	/*    64 */	"Usage: %s [-d <#>] [-e] [-i <init-file>] [-l] [-L <logfile>] [-p <#>] [-s]\n",

/* ==================== argv.c ==================== */

	/*    65 */	"can't allocate initial pointer element",
	/*    66 */	"new_str",
	/*    67 */	"failed to rmalloc() `%d' bytes",
	/*    68 */	"argvflatten",
	/*    69 */	"error malloc()ing `%u' bytes",
	/*    70 */	"argvify",
	/*    71 */	"failed to %s.",
	/*    72 */	"initialize argv",
	/*    73 */	"start new string",
	/*    74 */	"insert character",
	/*    75 */	"end current string",
	/*    76 */	"ended in bad state, `%d'.",

/* ==================== commands.c ==================== */

	/*    77 */	"cmd_disable",
	/*    78 */	"`%s': %s",
	/*    79 */	"get_cmd",

/* ==================== error.c ==================== */

	/*    80 */	"error: %s.\n",
	/*    81 */	"ERROR",
	/*    82 */	"INFO",
	/*    83 */	"INTERNAL ERROR",
	/*    84 */	"SYSTEM CALL ERROR",
	/*    85 */	"WARNING",
	/*    86 */	"# INTERNAL ERROR: error: unknown error type, %d.\n",
	/*    87 */	"# %s: %s: %s",
	/*    88 */	": unlisted error",
	/*    89 */	": ",
	/*    90 */	"%s  %s\n",
	/*    91 */	"/tmp/lll",
	/*    92 */	"w",

/* ==================== find.c ==================== */

	/*    93 */	"DIRECTORY",
	/*    94 */	"%s %s %s %s %s%s\n",
	/*    95 */	"LAST-MODIFIED",
	/*    96 */	"SIZE",
	/*    97 */	"UNIX-MODES",
	/*    98 */	"/",
	/*    99 */	"%H:%M %e %h %Y",
	/*   100 */	"%s  %12s %7s %s%s\n",
	/*   101 */	"AR_H_LAST_MOD",
	/*   102 */	"Host %s    (%s)\n",
	/*   103 */	"AR_H_IP_ADDR",
	/*   104 */	"Last updated %s\n",
	/*   105 */	"\n    Location: %s\n",
	/*   106 */	"      DIRECTORY    %s %13s  %12s  %s\n",
	/*   107 */	"      FILE    %s %13s  %12s  %s\n",
	/*   108 */	"fprint_list_item",
	/*   109 */	"unknown output style type `%d'.",
	/*   110 */	"do_matches",
	/*   111 */	"pager",
	/*   112 */	"verbose",
	/*   113 */	"terse",
	/*   114 */	"machine",
	/*   115 */	"\n# No matches were found.\n",
	/*   116 */	"\n# Error from Prospero server - ",
	/*   117 */	"find_it",

/* ==================== fork_wait.c ==================== */

	/*   118 */	"working...  ",
	/*   119 */	"# ERROR: fork_me: fork: ",
	/*   120 */	"fork_me",
  /*   121 */ "error waiting for child",

/* ==================== get_types.c ==================== */


/* ==================== help.c ==================== */

	/*   122 */	"help%s> ",
	/*   123 */	"%s/=",
	/*   124 */	"display_help",
	/*   125 */	"can't access() file `%s'",
	/*   126 */	"list_subtopics",
	/*   127 */	"can't open directory `%s'.",
	/*   128 */	"# Subtopics:\n#\n",
	/*   129 */	"#\t%s\n",
	/*   130 */	"# No subtopics.\n",
	/*   131 */	"pop_topic",
	/*   132 */	"can't find a `/' in the current directory string.",
	/*   133 */	"push_topics",
	/*   134 */	"# Can't find help on `%s'.\n",
	/*   135 */	"help_",
	/*   136 */	"can't find help file; looking in wrong directory?",
	/*   137 */	"?",
	/*   138 */	"done",
	/*   139 */	"Can't find the file with e-mail help information.",
	/*   140 */	"error getting new history context.",

/* ==================== input.c ==================== */

	/*   141 */	".\n",
	/*   142 */	"readline",
	/*   143 */	"can't malloc() `%d' bytes for a new line.",
	/*   144 */	"error reading from `stdin'.",
	/*   145 */	"unexpected return value from fread().",
	/*   146 */	"error realloc()ing `%d' bytes for input line.",

/* ==================== lang.c ==================== */


/* ==================== list.c ==================== */

	/*   147 */	"argument `item' is NULL.",
	/*   148 */	"%s %s\n",
	/*   149 */	"%-40s %15s     %17s\n",
	/*   150 */	"fprint_list_itme",
	/*   151 */	".*",
	/*   152 */	"list_it",

/* ==================== mail.c ==================== */

	/*   153 */	"mail_it",
	/*   154 */	"mailto",
	/*   155 */	"# You must either specify an e-mail address or set the `mailto' variable.\n",
	/*   156 */	"# `%s' takes no more than one argument.\n",
	/*   157 */	"# You haven't got any results to mail yet.\n",
	/*   158 */	"none",
	/*   159 */	"# If you specify compression you must also specify an encoding.\n",
	/*   160 */	"invalid port number `%s'.",
	/*   161 */	"getservbyname() failed -- can't get port for `%s'.",
	/*   162 */	"can't connect to mail host: `%s'.",
	/*   163 */	"error fdopen()ing socket to the mail host.",
	/*   164 */	"@Begin\n",
	/*   165 */	"Command: %s\n",
	/*   166 */	"Compress: %s\n",
	/*   167 */	"Encode: %s\n",
	/*   168 */	"MaxSplitSize: %d\n",
	/*   169 */	"@MailHeader\n",
	/*   170 */	"To: %s\n",
	/*   171 */	"From: %s@%s\n",
	/*   172 */	"Reply-To: %s@%s\n",
	/*   173 */	"Date: %s\n",
	/*   174 */	"@End\n",
	/*   175 */	"fputs() is EOF, errno is %d.\n",
	/*   176 */	"failed! errno = %d.\n",
	/*   177 */	"ferror(mfp) is %d, feof(mfp) is %d.\n",
	/*   178 */	"ferror(ofp) is %d, feof(ofp) is %d.\n",

/* ==================== misc.c ==================== */

	/*   179 */	"<no value>",
	/*   180 */	"first_word_of",
	/*   181 */	"dequote",
	/*   182 */	"nuke_newline",
	/*   183 */	"argument is NULL.",
	/*   184 */	"Name: `%s', Value: `%s'.\n",
	/*   185 */	"print_file",
	/*   186 */	"error from fgets() on `%s'",
	/*   187 */	"%4u%2u%2u%2u%2u%2u",
	/*   188 */	"cvt_to_inttime",
	/*   189 */	"error converting time `%s'.",
	/*   190 */	"prosp_strftime",
	/*   191 */	"prosp_strftime() failed on `%s'.",
	/*   192 */	"<bad time>",
	/*   193 */	"size_of_file",
	/*   194 */	"can't get size of mail file `%s'.",
	/*   195 */	"snarf_n_barf",
	/*   196 */	"failed to fputs()",
	/*   197 */	"fgets() failed",
	/*   198 */	"squeeze_whitespace",
	/*   199 */	"argument `str' is NULL.",
	/*   200 */	"initskip",
	/*   201 */	"can't malloc %d bytes for pattern.",
	/*   202 */	"can't realloc %d bytes for pattern.",
	/*   203 */	"# `%s' requires an argument.\n",
	/*   204 */	"copy_first_word",

/* ==================== mode.c ==================== */

	/*   205 */	"<no mode>",
	/*   206 */	"<bad mode>",
	/*   207 */	"set_mode",
	/*   208 */	"unknown mode `0x%08x'; changing to %s mode.",

/* ==================== pager.c ==================== */

	/*   209 */	"close_pager_file",
	/*   210 */	"error from `fclose'.",
	/*   211 */	"create_pager_file",
	/*   212 */	"couldn't create temporary file.",
	/*   213 */	"can't open newly created temporary file.",
	/*   214 */	"fopen() failed on pager file.",
	/*   215 */	"error from fork().",
	/*   216 */	"`%s' is NULL.",
	/*   217 */	"execl for pager `%s' failed",
	/*   218 */	"remove_pager_file",
	/*   219 */	"argument `pager_file_name' is NULL.",
	/*   220 */	"error removing temporary pager file.",
	/*   221 */	"set_pager_opts",
	/*   222 */	"argument `val' is NULL.",

/* ==================== pexec.c ==================== */

	/*   223 */	"make_argv",
	/*   224 */	"argument list is not NULL terminated.",
	/*   225 */	"p_close",
	/*   226 */	"tried to wait for non-existant child.",
	/*   227 */	"argument does not correspond to either end of the pipe.",
	/*   228 */	"p_execvp",
	/*   229 */	"# ERROR: p_execvp: pipe: ",
	/*   230 */	"# ERROR: p_execvp: fdopen: ",
	/*   231 */	"# ERROR: p_execvp: fdopen ",
	/*   232 */	"type argument to p_execvp is `%s'.",
	/*   233 */	"# ERROR: pexecvp: fork ",
	/*   234 */	"# ERROR: p_execvp: dup2(p[1], 1) ",
	/*   235 */	"# ERROR: p_execvp: dup2(p[0], 0)",
	/*   236 */	"# ERROR: p_execvp: execvp",

/* ==================== prospero_fix.c ==================== */


/* ==================== rmem.c ==================== */

	/*   237 */	"rfree %p\n",
	/*   238 */	"rmalloc: %p\n",
	/*   239 */	"got %p again!\n",
	/*   240 */	"set foo to %p.\n",

/* ==================== signals.c ==================== */

	/*   241 */	"catch_alarm",
	/*   242 */	"start",
	/*   243 */	"not waiting for child",
	/*   244 */	"\n# auto-logout\n",
	/*   245 */	"waiting for child",
	/*   246 */	"`get_var' returned null.",
	/*   247 */	"you can stick around -- with %d secs left",
	/*   248 */	"\n# Aborted.\n",

/* ==================== strmap.c ==================== */


/* ==================== terminal.c ==================== */

	/*   249 */	"get_key_char",
	/*   250 */	"# expected a single character or `^<char>', rather than `%s'.\n",
	/*   251 */	"# `%s' (0x%02x) does not make a valid control character.\n",
	/*   252 */	"get_key_str",
	/*   253 */	"buffer `%s' is too small.",
	/*   254 */	"buf",
	/*   255 */	"value of `%s' (`%lu') is out of range.",
	/*   256 */	"ch",
	/*   257 */	"^?",
	/*   258 */	"^%c",
	/*   259 */	"\\%03o",
	/*   260 */	"set_window_size",
	/*   261 */	"can't get winsize structure",
	/*   262 */	"can't set winsize structure",
	/*   263 */	"set_term_type",
	/*   264 */	"# Unable to open terminal description file.\n",
	/*   265 */	"# Terminal type `%s' is unknown to this system.\n",
	/*   266 */	"dumb",
	/*   267 */	"tgetent() returned an undocumented value; trying `dumb'.",
	/*   268 */	"TERM=",
	/*   269 */	"error from putenv().",
	/*   270 */	"TERM",
	/*   271 */	"init_term",
	/*   272 */	"%s %d %d",
	/*   273 */	"%s %s %s",
	/*   274 */	"%d",
	/*   275 */	"# Number of columns, `%s', must be numeric.\n",
	/*   276 */	"# Number of rows, `%s', must be numeric.\n",
	/*   277 */	"# Command format is `term <term-type> [<#rows> [<#cols>]]'.\n",
	/*   278 */	"set_term",
	/*   279 */	"error setting the terminal size.",
	/*   280 */	"# Terminal type set to `%s %d %d'.\n",
	/*   281 */	"set_tty",
	/*   282 */	"# `%s' takes arguments in pairs; see help on `%s' for usage.\n",
	/*   283 */	"error getting termios structure",
	/*   284 */	"# `erase' character is `%s'.\n",
	/*   285 */	"# `%s' is not a valid argument; see help on `%s' for usage.\n",
	/*   286 */	"error setting termios structure",

/* ==================== vars.c ==================== */

	/*   287 */	"# `%s' cannot be set in %s mode.\n",
	/*   288 */	"# `%s' is read-only; it's value may not be changed.\n",
	/*   289 */	"# `%s' is not a valid value, `help set %s' for a list.\n",
	/*   290 */	"# Value must be an integer.\n",
	/*   291 */	"# Value must be in the range [%d,%d].\n",
	/*   292 */	"set_var_",
	/*   293 */	"can't set new value.",
	/*   294 */	"# Value is too long (> %d bytes).\n",
	/*   295 */	"set_var",
	/*   296 */	"variable has unknown type.",
	/*   297 */	"# `%s' is not a known variable in `%s' mode.\n",
	/*   298 */	"boolean",
	/*   299 */	"# `%s' (type %s) is set.\n",
	/*   300 */	"# `%s' (type %s) is not set.\n",
	/*   301 */	"numeric",
	/*   302 */	"string",
	/*   303 */	"show_var",
	/*   304 */	"variable `%s' has an unknown type.",
	/*   305 */	"# `%s' (type %s) has the value `%s'.\n",
	/*   306 */	"# `%s' cannot be unset -- it must always have a value.\n",
	/*   307 */	"english",
	/*   308 */	"francais",
	/*   309 */	"# WARNING: messages will not be in `%s'.\n",
	/*   310 */	"# Can't set language to `%s'. No help directory.\n",

/* ==================== version.c ==================== */


/* ==================== whatis.c ==================== */

	/*   311 */	"get_whatis_list",
	/*   312 */	"can't open description database, `%s'.",
	/*   313 */	"%-25s %-54s\n",
	/*   314 */	"# Nothing found that matched `%s'.\n",

/* ==================== EXTRA ==================== */

  /*   315 */     "# `%s' does not take any arguments.\n",
  /*   316 */     "error ftruncate()ing temporary file `%s'",
  /*   317 */     "# `%s' requires at least two arguments.\n",
  /*   318 */     "# `%s' is not a valid sub-command.\n",
  /*   319 */     "# error trying to set mail path.\n",

  /*   320 */     "# Search type: %s",
  /*   321 */     "%sDomain: %s",
  /*   322 */     "%sPath: %s",
  /*   323 */	  "child terminated abnormally with signal %d.",
  /*   324 */     "\n  ftp://%s%s \n",
  /*   325 */     "\t\tDate: %12s      Size: %7s\n",
  /*   326 */	  "Precedence: junk\n"
};
