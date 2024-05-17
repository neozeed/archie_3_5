#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef ARCHIE_TIMING
#include <time.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include "typedef.h"
#include "strings_index.h"
#include "host_db.h"

/*
 *  #ifdef OLD_FILES # include "old-site_file.h" #else # include "site_file.h"
 *  #endif
 */
#include "site_file.h"
#include "parchie_search_files_db.h"


#include "ardp.h"
#include "plog.h"
#include "ar_search.h"
#include "db_ops.h"
#include "error.h"
#include "files.h"
#include "db_files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "times.h"
#include "parchie_cache.h"
#include "search.h"
#include "core_entry.h"
#include "master.h"
#include "start_db.h"
#include "lang_startdb.h"
#include "patrie.h"
#include "archstridx.h"
#include "get-info.h"

/* to prevent conflicts with the macro definition of "filename"
   pfs.h */

#undef filename   

#ifdef timezone
#undef timezone
#endif

#ifdef index
#undef index
#endif


extern status_t archQuery();


static struct match_cache *a_mcache = NULL;
static int a_mcachecount = 0;
extern char	hostname[];
extern char	hostwport[];
extern int ad2l_seq_atr(VLINK l, char precedence, char nature, char *aname, ...);
static file_info_t *site_file = (file_info_t *)NULL;

static char *get_anon_preferred_name();

#ifdef ARCHIE_TIMING
extern RREQ time_req;
#endif

extern char *prog;

int parchie_search_files_db( domaindb, hostdb, hostaux_db,
                            hostbyaddr, search_req, ob)
