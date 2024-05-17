/*
 * Copyright (c) 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>

#include <string.h>		/* No strings.h in SOLARIS */

#include <ardp.h>
#include <pfs.h>
#include <psite.h>
#include <pprot.h>
#include <plog.h>
#include <pmachine.h>
#include <psrv.h>
#include <perrno.h>

static	ACL	find_aclent();
char	*addrights();
char	*subtractrights();
extern ACL	nulldir_acl;
extern ACL	nullobj_acl;
extern ACL	nullcont_acl;
extern ACL	nulllink_acl;

ACL aclcopy();

/*
 * change_acl - change access control list
 *
 *       CHANGE_ACL change an access control list (*aclp)
 *       by adding, deleting, or modifying the entry
 *       specified by ae.
 *
 *  ARGS:  aclp - pointer to ACL pointer
 *           ae - a pointer to a single ACL entry to be added or deleted
 *           id - client identification
 *        flags - information on the operation to be performed
 *       diracl - the directory acl (to check if user has access)
 *
 *  NOTE:  This code which automatically adds administer (unless noself
 *         is specified) can be used to upgrade < or [ privs to A.  Dirsrv
 *         include a check which sets noself when admister access is
 *         allows by virtual of a [ or <.
 */
int
change_acl(aclp,ae,req,flags,diracl)
    ACL		*aclp;	/* Pointer to ACL pointer */
    ACL		ae;	/* ACL entry to change    */
    RREQ	req;	/* Client identification  */
    int		flags;  /* Operation and flags    */
    ACL		diracl;/* Directory ACL          */
    {
	ACL	a = *aclp;
	ACL	ws;
	ACL	wacl;
	int	operation;
	int	nosystem;
	int	noself;
	int	dflag;
	int	oflag;
	int	adminflag;
	int	acltype;
	int	sca_code;
	PAUTH	painfo = NULL;

	operation = flags & EACL_OP;
	nosystem = flags & EACL_NOSYSTEM;
	noself = flags & EACL_NOSELF;
	dflag = !((flags&EACL_OTYPE)^EACL_DIRECTORY);
	oflag = !((flags&EACL_OTYPE)^EACL_OBJECT);
	acltype = (flags&EACL_OTYPE);

	if(acltype == EACL_LINK) sca_code = SCA_LINK;
	else if(acltype == EACL_DIRECTORY) sca_code = SCA_DIRECTORY;
	else if(acltype == EACL_OBJECT) sca_code = SCA_OBJECT;
	else sca_code = SCA_MISC;

	switch(operation) {

	case EACL_DEFAULT:
	    aclfree(a);
	    acfree(ae);
	    a = NULL;

	    /* If adminflag is clear it means that the user would not be   */
	    /* able to administer the new directory or link, and that      */
	    /* such a situation is undesirable                             */
	    if(noself) adminflag = 1; /* Ok to clear admin rights for user */
	    else if(acltype == EACL_LINK) 
		adminflag = srv_check_acl(a,diracl,req,"a",sca_code,NULL,NULL);
	    else adminflag = srv_check_acl(a,a,req,"A",sca_code,NULL,NULL);

	    if(acltype == EACL_LINK) a = aclcopy(nulllink_acl);
	    else if(acltype == EACL_DIRECTORY) a = aclcopy(nulldir_acl);
	    else if(acltype == EACL_OBJECT) a = aclcopy(nullobj_acl);

	    if(adminflag == 0) srv_add_client_to_acl("Aa",req,&(a),acltype);

	    *aclp = a;
	    return(PSUCCESS);

	case EACL_SET:
	    aclfree(a);
	    a = ae;

	    /* Unless the nosystem flag has been specified, add an */
	    /* entry for the SYSTEM ACL                            */
	    if(!nosystem) {
		wacl = acalloc();
		wacl->acetype = ACL_SYSTEM;
		a->next = wacl;
		wacl->previous = a;
	    }

	    /* If adminflag is clear it means that the user would not be   */
	    /* able to administer the new directory or link, and that      */
	    /* such a situation is undesirable                             */
	    if(noself) adminflag = 1; /* Ok to clear admin rights for user */
	    else if(acltype == EACL_LINK) 
		adminflag = srv_check_acl(a,diracl,req,"a",sca_code,NULL,NULL);
	    else adminflag = srv_check_acl(a,a,req,"A",sca_code,NULL,NULL);

	    if(adminflag == 0) srv_add_client_to_acl("Aa",req,&(a),acltype);

	    *aclp = a;
	    return(PSUCCESS);

	eacl_insert:
	case EACL_INSERT:
	    /* Must make sure all required fields are specified      */
	    /* Rights must be included for all but NONE, DEFAULT,    */
	    /* SYSTEM, and DIRECTORY, CONTAINER, and IETF_AAC        */
	    if(!((ae->rights && *(ae->rights))||(ae->acetype==ACL_NONE) || 
		 (ae->acetype==ACL_DEFAULT)||(ae->acetype==ACL_SYSTEM) || 
		 (ae->acetype==ACL_DIRECTORY)||(ae->acetype==ACL_CONTAINER)||
		 (ae->acetype==ACL_IETF_AAC))) {
		RETURNPFAILURE;
	    }

	    /* No need to make sure the user can still access since  */
	    /* we are only adding rights.  This ignores the case of  */
	    /* negative rights, but for now we assume that if the    */
	    /* user is specifying negative rights that they are      */
	    /* intended                                              */

	    /* If NULL, first add entries for default values.  We need */
	    /* to leave them even if the user said not to add them     */
	    /* Since they were "already there".                        */
	    if(!a) {
		if(acltype == EACL_LINK) a = aclcopy(nulllink_acl);
		else if(acltype == EACL_DIRECTORY) a = aclcopy(nulldir_acl);
		else if(acltype == EACL_OBJECT) a = aclcopy(nullobj_acl);
	    }

	    /* Check to see if entry already exists */
	    wacl = find_aclent(a,ae,1);
	    if(wacl) {
		/* Already there */
		aclfree(ae);
		*aclp = a;
		return(PSUCCESS);
	    }

	    /* New ACL antries are added a head of list.  This means */
	    /* that any negative rights specified will override any  */
	    /* rights that were already present.  Additionaly, any   */
	    /* positive rights specified will override any negative  */
	    /* rights that already existed.                          */
	    wacl = ae;
	    while(wacl->next) wacl = wacl->next;
	    wacl->next = a;
	    a->previous = wacl;
	    a = wacl;
	    *aclp = a;
	    return(PSUCCESS);

	eacl_delete:
	case EACL_DELETE:
	    /* If NULL, first add entries for default values.  We need */
	    /* to leave them even if the user said not to add them     */
	    /* Since they were "already there".                        */
	    if(!a) {
		if(acltype == EACL_LINK) a = aclcopy(nulllink_acl);
		else if(acltype == EACL_DIRECTORY) a = aclcopy(nulldir_acl);
		else if(acltype == EACL_OBJECT) a = aclcopy(nullobj_acl);
	    }

	    wacl = find_aclent(a,ae,1);
	    if(!wacl) RETURNPFAILURE;
	    if(a == wacl) {
		a = a->next;
		a->previous = NULL;
		acfree(wacl);
	    }
	    else {
		wacl->previous->next = wacl->next;
		if(wacl->next) wacl->next->previous = wacl->previous;
		acfree(wacl);
	    }

	    if(noself) adminflag = 1; /* Ok to clear admin rights for user */
	    else if(acltype == EACL_LINK) 
		adminflag = srv_check_acl(a,diracl,req,"a",sca_code,NULL,NULL);
	    else adminflag = srv_check_acl(a,a,req,"A",sca_code,NULL,NULL);

	    if(adminflag == 0) srv_add_client_to_acl("Aa",req,&(a),acltype);

	    /* If empty, must create a placeholder so that it */
	    /* doesn't revert to nulldir                      */
	    if(!a) {
		a = acalloc();
		a->acetype = ACL_NONE;
	    }
	    
	    acfree(ae);
	    *aclp = a;
	    return(PSUCCESS);

	case EACL_ADD:
	    /* If no rights specified must be insert */
	    if(!(ae->rights)) goto eacl_insert;

	    /* Havn't figured out how to ADD ><][)(, so go to INSERT */
	    if(index("><][)(",*(ae->rights))) goto eacl_insert;

	    /* If NULL, first add entries for default values.  We need */
	    /* to leave them even if the user said not to add them     */
	    /* Since they were "already there".                        */
	    if(!a) {
		if(acltype == EACL_LINK) a = aclcopy(nulllink_acl);
		else if(acltype == EACL_DIRECTORY) a = aclcopy(nulldir_acl);
		else if(acltype == EACL_OBJECT) a = aclcopy(nullobj_acl);
	    }

	    wacl = find_aclent(a,ae,0);

	    /* If no other entries, then go to insert */
	    if(!wacl) goto eacl_insert;

	    /* Now we must add characters to wacl->rights for */
	    /* any new characters in ae->rights               */
	    wacl->rights = stcopyr(addrights(wacl->rights,ae->rights),wacl->rights);
	    acfree(ae);
	    return(PSUCCESS);

	case EACL_SUBTRACT:
	    /* If no rights specified must delete */
	    if(!(ae->rights)) goto eacl_delete;

	    /* Havn't figured out how to DELETE ><][)(, so go to DELETE */
	    if(index("><][)(",*(ae->rights))) goto eacl_delete;

	    wacl = find_aclent(a,ae,0);

	    /* If no other entries, return error */
	    if(!wacl) {
		acfree(ae);
		RETURNPFAILURE;
	    }

	    /* Now we must subtract characters from wacl->rights */
	    /* for the characters in ae->rights                  */
	    wacl->rights = subtractrights(wacl->rights,ae->rights);

	    /* If rights are now null, must delete the entry */
	    if(!wacl->rights || !*(wacl->rights)) {
		if(wacl->previous) {
		    wacl->previous->next = wacl->next;
		    if(wacl->next) wacl->next->previous = wacl->previous;
		    acfree(wacl);
		}
		else { /* It is at start of list */
		    a = wacl->next;
		    a->previous = NULL;
		    acfree(wacl);
		}
	    }

	    /* Make sure that user can fix his mistakes */
	    if(noself) adminflag = 1; /* Ok to clear admin rights for user */
	    else if(acltype == EACL_LINK) 
		adminflag = srv_check_acl(a,diracl,req,"a",sca_code,NULL,NULL);
	    else adminflag = srv_check_acl(a,a,req,"A",sca_code,NULL,NULL);

	    if(adminflag == 0) srv_add_client_to_acl("Aa",req,&(a),acltype);

	    /* If empty, must create a placeholder so that it */
	    /* doesn't revert to nulldir                      */
	    if(!a) {
		a = acalloc();
		a->acetype = ACL_NONE;
	    }
	    
	    acfree(ae);
	    *aclp = a;
	    return(PSUCCESS);
	}

	RETURNPFAILURE;
    }


