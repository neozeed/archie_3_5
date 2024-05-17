/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 */

#include <uw-copyright.h>
#include <usc-copyr.h>
#include <stdio.h>

#include <pfs.h>
#include <perrno.h>

int	pfs_debug = 0;

void
main(argc,argv)
    int 	argc;
    char	*argv[];
{
    char	*progname;
    int	flags = 0;
    int	tmp;

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    progname = argv[0];
    argc--;argv++;

    while (argc > 0 && **argv == '-') {
        switch (*(argv[0]+1)) {

        case 'D':
            pfs_debug = 1; /* Default debug level */
            sscanf(argv[0],"-D%d",&pfs_debug);
            break;

        case 'l':
            flags |= MKVD_LPRIV;
            break;

        case '-': /* -- means stop scanning options */
            argc--, argv++;
            goto scandone;

        default:
            printf("Usage: vmkdir [-l] new-directory-name\n");
            exit(1);
        }
        argc--, argv++;
    }

scandone:

    if(argc != 1) {
        printf("Usage: vmkdir [-l] new-directory-name\n");
        exit(1);
    }

    tmp = mk_vdir(argv[0],flags);

    if(tmp) {
        fprintf(stderr,"%s",progname);
        perrmesg(" failed: ", tmp, NULL);
        exit(1);
    }

    if(pwarn) pwarnmesg("WARNING: ",0,NULL);

    exit(0);
}
