/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <usc-license.h>
 */

#include <usc-license.h>

#include <stdio.h>
#include <pfs.h>
#include <perrno.h>


char *prog;
int	pfs_debug = 0;

PATTRIB		pget_at();
static const char *sequence_separator = " ; "; /* should be changed at some
                                                 point?  */

static void
display_link(VLINK l, int dmagic,int verbose, int pretty_flag, int show_at,
            char *at_name, int nesting);

void
main(argc,argv)
    int		argc;
    char	*argv[];
{
    VDIR_ST		dir_st;
    VDIR		dir= &dir_st;
    VLINK		l;
    VLINK		replica;
    int		flags = 0;
    int		verbose = 0;
    int		dmagic = 0;
    int		show_failed = 0;
    int		show_invisible = 0;
    int		show_at = 0;
    int		replica_flag = 0;
    int		pretty_flag = 0;
    int		conflict_flag = 0;
    int             dirflag = 0;
    int		tmp;
    char		*at_name = NULL; /* will point to allocated memory */
    char		*progname = argv[0];

    prog = progname;

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    flags = RVD_LREMEXP;

    vdir_init(dir);

    argc--;argv++;

    while (argc > 0 && **argv == '-') {
        switch ((*argv)[1]) {

        case 'a':   /* Right now, just shows object attributes. */
                    /* SHOULD show object and link attributes both. */
            show_at = 1; 
            verbose = 1;
            tmp = qsscanf(argv[0],"-a%&s", &at_name);
            if(tmp < 1) at_name = stcopy("#ALL");
            break;

        case 'A':  /* Show only link attributes */
            show_at = 2; /* Show only link attributes */
            verbose = 1;
            tmp = qsscanf(argv[0],"-A%&s", &at_name);
            if(tmp < 1) at_name = stcopy("#ALL");
            break;

        case 'c':  /* Show conflicting links */
            conflict_flag++;
            break;

        case 'd':           /* If target specified, treat it as a link even
                               if it's a directory.  This is just like the -d
                               flag in UNIX ls. */
            dirflag++;
            break;
        case 'D':  /* Debug level */
            pfs_debug = 1; /* Default debug level */
            sscanf(argv[0],"-D%d",&pfs_debug);
            break;

        case 'f':  /* Show failed links */
            show_failed++;
            break;

        case 'i':  /* Show invisible links */
            show_invisible++;
            break;

        case 'm':  /* Display magic number */
            dmagic++;
            break;

        case 'N':  /* ARDP priority level */
            sscanf(argv[0],"-N%d",&ardp_priority);
            break;

        case 'P':               /* Pretty flag -- truncate if too much. */
            pretty_flag++;
            break;
            
        case 'r':  /* Show replicas */
            replica_flag++;
            break;

        case 'u':  /* Do not expand union links */
            flags = RVD_UNION;
            break;

        case 'v':  /* Display link on more than one line (verbose) */
            verbose = 1;
            break;

        default: {
        usage:
            fprintf(stderr,
                    "Usage: vls [-a,-A,-c,-r,-u,-v,-d] \
[<file or directory name>]\n");
            exit(1);
        }
        }
        argc--, argv++;
    }

    if(show_at) flags |= RVD_ATTRIB;
    if (dirflag) flags |= RVD_DFILE_ONLY;

    if (argc > 1) goto usage;

    ardp_abort_on_int();

    tmp = rd_vdir((argc == 1 ? argv[0] : ""), (char *) NULL, dir,flags);

    if(tmp && (tmp != DIRSRV_NOT_DIRECTORY)) {
        fprintf(stderr,"%s",progname);
        perrmesg(" failed: ", tmp, NULL);
        exit(1);
    }

    if(pwarn) pwarnmesg("WARNING: ",0,NULL);

    l = dir->links;

    while(l) {
        if (show_invisible || l->linktype != 'I')
            display_link(l,dmagic,verbose,pretty_flag,show_at,at_name, 0);
        replica = l->replicas;
        while(replica) {
            if((replica_flag && (l->f_magic_no != 0) && 
                (l->f_magic_no == replica->f_magic_no)) || conflict_flag)
                if (show_invisible || l->linktype != 'I')
                    display_link(replica,
                         (dmagic || (l->f_magic_no != replica->f_magic_no)),
                        verbose, pretty_flag, show_at,at_name, 0);
            replica = replica->next;
        }
        l = l->next;
    }

    l = dir->ulinks;

    if((tmp != DIRSRV_NOT_DIRECTORY) || show_failed) {
        while(l) {
            if((l->expanded == FALSE) || (l->expanded == FAILED))
            display_link(l,dmagic,verbose, pretty_flag, show_at,at_name, 0);
            l = l->next;
        }
    }

    vllfree(dir->links);
    exit(0);

}

