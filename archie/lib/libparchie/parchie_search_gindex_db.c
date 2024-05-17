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
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "typedef.h"
#include "gstrings_index.h"
#include "gsite_file.h"
#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif
#include "ardp.h"
#include "plog.h"
#include "ar_search.h"
#include "gindexdb_ops.h"
#include "error.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "times.h"
#include "parchie_cache.h"
#include "parchie_search_gindex_db.h"

static char *get_gopher_preferred_name();
/* to prevent conflicts with the macro definition of "filename"
   pfs.h */

#undef filename   

#ifdef timezone
#undef timezone
#endif

#ifdef index
#undef index
#endif


static struct match_cache *g_mcache = NULL;
static int  g_mcachecount = 0;
extern char	hostname[];
extern char	hostwport[];
extern int ad2l_seq_atr(VLINK l, char precedence, char nature, char *aname, ...);
static file_info_t	  *site_file = (file_info_t *) NULL;


int parchie_search_gindex_db(strings, strings_idx, strings_hash, domaindb, hostdb,
                             hostaux_db, hostbyaddr, sel_finfo, host_finfo, search_req, ob,out_format)
  file_info_t	*strings;
  file_info_t	*strings_idx;
  file_info_t	*strings_hash;   
  file_info_t	*domaindb;
  file_info_t *hostdb;
  file_info_t *hostaux_db;
  file_info_t *hostbyaddr;
  file_info_t *sel_finfo;
  file_info_t *host_finfo;
  search_req_t *search_req;
  P_OBJECT ob;
  int out_format;
{
  VLINK curr_link;
  char **path_name=NULL;
  char **restricts=NULL;
  char *chostname = (char*)NULL;
  char fullpath[2048];
  char tmp_str[2048];
  domain_t domain_list[MAX_NO_DOMAINS];
  domain_t domains;
  gfull_site_entry_t *site_ptr = NULL;
  gsite_entry_ptr_t site_entry;
  hostdb_aux_t hostaux_rec;
  hostdb_t hostdb_rec;
  hostname_t primary_hostname;
  index_t *index_list = NULL;
  int domain_count;
  int linkcount = 0;
  int ret_status = PRARCH_SUCCESS;
  int slen;
  int this_host = 1;
  ip_addr_t new_addr;
  ip_addr_t old_addr = 0;
  pathname_t comp_rest;
  register int count = 0;
  register int total = 0;
  strings_idx_t *s_index = NULL;

#ifdef TIMING
  struct timeval tp1,tp2;
  char *timing_str = "unknown";
#endif

  if (site_file == (file_info_t *)NULL)
  {
    site_file = create_finfo();
  }

  search_req->curr_type = search_req->orig_type;
  if (search_req->search_str[0] == '\0')
  {
    return PRARCH_SUCCESS;
  }

  if (ob->links)
  {
    plog(L_DIR_ERR, NOREQ, "parchie_search_gindex_db handed non empty dir");
    return PRARCH_BAD_ARG;
  }

  if ((search_req->orig_type == S_FULL_REGEX)
      || (search_req->orig_type == S_E_FULL_REGEX)
      || (search_req->orig_type == S_X_REGEX) 
      || (search_req->orig_type == S_E_X_REGEX))
  {
    /* Regular expression search begins with ".*", strip it*/

    if ((slen = strlen(search_req->search_str)) >= 2)
    {
      if (strncmp(search_req->search_str, ".*", 2) == 0)
      {
        search_string_t tmp_srch;

        strcpy(tmp_srch, search_req->search_str + 2);
        strcpy(search_req->search_str, tmp_srch);

        if(search_req->search_str[0] == '\0')
        {
          return PRARCH_SUCCESS;
        }
      }

      /* Regular expression search ends with ".*", strip it*/

      if (strcmp(search_req->search_str + slen - 2, ".*") == 0)
      {
        search_string_t tmp_srch;

        strncpy(tmp_srch, search_req->search_str, slen - 2);
        tmp_srch[slen - 2] = '\0';
        strcpy(search_req->search_str, tmp_srch);

        if (search_req->search_str[0] == '\0')
        {
          return PRARCH_SUCCESS;
        }
      }
    }
  }

  /* See if we can use a less expensive search method */

  if ((search_req->orig_type == S_FULL_REGEX) &&
      (strpbrk(search_req->search_str, "\\^$.,[]<>*+?|(){}/") == NULL))
  {
    search_req->curr_type = S_SUB_CASE_STR;
  }
  else
  {
    if ((search_req->orig_type == S_E_FULL_REGEX) &&
        (strpbrk(search_req->search_str, "\\^$.,[]<>*+?|(){}/") == NULL))
    {
      search_req->curr_type = S_E_SUB_CASE_STR;
    }
  }

  if ((search_req->orig_type == S_X_REGEX) &&
      (strpbrk(search_req->search_str, "\\^$.,[]<>*+?|(){}/") == NULL))
  {
    search_req -> curr_type = S_SUB_KASE;
  }
  else if ((search_req->orig_type == S_E_X_REGEX) && (search_req->orig_type == S_E_X_REGEX))
  {
    search_req->curr_type = S_E_SUB_KASE;
  }

  SET_ARCHIE_DIR(search_req->attrib_list);

  /* If they have asked for 0 max_filename_hits, then make it as much
     as the server internal defaults will allow */

  if (search_req->maxhits == 0) search_req->maxhits = DEFAULT_MAXHITS;
  if (search_req->maxmatch == 0) search_req->maxmatch = DEFAULT_MAXMATCH;
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

  if ((index_list = (index_t *)malloc(((search_req->maxmatch) + 1) * sizeof(index_t)))
      == (index_t *) NULL)
  {
    return PRARCH_OUT_OF_MEMORY;
  }

  /* If there are pathname component restrictions on the search, then
     break them out here */

  if (search_req->comp_restrict)
  {
    if (cvt_token_to_internal(search_req->comp_restrict, comp_rest, NET_DELIM_CHAR) == ERROR)
    {
      return PRARCH_BAD_ARG;
    }

    restricts = str_sep(comp_rest, NET_DELIM_CHAR);
  }

  /* Same with the domain list */

  if(search_req->domains)
  {
    if (cvt_token_to_internal(search_req->domains, domains, NET_DELIM_CHAR) == ERROR)
    {
      return PRARCH_BAD_ARG;
    }

    if (compile_domains(domains, domain_list, domaindb, &domain_count) == ERROR)
    {
      plog(L_DB_INFO, NOREQ, "Loop in domain database detected. Aborting search");
      return PRARCH_BAD_DOMAINDB;
    }
  }

  if (check_cache(search_req,&(ob->links), domains, comp_rest, g_mcache) == TRUE)
  {
    plog(L_DB_INFO, NOREQ, "Responding with cached data");
    return PSUCCESS;
  }

  if (mmap_file(strings_idx, O_RDONLY) != A_OK)
  {
    plog(L_DB_INFO,NOREQ, "Can't mmap strings_idx db??");
    return PRARCH_BAD_MMAP;
  }

#ifdef TIMING
  gettimeofday(&tp1,NULL);
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
    if (mmap_file(strings, O_RDONLY) != A_OK)
    {
      plog(L_DB_INFO,NOREQ, "Can't mmap strings db??");
      ret_status = PRARCH_BAD_MMAP;
      goto atend;
    }

    if (ar_exact_match( search_req, index_list, strings_hash ) != A_OK)
    {
      if ((search_req -> curr_type == S_E_FULL_REGEX) ||
          (search_req -> curr_type == S_E_X_REGEX))
      {
        goto s_full_regex;
      }
	    else if ((search_req->curr_type == S_E_SUB_CASE_STR) ||
               (search_req -> curr_type == S_E_SUB_KASE))
      {
        goto s_sub_case_str;
      }
	    else if ((search_req->curr_type == S_E_SUB_NCASE_STR) ||
               (search_req -> curr_type == S_E_ZUB_NCASE))
      {
        goto s_sub_ncase_str;
      }
	    else
      {
        goto atend;
      }
    }
    break;

  s_full_regex: /* label */
  case S_X_REGEX:
  case S_FULL_REGEX:
#ifdef TIMING
   timing_str = "regex";
#endif
    if (strings->ptr == (char *)NULL)
    {
	    if (mmap_file(strings, O_RDONLY) != A_OK)
      {
        plog(L_DB_INFO,NOREQ, "Can't mmap strings db??");
        ret_status = PRARCH_BAD_MMAP;
        goto atend;
	    }
    }
    if (ar_regex_match(search_req, index_list, strings_idx, strings) != A_OK)
    {
	    ret_status = PRARCH_BAD_REGEX;
	    goto atend;
    }
    break;

  s_sub_case_str:
  s_sub_ncase_str: /* labels */
  case S_ZUB_NCASE:
  case S_SUB_KASE:
  case S_SUB_CASE_STR:
  case S_SUB_NCASE_STR:
#ifdef TIMING
   timing_str = "sub";
#endif

    if (strings->ptr != (char *)NULL) munmap_file(strings);

    if (mmap_file_private(strings, O_RDWR) != A_OK)
    {
      ret_status = PRARCH_BAD_MMAP;
      goto atend;
    }

    if (ar_sub_match(search_req, index_list, strings_idx, strings) != A_OK)
    {
      ret_status = PRARCH_BAD_ARG ; /* BUG: fix later */
      goto atend;
    }
    break;

  default:
    break;
  }

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

  if (mmap_file(host_finfo, O_RDONLY) == ERROR)
  {

    plog(L_DB_INFO,NOREQ, "Can't mmap hosts file?");
    return PRARCH_BAD_MMAP;
  }
  if (mmap_file(sel_finfo, O_RDONLY) == ERROR)
  {
    plog(L_DB_INFO,NOREQ, "Can't mmap selectors file?");
    return PRARCH_BAD_MMAP;
  }
   
  /* Have list of indices which might point to what you want */

  for (count = 0, total = 0;
       (index_list[count] != -1) && (total != search_req->maxhits);
       count++)
  {
    /* check if anything has come in during the search */

    if ((count > 29) && (count % 30 == 0))
    {
      ardp_accept();
    }

    s_index = ((strings_idx_t *)strings_idx->ptr) + index_list[count];
    if (s_index->index.site_addr == STRING_NOT_ACTIVE) /* Inactive string */
    { continue; }

    site_entry = s_index->index;
    new_addr = site_entry.site_addr;

    for (linkcount = 0;
         (site_entry.site_addr != END_OF_CHAIN)
         && (linkcount != search_req->maxhitpm)
         && (total != search_req->maxhits);
         linkcount++, total++)
    {
      if ((linkcount > 49) && (linkcount % 50 == 0))
      {
        ardp_accept();
      }

      if (old_addr != new_addr)
      {
        chostname = (char *)NULL;
        this_host = 1;

        if(site_file->ptr != (char *)NULL)
        {
          /* mummap performed in the close_file */
          close_file(site_file);
        }
		     
        strcpy(site_file->filename,
               gfiles_db_filename(inet_ntoa(ipaddr_to_inet(site_entry.site_addr))));

        if (open_file(site_file, O_RDONLY) != A_OK)
        {
          plog(L_DB_INFO, NOREQ, "Can't open site file %s", site_file->filename); 
          break;
        }

        if (mmap_file(site_file, O_RDONLY) != A_OK)
        {
          plog(L_DB_INFO, NOREQ, "Can't mmap site file %s", site_file->filename); 
          break;
        }

        memset(&hostdb_rec, '\0', sizeof(hostdb_t));
        memset(&hostaux_rec, '\0', sizeof(hostdb_aux_t));

        old_addr = new_addr;
      }

      /*
       * For S_EXACT An orig_offset which is non zero means skip
       * orig_offset links before continuing with processing
       */

      if ((search_req->curr_type == S_EXACT) && (search_req->orig_offset != 0) &&
          (linkcount != search_req->orig_offset))
      {
        goto next_one;
      }

      if (search_req->domains)
      {
        if ( ! this_host)
        {
          goto next_one;
        }
        else
        {
          if( ! chostname)
          {
            chostname = get_gopher_preferred_name(site_entry.site_addr,
                                           primary_hostname, hostdb, hostaux_db, hostbyaddr);
          }

          if (find_in_domains(chostname, domain_list, domain_count) == 0)
          {
            this_host = 0;
            goto next_one;
          }
        }
      }

      site_ptr = (gfull_site_entry_t *)(site_file->ptr) + site_entry.recno;
      if(search_req->comp_restrict)
      {
        if ((path_name[0] == '\0') || (check_comp_restrict(restricts, path_name) != A_OK))
        {
          free_opts(path_name);
          goto next_one;
        }
      }

      if ( ! chostname)
      {
        chostname = get_gopher_preferred_name(site_entry.site_addr,
                                       primary_hostname, hostdb, hostaux_db,hostbyaddr);
      }

      /* Now allocate and set up the link */

      curr_link = (VLINK)vlalloc();
      if( ! curr_link)
      {
        plog(L_DB_INFO, NOREQ, "Can't allocate link");
        ret_status = PRARCH_OUT_OF_MEMORY;
        goto atend;
      }

#if 0
      if (CSE_IS_DIR(site_ptr->core))
      {
        /* Generate the hsoname */

        sprintf(fullpath, "GOPHER-GW/%s(%d)/%c/%s",
                (host_finfo->ptr) + (site_ptr->host_offset),
                site_ptr->core.port, site_ptr->core.ftype,
                (sel_finfo->ptr) + (site_ptr->sel_offset));

        curr_link->hsoname = stcopyr(fullpath, curr_link->hsoname);
        curr_link->target = stcopyr("DIRECTORY", curr_link->target);
        ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                     "OBJECT-INTERPRETATION", "DIRECTORY", NULL);
      }
      else
      {
        curr_link->hsoname = stcopyr((sel_finfo->ptr) + (site_ptr->sel_offset),
                                     curr_link->hsoname);
        curr_link->target = stcopyr("EXTERNAL", curr_link->target);

        switch (site_ptr->core.ftype)
        {
        case AGOPHER_FILE:
        case AGOPHER_UNIX_UUENCODE:
          ad2l_seq_atr(curr_link, ATR_PREC_REPLACE, ATR_NATURE_APPLICATION,
                       "OBJECT-INTERPRETATION", "DOCUMENT", "TEXT", "ASCII", NULL);
          ad2l_am_atr(curr_link, "GOPHER", "TEXT", NULL);
          break;

        case AGOPHER_INDEX_SEARCH:
          ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                       "OBJECT-INTERPRETATION", "SEARCH", NULL);
          ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                       "QUERY-METHOD", "gopher-query(search-words)", "${search-words}", "", NULL);
          ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                       "QUERY-ARGMENT", "search-words", "Index word(s) to search for",
                       "mandatory char *", "%s", "", NULL);
          break;

        case AGOPHER_TELNET_TEXT:
        case AGOPHER_TN3270_TEXT:
          ad2l_am_atr(curr_link, "TELNET", "", NULL);
          ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                       "OBJECT-INTERPRETATION", "PORTAL", NULL);
          break;
		  
        case AGOPHER_IMAGE_GENERIC:
        case AGOPHER_IMAGE_GIF:
          ad2l_am_atr(curr_link, "GOPHER", "BINARY", NULL);
          break;

        default:
          plog(L_DB_INFO, NOREQ, "Unknown GOPHER type %c", site_ptr->core.ftype);
          vllfree(curr_link);
          continue;
          break;
        }
      }
