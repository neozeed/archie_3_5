/* This is the main function for the Prospero program vcache.
   Exit with status 0 if file retrieval was successful; nonzero if it was not. 
   This is called only by p__map_cache(), in lib/pcompat/pmap_cache.c and by
   VGET in user/vget.c
*/

/*
 * Copyright (c) 1991, 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <pfs.h>
#include <pcompat.h>
#include <perrno.h>

/* Variables declared in ftp_var.h */
extern int verbose;
extern int options;
extern int debug;
extern int trace;

/* Variables declared in vcache.c*/
extern int cache_verbose;


int		pfs_debug = 0;

void
main(argc, argv)
    int	argc;
    char *argv[];
{
	int retval;

	pfs_enable = PMAP_DISABLE;
	if (retval = vcache1(argc,argv))
		fprintf(stderr,"%s\n",p_err_string);
	exit(retval);
}

/* This next routine could go in the library as well, if anything wants
   to set options this way */
int
vcache1(argc, argv)
    int	argc;
    char *argv[];
{
    int	manage_cache = 0;       /* Must be set to manage the cache. */
    int	replace = 0;		/* set, but never used */
    int	flush = 0;		/* set, but never used */

    char	*host;          /* remote host */
    char	*remote;        /* remote filename */
    char	*local;         /* local filename */
    char	*method;	/* access method */
    int		retval;


    vcache2_init();

    p_initialize("VCACHE",0,NULL);

    argc--;argv++;

    while (argc > 0 && *argv[0] == '-') {
        switch (*(argv[0]+1)) {

        case 'D':
            pfs_debug = 1; /* Default debug level */
            verbose = 1;        /* maximum verbosity */
            cache_verbose = 1;  /* verbosity -- see -v flag */
            sscanf(argv[0],"-D%d",&pfs_debug);
            options |= SO_DEBUG;
            debug++;
            break;

        case 'f':
            flush = 1;
            break;

        case 'm':
            manage_cache = 1;
            break;

        case 'r':
            replace = 1;
            break;

        case 't':     /* FTP Trace */
            trace++;
            break;

        case 'v':
            verbose = 1;        /* need to unify verbose and cache_verbose
                                 variables -- they are used identically. */
            cache_verbose = 1;
            break;

        case '\0':              /* Ignore the dummy flag "-".  This lets us
                                   specify flags easily to execl(). */
            break;

        default: {
        usage:
			/* Note this doesnt use getopts, so the old usage
			   message was wrong, flags must be separated*/
            fprintf(stderr,
                  "Usage: vcache [-f] [-m] [-r] [-v] local access-method-name hosttype \
host nametype name [additional args...]\n");
            return(1);
        }                       /* enclosing case of switch */
        }                       /* switch */
        argc--; argv++;
    }
    if((argc == 1) && flush) {
        host = NULL;
        remote = NULL;
        local = argv[0]; argc--; argv++;
        method = NULL;
    }
    else if((argc == 0) && flush) {
        host = NULL;
        remote = NULL;
        local = NULL;
        method = NULL;
    }
    else if(argc >= 6) {
        local = argv[0]; argc--; argv++;
        method = argv[0]; argc--; argv++;
        if (!strequal(argv[0], "INTERNET-D")) goto usage;
        argc--, argv++;         /* hosttype always INTERNET-D for now. */
        host = argv[0]; argc--; argv++;
        if (!strequal(argv[0], "ASCII")) goto usage;
        argc--, argv++;         /* filetype always ASCII for now. */
        remote = argv[0]; argc--; argv++;
    }
    else {
        goto usage;
    }

    if (retval = vcache2a(host, remote, local, method, argv, manage_cache))
	fprintf(stderr, "vcache: %s", p_err_string);

    exit(retval);

}


