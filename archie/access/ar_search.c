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
#include "typedef.h"
#include "strings_index.h"
#ifdef OLD_FILES
#  include "old-site_file.h"
#  include "old-host_db.h"
#else
#  include "site_file.h"
#  include "host_db.h"
#endif
#include "plog.h"
#include "ar_search.h"
#include "db_ops.h"
#include "error.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "times.h"


static struct match_cache *mcache = NULL;
static int		  cachecount = 0;
extern char	hostname[];
extern char	hostwport[];


#ifdef eric
char trash[10000];

#endif /* eric */

int search_files_db(strings, strings_idx, strings_hash, domaindb, hostdb, hostaux_db, hostbyaddr, search_req, vdir)
  file_info_t	*strings;
  file_info_t	*strings_idx;
  file_info_t	*strings_hash;   
  file_info_t	*domaindb;
  file_info_t   *hostdb;
  file_info_t   *hostaux_db;
  file_info_t   *hostbyaddr;
  search_req_t	*search_req;
  VDIR		vdir;
{
  VLINK			curr_link;
  char                  **path_name=NULL;
  char                  **restricts=NULL;
  char			*chostname = (char*) NULL;
  hostname_t		primary_hostname;
  char			fullpath[MAX_PATH_LEN];
  char			tmp_str[MAX_PATH_LEN];
  char                  **tmp_ptr=NULL;
  char                  *tmp_ptr2=NULL;  

  domain_t	     domain_list[MAX_NO_DOMAINS];
  file_info_t	     *site_file  = create_finfo();
  full_site_entry_t  *site_ptr=NULL;
  hostdb_aux_t		hostaux_rec;
  hostdb_t		hostdb_rec;
  index_t             *index_list=NULL;
  int			count;
  int			domain_count;
  int			linkcount;
  int			this_host = 1;
  int			total;
  int			slen;
  ip_addr_t		old_addr = 0;
  ip_addr_t	        new_addr;
  site_entry_ptr_t	site_entry;
  strings_idx_t         *s_index=NULL;




  /* TESTING */

  search_req -> max_uniq_hits = search_req -> max_filename_hits;

  search_req -> curr_type = search_req -> orig_type;


  if(search_req -> search_str[0] == '\0')
  return(PRARCH_SUCCESS);

  if(vdir->links) {
    plog(L_DIR_ERR, NULL, NULL, "search_files_db handed non empty dir",0);
    return(PRARCH_BAD_ARG);
  }


  if((search_req -> orig_type == S_FULL_REGEX)
     || (search_req -> orig_type == S_E_FULL_REGEX)
     || (search_req -> orig_type == S_X_REGEX) 
     || (search_req -> orig_type == S_E_X_REGEX)){

    /* Regular expression search begins with ".*", strip it*/

    if((slen = strlen(search_req -> search_str)) >= 2){

      if(strncmp(search_req -> search_str, ".*", 2) == 0){
        search_string_t tmp_srch;

        strcpy(tmp_srch, search_req -> search_str + 2);
        strcpy(search_req -> search_str, tmp_srch);

        if(search_req -> search_str[0] == '\0')
        return(PRARCH_SUCCESS);

      }

      /* Regular expression search ends with ".*", strip it*/

      if(strcmp(search_req -> search_str + slen - 2, ".*") == 0){
        search_string_t tmp_srch;

        strncpy(tmp_srch, search_req -> search_str, slen - 2);
        tmp_srch[slen - 2] = '\0';
        strcpy(search_req -> search_str, tmp_srch);

        if(search_req -> search_str[0] == '\0')
        return(PRARCH_SUCCESS);
      }
    }

  }

  /* See if we can use a less expensive search method */


  if((search_req -> orig_type == S_FULL_REGEX) && (strpbrk(search_req -> search_str,"\\^$.,[]<>*+?|(){}/") == NULL)) {
    search_req -> curr_type = S_SUB_CASE_STR;
  }
  else {
    if((search_req -> orig_type == S_E_FULL_REGEX) && (strpbrk(search_req -> search_str,"\\^$.,[]<>*+?|(){}/") == NULL)) {
      search_req -> curr_type = S_E_SUB_CASE_STR;
    }

  }

  if((search_req -> orig_type == S_X_REGEX) && (strpbrk(search_req -> search_str,"\\^$.,[]<>*+?|(){}/") == NULL)) {
    search_req -> curr_type = S_SUB_KASE;
  }
  else if((search_req -> orig_type == S_E_X_REGEX) && (search_req -> orig_type == S_E_X_REGEX)) {
    search_req -> curr_type = S_E_SUB_KASE;
  }


  if(check_cache(search_req,&(vdir->links)) == TRUE) {
    plog(L_DB_INFO, NULL, NULL, "Responding with cached data",0);
    return(PSUCCESS);
  }

  SET_ARCHIE_DIR(search_req -> attrib_list);


  /* If they have asked for 0 max_filename_hits, then make it as much
     as the server internal defaults will allow */

  if(search_req -> max_filename_hits == 0)
  search_req -> max_filename_hits = DEFAULT_MAX_FILE_HITS;


  if((index_list = (index_t *) malloc( ((search_req -> max_filename_hits) + 1) * sizeof(index_t))) == (index_t *) NULL) {
    return(PRARCH_OUT_OF_MEMORY);
  }

  /* If there are pathname component restrictions on the search, then
     break them out here */

  if( search_req -> comp_restrict )
  restricts = str_sep(search_req -> comp_restrict,':');

  /* Same with the domain list */

  if( search_req -> domains ){

    if(compile_domains(search_req -> domains, domain_list, domaindb, &domain_count) == ERROR){
      plog(L_DB_INFO, NULL, NULL, "Loop in domain database detected. Aborting search",0);
      return(PRARCH_BAD_DOMAINDB);
    }
  }


  if(mmap_file(strings_idx, O_RDONLY) != A_OK) {
    plog(L_DB_INFO,NULL,NULL,"Can't mmap strings_idx db??");
    return(PRARCH_BAD_MMAP);
  }


      
  switch(search_req -> curr_type) {

  case S_E_FULL_REGEX:
  case S_E_SUB_CASE_STR:
  case S_E_SUB_NCASE_STR:
  case S_EXACT:
  case S_E_X_REGEX:
  case S_E_ZUB_NCASE:
  case S_E_SUB_KASE:

    if(mmap_file(strings, O_RDONLY) != A_OK) {
      plog(L_DB_INFO,NULL,NULL,"Can't mmap strings db??");
	    return(PRARCH_BAD_MMAP);
    }

    if(ar_exact_match( search_req, index_list, strings_hash ) != A_OK) {

      if((search_req -> curr_type == S_E_FULL_REGEX) ||
	       (search_req -> curr_type == S_E_X_REGEX))
      goto s_full_regex;
	    else
      if((search_req -> curr_type == S_E_SUB_CASE_STR) ||
         (search_req -> curr_type == S_E_SUB_KASE))
		  goto s_sub_case_str;
	    else
      if((search_req -> curr_type == S_E_SUB_NCASE_STR) ||
         (search_req -> curr_type == S_E_ZUB_NCASE))
      goto s_sub_ncase_str;
	    else
      return(PRARCH_SUCCESS);
    }

    break;

  s_full_regex:

  case S_X_REGEX:
  case S_FULL_REGEX:


    if(mmap_file(strings, O_RDONLY) != A_OK) {
	    plog(L_DB_INFO,NULL,NULL,"Can't mmap strings db??");
	    return(PRARCH_BAD_MMAP);
    }

    if(ar_regex_match( search_req, index_list, strings_idx, strings ) != A_OK) {
	    return(PRARCH_DB_ERROR);
    }

    break;

  s_sub_case_str:
  s_sub_ncase_str:

  case S_ZUB_NCASE:
  case S_SUB_KASE:
  case S_SUB_CASE_STR:
  case S_SUB_NCASE_STR:

    if(mmap_file_private(strings, O_RDWR) != A_OK) {
      return(PRARCH_BAD_MMAP);
    }

    if(ar_sub_match(search_req, index_list, strings_idx, strings) != A_OK){
      return PRARCH_BAD_ARG ;   /* BUG: fix later */
    }

    break;

  default:

    break;
  }


  /* Have list of indices which might point to what you want */

  for(count = 0, total = 0; index_list[count] != -1; count++){

    s_index = ((strings_idx_t *) strings_idx -> ptr) + index_list[count];

    if(s_index -> index.site_addr == STRING_NOT_ACTIVE) /* Inactive string */
    continue;

    switch(search_req -> curr_type){

    case S_E_X_REGEX:
    case S_X_REGEX:
    case S_E_SUB_KASE:
    case S_SUB_KASE:
    case S_E_ZUB_NCASE:
    case S_ZUB_NCASE:

	    curr_link = (VLINK) vlalloc();
	    if(curr_link){


        curr_link -> name = stcopy(strings -> ptr + s_index -> strings_offset, curr_link -> name);
        curr_link -> type = stcopy("DIRECTORY");
        curr_link -> linktype = 'L';

#if 0
        if(gethostname(tmp_str, sizeof(tmp_str)) == -1){
          plog(L_DB_INFO, NULL, NULL, "Can't get local hostname");
          return(PRARCH_NO_HOSTNAME);
        }

        curr_link -> host = stcopy(tmp_str);
#else
        curr_link -> host = stcopy(hostname);
#endif	       

        sprintf(tmp_str,"ARCHIE/MATCH(%d,0,=)/%s",search_req -> max_uniq_hits, curr_link -> name);
        curr_link -> filename = stcopy(tmp_str);

        vl_insert(curr_link, vdir, VLI_NOSORT);

        continue;
	    }
	    else{
        plog(L_DB_INFO, NULL, NULL, "Can't allocate link!",0);
        return(PRARCH_OUT_OF_MEMORY);
	    }

	    break;

    default:
	    break;
    }

    site_entry = s_index -> index;

    new_addr = site_entry.site_addr;

    for(linkcount = 0;
        (site_entry.site_addr != END_OF_CHAIN) && (linkcount != search_req -> max_uniq_hits);
        linkcount++){

      if(old_addr != new_addr) {
	    
        chostname = (char *) NULL;
        this_host = 1;

        if(site_file -> ptr != (char *) NULL) {

          munmap_file(site_file);

          close_file(site_file);

        }
		  
        strcpy(site_file -> filename, files_db_filename( inet_ntoa( ipaddr_to_inet(site_entry.site_addr))));

        if(open_file(site_file, O_RDONLY) != A_OK) {
          plog(L_DB_INFO, NULL, NULL, "Can't open site file %s",inet_ntoa( ipaddr_to_inet(site_entry.site_addr)),0); 
          break;
        }
		  

        if(mmap_file( site_file, O_RDONLY) != A_OK) {
          plog(L_DB_INFO, NULL, NULL, "Can't mmap site file %s", inet_ntoa( ipaddr_to_inet(site_entry.site_addr)),0); 
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

      if((search_req -> curr_type == S_EXACT) && (search_req -> orig_offset != 0))
	    if(linkcount != search_req -> orig_offset)
      goto next_one;
	       

      if(search_req -> domains){
        if(!this_host)
        goto next_one;
        else{

          if(!chostname)
          chostname = get_preferred_name(site_entry.site_addr, primary_hostname, hostdb, hostbyaddr);

          if(find_in_domains(chostname, domain_list, domain_count) == 0){
            this_host = 0;
            goto next_one;
          }
        }
      }

      site_ptr = (full_site_entry_t *) (site_file -> ptr) + site_entry.recno;

      path_name = dyna_find_ancestors((full_site_entry_t *) site_file -> ptr, site_ptr, strings);

      if(search_req -> comp_restrict) {

        if((path_name[0] == '\0') || (check_comp_restrict(restricts, path_name) != A_OK)){
          free_opts(path_name);
          goto next_one;

        }
      }

      if(!chostname)
	    chostname = get_preferred_name(site_entry.site_addr, primary_hostname, hostdb, hostbyaddr);

      /* Now allocate and set up the link */


      curr_link = (VLINK) vlalloc();

      if(!curr_link){
        plog(L_DB_INFO, NULL, NULL, "Can't allocate link");
        return(PRARCH_OUT_OF_MEMORY);
      }

      if( CSE_IS_DIR(site_ptr -> core )){

        /*
         * It's a directory - we should check to see if the site is
         * running prospero, and if so return a pointer to the actual
         * directory.  If it isn't then we return a real pointer to a
         * pseudo-directory maintained by this archie server.
         */

        curr_link ->type = stcopy("DIRECTORY");
      }
      else {

        /*
         * It's a file - we should check to see if the site is running
         * prospero, and if so return a pointer to the real file.  If it
         * isn't, then we generate an external link
         */

        curr_link -> type = stcopy("EXTERNAL(AFTP,BINARY)");
        UNSET_ARCHIE_DIR(search_req -> attrib_list);

      }
	      
      if(GET_ARCHIE_DIR(search_req -> attrib_list))
	    curr_link -> host = stcopy(hostwport);
      else
	    curr_link -> host = stcopy(chostname); 

      /* Get the the last component of name */
	      
      curr_link -> name = stcopy((strings -> ptr) + (site_ptr -> str_ind));

      /* Set up the full path name */

      tmp_str[0] = '\0';
	 
      for(tmp_ptr = path_name; *tmp_ptr != (char *) NULL; tmp_ptr++);

      for(tmp_ptr2 = tmp_str, --tmp_ptr; tmp_ptr >= path_name; tmp_ptr--){
        sprintf(tmp_ptr2,"/%s", *tmp_ptr);
        tmp_ptr2 += strlen(*tmp_ptr) + 1;
      }

      /*	 if(*tmp_ptr != (char *) NULL)
           sprintf(tmp_ptr2,"%s", *tmp_ptr);	 */

      if(GET_ARCHIE_DIR(search_req -> attrib_list)) {

        if(tmp_str[0] == '\0')
        sprintf(fullpath,"ARCHIE/HOST/%s/%s%s", chostname, tmp_str, curr_link -> name);
        else
        sprintf(fullpath,"ARCHIE/HOST/%s%s/%s", chostname, tmp_str, curr_link -> name);
      }
      else {
        sprintf(fullpath,"%s/%s",tmp_str, curr_link -> name);
      } 

      curr_link -> filename = stcopy(fullpath);

      /* Subcomponents look like pointers into mmapped stringsdb */

      if(path_name)
      free(path_name);

      if(hostdb_rec.primary_hostname[0] == '\0'){

        if(get_dbm_entry(primary_hostname, strlen(primary_hostname) + 1, &hostdb_rec, hostdb) == ERROR)
        continue;
      }

      if(hostaux_rec.access_methods[0] == '\0'){

        sprintf(tmp_str,"%s.%s", primary_hostname, ANONFTP_DB_NAME);

        if(get_dbm_entry(tmp_str, strlen(tmp_str) + 1, &hostaux_rec, hostaux_db) == ERROR)
        continue;

        if(hostaux_rec.current_status != ACTIVE)
        continue;
      }


      /* Do each attribute in turn */

      if(GET_AR_H_IP_ADDR(search_req -> attrib_list)){

        add_attribute(curr_link,NAME_AR_H_IP_ADDR,"ASCII", inet_ntoa(ipaddr_to_inet(new_addr)));
		  
      }

      if(GET_AR_H_OS_TYPE(search_req -> attrib_list)){
        char *ost;

        switch(hostdb_rec.os_type){


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

        add_attribute(curr_link, NAME_AR_H_OS_TYPE, "ASCII",ost);
      }
		 
      if(GET_AR_H_TIMEZ(search_req -> attrib_list)){

        sprintf(tmp_str,"%+d", hostdb_rec.timezone);

        add_attribute(curr_link, NAME_AR_H_TIMEZ, "ASCII", tmp_str);

      }


      if(GET_AUTHORITY(search_req -> attrib_list)){

        add_attribute(curr_link, NAME_AUTHORITY, "ASCII", hostaux_rec.source_archie_hostname);
      }

      if(GET_LK_LAST_MOD(search_req -> attrib_list)){
		     
        sprintf(tmp_str,"%sZ", cvt_from_inttime(site_ptr -> core.date));

        add_attribute(curr_link, NAME_LK_LAST_MOD, "ASCII", tmp_str);
      }

      if(GET_AR_H_LAST_MOD(search_req -> attrib_list)){

        sprintf(tmp_str,"%sZ", cvt_from_inttime(hostaux_rec.update_time));

        add_attribute(curr_link, NAME_AR_H_LAST_MOD, "ASCII", tmp_str);
      }
		     
      if(GET_AR_RECNO(search_req -> attrib_list)){

        sprintf(tmp_str,"%d",hostaux_rec.no_recs);

        add_attribute(curr_link, NAME_AR_RECNO, "ASCII", tmp_str);
      }

      if(GET_LINK_COUNT(search_req -> attrib_list)){
        full_site_entry_t *sptr;
        int no_children = 0;

        if(CSE_IS_DIR(site_ptr -> core) && (site_ptr -> core.child_idx != -1)){

          /* no children */

          no_children++;

          for(sptr = (full_site_entry_t *) site_file -> ptr + site_ptr -> core.child_idx;
              ((char *) sptr + sizeof(full_site_entry_t) < site_file -> ptr + site_file -> size) 
              && (sptr -> core.parent_idx == (sptr + 1) -> core.parent_idx);
              sptr++){

            no_children++;
          }
        }
	       
        sprintf(tmp_str,"%d", no_children);

        add_attribute(curr_link, NAME_LINK_COUNT, "ASCII", tmp_str);
      }

      if(GET_LINK_SIZE(search_req -> attrib_list)){

        sprintf(tmp_str,"%d", site_ptr -> core.size);

        add_attribute(curr_link, NAME_LINK_SIZE, "ASCII", tmp_str);
      }

      if(GET_NATIVE_MODES(search_req -> attrib_list)){

        sprintf(tmp_str,"%d", site_ptr -> core.perms);

        add_attribute(curr_link, NAME_NATIVE_MODES, "ASCII", tmp_str);
      }

      if(GET_LK_UNIX_MODES(search_req -> attrib_list)){

        sprintf(tmp_str,"%s", unix_perms_itoa(site_ptr -> core.perms, CSE_IS_DIR(site_ptr -> core)));

        add_attribute(curr_link, NAME_LK_UNIX_MODES, "ASCII", tmp_str);
      }


      vl_insert(curr_link, vdir, VLI_NOSORT);

    next_one:

      site_entry = ((full_site_entry_t *) site_file -> ptr  + site_entry.recno) -> next;

      new_addr = site_entry.site_addr;

    }
	 
  } /* for */

  /* Set curr_offset to the number of the last link */

  if(search_req -> curr_type == S_EXACT)
  search_req -> curr_offset = linkcount;


  add_to_cache(vdir -> links, search_req);

  free(index_list);


  if(site_file -> ptr != (char *) NULL) {

    munmap_file(site_file);

    close_file(site_file);
  }

  if(munmap_file(strings_idx) != A_OK) {
    return(PRARCH_BAD_MMAP);
  }
      
  if(munmap_file(strings) != A_OK) {
    return(PRARCH_BAD_MMAP);
  }

  if(restricts)
  free_opts(restricts);

  return(PSUCCESS);
}


char **dyna_find_ancestors( site_begin, site_entry, strings)
  void *site_begin;
  full_site_entry_t *site_entry;
  file_info_t *strings;

{
  char	**pathname;
  int	curr_path_comps;
  int	max_path_comps = DEF_MAX_PCOMPS;
  full_site_entry_t *my_sentry;
   
  pathname = (char **) malloc(max_path_comps * sizeof(char *));

  memset(pathname, '\0', max_path_comps * sizeof(char *));

  if(site_entry -> core.parent_idx == -1){
    pathname[0] = (char *) NULL;
    return(pathname);
  }

  for(curr_path_comps = 0, my_sentry = (full_site_entry_t *) site_begin + site_entry -> core.parent_idx;
      my_sentry -> core.parent_idx != -1;curr_path_comps++){

    pathname[curr_path_comps] = strings -> ptr + my_sentry -> str_ind;

    if(curr_path_comps == max_path_comps){
      char **newpath;

      max_path_comps += DEF_INCR_PCOMPS;

      newpath = (char **) realloc(pathname, max_path_comps * sizeof(char *));

      if(!newpath){
        pathname[0] = (char *) NULL;
        return(pathname);
      }
    }

    my_sentry = (full_site_entry_t *) site_begin + my_sentry -> core.parent_idx;

  }

  if(curr_path_comps == max_path_comps){
    char **newpath;

    max_path_comps += DEF_INCR_PCOMPS;

    newpath = (char **) realloc(pathname, max_path_comps * sizeof(char *));

    if(!newpath){
      pathname[0] = (char *) NULL;
      return(pathname);
    }
  }

  pathname[curr_path_comps] = strings -> ptr + my_sentry -> str_ind;

  pathname[++curr_path_comps] = (char *) NULL;

  return(pathname);
}
   

status_t check_comp_restrict( restrict, pathname)
  char **restrict;
  char **pathname;
{
  char **chptr;
  char **mypath;
  int found = FALSE;

  for(chptr = restrict; (*chptr != (char *) NULL) && !found; chptr++){

    for(mypath = pathname;(*mypath != (char *) NULL) && !found; mypath++){

      if(strstr(*pathname, *chptr))
      found = TRUE;
    }
  }

  return( found ? A_OK : ERROR);
}



  /*
   * Find the site file associated with given site name
   */

int find_sitefile_in_db(site_name, sitefile, hostdb)
  hostname_t  site_name;        /* site to be found in database		*/
  file_info_t *sitefile;        /* file info structure to be filled in	*/
  file_info_t *hostdb;

{

  AR_DNS *host_entry;
  ip_addr_t **addr;
  int i;
  int finished = 0;

  /* Get ip address of given site name, checking the local hostdb first */
   
  if((host_entry =  ar_open_dns_name(site_name, DNS_LOCAL_FIRST, hostdb)) == (struct hostent *) NULL )
  return(PRARCH_SITE_NOT_FOUND);

  for( addr = (ip_addr_t **) host_entry -> h_addr_list, i = 0;
      addr[i] != (ip_addr_t *) NULL && !finished;
      i++){

    if( access( files_db_filename( inet_ntoa ( ipaddr_to_inet(*addr[i]))), F_OK) != -1)
    finished = 1;
    else{
      error(A_SYSERR, "find_sitefile_in_db", "Error accessing site file %s", files_db_filename( inet_ntoa ( ipaddr_to_inet(*addr[i]))));
      return(PRARCH_CANT_OPEN_FILE);
    }
  }

  if( addr[i] == (ip_addr_t *) NULL )
  return(PRARCH_SITE_NOT_FOUND);

  strcpy( sitefile -> filename, files_db_filename( inet_ntoa ( ipaddr_to_inet(*addr[i]))));

  ar_dns_close(host_entry);

  return(A_OK);
}



  /* Check for cached results */
int check_cache(search_req, linkpp)
  search_req_t  *search_req;
  VLINK	  *linkpp;
{    
  char   *arg;
  int    offset;
  search_sel_t qtype;
  struct match_cache 	*cachep = mcache;
  struct match_cache 	*pcachep = NULL;
  VLINK			cur_link;
  VLINK			rest = NULL;
  VLINK			next = NULL;
  int			max_hits = search_req -> max_filename_hits;
  int			max_uniq_hits = search_req -> max_uniq_hits;
  int			count =  max_hits * max_uniq_hits;
  int			archiedir = GET_ARCHIE_DIR(search_req -> attrib_list);


  arg = search_req -> search_str;
  qtype = search_req -> curr_type;
  offset = search_req -> orig_offset;

  while(cachep) {
    if(((qtype == cachep->search_type)||(qtype == cachep->req_type))&&
       (cachep->offset == offset) &&
       /* All results are in cache - or enough to satisfy request */
       ((cachep->max_hits < 0) || (max_hits <= cachep->max_hits)) &&
       (strcmp(cachep->arg,arg) == 0) &&
       (cachep->archiedir == archiedir)) {
      /* We have a match.  Move to front of list */
      if(pcachep) {
        pcachep->next = cachep->next;
        cachep->next = mcache;
        mcache = cachep;
      }

      /* We now have to clear the expanded bits or the links  */
      /* returned in previous queries will not be returned    */
      /* We also need to truncate the list of there are more  */
      /* matches than requested                               */
      cur_link = cachep->matches;
#ifdef NOTDEF                   /* OLD code that is not maintained, it is unnecessary    */
      /* but it does show how we used to handle two-dimensions */
      while(cur_link) {
        tmp_link = cur_link;
        while(tmp_link) {
          tmp_link->expanded = FALSE;
          if(tmp_link == cur_link) tmp_link = tmp_link->replicas;
          else tmp_link = tmp_link->next;
        }
        cur_link = cur_link->next;
      }
#else  /* One dimensional */
      /* IMPORTANT: This code assumes the list is one         */
      /* dimensional, which is the case because we called     */
      /* vl_insert with the VLI_NOSORT option                 */
      while(cur_link) {
        cur_link->expanded = FALSE;
        next = cur_link->next;
        if(--count == 0) {      /* truncate list */
          rest = cur_link->next;
          cur_link->next = NULL;
          if(rest) rest->previous = NULL;
        }
        else if((next == NULL) && (rest == NULL)) {
          next = cachep->more;
          cur_link->next = next;
          if(next) next->previous = cur_link;
          cachep->more = NULL;
        }
        else if (next == NULL) {
          cur_link->next = cachep->more;
          if(cur_link->next)
          cur_link->next->previous = cur_link;
          cachep->more = rest;
        }
        cur_link = next;
      }
#endif
      *linkpp = cachep->matches;
      return(TRUE);
    }
    pcachep = cachep;
    cachep = cachep->next;
  }
  *linkpp = NULL;
  return(FALSE);
}

	
  /* Cache the response for later use */
  int add_to_cache(vl, search_req)
  VLINK	vl;
  search_req_t *search_req;
  {
    struct match_cache 	*newresults = NULL;
    struct match_cache 	*pcachep = NULL;

    if(cachecount < MATCH_CACHE_SIZE) { /* Create a new entry */
      newresults = (struct match_cache *) malloc(sizeof(struct match_cache));
      cachecount++;
      newresults->next = mcache;
      mcache = newresults;
      newresults->arg = stcopy(search_req -> search_str);
      newresults->max_hits = search_req -> max_filename_hits;
      newresults->offset = search_req -> orig_offset;
      newresults->search_type = search_req -> orig_type;
      newresults->req_type = search_req -> curr_type;
      newresults->archiedir = GET_ARCHIE_DIR(search_req -> attrib_list);
      newresults->matches = NULL;
      newresults->more = NULL;
    }
    else {                      /* Use last entry - Assumes list has at least two entries */
      pcachep = mcache;
      while(pcachep->next) pcachep = pcachep->next;
      newresults = pcachep;

      /* move to front of list */
      newresults->next = mcache;
      mcache = newresults;

      /* Fix the last entry so we don't have a cycle */
      while(pcachep->next != newresults) pcachep = pcachep->next;
      pcachep->next = NULL;

      /* Free the old results */
      if(newresults->matches) {
	      newresults->matches->dontfree = FALSE;
	      vllfree(newresults->matches);
	      newresults->matches = NULL;
      }
      if(newresults->more) {
	      newresults->more->dontfree = FALSE;
	      vllfree(newresults->more);
	      newresults->more = NULL;
      }

      newresults->arg = stcopyr(search_req -> search_str,newresults->arg);
      newresults->max_hits = search_req -> max_filename_hits;
      newresults->offset = search_req -> orig_offset;
      newresults->search_type = search_req -> orig_type;
      newresults->req_type = search_req -> curr_type;
      newresults->archiedir = GET_ARCHIE_DIR(search_req -> attrib_list);
    }

    /* Since we are caching the data.  If there are any links, */
    /* note that they should not be freed when sent back       */
    if(vl) vl->dontfree = TRUE;
    
    newresults->matches = vl;
    return(0);
  }
      

