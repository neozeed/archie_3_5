/*
 * Copyright (c) 1991-1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <sgtty.h>

#ifndef SOLARIS
#include <sys/dir.h>
#ifndef SOLARIS22
#include <memory.h>
#endif
#endif

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <string.h>
#include <search.h>
#ifdef ARCHIE_TIMING
#include <time.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

/* archie definitions */

#include "protos.h"
#include "typedef.h"
#include "ar_search.h"
#include "master.h"
#include "files.h"
#include "host_db.h"
#include "db_ops.h"

#ifdef GOPHERINDEX_SUPPORT
#include "gindexdb_ops.h"
#endif

#include "ar_attrib.h"
#include "error.h"
#include "archie_strings.h"

#ifdef WAIS_SUPPORT
#include "prwais_do_search.h"
#endif

#include "archie_catalogs.h"
#include "parchie_list_host.h"
#include "parchie_host_dir.h"
#include "domain.h"

#ifdef ARCHIE_TIMING
RREQ time_req;
#endif

#define GINDEX 1
#define GWINDEX 2

static file_info_t *domaindb;
static file_info_t *hostdb;
static file_info_t *hostbyaddr;
static file_info_t *hostaux_db;

static time_t   t_hostbyaddr,
                t_hostdb,
                t_domaindb,
                t_hostaux_db;

static file_info_t *catalogs_file;
static catalogs_list_t catalogs_list[MAX_CATALOGS];

/* End archie definitions */

#include <pserver.h>
#include <pfs.h>
#include <psrv.h>
#include <plog.h>
#include <pprot.h>
#include <perrno.h>
#include <pmachine.h>

#ifdef ARCHIE_TIMING
#include <sys/time.h>
#include <sys/resource.h>
#endif

int             archie_supported_version = 3;

extern char     hostname[];
extern char     hostwport[];
char            archie_prefix[] = "ARCHIE";
static int      num_slashes(char *s);
static int      tkllength(TOKEN tkl);
static int      dbopen_flag = 0;
extern int ad2l_seq_atr(VLINK l, char precedence, char nature, char *aname, ...);

extern char *prog;

/*
 * dsdb - Make a database query as if it were a directory lookup
 *
 */

/* This function is passed an empty object which has been initialized with
   oballoc().  It returns a possibly populated directory and
   DIRSRV_NOT_FOUND or PSUCCESS.  Note that the directory will need
   to have links within it freed, even if an error code is returned.  */

