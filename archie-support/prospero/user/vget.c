/*
 * Copyright (c) 1991, 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <pfs.h>
#include <psite.h>
#include <perrno.h>
#include <pcompat.h>
#include <pmachine.h>

int		pfs_debug = 0;

static int
retrieve_link(VLINK vl, char *lclfil, char *debugflag, char *verboseflag, 
              int forcecopyflag );

static int copy_file(char *inname, char *outname, int forcecopyflag);

static char *progname;          /* name of this program */
static char *prognamecolon = NULL;
void
main(argc, argv)
    char *argv[];
{
    char		path[MAX_VPATH];
    char		*lclfil;
    char		*method;
    VLINK		vl;

    int		message_option = 0;
    int		avsflag = 0;  /* If set - use active VS (no closure) */
    char		*debugflag = "-";
    char		*verboseflag = "-v";
    int                 forcecopyflag = 0;

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    progname = argv[0];
    prognamecolon = ((char *) NULL, "%s: ", progname);
    argc--;argv++;

    while (argc > 0 && *argv[0] == '-' && *(argv[0]+1) != '\0') {
        switch (*(argv[0]+1)) {

        case 'D':
            pfs_debug = 1; /* Default debug level */
            sscanf(argv[0],"-D%d",&pfs_debug);
            debugflag = argv[0];
            break;

        case 'a':
            avsflag++;
            break;

        case 'f':
            forcecopyflag++;
            break;

        case 'm':
            message_option++; 
            break;

        case 'q':
            verboseflag = "-";
            break;

        case 'v':
            verboseflag = "-v";
            break;

        default:
            goto usage;
        }
        argc--; argv++;
    }

    /* The next argument must be the name of the file to retrieve */
    /* within the virtual file system unless it is to be extracted   */
    /* from a mail message                                        */

    if(argc < 1 && !message_option) {
    usage:
        fprintf(stderr,"Usage: %s [-a,-f,-m,-q,-v] virtual-file [local-file]\n",
                progname);
        exit(1);
    }

    if (argc >= 1) strcpy(path,argv[0]);

    /* if stdin is not a tty and OK to use closure */
    /* then we have to extract closure info        */
    if(!avsflag && !isatty(0)) {
        char	*s;
        s = readheader(stdin,"virtual-system-name:");
        if(!s)  {
            fprintf(stderr,"%s: Can't find Virtual-System-Name.\n", progname);
            exit(1);
        }
        sprintf(path,"%s:",s);

        if(message_option) {
            s = readheader(stdin,"virtual-file-name:");
            if(!s) {
                fprintf(stderr,"%s: Can't find Virtual-file-name.\n", progname);
                exit(1);
            }
            strcat(path,s);
        }
        else strcat(path,argv[0]);

    }
    else if(message_option) {
        fprintf(stderr,"%s: Can't find Virtual-file-name.\n", progname);
        exit(1);
    }

    /* If second name was not specified, derive it from first */
    if(argc == (message_option ? 0 : 1)) {
        char *p;
        p = p_uln_rindex(path,'/');
        lclfil = (p != NULL) ? p + 1 : path;
    }
    /* Otherwise local file is the second name */
    else lclfil = (message_option ? argv[0] : argv[1]);

    vl = rd_vlink(path);

    if(!vl) {
        if(perrno == PSUCCESS) fprintf(stderr,"%s: File not found\n", progname);
        else perrmesg(prognamecolon, 0, NULL);
        exit(1);
    }

    if(retrieve_link(vl, lclfil, debugflag, verboseflag, forcecopyflag) != PSUCCESS)
        exit(1);
    exit(0);
}

/* This function looks a heck of a lot like lib/pcompat/mapname().  If there's
   a bug here, there's one there too. */
