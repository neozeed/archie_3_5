#define _BSD_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <memory.h>
#include <malloc.h>
#ifndef SOLARIS
#include <strings.h>
#else
#include <string.h>
#endif
#include <sys/mman.h>
#include "typedef.h"
#include "defines.h"
#include "pfs.h"
#include "plog.h"
#include "perrno.h"
/*
 *  #ifdef OLD_FILES # include "old-site_file.h" #else # include "site_file.h"
 *  #endif
 */
# include "site_file.h"
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
#include "parchie_search_files_db.h"
#include "parchie_host_dir.h"
#include "patrie.h"
#include "archstridx.h"


/* to prevent conflicts with the macro definition of "filename"
   pfs.h */

#undef filename   

int parchie_host_dir(site_name, attrib_list, dirname, ob, hostdb_finfo, hostaux_db, strings)
  hostname_t site_name; 	   /* Name of host to be searched            */
  attrib_list_t attrib_list;
  char *dirname;	           /* Name of directory to be listed         */
  P_OBJECT ob;		           /* Directory to be filled in              */
  file_info_t *hostdb_finfo; /* File info for host database		         */
  file_info_t *hostaux_db;
  file_info_t *strings;      /* File info for strings database	       */
{
  extern char hostwport[];

#ifdef __STC__
  extern char *re_comp(char *);
  extern int re_exec(char *);
#else
  extern char *re_comp();
  extern int re_exec();
#endif
  VLINK curr_link;
  char **dirptr = (char **)NULL;
  char **orig_ptr = (char **)NULL;
  char **path_name;
  char **tmp_ptr;
  char *tmp_ptr2;
  char fullpath[4096];
  char tmp_str[4096];
  file_info_t *sitefile = create_finfo();
  full_site_entry_t *site_end;
  full_site_entry_t *site_rec_ptr;
  hostdb_t hostdb_rec;
  index_t parent_i;
  int retcode = PRARCH_SUCCESS;
  int unfinished;
  pathname_t basepath;
  pathname_t files_database_dir;
  struct arch_stridx_handle *strhan;
  int site_limit=0;
  int curr_count=0, parent_count=0;
  int first=1;
  unsigned long hits=0;
  char *strres, *dbdir;
  full_site_entry_t *prnt, *child, *parent_ptr;
  index_t start[1];

  ptr_check(site_name, char, "parchie_host_dir", PRARCH_BAD_ARG);
  ptr_check(hostdb_finfo, file_info_t, "parchie_host_dir", PRARCH_BAD_ARG);
/*   ptr_check(strings, file_info_t, "parchie_host_dir", PRARCH_BAD_ARG); */
  files_database_dir[0] = '\0';
   
  plog(L_DB_INFO, NOREQ, "Just started parchie_host_dir() !");

  memset(&hostdb_rec, '\0', sizeof(hostdb_t));
  if (dirname != (char *)NULL)
  {
    if (dirname[0] != '/')
    {
      strcpy(basepath, dirname);
    }
    else
    {
      strcpy(basepath, dirname + 1);
    }
  }

  /* "SITE" command */

  /* Get filename of host wanted */

  if (get_dbm_entry(site_name, strlen(site_name) + 1, &hostdb_rec, hostdb_finfo) == ERROR)
  {
    AR_DNS *dns;
    hostname_t tmphost;

    if ((dns = ar_open_dns_name(site_name, DNS_LOCAL_FIRST, hostdb_finfo)) == (AR_DNS *)NULL)
    {
      return PRARCH_SITE_NOT_FOUND;
    }

    strcpy(tmphost, get_dns_primary_name(dns));
    if (get_dbm_entry(tmphost, strlen(tmphost) + 1, &hostdb_rec, hostdb_finfo) == ERROR)
    {
      p_err_string = qsprintf_stcopyr(p_err_string, "Site not found: %s", site_name);
      return PRARCH_SITE_NOT_FOUND;
    }

    ar_dns_close(dns);
  }

  if((dbdir = (char *)set_files_db_dir(files_database_dir)) == (char *) NULL){
    /* "Error while trying to set anonftp database directory" */
    plog(L_DB_INFO, NOREQ,"Error while trying to set anonftp database directory.");
    p_err_string = qsprintf_stcopyr(p_err_string, "Error while trying to set anonftp database directory.");
    retcode = PRARCH_BAD_MMAP;
    goto atend;
  }

  if ( !(strhan = archNewStrIdx()) ) {
    plog(L_DB_INFO, NOREQ, "Can't setup strings handle before opening strings files." );
    p_err_string = qsprintf_stcopyr(p_err_string, "Can't setup strings handle before opening strings files.");
    retcode = PRARCH_BAD_MMAP;
    goto atend;
  }
  
  if ( !archOpenStrIdx( strhan, dbdir, ARCH_STRIDX_SEARCH ) ){
    plog(L_DB_INFO, NOREQ, "Can't open strings files.");
    p_err_string = qsprintf_stcopyr(p_err_string, "Can't open strings files.");
    retcode = PRARCH_BAD_MMAP;
    goto atend;
  }


/*
  if (mmap_file(strings, O_RDONLY) == ERROR)
  {
    plog(L_DB_INFO, NOREQ, "Can't mmap strings file %s", strings->filename);
    p_err_string = qsprintf_stcopyr(p_err_string, "Server out of memory");
    retcode = PRARCH_BAD_MMAP;
    goto atend;
  }
*/
/*  MUST CLEAN the port number.
   */ 
  strcpy(sitefile->filename,
         files_db_filename(inet_ntoa(ipaddr_to_inet(hostdb_rec.primary_ipaddr)), 21));
  if (open_file(sitefile, O_RDONLY) == ERROR)
  {
    plog(L_DB_INFO, NOREQ, "Can't open site file %s", sitefile->filename);
    p_err_string = qsprintf_stcopyr(p_err_string, "Can't open site file %s", sitefile->filename);
    retcode = PRARCH_SITE_NOT_FOUND;
    goto atend;
  }

  if (mmap_file( sitefile, O_RDONLY) != A_OK)
  {
    plog(L_DB_INFO, NOREQ, "Can't mmap site file %s", sitefile->filename);
    p_err_string = qsprintf_stcopyr(p_err_string, "Can't mmap() site file %s", sitefile->filename);
    retcode = PRARCH_BAD_MMAP;
    goto atend;
  }

  site_rec_ptr = (full_site_entry_t *)sitefile->ptr;

  /* Calculate address of the end of the site file */

  site_end = site_rec_ptr + (sitefile->size / sizeof(full_site_entry_t));

  site_limit = sitefile->size / sizeof(full_site_entry_t);

  /* Go through path one component at a time finding the record in the
     site file corresponding to that name. Will end up with correct
     record or it won't be found */

  /* While there are still components in the directory name */

/*
       */
  for (orig_ptr = dirptr = str_sep(basepath, '/');
       (*dirptr != (char *) NULL) && (*dirptr[0] != '\0');
       dirptr++, curr_count++)
  {
    /* must exclude in my case the site-name and port */
    if( !archSearchExact( strhan, *dirptr, 1, 1, &hits, start)){
      /*       error(A_ERR, "archQuery","Could not perform exact search successfully.");*/
      retcode = PRARCH_DIR_NOT_FOUND;
      goto atend;
    }else if(hits==0){          /* I was afraid of side effects this is why it was not ORed with the above condition */
      retcode = PRARCH_DIR_NOT_FOUND;
      goto atend;
    }

    for( unfinished=1 ; site_rec_ptr < site_end; site_rec_ptr++, curr_count++ )  {
      if( CSE_IS_SUBPATH((*site_rec_ptr)) )
      {
        if( site_rec_ptr->core.prnt_entry.strt_2 <= site_limit  && site_rec_ptr->core.prnt_entry.strt_2 > 0 ){
          prnt = (full_site_entry_t *) sitefile -> ptr + site_rec_ptr->core.prnt_entry.strt_2;
          if( prnt->strt_1 == start[0] ){
            if( first==1 )
            {                   /* if this is the first directory to match my string then I don't check if the parent directory matches. */
              first = 0;
            }
            else if( parent_count != site_rec_ptr->strt_1 ) continue;
            parent_count = curr_count;
            unfinished = 0;
            break;
          }
        }
      }
    }
    /* Directory in path not found */

    if (unfinished != 0)
    {
      retcode = PRARCH_DIR_NOT_FOUND;
      p_err_string = qsprintf_stcopyr(p_err_string, "Directory not found: %s", dirname);
      goto atend;
    }

    /* Given pathname component is a directory ? */

    if ( ! CSE_IS_SUBPATH((*site_rec_ptr)))
    {
      retcode = PRARCH_NOT_DIRECTORY;
      p_err_string = qsprintf_stcopyr(p_err_string, "Not a directory: %s", dirname);
      goto atend;
    }
    /*
       if (site_rec_ptr->core.child_idx == 0) goto atend;
       */
    child = site_rec_ptr + 1;
    if( !(CSE_IS_FILE((*child)) || CSE_IS_DIR((*child)) || CSE_IS_DOC((*child)) || CSE_IS_LINK((*child)) ) ){
      goto atend;
    }
    /* This directory has no children */

    /* set site file pointer to desired child of current directory */

    parent_ptr = site_rec_ptr;
    site_rec_ptr = child;
  }
  

  /* site_rec_ptr points to first child of desired directory
     go through all children constructing VLINK of files */

  if(site_rec_ptr >= site_end){
    retcode = PRARCH_DIR_NOT_FOUND;
    p_err_string = qsprintf_stcopyr(p_err_string, "Cannot expand directory: %s", dirname);
    goto atend;
  }

  /*  for (parent_i = site_rec_ptr->core.parent_idx;
       (site_rec_ptr < site_end) &&
       (site_rec_ptr->core.parent_idx == parent_i);
       site_rec_ptr++)
       */
  for(; site_rec_ptr < site_end && (!(CSE_IS_SUBPATH((*site_rec_ptr)) || CSE_IS_NAME((*site_rec_ptr)) || CSE_IS_PORT((*site_rec_ptr))));  site_rec_ptr++)
  {
    /* Fill in  VLINK structure */

    if ( ! (curr_link = (VLINK)vlalloc()))
    {
      plog(L_DB_INFO, NOREQ, "Can't allocate link");
      p_err_string = qsprintf_stcopyr(p_err_string, "Server out of memory");
      retcode = PRARCH_OUT_OF_MEMORY;
      goto atend;
    }

    /* Find the last component of name */
    if( !archGetString( strhan,  site_rec_ptr->strt_1, &strres) ){
      /*       error(A_ERR, "archQuery","Could not perform exact search successfully.");*/
      retcode = PRARCH_DIR_NOT_FOUND;
    }else if( strres == (char *)NULL ) continue; 
    /* the string is not found or lost or something..! */
    /*    curr_link->name = stcopyr((strings->ptr) + (site_rec_ptr->str_ind), curr_link->name); */

    curr_link->name = stcopyr(strres, curr_link->name);
    /* Find the full path name */
    path_name = new_find_ancestors((full_site_entry_t *)sitefile->ptr, parent_ptr, strhan);
    
    free(strres);
    
    tmp_str[0] = '\0';
    for (tmp_ptr = path_name; *tmp_ptr != (char *)NULL; tmp_ptr++)
    { continue; }

    for (tmp_ptr2 = tmp_str, --tmp_ptr; tmp_ptr >= path_name; tmp_ptr--)
    {
      sprintf(tmp_ptr2, "/%s", *tmp_ptr);
      tmp_ptr2 += strlen(*tmp_ptr) + 1;
    }

    sprintf(fullpath, "%s/%s", tmp_str, curr_link->name);
    if (CSE_IS_SUBPATH((*site_rec_ptr)))
    {
      /*
       * It's a directory - we should check to see if the site is
       * running prospero, and if so return a pointer to the actual
       * directory.  If it isn't then we return a real pointer to a
       * pseudo-directory maintained by this archie server.
       */

      curr_link->target = stcopyr("DIRECTORY", curr_link->target);
      if (1                     /* host not running prospero */)
      {
        curr_link->host = stcopyr(hostwport, curr_link->host);
        if (*fullpath == '/')
        {
          sprintf(tmp_str, "ARCHIE/HOST/%s%s", site_name, fullpath);
        }
        else
        {
          sprintf(tmp_str, "ARCHIE/HOST/%s/%s", site_name, fullpath);
        }
        curr_link->hsoname = stcopyr(tmp_str, curr_link->hsoname);
      }
      else
      {
        curr_link->host = stcopyr(site_name, curr_link->host); 
        if (*fullpath == '/')
        {
          sprintf(tmp_str, "AFTP%s", fullpath);
        }
        else
        {
          sprintf(tmp_str, "ARCHIE/%s", fullpath);
        }
        curr_link->hsoname = stcopyr(fullpath, curr_link->hsoname);
      }
    }
    else
    {
      /*
       * It's a file - we should check to see if the site is running
       * prospero, and if so return a pointer to the real file.  If it
       * isn't, then we generate an external link
       */

      curr_link->target = stcopyr("EXTERNAL", curr_link->target);
      /* defined in "prealpha.5.3.10Mar94/lib/psrv/ad2l_atr.c" but not declared - wheelan */
      ad2l_am_atr(curr_link, "AFTP", "BINARY", NULL);
      curr_link->host = stcopyr(site_name, curr_link->host); 
      curr_link->hsoname = stcopyr(fullpath, curr_link->hsoname);
    }
       
    if (GET_LK_LAST_MOD(attrib_list))
    {
      sprintf(tmp_str, "%sZ", cvt_from_inttime(site_rec_ptr->core.entry.date));
      /* defined in "prealpha.5.3.10Mar94/lib/psrv/ad2l_atr.c" but not declared - wheelan */
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_INTRINSIC,
                   NAME_LK_LAST_MOD, tmp_str, NULL);
    }

    if (GET_LINK_SIZE(attrib_list))
    {
      sprintf(tmp_str, "%ld bytes", site_rec_ptr->core.entry.size);
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_INTRINSIC,
                   NAME_LINK_SIZE, tmp_str, NULL);
    }
	   
    if (GET_NATIVE_MODES(attrib_list))
    {
      sprintf(tmp_str, "%d", site_rec_ptr->core.entry.perms);
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_INTRINSIC,
                   NAME_NATIVE_MODES, tmp_str, NULL);
    }

    if (GET_LK_UNIX_MODES(attrib_list))
    {
      sprintf(tmp_str, "%s", unix_perms_itoa(site_rec_ptr->core.entry.perms,
                                             CSE_IS_DIR((*site_rec_ptr)), 0));
      ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_INTRINSIC,
                   NAME_LK_UNIX_MODES, tmp_str, NULL);
    }
	 
    /* Insert link in list */
	   
    APPEND_ITEM(curr_link, ob->links);

    /*
     *  This assures me that what ever got allocated from 
     *  archGetString() and put in path_name[] will be freed here
     */
  
    if( path_name ){
      for (tmp_ptr = path_name; *tmp_ptr != (char *)NULL; tmp_ptr++)
      { free(*tmp_ptr); }
    }

  } /* for */

 atend:

  archCloseStrIdx( strhan);
  archFreeStrIdx(&strhan);
/*  if( strhan )  archCloseStrIdx( &strhan); */
  if(orig_ptr) free_opts(orig_ptr);
  close_file(sitefile);
  destroy_finfo(sitefile);
  

/*   munmap_file(strings); */
  return retcode;
}