/*
 *  96-04-09
 *  
 *     file_info_t *strings; file_info_t *strings_idx; file_info_t
 *     *strings_hash;
 */
  file_info_t	*domaindb;
  file_info_t *hostdb;
  file_info_t *hostaux_db;
  file_info_t *hostbyaddr;
  search_req_t *search_req;
  P_OBJECT ob; /* directory being filled in */
{
  VLINK curr_link;
  char **tmp_ptr;
  char *chostname = (char*)NULL;
  char fullpath[2048];
  char tmp_str[2048];
  char *tmp_s;
  char *tmp_ptr2 = NULL;  
  char **path_name = NULL;
  char **restricts = NULL;
  domain_t domain_list[MAX_NO_DOMAINS];
  domain_t domains;
  /*  full_site_entry_t *site_ptr = NULL;*/
  hostdb_aux_t hostaux_rec;
  hostdb_t hostdb_rec;
  hostname_t primary_hostname;
/*  index_t *index_list = NULL; */
  int domain_count = 0;
  /*  int linkcount = 0;*/
  
  int ret_status = PRARCH_SUCCESS;
  int slen;
  int this_host = 1;
  ip_addr_t old_addr = 0;
  ip_addr_t new_addr;
  pathname_t comp_rest;
  register int count = 0;
  register int total = 0;

#ifdef TIMING
  struct timeval tp1,tp2;
  char *timing_str = "unknown";
#endif

  /* 96-04-09 */

  start_return_t start_ret;
  index_t **strs = NULL;  
  pathname_t master_database_dir;
  pathname_t files_database_dir;
  pathname_t start_database_dir;
  pathname_t logfile;

  /*startdb stuff*/
  file_info_t *start_db = create_finfo();
  /*startdb stuff*/

  struct arch_stridx_handle *strhan = NULL;
  char *dbdir;

  /*  domain_t d_list[MAX_NO_DOMAINS];*/
  int d_cnt = 0;
  query_result_t *result = NULL;
  int format = I_FORMAT_LINKS;
  
  int srch_type=0;
  int logging = 1;              /* send all error messages to log file */
  char *qry,*han;
  int path_rel;
  int path_items;
  char **path_r;                /* path restriction */
  int max_hits = 0;
  int ret_hits = 0;
  int match = 0;
  int hpm = 0;
  int type = EXACT;
  int case_sens = 0;
  int db;
  char *expath;
  char *serv_url;
  start_ret.stop = -1;
  start_ret.string = -1;

  logfile[0] = files_database_dir[0] = start_database_dir[0] = master_database_dir[0] = '\0';
  db = I_ANONFTP_DB;


  /* 96-04-09 */
  if((dbdir = (char *)set_files_db_dir(files_database_dir)) == (char *) NULL){
    /* "Error while trying to set anonftp database directory" */
    error(A_ERR,"dirsrv", "Error while trying to set anonftp database directory\n");
    ret_status = PRARCH_BAD_ARG;
    goto atend;
  }

  
  if(set_start_db_dir(start_database_dir,DEFAULT_FILES_DB_DIR) == (char *) NULL){
    /* "Error while trying to set start database directory" */
    error(A_ERR, "dirsrv", "Error while trying to set start database directory\n");
    ret_status = PRARCH_BAD_ARG;
    goto atend;
  }

  /* 96-04-09 */
  /* Open other files database files */


  
  if ( open_start_dbs(start_db,NULL,O_RDONLY ) == ERROR ) {
    /* "Can't open start/host database" */
    error(A_ERR, "dirsrv", "Can't open start/host database");
    ret_status = PRARCH_BAD_ARG;
    goto atend;
  }

  if ( !(strhan = archNewStrIdx()) ) {
    plog(L_DB_INFO, NOREQ, "Can't setup strings handle before opening strings files." );
    error(A_ERR,"dirsrv", "Cannot create string handler");
    ret_status = PRARCH_BAD_ARG;
    goto atend;
  }
  
  if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH ) ){
    error( A_ERR, "dirsrv", "Could not find strings files, abort." );
    ret_status = PRARCH_BAD_ARG;
    goto atend;
  }
  
  /* 96-04-09 */

  if (site_file == (file_info_t *) NULL)
  {
    site_file = create_finfo();
  }

  search_req -> curr_type = search_req -> orig_type;

  if (search_req->search_str[0] == '\0')
  {
    ret_status = PRARCH_SUCCESS;
    goto atend;
  }

  if (ob->links)
  {
    plog(L_DIR_ERR, NOREQ, "parchie_search_files_db handed non empty dir");
    ret_status = PRARCH_BAD_ARG;
    goto atend;
  }

  if ((search_req->orig_type == S_FULL_REGEX)
      || (search_req->orig_type == S_E_FULL_REGEX)
      || (search_req->orig_type == S_X_REGEX) 
      || (search_req->orig_type == S_E_X_REGEX))
  {
    /* Regular expression search begins with ".*", strip it*/

    if ((slen = strlen(search_req -> search_str)) >= 2)
    {
      if (strncmp(search_req -> search_str, ".*", 2) == 0)
      {
        search_string_t tmp_srch;

        strcpy(tmp_srch, search_req -> search_str + 2);
        strcpy(search_req -> search_str, tmp_srch);

        if (search_req -> search_str[0] == '\0')
        {
          ret_status = PRARCH_SUCCESS;
          goto atend;
        }
      }

      /* Regular expression search ends with ".*", strip it*/

      if (strcmp(search_req -> search_str + slen - 2, ".*") == 0)
      {
        search_string_t tmp_srch;

        strncpy(tmp_srch, search_req -> search_str, slen - 2);
        tmp_srch[slen - 2] = '\0';
        strcpy(search_req -> search_str, tmp_srch);

        if (search_req -> search_str[0] == '\0')
        {
          ret_status = PRARCH_SUCCESS;
          goto atend;
        }
      }
    }
  }

  /* See if we can use a less expensive search method */

  if ((search_req->orig_type == S_FULL_REGEX) &&
      (strpbrk(search_req -> search_str,"\\^$.,[]<>*+?|(){}/") == NULL))
  {
    search_req->curr_type = S_SUB_CASE_STR;
  }
  else
  {
    if ((search_req->orig_type == S_E_FULL_REGEX) &&
        (strpbrk(search_req -> search_str,"\\^$.,[]<>*+?|(){}/") == NULL))
    {
      search_req->curr_type = S_E_SUB_CASE_STR;
    }
  }

  if ((search_req->orig_type == S_X_REGEX) &&
      (strpbrk(search_req->search_str, "\\^$.,[]<>*+?|(){}/") == NULL))
  {
    search_req -> curr_type = S_SUB_KASE;
  }
  else if ((search_req->orig_type == S_E_X_REGEX) &&
           (search_req -> orig_type == S_E_X_REGEX))
  {
    search_req->curr_type = S_E_SUB_KASE;
  }

  SET_ARCHIE_DIR(search_req -> attrib_list);

  /* If they have asked for 0 max_filename_hits, then make it as much
     as the server internal defaults will allow */

  if(search_req->maxhits == 0) search_req->maxhits = DEFAULT_MAXHITS;
  if(search_req->maxmatch == 0) search_req->maxmatch = DEFAULT_MAXMATCH;
  if(search_req->maxhitpm == 0) search_req->maxhitpm = DEFAULT_MAXHITPM;

  comp_rest[0] = domains[0] = '\0';
  if (search_req->maxhits > MAX_MAXHITS)
  {
    search_req->maxhits = MAX_MAXHITS;
    sprintf(tmp_str, "Largest number of total hits is %d", MAX_MAXHITS);
    search_req->error_string = strdup(tmp_str);
  }

  if (search_req->maxmatch > MAX_MAXMATCH)
  {
    search_req->maxmatch = MAX_MAXMATCH;
    sprintf(tmp_str, "Largest number of matching strings is %d", MAX_MAXMATCH);
    search_req->error_string = strdup(tmp_str);
  }

  if (search_req->maxhitpm > MAX_MAXHITPM)
  {
    search_req->maxhitpm = MAX_MAXHITPM;
    sprintf(tmp_str, "Largest number of filename hits per matching string is %d", MAX_MAXHITPM);
    search_req->error_string = strdup(tmp_str);
  }
