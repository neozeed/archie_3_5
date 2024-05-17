/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>

#include <stdio.h>
#include <string.h>
#include <pfs.h>
#include <perrno.h>
#include <pmachine.h>

char *prog;
int	pfs_debug = 0;

/*
 * P__VCD:  Intended to be called from the alias VCD
 */

void
main(int argc,char *argv[])

{
    VDIR_ST		dir_st;
    VDIR		dir= &dir_st;
    VLINK		vl;
    char		*progname = argv[0];
    char		newdir[MAX_VPATH];

    char		newpath[MAX_VPATH];
    char		ts[MAX_VPATH]; /* Temporary */
    char		*prefix;
    char		*suffix;
    char		*colon;
    char		*slash;
    char		*stmp;

    int		uflag = 0;
    int		tmp;
    int shellflag;
    char *shellname = NULL;

    prog = argv[0];

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    for (;;) {
        if ((argc > 1) && (strncmp(argv[1],"-D",2) == 0)) {
            pfs_debug = 1; /* Default debug level */
            sscanf(argv[1],"-D%d",&pfs_debug);
            argc--;argv++;
            continue;
        }
        if ((argc > 2) && strequal(argv[1], "-s")) {
            shellname = argv[2];
            argc--; argv++;
            argc--; argv++;
            continue;
        }
        if ((argc > 1) && (strcmp(argv[1],"-u") == 0)) {
            uflag++;
            argc--;argv++;
            continue;
        }
        break;                  /* no option matched.  */
    }
    if (argc > 2) {
        fprintf(stderr,"Too many arguments.\n");
        exit(1);
    }
    shellflag = p__get_shellflag(shellname);
    if (!shellflag) {
        fprintf(stderr, "p__vcd: %s\n", p_err_string);
        exit(1);
    }

    vdir_init(dir);

    if ((argc == 1) || (!strcmp(argv[1],"~")) || (!strcmp(argv[1],"~/"))) {
        pset_wd(pget_hdhost(),pget_hdfile(),pget_hd());
        p__print_shellstring(shellflag | 0x04);	    
        exit(0);
    }

    if (!strcmp(argv[1],"/")) {
        pset_wd(pget_rdhost(),pget_rdfile(),"/");
        p__print_shellstring(shellflag | 0x04);	    
        exit(0);
    }

    strcpy(newdir,argv[1]);
    stmp = pget_wd();
    if(stmp) strcpy(newpath,stmp);
    else  {
        perrmesg(progname, PFS_ENV_NOT_INITIALIZED, NULL);
        exit(1);
    }

    if(uflag) {
        /* if -u option, then only a single component allowed */
        /* should eventally allow it and interpret the last   */
        /* component as a union link.                         */
        if(p_uln_index(argv[1],'/') || p_uln_index(argv[1], ':')) {
            fprintf(stderr,"Path for -u can not contain a / or :\n");
            exit(1);
        }
        tmp = rd_vdir("",0,dir,GVD_UNION);

        if(tmp || (!dir->ulinks)) {
            fprintf(stderr,"%s: %s is not a union link\n",
                    progname,argv[1]);
            exit(1);
        }

        vl = dir->ulinks;

        while(vl) {
            if(strcmp(vl->name,argv[1]) == 0) {
                strcat(newpath,"/#");
                strcat(newpath,argv[1]);
                pset_wd(vl->host,vl->hsoname,newpath);
                p__print_shellstring(shellflag | 0x04);	    
                vllfree(dir->links);
                vllfree(dir->ulinks);
                exit(0);
            }
            vl = vl->next;
        }
        fprintf(stderr,"%s: %s is not a union link\n",
                progname,argv[1]);
        vllfree(dir->links);
        vllfree(dir->ulinks);
        exit(1);
    }

    else {
        tmp = rd_vdir(newdir,0,dir,RVD_DFILE_ONLY);

        if(tmp || (!dir->links)) {
            fprintf(stderr,"%s: %s is not a directory%s\n",progname,argv[1],
                    tmp == RVD_DIR_NOT_THERE ? 
                    " (this may be a temporary failure; try again later)":"");
            exit(1);
        }

        prefix = newpath;
        suffix = argv[1];

        /* If arg is relative to root or another namespace, then  */
        /* prefix becomes arg and suffix "" so that we don't have */
        /* to check the rest                                      */
        if((*suffix == '/') || ((colon = p_uln_index(suffix,':')) &&
                                (*(colon+1) != ':'))) {
            prefix = suffix;
            suffix = "";
        }

        if(strncmp(suffix,"~/",2) == 0) {
            /* Set wdname and later check for ../'s */
            strcpy(newpath,pget_hd());
            suffix++;
            suffix++;
        }

        /* If we still allow ../'s and the dirname starts  */
        /* as such, determine the correct name for the new */
        /* file relative to VSROOT, and use that instead   */
        while((strncmp(suffix,"../",3) == 0) || 
              (strcmp(suffix,"..") == 0)) {
            suffix += 2;
            if(*suffix == '/') suffix++;

            slash = p_uln_rindex(prefix,'/');
            colon = p_uln_rindex(prefix,':');

            if(slash && (!colon || (slash > colon))) *slash = '\0';
            else if(colon) *(colon+1) = '\0';
            else prefix = "";
            if (!*prefix) prefix = "/";
        }

        /* If we still have a ./ left, remove it */
        if(strncmp(suffix,"./",2) == 0) {
            suffix++;
            suffix++;
        }

        if(*suffix && (*(prefix + strlen(prefix)-1) != '/'))
            strcat(prefix,"/");

        strcpy(ts,prefix);
        strcat(ts,suffix);

        pset_wd(dir->links->host,dir->links->hsoname,ts);
        p__print_shellstring(shellflag | 0x04);	    
    }

    vllfree(dir->links);

    exit(0);
}

