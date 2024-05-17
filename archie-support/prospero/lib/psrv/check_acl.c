/*
 * Copyright (c) 1991 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <string.h>
#include <sys/types.h>          /* for decl. of inet_ntoa() */
#include <sys/socket.h>     /* for decl. of inet_ntoa() */
#include <netinet/in.h>     /* for decl. of inet_ntoa() */
#include <arpa/inet.h>      /* for decl. of inet_ntoa() */


#include <ardp.h>
#include <pfs.h>
#include <pserver.h>
#include <pprot.h>
#include <plog.h>
#include <pmachine.h>
#include <psrv.h>

#ifndef IPPORT_RESERVED
#define IPPORT_RESERVED 1024
#endif


/* used to initialize ACLs */
struct aclinit {
    int acetype;
    char *atype;
    char *rights;
    char *prin_1;                /* principals list */
    char *prin_2;                /* principals list */
    char *prin_3;                /* principals list */
    char *prin_4;                /* principals list */
    char *prin_5;                /* principals list */
    char *prin_6;                /* principals list */
    char *prin_7;                /* principals list */
    char *prin_8;                /* principals list */
    char *prin_9;                /* principals list */
    char *prin_10;                /* principals list */
    char *prin_11;                /* principals list */
    char *prin_12;                /* principals list */
    char *prin_13;                /* principals list */
    char *prin_14;                /* principals list */
    char *prin_15;                /* principals list */
    char *prin_16;                /* principals list */
    char *prin_17;                /* principals list */
    char *prin_18;                /* principals list */
    char *prin_19;                /* principals list */
    char *prin_20;                /* principals list */
    char *prin_21;                /* principals list */
    char *prin_22;                /* principals list */
    char *prin_23;                /* principals list */
    char *prin_24;                /* principals list */
    char *prin_25;                /* principals list */
    char *prin_26;                /* principals list */
};

static	ACL_ST default_iacl = {ACL_DEFAULT, NULL};
static	ACL_ST system_iacl = {ACL_SYSTEM, NULL};
static	ACL_ST container_iacl = {ACL_CONTAINER, NULL};
static	ACL_ST directory_iacl = {ACL_DIRECTORY, NULL};

static struct aclinit default_aclinit[] = DEFAULT_ACL;
static struct aclinit system_aclinit[] = SYSTEM_ACL;
static struct aclinit override_aclinit[] = OVERRIDE_ACL;
static struct aclinit maint_aclinit[] = MAINT_ACL;

/* Only modified in p_srv_check_acl_initialize_defaults(), which is called
   before we go multi-threaded. */
ACL	default_acl = NULL;
ACL	system_acl = NULL;
ACL	override_acl = NULL;
ACL	nulldir_acl = NULL;
ACL	nullobj_acl = NULL;
ACL	nullcont_acl = NULL;
ACL	nulllink_acl = NULL;
ACL     maint_acl = NULL;

static checkl_acl_r();
static check_permissions();
static check_asserted_principal();
ACL get_container_acl();
static ACL aclinit2acl(struct aclinit init[], int nelems);
static int checkl_acl_r(ACL acl, ACL dacl, RREQ req, char *op, int depth, int ld,char *objid,char *itemid);

/* Called before things start to run in psrv. */
void
p_srv_check_acl_initialize_defaults(void)
{
    default_acl = aclinit2acl(default_aclinit, 
                              sizeof default_aclinit 
                              / sizeof default_aclinit[0]);
    system_acl = aclinit2acl(system_aclinit, 
                              sizeof system_aclinit 
                              / sizeof system_aclinit[0]);
    override_acl = aclinit2acl(override_aclinit, 
                              sizeof override_aclinit 
                              / sizeof override_aclinit[0]);
    maint_acl = aclinit2acl(maint_aclinit, 
                              sizeof maint_aclinit 
                              / sizeof maint_aclinit[0]);

    default_iacl.next = &system_iacl;
    system_iacl.previous = &default_iacl;
    nulldir_acl = &default_iacl;
    nulllink_acl = &directory_iacl;
    nullobj_acl = &container_iacl;
    nullcont_acl = nulldir_acl;

}

