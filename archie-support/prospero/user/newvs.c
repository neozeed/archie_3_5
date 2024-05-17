/*
 * Copyright (c) 1991, 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 */

#include <usc-license.h>
#include <psite.h>

#define VERBOSE

/*
 * newvs - Create new virtual systems
 *
 * Verbosity flag:  0 - Prompt for input
 *                  1 - Explain what is needed as input
 *                  2 - List each action taken
 *                  3 - Stop before each step
 *                  4 - Stop before each step, and explain the action
 */

/* make more efficinet by caching some of the dir pointers */

#include <stdio.h>
#include <string.h>

#include <pfs.h>
#include <perrno.h>

#define max(x,y)  (((x) > (y)) ? x : y)

char *prog;
int	pfs_debug = 0;

void
main(argc,argv)
    int		argc;
    char	*argv[];
{
    char		*progname = argv[0];
    char		*cur_arg;

    char		vshost[100];
    char		vsname[100];
    char		home[100];
    char		vsdescfile[100];
    char		buf[100]; /* used as temporary. */
    char		owner[1000];

    FILE		*vsdesc;

    char		vsnamepath[100];
    char		vsabsname[100];

    char		vs_root[MAX_VPATH];
    char		vs_subdir[MAX_VPATH];
    char		vs_temp[MAX_VPATH];

    char		cont[100];

    VDIR_ST		dir_st;         
    VDIR		dir= &dir_st;
    VLINK		pl; 		/* Prototype links */
    VLINK		vl;

    ACL_ST	oacl_st;		/* Owner ACL */
    ACL	oacl = &oacl_st;	/* Owner ACL pointer */

    int		empty = 0;

#ifdef VERBOSE
    int		verbose = 2;
#endif VERBOSE

    char		*period;
    int		nchr;
    int		retval;

    prog = argv[0];

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    vdir_init(dir);

    *vshost = '\0';
    *vsname = '\0';
    *home = '\0';
    *owner = '\0';
    *vsdescfile = '\0';

    argc--;argv++;

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

            case 'e': /* Empty */
                empty++;
                break;

#ifdef VERBOSE
            case 'v':  /* Verbosity */
            case 'V':  /* Verbosity */
                verbose = 1; /* Default debug level */
                sscanf(cur_arg,"%d",&verbose);
                cur_arg += strspn(cur_arg,"0123456789");
                break;
#endif VERBOSE

            default:
                fprintf(stderr,
                        "Usage: newvs [-D[#]] [-e] [-v[#]] [host [name [home [owner [descfile]]]]]\n");
                exit(1);
            }
        }
        argc--, argv++;
    }
scandone:

    if (argc > 5)  {
        fprintf(stderr,
                "Usage: newvs [host [name [home [owner [descfile]]]]]\n");
        exit(1);
    }

    /* check for args in order host, vsname, homedir */
    /* If still some left, print error message */
    if (argc > 0) strcpy(vshost,argv[1]);
    if (argc > 1) strcpy(vsname,argv[2]);
    if (argc > 2) strcpy(home,argv[3]);
    if (argc > 3) strcpy(owner,argv[4]);
    if (argc > 4) strcpy(vsdescfile,argv[5]);

#ifdef VERBOSE
    if(verbose >= 2) printf("\nSetting virtual system to %s...",P_MASTER_VS);
#endif VERBOSE

    sprintf(vs_temp,"/VIRTUAL-SYSTEMS/%s",P_MASTER_VS);
    if(pget_wdhost()) vfsetenv("","",vs_temp);
    else vfsetenv("","",P_MASTER_VS);

#ifdef VERBOSE
    if(verbose >= 2) printf("Done.\n");

    if(verbose >= 2) printf("Reading list of storage sites from /pfs_storage...");
#endif VERBOSE

    retval = rd_vdir("/pfs_storage",0,dir,0);

    if(retval) {
        perrmesg("Can't read /pfs_storage: ", retval, NULL);
        exit(1);
    }