#endif

      if (hostdb_rec.primary_hostname[0] == '\0')
      {
        if (get_dbm_entry(primary_hostname, strlen(primary_hostname) + 1, &hostdb_rec, hostdb)
            == ERROR)
        {
          vllfree(curr_link);
          goto next_one;
        }
      }

      if ( hostaux_rec.origin == NULL || hostaux_rec.origin->access_methods[0] == '\0')
      {
        sprintf(tmp_str,"%s.%s", primary_hostname, GOPHERINDEX_DB_NAME);
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

      sprintf(fullpath, "NUMERIC %d", total);
      ad2l_seq_atr(curr_link, ATR_PREC_LINK, ATR_NATURE_APPLICATION,
                   "COLLATION-ORDER", fullpath, NULL);

      /* Find the last component of name */

      sprintf(fullpath, "%-40.40s (%s)", (strings->ptr) + (site_ptr->str_ind), chostname);
      curr_link->name = stcopyr(fullpath, curr_link->name);
      sprintf(fullpath, "%s(%d)", (host_finfo->ptr) + (site_ptr->host_offset),
              site_ptr->core.port);
      curr_link->host = stcopyr(fullpath, curr_link->host);

      if ( out_format == GINDEX )  {
	 if(CSE_IS_DIR(site_ptr->core))
	 {
	   sprintf(fullpath, "GOPHER-GW/%s(%d)/%c/%s", (host_finfo->ptr) + (site_ptr->host_offset),
		   site_ptr->core.port, site_ptr->core.ftype,
		   (sel_finfo->ptr) + (site_ptr->sel_offset));
	   curr_link->target = stcopyr("DIRECTORY", curr_link->target);
	 }
	 else
	 {
	   strcpy(fullpath, (sel_finfo->ptr) + (site_ptr->sel_offset));
	   curr_link->target = stcopyr("EXTERNAL", curr_link->target);
	 }
      }
      else {   /* GWINDEX */
	 if(CSE_IS_DIR(site_ptr->core))
	 {
	   sprintf(fullpath, "GOPHER-GW/%s(%d)/%c/%s", (host_finfo->ptr) + (site_ptr->host_offset),
		   site_ptr->core.port, site_ptr->core.ftype,
		   (sel_finfo->ptr) + (site_ptr->sel_offset));
	   curr_link->target = stcopyr("DIRECTORY", curr_link->target);
	   curr_link->host = stcopy(hostwport);
	 }
	 else
	 {
	   strcpy(fullpath, (sel_finfo->ptr) + (site_ptr->sel_offset));
	   curr_link->target = stcopyr("EXTERNAL", curr_link->target);
	 }
      }


      curr_link->hsoname = stcopyr(fullpath, curr_link->hsoname);

      switch (site_ptr->core.ftype)
      {
	    case AGOPHER_DIRECTORY:
        ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                     "OBJECT-INTERPRETATION", "DIRECTORY", NULL);
        ad2l_am_atr(curr_link, "GOPHER", "TEXT", NULL);
        break;

	    case AGOPHER_FILE:
	    case AGOPHER_UNIX_UUENCODE:
        ad2l_seq_atr(curr_link, ATR_PREC_REPLACE, ATR_NATURE_APPLICATION,
                     "OBJECT-INTERPRETATION", "DOCUMENT", "TEXT", "ASCII", NULL);
        ad2l_am_atr(curr_link, "GOPHER", "TEXT", NULL);
        break;

	    case AGOPHER_INDEX_SEARCH:
        ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                     "OBJECT-INTERPRETATION", "SEARCH", NULL);
        ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                     "QUERY-METHOD", "gopher-query(search-words)", "${search-words}", "", NULL);
        ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                     "QUERY-ARGUMENT", "search-words", "Index word(s) to search for",
                     "mandatory char *", "%s", "", NULL);
        break;

	    case AGOPHER_TELNET_TEXT:
	    case AGOPHER_TN3270_TEXT:
        ad2l_am_atr(curr_link, "TELNET", "", NULL);
        ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                     "OBJECT-INTERPRETATION", "PORTAL", NULL);
        break;
	       
	    case AGOPHER_IMAGE_GENERIC:
	    case AGOPHER_IMAGE_GIF:
        ad2l_am_atr(curr_link, "GOPHER", "BINARY", NULL);
        break;

	    default:
        plog(L_DB_INFO, NOREQ, "Unknown GOPHER type %c", site_ptr->core.ftype);
        vllfree(curr_link);
        continue;
        break;
      }

      sprintf(fullpath, "%c%-40.40s (%s)\t%s\t%s\t%d", site_ptr->core.ftype,
              (strings->ptr) + (site_ptr->str_ind), chostname,
              (sel_finfo->ptr) + (site_ptr->sel_offset),
              (host_finfo->ptr) + (site_ptr->host_offset), site_ptr->core.port);
      ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                   "GOPHER-MENU-ITEM", fullpath, NULL);
      APPEND_ITEM(curr_link, ob->links);

    next_one:
      site_entry = ((gfull_site_entry_t *)site_file->ptr  + site_entry.recno)->next;
      new_addr = site_entry.site_addr;
    }
  } /* for */