#if 0
  if( ! (index_list = (index_t *)malloc(((search_req->maxmatch) + 1) * sizeof(index_t))))
  {
    ret_status = PRARCH_OUT_OF_MEMORY;
    goto atend;
  }
#endif
  /* If there are pathname component restrictions on the search, then
     break them out here */

  if (search_req->comp_restrict)
  {
    if (cvt_token_to_internal(search_req->comp_restrict, comp_rest, NET_DELIM_CHAR) == ERROR)
    {
      ret_status = PRARCH_BAD_ARG;
      goto atend;
    }
    restricts = str_sep(comp_rest, NET_DELIM_CHAR);
  }

  /* Same with the domain list */

  if (search_req->domains)
  {
    if (cvt_token_to_internal(search_req->domains, domains, NET_DELIM_CHAR) == ERROR)
    {
      ret_status = PRARCH_BAD_ARG;
      goto atend;
    }

    if (compile_domains(domains, domain_list, domaindb, &domain_count) == ERROR)
    {
      plog(L_DB_INFO, NOREQ, "Loop in domain database detected. Aborting search");
      ret_status = PRARCH_BAD_DOMAINDB;
      goto atend;
    }
  }

  if (check_cache(search_req, &(ob->links), domains, comp_rest, a_mcache) == TRUE)
  {
    plog(L_DB_INFO, NOREQ, "Responding with cached data");
#ifdef ARCHIE_TIMING
    time_req->cached = 'C';
#endif   
    ret_status = PSUCCESS;
    goto atend;
  }


  /* 96-04-09 */


#ifdef TIMING
  gettimeofday(&tp1,NULL);
#endif

#ifdef ARCHIE_TIMING
  getrusage(RUSAGE_SELF,&time_req->stime_start);
  memcpy(&time_req->stime_end,&time_req->stime_end, sizeof(struct rusage));
