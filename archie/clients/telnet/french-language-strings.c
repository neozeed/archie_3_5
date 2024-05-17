#include "ansi_compat.h"


const char *french[] =
{

/* ==================== alarm.c ==================== */

  /*     0 */ "", /* avail */
  /*     1 */ "get_var() est revenu un indicateur nul.",

/* ==================== archie.c ==================== */

  /*     2 */ "", /* avail */
  /*     3 */ "", /* avail */
  /*     4 */ "", /* avail */
  /*     5 */ "main", /*X*/
  /*     6 */ "param\350tre inattendu `%s'.",
  /*     7 */ "param\350tre n\351cessaire apr\350s `%s'.",
  /*     8 */ "ne peut ex\351cuter \340 la fois en mode courrier et en mode serveur.",
  /*     9 */ "`-e' option n\351cessitant `-o'.",
  /*    10 */ "w+", /*X*/
  /*    11 */ "ne peut ouvrir le fichier sortie de courrier `%s' en mode \"w+\"",
  /*    12 */ "can't freopen() `%s' onto stdout", /*???*/
  /*    13 */ "can't freopen() `%s' onto stderr", /*???*/
  /*    14 */ "", /* avail */
  /*    15 */ "", /* avail */
  /*    16 */ "", /* avail */
  /*    17 */ "r\351pertoire initial introuvable!",
  /*    18 */ "ne peut pas chdir() au r\351pertoire initial, `%s'",
  /*    19 */ ".", /*X*/
  /*    20 */ "ne peut pas chroot() au r\351pertoire initial",
  /*    21 */ "setuid() \351chou\351",
  /*    22 */ "%s/%s", /*X*/
  /*    23 */ "ne peut pas obtenir nouveau contexte historique.",
  /*    24 */ "# Version fran\347aise r\351alis\351e en collaboration avec le\n# Service des T\351l\351communications, Universit\351 du Qu\351bec \340 Montr\351al.\n\n",
  /*    25 */ "stty", /*X*/
  /*    26 */ "# Bunyip Information Systems, Inc., 1993, 1994, 1995\n\n", /*X*/
  /*    27 */ "", /*X*/
  /*    28 */ "\n", /*X*/
  /*    29 */ "# D\351sol\351, cette commande a \351t\351 mise hors service par le gestionnaire.\n",
  /*    30 */ "# Mode erron\351 `%s' pour l'instruction `%s'.\n",
  /*    31 */ "\n>> %s\n", /*X*/
  /*    32 */ "help", /*X*/
  /*    33 */ "mail", /*X*/
  /*    34 */ "# Au revoir.\n",
  /*    35 */ "# Cette instruction ne s'utilise qu'avec la commande `aide'.\n",
  /*    36 */ "# Pr\351fixe d'instruction multiple.\n",
  /*    37 */ "# D\351sol\351, la commande `site' n'est pas disponible.\n",
  /*    38 */ "# %s\n", /*X*/
  /*    39 */ "# Commande `%s' inconnue en mode %s.\n",
  /*    40 */ "disable_command", /*X*/
  /*    41 */ "`%s' exige au moins un param\350tre.",
  /*    42 */ "# Paginer \351chou\351; impression directement sur l'\351cran.\n",
  /*    43 */ "init_from_file", /*X*/
  /*    44 */ "r", /*X*/
  /*    45 */ "man_it", /*X*/
  /*    46 */ "# `%s' ne permet qu'un seul param\350tre.\n",
  /*    47 */ "roff", /*X*/
  /*    48 */ "code de retour inconnu de fork_me().",
  /*    49 */ "whatis_it", /*X*/
  /*    50 */ "new_tmp_file", /*X*/
  /*    51 */ "erreur en ouvrant fichier tmp `%s'",
  /*    52 */ "tempnam() a \351chou\351.",
  /*    53 */ "# `%s' exige au moins un param\350tre.\n",
  /*    54 */ "# les variables bool\351ennes ne permettent aucun param\350tre.\n",
  /*    55 */ "# les variables num\351riques ne permettent qu'un seul param\350tre.\n",
  /*    56 */ "# les variables de cha\356ne exigent un param\350tre.\n",
  /*    57 */ " ", /*X*/
  /*    58 */ "# `%s' est une variable inconnue.\n",
  /*    59 */ "set_it", /*X*/
  /*    60 */ "type de variable inconnu `%d'.",
  /*    61 */ "remove_tmp_file", /*X*/
  /*    62 */ "erreur en supprimant la liaison `%s'",
  /*    63 */ "# D\351sol\351, liste des serveurs introuvable.\n",
	/*    64 */	"Usage: %s [-d <#>] [-e] [-i <init-file>] [-l] [-L <logfile>] [-p <#>] [-s]\n",

/* ==================== argv.c ==================== */

  /*    65 */ "impossible d'attribuer un indicateur initial",
  /*    66 */ "new_str", /*X*/
  /*    67 */ "erreur rmalloc() `%d' bits ",
  /*    68 */ "argvflatten", /*X*/
  /*    69 */ "erreur malloc() `%u' bits ",
  /*    70 */ "argvify", /*X*/
  /*    71 */ "n'a pas r\351ussi  %s.",
  /*    72 */ "initialiser argv",
  /*    73 */ "commencer une autre cha\356ne",
  /*    74 */ "rentrer un caract\350re",
  /*    75 */ "finir la cha\356ne actuelle",
  /*    76 */ "fini en mauvais \351tat, `%d'.",

/* ==================== commands.c ==================== */

  /*    77 */ "cmd_disable", /*X*/
  /*    78 */ "`%s': %s", /*X*/
  /*    79 */ "get_cmd", /*X*/

/* ==================== error.c ==================== */

  /*    80 */ "erreur: %s.\n",
  /*    81 */ "ERREUR",
  /*    82 */ "INFO",
  /*    83 */ "ERREUR INTERNE",
  /*    84 */ "ERREUR D'APPEL DE PROGRAMME",
  /*    85 */ "MISE EN GARDE",
  /*    86 */ "# ERREUR INTERNE: erreur: type d'erreur inconnu, %d.\n",
  /*    87 */ "# %s: %s: %s", /*X*/
  /*    88 */ ": erreur non r\351pertori\351e",
  /*    89 */ ": ", /*X*/
  /*    90 */ "%s  %s\n", /*X*/
  /*    91 */ "/tmp/lll", /*X*/
  /*    92 */ "w", /*X*/

/* ==================== find.c ==================== */

  /*    93 */ "DIRECTORY", /*X*/
  /*    94 */ "%s %s %s %s %s%s\n", /*X*/
  /*    95 */ "LAST-MODIFIED", /*X*/
  /*    96 */ "SIZE", /*X*/
  /*    97 */ "UNIX-MODES", /*X*/
  /*    98 */ "/", /*X*/
  /*    99 */ "%R %e %h %Y", /*X*/
  /*   100 */ "%s  %12s %7s %s%s\n", /*X*/
  /*   101 */ "AR_H_LAST_MOD", /*X*/
  /*   102 */ "Ordinateur central %s    (%s)\n",
  /*   103 */ "AR_H_IP_ADDR", /*X*/
  /*   104 */ "Derni\350re mise \340 jour %s\n",
  /*   105 */ "\n    Endroit: %s\n",
  /*   106 */ "      R\311PERTOIRE    %s %13s  %12s  %s\n",
  /*   107 */ "      FICHIER    %s %13s  %12s  %s\n",
  /*   108 */ "fprint_list_item", /*X*/
  /*   109 */ "type de style de donn\351es inconnu `%d'.",
  /*   110 */ "do_matches", /*X*/
  /*   111 */ "pager", /*X*/
  /*   112 */ "verbose",
  /*   113 */ "terse",
  /*   114 */ "machine",
  /*   115 */ "# Correspondances introuvables.\n",
  /*   116 */ "\n# Erreur caus\351e par le serveur Prospero - ",
  /*   117 */ "find_it", /*X*/

/* ==================== fork_wait.c ==================== */

  /*   118 */ "en cours...  ",
  /*   119 */ "# ERREUR: fork_me: fork: ",
  /*   120 */ "fork_me", /*X*/
  /*   121 */ "erreur en attendant un processus secondaire.", /*???*/

/* ==================== get_types.c ==================== */


/* ==================== help.c ==================== */

  /*   122 */ "aide%s> ",
  /*   123 */ "%s/=", /*X*/
  /*   124 */ "display_help", /*X*/
  /*   125 */ "erreur access() pour le fichier `%s'",
  /*   126 */ "list_subtopics", /*X*/
  /*   127 */ "impossible d'ouvrir le r\351pertoire `%s'.",
  /*   128 */ "# Rubriques\n#\n",
  /*   129 */ "#\t%s\n", /*X*/
  /*   130 */ "# Aucun rubrique.\n", /*X*/
  /*   131 */ "pop_topic", /*X*/
  /*   132 */ "`/' introuvable dans la structure actuelle de r\351pertoires.",
  /*   133 */ "push_topics", /*X*/
  /*   134 */ "# `%s' ne semble pas exister.\n",
  /*   135 */ "help_", /*X*/
  /*   136 */ "fichier d'aide introuvable; consultez-vous le bon r\351pertoire?",
  /*   137 */ "?", /*X*/
  /*   138 */ "ex\351cut\351",
  /*   139 */ "fichier introuvable avec l'aide du courrier \351.",
  /*   140 */ "erreur en tentant d'obtenir un nouveau contexte historique.", /*???*/

/* ==================== input.c ==================== */

  /*   141 */ ".\n", /*X*/
  /*   142 */ "readline", /*X*/
  /*   143 */ "impossible de malloc()er `%d' bits pour une nouvelle ligne.",
  /*   144 */ "erreur de lecture sur `stdin'.",
  /*   145 */ "valeur de retour inattendue pour fread().",
  /*   146 */ "erreur en realloc()ant `%d' bits pour la ligne d'entr\351e.",

/* ==================== lang.c ==================== */


/* ==================== list.c ==================== */

  /*   147 */ "argument `article' est NUL.",
  /*   148 */ "%s %s\n", /*X*/
  /*   149 */ "%-40s %15s     %17s\n", /*X*/
  /*   150 */ "fprint_list_item", /*X*/
  /*   151 */ ".*", /*X*/
  /*   152 */ "list_it", /*X*/

/* ==================== mail.c ==================== */

  /*   153 */ "mail_it", /*X*/
  /*   154 */ "envoyer\340 ",
  /*   155 */ "# Vous devez pr\351ciser une adresse de courrier ou fixer la valeur de la variable 'envoyer\340'.\n",
  /*   156 */ "# `%s' ne prend qu'un seul param\350tre.\n",
  /*   157 */ "# Vous n'avez pas encore de r\351sultats \340 envoyer.\n",
  /*   158 */ "aucun",
  /*   159 */ "# Si vous pr\351cisez compression, vous devez aussi pr\351ciser un encodage. \n",
  /*   160 */ "num\351ro de porte d'acc\350s, `%s', invalide.",
  /*   161 */ "getservbyname() \351chou\351 -- porte d'acc\350s introuvable pour `%s'.",
  /*   162 */ "liaison avec l'ordinateur central impossible: `%s'.",
  /*   163 */ "error fdopen()ing socket to the mail host.", /*???*/
  /*   164 */ "@Begin\n", /*X*/
  /*   165 */ "Command: %s\n", /*X*/
  /*   166 */ "Compress: %s\n", /*X*/
  /*   167 */ "Encode: %s\n", /*X*/
  /*   168 */ "MaxSplitSize: %d\n", /*X*/
  /*   169 */ "@MailHeader\n", /*X*/
  /*   170 */ "To: %s\n", /*X*/
  /*   171 */ "From: %s@%s\n", /*X*/
  /*   172 */ "Reply-To: %s@%s\n", /*X*/
  /*   173 */ "Date: %s\n", /*X*/
  /*   174 */ "@End\n", /*X*/
  /*   175 */ "fputs() est EOF, errno est %d.\n",
  /*   176 */ "\351chec! errno = %d.\n",
  /*   177 */ "ferror(mfp) est %d, feof(mfp) est %d.\n",
  /*   178 */ "ferror(ofp) est %d, feof(ofp) est %d.\n",

/* ==================== misc.c ==================== */

  /*   179 */ "<sans valeur>",
  /*   180 */ "first_word_of", /*X*/
  /*   181 */ "dequote", /*X*/
  /*   182 */ "nuke_newline", /*X*/
  /*   183 */ "l'argument est NUL.",
  /*   184 */ "Nom: `%s', Valeur: `%s'.\n",
  /*   185 */ "print_file", /*X*/
  /*   186 */ "erreur \340 partir de fgets() sur `%s'",
  /*   187 */ "%4u%2u%2u%2u%2u%2u", /*X*/
  /*   188 */ "cvt_to_inttime", /*X*/
  /*   189 */ "erreur de conversion de temps `%s'.",
  /*   190 */ "prosp_strftime", /*X*/
  /*   191 */ "prosp_strftime() \351chou\351 sur `%s'.",
  /*   192 */ "<temps erron\351>",
  /*   193 */ "size_of_file", /*X*/
  /*   194 */ "impossible d'obtenir la taille du fichier du courrier `%s'.",
  /*   195 */ "snarf_n_barf", /*X*/
  /*   196 */ "\351chec dans fputs()",
  /*   197 */ "fgets() a \351chou\351",
  /*   198 */ "squeeze_whitespace", /*X*/
  /*   199 */ "argument `str' est NUL.",
  /*   200 */ "initskip", /*X*/
  /*   201 */ "impossible de malloc()er %d bits pour combinaison.",
  /*   202 */ "impossible de realloc()er %d bits pour combinaison.",
  /*   203 */ "# `%s' n\351cessite un param\350tre.\n",
  /*   204 */ "copy_first_word", /*X*/

/* ==================== mode.c ==================== */

  /*   205 */ "<aucun mode>",
  /*   206 */ "<mode erron\351>",
  /*   207 */ "set_mode", /*X*/
  /*   208 */ "mode `0x%08x inconnu'; remplacer par mode %s.",

/* ==================== pager.c ==================== */

  /*   209 */ "close_pager_file", /*X*/
  /*   210 */ "erreur \340 partir de fclose().",
  /*   211 */ "create_pager_file", /*X*/
  /*   212 */ "impossible de cr\351er le fichier temporaire.",
  /*   213 */ "impossible d'ouvrir un fichier temporaire cr\351\351 r\351cemment.",
  /*   214 */ "fopen() a \351chou\351 sur le fichier paginer.",
  /*   215 */ "erreur provenant de fork().",
  /*   216 */ "`%s' est NUL.",
  /*   217 */ "execl pour pager `%s' a \351chou\351",
  /*   218 */ "remove_pager_file", /*X*/
  /*   219 */ "argument `pager_file_name' est NUL.",
  /*   220 */ "erreur en effa\347ant un fichier temporaire.",
  /*   221 */ "set_pager_opts", /*X*/
  /*   222 */ "argument `val' est NUL.",

/* ==================== pexec.c ==================== */

  /*   223 */ "make_argv", /*X*/
  /*   224 */ "liste d'arguments ne se termine pas par NUL.",
  /*   225 */ "p_close", /*X*/
  /*   226 */ "a essay\351 d'attendre un processus inexistant.",
  /*   227 */ "le param\350tre ne correspond \340 aucun canal de communication.",
  /*   228 */ "p_execvp", /*X*/
  /*   229 */ "# ERREUR: p_execvp: pipe: ",
  /*   230 */ "# ERREUR: p_execvp: fdopen: ",
  /*   231 */ "# ERREUR: p_execvp: fdopen ",
  /*   232 */ "genre de param\350tre \340 p_execvp() est `%s'.",
  /*   233 */ "# ERREUR: pexecvp: fork ",
  /*   234 */ "# ERREUR: p_execvp: dup2(p[1], 1) ",
  /*   235 */ "# ERREUR: p_execvp: dup2(p[0], 0)",
  /*   236 */ "# ERREUR: p_execvp: execvp",

/* ==================== prospero_fix.c ==================== */


/* ==================== rmem.c ==================== */

  /*   237 */ "rfree %p\n", /*X*/
  /*   238 */ "rmalloc: %p\n", /*X*/
  /*   239 */ "encore %p!\n",
  /*   240 */ "positionner foo \340 %p.\n",

/* ==================== signals.c ==================== */

  /*   241 */ "catch_alarm", /*X*/
  /*   242 */ "marche",
  /*   243 */ "n'attend pas de processus secondaire",
  /*   244 */ "\n# auto-logout\n",
  /*   245 */ "attend un processus secondaire",
  /*   246 */ "`get_var' redevenu nul.",
  /*   247 */ "vous pouvez attendre -- il reste %d secs",
  /*   248 */ "\n# Suspendu.\n",

/* ==================== strmap.c ==================== */


/* ==================== terminal.c ==================== */

  /*   249 */ "get_key_char", /*X*/
  /*   250 */ "# s'attendait \340 seul caract\350re ou `^<char>', plut\364t que `%s'.\n",
  /*   251 */ "# `%s' (0x%02x) ne constitue pas un caract\350re de contr\364le valide.\n",
  /*   252 */ "get_key_str", /*X*/
  /*   253 */ "le tampon `%s' est trop petit.",
  /*   254 */ "buf", /*X*/
  /*   255 */ "la valeur de '%s' (`%lu') est hors limites.",
  /*   256 */ "ch", /*X*/
  /*   257 */ "^?", /*X*/
  /*   258 */ "^%c", /*X*/
  /*   259 */ "\\%03o", /*X*/
  /*   260 */ "set_window_size", /*X*/
  /*   261 */ "impossible d'obtenir une structure winsize",
  /*   262 */ "impossible de r\351gler une structure winsize",
  /*   263 */ "set_term_type", /*X*/
  /*   264 */ "# impossible d'ouvrir le fichier de description des terminaux.\n",
  /*   265 */ "# type de termnial `%s' inconnu de ce syst\350me, essai de `dumb'.\n",
  /*   266 */ "dumb", /*X*/
  /*   267 */ "tgetent() est redevenu une valeur non document\351e; essai de `dumb'.",
  /*   268 */ "TERM=", /*X*/
  /*   269 */ "erreur provenant de putenv().",
  /*   270 */ "TERM", /*X*/
  /*   271 */ "init_term", /*X*/
  /*   272 */ "%s %d %d", /*X*/
  /*   273 */ "%s %s %s", /*X*/
  /*   274 */ "%d", /*X*/
  /*   275 */ "# le nombre de colonnes, `%s', doit \352tre une valeur num\351rique.\n",
  /*   276 */ "# le nombre de lignes, `%s', doit \352tre une valeur num\351rique.\n",
  /*   277 */ "# le format de commande est `term <term-type> [<#lignes> [<#colonnes>]]'.\n",
  /*   278 */ "set_term", /*X*/
  /*   279 */ "erreur dans la capacit\351 de m\351moire du terminal.",
  /*   280 */ "# Type de terminal r\351gl\351 \340 `%s %d %d'.\n",
  /*   281 */ "set_tty", /*X*/
  /*   282 */ "# `%s' exige deux param\350tres; voir aide sur `%s' pour mode d'emploi.\n",
  /*   283 */ "erreur en obtenant la structure termios",
  /*   284 */ "# caract\350re `erase' est `%s'.\n",
  /*   285 */ "# `%s' n'est pas un param\350tre valide; consulter l'aide sur `%s' pour conna\356tre le mode d'emploi.\n",
  /*   286 */ "erreur en r\351glant la structure termios",

/* ==================== vars.c ==================== */

  /*   287 */ "# `%s' ne peut se r\351gler en mode %s.\n",
  /*   288 */ "# `%s' est en lecture seulement; sa valeur ne peut pas changer.\n",
  /*   289 */ "# `%s' n'est pas une valeur valide, `aide fixer %s' pour obtenir une liste.\n",
  /*   290 */ "# La valeur doit \352tre un nombre entier.\n",
  /*   291 */ "# La valeur doit \352tre comprise dans la gamme [%d,%d].\n",
  /*   292 */ "set_var_", /*X*/
  /*   293 */ "impossible de fixer une nouvelle valeur.",
  /*   294 */ "# La valeur est trop longue (> %d bits).\n",
  /*   295 */ "set_var", /*X*/
  /*   296 */ "variable de type inconnu.",
  /*   297 */ "# `%s' n'est pas une variable connue en mode `%s'.\n",
  /*   298 */ "bool\351en",
  /*   299 */ "# `%s' (type %s) est fix\351.\n",
  /*   300 */ "# `%s' (type %s) n'est pas fix\351.\n",
  /*   301 */ "num\351rique",
  /*   302 */ "cha\356ne",
  /*   303 */ "show_var", /*X*/
  /*   304 */ "variable `%s' de type inconnu.",
  /*   305 */ "# `%s' (type %s) a la valeur `%s'.\n",
  /*   306 */ "# `%s' ne peut s'effacer -- il doit toujours avoir une valeur.\n",
  /*   307 */ "english", /*X*/
  /*   308 */ "fran\347ais", /*X*/
  /*   309 */ "# MISE EN GARDE: les messages ne seront pas en `%s'.\n",
  /*   310 */ "# impossible de r\351gler le langage \340 `%s'. Aucun r\351pertoire d'aide.\n",

/* ==================== version.c ==================== */


/* ==================== whatis.c ==================== */

  /*   311 */ "get_whatis_list", /*X*/
  /*   312 */ "impossible d'ouvrir la base de donn\351es de description, `%s'.",
  /*   313 */ "%-25s %-54s\n", /*X*/
  /*   314 */ "# N'a rien trouv\351 qui correspond \340 `%s'.\n",

/* ==================== EXTRA ==================== */

  /*   315 */ "# `%s' n'exige pas des param\350tres.\n",
  /*   316 */ "erreur ftruncate() sur le fichier temporaire `%s'",
  /*   317 */ "# `%s' exige au moins deux param\350tres.\n",
  /*   318 */ "# `%s' n'est pas une sous-commande valide.\n"
  /*   319 */ "# erreur en fixant l'adresse.\n"

  /*   320 */     "# Type de recherche: %s", /* ??? */
  /*   321 */     "%sDomaine: %s",
  /*   322 */     "%sVoie d'acc\350s: %s",
  /*   323 */	  "child terminated abnormally with signal %d." /*FFF*/,
  /*   326 */	  "Precedence: junk\n"
};
