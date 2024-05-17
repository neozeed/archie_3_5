/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h> 
 */

#include <usc-license.h>
#include <stdio.h>
#include <string.h>

#include <ardp.h>
#include <pfs.h>
#include <perrno.h>
#include <pmachine.h>

char *prog;
int	pfs_debug = 0;

static PATTRIB parse_extam(int *argcp, char ***argvp, char *cur_arg, VLINK vl);
static VLINK rd_slink_with_attrib(char *path);

void
main(argc,argv)
    int		argc;
    char	*argv[];
{
    char		*progname = argv[0];
    char		*cur_arg;

    int		flags = 0;       /* Union, etc.                   */
    int		symbolic = 0;    /* Symbolic option               */
    int		native = 0;      /* Native options                */
    int		message = 0;     /* Read Virt filename from stdin */
    int		nm2notdir = 0;   /* Do not treat nm2 as a dir     */
    int		customize = 0;   /* Customizing current view      */
    int		avsflag = 0;     /* Use active VS (no closure)    */
    char		closure[MAX_VPATH];

    PATTRIB             extam = NULL; /* EXTERNAL access method, if specified
                                       */
    char		hst[MAX_VPATH];	 /* Host name                     */
    char		nm1[MAX_VPATH];	 /* Name of object to be linked   */
    char		nm2[MAX_VPATH];	 /* The new name for the object   */

    char		exst[MAX_VPATH]; /* Type string for ext access    */

    char		*nlnm;		 /* New link name                 */
    char		*ldir;		 /* Dir to get link               */
    VLINK		nl;              /* New link                      */

    VDIR_ST		dir_st;
    VDIR		dir = &dir_st;

    int		tmp = 0;         /* Temp return value             */

    prog = argv[0];

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    vdir_init(dir);

    *closure = '\0';

    argc--;argv++;

    nl = vlalloc();		/* CAN LEAK */

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

            case 'N':  /* Priority (nice) */
                ardp_priority = ARDP_MAX_PRI; /* Use this if no # */
                sscanf(cur_arg,"%d",&ardp_priority);
                if(ardp_priority > ARDP_MAX_SPRI) 
                    ardp_priority = ARDP_MAX_PRI;
                if(ardp_priority < ARDP_MIN_PRI) 
                    ardp_priority = ARDP_MIN_PRI;
                cur_arg += strspn(cur_arg,"-0123456789");
                break;

            case 'a':
                avsflag++;
                break;

            case 'c': 
                /* If customizing directory, then we do not want to find */
                /* the link that is already in the union linked directory*/
                customize++;
                nm2notdir++;
                break;

            case 'e': 
                /* External links. 
                   Last argument cannot be a directory, since there's no way
                   for us to tell what the linkname should be. */
                extam = parse_extam(&argc, &argv, cur_arg, nl);
                APPEND_ITEM(extam, nl->lattrib);
                nl->target = stcopyr("EXTERNAL", nl->target);
                cur_arg = "";
                ++nm2notdir;
                break;

            case 'i':
                flags = AVL_INVISIBLE;
                break;

            case 'm':
                message++; 
                break;

            case 'n':
                native++;
                break;

            case 's':
                nl->target = stcopyr("SYMBOLIC",nl->target);
                nl->hosttype = stcopyr("VIRTUAL-SYSTEM",nl->hosttype);
                symbolic++;
                break;

            case 'u':
                flags = AVL_UNION;
                break;

            default:
                fprintf(stderr,
                        "Usage: vln [-a,-m,-s,-i,-u] [-e,-n host] name1 [name2]\n");
                fputs(
"You can specify the source of a VLN link in one of three ways:\n\
1) As an existing virtual name:\n\
    <name1>\n\
2) As an EXTERNAL link, with one of:\n\
    -e GOPHER <host>(<port>) <gopher-selector> TEXT\n\
    -e GOPHER <host>(<port>) <gopher-selector> BINARY\n\
    -e TELNET <host>[(<optional-port>)] <introductory-message>\n\
        (note your shell will probably make you quote the parentheses)\n\
    -e AFTP <host> <path> BINARY\n\
    -e AFTP <host> <path> ASCII\n\
    -e AFS <afs-path>\n\
    -e <#-of-access-method-args> <method-name> <host-type> <host> \n\
        <hsoname-type> <hsoname> [<additional args>]\n\
3) With native information:\n\
    -n hostname hsoname\n\
\n\
The following flags are the important ones.  They modify the request:\n\
    -i  Make an INVISIBLE link\n\
    -u  Make a UNION link (source must be a directory)\n\
    -s  Make a SYMBOLIC link (not compatible with -n or -e flags)\n", stderr);

                exit(1);
            }
        }
        argc--, argv++;
    }
    if (extam && (native || (flags & AVL_UNION) 
                  || customize || message || symbolic)) {
        fprintf(stderr, "%s: you specified a flag not compatible with -e\n",
                progname);
        exit(1);
    }
        
        