int             arch_dsdb(RREQ req,	/* Request pointer (unused)            */
                   char *hsoname,	/* Name of the directory               */
                   long version,/* Version #; currently ignored        */
                   long magic_no,	/* Magic #; currently ignored          */
                   int flags,	/* Currently only recognize DRO_VERIFY */
                   struct dsrobject_list_options * listopts,	/* options (use
								 * *remcompp and
								 * *thiscompp) */
                   P_OBJECT ob)
{                               /* Object to be filled in */
  /* Note that listopts->thiscompp and listopts->remcompp are pointers to */
  /* pointers.  This is necessary because              */
  /* this routine must be able to update these values  */
  /* if more than one component of the name is         */
  /* resolved.                                         */
  TOKEN           tkl_tmp;
  VLINK           dirlink = NULL;
  char           *dbpart;
  char            dbarg1[MAXPATHLEN];
  char            dbarg2[MAXPATHLEN];
  char            dbarg3[MAXPATHLEN];
  char            dbargs[MAXPATHLEN];
  char            dbquery[MAXPATHLEN];
  char            fullquery[MAXPATHLEN];
  char            sep;
  char           *components = NULL;
  int             num_unresolvedcomps = 0;
  int             tmp;
  struct stat     statbuf;


#ifdef ARCHIE_TIMING
  struct tm *tmptr;
  time_t tt;

  time_req = req;

  req->query_state = 'X';
  req->match_type = 'X';
  req->cached = 'X';
  req->case_type = 'X';
  req->path_match = 'X';
  req->domain_match = 'X';
  req->search_str[0] = '\0';
  req->hosts = 0;
  req->hosts_searched = 0;

  memset(&req->stime_start,'\0',sizeof(struct rusage));
  memset(&req->stime_end,'\0',sizeof(struct rusage));
  memset(&req->htime_start,'\0',sizeof(struct rusage));
  memset(&req->htime_end,'\0',sizeof(struct rusage));
  time(&tt);
  tmptr = localtime(&tt);
  strcpy(req->launch,asctime(tmptr));

  getrusage(RUSAGE_SELF,&req->qtime_start);
  memcpy(&req->qtime_end,&req->qtime_start,sizeof(struct rusage));
#endif
  /*
   * Make sure HSONAME, LISTOPTS->THISCOMPP, and LISTOPTS->REMCOMPP
   * arguments are correct.
   */

  /*
   * Name components with slashes in them are malformed inputs to the ARCHIE
   * database.
   */
  if (listopts && listopts->thiscompp && (components = *listopts->thiscompp)) {
    if (strchr(components, '/')) {
#ifdef ARCHIE_TIMING
      req->query_state = 'F';
#endif
      return PFAILURE;
    }
    if (listopts && listopts->remcompp) {
      for (tkl_tmp = *listopts->remcompp; tkl_tmp; tkl_tmp = tkl_tmp->next) {
        if (strchr(tkl_tmp->token, '/')) {
#ifdef ARCHIE_TIMING
          req->query_state = 'F';
#endif
          return PFAILURE;
        }
      }
    }
  }
  else {
    if (listopts->remcompp && *listopts->remcompp) {
#ifdef ARCHIE_TIMING
      req->query_state = 'F';
#endif
      return PFAILURE;          /* ridiculous to specify additional comps and
                                 * no initial comps. */
    }
  }

  /* Directory already initialized, but note that this */
  /* is not a real directory                           */

  ob->version = 0;              /* unversioned */
  ob->inc_native = VDIN_PSEUDO;
  ob->flags = P_OBJECT_DIRECTORY;	/* Reset for ARCHIE/ITEM */
  ob->magic_no = 0;
  ob->acl = NULL;

  /* Note that if we are resolving multiple components */
  /* (listopts->rcompp != NULL) the directory will already be empty */
  /* since had anything been in it dirsrv would have   */
  /* already cleared it and moved on to the next comp  */

  if (!catalogs_file) {
    catalogs_file = create_finfo();
  }

  if (read_catalogs_list(catalogs_file, catalogs_list, MAX_CATALOGS, p_err_string) != A_OK) {
    plog(L_DB_ERROR, NOREQ, "Can't open archie catalogs list %s",
         catalogs_file->filename[0] == '\0' ? "(default)" : catalogs_file->filename);
#ifdef ARCHIE_TIMING
    req->query_state = 'F';
#endif
    return PFAILURE;
  }

  /* Do only once */

  if (!dbopen_flag) {
    domaindb = create_finfo();
    hostdb = create_finfo();
    hostbyaddr = create_finfo();
    hostaux_db = create_finfo();

    /* open the catalogs list */

    open_alog(PSRV_LOGFILE, 0, "dirsrv");

    if (open_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db, O_RDONLY) != A_OK) {
      plog(L_DB_ERROR, NOREQ, "Can't open archie host database");
#ifdef ARCHIE_TIMING
      req->query_state = 'F';
#endif
      return PFAILURE;
    }

    t_hostbyaddr = t_hostdb = t_domaindb = t_hostaux_db = time((time_t *) NULL);
    dbopen_flag++;
  }
  else {
    file_info_t    *a = hostbyaddr;
    file_info_t    *b = hostdb;
    file_info_t    *c = domaindb;
    file_info_t    *d = hostaux_db;
    pathname_t      tfile;

    sprintf(tfile, "%s.pag", hostbyaddr->filename);

    if (stat(tfile, &statbuf) == -1) {
      error(A_SYSERR, "dirsrv", "Can't stat(2) host address cache %s", hostbyaddr->filename);
#ifdef ARCHIE_TIMING
      req->query_state = 'F';
#endif
      return PFAILURE;
    }

    if (t_hostbyaddr >= statbuf.st_mtime) {
      a = (file_info_t *) NULL;
    }

    sprintf(tfile, "%s.pag", hostdb->filename);
    if (stat(tfile, &statbuf) == -1) {
      error(A_SYSERR, "dirsrv", "Can't stat(2) host database %s", hostdb->filename);
#ifdef ARCHIE_TIMING
      req->query_state = 'F';
#endif
      return PFAILURE;
    }

    if (t_hostdb >= statbuf.st_mtime) {
      b = (file_info_t *) NULL;
    }

    sprintf(tfile, "%s.pag", domaindb->filename);
    if (stat(tfile, &statbuf) == -1) {
      error(A_SYSERR, "dirsrv", "Can't stat(2) domain database %s", domaindb->filename);
#ifdef ARCHIE_TIMING
      req->query_state = 'F';
#endif
      return PFAILURE;
    }

    if (t_domaindb >= statbuf.st_mtime) {
      c = (file_info_t *) NULL;
    }

    sprintf(tfile, "%s.pag", hostaux_db->filename);
    if (stat(tfile, &statbuf) == -1) {
      error(A_SYSERR, "dirsrv", "Can't stat(2) auxiliary host database %s",
            hostaux_db->filename);
#ifdef ARCHIE_TIMING
      req->query_state = 'F';
#endif
      return PFAILURE;
    }

    if (t_hostaux_db >= statbuf.st_mtime) {
      d = (file_info_t *) NULL;
    }

    close_host_dbs(a, b, c, d);

    if (open_host_dbs(a, b, c, d, O_RDONLY) != A_OK) {
      plog(L_DB_ERROR, NOREQ, "Can't open archie host database");
#ifdef ARCHIE_TIMING
      req->query_state = 'F';
#endif
      return PFAILURE;
    }

    if (a)
    t_hostbyaddr = time((time_t *) NULL);
    if (b)
    t_hostdb = time((time_t *) NULL);
    if (c)
    t_domaindb = time((time_t *) NULL);
    if (d)
    t_hostaux_db = time((time_t *) NULL);
  }

  /* Construct the full query from the pieces passed to us */
  tmp = -1 + qsprintf(fullquery, sizeof fullquery, "%s%s%s", hsoname,
                      ((components && *components) ? "/" : ""),
                      ((components && *components) ? components : ""));
  if (listopts && listopts->remcompp) {
    for (tkl_tmp = *listopts->remcompp; tkl_tmp; tkl_tmp = tkl_tmp->next) {
      tmp += -1 + qsprintf(fullquery + tmp, sizeof fullquery - tmp,
                           "/%s", tkl_tmp->token);
    }
    if (tmp + 1 > sizeof fullquery) {
#ifdef ARCHIE_TIMING
      req->query_state = 'F';
#endif
      return DIRSRV_NOT_FOUND;
    }
  }

  /* For now, if only verifying, indicate success */
  /* We don't want to do a DB search.  Eventually */
  /* we might actually check that the directory   */
  /* is valid.                                    */
  if (flags & DRO_VERIFY) {
    if (strnequal(fullquery, "ARCHIE/ITEM", 11)) {
      ob->flags &= ~P_OBJECT_DIRECTORY;
      ob->flags |= P_OBJECT_FILE;
    }
    return PSUCCESS;
  }
  /* The format for the queries is            */
  /* ARCHIE/COMMAND(PARAMETERS)/ARGS          */

  /* Strip off the database prefix */
  dbpart = fullquery + strlen(archie_prefix);

  /* And we want to skip the next slash */

#if 1

  /*
   * There may not be a slash here; it may be '\0'.  In this case dbpart++
   * will skip over it and may point to `ITEM/WAIS/(...' left over from a
   * previous query.
   * 
   * This is just a quick fix; the problem ought to be looked at more closely.
   * 
   * [Doing a vls, for example, of a search link seems to produce two calls to
   * this routine: ARCHIE_/_* and just ARCHIE.  The latter causes the
   * problem.] [ when reading remove the _ in the line above. If they are
   * not there, gcc will complain about comments within comments - lucb]
   * 
   * - wheelan (Tue Jul 26 23:39:25 EDT 1994)
   */
  if (*dbpart)
  dbpart++;
#else
  dbpart++;
