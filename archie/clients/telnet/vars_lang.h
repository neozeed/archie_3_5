#include <limits.h>
#include "debug.h"
#include "terminal.h"
#include "vars.h"
#include "macros.h"
#include "strmap.h"


#define ALLOW	  1
#define DISALLOW  0
#define HIDDEN    1
#define NOFUNC    ((int (*)())0)
#define NOLIST	  {(char *)0}
#define ENDMAP    {(const char *)0, (const char *)0}
#define NOMAP     {ENDMAP}
#define VAR_SET   1
#define VAR_UNSET 0
#define VISIBLE   0


/* 
 *  'max_val' and 'min_val' only have meaning for variables of NUMERIC type.
 *  They contain the largest and smallest values that variable may have.
 */ 

struct var_desc_s
{
  const StrMap name[2];
  enum var_type_e type;
  int is_set;
  int allow_chval;              /* can we change its value? (applies to set & unset vars) */
  int allow_unset;
  int has_been_reset;           /* initially 0; 1 after first change of value (for free()ing) */
  int is_hidden;		/* show will not display when set */
  int modes;

  /*
    The string representation of the value only has meaning for variables of
    NUMERIC and STRING type.
  */

  StrMap val[2];

  /*
    These are pointers to functions returning int.  If 'doit' is non-null it
    is performed when the variable is set.  If 'undoit' is non-null it is
    performed when the variable is unset.  'doit' takes one argument, the
    string representation of the value to be set, while 'undoit' takes no
    arguments.
  */

  int (*const doit)();		/* one argument, the value as a string */
  int (*const undoit)();		/* this is getting out of hand */

  /*
    The following two values only have meaning for variables of NUMERIC type.
  */

  const int min_val;
  const int max_val;

  /*
    The following only has meaning for variables of STRING type.

    A null terminated list of values that the variable may have.  If the
    pointer itself is null, then the variable may take any value.
  */

  const StrMap valid_list[16];
};


static struct var_desc_s vars[] =
{
  {
    {
      {V_AUTOLOGOUT, FRENCH("sortie_automatique")},
      ENDMAP
    },
    NUMERIC, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"60"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 1, 300, NOMAP
  },
#ifdef MULTIPLE_COLLECTIONS
  {
    {
      {V_COLLECTIONS, FRENCH("collectes")},
      ENDMAP
    },
    STRING, VAR_UNSET, ALLOW, ALLOW, VAR_UNSET, VISIBLE, M_ALL, NOMAP, NOFUNC, NOFUNC, 0, 0, NOMAP
  },
#endif
  {
    {
      {V_COMPRESS, FRENCH("condenser")},
      ENDMAP
    },
    STRING, VAR_SET, ALLOW, ALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"none", FRENCH("aucun")},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    {
      {"compress"},
      {"none", FRENCH("aucun")},
      ENDMAP
    }
  },
  {
    {
      {V_DEBUG, FRENCH("debogage")},
      ENDMAP
    },
    NUMERIC, VAR_UNSET, ALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_ALL,
    {
      {"0"},
      ENDMAP
    },
    set_debug, unset_debug, 0, INT_MAX, NOMAP
  },
  {
    {{V_EMAIL_HELP_FILE}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_SYS_RC,
    {
      {"etc/email.help"},
      ENDMAP
    },
    NOFUNC, NOFUNC,
    0, 0, NOMAP
  },
  {
    {
      {V_ENCODE, FRENCH("encodage")},
      ENDMAP
    },
    STRING, VAR_SET, ALLOW, ALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"none", FRENCH("aucun")},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    {
      {"none", FRENCH("aucun")},
      {"uuencode"},
      ENDMAP
    }
  },
