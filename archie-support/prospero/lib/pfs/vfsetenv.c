/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>
#include <pfs.h>
#include <psite.h>
#include <pmachine.h>
#include <perrno.h>
#ifndef NULL
#define NULL 0
#endif

extern int pfs_debug;

int
vfsetenv(char *hostname, char *filename, char *path)
{
    VDIR_ST		dir_st;
    VDIR		dir= &dir_st;
    VLINK		vl;

    int		tmp;
    int		foundroot = 0;

    static char		ts[100];         /* Temporary string */

    static char		vsdesc_host[100];
    static char		vsdesc_file[100];
    static char		vshome[100];

    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
    vdir_init(dir);

    /* if hostname is "", but there is no definition for VSWORK_HOST */
    /* in the environment, then treat hostname and filename as is    */
    /* they are NULL.  That way, the default will be used.           */
    /*                                                               */
    /* ASSUMPTION: If VSWORK_HOST is non null, then the rest of the  */
    /* environment is probably filled in as well.  We should really  */
    /* check all necessary variables                                 */
    if(hostname && (*hostname == '\0') && (pget_wdhost() == NULL)){
        hostname = NULL;
        filename = NULL;
    }

    /* If a non-empty hostname has been specified, then set the     */
    /* starting point for the host and filenames.  If the hostname  */
    /* NULL, then use the default.                                  */
    if((hostname == NULL) || (*hostname != '\0')) {
        if(!filename) 
            if (qsprintf(ts, sizeof ts, 
                     "%s/%s",P_SITE_DIRECTORY,P_SITE_MASTER_VSLIST) 
                > sizeof ts)
                internal_error("internal buffer ts is too small!");

        pset_rd((hostname ? hostname : P_SITE_HOST),
                (filename ? filename : ts)); 

        pset_wd((hostname ? hostname : P_SITE_HOST),
                (filename ? filename : ts), 
                "/");

        strcpy(vshome,"/");
        pset_hd((hostname ? hostname : P_SITE_HOST),
                (filename ? filename : ts), 
                "/");
    }

    tmp = rd_vdir((path ? path : ""),0,dir,RVD_DFILE_ONLY);

    if(tmp) return(tmp);
    if(!dir->links) return(VFSN_NOT_A_VS);

    strcpy(vsdesc_host,dir->links->host);
    strcpy(vsdesc_file,dir->links->hsoname);

    tmp = rd_vdir((path ? path : ""),0,dir,0);

    if(tmp) return(tmp);
    if(!dir->links) return(VFSN_NOT_A_VS);

    vl = dir->links;

    pset_desc(vsdesc_host,vsdesc_file,NULL);

    foundroot = 0;

    while(vl) {
        if(vl->name && strequal(vl->name,"ROOT")) {
            if (!strequal(vl->target, "DIRECTORY")) {
                if (pfs_debug)
                    fprintf(stderr, "vfsetenv(): ROOT link in \
VS-DESCRIPTION must be DIRECTORY"); 
                vdir_freelinks(dir);
                return VFSN_NOT_A_VS;
            }
            pset_rd(vl->host,vl->hsoname);
            foundroot++;
        }
        if(vl->name && strequal(vl->name,"HOME")) {
            if (!strequal(vl->target, "SYMBOLIC")) {
                if (pfs_debug)
                    fprintf(stderr, "vfsetenv(): ROOT link in \
VS-DESCRIPTION must be SYMBOLIC"); 
                vdir_freelinks(dir);
                return VFSN_NOT_A_VS;
            }
            pset_desc(NULL,NULL,vl->host);
            pset_wd(NULL,NULL,vl->hsoname);
            pset_hd(NULL,NULL,vl->hsoname);
            strcpy(vshome,vl->hsoname);
        }
        vl = vl->next;
    }

    /* Need to check whether we found what was needed and */
    /* return error otherwise */
    if(!foundroot) return(VFSN_NOT_A_VS);

    tmp = rd_vdir(vshome,0,dir,RVD_DFILE_ONLY | RVD_NOCACHE);

    if(tmp) return(tmp);
    if(!dir->links) return(VFSN_CANT_FIND_DIR);

    pset_hd(dir->links->host,dir->links->hsoname,NULL);
    pset_wd(dir->links->host,dir->links->hsoname,NULL);

    vllfree(dir->links);
    vllfree(dir->ulinks);

    return(PSUCCESS);
}

