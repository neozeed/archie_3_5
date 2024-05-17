/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <sys/types.h>          /* Needed by SCO; doesn't hurt others. */
#include <sys/param.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>

#include <pmachine.h>


#include <pserver.h>

#ifdef SHARED_PREFIXES          /* if no SHARED_PREFIXES definition, this is
                                   useless. */
#include <pfs.h>
#include <plog.h>               /* log errors in initializing. */

/* isascii() is not provided in POSIX, so we just do it here. */
#ifndef isascii
#define isascii(c)	((unsigned)(c)<=0177)
#endif

extern char	*hostname;

/*
 * check_localpath - Check whether file is available via the LOCAL access
 * method. 
 *
 * 	  CHECK_LOCALPATH takes the name of a file and a network
 *        address.  If possible, it returns a string which is the name for the
 *        file in the client's filesystem.
 *
 *    ARGS: path    - Name of file to be retrieved
 *          client  - IP address of the client
 *
 * RETURNS: A character pointer to a static buffer containing the new name
 *          of the file in the client's namespace, or NULL if the file is not
 *          available to the client by the LOCAL access method. 
 *
 *   NOTES: The returned buffer will be reused on the next call.
 */

static struct shared_prefixes {
    char **locals;          /* array of char *; null terminated */
    char *remote;           /* string, or NULL indicating not exported. */
    /* A network byte order list of ip addresses of clients for which this
       entry is meaningful. */
    long *includes;          /* 0L terminated; octet w/255 means wildcard */
    /* A network byte order list of ip addresses of clients for which this
       entry is NOT meaningful.  Overrides includes.  Default (if no match in
       either includes or exceptions) is not to include. */
    long *exceptions;        /* 0L terminated; octet w/255 means wildcard */
} *shared_prefixes = NULL;

extern void p_init_shared_prefixes(void);

char *
check_localpath(char *path, long client)
{
    struct shared_prefixes *spp;
    AUTOSTAT_CHARPP(rempathp); /* remapped path that we return */
    char **locp;                 /* pointer to locals */
    long *ipaddrp;              /* pointer to IP addresses */


#ifndef PFS_THREADS
    if (!shared_prefixes) p_init_shared_prefixes();
#else
    assert(shared_prefixes);    /* must have been called from dirsrv */
#endif

    /* Try to match local path */
    for (spp = shared_prefixes; spp->locals; ++spp) 
        for (locp = spp->locals; *locp; ++locp)
            if (strnequal(*locp, path, p__bstsize(*locp) - 1))
                goto prefixmatch;          /* prefix match */

 prefixmatch:
    if (!spp->locals) return NULL; /* no match */
    /* Now make sure one of the includes addresses matches. */
    for (ipaddrp = spp->includes; *ipaddrp; ++ipaddrp) {
        if ((((*ipaddrp & 0xff000000) == 0xff000000) 
             || ((*ipaddrp & 0xff000000) == (client & 0xff000000)))
            && (((*ipaddrp & 0x00ff0000) == 0x00ff0000) 
             || ((*ipaddrp & 0x00ff0000) == (client & 0x00ff0000)))
            && (((*ipaddrp & 0x0000ff00) == 0x0000ff00) 
             || ((*ipaddrp & 0x0000ff00) == (client & 0x0000ff00)))
            && (((*ipaddrp & 0x000000ff) == 0x000000ff) 
             || ((*ipaddrp & 0x000000ff) == (client & 0x000000ff))))
            break;
    }
    if (*ipaddrp == 0L) /* no matches */
        return NULL;            
    /* Now make sure none of the exceptions addresses matches. */
    for (ipaddrp = spp->exceptions; *ipaddrp; ++ipaddrp) {
        if ((((*ipaddrp & 0xff000000) == 0xff000000) 
             || ((*ipaddrp & 0xff000000) == (client & 0xff000000)))
            && (((*ipaddrp & 0x00ff0000) == 0x00ff0000) 
             || ((*ipaddrp & 0x00ff0000) == (client & 0x00ff0000)))
            && (((*ipaddrp & 0x0000ff00) == 0x0000ff00) 
             || ((*ipaddrp & 0x0000ff00) == (client & 0x0000ff00)))
            && (((*ipaddrp & 0x000000ff) == 0x000000ff) 
             || ((*ipaddrp & 0x000000ff) == (client & 0x000000ff))))
            return NULL;        /* found a match */
    }
    /*  We have a match.  Map it! */
    *rempathp = qsprintf_stcopyr(*rempathp, "%s%s", spp->remote,
                                 path + strlen(*locp));
    return *rempathp;
}

