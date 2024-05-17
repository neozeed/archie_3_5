/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1991, 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>
#include <stdio.h>
#include <string.h>

#include <pfs.h>
#include <perrno.h>
#include <pmachine.h>

extern int	pfs_debug;
static PATTRIB expand_amat(PATTRIB ca, VLINK vl);
static int match_am(PATTRIB list, TOKEN *ainfop, int methods);

/* Returns the chosen access method, or zero if failure. 
   We do NOT modify link, unless the object that is the target oflink was
   forwarded, in which case vl may be returned updated. */
/* This interface is suboptimal, since it doesn't allow for retries in case
   one of the access methods fails. */
int
pget_am(vl,ainfop,methods)
    VLINK	vl;
    TOKEN	*ainfop;         /* This will be filled in with access info
                                   data.  */
    int		methods;        /* Methods this client supports */
{
    PATTRIB     hits = NULL;    /* list of the hits. */
    PATTRIB     ats;            /* attributes returned by pget_at() */
    PATTRIB     ca;             /* index variable */
    int retval;

    perrno = PSUCCESS;
    if(strcmp(vl->target,"NULL") == 0) 
        return(0);
    *ainfop = (TOKEN) NULL;
    
    /* For an EXTERNAL link, the access method should be somewhere on
       the lattrib member.   This is guaranteed in the protocol.  Look on the
       lattrib member in any case, so that we avoid going over the network for
       the ACCESS-METHOD attribute if it was already cached or otherwise
       available (perhaps it was returned as an OBJECT attribute). */
    for (ca = vl->lattrib; ca; ca = ca->next) {
        if (strequal(ca->aname, "ACCESS-METHOD")) {
            PATTRIB amat = expand_amat(ca, vl);
            if (amat)       /* if the amat was correctly formed */
                APPEND_ITEM(amat, hits);
        }
    }
    retval = match_am(hits, ainfop, methods);
    if (retval) return retval;   /* found a match. */

    /* If no match found on the link's attribute list try to retrieve the
       ACCESS-METHOD attribute directly from the object. 
       pget_at() will always return NULL for EXTERNAL links; we take advantage
       of this. */
    hits = NULL;                /* no hits for this try. */
    ats = pget_at(vl, "ACCESS-METHOD");
    for (ca = ats; ca; ca = ca->next)
        if (strequal(ca->aname, "ACCESS-METHOD")) {
            PATTRIB amat = expand_amat(ca, vl);
            if (amat)       /* if the amat was correctly formed */
                APPEND_ITEM(amat, hits);
        }
    atlfree(ats);
    if (!hits) 
	p_err_string = qsprintf_stcopyr(p_err_string, "No ACCESS-METHOD");
    return match_am(hits, ainfop, methods);
}

static void wrong_length(PATTRIB ca);

