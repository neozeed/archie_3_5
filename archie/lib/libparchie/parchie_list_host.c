#define _BSD_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <memory.h>
#ifndef SOLARIS
#include <strings.h>
#else
#include <string.h>
#include <regexpr.h>
#endif
#include <malloc.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "protos.h"
#include "typedef.h"
#include "defines.h"
#include "ardp.h"
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
#include "parchie_list_host.h"
#ifdef GOPHERINDEX_SUPPORT
#include "gindexdb_ops.h"
#endif

#define TOO_MANY_HOSTS 5000


static int parchie_add_hostent PROTO((P_OBJECT ob, attrib_list_t attrib_list, char *hostptr, char *dbname, hostdb_t	*hostdb_recp, hostdb_aux_t *hostaux_recp));
extern char hostwport[];


int parchie_list_host(site_name,dbname,attrib_list,ob,hostdb_finfo, hostaux_db, strings_finfo)
    hostname_t	site_name; 	/* Name of host to be searched            */
    char        *dbname;	/* name of archie database to be listed   */
    attrib_list_t attrib_list;	/* attribute list			  */
    P_OBJECT	ob;		/* Directory to be filled in              */
    file_info_t *hostdb_finfo;	/* File info for host database		  */
    file_info_t *hostaux_db;	/* auxiliarly host database		  */
    file_info_t *strings_finfo; /* File info for strings database	  */
{

#ifdef __STC__

#ifndef SOLARIS22
   extern char *re_comp(char *);
   extern int re_exec(char *);
#endif

#else

   extern char *re_comp();
   extern int re_exec();

#endif

   char		hosttemp[256];
   char		*htemp = site_name;
   char		*p = hosttemp;
#ifdef SOLARIS22
   char	        *re;
#endif   

   hostdb_t hostdb_rec;
   hostname_t *hostlist = (hostname_t *) NULL;
   hostname_t *hostptr, *host_end;
   hostdb_aux_t hostaux_rec;
   int hostcount;
   int found_count = 0;
   int result;
   int retcode = PRARCH_SUCCESS;


   ptr_check(site_name, char, "parchie_list_host", PRARCH_BAD_ARG);
   ptr_check(hostdb_finfo, file_info_t, "parchie_list_host", PRARCH_BAD_ARG);
/*   ptr_check(strings_finfo, file_info_t, "parchie_list_host", PRARCH_BAD_ARG); */
   
   memset(&hostdb_rec, '\0', sizeof(hostdb_t));

   /* "LIST" command */
   
   /* If regular expressions or wildcards */
   if((strchr(site_name,'(') || strchr(site_name,'?') ||
       strchr(site_name,'*'))) {

       if(get_hostnames(hostdb_finfo, &hostlist, &hostcount, (domain_t *) NULL, 0)== ERROR){
	   plog(L_DB_INFO, NOREQ, "Can't get list of hosts in database");
	   retcode = PRARCH_DB_ERROR;
	   goto atend;
       }
   
       if((*htemp == '(') && (*(htemp + strlen(htemp)-1) == ')')) {
	   strncpy(hosttemp,htemp+1,sizeof(hosttemp));
	   hosttemp[sizeof(hosttemp)-1] = '\0';
	   hosttemp[strlen(hosttemp)-1] = '\0';
       }
       else if(htemp) {
	   *(p++) = '^';
	   while(*htemp) {
	       if(*htemp == '*') {*(p++)='.'; *(p++) = *(htemp++);}
	       else if(*htemp == '?') {*(p++)='.';htemp++;}
	       else if(*htemp == '.') {*(p++)='\\';*(p++)='.';htemp++;}
	       else if(*htemp == '[') {*(p++)='\\';*(p++)='[';htemp++;}
	       else if(*htemp == '$') {*(p++)='\\';*(p++)='$';htemp++;}
	       else if(*htemp == '^') {*(p++)='\\';*(p++)='^';htemp++;}
	       else if(*htemp == '\\') {*(p++)='\\';*(p++)='\\';htemp++;}
	       else *(p++) = *(htemp++);
	   }
	   *(p++) = '$';
	   *(p++) = '\0';
       }


#ifndef SOLARIS22
       if(re_comp(hosttemp) != (char *) NULL){
       	   retcode = PRARCH_BAD_REGEX;
	   goto atend;
       }
#else
      if((re = compile(hosttemp, (char *) NULL, (char *) NULL)) == (char *) NULL){
       	   retcode = PRARCH_BAD_REGEX;
	   goto atend;
      }
#endif
       host_end = hostlist + hostcount;
       
       for(hostptr = hostlist; hostptr < host_end; hostptr++){

#ifndef SOLARIS22
	   if((result = re_exec(hostptr)) == -1){
	       retcode = PRARCH_INTERN_ERR;
	   }

	   if(result != 1)
	       continue;
#else
	   if((result = step(hostptr, re)) == 0)
	     continue;
#endif

	   if(++found_count > TOO_MANY_HOSTS) {
	       retcode = PRARCH_TOO_MANY;
	       goto atend;
	   }

	   if(get_dbm_entry(hostptr, strlen((char *) hostptr) + 1, &hostdb_rec, hostdb_finfo) == ERROR)
	       continue;

	   if(get_hostaux_entry((char *) hostptr, (dbname == (char *) NULL ? ANONFTP_DB_NAME : dbname), (index_t) 0, &hostaux_rec, hostaux_db) == ERROR)
	       continue;

	   if(hostaux_rec.update_time == 0)
	       continue;

	   result = parchie_add_hostent(ob,attrib_list, hostaux_rec.preferred_hostname[0] == '\0' ? hostdb_rec.primary_hostname : hostaux_rec.preferred_hostname, dbname,
					&hostdb_rec,&hostaux_rec);
	   if(result) {
	       if(hostlist) free(hostlist);
	       return(result);
	   }
       }

atend:
       if(hostlist)
          free(hostlist);

#ifdef SOLARIS22
       if(re)
	  free(re);
#endif

       return(retcode);


   }
   /* No regular expression or wildcards */
   else {

       /* Get filename of host wanted */

       if(get_dbm_entry(site_name, strlen(site_name) + 1, &hostdb_rec, hostdb_finfo) == ERROR) {
	 AR_DNS *dns;
	 hostname_t tmphost;

	 if((dns = ar_open_dns_name(site_name, DNS_EXTERN_ONLY, hostdb_finfo)) == (AR_DNS *) NULL)
	    return(PRARCH_SITE_NOT_FOUND);

	 strcpy(tmphost, get_dns_primary_name(dns));

	 if(get_dbm_entry(tmphost, strlen(tmphost) + 1, &hostdb_rec, hostdb_finfo) == ERROR)
	    return(PRARCH_SITE_NOT_FOUND);

	 ar_dns_close(dns);
       }

       return(parchie_add_hostent(ob,attrib_list,site_name, dbname,
				  &hostdb_rec,&hostaux_rec));
   }
}