static ACL aclentryinit2aclentry(struct aclinit *init);

/* constructs a real list of ACL entries out of an array of aclinit
   structures. */
static ACL 
aclinit2acl(struct aclinit init[], int nelems)
{

    ACL rethead = NULL;       /* head of list of AC entries being returned */ 
    int i;

    for (i = 0; i < nelems; ++i) {
        ACL tmp = aclentryinit2aclentry(&init[i]);
        APPEND_ITEM(tmp, rethead);
    }
    return rethead;
}


static ACL
aclentryinit2aclentry(struct aclinit *init)
{
    ACL retval;
    retval = acalloc();
    if (!retval) out_of_memory();
    retval->acetype = init->acetype;
    retval->atype = init->atype;
    retval->rights = init->rights;
    retval->principals = NULL;
    if (init->prin_1) 
        retval->principals = tkappend(init->prin_1, retval->principals);
    if (init->prin_2) 
        retval->principals = tkappend(init->prin_2, retval->principals);
    if (init->prin_3) 
        retval->principals = tkappend(init->prin_3, retval->principals);
    if (init->prin_4) 
        retval->principals = tkappend(init->prin_4, retval->principals);
    if (init->prin_5) 
        retval->principals = tkappend(init->prin_5, retval->principals);
    if (init->prin_6) 
        retval->principals = tkappend(init->prin_6, retval->principals);
    if (init->prin_7) 
        retval->principals = tkappend(init->prin_8, retval->principals);
    if (init->prin_9) 
        retval->principals = tkappend(init->prin_9, retval->principals);
    if (init->prin_10) 
        retval->principals = tkappend(init->prin_10, retval->principals);
    if (init->prin_11) 
        retval->principals = tkappend(init->prin_11, retval->principals);
    if (init->prin_12) 
        retval->principals = tkappend(init->prin_12, retval->principals);
    if (init->prin_13) 
        retval->principals = tkappend(init->prin_13, retval->principals);
    if (init->prin_14) 
        retval->principals = tkappend(init->prin_14, retval->principals);
    if (init->prin_15) 
        retval->principals = tkappend(init->prin_15, retval->principals);
    if (init->prin_16)
        retval->principals = tkappend(init->prin_16, retval->principals);
    if (init->prin_17) 
        retval->principals = tkappend(init->prin_17, retval->principals);
    if (init->prin_18) 
        retval->principals = tkappend(init->prin_18, retval->principals);
    if (init->prin_19) 
        retval->principals = tkappend(init->prin_19, retval->principals);
    if (init->prin_20) 
        retval->principals = tkappend(init->prin_20, retval->principals);
    if (init->prin_21) 
        retval->principals = tkappend(init->prin_21, retval->principals);
    if (init->prin_22) 
        retval->principals = tkappend(init->prin_22, retval->principals);
    if (init->prin_23) 
        retval->principals = tkappend(init->prin_23, retval->principals);
    if (init->prin_24) 
        retval->principals = tkappend(init->prin_24, retval->principals);
    if (init->prin_25) 
        retval->principals = tkappend(init->prin_25, retval->principals);
    if (init->prin_26) 
        retval->principals = tkappend(init->prin_26, retval->principals);
    return retval;
}

/*
 * srv_check_acl - check access control list
 *
 *     SRV_CHECK_ACL checks an access control list to see if a particular
 *     user is authorized to perform a particular operation.  It returns
 *     a positive number if the operation is authorized, and zero if
 *     not authorized.  SRV_CHECK_ACL actually checks up to three access
 *     control lists.  First, the access control list associated with
 *     a link is checked (if specified).  If the operation would not
 *     be authorized, the secondary ACL may be checked if present, and 
 *     depending on the type of ACL check specified in the flags field, 
 *     to see if it applies to or overrides the individual entry.  If sill
 *     not authorized,  an override ACL is checked to see if the operation 
 *     is performed.
 *
 *     NOTE on negative rights:  Negative rights apply within
 *     a particular access control list only.  Thus, a negative
 *     entry in the link ACL can override other entries in the
 *     link ACL, but it will not prevent access if the user
 *     is authorized to perform the operation by the directory
 *     or override ACL's.
 */