static int
retrieve_link(VLINK vl, char *lclfil, char *debugflag, char *verboseflag,
              int forcecopyflag)
{
    TOKEN           am_args;
    int             am;         /* access method number */
#ifdef BSD_UNION_WAIT
    union wait 	status;
#else
    int     status;
#endif
    int 		pid;
    int                 methods = P_AM_LOCAL | P_AM_AFTP | P_AM_FTP |
        P_AM_GOPHER | P_AM_RCP | P_AM_PROSPERO_CONTENTS;
    char    *vcargv[12];        /* vcache's ARGV.  Enough to hold any known
                                   access method arguments. */
    char    **vcargvp = vcargv; /* pointer to vcache's argv. */
    char    npath[MAXPATHLEN];  /* local pathname, if needed. */
    int     tmp;

#ifdef P_NFS
    methods |= P_AM_NFS;
#endif P_NFS

#ifdef P_AFS
    methods |= P_AM_AFS;
#endif P_AFS

    am = pget_am(vl,&am_args,methods);
    if (!am) {

        if (perrno) perrmesg(": ", 0, NULL);
        else fprintf(stderr,"%s: Can't access file using any available \
access method.\n", progname);
        RETURNPFAILURE;
    }
    if(pwarn) pwarnmesg("WARNING: ",0,NULL);

    switch(am) {
    case P_AM_AFTP:
    case P_AM_FTP:
    case P_AM_GOPHER:
    case P_AM_RCP:
    case P_AM_PROSPERO_CONTENTS:
        pid = fork();
        *vcargvp++ = "vcache";
        *vcargvp++ = debugflag;
        *vcargvp++ = verboseflag;
        *vcargvp++ = lclfil;
        for (;am_args; am_args = am_args->next)
            *vcargvp++ = am_args->token;
        *vcargvp = NULL;

        if (pid == 0) {
            char		vcachebin[MAXPATHLEN];
            char *p_binaries = getenv("P_BINARIES");
#ifdef P_BINARIES
            if (!p_binaries)
                p_binaries = P_BINARIES;
#endif

            if (!p_binaries || !*p_binaries) {
                DISABLE_PFS(execvp("vcache", vcargv));
                strcpy(vcachebin, "vcache"); /* for error message */
            } else {
                qsprintf(vcachebin, sizeof vcachebin, "%s/vcache", p_binaries);
                DISABLE_PFS(execv(vcachebin,vcargv));
            }
            fprintf(stderr, 
                    "%s: exec failed for %s (errno=%d): ", progname, 
                    vcachebin,errno);
            perror(NULL);
            RETURNPFAILURE;
        }
        else wait(&status);

#ifdef BSD_UNION_WAIT
        tmp = status.w_T.w_Retcode;
#else
        tmp = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
        if(tmp) {
            fprintf(stderr,"%s: Retrieve failed\n", progname);
            RETURNPFAILURE;
        }
        return PSUCCESS;

    case P_AM_LOCAL:
        return copy_file(elt(am_args, 4), lclfil, forcecopyflag);

#ifdef P_NFS
    case P_AM_NFS:
        /* XXX You must change pmap_nfs() to meet the needs of your site. */
        tmp = pmap_nfs(link->host,link->hsoname,npath, sizeof npath, am_args);
        if (!tmp) tmp = copy_file(npath, lclfil, forcecopyflag);
        return(tmp);
#endif

#ifdef P_AFS
    case P_AM_AFS:
        strcpy(npath,P_AFS);
        strcat(npath,elt(am_args,4)); /* 4th element is the hsoname. */
        return copy_file(npath, lclfil, forcecopyflag);
        return(PSUCCESS);
#endif
        
    default:
        fprintf(stderr,"%s: Can't access file using any available \
access method.\n", progname);
        RETURNPFAILURE;
    }
}



static int
copy_file(char *inname, char *outname, int forcecopyflag)
{
    struct stat st_buf;

    /* Check if the local file is a directory.  If so, don't copy it. */
    if(stat(inname, &st_buf)) {
        
        perror("%s: Can't access file, progname");
        RETURNPFAILURE;
    }
#ifdef S_ISDIR
    if (S_ISDIR(st_buf.st_mode)) {
#else
    if ((st_buf.st_mode & S_IFMT) == ST_IFDIR) {
#endif
        fprintf(stderr, "%s: Can't retrieve file %s: it is actually a \
directory.", progname, inname);
        RETURNPFAILURE;
    }
    if (isatty(0) && !forcecopyflag) {
        fprintf(stderr,
                "%s: This file is available in your native filesystem as %s.\n\
\tIf you still want to copy it to %s, invoke %s again with the -f flag.\n",
                progname, inname, outname, progname);
        RETURNPFAILURE;
    }
    if (copyFile(inname, outname)) {
	fprintf(stderr, "%s: %s", progname, p_err_string);
        RETURNPFAILURE;
    }
            return PSUCCESS;
}