static int parchie_add_hostent(P_OBJECT ob,	               /* Directory to receive entry */
                               attrib_list_t attrib_list,  /* attribute list	     */
                               char	*hostptr,              /* Name of the host           */
                               char *dbname,
                               hostdb_t	*hostdb_recp,      /* Pointer to host record     */
                               hostdb_aux_t *hostaux_recp) /* Auxilliary host record    */
{
  VLINK curr_link;
  pathname_t tmp_str;

  if ( ! (curr_link = (VLINK)vlalloc()))
  {
    plog(L_DB_INFO, NOREQ, "Can't allocate link for HOST command");
    return PRARCH_OUT_OF_MEMORY;
  }

  if (strcmp(dbname, ANONFTP_DB_NAME) == 0)
  {
    sprintf(tmp_str, "ARCHIE/HOST/%s", (char *)hostptr);
  }
  else
  {
    sprintf(tmp_str, "ARCHIE/HOST(%s)/%s", dbname, (char *)hostptr);
  }

  curr_link->target = stcopyr("DIRECTORY", curr_link->target);
  curr_link->name = stcopyr(hostptr, curr_link->name);
  curr_link->hsoname = stcopyr(tmp_str, curr_link->hsoname);
  curr_link->host = stcopyr(hostwport, curr_link->host);

  /* Do each attribute in turn */
    
  if (GET_AR_H_IP_ADDR(attrib_list))
  {
    ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION, NAME_AR_H_IP_ADDR,
                 inet_ntoa(ipaddr_to_inet(hostdb_recp->primary_ipaddr)), NULL);
  }

  if (GET_AR_DB_STAT(attrib_list))
  {
    char *ost;
	       
    switch (hostaux_recp->current_status)
    {
    case ACTIVE:        ost = "Active";                                       break;
    case NOT_SUPPORTED: ost = "OS Not Supported";                             break;
    case DEL_BY_ADMIN:  ost = "Scheduled for deletion by Site Administrator"; break;
    case DEL_BY_ARCHIE: ost = "Scheduled for deletion";                       break;
    case INACTIVE:      ost = "Inactive";                                     break;
    case DELETED:       ost = "Deleted from system";                          break;
    case DISABLED:      ost = "Disabled in system";                           break;
    default:            ost = "Unknown";                                      break;
    }

    ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                 NAME_AR_DB_STAT, ost, NULL);
  }

  if (GET_AR_H_OS_TYPE(attrib_list))
  {
    char *ost;

    switch (hostdb_recp->os_type)
    {
    case UNIX_BSD: ost = "BSD Unix"; break;
    case VMS_STD:  ost = "VMS";      break;
    default:       ost = "Unknown";  break;
    }

    ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                 NAME_AR_H_OS_TYPE, ost, NULL);
  }
		 
  if (GET_AR_H_TIMEZ(attrib_list))
  {
    sprintf(tmp_str, "%+ld", hostdb_recp->timezone);
    ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                 NAME_AR_H_TIMEZ, tmp_str, NULL);
  }

  if (GET_AUTHORITY(attrib_list))
  {
    ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                 NAME_AUTHORITY, hostaux_recp->source_archie_hostname,
                 NULL);
  }

  if (GET_AR_H_LAST_MOD(attrib_list))
  {
    sprintf(tmp_str,"%sZ", cvt_from_inttime(hostaux_recp->update_time));
    ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                 NAME_AR_H_LAST_MOD, tmp_str, NULL);
  }
		     
  if (GET_AR_RECNO(attrib_list))
  {
    sprintf(tmp_str, "%ld", hostaux_recp->no_recs);
    ad2l_seq_atr(curr_link, ATR_PREC_CACHED, ATR_NATURE_APPLICATION,
                 NAME_AR_RECNO, tmp_str, NULL);
  }