#endif

  /* Find the query (up to the next /), determine if the */
  /* / exists and then read the args                     */
  tmp = sscanf(dbpart, "%[^/]%c%[^\n]", dbquery, &sep, dbargs);

  /* If no separator, for now return nothing         */
  /* Eventually, we might return a list of the query */
  /* types supported                                 */
  if (tmp < 2)
  return PSUCCESS;

  /* Check query type */
  if (strncmp(dbquery, "MATCH", 5) == 0) {
    FILTER          cfil = NULL; /* To step through filters */
    char            stype = 's'; /* search type             */
    int             maxhitpm = 100;	/* max hits per match       */
    int             maxhits = 100; /* max entries to return   */
    int             maxmatch = 100;	/* max strings to match    */
    int             offset = 0;	/* entries to skip         */
    search_req_t    search_req;	/* search request          */

    search_req.attrib_list = 0;
    SET_LINK_SIZE(search_req.attrib_list);
    SET_LK_LAST_MOD(search_req.attrib_list);
    SET_LK_UNIX_MODES(search_req.attrib_list);
    SET_AR_H_IP_ADDR(search_req.attrib_list);
    SET_AR_H_LAST_MOD(search_req.attrib_list);

    search_req.orig_type = S_E_SUB_NCASE_STR;
    search_req.no_matches = 0;

    /*
     * In the MATCH querytype, the directory part of the query (the
     * argument named HSONAME) may have no more than 3 components.  There
     * are 3 possible formats: 1) DATABASE_PREFIX (one component) 2)
     * (1)/MATCH(...) 3) (2)/query-term (3 total components)
     */
    if (num_slashes(hsoname) > 2) {
#ifdef ARCHIE_TIMING
      req->query_state = 'F';
#endif
      return DIRSRV_NOT_FOUND;
    }

    /* if no strings to match, return nothing */
    if (tmp < 3)
    return PSUCCESS;

    /* Get arguments */
    tmp = sscanf(dbquery, "MATCH(%d,%d,%d,%d,%c", &maxhits,
                 &maxmatch, &maxhitpm, &offset, &stype);

    if (tmp < 3) {
      sscanf(dbquery, "MATCH(%d,%d,%c", &maxhits, &offset, &stype);
      maxmatch = maxhits;
      maxhitpm = maxhits;
    }
    /* Note: in maxhits, 0 means use default, -1 means use max */

    switch (stype) {
    case '=':
      search_req.orig_type = S_EXACT;
      break;
    case 'R':
      search_req.orig_type = S_FULL_REGEX;
      break;
    case 'r':
      search_req.orig_type = S_E_FULL_REGEX;
      break;
    case 'X':
      search_req.orig_type = S_X_REGEX;
      break;
    case 'x':
      search_req.orig_type = S_E_X_REGEX;
      break;
    case 'C':
      search_req.orig_type = S_SUB_CASE_STR;
      break;
    case 'c':
      search_req.orig_type = S_E_SUB_CASE_STR;
      break;
    case 'K':
      search_req.orig_type = S_SUB_KASE;
      break;
    case 'k':
      search_req.orig_type = S_E_SUB_KASE;
      break;
    case 'S':
      search_req.orig_type = S_SUB_NCASE_STR;
      break;
    case 'Z':
      search_req.orig_type = S_ZUB_NCASE;
      break;
    case 'z':
      search_req.orig_type = S_E_ZUB_NCASE;
      break;
    case 'n':
      search_req.orig_type = S_NOATTRIB_EXACT;
      break;
    case 's':                   /* same as default */
    default:
      search_req.orig_type = S_E_SUB_NCASE_STR;
      break;
    }
#ifdef ARCHIE_TIMING
    switch (stype) {
    case '=':
      req->match_type = 'E';
      req->case_type = 'C';
      break;
    case 'R':
      req->match_type = 'R';
      req->case_type = 'C';
      break;
    case 'r':
      req->match_type = 'E';
      req->case_type = 'C';
      break;
    case 'X':
      req->match_type = 'R';
      req->case_type = 'C';
      break;
    case 'x':
      req->match_type = 'E';
      req->case_type = 'C';
      break;
    case 'C':
      req->match_type = 'S';
      req->case_type = 'C';
      break;
    case 'c':
      req->match_type = 'E';
      req->case_type = 'C';
      break;
    case 'K':
      req->match_type = 'S';
      req->case_type = 'C';
      break;
    case 'k':
      req->match_type = 'E';
      req->case_type = 'C';
      break;
    case 'S':
      req->match_type = 'S';
      req->case_type = 'X';
      break;
    case 'Z':
      req->match_type = 'S';
      req->case_type = 'X';
      break;
    case 'z':
      req->match_type = 'E';
      req->case_type = 'X';
      break;
    case 'n':
      req->match_type = 'E';
      req->case_type = 'C';
      break;
    case 's':                   /* same as default */
    default:
      req->match_type = 'S';
      req->case_type = 'X';
      break;
    }
#endif
    *dbarg1 = *dbarg2 = *dbarg3 = '\0';
    tmp = sscanf(dbargs, "%[^/]%c%[^/]%c%s", dbarg1, &sep, dbarg2,
                 &sep, dbarg3);
    if (tmp < 2) {

      /*
       * This specifies a directory, but not a link within it create a
       * pseudo directory and return a pointer In other words, listing a
       * MATCH directory by itself yields an empty directory.
       */
      if (*dbarg1 && (strcmp(dbarg1, "*") != 0)) {
        dirlink = vlalloc();
        dirlink->target = stcopyr("DIRECTORY", dirlink->target);
        dirlink->name = stcopyr(dbarg1, dirlink->name);
        dirlink->host = stcopyr(hostwport, dirlink->host);
        dirlink->hsoname = qsprintf_stcopyr(dirlink->hsoname, "%s/%s/%s",
                                            archie_prefix, dbquery, dbarg1);
        APPEND_ITEM(dirlink, ob->links);
      }
    }
    else {
      static catalogs_list_t *anonftp_cat;
      static file_info_t *strings;
      static file_info_t *strings_idx;
      static file_info_t *strings_hash;

      if (tmp > 4) {
        /* There are remaining components */
        num_unresolvedcomps = num_slashes(dbarg3);
      }

      if (anonftp_cat == (catalogs_list_t *) NULL) {

        if ((anonftp_cat = find_in_catalogs(ANONFTP_DB_NAME, catalogs_list)) == (catalogs_list_t *) NULL) {
          plog(L_DB_INFO, req, "Database: %s not available", ANONFTP_DB_NAME);
          p_warn_string = qsprintf_stcopyr(p_warn_string, "Database: %s not available", ANONFTP_DB_NAME);
          return PSUCCESS;
        }
        strings = anonftp_cat->cat_ainfo.archie.strings;
        strings_idx = anonftp_cat->cat_ainfo.archie.strings_idx;
        strings_hash = anonftp_cat->cat_ainfo.archie.strings_hash;
      }

      /* Reject disabled searches */

      if (anonftp_cat->cat_ainfo.archie.search_vector != 0) {
        int             svector = anonftp_cat->cat_ainfo.archie.search_vector;
        int             sreqt = search_req.orig_type;

        if ((AR_NO_REGEX & svector) && ((sreqt == S_FULL_REGEX)
                                        || (sreqt == S_E_FULL_REGEX)
                                        || (sreqt == S_X_REGEX)
                                        || (sreqt == S_E_X_REGEX))) {

          plog(L_DB_INFO, req, "Regex requested and denied");
          p_warn_string = qsprintf_stcopyr(p_warn_string, "Sorry, REGEX searches have been disabled on this server");
          return PSUCCESS;
        }

        if ((AR_NO_SUB & svector) && ((sreqt == S_SUB_NCASE_STR)
                                      || (sreqt == S_E_SUB_NCASE_STR)
                                      || (sreqt == S_ZUB_NCASE)
                                      || (sreqt == S_E_ZUB_NCASE))) {

          plog(L_DB_INFO, req, "Substring (nocase) requested and denied");
          p_warn_string = qsprintf_stcopyr(p_warn_string, "Sorry, case insensitive searches have been disabled on this server");
          return PSUCCESS;
        }

        if ((AR_NO_SUBCASE & svector) && ((sreqt == S_SUB_CASE_STR)
                                          || (sreqt == S_E_SUB_CASE_STR)
                                          || (sreqt == S_SUB_KASE)
                                          || (sreqt == S_E_SUB_KASE))) {

          plog(L_DB_INFO, req, "Substring (case) requested and denied");
          p_warn_string = qsprintf_stcopyr(p_warn_string, "Sorry, case sensitive searches have been disabled on this server");
          return PSUCCESS;
        }

        if ((AR_NO_EXACT & svector) && ((sreqt == S_EXACT)
                                        || (sreqt == S_E_FULL_REGEX)
                                        || (sreqt == S_E_SUB_CASE_STR)
                                        || (sreqt == S_E_SUB_NCASE_STR)
                                        || (sreqt == S_E_ZUB_NCASE)
                                        || (sreqt == S_E_SUB_KASE)
                                        || (sreqt == S_E_X_REGEX)
                                        || (sreqt == S_NOATTRIB_EXACT))) {

          plog(L_DB_INFO, req, "EXACT requested and denied");
          p_warn_string = qsprintf_stcopyr(p_warn_string, "Sorry, EXACT searches have been disabled on this server");
          return PSUCCESS;
        }
      }

      search_req.maxhits = maxhits;
      search_req.maxmatch = maxmatch;
      search_req.maxhitpm = maxhitpm;
      strcpy(search_req.search_str, dbarg1);

#ifdef ARCHIE_TIMING
      strcpy(req->search_str,dbarg1);
#endif
     
      /*
       * Domains to restict search on. Colon separated list of domain
       * names:
       * 
       * eg "usa:mcgill.ca:.fi"
       * 
       * The actual domain names are resolved internally to archie so you
       * don't need to do anything other than format them (if necessary)
       * and pass them along. I assume that the clients will just send
       * them preformatted
       */

      search_req.domains = (struct token *) NULL;

      for (cfil = listopts->filters; cfil; cfil = cfil->next) {
        if (cfil->name && (strcmp(cfil->name, "AR_DOMAIN") == 0) &&
            (cfil->type == FIL_DIRECTORY) &&
            (cfil->execution_location == FIL_SERVER) &&
            (cfil->pre_or_post == FIL_PRE)) {

          search_req.domains = cfil->args;
          cfil->pre_or_post = FIL_ALREADY;

          /*
           * Can't apply two AR_DOMAIN filters; ARCHIE won't support
           * this.
           */

          /*
           * Note that there is special purpose error handling code in
           * server/list.c to handle this case, too.  Look at it.
           */
          break;
        }
      }
#ifdef ARCHIE_TIMING
      req->domain_match = (search_req.domains != (struct token*) NULL ) ? 'D' : 'X';
#endif

      /*
       * Offset: For exact matches it is the number of the link on the
       * chain for that unique filename. For others it is the record
       * number in the index file of the last hit returned (in previous
       * search). A negative value returned in this variable means that
       * all hits have been found.
       */

      search_req.orig_offset = offset;

      /*
       * Same in format as "domains". The list contains the pathname
       * components that must exist to make a valid hit. For the moment
       * all comparisons are done with a case insensitive substring match
       * and it is performed as a logical "or".
       */

      search_req.comp_restrict = (struct token *) NULL;
      for (cfil = listopts->filters; cfil; cfil = cfil->next) {
        if (cfil->name && (strcmp(cfil->name, "AR_PATHCOMP") == 0) &&
            ((cfil->type == FIL_DIRECTORY) || (cfil->type == FIL_HIERARCHY)) &&
            (cfil->execution_location == FIL_SERVER) &&
            (cfil->pre_or_post == FIL_PRE)) {

          search_req.comp_restrict = cfil->args;
          cfil->pre_or_post = FIL_ALREADY;

          /*
           * If there are two AR_PATHCOMP filters, only the first one
           * should be applied.   If two are sent, list() will return an
           * error.
           */

          /*
           * Note that there is special purpose error handling code in
           * server/list.c to handle this case, too.  Look at it.
           */
          break;
        }
      }

#ifdef ARCHIE_TIMING
      req->path_match = (search_req.comp_restrict != (struct token*) NULL ) ? 'P' : 'X';
#endif
      /*
       * Any user errors (bad regular expression etc) generated will set
       * this to an appropriate message. Not used at the moment
       */

      search_req.error_string = (char *) NULL;
      if (parchie_search_files_db( domaindb, hostdb, hostaux_db, hostbyaddr,
                                  &search_req, ob) == ERROR) {
        if (search_req.error_string) {
          p_err_string = stcopyr(search_req.error_string, p_err_string);
        }
#ifdef ARCHIE_TIMING
        req->query_state = 'F';
#endif
        return PFAILURE;
      }

      if (search_req.error_string) {
        p_warn_string = stcopyr(search_req.error_string, p_warn_string);
      }
#ifdef ARCHIE_TIMING
      req->no_matches = search_req.no_matches;
#endif
      plog(L_DB_INFO, req, "matches: %d", search_req.no_matches, 0);
    }
#ifdef ARCHIE_TIMING
    getrusage(RUSAGE_SELF,&req->qtime_end);
    req->query_state = 'S';
#endif
  }