#ifdef MULTI_LING
  {
    {{V_HELP_DIR}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_ALL,
    {
      {"help/francais"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
#else
  {
    {{V_HELP_DIR}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_ALL,
    {
      {"help/english"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
#endif
#ifdef MULTI_LING
  {
    {
      {V_LANGUAGE, FRENCH("langue")},
      ENDMAP
    },
    STRING, VAR_SET, ALLOW, ALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"francais"},
      ENDMAP
    },
    change_lang, NOFUNC, 0, 0,
    {
      {"francais"},
      {"english"},
      ENDMAP
    }
  },
#else
  {
    {
      {V_LANGUAGE},
      ENDMAP
    },
    STRING, VAR_SET, ALLOW, ALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"english"},
      ENDMAP
    },
    change_lang, NOFUNC, 0, 0,
    {
#if 0
      {"english"},
#endif
      ENDMAP
    }
  },
#endif
  {
    {{V_MAIL_FROM}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_SYS_RC,
    {
      {"(Archie Server) archie-errors", FRENCH("(Serveur archie) archie-errors")},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0, NOMAP
  },
  {
    {{V_MAIL_HOST}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_SYS_RC,
    {
      {"localhost"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
  {
    {{V_MAIL_SERVICE}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_SYS_RC,
    {
      {"archiemail"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
  {
    {
      {V_MAILTO, FRENCH("adresse")},
      ENDMAP
    },
    STRING, VAR_UNSET, ALLOW, ALLOW, VAR_UNSET, VISIBLE, M_EIU, NOMAP, NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
  {
    {{V_MAN_ASCII_FILE}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_SYS_RC,
    {
      {"etc/manpage.ascii"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0, NOMAP
  },
  {
    {{V_MAN_ROFF_FILE}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_SYS_RC,
    {
      {"etc/manpage.roff"},
      ENDMAP
    },
    NOFUNC, NOFUNC,
    0, 0, NOMAP
  },
  {
    {
      {V_MATCH_DOMAIN, FRENCH("match_domain")},
      ENDMAP
    },
    STRING, VAR_UNSET, ALLOW, ALLOW, VAR_UNSET, VISIBLE, M_ALL, NOMAP, NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
  {
    {
      {V_MATCH_PATH, FRENCH("match_path")},
      ENDMAP
    },
    STRING, VAR_UNSET, ALLOW, ALLOW, VAR_UNSET, VISIBLE, M_ALL, NOMAP, NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
  {
    {{V_MAX_SPLIT_SIZE}, ENDMAP},
    NUMERIC, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"51200"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 1024,
    INT_MAX, NOMAP
  },
#ifdef MULTIPLE_COLLECTIONS
  { /* WAIS */
    {
      {V_MAXDOCS, FRENCH("maxdocs")},
      ENDMAP
    },
    NUMERIC, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"10"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 1000, NOMAP
  },
  { /* WAIS */
    {
      {V_MAXHDRS, FRENCH("maxhdrs")},
      ENDMAP
    },
    NUMERIC, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"1000"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 1000, NOMAP
  },
#endif
  { /* archie */
    {
      {V_MAXHITS, FRENCH("maxhits")}, /*bug: xlation?*/
      ENDMAP
    },
    NUMERIC, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"100"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 1000, NOMAP
  },
  { /* archie */
    {
      {V_MAXHITSPM, FRENCH("maxhitspm")}, /*bug: xlation?*/
      ENDMAP
    },
    NUMERIC, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"100"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 1000, NOMAP
  },
  { /* archie */
    {
      {V_MAXMATCH, FRENCH("maxmatch")}, /*bug: xlation?*/
      ENDMAP
    },
    NUMERIC, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"100"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 1000, NOMAP
  },
#if 1
  {
    {{V_MOTD_FILE}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_SYS_RC,
    {
      {"etc/motd.telnet"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
#endif
  {
    {{V_NICENESS}, ENDMAP},
    NUMERIC, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_SYS_RC,
    {
      {"4"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 19, -20, NOMAP
  },
  {
    {
      {V_OUTPUT_FORMAT, FRENCH("format_des_resultats")},
      ENDMAP
    },
    STRING, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"verbose", FRENCH("verbeux")},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    {
      {"machine", FRENCH("ordinateur")},
      {"silent", FRENCH("silencieux")},
      {"terse", FRENCH("breve")},
      {"verbose", FRENCH("verbeux")},
      {"url",FRENCH("url")},
      ENDMAP
    }
  },
  {
    {
      {V_PAGER, FRENCH("paginer")},
      ENDMAP
    },
    BOOLEAN, VAR_UNSET, ALLOW, ALLOW, VAR_UNSET, VISIBLE, M_ALL, NOMAP, NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
  {
    {{V_PAGER_HELP_OPTS}, ENDMAP},
    STRING, VAR_SET, ALLOW, ALLOW, VAR_UNSET, HIDDEN, M_ALL,
    {
      {"-c"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0, NOMAP
  },
  {
    {{V_PAGER_OPTS}, ENDMAP},
    STRING, VAR_SET, ALLOW, ALLOW, VAR_UNSET, HIDDEN, M_ALL,
    {
      {"-c"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0, NOMAP
  },
  {
    {{V_PAGER_PATH}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_ALL,
    {
      {"less"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0, NOMAP
  },
  {
    {{V_PROMPT}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_ALL,
    {
      {"archie> "},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0, NOMAP
  },
  {
    {
      {V_SEARCH, FRENCH("chercher")},
      ENDMAP
    },
    STRING, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"exact"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    {
      {"exact", FRENCH("exact")},
      {"regex", FRENCH("regex")},
      {"sub", FRENCH("sub")},
      {"subcase", FRENCH("subcase")},
      ENDMAP
    }
  },
  {
    {{V_SERVER, FRENCH("serveur")}, ENDMAP},
    STRING, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"localhost"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0, NOMAP
  },
  {
    {{V_SERVERS_FILE}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_SYS_RC,
    {
      {"etc/serverlist"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
  {
    {
      {V_SORTBY, FRENCH("trier_par")},
      ENDMAP
    },
    STRING, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"none", FRENCH("aucun")},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    {
      {"filename", FRENCH("nom_du_fichier")},
      {"hostname", FRENCH("nom_d'hote")},
      {"none", FRENCH("aucun")},
      {"size", FRENCH("taille")},
      {"time", FRENCH("temps")},
      {"rfilename", FRENCH("inv_nom_du_fichier")},
      {"rhostname", FRENCH("inv_nom_d'hote")},
      {"rnothing", FRENCH("inv_aucun")},
      {"rsize", FRENCH("inv_taille")},
      {"rtime", FRENCH("inv_temps")}, /*bug: xlation?*/
      ENDMAP
    }
  },
  {
    {
      {V_STATUS, FRENCH("statut")},
      ENDMAP
    },
    BOOLEAN, VAR_SET, ALLOW, ALLOW, VAR_UNSET, VISIBLE, M_ALL, NOMAP, NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },
  {
    {
      {V_TERM},
      ENDMAP
    },
    STRING, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, VISIBLE, M_ALL,
    {
      {"dumb 24 80"},
      ENDMAP
    },
    set_term, NOFUNC, 0, 0,
    NOMAP
  },
  {
    {
      {V_TMPDIR},
      ENDMAP
    },
    STRING, VAR_SET, ALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_ALL,
    {
      {"db/tmp"}, /* so as to use ~archie/db/tmp whether suid or not */
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0, NOMAP
  },
  {
    {{V_WHATIS_FILE}, ENDMAP},
    STRING, VAR_SET, DISALLOW, DISALLOW, VAR_UNSET, HIDDEN, M_SYS_RC,
    {
      {"etc/whatis"},
      ENDMAP
    },
    NOFUNC, NOFUNC, 0, 0,
    NOMAP
  },

  /* End of the list */

  {
    NOMAP,
    VAR_BAD_TYPE, VAR_UNSET, ALLOW, ALLOW, VAR_UNSET, HIDDEN, M_NONE, NOMAP, NOFUNC, NOFUNC,
    0, 0, NOMAP
  }
};
