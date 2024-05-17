#include <sys/types.h>
#include <ctype.h>
#include "perrno.h"
#include "pfs.h"
#include "ppc.h"
#include "prsp_vlink.h"
#include "tcl.h"
#include "quoting.h"


static char *prsp_errmesg(int err);
static char *vlinksString(VLINK v);
static int Prsp_AddVlinkCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[]);
static int Prsp_DelVlinkCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[]);
static int Prsp_DequoteCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[]);
static int Prsp_GetDirCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[]);
static int Prsp_InitCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[]);
static int Prsp_QuoteCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[]);
static int Prsp_RdSlinkCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[]);
static int Prsp_RdVdirCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[]);
static int Prsp_RdVlinkCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[]);
static int Prsp_ReNoCaseCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[]);


const char *prog; /* bug: so we can use libppc.a */
int debug;        /* ditto */
static char phost[MAXHOSTNAMELEN+1];

/*  
 *  Taken from www_safe on Sep 8 1994.
 */  
static char prsp_safe[256] =
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


#define VSDESC "/pfs/pfsdat/local_vsystems/bunyip/VS-DESCRIPTION"


int Prsp_Init(Tcl_Interp *interp)
{
  if ( ! (prog = Tcl_GetVar(interp, "argv0", 0)))
  {
    prog = "prospero-shell";
  }

  if (Prsp_VInit(interp) != TCL_OK)
  {
    return TCL_ERROR;
  }
  
  /*  
   *  Register all our commands with the interpreter.
   */  
  Tcl_CreateCommand(interp, "prsp_add_vlink", Prsp_AddVlinkCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "prsp_del_vlink", Prsp_DelVlinkCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "prsp_get_dir", Prsp_GetDirCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "prsp_dequote", Prsp_DequoteCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "prsp_quote", Prsp_QuoteCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "prsp_rd_slink", Prsp_RdSlinkCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "prsp_rd_vdir", Prsp_RdVdirCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "prsp_rd_vlink", Prsp_RdVlinkCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "prsp_renocase", Prsp_ReNoCaseCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  Tcl_CreateCommand(interp, "prsp_init", Prsp_InitCmd,
                    (ClientData)0, (Tcl_CmdDeleteProc *)0);
  return TCL_OK;
}


/*  
 *
 *
 *                             Support Routines
 *
 *
 */  


#define MAX_STR_LEN 128
#define TOT_STR_LEN (2*MAX_STR_LEN)

/*  
 *  Based (somewhat) on `sperrmesg' in prospero/lib/pfs/perrmesg.c.
 */
static char *prsp_errmesg(int err)
{
  char *m0, *m1;
  int len0, len1;
  static char mesg[TOT_STR_LEN + 3 + 1]; /* extra space for " - " and nul. */

  m0 = p_err_text[err];
  len0 = strlen(m0);

  m1 = p_err_string;
  len1 = strlen(m1);
  
  if (len0 + len1 <= TOT_STR_LEN)
  {
    sprintf(mesg, "%s%s%s", m0, *m1 ? " - " : "", m1);
  }
  else if (len0 <= MAX_STR_LEN)
  {
    sprintf(mesg, "%s%s%.*s...", m0, *m1 ? " - " : "", TOT_STR_LEN - len0 - 3, m1);
  }
  else if (len1 <= MAX_STR_LEN)
  {
    sprintf(mesg, "%.*s...%s%s", TOT_STR_LEN - len1 - 3, m0, *m1 ? " - " : "", m1);
  }
  else
  {
    sprintf(mesg, "%.*s...%s%.*s...", MAX_STR_LEN - 3, m0, *m1 ? " - " : "", MAX_STR_LEN - 3, m1);
  }
  return mesg;
}

  
/*  
 *  Return a Tcl dynamic string corresponding to the list, v, of VLINKs.
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


static VLINK vlinkFromHandle(Tcl_Interp *interp, const char *handle)
{
  VLINK v;
  char **harray;
  int n;
  
  if (Tcl_SplitList(interp, handle, &n, &harray) != TCL_OK)
  {
    return 0;
  }

  if ( ! (v = vlalloc()))
  {
    free(harray);
    Tcl_AppendResult(interp, "can't allocate VLINK", (char *)0);
    return 0;
  }
  
  switch (n)
  {
  case 4:
    v->host = stcopyr(harray[0], v->host);
    v->hosttype = stcopyr(harray[1], v->hosttype);
    v->hsoname = stcopyr(harray[2], v->hsoname);
    v->hsonametype = stcopyr(harray[3], v->hsonametype);
    break;

  case 2:
    v->host = stcopyr(harray[0], v->host);
    v->hosttype = stcopyr("INTERNET-D", v->hosttype);
    v->hsoname = stcopyr(harray[1], v->hsoname);
    v->hsonametype = stcopyr("ASCII", v->hsonametype);
    break;

  case 1:
    v->host = stcopyr(phost, v->host);
    v->hosttype = stcopyr("INTERNET-D", v->hosttype);
    v->hsoname = stcopyr(harray[0], v->hsoname);
    v->hsonametype = stcopyr("ASCII", v->hsonametype);
    break;

  default:
    free(harray);
    vlfree(v);
    Tcl_SetResult(interp, "wrong # items in handle", TCL_STATIC);
    return 0;
  }

  free(harray);
  return v;
}


/*  
 *
 *
 *                                 Commands
 *
 *
 */  


/*  
 *  Add a new virtual link.
 *
 *  Usage:
 *
 *  prsp_add_vlink ?-flags flags? parent-dir link-name vlink
 */
static int Prsp_AddVlinkCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  VLINK v;
  char *dir = 0;
  char *fstr = 0;
  char *lname = 0;
  char *vstr = 0;
  int flags = 0;
  int perr;
  
  switch (ac)
  {
  case 6:
    if (strcmp(av[1], "-flags") != 0)
    {
      Tcl_SetResult(interp,
                    "usage: prsp_add_vlink [-flags flags] parent-dir link-name vlink",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    fstr = av[2];
    dir = av[3];
    lname = av[4];
    vstr = av[5];
    break;
    
  case 4:
    dir = av[1];
    lname = av[2];
    vstr = av[3];
    break;
    
  default:
    Tcl_SetResult(interp,
                  "usage: prsp_add_vlink [-flags flags] parent-dir link-name vlink",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  if (fstr)                     /* ignore flags for now */
  {
  }

  if ( ! (v = vlinkFromTclList(interp, vstr)))
  {
    Tcl_AppendResult(interp, "can't create vlink", (char *)0);
    return TCL_ERROR;
  }

  if ((perr = add_vlink(dir, lname, v, flags) != PSUCCESS))
  {
    vlfree(v);
    Tcl_AppendResult(interp, "can't add directory: ", prsp_errmesg(perr), (char *)0);
    return TCL_ERROR;
  }

  Tcl_SetResult(interp, vstr, TCL_VOLATILE);
  return TCL_OK;
}


/*  
 *  Delete a virtual link.
 *
 *  Usage:
 *
 *  prsp_del_vlink ?-flags flags? link-name
 */
static int Prsp_DelVlinkCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  char *fstr = 0;
  char *lname;
  int flags = 0;
  int perr;
  
  switch (ac)
  {
  case 4:
    if (strcmp(av[1], "-flags") != 0)
    {
      Tcl_SetResult(interp, "usage: prsp_del_vlink [-flags flags] link-name", TCL_STATIC);
      return TCL_ERROR;
    }
    fstr = av[2];
    lname = av[3];
    break;
    
  case 2:
    lname = av[1];
    break;
    
  default:
    Tcl_SetResult(interp, "usage: prsp_del_vlink [-flags flags] link-name", TCL_STATIC);
    return TCL_ERROR;
  }

  if (fstr) /* ignore flags for now */
  {
  }

  if ((perr = del_vlink(lname, flags)) != PSUCCESS)
  {
    Tcl_AppendResult(interp, "del_vlink() failed: ", prsp_errmesg(perr), (char *)0);
    return TCL_ERROR;
  }
  
  return TCL_OK;
}


/*  
 *  Dequote the argument according to WWW (URL?) conventions.  (I.e. The
 *  string "%HH" corresponds to the character 0xHH, where the Hs are
 *  hexadecimal numbers.)
 */      
static int Prsp_DequoteCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  char *s;

  if (ac != 2)
  {
    Tcl_SetResult(interp, "usage: prsp_dequote string", TCL_STATIC);
    return TCL_ERROR;
  }

  if ( ! (s = dequote_string(av[1], prsp_safe, malloc)))
  {
    Tcl_SetResult(interp, "memory error?", TCL_STATIC);
    return TCL_ERROR;
  }

  Tcl_SetResult(interp, s, TCL_DYNAMIC);
  return TCL_OK;
}


static int Prsp_InitCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  char *file = VSDESC;
  char *host = "localhost";
  char *path = "";
  int perr;

  switch (ac)
  {
  case 4: path = av[3];
  case 3: file = av[2];
  case 2: host = av[1];
  case 1:
    break;

  default:
    Tcl_SetResult(interp, "usage: prsp_init [host [file [path]]]", TCL_STATIC);
    return TCL_ERROR;
  }    
    
  /*  
   *  Copy the server name so other commands can use it as a default.
   */  
  strncpy(phost, host, sizeof phost - 1);
  phost[sizeof phost - 1] = '\0';

  p_initialize((char *)0, 0, (struct p_initialize_st *)0);
  if ((perr = vfsetenv(host, file, path)) != PSUCCESS)
  {
    Tcl_AppendResult(interp, "vfsetenv() failed: ", prsp_errmesg(perr), (char *)0);
    return TCL_ERROR;
  }
  return TCL_OK;
}


/*  
 *  Usage:
 *  
 *  prsp_get_dir <parent-dir> [<file-component> [<filt-args?>]]
 *  
 *  If <parent-dir> starts with two slashes it is treated as an absolute UNIX
 *  path, rather than a path relative to the current Prospero directory.  A
 *  <parent-dir> of ARCHIE is treated as a special case; it isn't interpreted
 *  as a path relative to the current directory.
 *  
 *  The return value is a list corresponding to the links in <parent-dir>.
 *  Each element of the list is composed of the link name, its handle (a four
 *  element list: host, host type, hsoname, and hsoname type) and a series of
 *  attributes.  Each attribute is itself a two element list consisting of
 *  the attribute name and its value.
 */                          
static int Prsp_GetDirCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  TOKEN acomp = 0;
  VDIR rd;
  VDIR_ST rd_;
  VLINK v;
  char *handle = 0;
  char *filecomp = "";
  char *filtargs = 0;
  char *res;
  int perr;
  
  av++, --ac;
  switch (ac)
  {
  case 3: filtargs = av[2];
  case 2: filecomp = av[1];
  case 1: handle = av[0];
    break;

  default:
    Tcl_SetResult(interp,
                  "usage: prsp_get_dir parent-dir [file-component [filt-args]]",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  if ( ! (v = vlinkFromHandle(interp, handle)))
  {
    return TCL_ERROR;
  }

  /*  
   *  Convert any filter arguments to a linked list of TOKENs.
   */  
  if (filtargs)
  {
    char **fal;                 /* filter argument list */
    int i, n;
    
    if (Tcl_SplitList(interp, filtargs, &n, &fal) != TCL_OK)
    {
      vlfree(v);
      return TCL_ERROR;
    }

    acomp = tkappend(n ? fal[0] : "", (TOKEN)0);
    for (i = 1; i < n; i++)
    {
      acomp = tkappend(fal[i], acomp);
    }
    free(fal);
  }

  /*  
   *  Retrieve the directory listing.
   */  
  rd = &rd_;
  vdir_init(rd);
  if ((perr = p_get_dir(v, filecomp, rd, GVD_ATTRIB|GVD_NOSORT, &acomp)) != PSUCCESS)
  {
    vlfree(v);
    if (acomp) tklfree(acomp);
    Tcl_AppendResult(interp, "p_get_dir() failed: ", prsp_errmesg(perr), (char *)0);
    return TCL_ERROR;
  }

  vlfree(v);
  vllfree(rd->ulinks);
  if (acomp) tklfree(acomp);

  /*  
   *  Generate the return value: a list, each of whose elements corresponds
   *  to one directory entry.
   */    
  res = vlinksString(rd->links);
  Tcl_SetResult(interp, res, TCL_DYNAMIC);

  vllfree(rd->links);
  return TCL_OK;
}


/*  
 *  Quote the argument according to WWW (URL?) conventions.  (I.e. The
 *  character 0xHH becomes the string "%HH", where the Hs are hexadecimal
 *  numbers.)
 */        
static int Prsp_QuoteCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  char *s;

  if (ac != 2)
  {
    Tcl_SetResult(interp, "usage: prsp_quote string", TCL_STATIC);
    return TCL_ERROR;
  }

  if ( ! (s = quote_string(av[1], prsp_safe, malloc)))
  {
    Tcl_SetResult(interp, "memory error?", TCL_STATIC);
    return TCL_ERROR;
  }

  Tcl_SetResult(interp, s, TCL_DYNAMIC);
  return TCL_OK;
}


/*  
 *  Usage:
 *  
 *  prsp_rd_vdir <virtual-dir> [<file-component>]
 *  
 *  Return a list corresponding to the contents of the Prospero
 *  <virtual-dir>.  If <file-component> exists return only those entries that
 *  match it.
 */  
static int Prsp_RdVdirCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  VDIR rd;
  VDIR_ST rd_;
  char *filecomp = 0;
  char *vdir = 0;
  char *res;
  int perr;

  switch (ac)
  {
  case 3: filecomp = av[2];
  case 2: vdir = av[1];
    break;
    
  default:
    Tcl_SetResult(interp, "usage: prsp_rd_vdir vdir [file-component]", TCL_STATIC);
    return TCL_ERROR;
  }

  rd = &rd_;
  vdir_init(rd);
  if ((perr = rd_vdir(vdir, filecomp, rd, RVD_ATTRIB|RVD_NOSORT)) != PSUCCESS)
  {
    Tcl_AppendResult(interp, "rd_vdir() failed: ", prsp_errmesg(perr), (char *)0);
    return TCL_ERROR;
  }

  res = vlinksString(rd->links);
  Tcl_SetResult(interp, res, TCL_DYNAMIC);

  return TCL_OK;
}