#ifdef GOPHERINDEX_SUPPORT
  else if (strncmp(dbquery, "GINDEX", 6) == 0 || 
           strncmp(dbquery, "GWINDEX", 7) == 0 ) {
    FILTER          cfil = NULL; /* To step through filters */
    char            stype = 's'; /* search type             */
    int             maxhitpm = 100;	/* max hits per match       */
    int             maxhits = 100; /* max entries to return   */
    int             maxmatch = 100;	/* max strings to match    */
    int             offset = 0;	/* entries to skip         */
    search_req_t    search_req;	/* search request          */
    static catalogs_list_t *gopherindex_cat;
    static file_info_t *gstrings;
    static file_info_t *gstrings_hash;
    static file_info_t *gstrings_idx;
    static file_info_t *host_finfo;
    static file_info_t *sel_finfo;
    static int out_format;

    if ( strncmp(dbquery, "GWINDEX", 7) == 0 ) 
    out_format = GWINDEX;
    else
    out_format = GINDEX;

    if (gopherindex_cat == (catalogs_list_t *) NULL) {

      if ((gopherindex_cat = find_in_catalogs(GOPHERINDEX_DB_NAME, catalogs_list)) == (catalogs_list_t *) NULL) {
        plog(L_DB_INFO, req, "Database: %s not available", GOPHERINDEX_DB_NAME);
        p_warn_string = qsprintf_stcopyr(p_warn_string, "Database: %s not available", ANONFTP_DB_NAME);
        return PSUCCESS;
      }

      gstrings = gopherindex_cat->cat_ainfo.archie.strings;
      gstrings_idx = gopherindex_cat->cat_ainfo.archie.strings_idx;
      gstrings_hash = gopherindex_cat->cat_ainfo.archie.strings_hash;
      sel_finfo = gopherindex_cat->cat_ainfo.archie.sel_finfo;
      host_finfo = gopherindex_cat->cat_ainfo.archie.host_finfo;
    }

    search_req.orig_type = S_E_SUB_NCASE_STR;
    search_req.no_matches = 0;

    /*
     * In the MATCH querytype, the directory part of the query (the
     * argument named HSONAME) may have no more than 3 components.  There
     * are 3 possible formats: 1) DATABASE_PREFIX (one component) 2)
     * (1)/MATCH(...) 3) (2)/query-term (3 total components)
     */
    if (num_slashes(hsoname) > 2)
    return DIRSRV_NOT_FOUND;
    /* if no strings to match, return nothing */
    if (tmp < 3)
    return PSUCCESS;

    /* Get arguments */
    if ( out_format == GINDEX ) 
    tmp = sscanf(dbquery, "GINDEX(%d,%d,%d,%d,%c", &maxhits,
                 &maxmatch, &maxhitpm, &offset, &stype);
    else
    tmp = sscanf(dbquery, "GWINDEX(%d,%d,%d,%d,%c", &maxhits,
                 &maxmatch, &maxhitpm, &offset, &stype);


    /* Note: in maxhits, 0 means use default, -1 means use max */

    switch (stype) {
    case '=':
      search_req.orig_type = S_EXACT;
      break;
    case 'R':
      search_req.orig_type = S_FULL_REGEX;
      break;
    case 'r':
      search_req.orig_type = S_E_FULL_REGEX;
      break;
    case 'X':
      search_req.orig_type = S_X_REGEX;
      break;
    case 'x':
      search_req.orig_type = S_E_X_REGEX;
      break;
    case 'C':
      search_req.orig_type = S_SUB_CASE_STR;
      break;
    case 'c':
      search_req.orig_type = S_E_SUB_CASE_STR;
      break;
    case 'K':
      search_req.orig_type = S_SUB_KASE;
      break;
    case 'k':
      search_req.orig_type = S_E_SUB_KASE;
      break;
    case 'S':
      search_req.orig_type = S_SUB_NCASE_STR;
      break;
    case 'Z':
      search_req.orig_type = S_ZUB_NCASE;
      break;
    case 'z':
      search_req.orig_type = S_E_ZUB_NCASE;
      break;
    case 'n':
      search_req.orig_type = S_NOATTRIB_EXACT;
      break;
    case 's':                   /* same as default */
    default:
      search_req.orig_type = S_E_SUB_NCASE_STR;
      break;
    }

    /* Reject disabled searches */

    if (gopherindex_cat->cat_ainfo.archie.search_vector != 0) {
      int             svector = gopherindex_cat->cat_ainfo.archie.search_vector;
      int             sreqt = search_req.orig_type;

      if ((AR_NO_REGEX & svector) && ((sreqt == S_FULL_REGEX)
                                      || (sreqt == S_E_FULL_REGEX)
                                      || (sreqt == S_X_REGEX)
                                      || (sreqt == S_E_X_REGEX))) {

        plog(L_DB_INFO, req, "Regex requested and denied");
        p_warn_string = qsprintf_stcopyr(p_warn_string, "Sorry, REGEX searches have been disabled on this server");
        return PSUCCESS;
      }

      if ((AR_NO_SUB & svector) && ((sreqt == S_SUB_NCASE_STR)
                                    || (sreqt == S_E_SUB_NCASE_STR)
                                    || (sreqt == S_ZUB_NCASE)
                                    || (sreqt == S_E_ZUB_NCASE))) {

        plog(L_DB_INFO, req, "Substring (nocase) requested and denied");
        p_warn_string = qsprintf_stcopyr(p_warn_string, "Sorry, case insensitive searches have been disabled on this server");
        return PSUCCESS;
      }

      if ((AR_NO_SUBCASE & svector) && ((sreqt == S_SUB_CASE_STR)
                                        || (sreqt == S_E_SUB_CASE_STR)
                                        || (sreqt == S_SUB_KASE)
                                        || (sreqt == S_E_SUB_KASE))) {

        plog(L_DB_INFO, req, "Substring (case) requested and denied");
        p_warn_string = qsprintf_stcopyr(p_warn_string, "Sorry, case sensitive searches have been disabled on this server");
        return PSUCCESS;
      }

      if ((AR_NO_EXACT & svector) && ((sreqt == S_EXACT)
                                      || (sreqt == S_E_FULL_REGEX)
                                      || (sreqt == S_E_SUB_CASE_STR)
                                      || (sreqt == S_E_SUB_NCASE_STR)
                                      || (sreqt == S_E_ZUB_NCASE)
                                      || (sreqt == S_E_SUB_KASE)
                                      || (sreqt == S_E_X_REGEX)
                                      || (sreqt == S_NOATTRIB_EXACT))) {

        plog(L_DB_INFO, req, "EXACT requested and denied");
        p_warn_string = qsprintf_stcopyr(p_warn_string, "Sorry, EXACT searches have been disabled on this server");
        return PSUCCESS;
      }
    }

    *dbarg1 = *dbarg2 = *dbarg3 = '\0';
    tmp = sscanf(dbargs, "%[^/]%c%[^/]%c%s", dbarg1, &sep,
                 dbarg2, &sep, dbarg3);
    if (tmp < 2) {
      /* This specifies a directory, but not a link within it  */
      /* create a pseudo directory and return a pointer        */

      /*
       * In other words, listing a MATCH directory by itself yields an
       * empty directory.
       */
      if (*dbarg1 && (strcmp(dbarg1, "*") != 0)) {
        dirlink = vlalloc();
        dirlink->target = stcopyr("DIRECTORY", dirlink->target);
        dirlink->name = stcopyr(dbarg1, dirlink->name);
        dirlink->host = stcopyr(hostwport, dirlink->host);
        dirlink->hsoname = qsprintf_stcopyr(dirlink->hsoname, "%s/%s/%s",
                                            archie_prefix, dbquery, dbarg1);
        APPEND_ITEM(dirlink, ob->links);
      }
    }
    else {
      if (tmp > 4) {
        /* There are remaining components */
        num_unresolvedcomps = num_slashes(dbarg3);
      }

      search_req.maxhits = maxhits;
      search_req.maxmatch = maxmatch;
      search_req.maxhitpm = maxhitpm;
      strcpy(search_req.search_str, dbarg1);

      /*
       * Domains to restict search on. Colon separated list of domain
       * names:
       * 
       * eg "usa:mcgill.ca:.fi"
       * 
       * The actual domain names are resolved internally to archie so you
       * don't need to do anything other than format them (if necessary)
       * and pass them along. I assume that the clients will just send
       * them preformatted
       */

      search_req.domains = (struct token *) NULL;
      for (cfil = listopts->filters; cfil; cfil = cfil->next) {
        if (cfil->name && (strcmp(cfil->name, "AR_DOMAIN") == 0) &&
            (cfil->type == FIL_DIRECTORY) &&
            (cfil->execution_location == FIL_SERVER) &&
            (cfil->pre_or_post == FIL_PRE)) {
          search_req.domains = cfil->args;
          cfil->pre_or_post = FIL_ALREADY;

          /*
           * Can't apply two AR_DOMAIN filters; ARCHIE won't support
           * this.
           */

          /*
           * Note that there is special purpose error handling code in
           * server/list.c to handle this case, too.  Look at it.
           */
          break;
        }
      }

      /*
       * Offset: For exact matches it is the number of the link on the
       * chain for that unique filename. For others it is the record
       * number in the index file of the last hit returned (in previous
       * search). A negative value returned in this variable means that
       * all hits have been found.
       */

      search_req.orig_offset = offset;

      /*
       * Same in format as "domains". The list contains the pathname
       * components that must exist to make a valid hit. For the moment
       * all comparisons are done with a case insensitive substring match
       * and it is performed as a logical "or".
       */

      search_req.comp_restrict = (struct token *) NULL;
      for (cfil = listopts->filters; cfil; cfil = cfil->next) {
        if (cfil->name && (strcmp(cfil->name, "AR_PATHCOMP") == 0) &&
            ((cfil->type == FIL_DIRECTORY) || (cfil->type == FIL_HIERARCHY)) &&
            (cfil->execution_location == FIL_SERVER) &&
            (cfil->pre_or_post == FIL_PRE)) {
          search_req.comp_restrict = cfil->args;
          cfil->pre_or_post = FIL_ALREADY;

          /*
           * If there are two AR_PATHCOMP filters, only the first one
           * should be applied.   If two are sent, list() will return an
           * error.
           */

          /*
           * Note that there is special purpose error handling code in
           * server/list.c to handle this case, too.  Look at it.
           */
          break;
        }
      }

      /*
       * Any user errors (bad regular expression etc) generated will set
       * this to an appropriate message. Not used at the moment
       */

      search_req.error_string = (char *) NULL;
      if (parchie_search_gindex_db(gstrings, gstrings_idx, gstrings_hash,
                                   domaindb, hostdb, hostaux_db, hostbyaddr,
                                   sel_finfo, host_finfo,
                                   &search_req, ob,out_format)
          == ERROR) {
        if (search_req.error_string) {
          p_err_string = stcopyr(search_req.error_string, p_err_string);
        }
        return PFAILURE;
      }

      if (search_req.error_string) {
        p_warn_string = stcopyr(search_req.error_string, p_warn_string);
      }
      plog(L_DB_INFO, req, "matches: %d", search_req.no_matches, 0);
    }
  }
