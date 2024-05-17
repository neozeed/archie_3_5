/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 *
 * Written  by bcn 1989     modified 1989-1991
 * modified by bcn 1/19/93  to support new argument conventions for dirsrv
 * Modified by swa 7/09/93  to take -p# option to specify a port.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <stdio.h>
#include <netdb.h>
#include <sys/param.h> 
#include <pmachine.h>		/* SCOUNIX for socket.h need sys/types.h*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <pwd.h> 
#include <unistd.h>         /* getuid etc */
 
#include <pserver.h>
#include <pfs.h>		/* For qsscanf */
#include <pprot.h>

char prog[100];
void
main(argc,argv)
    int		argc;
    char	*argv[];
{
    char			*dsargv[11]; /* Args to dirsrv          */
    int			dsargc = 0;  /* Count of args to dirsrv */
    char			prvparg[10]; /* -p%d (%d is file desc)  */
    char			shadowarg[MAXPATHLEN];
    char			securityarg[MAXPATHLEN];
    char			datarg[MAXPATHLEN];
    char			aftparg[MAXPATHLEN];

    struct sockaddr_in 	s_in = {AF_INET};
    int			prvport = -1;
    int                     alternate_portnum = 0;
    int			ruid,euid;
    struct servent 		*sp;
    struct passwd   	*pw;

    int			one = 1;

    if (argv[1] && qsscanf(argv[1], "-p#%d", &alternate_portnum) == 1)
        ++argv, --argc;
    if (argc > 2) {
    usage:
        fprintf(stderr,
                "Usage: %s [-p#<alternate-port>] [full-host-name]\n",
                argv[0]);
        exit(1);
    }
    strcpy(prog,argv[0]);
    ruid = getuid();
    euid = geteuid();

    /* If root and alternate port wasn't specified, try to bind privileged
       port before changing uid */ 
    /* The  effective uid needs to be root for bind() to succeed on
       the privileged port ; real uid of root isn't enough (bug fixed,
       sw@isi.edu, 1 Sept. 1993). */
    if(!alternate_portnum && euid == 0) {
      assert(P_IS_THIS_THREAD_MASTER()); /*getpwuid MT-Unsafe*/
        if ((sp = getservbyname("prospero", "udp")) == 0) 
            s_in.sin_port = htons((ushort) PROSPERO_PORT);
        else s_in.sin_port = sp->s_port;

        if ((prvport = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
            fprintf(stderr, "pstart: Can't open socket\n");

        setsockopt(prvport, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        if (bind(prvport, &s_in, S_AD_SZ) < 0) {
            fprintf(stderr, "pstart: Can not bind privileged port\n");
            close(prvport);
            prvport = -1;
        }
    }

    /* Find the Prospero UID, so we can setuid if we are root */
    if((pw = getpwnam(PSRV_USER_ID)) == NULL) {
        fprintf(stderr,"%s: Can't find passwd entry for %s.\n",
                argv[0],PSRV_USER_ID);
        exit(1);
    }

    /* Don't allow changes to paths unless running as root or */
    /* already running as PSRV_USER_ID                        */
    if((ruid != 0) && (ruid != pw->pw_uid) && 
       (getenv("PSRV_ROOT") || getenv("PSRV_FSHADOW") || 
        getenv("PSRV_FSTORAGE") || getenv("PSRV_FSECURITY") || 
        getenv("PSRV_AFTPDIR") || 
        getenv("PSRV_AFSDIR") || getenv("PSRV_LOGFILE"))) {
        fprintf(stderr,"%s: PSRV_* environment variables set but not authorized.\n",argv[0]);
        exit(1);
    }

    /* Set the uid and gid if necessary */
    if((ruid != pw->pw_uid) || (euid != pw->pw_uid)) {
        if(setgid(pw->pw_gid)) {
            fprintf(stderr,"%s: Can't set gid.\n",argv[0]);
            exit(1);
        }

        if(setuid(pw->pw_uid)) {
            fprintf(stderr,"%s: Can't set uid.\n",argv[0]);
            exit(1);
        }
    }

    dsargv[0] = "dirsrv"; dsargc++;

    /* prvport and alternate_port can't both be specified at once. */
    if(prvport >= 0) {
        sprintf(prvparg,"-p%d",prvport);
        dsargv[dsargc++] = prvparg;
    } else if (alternate_portnum) {
        sprintf(prvparg,"-p#%d",alternate_portnum);
        dsargv[dsargc++] = prvparg;
    }            

#ifdef P_UNDER_UDIR
    sprintf(shadowarg,"-S%s/%s",pw->pw_dir,PSRV_SHADOW);
    sprintf(securityarg,"-s%s/%s",pw->pw_dir,PSRV_SECURITY);
    sprintf(datarg,"-T%s/%s",pw->pw_dir,PSRV_STORAGE);
    dsargv[dsargc++] = shadowarg;
    dsargv[dsargc++] = securityarg;
    dsargv[dsargc++] = datarg;
#endif P_UNDER_UDIR

#ifdef AFTPUSER
    /* Find FTP directory - if error, use AFTPDIRECTORY */
    if((pw = getpwnam(AFTPUSER)) != NULL)  {
        sprintf(aftparg,"-f%s",pw->pw_dir);
        dsargv[dsargc++] = aftparg;
    }
#endif AFTPUSER

    if(argc > 1) {
        dsargv[argc++] = "-h";	    
        dsargv[argc++] = argv[1];
    }

    dsargv[dsargc++] = NULL;

    umask(7);

    execv(P_DIRSRV_BINARY,dsargv);

    /* Execl failed */
    exit(1);
}