#ifdef TIMING
  gettimeofday(&tp2,NULL);

  tp2.tv_sec -= tp1.tv_sec;
  tp2.tv_usec -= tp1.tv_usec;
  if ( tp2.tv_usec < 0 ) {
    tp2.tv_usec = 1000000 + tp2.tv_usec;
    if ( tp2.tv_sec >= 1 )
      tp2.tv_sec--;
  }

  plog(L_DB_INFO,NOREQ,"ELAPSED TIME build:  %ld secs %ld usec, matches: %d",tp2.tv_sec,tp2.tv_usec,total);
#endif

  /* Set curr_offset to the number of the last link */

  if (search_req->curr_type == S_EXACT)
  {
    search_req->curr_offset = linkcount;
  }

  add_to_cache(ob->links, search_req, domains, comp_rest, &g_mcache, &g_mcachecount);

 atend:

  if (index_list) free(index_list);

  if (site_file->ptr != (char *)NULL)
  {
    close_file(site_file);
  }

  if (strings_idx->ptr != (char *)NULL)
  {
    munmap_file(strings_idx);
  }
      
  if (strings->ptr != (char *)NULL)
  {
    munmap_file(strings);
  }

  if (host_finfo->ptr != (char *)NULL)
  {
    munmap_file(host_finfo);
  }

  if (sel_finfo->ptr != (char *)NULL)
  {
    munmap_file(sel_finfo);
  }

  search_req->no_matches = total;

  if(restricts) free_opts(restricts);

  return ret_status;
}




static char *get_gopher_preferred_name(address, primary_hostname, hostdb, hostaux_db, hostbyaddr)
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

  if ( get_hostaux_entry(primary_hostname,GOPHERINDEX_DB_NAME,(index_t)0,
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