#endif

  else if (strncmp(dbquery, "HOST", 4) == 0) {
    attrib_list_t   attrib_list = 0; /* uninit. error - wheelan */
    catalogs_list_t *db_cat;
    file_info_t    *strings;
    pathname_t      dbname;

    tmp = sscanf(dbquery, "HOST(%[^)]", dbname);
    if (tmp != 1) {
      strcpy(dbname, ANONFTP_DB_NAME);
    }

    if ((db_cat = find_in_catalogs(dbname, catalogs_list)) == (catalogs_list_t *) NULL) {
      plog(L_DB_INFO, req, "Database: %s not available", dbname);
      p_err_string = qsprintf_stcopyr(p_err_string, "Database: %s not available", dbname);
      return PFAILURE;
    }

    strings = db_cat->cat_ainfo.archie.strings;

    /* First component of args is the site name    */
    /* remaining components are the directory name */

    *dbarg1 = *dbarg2 = '\0';
    tmp = sscanf(dbargs, "%[^/]%c%s", dbarg1, &sep, dbarg2);

    /* If first component is null, return an empty directory */
    if (tmp < 1)
    return PSUCCESS;

    /* if first component exists, but is last component, */
    /* then it is the name of the subdirectory for the   */
    /* host, create a pseudo directory and return a      */
    /* pointer, If first component is a wildcard, and no */
    /* additional components, then return matching list  */
    /* of sites.                                         */

    if (dbarg2[0] == '*') {
      SET_AR_H_LAST_MOD(attrib_list);
      SET_AR_H_IP_ADDR(attrib_list);
      tmp = parchie_list_host(dbarg1, dbname, attrib_list, ob,
                              hostdb, hostaux_db, strings);
      if (tmp == PRARCH_TOO_MANY)
	    return DIRSRV_TOO_MANY;
      if (tmp)
	    return PFAILURE;
    }
    else {
      /* More than one component, Look up the requested directory  */
      /* Note that the since the full query is passed to us, it    */
      /* includes the component name, thus the directory name is   */
      /* what you get when you strip off the last component of the */
      /* name                                                      */

      char           *lastsep;

      SET_LINK_SIZE(attrib_list);
      SET_LK_LAST_MOD(attrib_list);
      SET_LK_UNIX_MODES(attrib_list);
      lastsep = strrchr(dbarg2, '/');

      if (lastsep)
	    *lastsep++ = '\0';
      else
	    *dbarg2 = '\0';

      tmp = parchie_host_dir(dbarg1, attrib_list, dbarg2,
                             ob, hostdb, hostaux_db, strings);
      if (tmp == PRARCH_SITE_NOT_FOUND)
	    return DIRSRV_NOT_FOUND;
      if (tmp)
	    return PFAILURE;
    }
  }