int
srv_check_acl(ACL	pacl,     /* Primary ACL                             */
	      ACL	sacl,     /* Secondary ACL                           */
	      RREQ	req,      /* Request; used for client identification */
	      char	*op,      /* Operation                               */
	      int	flags,    /* Type of ACL check                       */
	      char	*objid,   /* hsoname of object to which ACL applies  */
	      char	*itemid)  /* Name of item to be manipulated          */
{
    int	answer = 0;

    /* Now called from dirsrv */
#if 0
    if(!initialized) initialize_defaults();
#endif

    if(flags == SCA_LINK) {
	if(!pacl) pacl = nulllink_acl;
	if(!sacl) sacl = nulldir_acl;
	answer = checkl_acl_r(pacl,sacl,req,op,ACL_NESTING,
			      SCA_LINK,objid,itemid);
	if(!answer) answer = checkl_acl_r(sacl,sacl,req,op,ACL_NESTING,
					  SCA_DIRECTORY,objid,itemid);
    }
    else if(flags == SCA_LINKDIR) {
	if(!pacl) pacl = nulllink_acl;
	if(!sacl) sacl = nulldir_acl;
	answer = checkl_acl_r(pacl,sacl,req,op,ACL_NESTING,
			      SCA_LINK,objid,itemid);
	if(!answer) answer = checkl_acl_r(sacl,sacl,req,op,ACL_NESTING,
					  SCA_DIRECTORY,objid,itemid);
    }
    else if(flags == SCA_DIRECTORY) {
	if(!pacl) pacl = nulldir_acl;
	answer = checkl_acl_r(pacl,pacl,req,op,ACL_NESTING,
			      SCA_DIRECTORY,objid,itemid);
    }
    else if (flags == SCA_OBJECT) {
	ACL	cacl = NULL;
	if(!pacl) pacl = nullobj_acl;
	if(!sacl) {
	    if(objid && (cacl = get_container_acl(objid))) {
		answer = checkl_acl_r(pacl,cacl,req,op,ACL_NESTING,
				      SCA_OBJECT,objid,itemid);
		aclfree(cacl);
	    }
	    else answer = checkl_acl_r(pacl,nullcont_acl,req,op,ACL_NESTING,
				       SCA_OBJECT,objid,itemid);
	}
	else answer = checkl_acl_r(pacl,sacl,req,op,ACL_NESTING,SCA_OBJECT,
				   objid,itemid);
    }
    else if (flags == SCA_MISC) {
	if(!pacl) pacl = maint_acl;
	answer = checkl_acl_r(pacl,NULL,req,op,ACL_NESTING,
			      SCA_MISC,objid,itemid);
    }

    if(answer) return(answer);
 
    /* Check to see if absolute override applies */
    return(checkl_acl_r(override_acl,NULL,req,op,ACL_NESTING,
			SCA_MISC, objid,itemid));

}

