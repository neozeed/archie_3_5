/* This was the main function for the Prospero program vcache.
 * return with status 0 if file retrieval was successful; nonzero if it was
 * not.  This is called only by p__map_cache(), in lib/pcompat/pmap_cache.c and
 * by VGET in user/vget.c.
 *
 * The memory leaks have probably all been nailed by Mitra by now.
 */

/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <stdio.h>
#include <sys/param.h>
#include <errno.h>

#include <pmachine.h>		/*SCOUNIX needs sys/types.h*/
                                /* SOLARIS needs prototype for tempnam() */

/* #include "ftp_var.h"		/* Nasty - doesnt declare these external */
extern int debug;
extern int trace;
extern int anonlogin;

#include "vcache.h"
#include <pfs.h>
#include <psite.h>              /* for P_CACHE_ENABLED, among other
                                   definitions.  */
#include <perrno.h>

/* A lot of this code memory leaks - not a problem since it exits - but
   a pain if going to incorporate inside anything else. */

/* Benjamin Britt wrote routines enabling VCACHE to retrieve files 
   using WAIS.  These are not yet very useful, since modifications
   need to be made to user/vget.c and lib/pfs/pget_am.c.  Therefore, they are
   commented out.  #define WAIS and change the definitions of WAIS_LIBS and
   WAIS_TARGETS in the Makefile in order to link them in, if you wish to
   experiment with them and continue developing them.  You will also need the
   directory user/vcache/wais, which is not part of the standard distribution,
   but will be sent on request.

   #define WAIS
*/

#include "vcache_macros.h"

int		cache_verbose = 0;
#define verbose cache_verbose

static int
prospero_contents_get(VLINK vl, char *local);

char *old_err_string = NULL;

void
vcache2_init()
{
    debug = 0;
    trace = 0;
}

/*
 * If called with the MANAGE argument set, will manage the cache (if compiled
 * for cache management); otherwise won't.  Also, won't
 * This is an exported interface in Mitra's use of the Prospero library.
 * Not an exported interface otherwise.
 *
 * Returns 0 on success, -1 on FAILURE.
 */
int
vcache2a(char *host, char *remote, char *local, char *method, 
		char *argv[], int manage_cache)
{
#ifndef P_CACHE_ENABLED
    p__mkdirs(local, FALSE);
    return vc_get_file_A(host,remote,local,method,argv);
#else
    /* Add code here to manage the cache */
    /* This code implements the algorithm ...
       if (fetch to P_CACHE_TMP)
       then mv from P_CACHE_TMP to local, overwriting
       if local exists, then success, else failure
       NOTE this means it wont default to return a file that exists already
       but if the retrieval fails and the file exists then you'll get it
       - If someone doesnt like this behaviour, then change it with options.
       */

    char 	*tempfile = NULL;
    int	retval;


    char 	*tempfile = NULL;
    if (!manage_cache) {
        p__mkdirs(local, FALSE);
        return vc_get_file_A(host,remote,local,method,argv);
    } 

    if (p__mkdirs(P_CACHE_TMP,TRUE)) {
            /* p_err_string already set */
            RETURN(-1);
    }
    tempfile = tempnam(P_CACHE_TMP,NULL);
    
    if ((retval = vc_get_file_A(host,remote,tempfile,method,argv)) 
        == 0) {
        /* Succeeded in fetching file */
        if (file_incache(local))  {
            if (unlink(local)) {
                ERRSYS("Cant unlink existing %s %s %s",local);
                RETURN(-1);
            }
        }
        if (p__mkdirs(local,FALSE)) {
            /* p_err_string already set */
            RETURN(-1);
        }
        if (renameOrCopyAndDelete(tempfile,local)) {
            /* p_err_string set */
            RETURN(-1);
        }
        RETURN(0);
    } else {
        /* Couldnt retrieve, maybe in the cache already */
        if (file_incache(local))
            RETURN(0);
        RETURN(-1);
    }
    assert(0); /* Unreached */
cleanup:
    free(tempfile);
    return(retval);
#endif
}

/*
 * Retrieve the file with the HSONAME REMOTE from the computer HOST.
 * Retrieve it to the file LOCAL using access-method METHOD.
 * Arguments to the access method in argv.
 */
