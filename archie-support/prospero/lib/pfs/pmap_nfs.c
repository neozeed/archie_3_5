/*
 * Copyright (c) 1989, 1990 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>
#include <psite.h>

#ifdef P_NFS

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <stdio.h>

#include <pfs.h>
#include <pcompat.h>
#include <pmachine.h>

#ifdef ULTRIX
#include <sys/fs_types.h>
#else
#include <mntent.h>
#endif 

pmap_nfs(host,rpath,npath, npathlen, am_args)
    char	*host;
    char	*rpath;
    char	*npath;
    int         npathlen;
    TOKEN       am_args;
{
    char			rfile[MAXPATHLEN];
    char			lmpt[MAXPATHLEN];
    char			*rmpt; /* remote mount point */
    char			rmparg[MAXPATHLEN];
    char			*suffix; /* path from remote mount point.  */
    char			*rf = rfile;      
    char			*a;
    int			devnl;
    int			tmp;
    int pid;
#ifdef BSD_UNION_WAIT
    union wait 	status;
#else
    int     status;
#endif
    int       		start = 0;
#ifdef ULTRIX
    struct fs_data		buffer;
    struct fs_data     	*buf = &buffer;
#else
    struct mntent		*mtentry;
    FILE			*mtab;
#endif

    rmpt = elt(am_args, 5);     /* mount point on remote host. */
    suffix = elt(am_args, 4);   /* object handle used by NFS on remote host */

    sprintf(rf,"%s:%s/%s",host,rmpt,suffix);

#ifdef ULTRIX
    while(getmountent(&start, buf, 1) > 0) {
        devnl = strlen(buf->fd_devname);
        if(!strncmp(rfile,buf->fd_devname,devnl)) {
            qsprintf(npath, npathlen, "%s%s",buf->fd_path,rfile+devnl);
            return(PSUCCESS);
        }
    }
#else
    DISABLE_PFS(mtab = setmntent("/etc/mtab","r"));

    while(mtentry = getmntent(mtab)) {
        devnl = strlen(mtentry->mnt_fsname);
        if(!strncmp(rfile,mtentry->mnt_fsname,devnl)) {
            qsprintf(npath, npathlen, "%s%s",mtentry->mnt_dir,rfile+devnl);
            endmntent(mtab);
            return(PSUCCESS);
        }
    }
    endmntent(mtab);
#endif 

    sprintf(lmpt,"/tmp/pfs_mount/%s%s",host,rmpt);

    a = lmpt + strlen(lmpt) - strlen(rmpt);
    while(*a) {
        if(*a == '/') *a = '-';
        a++;
    }

    mkdir("/tmp/pfs_mount",0777);
    chmod("/tmp/pfs_mount",0777);
    mkdir(lmpt,0777);
    chmod(lmpt,0777);

    sprintf(rmparg,"%s:%s",host,rmpt);

    /* should really do this on our own without */
    /* calling system, but...                   */
    pid = fork();
    if (pid == 0) {
#ifdef ULTRIX
        DISABLE_PFS(execl("/etc/mount","mount",	"-t", "nfs",
                          "-o", "rw,hard,intr,retry=20",
                          rmparg, lmpt, 0));
#else
        DISABLE_PFS(execl("/etc/mount","mount", rmparg, lmpt, 0));
#endif
        exit(1);
    }
    else {
        wait(&status); 
    }
#ifdef BSD_UNION_WAIT
    tmp = status.w_T.w_Retcode;
#else
    tmp = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif

    if(tmp) RETURNPFAILURE;

    qsprintf(npath, npathlen, "%s/%s",lmpt,suffix);

    return(PSUCCESS);

}


#endif P_NFS
