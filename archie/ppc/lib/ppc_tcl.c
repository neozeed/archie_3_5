#ifndef NO_TCL
#include <ctype.h>
#include <string.h>
#if defined(SOLARIS) || defined(AIX)
#include <signal.h>
#endif
#include "error.h"
#include "misc.h"
#include "os_indep.h"
#include "pattrib.h"
#include "ppc_tcl.h"
#include "psearch.h"
#include "quoting.h"
#include "str.h"
#include "tcl.h"


static Tcl_Interp *interp = 0;
static char config_file[MAXPATHLEN+1];
static volatile reload_config = 0;


/*
 *
 *
 *                            Internal routines
 *
 *
 */  


static char *vlinksString proto_((VLINK v));
static int Ppc_DequoteCmd proto_((ClientData clientData, Tcl_Interp *interp, int ac, char *av[]));
static int Ppc_QuoteCmd proto_((ClientData clientData, Tcl_Interp *interp, int ac, char *av[]));
static int Ppc_ReNoCaseCmd proto_((ClientData clientData, Tcl_Interp *interp, int ac, char *av[]));
static int load_config proto_((const char *file));
static int tcl_configured proto_((void));
static void sig_usr1 proto_((int signo));


/*  
 *  Taken from tcl-dp/prsp.c on Tue Oct 25 16:18:24 EDT 1994.
 */  
/*  
 *  Return a string corresponding to the list, v, of VLINKs.
 *  
 *  Each list element (corresponding to one VLINK) has the form
 *  
 *  {{NAME name} {HOST host} ... {ATTRIBUTES ?<attr>? ...} {ACLS ?<acl>?
 *  ...}}
 *  
 *  where <attr> has the form
 *  
 *  {atname ?atval? ...}
 *  
 *  Currently the list of ACLs is empty.
 */            
static char *vlinksString(v)
  VLINK v;
{
  Tcl_DString vstr;
  char *res;
  
  Tcl_DStringInit(&vstr);
  while(v)
  {
    PATTRIB at;

    Tcl_DStringStartSublist(&vstr);

    /* Link name */
    Tcl_DStringStartSublist(&vstr);
    Tcl_DStringAppendElement(&vstr, "NAME");
    Tcl_DStringAppendElement(&vstr, v->name);
    Tcl_DStringEndSublist(&vstr);

    /* Host */
    Tcl_DStringStartSublist(&vstr);
    Tcl_DStringAppendElement(&vstr, "HOST");
    Tcl_DStringAppendElement(&vstr, v->host);
    Tcl_DStringEndSublist(&vstr);

    /* Host type */
    Tcl_DStringStartSublist(&vstr);
    Tcl_DStringAppendElement(&vstr, "HOSTTYPE");
    Tcl_DStringAppendElement(&vstr, v->hosttype);
    Tcl_DStringEndSublist(&vstr);

    /* Hsoname */
    Tcl_DStringStartSublist(&vstr);
    Tcl_DStringAppendElement(&vstr, "HSONAME");
    Tcl_DStringAppendElement(&vstr, v->hsoname);
    Tcl_DStringEndSublist(&vstr);

    /* Hsoname type */
    Tcl_DStringStartSublist(&vstr);
    Tcl_DStringAppendElement(&vstr, "HSONAMETYPE");
    Tcl_DStringAppendElement(&vstr, v->hsonametype);
    Tcl_DStringEndSublist(&vstr);

    /* Link type */
    Tcl_DStringStartSublist(&vstr);
    Tcl_DStringAppendElement(&vstr, "LINKTYPE");
    {
      char *t;
      switch (v->linktype)
      {
      case '\0': t = "<NULL>"   ; break;
      case 'L' : t = "LINK"     ; break;
      case 'U' : t = "UNION"    ; break;
      case 'N' : t = "NATIVE"   ; break;
      case 'I' : t = "INVISIBLE"; break;
      default:
        {
          static char l[2];
          l[0] = v->linktype; l[1] = '\0';
          t = l;
        }
      }
      Tcl_DStringAppendElement(&vstr, t);
    }
    Tcl_DStringEndSublist(&vstr);

    /* Target */
    Tcl_DStringStartSublist(&vstr);
    Tcl_DStringAppendElement(&vstr, "TARGET");
    Tcl_DStringAppendElement(&vstr, v->target);
    Tcl_DStringEndSublist(&vstr);

    /* Attributes */
    Tcl_DStringStartSublist(&vstr);
    Tcl_DStringAppendElement(&vstr, "ATTRIBUTES");
    for (at = v->lattrib; at; at = at->next)
    {
      TOKEN t;
        
      Tcl_DStringStartSublist(&vstr);
      Tcl_DStringAppendElement(&vstr, at->aname);
      for (t = at->value.sequence; t; t = t->next)
      {
        Tcl_DStringAppendElement(&vstr, t->token);
      }
      Tcl_DStringEndSublist(&vstr);
    }
    Tcl_DStringEndSublist(&vstr);

    /* ACLs: empty for now */
    Tcl_DStringStartSublist(&vstr);
    Tcl_DStringAppendElement(&vstr, "ACLS");
    Tcl_DStringEndSublist(&vstr);

    Tcl_DStringEndSublist(&vstr);

    v = v->next;
  }
  
  res = strdup(Tcl_DStringValue(&vstr));
  Tcl_DStringFree(&vstr);

  return res;
}