static long wcinet_aton(char *ascaddr);

/* Now called from dirsrv() upon initialization, so that we don't have to
   initialize it automatically in multithreaded case. */
void
p_init_shared_prefixes(void)
{
    char **cp;
    int numnulls = 0;
    int numentries;             /* # of entries in the shared_prefixes array */
    int centry;                     /* index of entry being initialized */
    static char *init_sd[] = SHARED_PREFIXES;
    int i;      /* I is just a reused index variable; value is also used right
                   after a loop closes, in an assertion check. */


    /* Count the # of entries */
    for (i = 0; i < sizeof init_sd / sizeof init_sd[0]; ++i)
        if (init_sd[i] == NULL) ++numnulls;
    /* More efficient version of: if ((numnulls % 4) != 0) */
    if (numnulls & 0x3) {
        plog(L_DATA_FRM_ERR, NOREQ,
             "Incorrectly formatted SHARED_PREFIXES entry in \
the pserver.h configuration file; got %d excess NULLs (out of %d total).  The number \
of NULLs will always be an even multiple of 4 in a correctly formatted \
SHARED_PREFIXES configuration file.", numnulls & 0x3, numnulls);
        plog(L_DATA_FRM_ERR, NOREQ,
             "Please correct this.  In the mean time, this server will \
muddle on as best it can.");
    }
    numentries = (numnulls >> 2);
    /* Allocate one extra as a sentinel marking end of the array. */
    shared_prefixes = 
        (struct shared_prefixes *) 
            stalloc(sizeof *shared_prefixes * (numentries + 1));
    shared_prefixes[numentries].locals = NULL;
    /* finish initializing */
    for (centry = 0, cp = init_sd; centry < numentries; ++centry) {
        int subarraynelem;
        /* continue to use I above as an index. */

        /* count # of entries for locals */
        for (subarraynelem = 0; cp[subarraynelem]; ++subarraynelem)
            ;
        shared_prefixes[centry].locals = 
            (char **) stalloc((1 + subarraynelem) * sizeof (char *));
        i = 0;
        for(;*cp; ++cp)
            shared_prefixes[centry].locals[i++] = stcopy(*cp);
        shared_prefixes[centry].locals[i] = NULL;
        assert(i == subarraynelem); /* sanity check */
        ++cp;                   /* onto remote entry */
        if (*cp) {
            shared_prefixes[centry].remote = stcopy(*cp);
            if (*++cp) {
                plog(L_DATA_FRM_ERR, NOREQ,
                     "Incorrectly formatted SHARED_PREFIXES entry in \
pserver.h configuration file: only one remote prefix should be specified per \
entry.  This occurred while scanning the %dth entry.", centry);
                plog(L_DATA_FRM_ERR, NOREQ,
                     "Please correct this.  In the mean time, this \
server will muddle on as best it can.");
            while (*++cp)       /* skip irrelevant entries. */
                ;
            }
        }
        else 
            shared_prefixes[centry].remote = NULL;
        ++cp;                   /* onto include internet addrs. */

         /* count # of entries for includes */
        for (subarraynelem = 0; cp[subarraynelem]; ++subarraynelem)
            ;
        shared_prefixes[centry].includes = 
            (long *) stalloc((1 + subarraynelem) * sizeof (long));
        for(i = 0; *cp; ++cp, ++i)
            shared_prefixes[centry].includes[i] = wcinet_aton(*cp);
        shared_prefixes[centry].includes[i] = 0L;
        assert(i == subarraynelem); /* sanity check */
        ++cp;                   /* onto exceptions entry */
         /* count # of entries for exceptions */
        for (subarraynelem = 0; cp[subarraynelem]; ++subarraynelem)
            ;
        shared_prefixes[centry].exceptions = 
            (long *) stalloc((1 + subarraynelem) * sizeof (long));
        for(i = 0; *cp; ++cp, ++i)
            shared_prefixes[centry].exceptions[i] = wcinet_aton(*cp);
        shared_prefixes[centry].exceptions[i] = 0L;
        assert(i == subarraynelem); /* sanity check */
        ++cp;                   /* onto next entry, if there is one. */
    }
}