scandone:

    /* I must still add support for allowing one to */
    /* specify the host type and id type and the    */
    /* link type with the native option             */

    /* if stdin is not a tty and OK to use closure */
    /* then we have to extract closure info        */
    if(!avsflag && !isatty(0)) {
        char	*s;
        s = readheader(stdin,"virtual-system-name:");
        if(!s)  {
            fprintf(stderr,"Can't find Virtual-System-Name.\n");
            exit(1);
        }
        strcpy(closure,s);

        /* And if the -m option was specified, read the rest to the */
        /* file looking for te virtual file name.                   */
        if(message) {
            s = readheader(stdin,"virtual-file-name:");
            if(!s) {
                fprintf(stderr,"Can't find Virtual-file-name.\n");
                exit(1);
            }
            if(*s == '/') s++;
            strcat(nm1,s);

        }
    }
    /* If message option specified, but no stdin, that's an error */
    else if(message) {
        fprintf(stderr,"vln: -m only works if input is redirected\n");
        exit(1);
    }

    /* We need at least 1 argument + 1 for host if a native */
    /* link and less name1 if it is read from stdin         */
    if((argc < 1 + (native ? 1 : 0) - (message ? 1 : 0)) ||
       (argc > 2 + (native ? 1 : 0) - (message ? 1 : 0)) ||
       (extam && argc != 1)) {
        fprintf(stderr,
                "Usage: vln [-a,-m,-s,-u] { [-n host] name1 | -e \
access-method-args } [name2]\n");
        exit(1);
    }

    /* If a native link, then first argument is the hostname */
    if(native) {
        strcpy(hst,argv[0]);
        argc--;argv++;
    }
    /* If symbolic, but not native, host name is the virtual system name */
    else if (symbolic) {
        char	*s;

        if(*closure) s = closure;
        else s = pget_vsname();

        if(s) strcpy(hst,s);
        else {
            fprintf(stderr,"vln: Environment not initialized - source vfsetup.source then run vfsetup");
            exit(1);
        }

    }

    /* Read the first file name if it is required. nm1 is not used in an
       EXTERNAL link.  */
    if(!message && !extam) {
        strcpy(nm1,argv[0]);
        argc--;argv++;
    }

    /* If non native symlink, and if reltive to working dir, */
    /* prepend working directory                             */
    if(!native && symbolic && (*nm1 != '/')) {
        char	temp[MAX_VPATH];
        char	*wd;
        wd = pget_wd();
        if(!wd) {
            fprintf(stderr,"vln: Environment not initialized - source vfsetup.source then run vfsetup");
            exit(1);
        }
        sprintf(temp,"%s/%s",wd,nm1);
        strcpy(nm1,temp);
    }

    /* If normal link and we have a closed namespace from */
    /* stdin, then make nm1 relative to closed namespace  */
    if(!native && !extam && !symbolic && *closure) {
        char	temp[MAX_VPATH];
        sprintf(temp,"%s:%s",closure,nm1);
        strcpy(nm1,temp);
    }

    /* Assume second file name is a directory.  If null, then */
    /* it is the current working directory.                   */
    if(argc > 0) {              /* If external, this should always be true. */
        strcpy(nm2,argv[0]);
        argc--;argv++;
    } else {
        assert(!extam);
        strcpy(nm2,"");
    }
    if (!extam) {
        /* Assume new link name is last component of nm1 */
        nlnm = p_uln_rindex(nm1,'/');
        if(!nlnm && !(nlnm = p_uln_rindex(nm1,':'))) nlnm = nm1;
        else nlnm++;

        /* If nm1 was either empty, or ended in / or :, must use last */
        /* component of nm2 as link name.  i.e. nm2 is not the        */
        /* directory                                                  */
        if(! *nlnm) nm2notdir++;
    }

    /* If a symbolic or native, fill in fields */
    if(symbolic || native) {
        nl->host = hst;
        nl->hsoname = nm1;
    }
    /* Otherwise, resolve nm1 */
    else if (!extam) {
        vlfree(nl);
        if ((nl = rd_slink_with_attrib(nm1)) == NULL) {
            fprintf(stderr,"%s: vlink: %s not found\n", progname, nm1);
            exit(1);
        } 
    }
    /* Fix type field if nl is a directory */
    if((strcmp(nl->target,"FILE") == 0)  &&
       (p_get_dir(nl,NULL,dir,GVD_VERIFY,NULL) == PSUCCESS))
        nl->target = stcopyr("DIRECTORY",nl->target);


    p_clear_errors();

    /* If nm2 might be the name of the directory to use */
    if(!nm2notdir) tmp = add_vlink(nm2,nlnm,nl,flags);

    /* If not a directory, then the last component on nm2 is */
    /* the link name, and what precedes it is the dir name   */
    if (nm2notdir || (tmp == DIRSRV_NOT_DIRECTORY)) {
        nlnm = p_uln_rindex(nm2,'/');
        if(!nlnm) {
            /* If nm2 is null, and the customize option was      */
            /* specified, then nm2 is the last component of nm1  */
            if(customize && (! *nm2)) {
                nlnm = p_uln_rindex(nm1,'/');
                if(!nlnm) nlnm = nm1;
            }
            else nlnm = nm2;
            ldir = "";
        }
        else {
            if(! *(nlnm+1)) {
                fprintf(stderr,"vln: invalid name %s\n",nm2);
                exit(1);
            }
            *(nlnm++) = '\0';
            ldir = nm2;
        }

        /* Add it, this time in the parent directory */
        tmp = add_vlink(ldir,nlnm,nl,flags);
    }

    if(tmp) {
        fprintf(stderr,"%s",progname);
        perrmesg(" failed: ", tmp, NULL);
        exit(1);
    }

    if(pwarn) pwarnmesg("WARNING: ",0,NULL);

    exit(0);
}


