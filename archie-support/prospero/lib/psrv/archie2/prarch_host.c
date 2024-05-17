#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/param.h>

#include <sys/mman.h>

/* Archie definitions */
#include <ndbm.h>
#include <defines.h>
#include <archie_defs.h>
#include <structs.h>
#include <database.h>
#include <error.h>

#include "prarch.h"

#include <pfs.h>
#include <perrno.h>
#include <plog.h>
#include <pmachine.h>		/* For bzero */
#define TOO_MANY_HOSTS  200

/*
 * prarch_host - Search host for contents of directory 
 *
 *  ARGS: site_name - name of host for which search is to be made
 *          dirname - name of directory to return (NULL if root )
 *               vd - pointer to directory to be filled in
 *        archiedir - flag - directory links should be to archie 
 */
int prarch_host(char	*site_name, /* Name of host to be searched         */
		char	*dirname,   /* Name of directory to be listed      */
		VDIR	vd,	    /* Directory to be filled in           */
		int	flags)      /* Flags: Which attributes to use      */
{
    site_out	so;
    char	*host_name;
    char	result[MAX_STRING_LEN];
    char	date_str[SMALL_STR_LEN];
    char	hostip_str[SMALL_STR_LEN];
    site_rec	curr_site_rec;
    site_rec	rootrec;
    int		recno;
    int		last_parent = -1;  
    site_rec 	*site_ptr;
    int		correct_dir = 0; /* Scanning the requested directory   */
    int		loopcount = 0;   /* To decide when to call ardp_accept */
    VLINK 	clink;           /* Current link                       */
    FILE *fp;
    
    caddr_t site_begin;
    site_rec *site_end;
    struct stat statbuf;
  
    if(!dirname) { /* Find host directory */
	char	hosttemp[200];
	char	*p = hosttemp;
	char	*htemp = site_name;
	char	tmp1[MAX_STRING_LEN];
	char	tmp2[MAX_STRING_LEN];
	char	dirlinkname[MAXPATHLEN];
	char	**test;
	int i;
	
	/* If a single wildcard, then return nothing */
	if(strcmp(site_name,"*") == 0) return(PRARCH_SUCCESS);

	/* If regular expressions or wildcards */
	if((index(site_name,'(') || index(site_name,'?') ||
	    index(site_name,'*'))) {
	    
	    if((*htemp == '(') && (*(htemp + strlen(htemp)-1) == ')')) {
		strncpy(hosttemp,htemp+1,sizeof(hosttemp));
		hosttemp[sizeof(hosttemp)-1] = '\0';
		hosttemp[strlen(hosttemp)-1] = '\0';
	    }
	    else if(htemp) {
		*p++ = '^';
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
		*p++ = '$';
		*p++ = '\0';
	    }
	    
	    test = (char **) find_sites(hosttemp,&i,tmp1);
	    if((int) test == BAD_REGEX) {
		p_err_string = qsprintf_stcopyr(p_err_string,
			"archie find_sites(): bad regular expression");
		return(PRARCH_BAD_REGEX);
	    }
	    if((int) test ==  DB_HBYADDR_ERROR) {
		p_err_string = qsprintf_stcopyr(p_err_string,
			"archie find_sites() hostbyaddr error");
		return(PRARCH_DB_ERROR);
	    }
	    if((int) test ==  BAD_MALLOC) {
		p_err_string = qsprintf_stcopyyr(p_err_string,
			"archie find_sites() out of memory");
		return(PRARCH_OUT_OF_MEMORY);
	    }
	    if(i > TOO_MANY_HOSTS) {
		free(test[i]);
		return(PRARCH_TOO_MANY);
	    }
	    else while( i-- ) {
		get_site_file(test[i],tmp2);
		if((fp = fopen(db_file(tmp2),"r")) != (FILE *) NULL) {
		    
		    if(fstat(fileno(fp),&statbuf) == -1) {
			plog(L_DB_ERROR,NOREQ,"can't stat site file %s",db_file(tmp2));
			fclose(fp);
			continue;
		    }
		    
		    site_begin = mmap(0,statbuf.st_size,PROT_READ,MAP_SHARED,
				      fileno(fp),0);   
		    
		    if((site_begin == (caddr_t)-1) || (site_begin == (caddr_t)NULL)){
			plog(L_DB_ERROR,NOREQ,"can't map site file %s",db_file(tmp2));
			fclose(fp);
			continue;
		    }
		    
		    bzero(&so,sizeof(so));
		    if(print_sinfo(site_begin,so.site_name,hostip_str,date_str) != 0) {
			plog(L_DB_ERROR,NOREQ,"can't obtain site info from %s",
			     db_file(tmp2));
			munmap(site_begin,statbuf.st_size);
			fclose(fp);
			continue;
		    }
		    
		    /* The root is the first record in the site after site info */
		    rootrec = *(((site_rec *) site_begin));
		    
		    bcopy(&rootrec,&(so.site_ent),sizeof(rootrec));
		    bcopy(&(rootrec.in_or_addr.ipaddress),&(so.site_ipaddr),
			  sizeof(so.site_ipaddr));
		    bcopy(&(rootrec.mod_time),&(so.site_mod_time),
			  sizeof(so.site_mod_time));
		    clink = atoplink(&so,flags|A2PL_ARDIR|A2PL_ROOT);
		    if(clink) vl_insert(clink,vd,VLI_NOSORT);

		    if(munmap(site_begin,statbuf.st_size) == -1) {
			plog(L_DB_ERROR,NOREQ,"archie munmap() failed on %s",db_file(tmp2));
			return(PRARCH_CLEANUP);
		    }
		    
		    fclose(fp);
		    free(test[i]);
		}
		else plog(L_DB_ERROR,NOREQ,"fopen failed for %s",db_file(tmp2));
	    }
	    return(PRARCH_SUCCESS);
	}
	/* No regular expression or wildcards */
	else {
	    if(( host_name = get_host_file_name( site_name )) == (char *)NULL )
		return(PRARCH_SUCCESS);	/* No match */
	    
	    if((fp = fopen(host_name,"r")) != (FILE *) NULL) {
		
		if(fstat(fileno(fp),&statbuf) == -1) {
		    plog(L_DB_ERROR,NOREQ,"can't stat site file %s",db_file(tmp2));
		    fclose(fp);
		    return(PRARCH_CANT_OPEN_FILE);
		}
		
		site_begin = mmap(0,statbuf.st_size,PROT_READ,MAP_SHARED,
				  fileno(fp),0);   
		
		if((site_begin == (caddr_t)-1) || (site_begin == (caddr_t)NULL)){
		    plog(L_DB_ERROR,NOREQ,"can't map site file %s",db_file(tmp2));
		    fclose(fp);
		    return(PRARCH_CANT_OPEN_FILE);
		}
		
		bzero(&so,sizeof(so));
		if(print_sinfo(site_begin,so.site_name,hostip_str,date_str) != 0) {
		    plog(L_DB_ERROR,NOREQ,"can't obtain site info from %s",
			 db_file(tmp2));
		    munmap(site_begin,statbuf.st_size);
		    fclose(fp);
		    return(PRARCH_DB_ERROR);
		}
		
		/* The root is the first record in the site after site info */
		rootrec = *(((site_rec *) site_begin));
		bcopy(&rootrec,&(so.site_ent),sizeof(rootrec));
		bcopy(&(rootrec.in_or_addr.ipaddress),&(so.site_ipaddr),
		      sizeof(so.site_ipaddr));
		bcopy(&(rootrec.mod_time),&(so.site_mod_time),
		      sizeof(so.site_mod_time));
		clink = atoplink(&so,flags|A2PL_ARDIR|A2PL_ROOT);
		if(clink) {
		    clink->name = stcopyr(site_name,clink->name);
		    vl_insert(clink,vd,VLI_NOSORT);
		}
		
		if(munmap(site_begin,statbuf.st_size) == -1) {
		    plog(L_DB_ERROR,NOREQ,"archie munmap() failed on %s",db_file(tmp2));
		    return(PRARCH_CLEANUP);
		}
		
		fclose(fp);
		return(PRARCH_SUCCESS);
	    }
	    else return(PRARCH_CANT_OPEN_FILE);
	}
    }
    
    bzero(&so,sizeof(so));
    
    if(( host_name = get_host_file_name( site_name )) == (char *)NULL )
	return(PRARCH_DONT_HAVE_SITE);
    
    if((fp = fopen(host_name, "r")) == NULL) 
	return(PRARCH_CANT_OPEN_FILE);
        
    if(fstat(fileno(fp),&statbuf) == -1) {
	fclose(fp);
	return(PRARCH_CANT_OPEN_FILE);
    }
    
    site_begin = mmap(0,statbuf.st_size,PROT_READ,MAP_SHARED,
		      fileno(fp),0);   
    
    if((site_begin == (caddr_t) -1) || (site_begin == (caddr_t) NULL))  {
	fclose(fp);
	return(PRARCH_CANT_OPEN_FILE);
    }
    
    if(print_sinfo(site_begin,so.site_name,hostip_str,date_str) != 0) {
	munmap(site_begin,statbuf.st_size);
	fclose(fp);
	return(PRARCH_DB_ERROR);
    }
    
    site_end = (site_rec *)site_begin + statbuf.st_size / sizeof(site_rec);
    
    rootrec = *(((site_rec *) site_begin));

    bcopy(&(rootrec.in_or_addr.ipaddress),&(so.site_ipaddr),
	  sizeof(so.site_ipaddr));
    bcopy(&(rootrec.mod_time),&(so.site_mod_time),
	  sizeof(so.site_mod_time));

    for(recno = 1;(site_ptr = (site_rec *)site_begin + recno) < site_end; 
	recno++){
	
	if((loopcount++ & 0x3ff) == 0) ardp_accept();
	
	curr_site_rec = *site_ptr;
	
	if(last_parent != curr_site_rec.parent_ind){
	    
	    if(find_ancestors(site_begin, recno, result) != 0) {
		munmap(site_begin,statbuf.st_size);
		fclose(fp);
		return(PRARCH_DB_ERROR);
	    }
	    
	    last_parent = curr_site_rec.parent_ind;
	    
	    /* Don't want to check the leading / */
	    if(strcmp(dirname,result+1) == 0)  {
		correct_dir++;
		strcpy(so.site_path,result);
	    }
	    else if(correct_dir) break;
	}
	bcopy(&curr_site_rec,&(so.site_ent),sizeof(curr_site_rec));
	if(correct_dir) {
	    if((loopcount & 0x7f) == 0) ardp_accept();
	    clink = atoplink(&so,flags);
	    if(clink) vl_insert(clink,vd,VLI_NOSORT);
	}
    }
    
    munmap(site_begin,statbuf.st_size);
    fclose(fp);
    return(PRARCH_SUCCESS);
}