#endif
  switch (search_req->curr_type)
  {
  case S_E_FULL_REGEX:
  case S_E_SUB_CASE_STR:
  case S_E_SUB_NCASE_STR:
  case S_EXACT:
  case S_E_X_REGEX:
  case S_E_ZUB_NCASE:
  case S_E_SUB_KASE:
  case S_NOATTRIB_EXACT:

#ifdef TIMING
    timing_str = "exact";
#endif


    /* 96-04-09 */
    type = EXACT;
    if (archQuery(strhan, search_req->search_str, type, 0, search_req->maxhits,
                  search_req->maxmatch, search_req->maxhitpm, restricts,
                  (char *)NULL, PATH_AND, (-1), domain_list, domain_count,
                  db, start_db, hostbyaddr, &result, &strs, &ret_hits, &start_ret,
                  ardp_accept, 50, format) == ERROR){
      sprintf(tmp_str, "archQuery failed");
      search_req->error_string = strdup(tmp_str);
      ret_status =  PRARCH_BAD_ARG;
      goto atend;
    }else if (ret_hits <= 0)
    {
      if ((search_req->curr_type == S_E_FULL_REGEX) ||
          (search_req->curr_type == S_E_X_REGEX))
      {
        type = REGEX;
        case_sens = 0;
        goto s_full_regex;
      }
	    else if ((search_req->curr_type == S_E_SUB_CASE_STR) ||
               (search_req->curr_type == S_E_SUB_KASE))
      {
        type = SUB;
        case_sens = 1;
        goto s_sub_case_str;
      }
	    else if ((search_req->curr_type == S_E_SUB_NCASE_STR) ||
               (search_req->curr_type == S_E_ZUB_NCASE))
      {
        type = SUB;
        case_sens = 0;
        goto s_sub_ncase_str;
      }
	    else
      {
        goto atend;
      }
    }
    break;

  s_full_regex:

  case S_X_REGEX:
  case S_FULL_REGEX:
#ifdef TIMING
    timing_str = "regex";
#endif

    type = REGEX;

    if (archQuery(strhan, search_req->search_str, type, case_sens, search_req->maxhits,
                  search_req->maxmatch, search_req->maxhitpm, restricts,
                  (char *)NULL, PATH_AND, (-1), domain_list, domain_count,
                  db, start_db, hostbyaddr, &result, &strs, &ret_hits, &start_ret,
                  ardp_accept, 50, format) == ERROR){
      sprintf(tmp_str, "archQuery failed");
      search_req->error_string = strdup(tmp_str);
	    ret_status = PRARCH_BAD_REGEX;
	    goto atend;
    }
    break;

  s_sub_case_str:

  case S_SUB_KASE:
  case S_SUB_CASE_STR:

    type = SUB;
    case_sens = 1;

    if (archQuery(strhan, search_req->search_str, type, case_sens, search_req->maxhits,
                  search_req->maxmatch, search_req->maxhitpm, restricts,
                  (char *)NULL, PATH_AND, (-1), domain_list, domain_count,
                  db, start_db, hostbyaddr, &result, &strs, &ret_hits, &start_ret,
                  ardp_accept, 50, format) == ERROR){
      sprintf(tmp_str, "archQuery failed");
      search_req->error_string = strdup(tmp_str);
      ret_status = PRARCH_BAD_ARG ; /* BUG: fix later */
      goto atend;
      
    }
    break;


  s_sub_ncase_str:

  case S_ZUB_NCASE:
  case S_SUB_NCASE_STR:
#ifdef TIMING
    timing_str = "sub";
#endif

    type = SUB;
    case_sens = 0;

    if (archQuery(strhan, search_req->search_str, type, case_sens, search_req->maxhits,
                  search_req->maxmatch, search_req->maxhitpm, restricts,
                  (char *)NULL, PATH_AND, (-1), domain_list, domain_count,
                  db, start_db, hostbyaddr, &result, &strs, &ret_hits, &start_ret,
                  ardp_accept, 50, format) == ERROR){
      plog(L_DB_INFO, NOREQ, "No Match");
      sprintf(tmp_str, "archQuery failed");
      search_req->error_string = strdup(tmp_str);
      ret_status = PRARCH_BAD_ARG ; /* BUG: fix later */
      goto atend;
    }
    break;

  default:
    type = EXACT;
    if (archQuery(strhan, search_req->search_str, type, 0, search_req->maxhits,
                  search_req->maxmatch, search_req->maxhitpm, restricts,
                  (char *)NULL, PATH_AND, (-1), domain_list, domain_count,
                  db, start_db, hostbyaddr, &result, &strs, &ret_hits, &start_ret,
                  ardp_accept, 50, format) == ERROR){
      sprintf(tmp_str, "archQuery failed");
      search_req->error_string = strdup(tmp_str);
      ret_status =  PRARCH_BAD_ARG;
      goto atend;
    }
    break;
  }


  /* 96-04-09 */


#ifdef ARCHIE_TIMING
  getrusage(RUSAGE_SELF,&time_req->stime_end);
