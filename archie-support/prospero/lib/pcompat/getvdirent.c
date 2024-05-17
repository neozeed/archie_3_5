/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>


#include <sys/param.h>

#define DIRSIZ_MACRO            /* needed for HPUX; does no harm otherwise. */

#include <stdio.h>
#include <stdlib.h>             /* For malloc or free */

#include <pmachine.h>
/* Needed if not already included. */
#ifdef USE_SYS_DIR_H            /* Support for SYS_DIR_H may be finally dead.
                                   I hope.  */
#include <sys/dir.h>
#else
#include <dirent.h>
#endif

#include <pfs.h>
#include <pcompat.h>
#include <perrno.h>

/* Maximum open virtual directories */
#define MAX_VDDESC 16

static struct dirent	*dirbuf[MAX_VDDESC + 1] = {NULL};
static int		dirpos[MAX_VDDESC + 1]  = {0};
static int		dirbsz[MAX_VDDESC + 1]  = {0};

int
p__getvdirentries(int fd,  char *buf,  int nbytes,  int *basep)
{
    int		bytes = 0;
    struct dirent	*dp;
    char		*bp;

    if(fd > -1) return(0);
    if(fd < - MAX_VDDESC) return(0);

    dp = (struct dirent *) ((char *)dirbuf[-fd] + dirpos[-fd]);

    while(dp->d_reclen && (dp->d_reclen <= nbytes)) {
        bcopy(dp,buf,dp->d_reclen);
        nbytes = nbytes - (unsigned short) dp->d_reclen;
        buf = buf + (unsigned short) dp->d_reclen;
        bytes = bytes + (unsigned short) dp->d_reclen;
        bp = (char *) dp;bp += dp->d_reclen;dp = (struct dirent *) bp;
    }

    *basep = dirpos[-fd];
    dirpos[-fd] = dirpos[-fd] + bytes;
    return(bytes);
}
    

int
p__readvdirentries(char *dirname)
{
    VDIR_ST		dir_st;
    VDIR		dir= &dir_st;
    VLINK		l;

    long		dsize = 0;
    int		dirnum = 0;
    struct dirent	*dp;
    int		tmp;

    vdir_init(dir);

    check_pfs_default();

    /* If disabled, do no mapping */
    if(pfs_enable == PMAP_DISABLE) return(PSUCCESS);

    /* This is a kludge.  We should not be modifying the */
    /* path arg, but...                                  */
    if(pfs_enable == PMAP_ATSIGN) {
        if(*dirname == '@') {
            strcpy(dirname,dirname+1);
            return(PSUCCESS);
        }
    }

    if(pfs_enable == PMAP_COLON) {
        if(*dirname == ':') dirname++;
        else  return(PSUCCESS);
    }

    p__compat_initialize();
    tmp = rd_vdir(dirname,0,dir,RVD_LREMEXP);

    if(tmp) return(tmp);

    l = dir->links;

    while(l) {
        /* The next statement should track the DIRSIZ macro */
        dsize = dsize + (sizeof(struct dirent) - (MAXNAMLEN+1)) +
            ((strlen(l->name)+1 +3) & ~3);

        l = l->next;
    }

    /* This is just in case */
    dsize = dsize + 256;

    dirnum = 0;
    while(dirnum++ <= MAX_VDDESC) {
        if(!dirbuf[dirnum]) break;
        }
	if(dirnum > MAX_VDDESC) RETURNPFAILURE;

    dp = (struct dirent *) malloc(dsize);
    dirbuf[dirnum] = dp;
    dirpos[dirnum] = 0;
    dirbsz[dirnum] = dsize;

    l = dir->links;

    while(l) {
        dp->d_ino = (unsigned long) 999;
#if !defined (SOLARIS)        
        dp->d_namlen = (unsigned short) strlen(l->name);
#endif
        dp->d_reclen = (unsigned short) DIRSIZ(dp);
        strcpy(dp->d_name,l->name);
        dp = (struct dirent *) ((char *) dp + dp->d_reclen);
        l = l->next;
    }	

    dp->d_ino = (unsigned long) 0;
    dp->d_reclen = (unsigned short) 0;
#if !defined (SOLARIS)
    dp->d_namlen = (unsigned short) 0;
#endif
    *(dp->d_name) = '\0';

    vllfree(dir->links);
    vllfree(dir->ulinks);

    return(-dirnum);

}


int
p__delvdirentries(int desc)
{
    if(desc > -1) RETURNPFAILURE;
    if(desc < - MAX_VDDESC) RETURNPFAILURE;

    if(dirbuf[- desc]) {
        free(dirbuf[- desc]);
        dirbuf[- desc] = NULL;
        return(PSUCCESS);
    }
    RETURNPFAILURE;
}

int
p__seekvdir(int desc, int pos)
{
    if(desc > -1) RETURNPFAILURE;
    if(desc < - MAX_VDDESC) RETURNPFAILURE;
    if(!dirbuf[- desc]) RETURNPFAILURE;
    dirpos[-desc] = pos;
    return(PSUCCESS);
}

int
p__getvdbsize(int desc, int pos)
{
    if(desc > -1) RETURNPFAILURE;
    if(desc < - MAX_VDDESC) RETURNPFAILURE;
    if(!dirbuf[- desc]) RETURNPFAILURE;
    return(dirbsz[-desc]);
}

