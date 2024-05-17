/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 */
#include <uw-copyright.h>
#include <usc-copyr.h>
#include <pfs.h>
#include <pserver.h>
#include <pprot.h>
#include <pparse.h>

#include <stdio.h>
#ifndef MAXPATHLEN
#ifdef CONFLICT_BETWEEN_DEFINITION_OF_NULL_IN_SYS_PARAM_H_AND_STDDEF_H
#undef NULL
#endif				/* #ifdef CONFLICT...  */
#include <sys/param.h>          /* XXX This should Change.  MAXPATHLEN is too
                                   UNIX-specific.   It is almost always 1024,
                                   but we should use MAX_VPATH instead. */
#ifdef CONFLICT_BETWEEN_DEFINITION_OF_NULL_IN_SYS_PARAM_H_AND_STDDEF_H
#ifndef NULL
#include <stddef.h>
#endif				/* #ifndef NULL */
#endif				/* #ifdef CONFLICT... */
#endif				/* #ifndef MAXPATHLEN */

extern char *rindex();

/* These are used by some other routines.  We don't ever actually use these 
   definitions, but functions that we call refer to them, although they
   never actually make it into the  files we're converting. */
char	*aftpdir = "";
char    shadow[MAXPATHLEN]    = PSRV_FSHADOW;
char	dirshadow[MAXPATHLEN] = DSHADOW;
char	dircont[MAXPATHLEN]   = DCONTENTS;
char	pfsdat[MAXPATHLEN]    = PSRV_FSTORAGE;
char	hostwport[MAXPATHLEN+30] = "THIS SHOULD NOT EVER BE USED; hostwport[]";
/* Host name w/ port if non-standard */


/* shadowcvt.c */
/* Convert v1 format shadow directory files to v5 format. */
/* read shadowcvt.doc before running this program. */

main(argc,argv)
    int argc;
    char *argv[];
{
    int numconv = 0;            /* How many files converted successfully? */
    char line[MAXPATHLEN];
    extern int fseek();

    /* server initialization routines.  Needed to set stuff to files.*/
    qoprintf = srv_qoprintf;
    stdio_fseek = fseek;

    if (argc != 3) {
        fputs("shadowcvt: usage: shadowcvt old-directory \
new-directory < file-name-listing\n", stderr);
        exit(1);
    }
    /* While there's a line with a single filename on it. */
    while(scanf(" %s ", line) == 1) {
        char infile[MAXPATHLEN], outfile[MAXPATHLEN], outdir[MAXPATHLEN] ;
        FILE *in, *out;
        char *slashp;           /* pointer to last slash in string. */

        assert(qsprintf(infile, sizeof infile, "%s/%s", argv[1], line)
               <= sizeof infile);
        assert(qsprintf(outfile, sizeof outfile, "%s/%s", argv[2], line)
               <= sizeof outfile);
        strcpy(outdir,outfile);
        if ((slashp = rindex(outdir, '/')) == NULL) {
            fprintf(stderr, "Can't find a last component in the output file \
%s.  Something is very wrong!  Aborting execution!\n", outdir);
            exit(1);
        }
        *slashp = '\0';
        /* create directory for output file  */
        if(mkdirs(outdir)) {
            fprintf(stderr, 
                    "Couldn't create a directory to hold the output file %s. \
Aborting.", outfile);
            exit(1);
        }
        /* open infile and outfile */
        if ((in = locked_fopen(infile, "r")) == NULL) {
            fprintf(stderr, "Couldn't open input file %s; aborting \
execution!\n", infile);
            exit(1);
        }
        if ((out = locked_fopen(outfile, "w")) == NULL) {
            fprintf(stderr," Couldn't open output file %s;aborting \
execution!\n", outfile);
	    locked_fclose_A(in,infile,TRUE);
            exit(1);
        }
        if(cvt_dir(infile, in, outfile, out)) {
            fprintf(stderr, "Continuing to attempt to convert directory \
information.\n");
        } else {
            numconv++;
        }
        locked_fclose_A(in,infile,TRUE);
        locked_fclose_A(out,outfile,FALSE);
    }
    exit(0);
}


static TOKEN tokenize();
static int fdsrdir_v1(char vfs_dirname[], FILE *in, VDIR dir);
int convert_v1_ltype(char l_type[], VLINK cur_link);

cvt_dir(infile, inf, outfile, outf)
    char infile[];         /* for reporting error messages */
    char outfile[];
    FILE *inf, *outf;
{
    VDIR_ST dir_st;
    VDIR dir = &dir_st;


    vdir_init(dir);

    /* Read in the directory. */

    if (fdsrdir_v1(infile, inf, dir) == 0) {
        return fdswdir_v5(outf, dir);
    }
    RETURNPFAILURE;
}


