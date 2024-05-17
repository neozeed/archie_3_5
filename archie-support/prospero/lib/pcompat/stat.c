/*
 * Copyright (c) 1993, 1994      by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <usc-license.h>
 */

#include <usc-license.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef SOLARIS
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif
#include <errno.h>

#include <pfs.h>
#include <pcompat.h>
#include <pmachine.h>		/* for bzero */
#include <perrno.h>             /* for testing against errors returned by
                                   prospero subfunctions. */

static int stat_l(const char *path, struct stat *buf, int lflag);

/* syscall is not MT-safe (at least under Solaris) */
#ifndef PFS_THREADS
/* Call stat_l indicating normal stat */
int
stat(const char *path, struct stat *buf)
{
    return(stat_l(path,buf,0));
}


/* Call stat_l indicating lstat */
int
lstat(const char *path, struct stat *buf)
{
    return(stat_l(path,buf,1));
}


static int
stat_l(const char *path, struct stat *buf, int lflag)
{
    VDIR_ST		dir_st;
    VDIR		dir= &dir_st;
    PATTRIB		ap,nextap;
    int		tmp;

    vdir_init(dir);
    check_pfs_default();

    /* If disabled, make system call */
    if(pfs_enable == PMAP_DISABLE) {
        return(syscall((lflag ? SYS_lstat : SYS_stat), path, buf));
    }

    if(pfs_enable == PMAP_COLON) {
        if(*path == ':') path++;
        else if(index(path,':'));
        else return(syscall((lflag ? SYS_lstat : SYS_stat), path, buf));
    }

    if((pfs_enable == PMAP_ATSIGN_NF) || (pfs_enable == PMAP_ATSIGN)) {
        if(*path == '@') {
            path++;
            return(syscall((lflag ? SYS_lstat : SYS_stat), path, buf));
        }
    }

    bzero(buf,sizeof(struct stat));
    buf->st_uid = (uid_t) -1;
    buf->st_gid = (gid_t) -1;

    p__compat_initialize();
    /* I should probably choose better values for errno */
    tmp = rd_vdir(path,0,dir,RVD_DFILE_ONLY|RVD_ATTRIB);
    if((dir->links == NULL) || (tmp && (tmp != DIRSRV_NOT_DIRECTORY))) {

       if(((tmp == PFS_DIR_NOT_FOUND)||(tmp == RVD_DIR_NOT_THERE)|| !tmp)&&
          (pfs_enable == PMAP_ATSIGN_NF) && (*path == '/')) {
           return(syscall((lflag ? SYS_lstat : SYS_stat), path, buf));
       }
       errno = ENOENT;
       return(-1);
    }

    if(tmp == 0) buf->st_mode |= (S_IFDIR | 0555);
    else buf->st_mode |= 0444;

    if(strncmp(dir->links->target,"EXTERNAL",8) == 0) ap = NULL;
    else ap = pget_at(dir->links,"#ALL");

    /* If can't get real attributes, try those stored with link */
    if((ap == NULL) && dir->links->lattrib) {
        ap = dir->links->lattrib;
        dir->links->lattrib = NULL;
    }

    while(ap) {
        switch (*(ap->aname)) {
        case 'L':
            if((strcmp(ap->aname,"LAST-MODIFIED") == 0) &&
               ap->avtype == ATR_SEQUENCE ) {
                buf->st_mtime = asntotime(ap->value.sequence->token);
            }
            break;

        case 'S':
            if((strcmp(ap->aname,"SIZE") == 0) &&
               ap->avtype == ATR_SEQUENCE) {
                int		size;
                qsscanf(ap->value.sequence->token,"%d",&size);
                buf->st_size = size;
                /* The following is a good guess at number of blocks */
                buf->st_blocks = (size+511) / 512;
            }
            break;

        default:
            break;
        }

        nextap = ap->next;
        atfree(ap);
        ap = nextap;
    }
    vllfree(dir->links);
    vllfree(dir->ulinks);
    return(0);
}
#endif /*PFS_THREADS*/
