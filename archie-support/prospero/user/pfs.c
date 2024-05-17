/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <stdio.h>

#include <pfs.h>
#include <pcompat.h>
#include <pmachine.h>

char	*getenv();
char	*rindex();

/*char *prog; */

void
main()
{
    char	*pfs_defst;
    char	*sitename;
    char	*vsname;
    char	*homedir;
    char	*workdir;
    int	pfs_defint = -1;

/*    prog = argv[0]; */

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    pfs_defst = getenv("PFS_DEFAULT");
    if(pfs_defst) {
        sscanf(pfs_defst,"%d",&pfs_defint);
        if(pfs_defint == PMAP_DISABLE) pfs_defst = "Disabled";
        else if(pfs_defint == PMAP_ENABLE) pfs_defst = "Always";
        else if(pfs_defint == PMAP_COLON) pfs_defst = "On colon";
        else if(pfs_defint == PMAP_ATSIGN_NF) pfs_defst = "Enabled";
        else if(pfs_defint == PMAP_ATSIGN) pfs_defst = "Enabled (won't fall through)";
        else pfs_defst = "<status unknown>";
    }
    else pfs_defst = "<at program's option>";

    sitename = pget_vsname();
    vsname = NULL;
    homedir = pget_hd();
    workdir = pget_wd();

    if(sitename) sitename = stcopy(sitename);
    if(sitename) {
        vsname = rindex(sitename,'/');
        *(vsname++) = '\0';
    }

    printf("Prospero Name Resolution: %s\n",pfs_defst);
    if(sitename) printf("                    Site: %s\n",sitename);
    if(vsname)   printf("                    Name: %s\n",vsname);
    if(homedir)  printf("          Home Directory: %s\n",homedir);
    if(workdir)  printf("       Working Directory: %s\n",workdir);
}