#endif

#ifdef TIMING
  gettimeofday(&tp2,NULL);

  tp2.tv_sec -= tp1.tv_sec;
  tp2.tv_usec -= tp1.tv_usec;
  if ( tp2.tv_usec < 0 ) {
    tp2.tv_usec = 1000000 + tp2.tv_usec;
    if ( tp2.tv_sec >= 1 )
    tp2.tv_sec--;
  }


  plog(L_DB_INFO,NOREQ,"ELAPSED TIME search: %ld sec %ld usec, search: %s",tp2.tv_sec,tp2.tv_usec,timing_str);
  gettimeofday(&tp1,NULL);
#endif

#ifdef ARCHIE_TIMING
  getrusage(RUSAGE_SELF,&time_req->htime_start);
  memcpy(&time_req->htime_end,&time_req->htime_start,sizeof(struct rusage));
#endif

  /* Have list of indices which might point to what you want */

#ifdef ARCHIE_TIMING

  for (count = 0, total = 0; count < ret_hits; count++)

#else

  for (count = 0, total = 0; (count < ret_hits) && (total != search_req -> maxhits);
       count++)
#endif

  {
    /* check if anything has come in during the search */

    switch(search_req->curr_type)
    {
    case S_E_X_REGEX:
    case S_X_REGEX:
    case S_E_SUB_KASE:
    case S_SUB_KASE:
    case S_E_ZUB_NCASE:
    case S_ZUB_NCASE:
    case S_NOATTRIB_EXACT:
	    curr_link = (VLINK) vlalloc();
	    if (curr_link)
      {
        curr_link->name = stcopyr( result[count].qrystr, curr_link->name);
        curr_link->target = stcopyr("DIRECTORY", curr_link->target);
        curr_link->linktype = 'L';
        curr_link->host = stcopyr(hostwport, curr_link->host);

        sprintf(tmp_str, "ARCHIE/MATCH(%d,%d,%d,0,=)/%s", search_req->maxhits,
                search_req->maxmatch, search_req->maxhitpm, curr_link->name);
        curr_link->hsoname = stcopyr(tmp_str, curr_link->hsoname);

#ifdef 1
        if (search_req->domains)
        {
          FILTER nf;

          nf = (FILTER)flalloc();
          if (nf)
          {
            nf->name = stcopyr("AR_DOMAIN", nf->name);
            nf->type = FIL_DIRECTORY;
            nf->execution_location = FIL_SERVER;
            nf->pre_or_post = FIL_PRE;
            nf->args = tkcopy(search_req->domains);
            APPEND_ITEM(nf, curr_link->filters);
          }
        }
        if (search_req->comp_restrict)
        {
          FILTER nf;

          nf = (FILTER)flalloc();
          if (nf)
          {
            nf->name = stcopyr("AR_PATHCOMP", nf->name);
            nf->type = FIL_DIRECTORY;
            nf->execution_location = FIL_SERVER;
            nf->pre_or_post = FIL_PRE;
            nf->args = tkcopy(search_req->comp_restrict);
            APPEND_ITEM(nf, curr_link->filters);
          }
        }
#endif
        /* bug? should `ob' be `ob->links'?
           I'll assume it should -wheelan */
        APPEND_ITEM(curr_link, ob->links);
        continue;
	    }
	    else
      {
        plog(L_DB_INFO,NOREQ, "Can't allocate link!");
        ret_status = PRARCH_OUT_OF_MEMORY;
        goto atend;
	    }
	    break;

    default:
	    break;
    }
  

    new_addr = result[count].ipaddr;
    memset(&hostdb_rec, '\0', sizeof(hostdb_t));
    memset(&hostaux_rec, '\0',sizeof(hostdb_aux_t));

    if ( ! chostname &&
        ! (chostname = get_anon_preferred_name(new_addr,
                                               primary_hostname, hostdb,
                                               hostaux_db, hostbyaddr))) {
      goto next_one;
    }
  

    /* Now allocate and set up the link */

    curr_link = (VLINK)vlalloc();
    if ( !curr_link )
    {
      plog(L_DB_INFO, NOREQ, "Can't allocate link");
      ret_status = PRARCH_OUT_OF_MEMORY;
      goto atend;
    }

    curr_link->name = stcopyr(result[count].qrystr, curr_link->name);
    tmp_str[0] = '\0';

    if (CSE_IS_DIR( result[count].details ))
    {
      /*
       * It's a directory - we should check to see if the site is
       * running prospero, and if so return a pointer to the actual
       * directory.  If it isn't then we return a real pointer to a
       * pseudo-directory maintained by this archie server.
       */
      curr_link->target = stcopyr("DIRECTORY", curr_link->target);
      if (GET_ARCHIE_DIR(search_req->attrib_list) ||
          1                     /* or host not running prospero */)
      {
        curr_link->host = stcopyr(hostwport, curr_link->host);
        sprintf(tmp_str, "ARCHIE/HOST/%s", result[count].str);
        curr_link->hsoname = stcopyr(tmp_str, curr_link->hsoname);
      }
      /*
       *  else { curr_link->host = stcopyr(chostname, curr_link->host);
       *  sprintf(tmp_str, "AFTP%s", ); else sprintf(tmp_str, "ARCHIE/%s",
       *  fullpath);
       *  
       *    curr_link->hsoname = stcopyr(fullpath, curr_link->hsoname);
       *  }
       */

    }
    else
    {
      /*
       * It's a file - we should check to see if the site is running
       * prospero, and if so return a pointer to the real file.  If it
       * isn't, then we generate an external link
       */
      curr_link->target = stcopyr("EXTERNAL", curr_link->target);
      ad2l_am_atr(curr_link, "AFTP", "BINARY", NULL);
      UNSET_ARCHIE_DIR(search_req->attrib_list);

      curr_link->host = stcopyr(chostname, curr_link->host); 

      if( !(tmp_s = strstr(result[count].str,"/")) ){
        sprintf(tmp_str, "%s", result[count].str);
      }else sprintf(tmp_str, "%s", tmp_s);

      curr_link->hsoname = stcopyr(tmp_str, curr_link->hsoname);
    }

    free(  result[count].str );
    result[count].str = NULL;

    /* Subcomponents look like pointers into mmapped stringsdb */

    /*
     *  #if 0 if (path_name) free(path_name); #endif
     */

    if (hostdb_rec.primary_hostname[0] == '\0')
    {
      if (get_dbm_entry(primary_hostname, strlen(primary_hostname) + 1, &hostdb_rec, hostdb)
          == ERROR)
      {
        vllfree(curr_link);
        goto next_one;
      }
    }

    if (hostaux_rec.origin == NULL  || hostaux_rec.origin->access_methods[0] == '\0')
    {
      sprintf(tmp_str, "%s.%s.0", primary_hostname, ANONFTP_DB_NAME);
      if (get_dbm_entry(tmp_str, strlen(tmp_str) + 1, &hostaux_rec, hostaux_db) == ERROR)
      {
        vllfree(curr_link);
        goto next_one;
      }

      if (hostaux_rec.current_status != ACTIVE)
      {
        vllfree(curr_link);
        goto next_one;
      }
    }

    /* Do each attribute in turn */
    if (GET_AR_H_IP_ADDR(search_req->attrib_list))
    {
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                   NAME_AR_H_IP_ADDR, inet_ntoa(ipaddr_to_inet(new_addr)),
                   NULL);
    }

    if (GET_AR_H_OS_TYPE(search_req->attrib_list))
    {
      char *ost;

      switch (hostdb_rec.os_type)
      {
      case UNIX_BSD:
        ost = OS_TYPE_UNIX_BSD;
        break;
			
      case VMS_STD:
        ost = OS_TYPE_VMS_STD;
        break;

      default:
        ost = "Unknown";
        break;
      }

      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                   NAME_AR_H_OS_TYPE, ost, NULL);
    }
		 
    if (GET_AR_H_TIMEZ(search_req->attrib_list))
    {
      sprintf(tmp_str, "%+ld", (long)hostdb_rec.timezone);
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                   NAME_AR_H_TIMEZ, tmp_str, NULL);
    }

    if (GET_AUTHORITY(search_req->attrib_list))
    {
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                   NAME_AUTHORITY, hostaux_rec.source_archie_hostname,
                   NULL);
    }

    if (GET_LK_LAST_MOD(search_req->attrib_list))
    {
      /*        sprintf(tmp_str, "%sZ", cvt_from_inttime(site_ptr->core.date));*/
      sprintf(tmp_str, "%sZ", cvt_from_inttime(result[count].details.type.file.date));        
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_INTRINSIC,
                   NAME_LK_LAST_MOD, tmp_str, NULL);
    }

    if (GET_AR_H_LAST_MOD(search_req->attrib_list))
    {
      sprintf(tmp_str, "%sZ", cvt_from_inttime(hostaux_rec.update_time));
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                   NAME_AR_H_LAST_MOD, tmp_str, NULL);
    }
		     
    if (GET_AR_RECNO(search_req->attrib_list))
    {
      sprintf(tmp_str, "%ld", (long)hostaux_rec.no_recs);
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                   NAME_AR_RECNO, tmp_str, NULL);
    }

    if (GET_LINK_SIZE(search_req->attrib_list))
    {
      sprintf(tmp_str, "%ld bytes", (long)result[count].details.type.file.size);
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_INTRINSIC,
                   NAME_LINK_SIZE, tmp_str, NULL);
    }

    if (GET_NATIVE_MODES(search_req->attrib_list))
    {
      sprintf(tmp_str, "%d", result[count].details.type.file.perms);
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_INTRINSIC,
                   NAME_NATIVE_MODES, tmp_str, NULL);
    }

    if (GET_LK_UNIX_MODES(search_req->attrib_list))
    {
      sprintf(tmp_str, "%s",
              unix_perms_itoa(result[count].details.type.file.perms,
                              CSE_IS_DIR(result[count].details), 0));
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_INTRINSIC,
                   NAME_LK_UNIX_MODES, tmp_str, NULL);
    }
    
    APPEND_ITEM(curr_link, ob->links);

  next_one:

    total++;
    old_addr = new_addr;
    if( total < search_req -> maxhits && total < ret_hits){
      new_addr = result[total].ipaddr;
      if(old_addr!=new_addr) chostname = (char *)NULL;
    }
  } /* for */