/* If there are any problems with the format, return 127.0.0.1, which will
   never be useful, since the local AM code will already snag it.  Yaay! */
#if BYTE_ORDER == BIG_ENDIAN
#define LOOPBACK_ADDR  0x7f000001
/* octets are numbered 'a' for leftmost in ascii form, 'b', 'c', and 'd' for
   rightmost. */ 
#define octetat(octetnum, value)  ((value & 0xff) << ((3 - octetnum) * 8))
#else
#define LOOPBACK_ADDR  0x0100007f
#define octetat(octetnum, value)  ((value & 0xff) << (octetnum * 8))
#endif


/* This icky function takes an internet address with exactly 4 octets in it,
 * separated by 3 dots.  The octets may be exactly one of:
 *  a) decimal numbers between 0 and 255
 *  b) * -- indicates wildcard
 *  c) % -- indicates address of current host. 
 */
static long
wcinet_aton(char *asc_addr)
{
    char workaddr[30];          /* working copy of ascii address. */
    long retval = 0L;
    char *thisp, *nextp;
    int octetval;
    
    if (strlen(asc_addr) >= sizeof workaddr)
        goto malformed;
    strcpy(workaddr, asc_addr);  /* leave asc_addr for error reporting */

    
    thisp = nextp = workaddr;
    nextp = strchr(nextp, '.');
    if (!nextp) goto malformed;
    *nextp = '\0';
    ++nextp;
    if ((octetval = parse_octet(0, thisp)) < 0)
        goto malformed;
    retval |= octetat(0, octetval);
    thisp = nextp;
    nextp = strchr(nextp, '.');
    if (!nextp) goto malformed;
    *nextp = '\0';
    ++nextp;
    if ((octetval = parse_octet(1, thisp)) < 0)
        goto malformed;
    retval |= octetat(1, octetval);
    thisp = nextp;
    nextp = strchr(nextp, '.');
    if (!nextp) goto malformed;
    *nextp = '\0';
    ++nextp;
    if ((octetval = parse_octet(2, thisp)) < 0)
        goto malformed;
    retval |= octetat(2, octetval);
    thisp = nextp;
    if ((octetval = parse_octet(3, thisp)) < 0)
        goto malformed;
    retval |= octetat(3, octetval);
    return retval;

 malformed:
    plog(L_DATA_FRM_ERR, NOREQ, "Malformed internet address in \
SHARED_PREFIXES initializer: %s\n", asc_addr);
    return INADDR_LOOPBACK;     /* in <netinet/in.h> */
}


/* 0 to 255, or neg. value for failure/malformed input. */
int
parse_octet(int octetnum, char *asc_octet)
{
    static long myaddr = 0L;    /* safe; updated atomically  (in one instr.) */
    int retval;
    if (!myaddr) myaddr = myaddress();      /* myaddress() is now safe. 
                                             Atomic; can't overwrite*/
    if (strequal(asc_octet, "*")) return 255;
    if (strequal(asc_octet, "%")) {
#if BYTE_ORDER == BIG_ENDIAN
        return (myaddr & (0xff000000 >> (octetnum * 8)))
            >> ((3 - octetnum) * 8);
#else
        return  (myaddr & (0xff << (octetnum * 8))) >> (octetnum * 8);
#endif
    } 
    retval = atoi(asc_octet);
    for (;*asc_octet; ++asc_octet)
        if (!isascii(*asc_octet) ||  !isdigit(*asc_octet))
            return -1;
    return retval;
}
#endif /* SHARED_PREFIXES */
