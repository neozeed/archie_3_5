/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 *  <usc-license.h>
 */

#include <usc-license.h>

#include <stdio.h>
#include <string.h>

#include <psite.h>
#include <pfs.h>
#include <pcompat.h>
#include <perrno.h>
#include <pmachine.h>


char *prog;
int	pfs_debug = 0;

char	*getenv();

/* P__VFSETUP: This implements the user command 'vfsetup'
 * It is intended to be called from an alias named 'vfsetup'.
 */
  

void
main(int argc,char *argv[])
{
    int		reset = 0;
    int		tmp;

    char		vsdesc_host[100];
    char		vsdesc_file[100];
    char		vsname[100];
    char		*progname = argv[0];
    char		vsfname[100];
    FILE		*vsdesc;
    char		*colon;
    int         shellflag;          /* shell flag for p__print_shellstring() */
    char                *shellname = NULL; /* passed to p__get_shellflag() */
    
    prog = progname;

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    *vsdesc_host = '\0';
    *vsdesc_file = '\0';
    *vsname = '\0';

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
        break;                  /* neither option matched.  */
    }
    if((argc == 4) && (strcmp(argv[1],"-n") == 0)) {
        /* parse the native values */
        strcpy(vsdesc_host,argv[2]);
        strcpy(vsdesc_file,argv[3]);
    }
    else if ((argc == 3) && (strcmp(argv[1],"-r") == 0)) {
        /* parse name of the vs and look it up, but ignore    */
        /* the present virtual system, and do the search from */
        /* the initial one. (reset to starting configuration) */
        strcpy(vsname,argv[2]);
        reset = 1;
    }
    else if ((argc == 3) && (strcmp(argv[1],"-v") == 0)) {
        /* parse name of the vs and look it up, but treat the */
        /* name as an absolute name relative to current       */
        /* location instead of relative to VIRTUAL-SYSTEMS    */
        strcpy(vsname,argv[2]);
    }
    else if (argc == 2) {
        /* parse the name of the vs and look it up.           */
        /* Kludge: if VSWORK_HOST is defined, then the search */
        /* will be relative to our current virtual system, so */
        /* the name should be prepended with /VIRTUAL-SYSTEMS */
        /* Aditionally, if the name includes any namespace    */
        /* operators, they should all precede the /VIRT...    */
        strcpy(vsname,argv[1]);
        colon = strrchr(vsname,':');
        if(colon) *(colon+1) = '\0';
        else *vsname = '\0';
        strcat(vsname,(pget_wdhost() ? "/VIRTUAL-SYSTEMS/" : ""));
        colon = strrchr(argv[1],':');
        if(colon) strcat(vsname,colon+1);
        else strcat(vsname,argv[1]);
    }
    else if ((argc == 1) || ((argc == 3) && (strcmp(argv[1],"-f") == 0))) {
        if(argc == 1) {
            strcpy(vsfname,getenv("HOME"));
            strcat(vsfname,"/.virt-sys");
        }
        else strcpy(vsfname,argv[2]);
        if((vsdesc = fopen(vsfname,"r")) == NULL) {
            fprintf(stderr,"%s: Can't open system description file - %s\n",
                    progname,vsfname);
            exit(1);
        }

        fscanf(vsdesc,"%s %s",vsdesc_host,vsdesc_file);

        fclose(vsdesc);

    }
    else {
        fprintf(stderr,"usage: %s [-s shellname ] [-n host file, -f file, [-r,v] vsname]\n",
                progname);
        exit(1);
    }

    tmp = vfsetenv((reset ? NULL : vsdesc_host),
                   (reset ? NULL : vsdesc_file),
                   vsname);

    if(tmp == VFSN_NOT_A_VS) {
        fprintf(stderr,"%s: %s is not a virtual system\n",progname,
                (*vsname ? vsname : vsdesc_file));
        exit(1);
    }
    else if(tmp) {
        fprintf(stderr,"%s",progname);
        perrmesg(" failed: ", tmp, NULL);
        exit(1);
    }

    if(pwarn) pwarnmesg("WARNING: ",0,NULL);

    shellflag = p__get_shellflag(shellname);
    if (!shellflag) {
        fprintf(stderr, "vfsetup: %s\n", p_err_string);
        exit(1);
    }
    p__print_shellstring(0x1F | shellflag);
    exit(0);
}

