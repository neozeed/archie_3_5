/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>

#include <netdb.h>

#include <pfs.h>
#include <psrv.h>        /* For dsrfinfo */
#include <perrno.h>

VLINK	check_fwd();

extern char		hostname[];

/* retrieve_fp will retrieve a forwarding pointer and will 
 * replace the changed fields of the link it is passed.
 * Retrieve will use pget_at to retriev the forwarding
 * pointer if it is on a remote host.  If the same host,
 * it will retrieve the forwarding pointer directly.
 * Retrieve fp returs PSUCCESS on success and PFAILURE
 * on failure.
 */
#define RETURN(val) { retval = (val) ; goto  cleanup; }
int
retrieve_fp(l)
    VLINK	l;
{
    struct hostent	*h_ent;

    static int	firsttime = 0;
    static char	thishost[100];
    char		lhost[100];

    PATTRIB 	fa = NULL;          /* Forward attribute    */
    VLINK		fp;          /* Forwarding Pointer   */
    PFILE		fi = pfalloc(); /* Pointer to fi_st     */

    int		tmp;
    int			retval;

    assert(P_IS_THIS_THREAD_MASTER()); /* not yet converted. Not used currently
                                          either. gethostbyname MT-Unsafe  */
    if(firsttime++ == 0) {
        h_ent = gethostbyname(hostname);
        strcpy(thishost,h_ent->h_name);
    }

    /* We should check port numbers too */

    /* Find out if link is on this host */
    h_ent = gethostbyname(l->host);
    if(h_ent) strcpy(lhost,h_ent->h_name);
    else RETURNPFAILURE;
    if(strcmp(lhost,thishost) == 0) { /* Local */
        tmp = dsrfinfo(l->hsoname,l->f_magic_no,fi);
        if((tmp == DSRFINFO_FORWARDED) && 
           (fp = check_fwd(fi->forward,l->hsoname,l->f_magic_no))) {
            l->hosttype = stcopyr(fp->hosttype,l->hosttype);
            l->host = stcopyr(fp->host,l->host);
            l->hsonametype = stcopyr(fp->hsonametype,l->hsonametype);
            l->hsoname = stcopyr(fp->hsoname,l->hsoname);
            l->version = fp->version;
            l->f_magic_no = fp->f_magic_no;
            RETURN(PSUCCESS);
        }
        RETURN(PFAILURE);
    }
    else { /* Remote */    
        /* fa could memory leak if pget_at succeeds, but its not ATR_LINK*/
        fa = pget_at(l,"FORWARDING-POINTER");
        if(fa && fa->avtype == ATR_LINK && fa->value.link) {
            fp = fa->value.link;
            l->hosttype = stcopyr(fp->hosttype,l->hosttype);
            l->host = stcopyr(fp->host,l->host);
            l->hsonametype = stcopyr(fp->hsonametype,l->hsonametype);
            l->hsoname = stcopyr(fp->hsoname,l->hsoname);
            l->version = fp->version;
            l->f_magic_no = fp->f_magic_no;
            RETURN(PSUCCESS);
        }
        else RETURN(PFAILURE);
    }
cleanup:
	pffree(fi);
            if (fa) atlfree(fa);
	return(retval);
}