#ifdef GOPHERINDEX_SUPPORT
  if (strcasecmp(dbname, GOPHERINDEX_DB_NAME) == 0)
  {
    char **sptr;
    int host_port = DEFAULT_GOPHER_PORT;
    pathname_t host_path;
    pathname_t fullpath;

    if ((sptr = str_sep(hostaux_recp->access_command, NET_DELIM_CHAR)) == (char **)NULL)
    {
      sprintf(fullpath, "1%-30s (%s)\t%s\t%s\t%d", hostptr,
              cvt_to_usertime(hostaux_recp->update_time, 1), CRLF, hostptr, DEFAULT_GOPHER_PORT);
    }
    else
    {
      if(sptr[1] && *sptr[1])
      {
        strcpy(host_path, sptr[1]);
      }
      else
      {
        host_path[0] = '\0';
      }

      if(sptr[0] && *sptr[0]) host_port = atoi(sptr[0]);

      sprintf(fullpath, "1%-30s (%s)\t%s\t%s\t%d", hostptr,
              cvt_to_usertime(hostaux_recp->update_time,1), host_path, hostptr, host_port);
    }

    ad2l_seq_atr(curr_link, ATR_PREC_OBJECT, ATR_NATURE_APPLICATION,
                 "GOPHER-MENU-ITEM", fullpath, NULL);
  }
#endif    

  APPEND_ITEM(curr_link, ob->links);
  return PRARCH_SUCCESS;
}