#ifdef VERBOSE
    if(verbose >= 2) printf("Done.\n");

    if((verbose >= 1) && (*vshost == '\0')) {
        printf("\nHost is the name of the host on which the root\n");
        printf("of the new virtual system is to be stored.  For a\n");
        printf("list of choices type a question mark.\n\n");
    }
#endif VERBOSE

prompthost:
    if(*vshost == '\0') {
        printf("Host: ");
        if(gets(vshost) == NULL)
            exit(1);
    }

    if((*vshost == '\0') || (strcmp(vshost,"?") == 0)) {
        vl = dir->links;

        printf("\n");
        while(vl) {
            printf(" %s\n",vl->name);
            vl = vl->next;
        }
        printf("\n");
        *vshost = '\0';
        goto prompthost;
    }

    vl = dir->links;

    while(vl) {
        nchr = 0; period = strchr(vl->name,'.');
        if(period) nchr = period - vl->name;
        nchr = max(strlen(vshost), nchr);

        if(strcncmp(vl->name,vshost,nchr) == 0) break;
        vl = vl->next;
    }

    if(vl == NULL) {
        printf("Not found.\n");
        *vshost = '\0';
        goto prompthost;
    }

    sprintf(vs_root,"/pfs_storage/%s/local_vsystems/",vl->name);

#ifdef VERBOSE
    if((verbose >= 1) && (*vsname == '\0')) {
        printf("\nName is the default name by which others will refer\n");
        printf("to the new virtual system.  If this is your personal\n");
        printf("virtual system, it should be your username.  If this\n");
        printf("virtual system is for a project, a name describing the\n");
        printf("project is appropriate.  Additional names may be\n");
        printf("added at a later time.\n\n");
    }
#endif VERBOSE

promptvsname:
    if(*vsname == '\0') {
        printf("Name: ");
        if(gets(vsname) == NULL)
            exit(1);
    }

    if(*vsname == '\0') goto promptvsname;

    sprintf(vsnamepath,"/%s/VIRTUAL-SYSTEMS/%s",P_PROTOTYPE_VS,vsname);

    retval = rd_vdir(vsnamepath,0,dir,RVD_DFILE_ONLY);

    if((retval == 0) && dir->links) {
#ifdef VERBOSE	   
        if(verbose >= 1) {
            printf("\nThe name you have chosen is already in use.\n");
            printf("This means that the name is already being used\n");
            printf("by local users to refer to a different virtual\n");
            printf("system.  Please choose a different name.  You\n");
            printf("may add an additional name at a later time, \n");
            printf("including one that is already in use.\n\n");
        }
        else 
#endif VERBOSE
            printf("Name already in use, please choose another.\n");

        *vsname = '\0';
        goto promptvsname;
    }

    strcat(vs_root,vsname);

#ifdef VERBOSE
    if((verbose >= 1) && (*home == '\0')) {
        printf("\nHome is the name of the home directory relative\n");
        printf("to the root of the new virtual system.  If this is\n");
        printf("a personal virtual system, \"/vsname\" where vsname is\n");
        printf("the name of the virtual system is suggested.  If the\n");
        printf("directory is top level and does not already exist,\n");
        printf("then it will be automatically created.\n\n");
    }
#endif VERBOSE

prompthome:
    if(*home == '\0') {
        printf("Home Directory: ");
        if(gets(home) == NULL) exit(1);
    }

    if(*home == '\0') goto prompthome;

    /* Need better checking for valid names */
    if(*home != '/') {
        printf("Path must begin with a /.\n");
        *home = '\0';
        goto prompthome;
    }

#ifdef VERBOSE
    if((verbose >= 1) && (*owner == '\0')) {
        printf("\nOwner is a list of users authorized to change\n");
        printf("the new virtual system.  The owner will be granted\n");
        printf("full rights in the directories that are created.\n");
        printf("The owner or owners will get an ACL entry of the type ASRTHOST.\n");
    }
#endif VERBOSE

promptowner:
    if(*owner == '\0') {
        printf("Owner: ");
        if(gets(owner) == NULL) exit(1);
    }

    if(*owner == '\0') goto promptowner;

    /* Start constructing systems */