static int
checkl_acl_r(acl,sacl,req,op,depth,ld,objid,itemid)
    ACL		acl;  	     /* Access control list              */
    ACL		sacl;  	     /* Secondary access control list    */
    char	*op;	     /* Operation                        */
    RREQ	req;	     /* Client identification            */
    int		depth;	     /* Maximum nesting                  */
    int		ld;	     /* 0 = link, 1 = dir, 2 = both, 4=misc  */
    char	*objid;      /* Object associated with ACL       */
    char	*itemid;     /* Name of item to manipulate       */
{
    int	retval = 0;    /* Would this entry authorize op  */
    int	answer = 1;    /* How to answer if match         */

    if(depth == 0) return(NOT_AUTHORIZED);

    while(acl) {
        retval = check_permissions(op,acl->rights,ld);
        if(retval||((acl->rights==NULL)&&((acl->acetype == ACL_DEFAULT)
                                     ||(acl->acetype == ACL_SYSTEM)
                                     ||(acl->acetype == ACL_DIRECTORY)
                                     ||(acl->acetype == ACL_CONTAINER)
                                     ||(acl->acetype == ACL_IETF_AAC)))) {
            if(retval == NEG_AUTHORIZED) answer = NOT_AUTHORIZED;
            else answer = AUTHORIZED;

            switch(acl->acetype) {

            case ACL_NONE:
                break;
            case ACL_DEFAULT:
                retval = checkl_acl_r(default_acl,sacl,req,op,depth-1,ld,objid,itemid);
                if(retval) return(answer);
                break;
            case ACL_SYSTEM:
                retval = checkl_acl_r(system_acl,sacl,req,op,depth-1,ld,objid,itemid);
                if(retval) return(answer);
                break;
            case ACL_OWNER:
                /* Check if user is the owner of the dirtectory */
#if 0
                /* We need to find the owner of the file/directory  */
                /* for which this ACL applies and check whether it  */
		/* is the current principal.  For now, we don't     */
		/* know the name of the file or directory.  When    */
		/* the interface is changed so we do know it, we    */
		/* will check against the current host address and  */
		/* the TRSTHOST authentication type.  Alternatively */
		/* we will use an OBJECT-OWNER attribute from the   */
		/* object attribute list which will be in the form  */
		/* of an ACL entry, but without a rights field      */
#endif
                break;
            case ACL_DIRECTORY:
		if(ld == SCA_LINK) {
		    retval = checkl_acl_r(sacl,sacl,req,op,depth-1,
					  SCA_LINKDIR,objid,itemid);
		    if(retval) return(answer);
		}
                break;
            case ACL_CONTAINER:
		if(ld == SCA_OBJECT) {
		    retval = checkl_acl_r(sacl,sacl,req,op,depth-1,
					  SCA_CONTAINER,objid,itemid);
		    if(retval) return(answer);
		}
                break;
            case ACL_ANY:
                return(answer);
            case ACL_AUTHENT: 
                if (acl->atype && strequal(acl->atype, "KERBEROS")) {
                    PAUTH pap;      /* Iteration variable for PAUTH. */
                    TOKEN prin; /* principals */
                    /* Loop through all of the Kerberos authenticated
                       principals.  (This is upward-compatible with future
                       versions of Kerberos that will allow one to be
                       registered as multiple principals simultaneously.) 
                       */
                    for (pap = req->auth_info; pap; pap = pap->next) {
                        if (pap->ainfo_type == PFSA_KERBEROS) {
                            for (prin = pap->principals; prin; 
                                 prin = prin->next) {
                                if (member(prin->token, acl->principals))
                                    return answer;
                            }
                        }
                    }
                }
                else if (acl->atype && strequal(acl->atype, "P_PASSWORD")) {
                    PAUTH pap;      /* Iteration variable for PAUTH. */
                    TOKEN prin; /* principals */
                    /* Loop through all of the principals. */
                    for (pap = req->auth_info; pap; pap = pap->next) {
                        if (pap->ainfo_type == PFSA_P_PASSWORD) {
                            for (prin = pap->principals; prin; 
                                 prin = prin->next) {
                                if (member(prin->token, acl->principals))
                                    return answer;
                            }
                        }
                    }
                }

                break;
            case ACL_LGROUP: /* Not yet implemented */
                break;
            case ACL_GROUP: /* Not yet implemented */
                break;
            case ACL_TRSTHOST:  /* Check host and userid */
                if (!check_prvport(req))
                    break;
                /* DELIBERATE FALLTHROUGH */
            case ACL_ASRTHOST:  /* Check host and asserted userid */
            {
                PAUTH pap;
                TOKEN prin; /* principals */
                /* Loop through all of the asserted principals. */
                for (pap = req->auth_info; pap; pap = pap->next) {
                    if (pap->ainfo_type == PFSA_UNAUTHENTICATED) {
                        for (prin = pap->principals; prin; 
                             prin = prin->next) {
                            if (check_asserted_principal(prin->token,
                                                         acl->principals,
                                                         req->peer_addr))
                                return answer;
                        }
                    }
                }
            }
                break;
            case ACL_IETF_AAC:
                /* This ACL entry type is not yet implemented */
                break;
            default: /* Not implemented */
                break;
            }
        }
        acl = acl->next;
    }
    return(NOT_AUTHORIZED);
}