/*  
 *  Usage:
 *  
 *  prsp_rd_slink link-name
 */  
static int Prsp_RdSlinkCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  VLINK v;
  char *res;

  if (ac != 2)
  {
    Tcl_SetResult(interp, "usage: prsp_rd_slink vlink", TCL_STATIC);
    return TCL_ERROR;
  }

  if ( ! (v = rd_slink(av[1])))
  {
    Tcl_AppendResult(interp, "rd_vlink() failed: ", prsp_errmesg(perrno), (char *)0);
    return TCL_ERROR;
  }

  res = vlinksString(v);
  Tcl_SetResult(interp, res, TCL_DYNAMIC);

  return TCL_OK;
}


/*  
 *  Usage:
 *  
 *  prsp_rd_vlink link-name
 */  
static int Prsp_RdVlinkCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
{
  VLINK v;
  char *res;

  if (ac != 2)
  {
    Tcl_SetResult(interp, "usage: prsp_rd_vlink vlink", TCL_STATIC);
    return TCL_ERROR;
  }

  if ( ! (v = rd_vlink(av[1])))
  {
    Tcl_AppendResult(interp, "rd_vlink() failed: ", prsp_errmesg(perrno), (char *)0);
    return TCL_ERROR;
  }

  res = vlinksString(v);
  Tcl_SetResult(interp, res, TCL_DYNAMIC);

  return TCL_OK;
}


/*  
 *  Make the given string case insensitive under Tcl `regexp' comparison.
 *
 *  E.g. "x(1)" would become "[Xx]\(1\)".
 */  
static int Prsp_ReNoCaseCmd(ClientData clientData, Tcl_Interp *interp, int ac, char *av[])
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