static  void    display_fil(FILTER fil, int nesting);
static void     display_nesting(int nesting);


static void
display_link(VLINK l, int dmagic,int verbose, int pretty_flag, int show_at,
            char *at_name, int nesting)
{
/* check if a string is a null pointer.  If it is, return an empty string. */ 
#define chknl(s) ((s) ? (s) : "")
    FILTER      fil;
    PATTRIB 	ap;

    if(l->linktype == '-') return;

    if (verbose) {
        if (!nesting) putchar('\n');
        display_nesting(nesting);
        printf("      Name: %s\n",chknl(l->name));
        display_nesting(nesting);
        printf("    Target: %s\n",chknl(l->target));
        display_nesting(nesting);
        printf("  LinkType: %s\n",
               ((l->linktype == 'U') ? "Union" : 
                (l->linktype == 'I' ? "Invisible" : "Standard")));
        if (l->hosttype && !strequal(l->hosttype, "") && 
            !strequal(l->hosttype, "INTERNET-D")) {
            display_nesting(nesting);
            printf("  HostType: %s\n",chknl(l->hosttype));
        }
        display_nesting(nesting);
        printf("      Host: %s\n",chknl(l->host));
        if (l->hsonametype && !strequal(l->hsonametype, "") && 
            !strequal(l->hsonametype, "ASCII")) {
            display_nesting(nesting);
            printf("  NameType: %s\n",chknl(l->hsonametype));
        }
        display_nesting(nesting);
        printf("   Hsoname: %s\n",chknl(l->hsoname));
        if(l->version) {
            display_nesting(nesting);
            printf("   Version: %ld\n",l->version);
        }
        if(dmagic || l->f_magic_no)  {
            display_nesting(nesting);
            printf("  RemoteID: %ld\n",l->f_magic_no);
        }
        if(show_at) {
            /* If only link attributes, then use l->lattrib */
            /* otherwise get attribues for object.          */
            if(show_at == 2) ap = l->lattrib;
            else {
                /* Eventually we will merge the link attributes with  */
                /* the object attribues based on the precedence field,*/
                /* but for now, if attributes are returned for the    */
                /* object we use them, otherwise we use those associated */
                /* with the link                                       */ 
                ap = pget_at(l,at_name);
                if(!ap) ap = l->lattrib; /* This really needs fixing soon. */
            }
            if(ap) {
                display_nesting(nesting);
                printf("Attributes:\n");
                if (!nesting) putchar('\n');
                while(ap) {
                    display_nesting(nesting);
                    printf("%15s: ",ap->aname);
                    if(ap->avtype == ATR_SEQUENCE) {
                        TOKEN tk;
                        int first_token_output = 0;
                        for (tk = ap->value.sequence; tk; tk = tk->next) {
                            if (first_token_output++)
                                fputs(sequence_separator, stdout);
                            /* if (tk->token) */
                                fputs(tk->token, stdout);
                            /* else fputs("''", stdout); */
                        }
                        putchar('\n');
                    } else if(ap->avtype == ATR_LINK) {
#if 1
                        puts("LINK valued attribute:");
                        /* Display the link WITH subattributes. */
                        /* Downgrade the show_at to not retrieving
                           OBJECT attributes from the sublinks, though. */
                        display_link(l, dmagic, verbose, pretty_flag, 
                                     show_at? 1 : 0,
                                     at_name, nesting + 1);
#else
                        /* Display the link without subattributes */
                        VLINK vl = ap->value.link;
                        printf("%s %s %s %d %d\n",
                               vl->name, vl->host, vl->hsoname, vl->version, 
                               vl->f_magic_no);
#endif
                    } else if (ap->avtype == ATR_FILTER)
                        display_fil(ap->value.filter, nesting);
                    else printf("<unknown-type %d>\n",ap->avtype);
                    ap = ap->next;
                }
                if(l->filters) printf("\n");
            }
            atlfree(ap);
        }

        if(l->filters) {
            display_nesting(nesting);
            printf("   Filters:\n");
            for(fil = l->filters; fil; fil = fil->next) {
                display_nesting(nesting);
                display_fil(fil, nesting);
            }
        }
        putchar('\n');
    }
    /* First character is determined by l->target and l->linktype.
       U for a union link (always to a DIRECTORY or DIRECTORY+FILE)
       I for an invisible link (only shown if -i flag specified)
            (could be to a FILE, DIRECTORY, or DIRECTORY+FILE).
       If a normal link, determined by l->target:
          blank (' ') if a normal link to a FILE.
          S for SYMBOLIC
          E for EXTERNAL
          N for NULL (returned if inadequate permissions),
          D for DIRECTORY
          B (Both) for DIRECTORY+FILE
          O for OBJECT 
       
       Second character is F if expanding the union link failed. */

    else {
        char		linkname[MAX_VPATH];
        if(dmagic) sprintf(linkname,"%s#%ld",chknl(l->name),l->f_magic_no);
        else strcpy(linkname,chknl(l->name));

        printf((pretty_flag ? "%c%c %-20.20s %c%-15.15s %-38.38s\n"
                : "%c%c %-20s %c%-15s %s\n"),
               ((l->linktype != 'L') ? l->linktype :
                (l->target && strequal(l->target, "DIRECTORY+FILE")) ? 'B' :
                (l->target && !strequal(l->target, "FILE")) ? *l->target 
                : ' '),
               (l->expanded ? 'F' : ' '),
               chknl(linkname),
               (l->filters ? '*' : ' '),
               chknl(l->host),chknl(l->hsoname));
    }
}

