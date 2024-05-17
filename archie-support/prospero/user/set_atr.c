/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>

#include <stdio.h>
#include <string.h>

#include <pfs.h>
#include <perrno.h>

char *prog;
int	pfs_debug = 0;

static void usage();

void
main(argc,argv)
    int		argc;
    char	*argv[];
{
    int		tmp;            /* return from subfunctions */
    int         modreq = EOI_REPLACE; /* modification request. */
    int         specified_prec = 0; /* Did the user specify any precedence?  If
                                       not, behave adaptively. */
    int         set_object = 0;     /* Did we successfully set an OBJECT at? */
    VLINK       vl;
    PATTRIB     at = atalloc();
    FILTER      fl = flalloc(); /* in case a filter argument is specified. */

    char        *linkname = NULL;
    char        *nativehost = NULL;
    char        *nativehandle = NULL;

    prog = argv[0];

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    /* Initialize attribute; this may be modified by later flags. */
    at->precedence = ATR_PREC_UNKNOWN;
    at->nature = ATR_NATURE_APPLICATION;
    at->avtype = ATR_UNKNOWN;
    at->value.sequence = NULL;
    at->previous = NULL;
    at->next = NULL;

    fl->type = FIL_DIRECTORY;   /* only type supported right now, I believe. */
    fl->execution_location = FIL_CLIENT;
    fl->pre_or_post = FIL_POST;
    for (;*++argv;) {                  /* process flags */
        /* We are now positioned at an unread argument. */

        if (strnequal(argv[0],"-D",2)) {
            pfs_debug = 1; /* Default debug level */
            sscanf(argv[0],"-D%d",&pfs_debug);
            continue;
        }

        /* Modification requests. */
        if (strequal(argv[0], "-add")) {
            modreq = EOI_ADD;
            continue;
        }
        if (strequal(argv[0], "-delete")) {
            modreq = EOI_DELETE;
            continue;
        }
        if (strequal(argv[0], "-delete-all")) {
            modreq = EOI_DELETE_ALL;
            continue;
        }
        if (strequal(argv[0], "-replace")) {
            modreq = EOI_REPLACE;
            continue;
        }

        /* Attribute values. */
        if (strequal(*argv, "-object")) {
            at->precedence = ATR_PREC_OBJECT;
            continue;
        }
        if (strequal(*argv, "-linkprec")) {
            at->precedence = ATR_PREC_LINK;
            continue;
        }
        if (strequal(*argv, "-cached")) {
            at->precedence = ATR_PREC_CACHED;
            continue;
        }
        if (strequal(*argv, "-replacement")) {
            at->precedence = ATR_PREC_REPLACE;
            continue;
        }
        if (strequal(*argv, "-additional")) {
            at->precedence = ATR_PREC_ADD;
            continue;
        }

        /* nature */
        if (strequal(*argv, "-field")) {
            at->nature = ATR_NATURE_FIELD;
            continue;
        }
        if (strequal(*argv, "-application")) {
            at->nature = ATR_NATURE_APPLICATION;
            continue;
        }
        if (strequal(*argv, "-intrinsic")) {
            at->nature = ATR_NATURE_INTRINSIC;
            continue;
        }

        if (strequal(argv[0], "-native")) {
            if (!argv[1] || !argv[2]) usage();
            if (linkname) {
                fprintf(stderr, "Can't use -native after you've already \n
specified a target link.\n");
                usage();
            }
            nativehost = *++argv;
            nativehandle = *++argv;
            /* leave us on top of the last used argument. */
            continue;
        } 

        /* Filter arguments. */
        if (strequal(*argv, "-filter-predefined")) {
            assert(at->value.filter = fl); /* make sure flalloc() didn't return
                                              NULL.  */
            at->avtype = ATR_FILTER;
            if (!argv[1]) usage();
            fl->name = *++argv;
            continue;
        }
        if (strequal(*argv, "-filter-loadable")) {
            assert(at->value.filter = fl); /* make sure flalloc() didn't return
                                              NULL.  */
            at->avtype = ATR_FILTER;
            if (!argv[1]) usage();
            p_clear_errors();
            if((fl->link = rd_vlink(*++argv)) == NULL) {
                fprintf(stderr, "%s not found: ", *argv);
                perrmesg(NULL, 0, NULL);
                exit(1);
            }
            continue;
        }
        if (strequal(*argv, "-client")) {
            fl->execution_location = FIL_CLIENT;
            continue;
        }
        if (strequal(*argv, "-server")) {
            fl->execution_location = FIL_SERVER;
            continue;
        }
        if (strequal(*argv, "-pre")) {
            fl->pre_or_post = FIL_PRE;
            continue;
        }
        if (strequal(*argv, "-post")) {
            fl->pre_or_post = FIL_POST;
            continue;
        }
        if (strequal(*argv, "-args")) {
            while (argv[1]) 
                fl->args = tkappend(*++argv, fl->args);
            continue;
        }

        /* Other types of attributes -- link & sequence. */
        if (strequal(*argv, "-linkvalue")) {
            at->avtype = ATR_LINK;
            p_clear_errors();
            if((at->value.link = rd_vlink(linkname = *++argv)) == NULL) {
                fprintf(stderr, "%s not found: ", linkname);
                perrmesg(NULL, 0, NULL);
                exit(1);
            }
            continue;
        }

        if (strequal(*argv, "-sequence")) {
            if (at->avtype != ATR_UNKNOWN) {
                fprintf(stderr, "set_atr: May not specify more than one of \
-sequence, -linkvalue, -filter-loadable and -filter-predefined\n");
                exit(1);
            }
            ++argv;             /* put us on top of an unused argument */
            break;              /* bail */
        }

        /* Unrecognized argument.  If no host specified yet, it's the link to
           munge.  Otherwise, it's the attribute name.   */
        if (!nativehost && !linkname) {
            linkname = *argv;
            continue;
        } else if (!at->aname) {
            at->aname = stcopy(*argv);
            continue;
        } else {
            break;              /* must be a sequence argument. */
        }
    }
    /*  We're done reading all the arguments (except, perhaps, for sequence
        values. */
    if (!nativehost && !linkname) {
        fprintf(stderr, "No target specified!\n");
        usage();
    }
    if (!at->aname) { 
        fprintf(stderr, "No attribute name specified!\n");
        usage();
    }
    /* Read the sequence values. */
    if (at->avtype != ATR_UNKNOWN) {
        if (*argv) {  /* not a sequence type. */
            fprintf(stderr, "Too many arguments.\n");
            usage();
        }
    } else {
        at->avtype = ATR_SEQUENCE;
    }
    while (*argv)
        at->value.sequence = tkappend(*argv++, at->value.sequence);

    if (!linkname && at->precedence != ATR_PREC_OBJECT)  {
        fprintf(stderr, "-native must be specified with the -object flag; \
it only works to set OBJECT attributes.\n");
        usage();
    }
    if (at->precedence != ATR_PREC_UNKNOWN) ++specified_prec;
    if (!specified_prec && strequal(at->aname, "COLLATION-ORDER")) {
        puts("The standard COLLATION-ORDER attribute is normally an attribute \
with LINK precedence.\n\
It looks like you're trying to set it to have OBJECT precedence.  Since \n\
we're operating in adaptive mode here, press RETURN to set it with LINK\n\
precedence.  If you really wanted to set it with some other precedence,\n\
run this command again and specify it explicitly on the command line.\n\
Specify -linkprec on the command line next time in order to avoid getting\n\
this message.");
        if (getchar() != '\n') exit(1);
        at->precedence = ATR_PREC_LINK;
        specified_prec++;
    }
    if (at->precedence == ATR_PREC_OBJECT || !specified_prec) {
        if (!specified_prec) at->precedence = ATR_PREC_OBJECT;
        /* Set the link VL to point to the object whose attributes we're
           modifying. */
        if (nativehost) {
            vl = vlalloc();	/* CAN LEAK */
            vl->host = stcopy(nativehost);
            vl->hsoname = stcopy(nativehandle);
        } else {
            p_clear_errors();
            if((vl = rd_vlink(linkname)) == NULL) {
                fprintf(stderr, "%s not found: ", linkname);
                perrmesg(NULL, 0, NULL);
                exit(1);
            }
        }
        p_clear_errors();
        if(tmp = pset_at(vl, modreq, at)) {
            if (!specified_prec &&tmp == DIRSRV_NOT_AUTHORIZED) {
                puts("Not authorized to set an OBJECT attribute on the \
object. We will \nset a REPLACEMENT attribute on the link.");
                at->precedence = ATR_PREC_REPLACE;
            } else if (!specified_prec && tmp == PSET_AT_TARGET_NOT_AN_OBJECT) {
                printf("This link has a TARGET of %s, which means that it \
does\n\
not point to a real underlying Prospero object, so we cannot set an OBJECT\n\
attribute on it.  We will set a REPLACEMENT attribute on the link instead.\n",
                       vl->target);
                at->precedence = ATR_PREC_REPLACE;
            } else {
                perrmesg("set_atr: pset_at() failed: ", 0, NULL);
                exit(1);
            }
        } else {
            ++set_object;
            if (linkname)
                at->precedence = ATR_PREC_CACHED;
        }
    } 
    /* Now try to set an explicit value on the link. */
    if (!nativehost && at->precedence != ATR_PREC_OBJECT) {
        /* Modifying other than an object attribute. */
        char *compname; 
        VDIR_ST	dir_st;
        VDIR	dir = &dir_st;

        vdir_init(dir);
        assert(linkname);
        compname = p_uln_rindex(linkname, '/');
        if(compname) *compname++ = '\0';
        else {
            compname = linkname;
            linkname = "";
        }
        /* linkname is now the name of the directory in question. */
        p_clear_errors();
        tmp = rd_vdir(linkname, 0, dir, RVD_DFILE_ONLY);
        if (tmp || (dir->links == NULL)) exit(DIRSRV_NOT_DIRECTORY);
        /* Dir->links is a link to the directory */
        if(tmp = pset_linkat(dir->links, compname, modreq, at)) {
            if (set_object)
                printf("We were unable to set a(n) %s attribute on the \
link to the object, but we did successfully set an OBJECT attribute on the \
object itself.\n", lookup_precedencename_by_precedence(at->precedence));
            perrmesg("set_atr: pset_linkat() failed: ", 0, NULL);
            exit(1);
        }
    }        
    if(pwarn) pwarnmesg("WARNING: ",0,NULL);

    exit(0);
}


static void
usage()
{
    fprintf(stderr, 
            "Usage: set_atr [-D[#]] { -native host handle | linkname } attribute-name\n\
\t{ -add | -delete | -replace | -delete-all } (defaults to -replace)\n\
\t{ -object | -linkprec | -cached | -replacement | -additional } \
\t(defaults to adaptive behavior: -object and -cached, unless there is no \
underlying object or unless permission is denied)\n\
\t{ -field | -application | -intrinsic } (defaults to -application)\n\
\t<filter options> (see below)\n\
\t | -linkvalue linkname  | [ [-sequence] seqelem1 seqelem2 ... ]\n\n");
    fputs("\
<filter options>  (you only need to know these if you are setting\n\
                   attributes whose values are filters):\n\
        { { -filter-predefined name | -filter-loadable linkname }\n\
          { -client | -server } { -pre | -post } [ -args arg1 arg2... ] }\n\n",
          stderr);
    fputs("Admittedly, this is a pretty long list of options.  For most \
needs, though, you can just type:\n\
\tset_atr target-link-name attribute-name token1\n\
and the right thing will happen.\n", stderr);
    exit(1);
}