/* 
 * check_permissions - Check if operation is authorized
 *
 *           CHECK_PERMISIONS takes a string with letters representing
 *           the permissions required for the current opration, and
 *           a string listing operating authorized by the current ACL
 *           entry.  It returns a 1 if the operation would be
 *           authorized by the current entry, a 0 if not, and a -1
 *           if the ACL entry would have authorized the operation,
 *           but began with a "-" indicating negative authorization.
 *  
 *   ARGS:   op  - String with operations to be performed
 *           p   - Permissions from ACL entry
 *           ld  - Whther ACL entry is for directory or link (or both)
 *
 *   RETURNS: 1 if authorized
 *            0 if not authorized
 *           -1 if negative authorization
 *
 * Protections
 *
 * The more common entry appears first if multiple rights allow an
 * operation.  The operation identifier appears second.
 * 
 * Object  File  Directory    Link*  Meaning
 *   AB     AB       AB        Aa     Administer ACL
 *   VYAB   VYAB     VYAB      VvAa   View ACL
 *   -      -        L         Ll     List link
 *   Rg     RG       RQ        RrQ    Read link, get attribute or file
 *   Wu     Ww       WM        WmM    Modify attribute, data, links
 *   EiWu   EeWw     EIWM      -      Insert attributes links, append (extend)
 *  DzWu   -        DKWM      DdKWMm Delete link or attribute
 *
 * The following will eventually be replaced by restricted forms of
 * Administer (A or B).
 * 
 *   >      >        >         ]      Add rights
 *   <      <        <         [      Remove rights
 *   )      )        )         -      Add rights
 *   (      (        (         -      Remove rights
 *
 * The following only appear on the server maintenance ACL
 * 
 *   S       Restart server
 *   T       Terminate server
 *   U       Update system information
 *   P       Administer passwords
 *   p       Add new password entry
 *
 * A - sign in an ACL means that the specified rights are explicitly
 * denied.  In the table, it means not applicable.
 *
 * * A capital letter on a link ACL means that this right exists in the
 *   direcory ACL for the directory containing the link.
 *
 * ** When restrictions are supported, they can be used to restrict the
 *    specific attributes to which a right applies, or to restrict the
 *    interpretation of an ACL entry to only the Object, File, or Directory,
 *    or link. 
 *
 *  When a small letter is associated with a directory, it is the default
 *  used for those links in the directory which do not otherwise specify
 *  protections.  The large letter for a directory indicates that the 
 *  right applies to ALL entries in the directory, regardless of the ACLs
 *  associated with the individual link.  
 *
 *  These rights apply to the directory and individual links.  The ability
 *  to read a link does not grant any rights to read the file that the
 *  link points to.  Instead, it grants the right to read the binding
 *  of the link.  The protection mechanisms for the objects themselves
 *  depend on the underlying access mechanism.
 *
 *  The Administer right is required to change the ACL.  "A" allows one
 *  to change the ACLs for the directory as a whole, as well as those 
 *  for individual links.  It does not, however, grant any rights to 
 *  the object pointed to by those links (e.g. it doesn't allow one
 *  to change the permissions on subdirectories.
 *
 *  List allows one to list an entry in a wildcarded search.  Read allows
 *  one to learn the binding of a link.  If one can read a link, but
 *  not list it, then it can only be seen when the user specifies the
 *  exact name of the link in a query.
 *
 *  Modify allows one to change the binding of a link.  It does not
 *  allow one to create a new link or delete an existing one.  Insert
 *  or delete access are necessary for that. 
 *
 *  View allows one to view the contents of the ACL.  Administer implies view.
 *  Add rights and remove rights, ><][)(, allow one to add or remove the
 *  other rights that are specified as part of the same ACL entry.  It is
 *  a limited form of administer.  The other rights included in the
 *  same ACL entry do not apply, but only restrict which rights may be
 *  added or removed.  The add or remove indicators must appear in the
 *  first one or two positions in the rights field, and can not themselves
 *  be added or removed unless they also appear later in the rights field.
 *
 *  If the permission string begins with a "-" and if the specified operation
 *  would otherwise be authorized, check_permissions returns -1 indicating 
 *  that an applicable negative right has been found, and that the operation
 *  should be denied even if subsequent ACL entries authorizing it are found.
 *  If an ACL entry preceding this one has already authorized the operation,
 *  the operation will be performed.
 *
 *  BUGS: For now, only the first character in ><][])( means add or
 *        delete the following rights, and all rights included in the
 *        entry including the first ><][)( may be added or deleted.
 *        Eventually, we will check the first two positions to see
 *        if adding and deleting is allowed, and the matching
 *        characters in those positions will be removed before 
 *        checking subsequent characters.
 */