#ifdef DOMAINS_SUPPORT
  else if (strncmp(dbquery, "DOMAINS", 7) == 0) {
    VLINK           curr_link;
    domain_struct  *cdom;
    domain_struct   domain_set[MAX_NO_DOMAINS];
    int             maxno = MAX_NO_DOMAINS;

    if (get_domain_list(domain_set, maxno, domaindb) == ERROR) {
      p_warn_string = stcopyr("Error while listing domains", p_warn_string);
      return PFAILURE;
    }
    else {
      curr_link = (VLINK) NULL;
      sort_domains(domain_set);
      for (cdom = domain_set; cdom->domain_name[0] != '\0'; cdom++) {

#if 0
        if (cdom->domain_desc[0] == '\0')
        continue;
#endif

        if ((curr_link = (VLINK) vlalloc()) == (VLINK) NULL) {
          plog(L_DB_INFO, NOREQ, "Can't allocate link!");
          return PRARCH_OUT_OF_MEMORY;
        }

        curr_link->hsoname = stcopyr("DOMAINS", curr_link->hsoname);
        curr_link->name = stcopyr("DOMAINS", curr_link->name);

        curr_link->host = stcopyr(hostwport, curr_link->host);
        curr_link->target = stcopyr("OBJECT", curr_link->target);

        ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                     "CONTENTS", cdom->domain_name, cdom->domain_def,
                     cdom->domain_desc,0);

        APPEND_ITEM(curr_link, ob->links);
      }
    }
  }