static void badext();

static PATTRIB
parse_extam(int *argcp, char ***argvp, char *cur_arg, VLINK vl) 
{
    PATTRIB retval = atalloc();
    char *am_name;
    int numargs;                /* if numeric option */

    retval->precedence = ATR_PREC_CACHED;
    retval->nature = ATR_NATURE_FIELD;
    retval->avtype = ATR_SEQUENCE;
    retval->aname = stcopyr("ACCESS-METHOD", retval->aname);
    retval->value.sequence = NULL;
    if (*cur_arg)  am_name = cur_arg;
    else am_name = *++*argvp, --*argcp;
    if (*argcp < 1) badext();
    ++*argvp, --*argcp;
    if (strequal(am_name, "AFS")) {
        if (*argcp < 1) badext();
        retval->value.sequence = tkappend("AFS", retval->value.sequence);
        /* no hosttype or hostname.  nametype is just ASCII. */ 
        retval->value.sequence = tkappend("", retval->value.sequence);
        retval->value.sequence = tkappend("", retval->value.sequence);
        retval->value.sequence = tkappend("", retval->value.sequence);
        vl->hsoname = stcopyr(**argvp, vl->hsoname);
        retval->value.sequence = tkappend("", retval->value.sequence);
        /* Leave ourselves on top of the last used argument.  The enclosing
           loop will increment for us. */
    } else if (strequal(am_name, "NFS")) {
        if(*argcp < 3) badext();
        retval->value.sequence = tkappend("NFS", retval->value.sequence);
        retval->value.sequence = tkappend("", retval->value.sequence);
        vl->host = stcopyr(**argvp, vl->host);
        retval->value.sequence = tkappend("", retval->value.sequence);
        ++*argvp, --*argcp;
        retval->value.sequence = tkappend("", retval->value.sequence);
        vl->hsoname = stcopyr(**argvp, vl->hsoname);
        retval->value.sequence = tkappend("", retval->value.sequence);
        ++*argvp, --*argcp;
        retval->value.sequence = tkappend(**argvp, retval->value.sequence);
    } else if (strequal(am_name, "AFTP")) {
        if(*argcp < 3) badext();
        retval->value.sequence = tkappend("AFTP", retval->value.sequence);
        retval->value.sequence = tkappend("", retval->value.sequence);
        vl->host = stcopyr(**argvp, vl->host);
        retval->value.sequence = tkappend("", retval->value.sequence);
        ++*argvp, --*argcp;
        retval->value.sequence = tkappend("", retval->value.sequence);
        vl->hsoname = stcopyr(**argvp, vl->hsoname);
        retval->value.sequence = tkappend("", retval->value.sequence);
        ++*argvp, --*argcp;
        if (***argvp == 'b' || ***argvp == 'B')
            retval->value.sequence = tkappend("BINARY", retval->value.sequence);
        else if (***argvp == 't' || ***argvp == 'T' || 
                 ***argvp == 'a' || ***argvp == 'A') 
            retval->value.sequence = tkappend("ASCII", retval->value.sequence);
        else
            badext();
    } else if (strequal(am_name, "GOPHER")) {
        if(*argcp < 3) badext();
        retval->value.sequence = tkappend("GOPHER", retval->value.sequence);
        retval->value.sequence = tkappend("", retval->value.sequence);
        vl->host = stcopyr(**argvp, vl->host);
        retval->value.sequence = tkappend("", retval->value.sequence);
        ++*argvp, --*argcp;
        retval->value.sequence = tkappend("", retval->value.sequence);
        vl->hsoname = stcopyr(**argvp, vl->hsoname);
        retval->value.sequence = tkappend("", retval->value.sequence);
        ++*argvp, --*argcp;
        if (***argvp == 'b' || ***argvp == 'B')
            retval->value.sequence = tkappend("BINARY", retval->value.sequence);
        else if (***argvp == 't' || ***argvp == 'T')
            retval->value.sequence = tkappend("TEXT", retval->value.sequence);
        else
            badext();
    } else if (strequal(am_name, "TELNET")) {
        if(*argcp < 2) badext();
        retval->value.sequence = tkappend("TELNET", retval->value.sequence);
        retval->value.sequence = tkappend("", retval->value.sequence);
        vl->host = stcopyr(**argvp, vl->host);
        ++*argvp, --*argcp;
        retval->value.sequence = tkappend("", retval->value.sequence);
        /* Empty HSONAME. */
        vl->hsoname = stcopyr("", vl->hsoname);
        retval->value.sequence = tkappend("", retval->value.sequence);
        retval->value.sequence = tkappend("", retval->value.sequence);
        /* The login message */
        retval->value.sequence = tkappend(**argvp, retval->value.sequence);
    } else if (qsscanf(am_name, "%d", &numargs) == 1) {
        int i;
        if (numargs > *argcp || numargs < 1) badext();
        vl->hsoname = stcopyr("", vl->hsoname);
        retval->value.sequence = 
            tkappend(**argvp, retval->value.sequence);
        for (i = 1; i < numargs; ++i) {
            ++*argvp, --*argcp;
            if (i == 2) {
                vl->host = stcopyr(**argvp, vl->host);
                retval->value.sequence = tkappend("", retval->value.sequence);
#if 0
            } else if (i == 4) {
                vl->hsoname = stcopyr(**argvp, vl->hsoname);
                retval->value.sequence = tkappend("", retval->value.sequence);
#endif
            } else {
                retval->value.sequence = 
                    tkappend(**argvp, retval->value.sequence);
            }
        }
    } else {
        badext();
    }
    return retval;
}