int
static check_permissions(op,p,ld)
    char	*op;	/* Operation                        */
    char	*p;	/* Permissions                      */
    int		ld;     /* 0 =link, 1 =directory, 2=both, 4=misc */
{
    char	*tp = p;
    int	polarity = 1; /* -1 = neg authorization */

    if(!p || !(*p)) return(NOT_AUTHORIZED);

    if(*p == '-') polarity = -1;

    /* Reject if ACL entry for insert or delete rights, but not operation */
    if(index("><][)(",*p) && !index("><][",*op))
        return(NOT_AUTHORIZED);
    /* Insert or delete rights must be first in ACL permissions */
    while(*(++tp)) if(index("><][)(",*tp)) return(NOT_AUTHORIZED);

    while(*op) {
        switch(*(op++)) {

        case 'a':
            if((ld != 1) && index(p,'a')) continue;
            if((ld != 0) && index(p,'A')) continue;
            else return(NOT_AUTHORIZED);
        case 'A': 
        case 'B': 
            if((ld != 0) && index(p,'A')) continue;
            if((ld != 0) && index(p,'B')) continue;
            else return(NOT_AUTHORIZED);

        case 'v':
            if((ld != 1) && index(p,'v')) continue;
            if((ld != 0) && index(p,'V')) continue;
            if((ld != 1) && index(p,'a')) continue;
            if((ld != 0) && index(p,'A')) continue;
            else return(NOT_AUTHORIZED);
        case 'V': 
        case 'Y': 
            if((ld != 0) && index(p,'V')) continue;
            if((ld != 0) && index(p,'Y')) continue;
            if((ld != 0) && index(p,'A')) continue;
	    if((ld != 0) && index(p,'B')) continue;
	    else return(NOT_AUTHORIZED);

        case 'l':
            if((ld != 1) && index(p,'l')) continue;
        case 'L': /* and fall through */
            if((ld != 0) && index(p,'L')) continue;
            else return(NOT_AUTHORIZED);

        case 'r':
            if((ld != 1) && index(p,'r')) continue;
        case 'R': /* and fall through */
        case 'Q': /* and fall through */
            if((ld != 0) && index(p,'R')) continue;
            if((ld != 0) && index(p,'Q')) continue;
            else return(NOT_AUTHORIZED);

        case 'G':
            if((ld != 0) && index(p,'G')) continue;
            if((ld != 0) && index(p,'R')) continue;
            else return(NOT_AUTHORIZED);

        case 'g':
            if((ld != 0) && index(p,'g')) continue;
            if((ld != 0) && index(p,'R')) continue;
            else return(NOT_AUTHORIZED);

        case 'm':
            if((ld != 1) && index(p,'m')) continue;
        case 'M': /* and fall through */
            if((ld != 0) && index(p,'M')) continue;
            if((ld != 0) && index(p,'W')) continue;
            else return(NOT_AUTHORIZED);

        case 'w': /* and fall through */
            if((ld != 0) && index(p,'w')) continue;
            if((ld != 0) && index(p,'W')) continue;
            else return(NOT_AUTHORIZED);

        case 'u': /* and fall through */
            if((ld != 0) && index(p,'u')) continue;
            if((ld != 0) && index(p,'W')) continue;
            else return(NOT_AUTHORIZED);

        case 'I':
            if((ld != 0) && index(p,'I')) continue;
            if((ld != 0) && index(p,'E')) continue;
            if((ld != 0) && index(p,'W')) continue;
            if((ld != 0) && index(p,'M')) continue;
            else return(NOT_AUTHORIZED);

        case 'e':
            if((ld != 0) && index(p,'e')) continue;
            if((ld != 0) && index(p,'E')) continue;
            if((ld != 0) && index(p,'W')) continue;
            if((ld != 0) && index(p,'w')) continue;
            else return(NOT_AUTHORIZED);

        case 'i':
            if((ld != 0) && index(p,'i')) continue;
            if((ld != 0) && index(p,'E')) continue;
            if((ld != 0) && index(p,'W')) continue;
            if((ld != 0) && index(p,'u')) continue;
            else return(NOT_AUTHORIZED);
 
	case 'd':
            if((ld != 1) && index(p,'d')) continue;
            if((ld != 1) && index(p,'m')) continue;
        case 'D': /* and fall through */
            if((ld != 0) && index(p,'D')) continue;
            if((ld != 0) && index(p,'K')) continue;
            if((ld != 0) && index(p,'W')) continue;
            if((ld != 0) && index(p,'M')) continue;
            else return(NOT_AUTHORIZED);

        case 'z': /* and fall through */
            if((ld != 0) && index(p,'D')) continue;
            if((ld != 0) && index(p,'z')) continue;
            if((ld != 0) && index(p,'W')) continue;
            if((ld != 0) && index(p,'u')) continue;
            else return(NOT_AUTHORIZED);

        case ']':
            if((ld != 1) && index(p,']')) continue;
            if((ld != 0) && index(p,'>')) continue;
            else return(NOT_AUTHORIZED);
        case '>': 
            if((ld != 0) && index(p,'>')) continue;
            if((ld != 0) && index(p,')')) continue;
            else return(NOT_AUTHORIZED);

        case '[':
            if((ld != 1) && index(p,'[')) continue;
            if((ld != 0) && index(p,'<')) continue;
            else return(NOT_AUTHORIZED);
        case '<': 
            if((ld != 0) && index(p,'<')) continue;
            if((ld != 0) && index(p,'(')) continue;
            else return(NOT_AUTHORIZED);

            /* Maintenance operations. */
        case 'S':
            if (index(p, 'S')) continue;
            else return NOT_AUTHORIZED;
        case 'T':
            if (index(p, 'T')) continue;
            else return NOT_AUTHORIZED;
        case 'U':
            if (index(p, 'U')) continue;
            else return NOT_AUTHORIZED;
        case 'P':
            if (index(p, 'P')) continue;
            else return NOT_AUTHORIZED;
        case 'p':
            if (index(p, 'p')) continue;
            if (index(p, 'P')) continue;
            else return NOT_AUTHORIZED;
        default:
            return(NOT_AUTHORIZED);
        }
    }
    return(polarity);
}

