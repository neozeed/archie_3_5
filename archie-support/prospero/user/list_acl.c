/*
 * Copyright (c) 1991 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
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

char *prog;
int	pfs_debug = 0;

extern	char	*acltypes[];

/*
 * List ACL
 */
void
main(argc,argv)
    int		argc;
    char	*argv[];
{
    char		*dname = "";        /* Directory name                */
    char		*lname = NULL;
    VLINK		dlink = NULL;
    int		options = 0;
    ACL		a;

    prog = argv[0];

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    argc--;argv++;

    while (argc > 0 && **argv == '-') {
        switch (*(argv[0]+1)) {

        case 'D':
            pfs_debug = 1; /* Default debug level */
            sscanf(argv[0],"-D%d",&pfs_debug);
            break;

        case 'd':
	    options = 0;
            if (*(argv[0] + 2))
                dname = argv[0] + 2;
            else if (argv[1]) {
                dname = argv[1];
                argc--;argv++;
            } else {
                goto usage;
            }
            break;

            /* This is the default, but it makes it consistent with set_acl */
        case 'l': /* Next arg is a link name */
	    options = 1;
            if (*(argv[0] + 2))
                lname = argv[0] + 2;
            else if (argv[1]) {
                lname = argv[1];
                argc--;argv++;
            } else {
                goto usage;
            }
            break;

        case 'n':               /* Named: the same as included.   */
            options = 5;
            goto namedincluded;
        case 'i':
	    options = 3;
        namedincluded:
            /* Next two args are host and ACL name */
	    if(argc < 3) goto usage;
	    dlink = vlalloc();	/* Can LEAK  */
	    dlink->host = stcopyr(argv[1],dlink->host);
	    dlink->hsoname = stcopyr("",dlink->hsoname);
	    lname = argv[2];
	    argc--;argv++;
	    argc--;argv++;
            break;

	case 'c': /* Container - same as object */
	    options = 4;
	    goto cobject;
        case 'o': /* Next arg is a link name */
	    options = 2;
	cobject:
            if (*(argv[0] + 2))
                dname = argv[0] + 2;
            else if (argv[1]) {
                dname = argv[1];
                argc--;argv++;
            } else {
                goto usage;
            }
            break;

        default:
            goto usage;
        }
        argc--, argv++;
    }

    if(argc > (lname ? 0 : 1)) {
    usage:
        fprintf(stderr, "Usage: list_acl [-d dir, -o object, -n host aclname, -i host aclname] [link-name]\n");
        exit(1);
    }

    if(argc == 1) {
	lname = argv[0];
	options = 1;
    }

    if(options != 3) dlink = rd_vlink(dname);

    if(!dlink) {
	fprintf(stderr, "list_acl: Failed to read %s link\n",
		((options == 2) ? "object" : "directory"));
	exit(1);
    }

    a = get_acl(dlink,lname,options);

    if(!a && perrno) {
        perrmesg("list_acl failed: ", 0, NULL);
        exit(1);
    }

    if(pwarn) pwarnmesg("WARNING: ",0,NULL);

    printf("Access Control list for %s%s%s\n\n", (*dname ? dname : "."), 
           (lname ? " -> " : ""), (lname ? lname : ""));

    while(a) {
        char	typestring[40];
        char	*rightst = "";
        TOKEN       tkindex;

        if(a->rights) rightst = a->rights;

        if(a->atype && *(a->atype))
            sprintf(typestring,"%s(%s)",acltypes[a->acetype],a->atype); 
        else strcpy(typestring,acltypes[a->acetype]);

        printf("  %-26s %-16s",typestring,rightst);
        for (tkindex = a->principals; tkindex; tkindex = tkindex->next)
            printf(" %s", tkindex->token);
        putchar('\n');

        a = a->next;
    }

    printf("\n");

    exit(0);
}