static TOKEN
tokenize(s)
    char *s;
{
    TOKEN retval;
    char buf[MAX_DIR_LINESIZE];

    int tmp = qsscanf(s, "%!!s %r", buf, sizeof buf, &s);
    if (tmp < 0)
        internal_error("Buffer overflow!");
    if (tmp == 0)
        return NULL;
    if ((retval = tkalloc(buf)) == NULL)
        internal_error("tkalloc() out of memory!");
    if (tmp == 2)
        retval->next = tokenize(s);
    return retval;
}

/* Read in a directory.  Complaints to stderr. */
static int fdsrdir_v1(char vfs_dirname[], FILE *vfs_dir, VDIR dir)
{
    char		line[MAX_DIR_LINESIZE];
    VLINK		cur_link = NULL;
    ACL		cur_acl = NULL;
    ACL		prev_acl = NULL;
    int		tmp;
    char		truefalse[16];
    int         bad_link = 0;   /* set to 1 if this link is bad and the ACL and
                                   ATTRIBUTEs associated with it should be
                                   skipped.  */

    int retval = PSUCCESS;

    while(fgets(line,MAX_DIR_LINESIZE,vfs_dir)) {
        switch(*line) {
        case 'V':
            tmp = sscanf(line,"VERSION %d",&(dir->version));
            if(tmp != 1) {
                 fprintf(stderr,"Bad directory format %s: %s",
                     vfs_dirname,line,0);
            }
            break;
        case 'M': /* Magic Number */
            tmp = sscanf(line,"MAGIC-NUMBER %d",&(dir->magic_no));
            if(tmp != 1) {
                fprintf(stderr,"Bad directory format %s: %s",
                     vfs_dirname,line,0);
            }
            break;
        case 'I':
            tmp = sscanf(line,"INCLUDE-NATIVE %s",truefalse);
            dir->inc_native = VDIN_INCLNATIVE;
            if((*truefalse == 'F') || (*truefalse == 'f'))
                dir->inc_native = VDIN_NONATIVE;
            /* REAL-ONLY - Do not include . and .. */
            if((*truefalse == 'R') || (*truefalse == 'r'))
                dir->inc_native = VDIN_INCLREAL;
            if(tmp != 1) {
                fprintf(stderr,"Bad directory format %s: %s",
                     vfs_dirname,line,0);
            }
            break;
        case 'A': {
#if 1
            char 	a_acetype[MAX_DIR_LINESIZE];
#else
            char 	a_acltype[MAX_DIR_LINESIZE];
#endif
            char	a_atype[MAX_DIR_LINESIZE];
            char 	a_rights[MAX_DIR_LINESIZE];
            char	*p_principals;
            extern char *acltypes[];

            if (strncmp(line, "ACL ", 4) && bad_link)
                break;
            cur_acl = acalloc();
            /* Code partly taken from parse_owner(). */
            p_principals = NULL;
            /* V1 dswdir() always wrote at least 3 elements. */
            if(qsscanf(line, "ACL %'!!s %'!!s %'!!s %r",
                       a_acetype, sizeof a_acetype, a_atype, sizeof a_atype,
                       a_rights, sizeof a_rights, &p_principals) < 3) {
                fprintf(stderr,"Bad directory format %s: less than 3 \
arguments to an ACL entry (skipping this ACL line, but converting the rest of \
the directory): %s",
                     vfs_dirname,line,0);
                acfree(cur_acl);
                break;
            }
            for(cur_acl->acetype = 0;acltypes[cur_acl->acetype];
                (cur_acl->acetype)++) {
                if(strequal(acltypes[cur_acl->acetype],a_acetype))
                    break;
            }
            if(acltypes[cur_acl->acetype] == NULL) {
                fprintf(stderr,"Bad directory format %s: \
couldn't find acl type %s (skipping this ACL line, but converting the rest of \
the directory): %s",
                     vfs_dirname,a_acetype, line,0);
                acfree(cur_acl);
                break;
            }
            cur_acl->atype = stcopyr(a_atype, cur_acl->atype);
            cur_acl->rights = stcopyr(a_rights,cur_acl->rights);
            cur_acl->principals = tokenize(unquote(p_principals));
            if(prev_acl) {
                prev_acl->next = cur_acl;
                cur_acl->previous = prev_acl;
            }
            else if(cur_link) cur_link->acl = cur_acl;
            else dir->dacl = cur_acl;

            prev_acl = cur_acl;
        }
            break;

        case 'L': {
            char 	l_name[MAX_DIR_LINESIZE];
            char	l_type[MAX_DIR_LINESIZE];
            char 	l_htype[MAX_DIR_LINESIZE];
            char 	l_host[MAX_DIR_LINESIZE];
            char 	l_ntype[MAX_DIR_LINESIZE];
            char 	l_fname[MAX_DIR_LINESIZE];

            prev_acl = NULL; /* So next acl entry is for a new link */
            bad_link = 0;       /* assume this link is 
                                   good until proven guilty :) */

            cur_link = vlalloc();
            cur_link->f_magic_no = 0; /* in case not set */
            tmp = qsscanf(line,
                          "LINK %'s %c %s %d %*d %s %'s %s %'s %d %d",
                         l_name, &(cur_link->linktype), l_type,
                         &(cur_link->dest_exp),
                         /* &(cur_link->link_exp), */ 
                         l_htype,l_host,
                         l_ntype,l_fname,
                         &(cur_link->version),
                         &(cur_link->f_magic_no));
            if(tmp >= 9) {
                cur_link->name = stcopy(l_name);
                cur_link->target = stcopyr(l_type, cur_link->target);
                if(convert_v1_ltype(l_type, cur_link)) {
                    bad_link = 1;
                    fprintf(stderr, "Directory %s: Did not understand LINK of \
type %s (skipping this link and associated information, but converting the \
rest of the directory): %s", vfs_dirname, l_type, line);
		    vlfree(cur_link); cur_link = NULL;
                    break;
                }
                   
                stfree(cur_link->hosttype); /* Should punt if same */
                cur_link->hosttype = stcopy(l_htype);
                cur_link->host = stcopy(l_host);
                stfree(cur_link->hsonametype); /* Should punt if same */
                cur_link->hsonametype = stcopy(l_ntype);
                cur_link->hsoname = stcopy(l_fname);
                /* if ul, then vl_insert will call ul_insert */
                vl_insert(cur_link,dir,VLI_ALLOW_CONF);
		/* No need to free cur_link once inserted into dir */
            }
            else {
                fprintf(stderr,"Bad directory format %s: %s",
                     vfs_dirname,line,0);
                vlfree(cur_link); cur_link = NULL;
            }
        }
            break;

        case 'F': { /* Filter attached to previous link */
            char	f_type;
            char 	f_htype[MAX_DIR_LINESIZE];
            char 	f_host[MAX_DIR_LINESIZE];
            char 	f_ntype[MAX_DIR_LINESIZE];
            char        f_fname[MAX_DIR_LINESIZE];
            int		f_vers = 0;
            int		f_mno = 0;
            char 	f_args[MAX_DIR_LINESIZE];
            FILTER		cur_fil;

            if (strnequal(line, "FILTER ", 7) && bad_link)
                break;
            if(!cur_link) {
                fprintf(stderr, "No reference to previous link.  Can't add filter %s: %s",
                     vfs_dirname,line,0);
               break; 
            }


            cur_fil = flalloc();
            cur_fil->execution_location = FIL_CLIENT;
            cur_fil->link = vlalloc();
            f_vers = 0; f_mno = 0;
            tmp = sscanf(line,"FILTER %c %s %s %s %s %d %d ARGS ' %[^']",
                         &f_type, f_htype, f_host, f_ntype, f_fname,
                         &f_vers, &f_mno, f_args);
            if(tmp >= 5) {
                cur_fil->pre_or_post = FIL_POST;
                switch(f_type) {
                case 'd':
                    cur_fil->pre_or_post = FIL_PRE;
                case 'D':
                    cur_fil->type = FIL_DIRECTORY;
                    break;
                case 'h':
                    cur_fil->pre_or_post = FIL_PRE;
                case 'H':
                    cur_fil->type = FIL_HIERARCHY;
                    break;
                case 'O':
                    cur_fil->type = FIL_OBJECT;
                    break;
                case 'U':
                    cur_fil->type = FIL_UPDATE;
                    break;
                default:
                    fprintf(stderr, "Bad directory format %s: unknown filter \
type %c: %s", vfs_dirname, f_type, line);
		    flfree(cur_fil); cur_fil = NULL;
                    break;
                }

                cur_fil->link->linktype = f_type;
                cur_fil->link->hosttype = stcopyr(f_htype,cur_fil->link->hosttype);
                cur_fil->link->host = stcopy(f_host);
                stfree(cur_fil->link->hsonametype); /* Should punt if same */
                cur_fil->link->hsonametype = stcopy(f_ntype);
                cur_fil->link->hsoname = stcopy(f_fname);
                cur_fil->link->version = f_vers;
                cur_fil->link->f_magic_no = f_mno;
                if(tmp == 8) cur_fil->args = tokenize(f_args);

                fl_insert(cur_fil,cur_link);
            }
            else {
                fprintf(stderr,"Bad directory format %s: %s",
                     vfs_dirname,line,0);
                flfree(cur_fil); cur_fil = NULL;
            }
            
        } /* Case 'F': */
            break;

        default: {
            fprintf(stderr,"Bad directory format %s: %s", vfs_dirname,line,0);
        }
        } /* switch */ 
    } /* while */
    return retval;
}