#endif /* DOMAINS_SUPPORT */

#ifdef WAIS_SUPPORT
  else if (strncmp(dbquery, "FIND", 4) == 0) {
    catalogs_list_t *catalog;
    char          **cptr;
    char          **database_list;
    char          **keyword_list;
    wais_search_req_t wsearch_req;

    *dbarg1 = *dbarg2 = '\0';
    memset(&wsearch_req, '\0', sizeof(wsearch_req));

    if ((tmp = sscanf(dbargs, "%[^/]%*c", dbarg3)) != 1) {
      p_err_string = stcopyr("Malformed request", p_err_string);
      return PFAILURE;
    }

    if ((keyword_list = str_sep(dbarg3, ';')) == (char **) NULL) {
      p_err_string = qsprintf_stcopyr(p_err_string, "Malformed keyword list %s", dbarg3);
      return PFAILURE;
    }

    /*
     * -- wheelan [Tue Mar 8 16:51:01 EST 1994]
     * 
     * Changed from "FIND(%d,%d,%d,%[^)])" so the last argument will be
     * scanned correctly, even if it is surrounded by spaces.
     */
    if ((tmp = sscanf(dbquery, "FIND(%ld,%ld,%d, %[^ )])", &wsearch_req.Max_Docs,
                      &wsearch_req.Max_Headers, &wsearch_req.expand, dbarg2)) < 4) {
      if (tmp == 1) {
        wsearch_req.Max_Headers = DEFAULT_MAXHEADERS;
      }
      else {
        p_err_string = qsprintf_stcopyr(p_err_string, "Malformed parameter list %s", dbarg1);
        return PFAILURE;
      }
    }

    if ((database_list = str_sep(dbarg2, ':')) == (char **) NULL) {
      p_err_string = qsprintf_stcopyr(p_err_string, "Malformed database list %s", dbarg2);
      return PFAILURE;
    }

    if (wsearch_req.Max_Docs == 0) {
      wsearch_req.Max_Docs = DEFAULT_MAXDOCS;
    }

    for (cptr = database_list; *cptr != (char *) NULL; cptr++) {
      /* find the right catalog */

      if ((catalog = find_in_catalogs(*cptr, catalogs_list)) == (catalogs_list_t *) NULL) {
        plog(L_DB_INFO, NOREQ, "Catalog '%s' not available", *cptr);
        p_err_string = qsprintf_stcopyr(p_err_string, "Catalog '%s' currently not available", *cptr);
        return PFAILURE;
      }

      wsearch_req.error_string[0] = '\0';
      wsearch_req.keyword_list[0] = '\0';
      wsearch_req.wais_attribs = -1; /* 0xffffffff */

      if ((catalog->cat_access == CATA_WAIS)) {

        /*
         * Don't have to preprend attribute-specfic prefix for non
         * template WAIS databases
         */

        if (catalog->cat_type != CAT_FREETEXT) {
          if (do_attr_value(keyword_list, catalog, wsearch_req.keyword_list) == ERROR) {
            error(A_WARN, "arch_dsdb", "Can't parse incoming request %s", dbarg3);
            p_err_string = qsprintf_stcopyr(p_err_string, "Sorry, this request is not valid. %s\nPlease consult the Help documentation", dbarg3);
            return PFAILURE;
          }
        }
        else {
          char            kwl[2048];
          char          **s;
          pathname_t      xx;

          kwl[0] = '\0';
          for (s = keyword_list; *s != '\0'; s++) {
            sprintf(xx, "%s ", *s);
            strcat(kwl, xx);
          }
          strcpy(wsearch_req.keyword_list, kwl);
        }

        sprintf(wsearch_req.database, "%s/%s", catalog->cat_db, catalog->cat_name);

        if (prwais_do_search(NULL, 0, ob, &wsearch_req, catalog, req) != PRWAIS_SUCCESS) {
          if (wsearch_req.error_string[0] != '\0') {
            p_err_string = stcopyr(wsearch_req.error_string, p_err_string);
          }
          return PSUCCESS;
        }
      }
    }

    if (wsearch_req.error_string[0] != '\0') {
      p_warn_string = stcopyr(wsearch_req.error_string, p_warn_string);
    }

    free_opts(database_list);
    free_opts(keyword_list);
  }