#if 1
/*  
 *  Taken from www_safe on Sep 8 1994.
 */  
static char ppc_safe[256] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*   0 -  15 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  16 -  31 */
  0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, /*  32 -  47 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, /*  48 -  63 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  64 -  79 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /*  80 -  95 */
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  96 - 111 */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 112 - 127 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 128 - 143 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 144 - 159 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 160 - 175 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 176 - 191 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 192 - 207 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 208 - 223 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 224 - 239 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* 240 - 255 */
};


/*  
 *  Taken from tcl-dp/prsp.c on Sun Nov 6 21:52:11 EST 1994.
 */  
/*  
 *  Dequote the argument according to WWW (URL?) conventions.  (I.e. The
 *  string "%HH" corresponds to the character 0xHH, where the Hs are
 *  hexadecimal numbers.)
 */      
static int Ppc_DequoteCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  char *s;

  if (ac != 2)
  {
    Tcl_SetResult(interp, "usage: ppc_dequote string", TCL_STATIC);
    return TCL_ERROR;
  }

  if ( ! (s = dequote_string(av[1], ppc_safe, malloc)))
  {
    Tcl_SetResult(interp, "memory error?", TCL_STATIC);
    return TCL_ERROR;
  }

  Tcl_SetResult(interp, s, TCL_DYNAMIC);
  return TCL_OK;
}


/*  
 *  Taken from tcl-dp/prsp.c on Sun Nov 6 21:53:04 EST 1994.
 */  
/*  
 *  Quote the argument according to WWW (URL?) conventions.  (I.e. The
 *  character 0xHH becomes the string "%HH", where the Hs are hexadecimal
 *  numbers.)
 */        
static int Ppc_QuoteCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  char *s;

  if (ac != 2)
  {
    Tcl_SetResult(interp, "usage: ppc_quote string", TCL_STATIC);
    return TCL_ERROR;
  }

  if ( ! (s = quote_string(av[1], ppc_safe, malloc)))
  {
    Tcl_SetResult(interp, "memory error?", TCL_STATIC);
    return TCL_ERROR;
  }

  Tcl_SetResult(interp, s, TCL_DYNAMIC);
  return TCL_OK;
}
#endif


/*  
 *  Taken from tcl-dp/prsp.c on Tue Oct 25 18:41:25 EDT 1994.
 */  
/*  
 *  Make the given string case insensitive under Tcl `regexp' comparison.
 *
 *  E.g. "x(1)" would become "[Xx]\(1\)".
 */  
static int Ppc_ReNoCaseCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  char *mem;
  char *s;
  const char *meta = ".^$\\[]()*+?|";
  int i, j;
  int n = 0;
  
  if (ac != 2)
  {
    Tcl_SetResult(interp, "wrong # arguments", TCL_STATIC);
    return TCL_ERROR;
  }

  /* How much memory will we need? */
  s = av[1];
  for (i = 0; s[i]; i++)
  {
    if (isalpha(s[i]))
    {
      n += 3;                   /* "c" becomes "[Cc]" */
    }
    else if (strchr(meta, s[i]))
    {
      n++;                      /* "c" becomes "\c" */
    }
  }

  if ( ! (mem = malloc(i + n + 1)))
  {
    Tcl_AppendResult(interp, "can't allocate memory", Tcl_PosixError(interp), (char *)0);
    return TCL_ERROR;
  }

  for (i = 0, j = 0; s[i]; i++, j++)
  {
    if (isalpha(s[i]))
    {
      mem[j++] = '['; mem[j++] = toupper(s[i]);
      mem[j++] = tolower(s[i]); mem[j] = ']'; /* no ++! */
    }
    else if (strchr(meta, s[i]))
    {
      mem[j++] = '\\'; mem[j] = s[i]; /* no ++! */
    }
    else
    {
      mem[j] = s[i];            /* no ++! */
    }
  }
  mem[j] = '\0';

  Tcl_SetResult(interp, mem, TCL_DYNAMIC);
  return TCL_OK;
}


static int load_config(file)
  const char *file;
{
  if (Tcl_EvalFile(interp, file) == TCL_OK)
  {
    return 1;
  }
  else
  {
    efprintf(stderr, "%s: load_config: %s.\n", logpfx(), interp->result);
    return 0;
  }
}


/*  
 *  Return 1 if we have read in a configuration file, otherwise 0.
 */  
static int tcl_configured()
{
  return config_file[0] != '\0';
}


static void sig_usr1(signo)
  int signo;
{
  reload_config = 1;
}


/*
 *
 *
 *                            External routines
 *
 *
 */  