/* 
 * This function adds rights to the ACL for all principals identified
 * in the clients auth_info structure   
 */
void
srv_add_client_to_acl(char *rights,     /* Rights to be added     */
		      RREQ	req,	/* Client identification  */
		      ACL	*inoutacl,
		      int	flags) /* Directory ACL          */
{
    ACL 	nacl = NULL;               /* new ACL entry */
    PAUTH	painfo = req->auth_info;
    int		times_thru = 0;
    while(painfo) {
	/* Defer the weakest method unless it's the only method */
	if((painfo->ainfo_type == PFSA_UNAUTHENTICATED) && !check_prvport(req))
	    {painfo = painfo->next; continue;}
	nacl = acalloc();
	nacl->rights = stcopyr(rights,nacl->rights);

	if(painfo->ainfo_type == PFSA_UNAUTHENTICATED) {
	    TOKEN p = req->auth_info->principals;
	    nacl->acetype = ACL_TRSTHOST;
	    nacl->rights = stcopyr(rights,nacl->rights);
	    
	    for ( ; p ; p = p->next) {
		nacl->principals = tkappend(NULL,nacl->principals);
		nacl->principals->previous->token = 
		    qsprintf_stcopyr(NULL,"%s@%s",p->token,
				     inet_ntoa(req->peer_addr));
	    }
	}
	else if(painfo->ainfo_type == PFSA_KERBEROS) {
	    nacl->acetype = ACL_AUTHENT;
	    nacl->atype = stcopy("KERBEROS");
	}
	else if(painfo->ainfo_type == PFSA_P_PASSWORD) {
	    nacl->acetype = ACL_AUTHENT;
	    nacl->atype = stcopy("P_PASSWORD");
	}

	if(!(nacl->principals)) nacl->principals = tkcopy(painfo->principals);
	change_acl(inoutacl,nacl,req,EACL_ADD|flags,*inoutacl); 

	painfo = painfo->next;
    }
    /* OK, so ASRTHOST is the only method, I guess we'll use it */
    if(!nacl && req->auth_info && 
       (req->auth_info->ainfo_type == PFSA_UNAUTHENTICATED)) {
	TOKEN p = req->auth_info->principals;
	nacl = acalloc();
	nacl->acetype = ACL_ASRTHOST;
	nacl->rights = stcopyr(rights,nacl->rights);

	for ( ; p ; p = p->next) {
	    nacl->principals = tkappend(NULL,nacl->principals);
	    nacl->principals->previous->token = 
		qsprintf_stcopyr(NULL,"%s@%s",p->token,
				 inet_ntoa(req->peer_addr));
	}
	change_acl(inoutacl,nacl,req,EACL_ADD|flags,*inoutacl); 
    }
}

