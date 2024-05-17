#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <memory.h>
#include <malloc.h>
#include <strings.h>
#include "typedef.h"
#include "defines.h"
#include "plog.h"
#ifdef OLD_FILES
#  include "old-site_file.h"
#else
#  include "site_file.h"
#endif
#include "ar_search.h"
#include "error.h"
#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif
#include "db_ops.h"
#include "files.h"
#include "archie_strings.h"
#include "archie_dbm.h"
#include "times.h"


int prarch_host_dir(site_name,dbname,attrib_list,dirname,vd,hostdb_finfo, hostaux_db, strings_finfo)
    hostname_t	site_name; 	/* Name of host to be searched            */
    char        *dbname;
    attrib_list_t attrib_list;
    char	*dirname;	/* Name of directory to be listed */
    VDIR	vd;		/* Directory to be filled in              */
    file_info_t *hostdb_finfo;	/* File info for host database		  */
    file_info_t *hostaux_db;
    file_info_t *strings_finfo; /* File info for strings database	  */

{
#ifdef __STC__

   extern char *re_comp(char *);
   extern int re_exec(char *);

#else

   extern char *re_comp();
   extern int re_exec();

#endif
   pathname_t basepath;
   full_site_entry_t *site_rec_ptr, *site_end;
   char **dirptr;
   index_t parent_i;
   VLINK curr_link;
   int unfinished;
   file_info_t *sitefile = create_finfo();
   pathname_t tmp_str;
   hostdb_t hostdb_rec;


   ptr_check(site_name, char, "prarch_host_dir", PRARCH_BAD_ARG);
   ptr_check(hostdb_finfo, file_info_t, "prarch_host_dir", PRARCH_BAD_ARG);
   ptr_check(strings_finfo, file_info_t, "prarch_host_dir", PRARCH_BAD_ARG);
   
   memset(&hostdb_rec, '\0', sizeof(hostdb_t));

   if(dirname != (char *) NULL){
      if(dirname[0] != '/')
	 strcpy(basepath, dirname);
      else
	 strcpy(basepath, dirname + 1);
   }

   /* check to see if the name contains an regex character */

   /* "LIST" command */

   if(strpbrk(site_name,"\\^$[]<>*+?|(){}/") != NULL){
      hostname_t *hostlist;
      int hostcount;
      hostname_t *hostptr, *host_end;
      int result;
      hostdb_aux_t hostaux_rec;

      if(get_hostnames(hostdb_finfo, &hostlist, &hostcount, (domain_t *) NULL, 0)== ERROR){

	 plog(L_DB_INFO, NULL, NULL, "Can't get list of hosts in database");
	 return(PRARCH_DB_ERROR);
      }

      if(re_comp(site_name) != (char *) NULL)
         return(PRARCH_BAD_REGEX);

      host_end = hostlist + hostcount;
	 
      for(hostptr = hostlist; hostptr < host_end; hostptr++){

	 if((result = re_exec(hostptr)) == -1)
	    return(PRARCH_INTERN_ERR);

	 if(result != 1)
	    continue;

	 if(get_dbm_entry(hostptr, strlen((char *) hostptr) + 1, &hostdb_rec, hostdb_finfo) == ERROR)
	    continue;

	 if(get_hostaux_ent(hostptr, (dbname == (char *) NULL ? ANONFTP_DB_NAME : dbname), &hostaux_rec, hostaux_db) == ERROR)
	    continue;

	 if(!(curr_link = (VLINK) vlalloc())){
	    plog(L_DB_INFO, NULL, NULL, "Can't allocate link for HOST command");
	    return(PRARCH_OUT_OF_MEMORY);
	 }

	 curr_link -> host = stcopy(hostptr);

	 curr_link -> name = stcopy("/");

	 /* Do each attribute in turn */

	 if(GET_AR_H_IP_ADDR(attrib_list)){

	    add_attribute(curr_link,NAME_AR_H_IP_ADDR,"ASCII", inet_ntoa(ipaddr_to_inet(hostdb_rec.primary_ipaddr)));
		  
	 }

	 if(GET_AR_DB_STAT(attrib_list)){
	    char *ost;

	    switch(hostaux_rec.current_status){

	       case ACTIVE:
		  ost = "Active";
		  break;

	       case NOT_SUPPORTED:
		  ost = "OS Not Supported";
		  break;

	       case DEL_BY_ADMIN:
		  ost = "Scheduled for deletion by Site Administrator";
		  break;

	       case DEL_BY_ARCHIE:
		  ost = "Scheduled for deletion";
		  break;

	       case INACTIVE:
		  ost = "Inactive";
		  break;

	       case DELETED:
		  ost = "Deleted from system";
		  break;

	       case DISABLED:
		  ost = "Disabled in system";
		  break;
		  
	       default:
		  ost = "Unknown";
		  break;
	    }

	    add_attribute(curr_link, NAME_AR_DB_STAT, "ASCII", ost);
	 }

	 if(GET_AR_H_OS_TYPE(attrib_list)){
	    char *ost;

	    switch(hostdb_rec.os_type){


	       case UNIX_BSD:
		  ost = "BSD Unix";
		  break;
			
	       case VMS_STD:
		  ost = "VMS";
		  break;

	       default:
		  ost = "Unknown";
		  break;
	    }

	    add_attribute(curr_link, NAME_AR_H_OS_TYPE, "ASCII",ost);
	 }
		 
	 if(GET_AR_H_TIMEZ(attrib_list)){

	    sprintf(tmp_str,"%+d", hostdb_rec.timezone);

	    add_attribute(curr_link, NAME_AR_H_TIMEZ, "ASCII", tmp_str);

	 }


	 if(GET_AUTHORITY(attrib_list)){

	    add_attribute(curr_link, NAME_AUTHORITY, "ASCII", hostaux_rec.source_archie_hostname);
	 }

	 if(GET_AR_H_LAST_MOD(attrib_list)){

	    sprintf(tmp_str,"%sZ", cvt_from_inttime(hostaux_rec.update_time));

	    add_attribute(curr_link, NAME_AR_H_LAST_MOD, "ASCII", tmp_str);
	 }
		     
	 if(GET_AR_RECNO(attrib_list)){

	    sprintf(tmp_str,"%d",hostaux_rec.no_recs);

	    add_attribute(curr_link, NAME_AR_RECNO, "ASCII", tmp_str);
	 }

	 vl_insert(curr_link, vd, VLI_NOSORT);

      }

      if(hostlist)
         free(hostlist);

      return(PRARCH_SUCCESS);
   }

   /* "SITE" command */

   /* Currently only the anonftp database is supported */

   if(dbname != (char *) NULL){
     if(strcasecmp(dbname, ANONFTP_DB_NAME) != 0)
        return(PRARCH_SUCCESS);
   }



   /* Get filename of host wanted */

   if(get_dbm_entry(site_name, strlen(site_name) + 1, &hostdb_rec, hostdb_finfo) == ERROR)
      return(PRARCH_SITE_NOT_FOUND);

   if(mmap_file(strings_finfo, O_RDONLY) == ERROR){
      plog(L_DB_INFO, NULL, NULL, "Can't mmap strings file");
      return(PRARCH_BAD_MMAP);
   }

   strcpy(sitefile -> filename, files_db_filename( inet_ntoa( ipaddr_to_inet(hostdb_rec.primary_ipaddr))));

   if(open_file(sitefile, O_RDONLY) == ERROR){
      plog(L_DB_INFO, NULL, NULL, "Can't open site file %s",inet_ntoa( ipaddr_to_inet(hostdb_rec.primary_ipaddr)),0);
      return(PRARCH_SITE_NOT_FOUND);
   }

   if(mmap_file( sitefile, O_RDONLY) != A_OK){
      plog(L_DB_INFO, NULL, NULL, "Can't mmap site file %s",inet_ntoa( ipaddr_to_inet(hostdb_rec.primary_ipaddr)),0);
      return(ERROR);
   }


   site_rec_ptr = (full_site_entry_t *) sitefile -> ptr;

   /* Calculate address of the end of the site file */

   site_end = site_rec_ptr + (sitefile -> size / sizeof(full_site_entry_t));


   /* Go through path one component at a time finding the record in the
      site file corresponding to that name. Will end up with correct
      record or it won't be found */

   /* While there are still components in the directory name */

   for(dirptr = str_sep(basepath, '/'); (*dirptr != (char *) NULL) && (*dirptr[0] != '\0'); dirptr++){

      for(parent_i = site_rec_ptr -> core.parent_idx, unfinished = 1;
         (site_rec_ptr -> core.parent_idx == parent_i) &&
	 ((unfinished = strcmp( (strings_finfo -> ptr) + (site_rec_ptr -> str_ind), *dirptr)) != 0) &&
	 site_rec_ptr < site_end;
	 site_rec_ptr++);

      /* Directory in path not found */

      if(unfinished == 1)
	 return(PRARCH_DIR_NOT_FOUND);

      /* Given pathname component is a directory ? */

      if(!CSE_IS_DIR( site_rec_ptr -> core) )
	 return(PRARCH_NOT_DIRECTORY);

      /* set site file pointer to desired child of current directory */

      site_rec_ptr = (site_rec_ptr -> core.child_idx) + (full_site_entry_t *) sitefile -> ptr;
       
   }


   /* site_rec_ptr points to first child of desired directory
         go through all children constructing VLINK of files */


   for(parent_i = site_rec_ptr -> core.parent_idx;
      (site_rec_ptr -> core.parent_idx == parent_i) &&
      site_rec_ptr < site_end;
      site_rec_ptr++){

      /* Fill in  VLINK structure */


/*      curr_link = atoplink( site_rec_ptr, archiedir, site_name, strings_finfo, dirname,0); */

      if(!(curr_link = (VLINK) vlalloc())){
	 plog(L_DB_INFO, NULL, NULL, "Can't allocate link");
	 return(PRARCH_OUT_OF_MEMORY);
      }


      if( CSE_IS_DIR( site_rec_ptr -> core )){

	 /* It's a directory - we should check to see if the site is   */
	 /* running prospero, and if so return a pointer to the actual */
	 /* directory.  If it isn't then we return a real pointer to   */
	 /* a pseudo-directory maintained by this archie server.       */

	 curr_link ->type = stcopy("DIRECTORY");
      }
      else {

	 /* It's a file - we should check to see if the site is        */
	 /* running prospero, and if so return a pointer to the real   */
	 /* file.  If it isn't, then we generate an external link      */

	 curr_link -> type = stcopy("EXTERNAL(AFTP,BINARY)");
      }


      curr_link -> name = stcopy((strings_finfo -> ptr) + (site_rec_ptr -> str_ind));

      curr_link -> host = stcopy(site_name);

     if((dirname != (char *) NULL) && (dirname[0] != '/')){
	sprintf(tmp_str,"/%s",dirname);
	curr_link -> filename = stcopy(tmp_str);
     }
     else
	curr_link -> filename = stcopy(dirname);

      if(GET_LK_LAST_MOD(attrib_list)){
		  
	 sprintf(tmp_str,"%sZ", cvt_from_inttime(site_rec_ptr -> core.date));

	 add_attribute(curr_link, NAME_LK_LAST_MOD, "ASCII", tmp_str);
      }


      if(GET_LINK_SIZE(attrib_list)){

	 sprintf(tmp_str,"%d", site_rec_ptr -> core.size);

	 add_attribute(curr_link, NAME_LINK_SIZE, "ASCII", tmp_str);
      }


      if(GET_NATIVE_MODES(attrib_list)){

	 sprintf(tmp_str,"%d", site_rec_ptr -> core.perms);

	 add_attribute(curr_link, NAME_NATIVE_MODES, "ASCII", tmp_str);
      }

         /* Insert link in list */

      vl_insert(curr_link,vd,VLI_NOSORT);

   }/* for */


   if( munmap_file( sitefile ) != A_OK )
      return(PRARCH_DB_ERROR);

   if( close_file( sitefile ) != A_OK)
      return(PRARCH_DB_ERROR);

  if(munmap_file(strings_finfo) != A_OK)
     return(PRARCH_BAD_MMAP);

    
   return(A_OK);
}