#ifdef ARCHIE_TIMING
  if ( total < search_req->maxhits ) {
    getrusage(RUSAGE_SELF,&time_req->htime_end);
  }
#endif


#ifdef TIMING
  gettimeofday(&tp2,NULL);

  tp2.tv_sec -= tp1.tv_sec;
  tp2.tv_usec -= tp1.tv_usec;
  if ( tp2.tv_usec < 0 ) {
    tp2.tv_usec = 1000000 + tp2.tv_usec;
    if ( tp2.tv_sec >= 1 )
    tp2.tv_sec--;
  }

  plog(L_DB_INFO,NOREQ,"ELAPSED TIME build:  %ld sec %ld usec, matches: %d",tp2.tv_sec,tp2.tv_usec,total);
#endif

  /* Set curr_offset to the number of the last link */

  add_to_cache(ob->links, search_req, domains, comp_rest, &a_mcache, &a_mcachecount); 

 atend:
  free_strings( &strs,search_req->maxhits );
  if ( result != NULL ) free(result);
  
  search_req->no_matches = total;

  archCloseStrIdx( strhan);
  archFreeStrIdx(&strhan);
/*  if( strhan )  archCloseStrIdx( &strhan);*/
  close_start_dbs(start_db,NULL);
  destroy_finfo(start_db);

  if (restricts) free_opts(restricts);

  return ret_status;

}