#endif /* WAIS_SUPPORT */

#ifdef ITEM_SUPPORT
  else if (strncmp(dbquery, "ITEM", 4) == 0) {
#define RANDKEY 0
#define DOCLEN  1
#define DOCTYPE 2
#define DOCDB   3
#define SEQSTR  4

    catalogs_list_t *catalog;
    char            *docst;
    char            **elts;
    char            tmp_str3[2048];

    *dbarg1 = *dbarg2 = *dbarg3 = '\0';
    if ((tmp = sscanf(dbpart, "%[^/]%*c%[^/]%*c(%[^)])", dbquery, dbarg1, dbarg2)) != 3) {
      p_err_string = stcopyr("Malformed request", p_err_string);
      return PFAILURE;
    }

    ob->flags &= ~P_OBJECT_DIRECTORY;
    ob->flags |= P_OBJECT_FILE;

    if (strncmp("WAIS", dbarg1, 4) == 0) {
      char seqstr[64];

      if(listopts && listopts->requested_attrs && *listopts->requested_attrs &&
         strstr(listopts->requested_attrs, "+CONTENTS+") == (char *)NULL) {
        return(PSUCCESS);
      }

      /*  
       *  `dbarg2' should look something like:
       *  
       *    806015413910124968,264,TEXT,wp,000005
       */

      if ( ! (elts = str_sep_single_free(dbarg2, ','))) {
        /* bug: should be malloc() error, for example. */
        p_err_string = stcopyr("Malformed request", p_err_string);
        return PFAILURE;
      }

      /* Ensure we got exactly five fields. */

      if ( ! (elts[RANDKEY] && elts[DOCLEN] && elts[DOCTYPE] &&
              elts[DOCDB] && elts[SEQSTR] && ! elts[SEQSTR+1])) {
        p_err_string = stcopyr("Malformed request", p_err_string);
        free(elts);
        return PFAILURE;
      }

      /* Ensure none of the fields are empty. */
       
      if ( ! (*elts[RANDKEY] && *elts[DOCLEN] && *elts[DOCTYPE] &&
              *elts[DOCDB] && *elts[SEQSTR])) {
        p_err_string = stcopyr("Malformed request", p_err_string);
        free(elts);
        return PFAILURE;
      }

      if ((catalog = find_in_catalogs(elts[DOCDB], catalogs_list)) == (catalogs_list_t *)NULL) {
        error(A_WARN, "arch_dsdb", "Catalog '%s' not found", elts[DOCDB]);
        plog(L_DB_INFO, NOREQ, "Can't find '%s' catalog", elts[DOCDB]);
        free(elts);
        return PFAILURE;
      }

      sprintf(tmp_str3, "%s/%s", catalog->cat_db, elts[DOCDB]);

      if ((docst = wais_retrieve(NULL, 0, elts[DOCLEN], elts[RANDKEY], elts[DOCTYPE],
                                 tmp_str3, elts[SEQSTR])) == (char *) NULL) {
        free(elts);
        return PFAILURE;
      }

      /* Instead, return it as CONTENTS attributes on the object itself. */

      if (catalog->cat_type == CAT_FREETEXT) {
        contentsAttrFromFreetext(docst, &ob->attributes);
      } else {
        contentsAttrFromTemplate(docst, catalog, 1 /* expand */, &ob->attributes);
      }

      free(elts);
     
#undef RANDKEY
#undef DOCLEN
#undef DOCTYPE
#undef DOCDB 
#undef SEQSTR

#ifdef BUNYIP_AUTHENTICATION
      if ( catalog->restricted ) {

        if ( catalog->classes != NULL ) {
          char *buffer,**aptr;
          PATTRIB at;

          buffer = (char*)malloc((strlen(catalog->classes)+1)*sizeof(char));
          if ( buffer == NULL ) {
            plog(L_DB_INFO, NOREQ, "Can't allocate memory!");
            return PFAILURE;
          }

          strcpy(buffer,catalog->classes);
          aptr = str_sep(buffer, ',');
          at = atalloc();
          at->aname = stcopy("BUNYIP-RESTRICTED-ACCESS");
          at->avtype = ATR_SEQUENCE;
          at->nature = ATR_NATURE_APPLICATION;
          at->precedence = ATR_PREC_OBJECT;

          while ( *aptr != NULL ) {
            at->value.sequence = tkappend(stcopy(*aptr), at->value.sequence);
            aptr++;
          }
          APPEND_ITEM(at, ob->attributes);
        }
        else {
          ob_atput(ob,"BUNYIP-RESTRICTED-ACCESS",'\0');
        }
      }
#endif

      free(docst);

    }
  }
#endif /* ITEM_SUPPORT */

  else {
    /* Query type not supported */
    return DIRSRV_NOT_FOUND;
  }

  /*
   * We are done, but we need to figure out if we resolved multiple
   * components and reset *listopts->thiscompp and *listopts->remcompp
   * appropriately.
   */

  if (num_unresolvedcomps) {
    int             skip;

    if (listopts && listopts->remcompp) {
      skip = tkllength(*listopts->remcompp) - num_unresolvedcomps;
    }
    else {
      skip = -num_unresolvedcomps;
    }

    if (skip < 0)
    return DIRSRV_NOT_FOUND;    /* shouldn't happen. */

    while (skip-- > 0) {
      assert(*listopts->remcompp);
      *listopts->thiscompp = (*listopts->remcompp)->token;
      *listopts->remcompp = (*listopts->remcompp)->next;
    }
  }
  else {
    while (listopts && listopts->remcompp && *listopts->remcompp) {
      *listopts->thiscompp = (*listopts->remcompp)->token;
      *listopts->remcompp = (*listopts->remcompp)->next;
    }
  }
  return PSUCCESS;
}


static int      tkllength(TOKEN tkl)
{
   int             retval = 0;

   for (; tkl; tkl = tkl->next)
      ++retval;
   return retval;
}


static int      num_slashes(char *s)
{
   int             retval = 0;

   for (; *s; ++s) {
      if (*s == '/')
	 ++retval;
   }
   return retval;
}
