/*
 * Copyright (c) 1991 by the University of Washington
 * Copyright (c) 1992 by the University of Southern California
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
#include <pprot.h>
#include <pmachine.h>

char *prog;
int	pfs_debug = 0;

extern	char	*acltypes[];

/*
 * Set ACL
 */
void
main(argc,argv)
    int		argc;
    char	*argv[];
{
    char		*cur_arg;
    char		*dname = "";        /* Directory name                */
    VLINK		dlink;
    int		retval;
    char		*lname = NULL;
    int		flags = EACL_DIRECTORY;
    int		incomp = 0;        /* Incompatible options */

    ACL	a = acalloc();          /* memory will be freed on program exit. */

    prog = argv[0];

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    argc--;argv++;

    a->acetype = ACL_ASRTHOST;
    a->atype = "";
    a->rights = NULL;
    a->principals = NULL;

    while (argc > 0 && **argv == '-') {
        cur_arg = argv[0]+1;

        /* If a - by itself, then no more arguments */
        if(!*cur_arg) {
            argc--, argv++;
            goto scandone;
        }

        while (*cur_arg) {
            switch (*cur_arg++) {

            case 'D':  /* Debug level */
                pfs_debug = 1; /* Default debug level */
                sscanf(cur_arg,"%d",&pfs_debug);
                cur_arg += strspn(cur_arg,"0123456789");
                break;

            case 'a': /* Add privs */
                if(incomp++) goto incomp_o;
                flags |= EACL_ADD;
                break;

            case 'A': /* Next arg is authtype */
                if (*cur_arg) {
                    a->atype = cur_arg;
                    cur_arg = "";
                } else if (argv[1]) {
                    a->atype = argv[1];
                    argc--;argv++;
                } else {
                    goto usage;
                }
                break;

            case 'd': /* Next arg is the directory name */
                if (*cur_arg) {
                    dname = cur_arg;
                    cur_arg = "";
                } else if (argv[1]) {
                    dname = argv[1];
                    argc--;argv++;
                } else {
                    goto usage;
                }
                break;

            case 'E':  /* Entire create an entirely new ACL */
                if(incomp++) goto incomp_o;
                flags |= EACL_SET;
                break;

            case 'i': /* Insert separate entry in ACL */
                if(incomp++) goto incomp_o;
                flags |= EACL_INSERT;
                break;

            case 'K':  /* Kill ACL (reverts to DEFAULT or DIRECTORY) */
                if(incomp++) goto incomp_o;
                a->acetype = ACL_NONE;
                flags |= EACL_DEFAULT;
                break;

            case 'l': /* Next arg is a link name */
		flags |= EACL_LINK;
                if (*cur_arg) {
                    lname = cur_arg;
                    cur_arg = "";
                } else if (argv[1]) {
                    lname = argv[1];
                    argc--;argv++;
                } else {
                    goto usage;
                }
                break;

            case 'n': /* Do not include SYSTEM */
                flags |= EACL_NOSYSTEM;
                break;

            case 'N': /* It's OK if I won't be able to change it again */
                flags |= EACL_NOSELF;
                break;

            case 'o': /* Next arg is an object name */
		flags |= EACL_OBJECT;
                if (*cur_arg) {
                    dname = cur_arg;
                    cur_arg = "";
                } else if (argv[1]) {
                    dname = argv[1];
                    argc--;argv++;
                } else {
                    goto usage;
                }
                break;

            case 'r':  /* Remove individual ACL entry */
                if(incomp++) goto incomp_o;
                flags |= EACL_DELETE;
                break;

            case 's': /* Subtract privs */
                if(incomp++) goto incomp_o;
                flags |= EACL_SUBTRACT;
                break;

            case 't': /* Next arg is the type of ACL entry */
                if (!*cur_arg) {
                    if((cur_arg = argv[1]) == NULL)
                        goto usage;
                    argc--,argv++;
                } 
                for(a->acetype = 0;acltypes[a->acetype];(a->acetype)++) {
                    if(strccmp(acltypes[a->acetype],cur_arg)==0)
                        break;
                }
                if(acltypes[a->acetype] == NULL) {
                    fprintf(stderr,"set_acl: Unknown ACL type\n");
                    exit(1);
                }		    
                cur_arg = "";   /* done with this argument. */
                break;

            incomp_o:
                fprintf(stderr,
                        "set_acl: Incompatible options specified\n");
            default:
                fprintf(stderr,"Usage: set_acl [-asirKE,-n,-N] [-t type] \
[-d dir] [-l link] rights [principal ... ]\n");
                exit(1);
            }
        }
        argc--, argv++;
    }

scandone:

    if(!incomp) flags |= EACL_ADD;

    if(((argc < 1) && ((a->acetype == ACL_OWNER)||
                       (a->acetype == ACL_ANY)))||
       ((argc < 2) && ((a->acetype == ACL_AUTHENT)||
                       (a->acetype == ACL_LGROUP)||
                       (a->acetype == ACL_GROUP)||
                       (a->acetype == ACL_ASRTHOST)||
                       (a->acetype == ACL_TRSTHOST)))) {
            usage:
                fprintf(stderr,"Usage: set_acl [-asirKE,-n,-N] [-t type] \
[-d dir] [-l link] [ rights [principal ... ] ]\n");
        exit(1);
     }

    if(argc >= 1) a->rights = argv[0];
    argc--;argv++;

    while(argc > 0) {
        a->principals = tkappend(argv[0],a->principals);
        argc--;argv++;
    }

    perrno = 0;
    dlink = rd_vlink(dname);
    if(!dlink) {
        fprintf(stderr, "set_acl: Couldn't find %s\n",dname);
        exit(1);
    }

    retval = modify_acl(dlink,lname,a,flags);

    if(retval) {
        perrmesg("set_acl failed: ", retval, NULL);
        exit(1);
    }

    if(pwarn) pwarnmesg("WARNING: ",0,NULL);

    exit(0);
}