#ifdef 0
char **find_ancestors( site_begin, site_entry, strings)
  void *site_begin;
  full_site_entry_t *site_entry;
  file_info_t *strings;
{
  static char *pathname[MAX_PCOMPS];
  register full_site_entry_t *my_sentry;
  register int curr_path_comps;

  memset(pathname, '\0', MAX_PCOMPS * sizeof(char *));

  if (site_entry->core.parent_idx == -1)
  {
    pathname[0] = (char *)NULL;
    return pathname;
  }

  for (curr_path_comps = 0,
       my_sentry = (full_site_entry_t *)site_begin + site_entry->core.parent_idx;
       my_sentry->core.parent_idx != -1;
       curr_path_comps++)
  {
    pathname[curr_path_comps] = strings->ptr + my_sentry->str_ind;
    my_sentry = (full_site_entry_t *)site_begin + my_sentry->core.parent_idx;
  }

  pathname[curr_path_comps] = strings->ptr + my_sentry->str_ind;
  pathname[++curr_path_comps] = (char *)NULL;

  return pathname;
}
#endif

char **new_find_ancestors( site_begin, site_entry, strhan)
  void *site_begin;
  full_site_entry_t *site_entry;
  struct arch_stridx_handle *strhan;
{
  static char *pathname[MAX_PCOMPS];
  register full_site_entry_t *my_sentry;
  register int curr_path_comps=0;
  full_site_entry_t *parent_file;
  char *strres;