#ifdef VERBOSE
    if(verbose == 3) 
        printf("\nAbout to create root of virtual system.\n[Type <CR> to continue]");

    if(verbose >= 4) {
        printf("\nAbout to create the root of the virtual system.\n");
        printf("This is accomplished by creating the directory:\n\n");
        printf("    %s\n\n",vs_root);
        printf("relative to the root of the master virtual system.\n");
        printf("[Type <CR> to continue]");
    }

    if(verbose >= 3) {
        gets(cont);
        printf("\n");
    }

    if(verbose >= 2) 
        printf("Creating %s...",vs_root);
#endif VERBOSE

    retval = mk_vdir(vs_root,MKVD_LPRIV);
    if(retval == DIRSRV_NOT_AUTHORIZED) {
        fflush(stdout);
        perrmesg("\nUnable to create virtual system: ", retval, NULL);
        exit(1);
    }
    if(retval && (retval != DIRSRV_ALREADY_EXISTS)) {
        perrmesg("Error creating root: ", retval, NULL);
        exit(1);
    }

#ifdef VERBOSE
    if(verbose >= 2) printf("Done.\n");
    if(verbose >= 2) printf("Setting ACL...");
#endif VERBOSE

    retval = rd_vdir(vs_root,0,dir,RVD_DFILE_ONLY);

    oacl->acetype = ACL_ASRTHOST;
    oacl->atype = "";
    oacl->rights = "ALRMDI";
    oacl->principals = qtokenize(owner);
    if(!retval) 
        retval = modify_acl(dir->links,NULL,oacl,EACL_DIRECTORY|EACL_ADD);

    if(retval) {
        perrmesg("Error setting ACL: ", retval, NULL);
        exit(1);
    }

#ifdef VERBOSE
    if(verbose >= 2) printf("Done.\n");
    if(verbose >= 2) printf("Reading /%s...",P_PROTOTYPE_VS);
#endif VERBOSE
    sprintf(vs_temp,"/%s",P_PROTOTYPE_VS);
    retval = rd_vdir(vs_temp,0,dir,RVD_EXPAND);
    if(retval) {
        perrmesg("Error reading prototype: ", retval, NULL);
        exit(1);
    }

#ifdef VERBOSE
    if(verbose >= 2) printf("Done.\n\n");
#endif VERBOSE

    if(empty) goto leave_empty;

#ifdef VERBOSE
    if(verbose == 3) printf("About to setup subdirectories.\n[Type <CR> to continue]");

    if(verbose >= 4) {
        printf("About to setup subdirectories.  For each subdirectory in\n");
        printf("/%s, a new subdirectory will be created in the new\n",P_PROTOTYPE_VS);
        printf("virtual system.  The subdirectory from /%s will\n",P_PROTOTYPE_VS);
        printf("then be added as a union link.  This creates a customizable\n");
        printf("subdirectory whose contents track that of the master copy.\n");
        printf("[Type <CR> to continue]");
    }

    if(verbose >= 3) {
        if(gets(cont) == NULL) exit(1);
        printf("\n");
    }
#endif VERBOSE

    pl = dir->links;

    while(pl) {
        if(strcmp(pl->name,"VS-DESCRIPTION") != 0) {
            sprintf(vs_subdir,"%s/%s",vs_root,pl->name);
#ifdef VERBOSE
            if(verbose >= 2) printf("Creating %s...",vs_subdir);
#endif VERBOSE
            retval = mk_vdir(vs_subdir,MKVD_LPRIV);
            if(retval && (retval != DIRSRV_ALREADY_EXISTS)) {
                perrmesg("Error creating subdir: ", retval, NULL);
                exit(1);
            }

#ifdef VERBOSE
            if(verbose >= 2) printf("Adding union link to master copy...");
#endif VERBOSE
            retval = add_vlink(vs_subdir,"master",pl,AVL_UNION);
            if(retval && (retval != DIRSRV_ALREADY_EXISTS)) {
                perrmesg("Error adding union link: ", retval, NULL);
                exit(1);
            } 
#ifdef VERBOSE
            if(verbose >= 2) printf("Done.\n");
#endif VERBOSE
        }
        pl = pl->next;
    }