#ifdef  NOTDEF
/* Unused.  Make this thread-safe before commenting it out. */
static char *inet_ntoa(a)
    struct in_addr a;
    {
	static char	astring[20];

#if BYTE_ORDER == BIG_ENDIAN
	sprintf(astring,"%d.%d.%d.%d",(a.s_addr >> 24) & 0xff,
		(a.s_addr >> 16) & 0xff,(a.s_addr >> 8) & 0xff, 
		a.s_addr & 0xff);
#else
	sprintf(astring,"%d.%d.%d.%d", a.s_addr & 0xff,
		(a.s_addr >> 8) & 0xff,(a.s_addr >> 16) & 0xff, 
		(a.s_addr >> 24) & 0xff);
#endif
	
	return(astring);
    }
#endif NOTDEF

/* Only used locally in a context where it returns static data overwritten by
   each call. */
static char *
inet_def_local(char *s)
{
    AUTOSTAT_CHARPP(adstringp);
    static long	myaddr = 0;      /* look below to see how this is mutexed */ 
    char		intstring[10];
    static long o[4];          /* look below to see how this is mutexed */ 
    char		*ads;
    long		*octet;

    if (!*adstringp) *adstringp = stalloc(50); /* effectively, an array of size
                                                  50.  */
    if (!myaddr) {              
        p_th_mutex_lock(p_th_mutexPSRV_CHECK_ACL_INET_DEF_LOCAL);
        /* Now that we have a mutex, check adstring again, since it might
           have changed since we tested it.  This tests whether another
           initialization successfully completed since we called this. */
        if (!myaddr) {
            long local_myaddr = myaddress();
#if BYTE_ORDER == BIG_ENDIAN
            o[0] = (local_myaddr >> 24) & 0xff;
            o[1] = (local_myaddr >> 16) & 0xff;
            o[2] = (local_myaddr >> 8) & 0xff;
            o[3] = local_myaddr & 0xff;
#else
            o[0] = local_myaddr & 0xff;
            o[1] = (local_myaddr >> 8) & 0xff;
            o[2] = (local_myaddr >> 16) & 0xff;
            o[3] = (local_myaddr >> 24) & 0xff;
#endif
            myaddr = local_myaddr; /* change done; commit. */
        }
        p_th_mutex_unlock(p_th_mutexPSRV_CHECK_ACL_INET_DEF_LOCAL);
    }

    octet = o;
    
    ads = *adstringp;
    while(*s && ((ads - *adstringp) < 40) ) {
        switch(*s) {
        case '.':
            octet++;
            *ads++ = *s;
            break;
        case '%':
            qsprintf(intstring,sizeof intstring, "%d",*octet);
            bcopy(intstring,ads,strlen(intstring));
            ads += strlen(intstring);
            break;
        default:
            *ads++ = *s;
            break;
        }
        s++;
    }
    *ads++ = '\0';

    return(*adstringp);

}

