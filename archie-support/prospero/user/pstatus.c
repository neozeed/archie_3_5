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
#include <pfs.h>
#include <ardp.h>
#include <perrno.h>

char *prog;
int		pfs_debug = 0;

void
main(argc,argv)
    int		argc;
    char	*argv[];
{
    RREQ	req;
    char	dirhst[40];
    char	*command = "STATUS";
    int	tmp;
    int motd = 0;

    prog = argv[0];

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    argc--;argv++;

    while (argc > 0 && **argv == '-') {

        switch (*(argv[0]+1)) {

        case 'D':
            pfs_debug = 1; /* Default debug level */
            sscanf(argv[0],"-D%d",&pfs_debug);
            break;

        case 'N':  /* Priority (nice) */
            ardp_priority = ARDP_MAX_PRI; /* Use this if no # */
            sscanf(argv[0],"-N%d",&ardp_priority);
            if(ardp_priority > ARDP_MAX_SPRI) 
                ardp_priority = ARDP_MAX_PRI;
            if(ardp_priority < ARDP_MIN_PRI) 
                ardp_priority = ARDP_MIN_PRI;
            break;

        case 'v':
            command = "VERSION";
            break;

        case 'M':
            motd = 1;
            break;

        default:
            fprintf(stderr,
                    "Usage: pstatus [-v] [-M] [-N[#]] [-D[#]] [host name]\n");
            exit(1);
        }
        argc--, argv++;
    }

    req = ardp_rqalloc();

    gethostname(dirhst,40);

    if(argc > 1) {
        fprintf(stderr, 
                "Usage: pstatus [-v] [-M] [-N[#]] [-D[#]] [host name]\n");
        exit(1);
    }

    if(argc > 0)
        strcpy(dirhst,argv[0]);

    if (motd) p__add_req(req, "PARAMETER GET MOTD\n");
    p__add_req(req, "%s\n", command);

    printf("Sending message to %s...\n",dirhst);

    tmp = ardp_send(req,dirhst,0,ARDP_WAIT_TILL_TO);

    if(tmp) {
        perrmesg("pstatus failed: ", 0, NULL);
        exit(1);
    }

    if(pwarn) pwarnmesg("WARNING: ",0,NULL);

    printf("Response:\n");

    while(req->inpkt) {
        printf("%s",req->inpkt->text);
        req->inpkt = req->inpkt->next;
    }

    ardp_rqfree(req);
    exit(0);

}