static void
badext()
{
    fprintf(stderr, "Bad arguments to -e flag.  Usage:\n\
  -e AFS <afs-hsoname>\n\
  -e NFS <hostname> <hsoname> <filesystem>\n\
  -e AFTP <hostname> <hsoname> { BINARY | TEXT }\n\
  -e GOPHER <hostname>(<port>) <gopher-selector> { BINARY | TEXT }\n\
  -e <num-args> arg1 arg2 ... argnum\n");
    exit(1);
}


static VLINK
rd_slink_with_attrib(char *path)
{
    char		*c;		 /* Component part of name         */
    char		*d;		 /* Directory part of name	   */
    char		*slash;		 /* Position of slash              */
    char		*colon;		 /* Position of colon              */

    int		flags;

    VDIR_ST		dir_st;
    VDIR		dir = &dir_st;
    VLINK		v;

    vdir_init(dir);

    /* special case just a . as a name for the current working directory,
       by mapping it onto the empty string. . */
    if (strequal(path, ".")) ++path;
    d = path;
    slash = p_uln_rindex(path,'/');
    colon = p_uln_rindex(path,':');

    if(colon && (!slash || (colon > slash))) {
        *(colon + 1) = '\0';
        c = p_uln_rindex(path,':') + 1;
    }
    else if(slash) {
        if(slash == path) d = "/";
        *slash = '\0';
        c = slash + 1;
    }
    else {
        d = "";
        c = path;
    }

    if(*c) 	flags = RVD_FIND;
    else flags = RVD_DFILE_ONLY;

    perrno = rd_vdir(d,c,dir,flags | RVD_ATTRIB);

    if(perrno || !dir->links) {
        if (!perrno) perrno = PFAILURE;
        vllfree(dir->links);
        vllfree(dir->ulinks);
        return(NULL);
    }

    vllfree(dir->ulinks);
    return(dir->links);
}
