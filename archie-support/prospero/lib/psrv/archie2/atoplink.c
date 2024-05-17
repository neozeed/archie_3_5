#include <stdio.h>
#include <sys/types.h>

#include <database.h>
#include <sys/mman.h>
#include <defines.h>
#include <structs.h>
#include <pfs.h>
#include <psite.h>
#include "prarch.h"

extern FILE *strings_table;
extern char *strings_begin;

extern char	hostname[];
extern char	hostwport[];
extern char	archie_prefix[];

char	*perms_itoa();
char	*print_date();
char	*atopdate();
char	*strstr();

VLINK atoplink(site_out	*sop,	/* Site output pointer                  */
	       int	flags)	/* Flags: see above                     */
{
    VLINK		vl = vlalloc();  /* New link			*/
    PATTRIB		at;	         /* Attributes                  */
    PATTRIB		last_at;	 /* Last attribute              */
    char 		fullpath[MAX_STRING_LEN];
    char 		namebuf[MAX_STRING_LEN];
    char		*endname = NULL;
    char 		modestring[20];
    char 		str_ent[256];
    char 		atval[256];
    char		*nameptr;        /* Last component of file name */
    char		*ptr;
    site_rec	*srp = &(sop->site_ent); /* Site record pointer */
    long		strings_pos;
  
    /* For now, all directory pointers are to pseudo-directories */
    flags |= A2PL_ARDIR;
  
    if((flags & A2PL_ROOT) || (srp->dir_or_f == 'T')) {
	/* It's a directory - we should check to see if the site is   */
	/* running prospero, and if so return a pointer to the actual */
	/* directory.  If it isn't then we return a real pointer to   */
	/* a pseudo-directory maintained by this archie server.       */
	vl->target = stcopyr("DIRECTORY",vl->target);
    }
    else {
	/* It's a file - we should check to see if the site is        */
	/* running prospero, and if so return a pointer to the real   */
	/* file.  If it isn't, then we generate an external link      */
	vl->target = stcopyr("EXTERNAL",vl->target);
	ad2l_am_atr(vl,"AFTP","BINARY",NULL);
	flags &= (~A2PL_ARDIR);
    }
  
    if(flags & A2PL_ARDIR) vl->host = stcopyr(hostwport,vl->host);
    else vl->host = stcopyr(sop->site_name,vl->host);
  
    /* Get the the last component of name */
  
    if(flags & A2PL_ROOT) vl->name = stcopyr(sop->site_name,vl->name);
    else {
	strncpy(namebuf,strings_begin + srp->in_or_addr.strings_ind + 
		sizeof(strings_header),sizeof(namebuf));
	namebuf[sizeof(namebuf)-1] = '\0';
	if(endname = strstr(namebuf," -> ")) *endname = '\0';
	nameptr = namebuf;
	vl->name = stcopyr(nameptr,vl->name);
    }
  
    if(flags & A2PL_ARDIR) {
	if(flags & A2PL_ROOT) 
	    sprintf(fullpath,"%s/HOST/%s",archie_prefix, sop->site_name);
	else 
	    sprintf(fullpath,"%s/HOST/%s%s%s%s",archie_prefix,
		    sop->site_name, sop->site_path,
		    ((*(sop->site_path + strlen(sop->site_path) - 1) == '/') ?
		     "" : "/"), (nameptr ? nameptr : ""));
    }
    else {
	if(flags & A2PL_ROOT) 
	    sprintf(fullpath,"/");
	else
	    sprintf(fullpath,"%s%s%s",sop->site_path, 
		    ((*(sop->site_path + strlen(sop->site_path) - 1) == '/') ?
		     "" : "/"), (nameptr ? nameptr : ""));
    }
  
    vl->hsoname = stcopyr(fullpath,vl->hsoname);
  
    if(!(flags & A2PL_ROOT)) {
	/* Here we can add cached attribute values from the archie   */
	/* database such as size, protection, and last modified time */
	sprintf(atval,"%d bytes",srp->size);
	ad2l_seq_atr(vl,ATR_PREC_CACHED,ATR_NATURE_INTRINSIC,
		     "SIZE",atval,NULL);
    
	/* Directory modes in unix string format */
	if(ptr = perms_itoa(srp->perms)) {
	    if(endname) sprintf(modestring,"%c%s",'l',ptr);
	    else sprintf(modestring,"%c%s",((srp->dir_or_f=='T')?'d':'-'),ptr);
	    ad2l_seq_atr(vl,ATR_PREC_CACHED,ATR_NATURE_INTRINSIC,
			 "UNIX-MODES", modestring, NULL);
	}
    
	/* Modified date - in prospero format */
	if(ptr = atopdate(srp->mod_time)) {
	    ad2l_seq_atr(vl,ATR_PREC_CACHED,ATR_NATURE_INTRINSIC,
			 "LAST-MODIFIED", ptr, NULL);
	}
    }

    if((flags & A2PL_ROOT) || (flags & A2PL_H_LAST_MOD)) {
	/* Modified date - in prospero format */
	if(ptr = atopdate(sop->site_mod_time))
	    ad2l_seq_atr(vl,ATR_PREC_CACHED,ATR_NATURE_APPLICATION,
			 "AR_H_LAST_MOD", ptr, NULL);
    }
       
    if((flags & A2PL_ROOT || (flags & A2PL_H_IP_ADDR))) {
	/* Host IP Address */
	if(sop->site_ipaddr.s_addr) 
	    ad2l_seq_atr(vl,ATR_PREC_CACHED,ATR_NATURE_APPLICATION,
			 "AR_H_IP_ADDR", inet_ntoa(sop->site_ipaddr),
			 NULL);
    }
    return(vl);
}

VLINK atoqlink(char *str,int maxhit,int maxmatch,int maxhitpm)
{
    VLINK		vl = vlalloc();  
    char 		fullpath[MAX_STRING_LEN];

    sprintf(fullpath,"%s/MATCH(%d,%d,%d,0,=)/%s", archie_prefix, 
	    maxhit, maxmatch, maxhitpm, str);
  
    vl->name = stcopyr(str,vl->host);
    vl->target = stcopyr("DIRECTORY",vl->target);
    vl->hsoname = stcopyr(fullpath,vl->hsoname);
    vl->host = stcopyr(hostwport,vl->host);
    return(vl);
} 