/*
 * Find an ACL entry matching entry e in list a
 *
 *         r is a flag which if set means the rights must match.
 *         if clear, then the rights can be different.
 */
static ACL find_aclent(a,e,r)
    ACL	a;
    ACL e;
    int r;
    {
	ACL w;

	w = a;
	while(w) {
	    if((w->acetype == e->acetype) &&
	       ((!(w->atype) && !(e->atype)) ||
		(w->atype && e->atype && (strcmp(w->atype,e->atype)==0))) &&
	       ((!(w->principals) && !(e->principals)) ||
		(w->principals && e->principals 
                 && equal_sequences(w->principals,e->principals))) &&
	       ((!r && (!(w->rights) || !(index("><)(][",*(w->rights))))) ||
		(!(w->rights) && !(e->rights)) ||
		(w->rights && e->rights && (strcmp(w->rights,e->rights)==0))))
		return(w);
	    w = w->next;
	}
	return(NULL);
    }



/* 
 * Add rights returns a string containing those rights
 * that are in r or a.  For known rights, addrights
 * will try to place them in canonical order.
 *
 * WARNING: Subtractrights frees r.  It is expected that
 *          the string pointed to by r will be replaced
 *          by the string handed back.
 */
char *addrights(char *r, char *a)
{
    /* Don't need to mutex; constant. */
    const char	canonical[] = "-><)(][STUABVYLlRQGgrWMmwuDKzdEeIiPp";
    char		*cp;    /* index variable */
    const char          *ccp;   /* index variable */
    char		*newrights;
    char		*nr; /* newrights */

    if(!r) return(stcopy(a));

    nr = newrights = stalloc(strlen(r)+strlen(a)+1);

    /* First check each known right */
    for(ccp = canonical;*ccp;ccp++) {
        if(index(r,*ccp)) *(nr++) = *ccp;
        else if(index(a,*ccp)) *(nr++) = *ccp;
    }
    /* Now scan r and include anything that is not canonical */
    for(cp = r;*cp;cp++) {
        if(index(canonical,*cp)==0) *(nr++) = *cp;
    }
    /* Now scan a and include anything that is not canonical */
    /* and isn't in r                                        */
    for(cp = a;*cp;cp++) {
        if((index(canonical,*cp)==0)&&
           (index(r,*cp)==0)) *(nr++) = *cp;
    }
    *(nr++) = '\0';
    stfree(r);
    return(newrights);
}

/* 
 * Subtract rights returns a string containing those rights
 * in r that are not in s. 
 *
 * WARNING: Subtractrights frees r.  It is expected that
 *          the string pointed to by r will be replaced
 *          by the string handed back.
 */
char *subtractrights(r,s)
    char	*r;
    char	*s;
    {
	char		*newrights = stalloc(strlen(r)+1);
	char		*or; /* oldrights */
	char		*nr; /* newrights */

	for(or = r,nr = newrights;*or;or++) {
	    if(strchr(s,*or)==NULL) *(nr++) = *or;
	}
	*(nr++) = '\0';

	stfree(r);
	return(newrights);
    }