/* Takes responsibility for freeing the list of attributes passed to it. */
/* Returns a matching access method found on the list, or zero if failure. */
static int
match_am(PATTRIB hits, TOKEN *ainfop, int methods)
{
    PATTRIB ca;
    /* Now, go through each of the hits, if any.  */
    for (ca = hits; ca; ca = ca->next) {
        /* First element of the sequence is the name of the access method. */
        
        if (!ca->value.sequence) {
            wrong_length(ca);     /* Record that it was malformed */
            continue;
        }
        if((methods & P_AM_LOCAL) 
           && (strequal(ca->value.sequence->token,"LOCAL"))) {
            if (length(ca->value.sequence) != 5) {
                wrong_length(ca);
                continue;
            }
            *ainfop = ca->value.sequence;
            ca->value.sequence = (TOKEN) NULL;
            atlfree(hits);
            return(P_AM_LOCAL);
        }
        if((methods & P_AM_NFS) 
           && (strequal(ca->value.sequence->token,"NFS"))) {
            if (length(ca->value.sequence) != 6) {
                wrong_length(ca);
                continue;
            }
            *ainfop = ca->value.sequence;
            ca->value.sequence = (TOKEN) NULL;
            atlfree(hits);
            return(P_AM_NFS);
        }
        if((methods & P_AM_PROSPERO_CONTENTS) 
           && (strequal(ca->value.sequence->token,"PROSPERO-CONTENTS"))) {
            if (length(ca->value.sequence) != 5) {
                wrong_length(ca);
                continue;
            }
            *ainfop = ca->value.sequence;
            ca->value.sequence = (TOKEN) NULL;
            atlfree(hits);
            return(P_AM_PROSPERO_CONTENTS);
        }
        if((methods & P_AM_WAIS) 
           && (strequal(ca->value.sequence->token,"WAIS"))) {
            if (length(ca->value.sequence) != 5) {
                wrong_length(ca);
                continue;
            }
            *ainfop = ca->value.sequence;
            ca->value.sequence = (TOKEN) NULL;
            atlfree(hits);
            return(P_AM_WAIS);
        }
        if((methods & P_AM_TELNET)
           && (strequal(ca->value.sequence->token,"TELNET"))) {
            if (length(ca->value.sequence) < 5) {
                wrong_length(ca);
                continue;
            }
            *ainfop = ca->value.sequence;
            ca->value.sequence = (TOKEN) NULL;
            atlfree(hits);
            return(P_AM_TELNET);
        }
        if((methods & P_AM_AFS) 
           && (strequal(ca->value.sequence->token,"AFS"))) {
            if (length(ca->value.sequence) != 5) {
                wrong_length(ca);
                continue;
            }
            *ainfop = ca->value.sequence;
            ca->value.sequence = (TOKEN) NULL;
            atlfree(hits);
            return(P_AM_AFS);
        }
        if((methods & P_AM_AFTP) 
           && (strequal(ca->value.sequence->token,"AFTP"))) {
            if (length(ca->value.sequence) != 6) {
                wrong_length(ca);
                continue;
            }
            *ainfop = ca->value.sequence;
            ca->value.sequence = (TOKEN) NULL;
            atlfree(hits);
            return(P_AM_AFTP);
        }
        if((methods & P_AM_GOPHER) 
           && (strequal(ca->value.sequence->token,"GOPHER"))) {
            int len = length(ca->value.sequence);
            if (len != 5 && len != 6) {
                wrong_length(ca);
                continue;
            }
            *ainfop = ca->value.sequence;
            ca->value.sequence = (TOKEN) NULL;
            if (len == 5) {
                /* Canonicalize 5-token format to six-token format internally.
                   */ 
                TOKEN tmp = tkalloc(" ");
                *(tmp->token) = *elt(*ainfop, 4);
                APPEND_ITEM(tmp, *ainfop);
            }                
            atlfree(hits);
            return(P_AM_GOPHER);
        }
        if((methods & P_AM_RCP) 
           && (strequal(ca->value.sequence->token,"RCP"))) {
            if (length(ca->value.sequence) != 5) {
                wrong_length(ca);
                continue;
            }
            *ainfop = ca->value.sequence;
            ca->value.sequence = (TOKEN) NULL;
            atlfree(hits);
            return(P_AM_RCP);
        }
        /* The FTP method requires prompting for a password, which is not
           transparent to the user.  Thus, it is the least desirable method,
           from our point of view. */
        if((methods & P_AM_FTP) 
           && (strequal(ca->value.sequence->token,"FTP"))) {            
            if (length(ca->value.sequence) != 6) {
                wrong_length(ca);
                continue;
            }
            *ainfop = ca->value.sequence;
            ca->value.sequence = (TOKEN) NULL;
            atlfree(hits);
            return(P_AM_FTP);
        }
    }
    atlfree(hits);
    *ainfop = NULL;
    return 0;
}


/* Inform the user of a problem if pfs_debug is set. */
static void
wrong_length(PATTRIB ca)
{
    p_warn_string = qsprintf_stcopyr(p_warn_string,
             "Parse error: the server returned ACCESS-METHOD %s%swith %d \
elements; not what was expected by pget_am()!", 
             length(ca->value.sequence) > 0 ? ca->value.sequence->token : "",
             length(ca->value.sequence) > 0 ? " " : "",
             length(ca->value.sequence));
    if (pfs_debug) {
        fputs(p_warn_string, stderr);
        fputc('\n', stderr);
    }
    pwarn = PWARNING;
}


/* Copy an attribute and expand it. */
static PATTRIB 
expand_amat(PATTRIB ca, VLINK vl)
{
    TOKEN tk;

    if (length(ca->value.sequence) < 5) return NULL; /* malformed. */
    ca = atcopy(ca);            /* don't modify the original. */
    /* atcopy has been fixed to do this properly! - Mitra*/
#ifdef NEVERDEFINED
    ca->value.sequence = tkcopy(ca->value.sequence); /* copy the data too! */
#endif
    tk = ca->value.sequence;

    tk = tk->next;              /* skip access method name */
    if(*tk->token == '\0') tk->token = stcopyr(vl->hosttype, tk->token);
    tk = tk->next;              /* go on */
    if(*tk->token == '\0') tk->token = stcopyr(vl->host, tk->token);
    tk = tk->next;              /* go on */
    if(*tk->token == '\0') tk->token = stcopyr(vl->hsonametype, tk->token);
    tk = tk->next;              /* go on */
    if(*tk->token == '\0') tk->token = stcopyr(vl->hsoname, tk->token);
    return ca;
}