int tcl_init(startup)
  const char *startup;
{
  if ( ! (interp = Tcl_CreateInterp()))
  {
    efprintf(stderr, "%s: tcl_init: can't create interpreter.\n", logpfx());
    return 0;
  }

  ppc_signal(SIGUSR1, sig_usr1);

  Tcl_CreateCommand(interp, "ppc_dequote", Ppc_DequoteCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "ppc_quote", Ppc_QuoteCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "ppc_renocase", Ppc_ReNoCaseCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);

  if (startup)
  {
    strcpy(config_file, startup);
    return load_config(startup);
  }

  return 1;
}


#define HDLN_CMD_FMT " \
  if {[catch { $headlinesPrintProc %s %s {%s} %s { %s } }]} { \
    error $errorInfo \
  }"


int tcl_headlines_print(ofp, dbname, srch_str, dir_fmt, v)
  FILE *ofp;
  const char *dbname;
  const char *srch_str;
  int dir_fmt;
  VLINK v;
{
  char *cmd;
  char *vstr;
  const char *file;
  
  if ( ! tcl_configured())
  {
    return 0;
  }
  
  Tcl_ResetResult(interp);

  Tcl_EnterFile(interp, ofp, TCL_FILE_WRITABLE);
  if ( ! (file = strdup(interp->result)))
  {
    efprintf(stderr, "%s: tcl_headlines_print: can't strdup() fileID: %m.\n",
             logpfx());
    return 0;
  }

  /* bug! needs error checking. */
  vstr = vlinksString(v);

#if 0
  fprintf(stderr, "vstr is `%s'.", vstr);
  if (strlen(vstr) > 0 && strlen(vstr) < 10) abort();
#endif

  if (memsprintf(&cmd, HDLN_CMD_FMT, file, dbname, srch_str,
                 dir_fmt ? "true" : "false", vstr) < 0)
  {
    efprintf(stderr, "%s: tcl_headlines_print: error from memsprintf(): %m.\n",
             logpfx());
    free((char *)file);
    free(vstr);
    return 0;
  }

#if 0
  efprintf(stderr, "%s: tcl_headlines_print: command is `%s'.\n\n",
           logpfx(), cmd);
#endif

  if (Tcl_Eval(interp, cmd) != TCL_OK)
  {
    efprintf(stderr, "%s: tcl_headlines_print: error from Tcl_Eval(): %s.\n",
             logpfx(), interp->result);
    free(cmd);
    free((char *)file);
    free(vstr);
    return 0;
  }

  free(cmd);
  free((char *)file);
  free(vstr);
  Tcl_ResetResult(interp);

  return 1;
}


#define TAG_CMD_FMT " \
  if {[catch { $taggedPrintProc %s %s }]} { \
    error $errorInfo \
  }"

int tcl_tagged_print(ofp, cat)
  FILE *ofp;
  PATTRIB cat;
{
  Tcl_DString atlist;
  char *cmd;
  const char *atstr;
  const char *file;
  
  if ( ! tcl_configured())
  {
    return 0;
  }
  
  Tcl_ResetResult(interp);

  Tcl_EnterFile(interp, ofp, TCL_FILE_WRITABLE);
  if ( ! (file = strdup(interp->result)))
  {
    efprintf(stderr, "%s: tcl_tagged_print: can't strdup() fileID: %m.\n",
             logpfx());
    return 0;
  }

  Tcl_DStringInit(&atlist);
  Tcl_DStringStartSublist(&atlist);
  for (; (cat = nextAttr(CONTENTS, cat)); cat = cat->next)
  {
    Tcl_DStringStartSublist(&atlist);
    Tcl_DStringAppendElement(&atlist, nth_token_str(cat->value.sequence, 1));
    Tcl_DStringAppendElement(&atlist, nth_token_str(cat->value.sequence, 2));
    Tcl_DStringEndSublist(&atlist);
  }
  Tcl_DStringEndSublist(&atlist);

  atstr = Tcl_DStringValue(&atlist);

  if (memsprintf(&cmd, TAG_CMD_FMT, file, atstr) < 0)
  {
    efprintf(stderr, "%s: tcl_tagged_print: error from memsprintf(): %m.\n",
             logpfx());
    free((char *)file);
    Tcl_DStringFree(&atlist);
    return 0;
  }

  if (Tcl_Eval(interp, cmd) != TCL_OK)
  {
    efprintf(stderr, "%s: tcl_tagged_print: error from Tcl_Eval(): %s.\n",
             logpfx(), interp->result);
    free(cmd);
    free((char *)file);
    Tcl_DStringFree(&atlist);
    return 0;
  }

  free(cmd);
  free((char *)file);
  Tcl_ResetResult(interp);
  Tcl_DStringFree(&atlist);

  return 1;
}


void tcl_reinit_if_needed()
{
  if (reload_config)
  {
    if (interp)
    {
      Tcl_DeleteInterp(interp);
      interp = 0;
    }
#if 1
    tcl_init(config_file);
#else
    load_config(config_file);
#endif
    reload_config = 0;
  }
}
#endif
