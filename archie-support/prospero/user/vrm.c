/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1993             by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>
#include <stdio.h>

#include <perrno.h>
#include <pfs.h>        /* For p_initialize etc */

char *prog;
int	pfs_debug = 0;

void
main(argc,argv)
    int 	argc;
    char	*argv[];
{
    char	*progname;
    int	tmp;

    prog = argv[0];

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    progname = argv[0];
    argc--;argv++;

    while (argc > 0 && **argv == '-') {
        switch (*(argv[0]+1)) {

        case 'D':
            pfs_debug = 1; /* Default debug level */
            sscanf(argv[0],"-D%d",&pfs_debug);
            break;

        case '-': /* -- means stop scanning options */
            argc--, argv++;
            goto scandone;

        default:
            fprintf(stderr,"Usage: vrm link\n");
            exit(1);
        }
        argc--, argv++;
    }

scandone:

    if(argc != 1) {
        fprintf(stderr,"Usage: vrm link\n");
        exit(1);
    }

    tmp = del_vlink(argv[0],0);

    if (tmp == DIRSRV_NOT_FOUND) {
        fprintf(stderr, "%s: The link %s was not found.\n", progname, argv[0]);
        exit(1);
    } else if(tmp) {
        fprintf(stderr,"%s",progname);
        perrmesg(" failed: ", 0, NULL); /* use '0' so we display the message */
        exit(1);
    }

    if(pwarn) pwarnmesg("WARNING: ",0,NULL);

    exit(0);
}