int
vc_get_file_A(char *host, char *remote, char *local, char *method, 
		char *argv[])
{
    int	retval = 0;
    char	*slash;
    int         argc;

    /* Set ARGC appropriately. */
    for (argc = 0; argv[argc]; ++argc)
        ;

    /* If no method provided, then we are done */
    if(!method) return(0);

    /* Make the directory to include the cached copy */
    /* if it does not already exist                  */
    if (p__mkdirs(local,FALSE)) {
	/* p_err_string already set */
        return(1);
    }

    anonlogin = strequal(method,"AFTP");
    if(anonlogin || strcmp(method,"FTP") == 0) {
        char	*trans_mode;
        if(argc != 1) {
	    ERR( "vcache: wrong number of arguments for %s%s", method);
            return(1);
        }
        trans_mode = argv[0];
        if(strcmp(trans_mode,"DIRECTORY") == 0) {
            ERR("File is the directory %s on the host %s which may not be running Prospero%s",
		 remote,host);
          return(1);
        }
	/* Note "aftpget" used to do a exit*/
        return(aftpget(host,local,remote,trans_mode));
    } else if (strequal(method, "PROSPERO-CONTENTS")) {
        VLINK vl = vlalloc();
        if(argc != 0) {
            ERR("vcache: wrong number of arguments for %s%s", method);
	    vlfree(vl);
            return(1);
        }
        vl->host = stcopyr(host, vl->host);
        vl->hsoname = stcopyr(remote, vl->hsoname);
        retval = prospero_contents_get(vl, local);
	vlfree(vl);
        if(retval) return(1);
        else return(0);
    } else if (strcmp(method, "GOPHER") == 0) {
        extern char *strchr();
        /* Remote file name is Gopher selector string. */
        /* port # included in the hostname. */
        /* Check validity of the argument. */
        register char *cp = strchr(host, '(');
        int gopherport;
        int gophertype;

        if (argc != 1) {
                ERR("vcache: The GOPHER access method expects \
one additional argument.  Got %d arguments.%s", argc);
            return(1);
        }
        if (cp == NULL) {
                ERR("vcache: The GOPHER access method requires \
that the specified hostname have a port number included.  Got the hostname \
%s.%s", host);
            return(1);
        }
        *cp++ = '\0';             /* terminate the hostname normally. 
                                     cp now points to the port start. */
        gopherport = atoi(cp);
        
        if (strlen(argv[0]) == 1) {
            gophertype = *argv[0];
        } else if (strequal(argv[0], "BINARY")) {
            gophertype = '9';   /* generic binary type */
        } else if (strequal(argv[0], "TEXT")) {
            gophertype = '0';   /* generic text type */
        } else {
                ERR("Malformed argument to GOPHER access method.%s");
            return(1);
        }
        if(retval = gopherget(host,local,remote,gopherport, gophertype))
            return(1);
        else
            return(0);
    } if (strcmp(method,"WAIS") == 0) {

        /* Note that the remote file name is actually the docid */
        retval = waisRetrieveFileByHsoname(local,remote);
	return(retval ? 1 : 0 );
    } else {
       ERR("vcache: access method (%s) not supported.%s",method);
       return(1);
    }
}

static int
prospero_contents_get(VLINK vl, char *local)
{
    PATTRIB at = NULL;
    FILE *local_file = NULL;    /* file pointer for local destination */ 
    int retval = PSUCCESS;      /* return from function */
    int need_newline = 0;       /* set to 1 if data didn't end with a newline
                                   */ 
    /* Seek multiple instances of the attribute.  */
    
    for (at = pget_at(vl,"CONTENTS"); at; at = at->next) {
        TOKEN seq = at->value.sequence;
        if (strequal(at->aname, "CONTENTS") && at->avtype == ATR_SEQUENCE &&
            ((length(seq) >= 2 
                && strequal(seq->token, "DATA"))
             || (length(seq) >= 3 
                 && strequal(seq->token, "TAGGED")))) {
            /* Found one.  Open the file */
            if (!local_file) {
                local_file = fopen(local, "w");
                if (local_file == NULL) {
                    ERRSYS ( "vcache: Couldn't create the local file %s: %s", 
                                local);
                    RETURN(PFAILURE);
                }
            }
        }
        /* So, we now have a file open for output and something to put in it.
           */ 
        if (need_newline) {
            putc('\n', local_file);
            need_newline = 0;
        }
        if (strequal(elt(seq, 0), "DATA")) {
	    p__fputbst(elt(seq, 1), local_file);
        } else if (strequal(elt(seq, 0), "TAGGED")) {
            char *s;

	     p__fputbst(elt(seq, 1), local_file);
            fputs(": ", local_file);
	    need_newline = p__fputbst(elt(seq, 2), local_file);
        } else 
            internal_error("Shouldn't get here with unrecognized attribute \
0th element.");
    }
    if (!local_file) {
        ERR("vcache: Couldn't get remote object's CONTENTS attribute.%s");
        RETURN(PFAILURE);
    }
    if (ferror(local_file)) retval = PFAILURE;
  cleanup:
    if (local_file) { if (fclose(local_file)) { retval = PFAILURE; } }
    if (at) atfree(at);
    return(retval);
}