leave_empty:

    if(strcmp(home,"/") == 0) goto no_home;

    sprintf(vs_subdir,"%s%s",vs_root,home);

#ifdef VERBOSE
    if(verbose >= 3) printf("\nAbout to create home directory.\n[Type <CR> to continue]");

    if(verbose >= 3) {
        if(gets(cont) == NULL) exit(1);
        printf("\n");
    }

    if(verbose >= 2) printf("Creating %s...",vs_subdir);
#endif VERBOSE

    retval = mk_vdir(vs_subdir,MKVD_LPRIV);
    if(retval) perrmesg("Home directory not created: ", retval, NULL);

#ifdef VERBOSE
    if(verbose >= 2) printf("Done %s.\n",vs_subdir);
#endif VERBOSE

no_home:

#ifdef VERBOSE
    if(verbose == 3) printf("\nAbout to create VS-DESCRIPTION.\n[Type <CR> to continue]");

    if(verbose >= 4) {
        printf("\nAbout to create VS-DESCRIPTION.  This will contain information\n");
        printf("about the root and home directory of the virtual system.\n");
        printf("[Type <CR> to continue]");
    }

    if(verbose >= 3) {
        if(gets(cont) == NULL) exit(1);
        printf("\n");
    }

    if(verbose >= 2) printf("Creating /VS-DESCRIPTION...");
#endif VERBOSE

    /* Create the VS-DESCRIPTION */
    sprintf(vs_subdir,"%s/VS-DESCRIPTION",vs_root);
    retval = mk_vdir(vs_subdir,MKVD_LPRIV);
    if(retval && (retval != DIRSRV_ALREADY_EXISTS)) {
        perrmesg("Error creating subdir: ", retval, NULL);
        exit(1);
    }

    /* create ROOT link */
    sprintf(vs_subdir,"%s/VS-DESCRIPTION",vs_root);

    retval = rd_vdir(vs_root,0,dir,RVD_DFILE_ONLY);

#ifdef VERBOSE
    if(verbose >= 2) printf("Done.\nAdding ROOT...");
#endif VERBOSE

    retval = add_vlink(vs_subdir,"ROOT",dir->links,0);
    if(retval) 
        perrmesg("Error adding link to VS-DESCRIPTION: ", retval, NULL);

#ifdef VERBOSE
    if(verbose >= 2) printf("Done.\nAdding HOME...");
#endif VERBOSE

    vl = vlalloc();		/* LEAKS */

    vl->name = stcopy("HOME");
    vl->target = stcopyr("SYMBOLIC", vl->target);
    vl->hosttype = stcopyr("VIRTUAL-SYSTEM", vl->hosttype);
    vl->hsoname = stcopyr(home, vl->hsoname);

    /* Should LOCAL-VSLIST really be in prototype, or should it */
    /* be in master.  I think in prototype, but should it       */
    /* somehow encoded in HOME                                  */

    /* Clean up this code so we don't use dir->links */
    sprintf(vs_temp,"/%s/VS-DESCRIPTION/LOCAL-VSLIST", P_PROTOTYPE_VS);
    vllfree(dir->links);
    dir->links = rd_slink(vs_temp);
    if(!dir->links) {
        fprintf(stderr,"Can't find prototype:/VS-DESCRIPTION/LOCAL-VSLIST\n");
    }
    /* hostname - need to figure it out in a better way */
    sprintf(vsabsname,"%s/%s",dir->links->host,vsname);
    vl->host = stcopy(vsabsname);

    retval = add_vlink(vs_subdir,"HOME",vl,0);
    if(retval) 
        perrmesg("Error adding VS-DESCRIPTION link: ", retval, NULL);

