/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <errno.h>

#include <pfs.h>
#include <psite.h>
#include <pcompat.h>
#include <pmachine.h>
#include <perrno.h>

/*
 * see pcompat.h for the meanings of the flage
 */
int
pfs_access(const char *path, char *npath, int npathlen, int flags)
{

    char		cpath[MAXPATHLEN];
    char		*prefix;
    char		*suffix;
    VLINK		vl;
    int		tmp;

    suffix = "";

    check_pfs_default();

    errno = 0;

    /* If disabled, do no mapping */
    if(pfs_enable == PMAP_DISABLE) {
        strcpy(npath,path);
        return(PSUCCESS);
    }

    if(pfs_enable == PMAP_COLON) {
        if(*path == ':') path++;
        else if(index(path,':'));
        else {strcpy(npath,path); return(PSUCCESS);}
    }

    if((pfs_enable == PMAP_ATSIGN_NF) || (pfs_enable == PMAP_ATSIGN)) {
        if(*path == '@') {
            path++;
            strcpy(npath,path); 
            return(PSUCCESS);
        }
    }

    p__compat_initialize();
    /* I should probably choose better values for errno */
    vl = rd_vlink(path);
    if((perrno || !vl) && ((flags == PFA_CRMAP)||(flags == PFA_CREATE))) {
        strcpy(cpath,path);
        prefix = cpath;
        suffix = strrchr(cpath,'/');
        if(suffix) {
            if(suffix == prefix) prefix = "/";
            *(suffix++) = '\0';
            vl = rd_vlink(prefix);
            if(vl) {
                sprintf(cpath,"%s/%s",vl->hsoname,suffix);
                vl->hsoname = stcopyr(cpath,vl->hsoname);
            }
        }
    }


    /* If not found, but PMAP_ATSIGN_NF, then check if a real file */
    if(((perrno == PFS_DIR_NOT_FOUND) || (perrno == RVD_DIR_NOT_THERE) ||
        (!perrno && !vl)) && 
       (pfs_enable == PMAP_ATSIGN_NF) && 
       (*path == '/') && ((strncmp(path,"/tmp",4) == 0) ||
          ((flags != PFA_CRMAP) && (flags != PFA_CREATE)))) {
           strcpy(npath,path); 
           return(PSUCCESS);
    }

    if(perrno) {errno = ENOENT;return(perrno);}
    if(!vl) {errno = ENOENT;return(PFS_FILE_NOT_FOUND);}

#ifdef PCOMPAT_SUPPORT_FTP
    /* P_AM_FTP requires prompting for a password, which is not something
       transparent to the user.  That's why without the special 
       PCOMPAT_SUPPORT_FTP definition, mapname isn't called with the
       MAP_PROMPT_OK option. */
    tmp = mapname(vl,npath, npathlen, 
                  MAP_PROMPT_OK | ((flags & PFA_RO) ? 
                                   MAP_READONLY : MAP_READWRITE));
#else
    tmp = mapname(vl,npath, npathlen, ((flags & PFA_RO) ? 
                            MAP_READONLY : MAP_READWRITE));
#endif
    vllfree(vl);
    if(tmp && (tmp != PMC_DELETE_ON_CLOSE)) errno = ENOENT;
    return(tmp);
}