static void
display_fil(FILTER fil, int nesting)
{
    display_nesting(nesting);
    if (fil->name)
        printf("Predefined Filter: %s\n", fil->name);
    else
        /* Should we display link filters the same way we display other links?
           Should we allow them to have subattributes? */
        printf("Link Filter: %s %s %s %ld %ld\n", chknl(fil->link->name),
               chknl(fil->link->host), 
               chknl(fil->link->hsoname), fil->link->version, fil->link->f_magic_no);
    display_nesting(nesting);
    switch(fil->type) {
    case FIL_DIRECTORY:
        printf(" DIRECTORY");
        break;
    case FIL_HIERARCHY:
        printf(" HIERARCHY");
        break;
    case FIL_OBJECT:
        printf(" OBJECT");
        break;
    case FIL_UPDATE:
        printf(" UPDATE");
        break;
    default:
        internal_error("unknown fil->type value.");
    }
    switch(fil->execution_location) {
    case FIL_SERVER:
        printf(" SERVER");
        break;
    case FIL_CLIENT:
        printf(" CLIENT");
        break;
    default:
        internal_error("unknown fil->execution_location");
    }
    switch(fil->pre_or_post) {
    case FIL_PRE:
        printf(" PRE");
        break;
    case FIL_POST:
        printf(" POST");
        break;
    default:
        internal_error("unknown fil->pre_or_post");
    }
    if (fil->args) {
        TOKEN tk;
        printf(" ARGS");
        for (tk = fil->args; tk; tk = tk->next)
            printf(" %s", tk->token);
    }
    putchar('\n');
}


static void
display_nesting(int nesting)
{
    assert(nesting >= 0);
    while (nesting--) 
        putchar('>');
}