#ifdef VERBOSE
    if(verbose >= 2) printf("Done.\n");

    if(verbose == 3) 
        printf("\nAbout to add VS-DESCRIPTION to local VS list.\n[Type <CR> to continue]");

    if(verbose >= 4) {
        printf("\nThe description of the new virtual system has been created, \n");
        printf("but it has not yet been inserted into the global namespace.\n");
        printf("We do this by adding the description to the local VS list.\n");
        printf("This provides the VS with a default local name, and automatically\n");
        printf("gives it a longer global name as well.  The global name is\n");
        printf("The path from a well known point to to the local VS list, plus\n");
        printf("the local name of the system.\n");

        printf("[Type <CR> to continue]");
    }

    if(verbose >= 3) {
        if(gets(cont) == NULL) exit(1);
        printf("\n");
    }

    if(verbose >= 2) 
        printf("Adding to local VS list...");
#endif VERBOSE


    /* add a directory pointer to /VIRTUAL-SYSTEMS/#       */
    /* This is done now through /prototype/VIRTUAL-SYSTEMS */
    /* which is a pointer to the correct subdirectory      */
    /* It might be better to start from # and traverse the */
    /* whole path, but it would be slower.  Also should we */
    /* start from prototype or master/VIRTUAL-SYSTEMS/#?   */

    /* create ROOT link */
    sprintf(vs_subdir,"%s/VS-DESCRIPTION",vs_root);
    retval = rd_vdir(vs_subdir,0,dir,RVD_DFILE_ONLY);

    if(retval == 0) {
        sprintf(vs_temp,"/%s/VIRTUAL-SYSTEMS",P_PROTOTYPE_VS);
        retval = add_vlink(vs_temp,vsname,dir->links,0); 
    }
#ifdef VERBOSE
    if(verbose >= 2) printf("Done.\n");
#endif VERBOSE


    if(strcmp(vsdescfile,"-") == 0) goto nodesc;

    if((verbose >= 1) && (*vsdescfile == '\0')) {
        printf("\nDo you want to create a virtual system description file?\n");
        printf("The virtual system description file can be read by\n");
        printf("vfsetup to find the description for this virtual\n");
        printf("It is not absolutely necessary, because the\n");
        printf("description can also be found by name.  If this is\n");
        printf("your primary virtual system, then it is suggested that\n");
        printf("you create a virtual system description file named\n");
        printf("\"~/.virt-sys\".  This file is read when vfsetup is\n");
        printf("called without arguments.\n\n");
    }

promptdescf:
    if(*vsdescfile == '\0') {
        printf("Create a virtual system description file (y/n)? ");
        if(gets(vsdescfile) == NULL) exit(1);
    }

    if((*vsdescfile == 'n') || (*vsdescfile == 'N')) goto nodesc;

    if((*vsdescfile != 'y') && (*vsdescfile != 'Y')) {
        *vsdescfile = '\0';
        goto promptdescf;
    }

    printf("Description file [~/.virt-sys]: ");
    if(gets(vsdescfile) == NULL) exit(1);

    if(*vsdescfile == '\0') {
        strcpy(vsdescfile,"~/.virt-sys");
    }
    if (*vsdescfile == '~') {
        extern char *getenv();

        strcpy(buf, getenv("HOME"));
        strcat(buf, vsdescfile + 1);
        strcpy(vsdescfile, buf);
    }
    if((vsdesc = fopen(vsdescfile,"w")) == NULL) {
            fprintf(stderr,"%s: Can't open system description file for write -
%s; no description file written.  All other changes still remain in effect.\n",
                    progname,vsdescfile);
            goto nodesc;
        }

    /* Find VS-DESCRIPTION again */
    sprintf(vs_subdir,"%s/VS-DESCRIPTION",vs_root);
    retval = rd_vdir(vs_subdir,0,dir,RVD_DFILE_ONLY);

    if(!retval)
        fprintf(vsdesc,"%s %s\n",dir->links->host,dir->links->hsoname);
    else fprintf(stderr,"%s: Can't find VS-DESCRIPTION\n", progname);

    fclose(vsdesc);

nodesc:
    exit(0);

}