  memset(pathname, '\0', MAX_PCOMPS * sizeof(char *));

  if( site_entry->core.prnt_entry.strt_2 >= 0 ){
    parent_file = (full_site_entry_t *)site_begin + site_entry->core.prnt_entry.strt_2;
  }else goto wrong;

  if( site_entry->strt_1 >= 0 ){
    my_sentry = (full_site_entry_t *)site_begin + site_entry->strt_1;
  }else goto last;
  
  while( (parent_file) && parent_file->strt_1 >= 0){
    if( !archGetString( strhan, parent_file->strt_1, &strres) ){
      /*       error(A_ERR, "archQuery","Could not perform exact search successfully.");*/
      pathname[0] = (char *)NULL;
      return pathname;
    }
    pathname[curr_path_comps] = strres;
    pathname[++curr_path_comps] = (char *)NULL;
    if( my_sentry->core.prnt_entry.strt_2 >= 0 ){
      parent_file = (full_site_entry_t *)site_begin + my_sentry->core.prnt_entry.strt_2;
    }else goto wrong;
    if( my_sentry->strt_1 >= 0 ){
      my_sentry = (full_site_entry_t *)site_begin + my_sentry->strt_1;
    }else goto last;
  }
      
 last:
  if( !archGetString( strhan, parent_file->strt_1, &strres) ){
    /*       error(A_ERR, "archQuery","Could not perform exact search successfully.");*/
    pathname[0] = (char *)NULL;
    return pathname;
  }

  pathname[curr_path_comps] = strres;
  pathname[++curr_path_comps] = (char *)NULL;
 wrong:
  return pathname;
}



static char *get_anon_preferred_name(address, primary_hostname, hostdb, hostaux_db, hostbyaddr)
  ip_addr_t address;
  hostname_t primary_hostname;
  file_info_t *hostdb, *hostaux_db, *hostbyaddr;
{
  static hostname_t  hostname;
  hostbyaddr_t	  hostbyaddr_rec;
  hostdb_t	  hostdb_rec;
  hostdb_aux_t hostaux_rec;

  if(get_dbm_entry(&address, sizeof(ip_addr_t), &hostbyaddr_rec, hostbyaddr) == ERROR){

    /* "Can't find address in host address cache" */

    error(A_ERR, "get_preferred_name", "Can't find address in host address cache" );
    return((char *) NULL);
  }

  strcpy(primary_hostname, hostbyaddr_rec.primary_hostname);

  if(get_dbm_entry(hostbyaddr_rec.primary_hostname, strlen(hostbyaddr_rec.primary_hostname) + 1, &hostdb_rec, hostdb) == ERROR){

    /* "Can't find name from host address cache in primary host database" */

    error(A_ERR, "get_preferred_name", "Can't find name from host address cache in primary host database" );
    return((char *) NULL);
  }

  if ( get_hostaux_entry(primary_hostname,ANONFTP_DB_NAME,(index_t)0,
                         &hostaux_rec, hostaux_db) == ERROR ) {

    /* "Can't find name from host address cache in primary host database" */

    error(A_ERR, "get_preferred_name", "Can't find name from host address cache in primary host database" );
    return((char *) NULL);
  }

  if(hostaux_rec.preferred_hostname[0] == '\0')
  strcpy(hostname, hostdb_rec.primary_hostname);
  else
  strcpy(hostname, hostaux_rec.preferred_hostname);

  return hostname;
}