/* Are we logged into a privileged port?  1 if yes, zero if no. */
int
check_prvport(req)
    RREQ req;
{
    return PEER_PORT(req) <= IPPORT_RESERVED;
}

/*
 * check_asserted_principal - check for membership in list of principals
 * and check optional host ID.  The principals have the format used by ASRTHOST
 * and TRSTHOST.
 */
static int
check_asserted_principal(username,principals, hostaddr)
    char username[];
    struct in_addr hostaddr;
    TOKEN principals;
{
    for(; principals; principals = principals->next) {
        char	*host;

        /* principal specified in the ACL entry.  We copy it because we can't
           modify the principals structure, since the principals structure
           might contain strings that are in readonly memory. */
        AUTOSTAT_CHARPP(aclprinp);
        *aclprinp = stcopyr(principals->token, *aclprinp);

        host = strchr(*aclprinp,'@');
        if(host == NULL) {
            if(wcmatch(username,*aclprinp)) return(TRUE);
            else continue;
        }
        *(host++) = '\0';

        if(!wcmatch(username,*aclprinp)) continue;

        if(index("%123456789.",*host)) { /* Host address */
            if(wcmatch(inet_ntoa(hostaddr),inet_def_local(host)))
                return(TRUE);
            continue;
        }
        else if(*host == '0') { /* Host address with leading 0 */
            if(wcmatch(inet_ntoa(hostaddr),inet_def_local(host+1)))
                return(TRUE);
            continue;
        }
        else { /* Host name - not implemented */
            continue;
        }
    }
    return(FALSE);
}

ACL get_container_acl(char *path)
{
    PFILE	container;
    ACL		cont_acl = NULL;
    ACL		tacl = NULL;    
    ACL		tacl2 = NULL;
    char	cpath[MAXPATHLEN];
    char	*slash;

    strcpy(cpath,path);
    slash = strrchr(cpath,'/');

    if(slash) {
	*slash = '\0';
	container = pfalloc();
	dsrfinfo(cpath,0,container);
	cont_acl = container->oacl; container->oacl = NULL;
	pffree(container);
	tacl = cont_acl;
	while(tacl) {
	    if(tacl->acetype == ACL_CONTAINER) {
		tacl2 = tacl;
		tacl = tacl->next;
		EXTRACT_ITEM(tacl2,cont_acl);
		acfree(tacl2);
	    }
	    else tacl = tacl->next;
	}
	return(cont_acl);
    }
    return(NULL);
}
	
