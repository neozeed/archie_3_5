/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>
 *
 * Written  by bcn 1989     modified 1989-1992
 * Modified by swa 7/28/92  to use qsscanf()
 * Modified by swa 1992     to break individual commands into modules
 * Modified by swa 1992     to support V5 protocol
 * Modified by bcn 1/19/93  to take args from environment and command loop
 * Modified by swa 11/6/93  to call p_initialize(). 
 * Modified by swa Dec 93   to be multithreaded.
 */
#include <usc-license.h>

#include <stdio.h>
#include <sgtty.h>
#include <string.h>
#include <posix_signal.h>	/* get our version */
#include <errno.h>
#include <unistd.h>             /*SOLARIS:  for "write" */
#include <stdlib.h>             /*SOLARIS: qfor malloc, free etc */

#include <ardp.h>
#include <pserver.h>            /* must precede psrv.h since psrv.h prototypes
                                   check_krb_auth() only if
				   PSRV_KERBEROS defined. 
                                   */
#include <psite.h>
#include <pfs.h>
#include <plog.h>
#include <pprot.h>
#include <psrv.h>
#include <perrno.h>
#include <pparse.h>

#include <pmachine.h>
#if defined(AIX)  || defined(SCOUNIX)
#include <time.h>
#include <signal.h>
#endif

#ifdef PSRV_P_PASSWORD
#include <ppasswd.h>
#endif
#include "dirsrv.h"

#ifdef PSRV_ACCOUNT
#include <math.h>
#endif

/*#define MASTER_IS_ONLY_SUBTHREAD */ /* DEBUGGING ONLY */

#ifdef PFS_THREADS
/* #define DIRSRV_SUB_THREAD_COUNT	60 /* # of sub-threads we're using in
   dirsrv.  This is usually set to the default value below.  However, you can
   redefine this if you want to experiment with using different thread counts
   (performance tuning).
*/
#ifndef DIRSRV_SUB_THREAD_COUNT
/* Unless the user overrides the value for DIRSRV_SUB_THREAD_COUNT, pick the
   largest legal value. */
#define DIRSRV_SUB_THREAD_COUNT (P_MAX_NUM_THREADS - 1)
#endif
/* Must be less than P_MAX_NUM_THREADS, since need one thread for dirsrv. . */
#if DIRSRV_SUB_THREAD_COUNT >= P_MAX_NUM_THREADS
#error DIRSRV_SUB_THREAD_COUNT too big.
#endif
#ifdef MASTER_IS_ONLY_SUBTHREAD
#undef DIRSRV_SUB_THREAD_COUNT
#define DIRSRV_SUB_THREAD_COUNT 1
#endif
#endif

extern char	*acltypes[];

/* To check for memory leaks */
/* There needs to be a common declarations file shared between the PFS library
   and dirsrv.c.  Some way for these declarations to not needt o be
   duplicated. */
/* There needs to be a common declarations file shared between the ARDP library
   and dirsrv.c.  Some way for these declarations to not needt o be
   duplicated. */
extern int vlink_count, pattrib_count, acl_count,    pfile_count;
extern int rreq_count,  ptext_count,   string_count, token_count; 
extern int pauth_count, opt_count,     filter_count, p_object_count;

/* There needs to be a common declarations file shared between the PSRV library
   and dirsrv.c.  Some way for these declarations to not needt o be
   duplicated. */
extern int vlink_max,   pattrib_max,   acl_max,      pfile_max;
extern int rreq_max,    ptext_max,     string_max,   token_max;
extern int pauth_max,   opt_max,       filter_max,   p_object_max;
extern int filelock_open, filelock_open_max, filelock_sepwaits;
extern int filelock_secwaits;
#ifdef DIRECTORYCACHING
extern int cache_attempt, cache_can, cache_yes, dsrobject_fail;
#endif
/* In ardp.h */
extern int dnscache_count, dnscache_max /* , filelock_count, filelock_max */;
extern int filelock_open, filelock_open_max;
#ifdef PSRV_GOPHER_GW
/* There needs to be a common declarations file shared between the gopher_gw
   library and dirsrv.c.  Some way for these declarations to not needt o be
extern int dnscache_count, dnscache_max, alldnscache_count;
   duplicated. */
extern int glink_count, glink_max;
#endif
#ifdef PSRV_WAIS_GW
/* There needs to be a common declarations file shared between the wais_gw
   library and dirsrv.c.  Some way for these declarations to not needt o be
   duplicated. */
extern int waismsgbuff_count, waismsgbuff_max;
extern int ietftype_count, ietftype_max;
extern int waissource_count, waissource_max;
#endif

extern int pQlen;

static	cmd_lookup();
VLINK	check_fwd();
char	*month_sname();
char	*getenv();

static int auth_fail_reply(RREQ req, char formatstring[], ...);
static void setup_disc();

#ifdef PSRV_ACCOUNT
static int str_to_fp();
static int subtract_fp();
static int add_fp();
static int charge();
static char *acc_method_name();
struct acc_lookup {
    int  acc_method_num;
    char *acc_method_name;
};
static struct acc_lookup acc_methods[] = 
{
    PFSA_CREDIT_CARD, "CREDIT_CARD",
    0, 0
 };
#endif

char    	prog[MAXPATHLEN];
int 		fault_count = 0; /* # of serious faults in the server -- # of
                                  times it has had to be automatically
                                  restarted.  */
/* #define DIRSRV_EXPLAIN_LAST_RESTART to report on the last request made to
   the server before it crashed. */ 
#ifdef DIRSRV_EXPLAIN_LAST_RESTART
char		last_request[ARDP_PTXT_LEN_R] = "";
#endif
char		*last_error = NULL;
char		st_time_str[40];

int		in_port = -1;
char		*portname = NULL;

static int    		mflag = 0;       /* Manual start of server         */

int		req_count = 0;
#ifdef SERVER_SUPPORT_V1
int             v1_req_count = 0;
int             crdir_count = 0;
#endif
int		crlnk_count = 0;
int		crobj_count = 0;
int		dellnk_count = 0;
int		eoi_count = 0;
int		goi_count = 0;
int		list_count = 0;
int		lacl_count = 0;
int		eli_count = 0;
int		eacl_count = 0;
int		status_count = 0;
int		upddir_count = 0;
int             parameter_count = 0;

char    shadow[MAXPATHLEN]    = PSRV_FSHADOW;
char    security[MAXPATHLEN]  = PSRV_FSECURITY;
char	pfsdat[MAXPATHLEN]    = PSRV_FSTORAGE;
char	dirshadow[MAXPATHLEN] = DSHADOW;
char	dircont[MAXPATHLEN]   = DCONTENTS;
char    *object_pool          = NULL; /* set later. */

char    root[MAXPATHLEN]      = "";
char	aftpdir[MAXPATHLEN]   = "";
char	afsdir[MAXPATHLEN]    = "";

char	*logfile_arg = NULL;
extern int p__server;

#ifdef PSRV_ARCHIE
extern int 	arch_prioritize_request();
#endif /* PSRV_ARCHIE */


struct db_entry db_prefixes[] = DATABASE_PREFIXES;
const int	db_num_ents = sizeof(db_prefixes)/sizeof(db_prefixes[0]);

char	hostname[MAXPATHLEN]  = "";    /* Server's host name                */
char	hostwport[MAXPATHLEN+30] = ""; /* Host name w/ port if non-standard */

#ifdef PFS_THREADS
int		free_subthread_count = DIRSRV_SUB_THREAD_COUNT;
int             subthread_count = 0;
int             subthread_max = 0;
#endif

static void dirsrv(RREQ req);

#if defined(AIX) || defined(SOLARIS)
void  *bsdSignal(sig, fn)
  int sig;
  void *fn;
{
  struct sigaction act, oact;

  act.sa_handler = fn;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if (sig == SIGALRM)
  {
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT; /* SunOS */
#endif
  }
  else
  {
#ifdef SA_RESTART
    act.sa_flags |= SA_RESTART; /* SVR4, 4.3+BSD */
#endif
  }

  if (sigaction(sig, &act, &oact) < 0)
  {
    return SIG_ERR;
  }

  return oact.sa_handler;
}
#endif

void
main(int argc, char *argv[])
{
    char		*cur_arg;        /* For argument parsing           */
    int     		on = 1;

    struct sockaddr_in 	from;
    int			port_no;
    struct hostent		*current_host;
    int     		fromlen;
    PTEXT   		pkt;
    RREQ			curr_req;
    register int		n;
    int     		child;
    int			retval;
    char		*envv;  /* Temp pointer to environment variable */

    time_t 		now;
    static int dirsrv_internal_error_handler();
    extern int fseek();         /* RHS of assignment. */
    extern int dQmaxlen;

    /* Sets thread to master if necessary. */
    p_initialize("sP", 0, (struct p_initialize_st *) NULL);
#ifdef PFS_THREADS
    psrv_init_mutexes();
#ifdef PSRV_GOPHER_GW
    gopher_gw_init_mutexes();
#endif
#ifdef PSRV_WAIS_GW
    wais_gw_init_mutexes();
#endif
    p_th_mutex_init(p_th_mutexP_PARAMETER_MOTD);
#endif
    p_srv_check_acl_initialize_defaults();
#ifdef SHARED_PREFIXES
    p_init_shared_prefixes();     /* psrv routine. */
#endif
    umask(0);

    /* These function-valued variables depend upon whether we're the */
    /* server or the client.  The default values are the client      */
    /* values; the server values are these.                          */
    internal_error_handler = dirsrv_internal_error_handler;
    qoprintf = srv_qoprintf;
    stdio_fseek = fseek;
    p__server = 1;              /* We are the Server. */


    strcpy(prog,argv[0]);
    argc--;argv++;

    if(envv = getenv("PSRV_ROOT"))
	strcpy(root,envv);
#ifdef PSRV_ROOT
    else strcpy(root,PSRV_ROOT);
#endif

    if(envv = getenv("PSRV_FSHADOW"))
        strcpy(shadow,envv);

    if(envv = getenv("PSRV_FSECURITY"))
        strcpy(security,envv);

    if(envv = getenv("PSRV_FSTORAGE"))
        strcpy(pfsdat,envv);

    if(envv = getenv("PSRV_AFTPDIR"))
        strcpy(aftpdir,envv);
#ifdef AFTPDIRECTORY
    else strcpy(aftpdir,AFTPDIRECTORY);
#endif

    if(envv = getenv("PSRV_AFSDIR"))
        strcpy(afsdir,envv);
#ifdef AFSDIRECTORY
    else strcpy(afsdir,AFSDIRECTORY);
#endif

    if(envv = getenv("PSRV_HOSTNAME"))
        strncpy(hostname,envv,sizeof(hostname));
#ifdef PSRV_HOSTNAME
    else strncpy(hostname,PSRV_HOSTNAME,sizeof(hostname));
#else
    else strncpy(hostname,myhostname(),sizeof(hostname));
#endif
    ardp_hostname2addr_initcache();
    ardp_hostname2addr(hostname,NULL);
#ifdef P_SITE_HOST
    ardp_hostname2addr(P_SITE_HOST,NULL);
#endif /*P_SITE_HOST*/
#ifdef GOPHER_GW_NEARBY_GOPHER_SERVER
    ardp_hostname2addr(GOPHER_GW_NEARBY_GOPHER_SERVER,NULL);
#endif /*GOPHER_GW_NEARBY_GOPHER_SERVER*/

    object_pool = qsprintf_stcopyr(object_pool, "%s/%s", pfsdat, OBJECT_POOL);
    if(envv = getenv("PSRV_LOGFILE"))
	set_logfile(envv);

    while (argc > 0 && **argv == '-') {
        cur_arg = argv[0]+1;

        while (*cur_arg) {
            switch (*cur_arg++) {

            case 'a':	/* Set AFS directory */
		if(*cur_arg) {
		    strncpy(afsdir,cur_arg,sizeof(afsdir));
		    cur_arg += strlen(cur_arg);
		}
		else if (argc > 1) {
		    strncpy(afsdir,argv[1],sizeof(afsdir));
		    argc--;argv++;
		}
		else strcpy(afsdir,"/afs");
		break;

	    case 'E':	/* Set last error */
		if(*cur_arg) {
		    last_error = cur_arg;
		    cur_arg += strlen(cur_arg);
		}
		else if (argc > 1) {
		    last_error = argv[1];
		    argc--;argv++;
		}
		else goto parse_err;
		break;

            case 'f':	/* Set anonymous ftp area */
		if(*cur_arg) {
		    strncpy(aftpdir,cur_arg,sizeof(aftpdir));
		    cur_arg += strlen(cur_arg);
		}
		else if (argc > 1) {
		    strncpy(aftpdir,argv[1],sizeof(aftpdir));
		    argc--;argv++;
		}
		else goto parse_err;
		break;

	    case 'F':	/* Set fault count from next argument */
		fault_count = -1;
		if(*cur_arg && strchr("0123456789",*cur_arg)) {
		    sscanf(cur_arg,"%d",&fault_count);
		    cur_arg += strspn(cur_arg,"0123456789");
		}
		else if(argc > 1) {
		    retval = sscanf(argv[1],"%d",&fault_count);
		    if (retval == 1) {argc--;argv++;}
		}
		else goto parse_err;
		break;

            case 'h':	/* Set hostname */
		if(*cur_arg) {
		    strncpy(hostname,cur_arg,sizeof(hostname));
		    cur_arg += strlen(cur_arg);
		}
		else if (argc > 1) {
		    strncpy(hostname,argv[1],sizeof(hostname));
		    argc--;argv++;
		}
		else goto parse_err;
		break;

            case 'L':	/* Set logfile */
		if(*cur_arg) {
		    set_logfile(cur_arg);
		    logfile_arg = cur_arg;
		    cur_arg += strlen(cur_arg);
		}
		else if (argc > 1) {
		    set_logfile(argv[1]);
		    logfile_arg = cur_arg;
		    argc--;argv++;
		}
		else goto parse_err;
		break;
		
            case 'm':	/* Run server from terminal */
		mflag++;
                break;

            case 'n':  {        /* clear info from the ardp_doneQ; this affects
                                   how much stuff is saved and the accuracy of
                                   logging.  */
                extern int ardp_clear_doneQ_loginfo; /* in ardp_respond() */
                ardp_clear_doneQ_loginfo = 1;
            }
            break;

	    /* If form of port identifier is a positive integer, then  */
	    /* integer reperesents the file descriptor of a privileged */
	    /* port already open.  If it is of any other form, it is   */
	    /* a port identifier to be opened later by bind_port.  At  */
	    /* most one of each may be specified.  If neither, then    */
	    /* -p dirsrv is assumed.  Because most clients don't use   */
	    /* the privileged prospero port, for the time being dirsrv */
	    /* is assumed even if a file descriptor is specified as    */
	    /* as no other port is specified. For port names, a name   */
            /* is looked up in the /etc/services files.  If the first  */
            /* char is #, then what follows is the port number itself. */
	    case 'p':	/* Port on which to listen */
		in_port = -1;
		if(*cur_arg) {
		    if(strchr("0123456789",*cur_arg)) {
			sscanf(cur_arg,"%d",&in_port);
			ardp_set_prvport(in_port);
			cur_arg += strspn(cur_arg,"0123456789");
		    }
		    else {
			portname = cur_arg;
			cur_arg += strlen(cur_arg);
		    }
		}
		else if(argc > 1) {
		    if(strchr("0123456789",*argv[1])) {
			sscanf(argv[1],"%d",&in_port);
			ardp_set_prvport(in_port);
			cur_arg += strspn(cur_arg,"0123456789");
		    }
		    else {
			portname = argv[1];
		    }
		    argc--;argv++;
		}
		else goto parse_err;
		break;

            case 'r':	/* Set root of tree to make available */
		if(*cur_arg) {
		    strncpy(root,cur_arg,sizeof(root));
		    cur_arg += strlen(cur_arg);
		}
		else if (argc > 1) {
		    strncpy(root,argv[1],sizeof(root));
		    argc--;argv++;
		}
		else goto parse_err;
		break;

            case 'S':	/* Set shadow hierarchy */
		if(*cur_arg) {
		    strncpy(shadow,cur_arg,sizeof(shadow));
		    cur_arg += strlen(cur_arg);
		}
		else if (argc > 1) {
		    strncpy(shadow,argv[1],sizeof(shadow));
		    argc--;argv++;
		}
		else goto parse_err;
		break;

            case 's':	/* Set security hierarchy */
		if(*cur_arg) {
		    strncpy(security,cur_arg,sizeof(security));
		    cur_arg += strlen(cur_arg);
		}
		else if (argc > 1) {
		    strncpy(security,argv[1],sizeof(security));
		    argc--;argv++;
		}
		else goto parse_err;
		break;

            case 'T':	/* Set storage hierarchy */
		if(*cur_arg) {
		    strncpy(pfsdat,cur_arg,sizeof(pfsdat));
		    cur_arg += strlen(cur_arg);
		}
		else if (argc > 1) {
		    strncpy(pfsdat,argv[1],sizeof(pfsdat));
		    argc--;argv++;
		}
		else goto parse_err;
		break;

#ifdef PFS_THREADS
            case 't':           /* set # of threads */
		if(*cur_arg) {
		    if(strchr("0123456789",*cur_arg)) {
			sscanf(cur_arg,"%d",&free_subthread_count);
			cur_arg += strspn(cur_arg,"0123456789");
		    } else goto parse_err;
		} else if(argc > 1) {
		    if(strchr("0123456789",*argv[1])) {
			sscanf(argv[1],"%d",&free_subthread_count);
			cur_arg += strspn(cur_arg,"0123456789");
		    } else goto parse_err;
		    argc--;argv++;
		}
		else goto parse_err;
                if (free_subthread_count >= P_MAX_NUM_THREADS 
                    || free_subthread_count < 1) {
                    fprintf(stderr, "dirsrv: asked to use %d subthreads; \
must use between 1 and the compiled-in maximum (%d).\n", free_subthread_count,
                            P_MAX_NUM_THREADS - 1);
                    exit(1);
                }
		break;

#endif

	    parse_err:
            default:
                fprintf(stderr,
                   "Usage: dirsrv [-m] [-p<port>] [-r root] [-h hostname]\n");
                exit(1);
            }
        }
        argc--, argv++;
    }

    if (argc > 0) {
#ifdef PFS_THREADS
	fprintf(stderr,
		"Usage: dirsrv [-m] [-t<num-subthreads>] [-p<port>] [-r root] [-h hostname]\n");
#else
	fprintf(stderr,
		"Usage: dirsrv [-m] [-p<port>] [-r root] [-h hostname]\n");
#endif
	exit(1);
    }

    /* Here re really should get the host name in cannonical form */
    ucase(hostname);

    if (!mflag) {
	if(envv = getenv("PSRV_RUNDIR")) 
	    retval = chdir(envv);
	else {
#ifdef P_RUNDIR
	    retval = chdir(P_RUNDIR);
#else
	    retval = chdir("/tmp");
#endif
	}
	if(retval) plog(L_STATUS,NOREQ,"Startup - chdir failed: %d",errno);
    }

    /* Note our start time */
    (void) time(&now);
    assert(P_IS_THIS_THREAD_MASTER()); /* gmtime unsafe */
    p_th_mutex_lock(p_th_mutexPFS_TIMETOASN);
    {
    struct tm 	*tm= gmtime(&now);          /* safe since parent thread. */
    sprintf(st_time_str,"%2d-%s-%02d %02d:%02d:%02d UTC",tm->tm_mday,
             month_sname(tm->tm_mon + 1),tm->tm_year,
             tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
    p_th_mutex_unlock(p_th_mutexPFS_TIMETOASN);

    /* Eventually, we will only set up and bind a port if one    */
    /* wasn't already passed to us using the -p option.  Until   */
    /* all clients can support alternative ports, however, we    */
    /* must bind the DIRSRV port in addition to any that were    */
    /* passed to us.  Note that at this point, unless we are     */
    /* running as root (which we should not be) we would not     */
    /* have been able to bind the priveleged port.               */

    port_no = ardp_bind_port(portname ? portname : "dirsrv");

#ifdef TAG_UNASSIGNED_PORT
    if((in_port < 0) && (port_no != PROSPERO_PORT)) 
        sprintf(hostwport,"%s(%d)",hostname,port_no);
    else
#endif 
    if((port_no != PROSPERO_PORT) && (port_no != DIRSRV_PORT)) 
        sprintf(hostwport,"%s(%d)",hostname,port_no);
    else strcpy(hostwport,hostname);

#ifdef PSRV_ARCHIE
    ardp_set_queuing_policy(arch_prioritize_request,0);
#endif /* PSRV_ARCHIE */

    if (!mflag) {
        if ((child = fork()) != 0) {
            printf("%s started, PID=%d, PORT=%d\n", 
                   prog, child, port_no);
            exit(0);
        }
        setup_disc();
    }
    else {
        plog_manual(stderr);
        printf("%s started, PORT=%d\n", prog, port_no);
    }
    
    set_restart_params(fault_count+1, port_no);

    plog(L_STATUS,NOREQ,
#ifdef PFS_THREADS
	 "Startup - %sPID: %d, Root: %s, Shadow: %s, Security: %s, Aftpdir: %s, %s%s%sNum-Subthreads: %d, Host: %s", 
#else
	 "Startup - %sPID: %d, Root: %s, Shadow: %s, Security: %s, Aftpdir: %s, %s%s%sHost: %s", 

#endif
         (mflag ? "Mode: manual " : ""), getpid(), root, shadow, security, 
	 aftpdir, (*afsdir ? "Afsdir: " : ""), afsdir, (*afsdir ? ", " : ""),
#ifdef PFS_THREADS
         free_subthread_count,
#endif
         hostwport);

    /* set dirsend timeout for chasing forwarding pointers */
    ardp_set_retry(4,1);

#ifndef PFS_THREADS
    /* receive loop */
    for (;;) {
        curr_req = ardp_get_nxt();
#ifdef DIRSRV_EXPLAIN_LAST_RESTART		
        pkt = curr_req->rcvd;
        strcpy(last_request,pkt->text); /* For error logging */
#endif DIRSRV_EXPLAIN_LAST_RESTART
            req_count++;
            dirsrv(curr_req);
    }	
#else /* PFS_THREADS */
    /* We've already called p__th_set_self_master() */
    /* receive loop */
    for (;;) {
        curr_req = NULL;        /* req to process */
        if (subthread_count == 0) {
            /* No worker threads currently running; ok to block in
               ardp_get_nxt() */
            curr_req = ardp_get_nxt();
        } else {
	  if (free_subthread_count > 0) {
            /* Worker threads are currently running, but there are also
               threads that are free to get more work done, if it arrives or
               is on the pending queue. */
            /* can't call ardp_get_nxt() because it's blocking. */

            curr_req = ardp_get_nxt_nonblocking();
	  }
            if (!curr_req) {
                /* sleep for a second to see if work gets done.
                   do this because a nonblocking select() is not currently
                   implemented. */
	    ardp_accept_and_wait(0,500000);
            }
        }
        if (curr_req) {
            p_th_t new_thread;
            int tmp;
            /* A request arrived and was ready to process */
            if(++subthread_count > subthread_max)
                subthread_max = subthread_count;
            --free_subthread_count;
            /* If pthread_create() ever fails, we have no way to recover, and
               might as well consider it a fatal error. */
#ifdef MASTER_IS_ONLY_SUBTHREAD
            req_count++;
            dirsrv(curr_req);
#else
#if 1
            req_count++;
	    if (p_th_create_detached(new_thread, dirsrv, curr_req))
		abort();
            /* If p_th_create_detached ever fails, we have no good way to 
               recover. So fail if it ever gives us a non-zero return value. */
#else                           /* the above replaces the following code */
            /* Throw this away once a working FSU pthreads based implementation
               is going again. */
            tmp = pthread_create(&new_thread, (pthread_attr_t *) NULL,
                           (pthread_func_t) dirsrv, (any_t) curr_req);
            assert(tmp == 0);
            /* If pthread_detach ever fails, we have no good way to recover. */
            /* So fail if it ever gives us a non-zero return value. */
            /* Note that under the Draft 7 standard, pthread_detach
               takes a pthread_t, whereas under Draft 6, it takes a pointer to
               a pthread_t.  This implementation uses Draft 6. */
            tmp = pthread_detach(&new_thread);
            assert(tmp == 0);
#endif
#endif
            /* dirsrv handles decrementing subthread_count, incrementing
               free_subthread_count, and de-assigning itself the thread
               number on exit*/ 
            /* dirsrv assigns itself a newthread # on startup, unless
               MASTER_IS_ONLY_SUBTHREAD. */ 
        }
#if 0
#ifdef DIRSRV_EXPLAIN_LAST_RESTART		
#error /* This won't work correctly right now.  */
            pkt = curr_req->rcvd;
            strcpy(last_request,pkt->text); /* For error logging */
#endif DIRSRV_EXPLAIN_LAST_RESTART
#endif /* 0 */
    }
#endif /* PFS_THREADS */
}

#ifndef NDEBUG
void
diagnose(void)
{
#ifdef PFS_THREADS
  printf("Subthreads: %d(%d)\n",subthread_count,subthread_max);
#endif
  p_diagnose();
  psrv_diagnose_mutexes();
#ifdef PSRV_GOPHER_GW
  gopher_gw_diagnose_mutexes();
#endif
#ifdef PSRV_WAIS_GW
  wais_gw_diagnose_mutexes();
#endif
#ifdef PFS_THREADS
  DIAGMUTEX(P_PARAMETER_MOTD,"P_PARAMETER_MOTD"); 
#endif
}
#endif /*NDEBUG*/
static int dirsrv4real(RREQ req);

static void
dirsrv(RREQ req)
{
    CHECK_MEM();
    p_thread_initialize();
    VLDEBUGBEGIN;
    dirsrv4real(req);
    VLDEBUGEND;
#ifdef PFS_THREADS
#ifndef MASTER_IS_ONLY_SUBTHREAD
    p_th_deallocate_self_num();
#endif
    CHECK_MEM();
    --subthread_count;
    ++free_subthread_count;
#endif /*PFS_THREADS*/
}

/* dirsrv4real() returns a SUCCESS or FAILURE code that is currently unused.
   It's there, in case we ever have a use for it. */
static int
dirsrv4real(RREQ req)
{

/*************************/

    /* The following are set in one line and used by subsequent */
    /* lines of the same message                                */

    int	client_version = MAX_VERSION;   /* Protocol version no.; 
                                           default to max version. */
    /* These store directory information, set on one line and used in
       subsequent lines of the message. */
    char        dir_type[40];     /* Type of dir name (Currently, the
                                             only supported value is ASCII) */
    char	client_dir[MAXPATHLEN] = ""; 	/* Current directory      */
    long	dir_version = -1;       	/* Directory version nbr.
                                                   Currently ignored.  */
    long	dir_magic_no = -1;      	/* Directory magic number */

#ifdef PSRV_ARCHIE
    int	max_list_commands = 5;		/* Max lists in request   */
#endif /* PSRV_ARCHIE */


    /* used to parse commands. */
    char                *tmpline;  /* line returned by in_line()  */
    AUTOSTAT_CHARPP(commandp);       /* The current line.  This is
                                                memory allocated through 
                                                stcopy(). */
    char *next_word;         /* next_word:    a temporary pointer to a position
                                within the command line.  Points to the
                                next place from which we'll want to read
                                data. */ 
    
    int		retval;         /* Return value from subfunctions */
    INPUT_ST in_st;
    INPUT in = &in_st;

    *client_dir = '\0';

    CHECK_MEM();
    /* in_nextline(): the start of the next command line in the packet, or
       NULL.  
       command: the head of the current command line, without newline, NULL
       terminated. */
    rreqtoin(req, in);

    if (in_eof(in))
        return error_reply(req, "ERROR Received empty Prospero request packet");
    /* This loop always begins with in_nextline() either the head of the
       next command line in the packet, or NULL. */
    while (!in_eof(in)) {
        VLDEBUGBEGIN;
        /* get the next line. */
        if(in_line(in, &tmpline, &next_word)) {
            creplyf(req, "ERROR Malformed input line: %'s\n", p_err_string);
            plog(L_DIR_ERR, req, "Malformed input line: %s", p_err_string);
            RETURNPFAILURE;
        }
        *commandp = stcopyr(tmpline, *commandp);
        next_word = *commandp + (next_word - tmpline);
        /* now, in_nextline(in) either points to the start of the following
           line or is NULL.  *commandp points to the start of a NUL-terminated
           string which contains what's supposed to be a Prospero protocol
           command line.   next_word points to the first word in that string
           after command. */ 

        switch(cmd_lookup(*commandp)) {
#ifdef PSRV_ACCOUNT
    case ACCOUNT:
	{
            static char *t_options  = NULL;
            static char	*acc_method = NULL;    /* Accounting method */
	    static char *acc_server = NULL;    /* Accounting server */
	    static char *acc_name   = NULL;    /* Account name */
            static char *verifier   = NULL;    /* Verifier string */
	    static char *int_bill_ref = NULL;  /* Internal billing */
					       /* reference */
	    static char *currency = NULL;
	    static char *amount = NULL;
            int tmp, i;
            char *cp;           /* Temporary variable for use by qsscanf() */
            PAUTH patmp;

            tmp = qsscanf(next_word,"%'&s %'&s %'&s %'&s %'&s %'&s %r", 
			  &t_options, &acc_method, &acc_server,
			  &acc_name, &verifier, &int_bill_ref, &cp);
	    if(tmp < 7)
                return error_reply(req, "Invalid arguments: %s", *commandp);

            if ((patmp = paalloc()) == NULL)
                return error_reply(req, "Out of Memory (paalloc())!");
            /* This memory will be freed by the rdgram transmit() function. */
            patmp->next = req->auth_info;
            req->auth_info = patmp;

	    /* Search for acc_method in array */
	    for (i=0; acc_methods[i].acc_method_num && 
		      strcmp(acc_methods[i].acc_method_name,
			     acc_method); 
		 i++);
	    if (!acc_methods[i].acc_method_num)
		return error_reply(req, "Invalid accounting method!");
	    patmp->ainfo_type = acc_methods[i].acc_method_num;
	    patmp->acc_server = stcopy(acc_server);
	    patmp->acc_name = stcopy(acc_name);
	    patmp->acc_verifier = stcopy(verifier);
	    patmp->int_bill_ref = stcopy(int_bill_ref);

	    while (1) {
		struct amt_info *amt = NULL;
		char *remainder = NULL;
		
		if (!cp)
		    break;

		tmp = qsscanf(cp, "%'&s %'&s %r",
			      &currency, &amount, &remainder);

		if (tmp < 2)
		    break;

		stfree(cp);
		cp = remainder;

		amt = (struct amt_info *) stalloc(sizeof(struct
							 amt_info));
		amt->currency = stcopy(currency);
		str_to_fp(amount, &amt->limit);
		amt->amt_spent.sig = 0;
		amt->amt_spent.exp = 0;
		amt->next = patmp->allocated_amts;
		patmp->allocated_amts = amt;
	    }
	    break;
	}
#endif

        case AUTHENTICATE:
	{
            AUTOSTAT_CHARPP(auth_typep);    /* Type of Authentication */ 
            /* Authentication data */ 
            AUTOSTAT_CHARPP(t_optionsp);
            AUTOSTAT_CHARPP(authentp);
            AUTOSTAT_CHARPP(t_client_idp);
            int tmp;
            char *cp;           /* Temporary variable for use by qsscanf() */
            PAUTH patmp;

            tmp = qsscanf(next_word,"%'&s %'&s %'&s %r", 
                          t_optionsp, auth_typep, authentp, &cp);
            /* Additional tokens are just principal names; purely
               informational. */ 
            if(tmp < 3)
                return error_reply(req, "Invalid arguments: %s", *commandp);

            if ((patmp = paalloc()) == NULL)
                return error_reply(req, "Out of Memory (paalloc())!");
            /* This memory will be freed by the rdgram transmit() function. */
            patmp->next = req->auth_info;
            req->auth_info = patmp;

#ifdef PSRV_KERBEROS
            if (strequal(*auth_typep,"KERBEROS")) {
                if (retval = check_krb_auth(*authentp, req->peer, 
					    t_client_idp)) {
		    if (!*t_options || strequal(*t_optionsp, "MANDATORY"))
			return auth_fail_reply(req, 
					       "Invalid Kerberos Authenticator");
		}
		else {
		    patmp->principals = tkalloc(*t_client_idp);
		    patmp->ainfo_type = PFSA_KERBEROS;
		    req->client_name = stcopyr(t_client_id,
					       req->client_name);
		}
            } else 
#endif
#ifdef PSRV_P_PASSWORD
            if (strequal(*auth_typep, "P_PASSWORD")) {
		qsscanf(cp, "%'&s", t_client_idp);
		if (!passwd_correct(*t_client_idp, *authentp)) {
		    if ((!*t_optionsp && get_ppw_entry(*t_client_idp))
			|| (*t_optionsp && strequal(*t_optionsp, "MANDATORY")))
			return auth_fail_reply(req, 
					       "Invalid password");
		}
		else {
		    req->client_name = stcopyr(*t_client_idp, req->client_name);
		    patmp->principals = tkalloc(*t_client_idp);
		    patmp->ainfo_type = PFSA_P_PASSWORD;
		}
	    } else
#endif		
	    if (strequal(*auth_typep, "UNAUTHENTICATED")) {
		req->client_name = stcopyr(*authentp,req->client_name);
                patmp->principals = tkalloc(*authentp);
                patmp->ainfo_type = PFSA_UNAUTHENTICATED;
            }
            else {
		if (strequal(*t_optionsp, "MANDATORY"))
		    return auth_fail_reply(req, 
					   "Authentication type %s not supported",
				      *auth_typep,0);
            }
        }
            break;

#ifndef PSRV_READ_ONLY
    case CREATE_LINK: 
            crlnk_count++;
            if(create_link(req, &*commandp, &next_word, in, client_dir,
                           dir_magic_no)) 
                RETURNPFAILURE;
            break;

    case CREATE_OBJECT:
            crobj_count++;
            if(create_object(req, &*commandp, &next_word, in, client_dir,
                             dir_magic_no))
                RETURNPFAILURE;
            break;
            
        case DELETE_LINK: 
            dellnk_count++;
            if(delete_link(req, *commandp, next_word, in,
                           client_dir, dir_magic_no))
                RETURNPFAILURE;
            break;
#endif

    case DIRECTORY:
        {
            int tmp;
            char *cp;

            dir_version = 0;
            dir_magic_no = 0;
            tmp = qsscanf(next_word,"%!!'s %!!'s %d %r",
                          dir_type, sizeof dir_type, 
                           client_dir, sizeof client_dir,
                          &dir_version, &cp);
            if (tmp < 0)
                interr_buffer_full();
            if(tmp < 2 || tmp > 3)
                return error_reply(req, "Invalid arguments: %'s", *commandp);
            if(!strequal(dir_type,"ASCII"))
                return error_reply(req, "Directory handle type %'s not supported",
                                   dir_type); 
            if(in_select(in, &dir_magic_no)) {
                return error_reply(req, "DIRECTORY: %'s", p_err_string);
            }
                
            if(check_handle(client_dir) == FALSE) {
               creply(req,"FAILURE NOT-AUTHORIZED\n");
               plog(L_AUTH_ERR,req,"Invalid directory name: %'s",client_dir);
               RETURNPFAILURE;
            }
        }
            break;

#ifndef PSRV_READ_ONLY
    case EDIT_ACL: 
            eacl_count++;

            if(edit_acl(req, &*commandp, &next_word, in, client_dir,
                           dir_magic_no)) 
                RETURNPFAILURE;
            break;
        
    case EDIT_LINK_INFO: 
            eli_count++;
            if(edit_link_info(req, &*commandp, &next_word, in, 
                              client_dir, dir_magic_no))
                RETURNPFAILURE;
            break;

    case EDIT_OBJECT_INFO:
            eoi_count++;
            if(srv_edit_object_info(req, *commandp, next_word, in))
                RETURNPFAILURE;
            break;
        
#endif      /* PSRV_READ_ONLY */

    case GET_OBJECT_INFO:	
	goi_count++;
	CHECK_MEM();
	VLDEBUGBEGIN;
	if(get_object_info(req, *commandp, next_word, in))
	    RETURNPFAILURE;
	VLDEBUGEND;
	CHECK_MEM();
	break;

    case LIST: {
	list_count++;
#ifdef PSRV_ARCHIE
	if(max_list_commands-- <= 0) {
                creply(req,"FAILURE NOT-AUTHORIZED Too many list commands in a single request\n");
                plog(L_AUTH_ERR,req,"Too many list commands");
                RETURNPFAILURE;
            }
#endif /* PSRV_ARCHIE */

      VLDEBUGBEGIN;
	if(list(req, &*commandp, &next_word, in, client_dir,
                    dir_magic_no))
                RETURNPFAILURE;
	VLDEBUGEND;
	break;
    }
        case LIST_ACL: 
            lacl_count++;
            if(retval = 
               list_acl(req, *commandp, next_word, in,
                        client_dir, dir_magic_no)) 
                return retval;
            break;
        
        case PARAMETER:
            parameter_count++;
            if(retval = parameter(req, *commandp, next_word)) return retval;
            break;
        
        case STATUS:
        {
            char 	*cp;           /* temporary */
	    int		i;

            status_count++;
            
            if (qsscanf(next_word, "%*s%r", &cp,NULL) > 0)
                return error_reply(req, "STATUS command takes no arguments, \
but we received: %s", *commandp);
            replyf(req,"Prospero server (%s) %s\n",PFS_RELEASE,hostwport);
            if(fault_count) 
                replyf(req,"Faults since startup %d\n",fault_count);
#ifdef SERVER_SUPPORT_V1
            replyf(req,"Requests since startup %d (%d V1) (%d+%d+%d %d+%d+%d \
%d %d+%d+%d %d %d %d)\n",
                   req_count, v1_req_count, list_count, goi_count, lacl_count, 
                   crlnk_count, crdir_count, crobj_count, dellnk_count, 
                   eli_count, eoi_count, eacl_count, upddir_count, 
                    parameter_count, status_count);
#else
            replyf(req,"Requests since startup %d (%d+%d+%d %d+%d %d \
%d+%d+%d %d %d %d)\n",
                   req_count, list_count, goi_count, lacl_count, 
                   crlnk_count, crobj_count, dellnk_count, 
                   eli_count, eoi_count, eacl_count, upddir_count, 
                    parameter_count, status_count);
#endif

            replyf(req,"Started: %s\n",st_time_str);
#ifdef PROSPERO_CONTACT
            replyf(req,"Contact: %s\n",PROSPERO_CONTACT);
#endif PROSPERO_CONTACT
#ifdef PFS_THREADS
            replyf(req, "Threads: %d total worker, %d(%d) active, %d free.\n",
                   subthread_count + free_subthread_count, subthread_count, 
                   subthread_max, free_subthread_count);
#endif
	    replyf(req,"  Files: %d(%d)open, %d waits, %d secs\n",
		   filelock_open,filelock_open_max, filelock_sepwaits,
		   filelock_secwaits);
#ifdef DNSCACHE_MAX
	    replyf(req,"    DNS: %d(%d) caching %d(%d)\n",
		   dnscache_count,dnscache_max,alldnscache_count,DNSCACHE_MAX);
#endif
            replyf(req," Memory: %d(%d)vlink %d(%d)pattrib %d(%d)acl \
%d(%d)pfile %d(%d)rreq %d(%d)ptext\n",
                   vlink_count,vlink_max,pattrib_count,pattrib_max,
                   acl_count,acl_max,pfile_count,pfile_max,rreq_count,
                   rreq_max,ptext_count,ptext_max);
            replyf(req, 
                    " Memory: %d(%d)string %d(%d)token %d(%d)pauth \
%d(%d)filter %d(%d)p_object\n",
/* opt not implemented yet " Memory: %d(%d)str %d(%d)tk %d(%d)pa %d(%d)opt \
%d(%d)fil\n", */
                    string_count,string_max, token_count, token_max, 
                    pauth_count, pauth_max, /* opt_count, opt_max, */
                    filter_count, filter_max, p_object_count, p_object_max);
#if 0
            replyf(req, "Memory: %d(%d)dnscache %d(%d)filelock\n", 
                   dnscache_count, dnscache_max,
                   filelock_count, filelock_max);
#endif
           replyf(req, " Memory: %d(%d)dnscache\n",
                   dnscache_count, dnscache_max /*,
                   filelock_count, filelock_max */);
#ifdef PSRV_GOPHER_GW
            replyf(req, " Memory: %d(%d)glink\n", glink_count, glink_max);
#endif
#ifdef PSRV_WAIS_GW
            replyf(req, " Memory: %d(%d)waismsgbuff %d(%d)ietftype %d(%d)waissource\n",
                   waismsgbuff_count, waismsgbuff_max, 
                   ietftype_count, ietftype_max,
                   waissource_count, waissource_max);
#endif
#ifdef DIRECTORYCACHING
	    replyf(req, "Caching: %d attempts, %d can, %d yes, %d fail\n",
		   cache_attempt, cache_can, cache_yes, dsrobject_fail);
#endif
#ifndef PSRV_READ_ONLY
            if(*pfsdat) replyf(req,"   Data: %s\n", pfsdat);
#endif PSRV_READ_ONLY
            if(*root) replyf(req,"   Root: %s\n", root);
            if(*aftpdir) replyf(req,"   AFTP: %s\n", aftpdir);
            if(*afsdir) replyf(req,"    AFS: %s\n", afsdir);
            if(db_num_ents > 0) {
		replyf(req,"     DB:");
		for(i=0;i<db_num_ents;i++) 
		    replyf(req," %s",db_prefixes[i].prefix);
		replyf(req,"\n");
	    }
            if(last_error) replyf(req,"  Error: %s\n",last_error);
/**/

#ifdef PSRV_ARCHIE
	    replyf(req, "%s", print_queue_timings());
#endif

            plog(L_DIR_PINFO, req, "STATUS Request");
            break;
        }

#ifndef PSRV_READ_ONLY
        case UPDATE: 
            upddir_count++;
            if(update(req, *commandp, next_word, in,
                      client_dir, dir_magic_no))
                RETURNPFAILURE;
            break;
#endif

        case VERSION:
            if((client_version = 
                version(req, *commandp, next_word)) < 0) 
                RETURNPFAILURE;
#ifdef SERVER_SUPPORT_V1
            if (client_version == 1) {
                ++v1_req_count;
                /* We must pass a character pointer for the start of the place
                   in the packet to grab the next token from; this is just the
                   old next_line pointer. */
                return dirsrv_v1(req,strchr(req->rcvd->text, '\n'));
            }
#endif
            /* Otherwise, we can continue to process this packet. */
            break;
	
        case UNIMPLEMENTED:
            plog(L_DIR_PERR,req,"Unimplemented message: %s", *commandp);
            creplyf(req,"FAILURE UNIMPLEMENTED %'s\n", *commandp);
            RETURNPFAILURE;
            break;

        case UNKNOWN:
            return error_reply(req, "Unknown message: %s", *commandp, 0);
            break;

        default: 
            plog(L_FAILURE,req,
                 "Somehow, we got to the default case in the giant switch \
in dirsrv (file %s, line %d).  This should never happen.", 
                 __FILE__, __LINE__); 
            return error_reply(req, "Unknown message: %s", *commandp, 0);
            break;
        }                   /* End of switch(cmd_lookup(*commandp)) */
	VLDEBUGEND;
    }               /* command processing loop end: while(in_nextline(in)) */
    creply(req,NULL);   /* Send it out! */  
    return(PSUCCESS);
}

/*
 * cmd_lookup - lookup the command name and return integer
 *
 *    CMD_LOOKUP takes a pointer to a string containing a command.
 *    It then looks up the first word found in the string and
 *    returns an int that can be used in a switch to dispatch
 *    to the correct routines.
 *
 *    This has been optimzed for the server side of the Prospero protocol.
 */

static int
cmd_lookup(char *cmd)
{

#ifdef PSRV_READ_ONLY
/* This command modifies the server data files. */
#define     modcmd(dispatch_code)   UNIMPLEMENTED
#else
#define     modcmd(dispatch_code)   (dispatch_code)
#endif

    switch(*cmd) {
    case 'A':
	if(strnequal(cmd,"AUTHENTICATE", 12))
            return AUTHENTICATOR;
#ifdef PSRV_ACCOUNT
	else if(strnequal(cmd,"ACCOUNT",7))
	    return ACCOUNT;
#endif
        else if(strnequal(cmd, "ATOMIC", 6))
            return UNIMPLEMENTED;
        else
            return UNKNOWN;
    case 'C':
        if(strnequal(cmd,"CREATE-OBJECT",11))
            return modcmd(CREATE_OBJECT);
        else if(strnequal(cmd,"CREATE-LINK",11))
            return modcmd(CREATE_LINK);
        else return UNKNOWN;
    case 'D':
        if(strnequal(cmd,"DELETE-LINK",11))
            return modcmd(DELETE_LINK);
        else if(strnequal(cmd,"DIRECTORY",9))
            return DIRECTORY;
        else return UNKNOWN;
    case 'E':
        if (strnequal(cmd, "EDIT-LINK-INFO", 14))
            return modcmd(EDIT_LINK_INFO);
        else if(strnequal(cmd,"EDIT-OBJECT-INFO",16))
            return modcmd(EDIT_OBJECT_INFO);
        else if (strnequal(cmd, "EDIT-ACL", 8))
            return modcmd(EDIT_ACL);
        else 
            return UNKNOWN;
    case 'G':
        if(strnequal(cmd,"GET-OBJECT-INFO",13))
            return GET_OBJECT_INFO;
        else return UNKNOWN;
    case 'L':
        if(strnequal(cmd,"LIST-ACL",8))
            return LIST_ACL;
        else if(strnequal(cmd,"LIST",4))
            return LIST;
        else return UNKNOWN;
    case 'P':
        if(strnequal(cmd, "PARAMETER", 9))
            return PARAMETER;
        else return UNKNOWN;
    case 'S':
        if(strnequal(cmd,"STATUS",6))
            return STATUS;
        else return UNKNOWN;
    case 'U':
        if(strnequal(cmd,"UPDATE",6))
            return modcmd(UPDATE);
        else return UNKNOWN;
    case 'V':
        if(strnequal(cmd,"VERSION",7))
            return VERSION;
        else return UNKNOWN;
    default:
        return UNKNOWN;
    }
}


SIGNAL_RET_TYPE term_sig()
{
    char		qlstring[30];

    if(pQlen > 0) sprintf(qlstring," [%d pending]",pQlen);
    else *qlstring = '\0';

    plog(L_STATUS, NOREQ,
	 "Server killed (terminate signal received)%s",qlstring);
    log_server_stats();
    exit(0);
}

SIGNAL_RET_TYPE restart_sig()
{
    char		qlstring[30];

    if(pQlen > 0) sprintf(qlstring," [%d pending]",pQlen);
    else *qlstring = '\0';

    plog(L_STATUS, NOREQ,
	 "Attempting server restart (restart signal received)%s",qlstring);
    restart_server(fault_count,(char *) NULL);
}

SIGNAL_RET_TYPE trap_error(sig, code, scp)
    int			sig;
    int			code;
#ifndef SCOUNIX
    struct  sigcontext	*scp;
#endif
{
    char		estring[400];
#ifdef COREDUMP
    int pid;

    if ( (pid = fork()) >  0 ) {	/* parent */

	signal(SIGIOT,SIG_DFL);
	signal(SIGABRT,SIG_DFL);
	abort();
	exit(-1);
    }
    else {		/* child */

	sleep(5); /* just make sure that the parent creates the core */
#endif

    if(pQlen > 0) qsprintf(estring, sizeof estring, " [%d pending]",pQlen);
    else *estring = '\0';

#ifdef SIGCONTEXT_LACKS_SC_PC
    plog(L_FAILURE,NOREQ,"Failure - Recovering from error (%d,%d)%s",sig,code,estring);
#else
    plog(L_FAILURE,NOREQ,"Failure - Recovering from error at address 0x%x (%d,%d)%s",scp->sc_pc,sig,code,estring);
#endif
#ifdef DIRSRV_EXPLAIN_LAST_RESTART
    plog(L_DIR_ERR,NOREQ,"Last request was: %s",last_request);
#ifdef SIGCONTEXT_LACKS_SC_PC
    qsprintf(estring,sizeof estring, "Signal(%d,%d)\n%s", sig, code,
	    last_request);
#else
    qsprintf(estring,sizeof estring, "Signal(%d,%d) at 0x%x\n%s", sig, code,
	    scp->sc_pc, last_request);
#endif
#else
#ifdef SIGCONTEXT_LACKS_SC_PC
    qsprintf(estring,sizeof estring, "Signal(%d,%d) at 0x%x", sig, code);
#else
    qsprintf(estring,sizeof estring, "Signal(%d,%d) at 0x%x", sig, code,
             scp->sc_pc);
#endif
#endif DIRSRV_EXPLAIN_LAST_RESTART

    restart_server(fault_count,estring);
    abort();
#ifdef COREDUMP
    }
#endif

}

/*
 * setup_disc 
 *
 *      disconnect all descriptors, remove ourself from the process
 *      group that spawned us and set signal handlers.
 */
void
setup_disc()
{

    int     s;

    for (s = 0; s < 3; s++) {
        (void) close(s);
    }

    /* This routine is called EXCLUSIVELY by the MASTER thread before going
       multithreaded. */
    assert(P_IS_THIS_THREAD_MASTER());
    (void) open("/dev/null", 0, 0);
    (void) dup2(0, 1);       /* Under Solaris 2.3, dup2() is MT unsafe; master
                                thread only */ 
    (void) dup2(0, 2);       /* MT unsafe; master thread only */

#ifdef SETSID
    setsid();
#else
    s = open("/dev/tty", 2, 0);

    if (s >= 0) {
        ioctl(s, TIOCNOTTY, (struct sgttyb *) 0);
        (void) close(s);
    }
#endif

    signal(SIGHUP,term_sig);
    signal(SIGINT,term_sig);
    signal(SIGTERM,term_sig);
    signal(SIGQUIT,term_sig);

    signal(SIGILL,trap_error);
    signal(SIGIOT,trap_error);
    signal(SIGEMT,trap_error);
    signal(SIGFPE,trap_error);
    signal(SIGBUS,trap_error);
    signal(SIGSEGV,trap_error);
    signal(SIGPIPE,trap_error);    /* Seen these with non-blocking io */
    signal(SIGSYS,trap_error);

    signal(SIGUSR1,restart_sig);
#if defined(AIX) || defined(SOLARIS)
    bsdSignal(SIGUSR2,close_plog);
#else
    signal(SIGUSR2,close_plog);
#endif
    return;
}

void
log_server_stats()
{
    plog(L_STATS,NOREQ,"Stats: %d List, %d GOI, %d LACL",
	 list_count,goi_count,lacl_count);
#ifdef SERVER_SUPPORT_V1
    plog(L_STATS,NOREQ,"       %d CL, %d CD, %d CO, %d DL",
	 crlnk_count,crdir_count,crobj_count,dellnk_count);
#else
    plog(L_STATS,NOREQ,"       %d CL, %d CO, %d DL",
	 crlnk_count,crobj_count,dellnk_count);
#endif
     plog(L_STATS,NOREQ,"       %d ELI, %d EOI, %d EACL, %d Upd", 
	 eli_count,eoi_count,eacl_count,upddir_count,0);
#ifdef SERVER_SUPPORT_V1
    if(v1_req_count) plog(L_STATS,NOREQ,"       %d Total, %d V1 format", 
			   req_count, v1_req_count);
    else 
#endif
        plog(L_STATS,NOREQ,"       %d Total", req_count);

}

/* Check a handle (the filename portion of a system level name) and make sure
   that we are allowed to access it.  This uses the definitions from
   "psite.h". */

int
check_handle(char *handle)
{
    int		i;
#ifdef NODOTDOT
    char 	*t_handle = NULL;
#endif
    
    if(*handle != '/') {  /* Database or special prefix in use */
	for(i=0;i<db_num_ents;i++) {
	    if(strnequal(handle, db_prefixes[i].prefix,
                         strlen(db_prefixes[i].prefix)))
		return(TRUE);
	}
    }

#ifdef NODOTDOT
    t_handle = qsprintf_stcopyr(t_handle, "/%s/", handle);
    if(sindex(t_handle, "/../")) return FALSE;
    stfree(t_handle); t_handle = NULL;
#endif
    if(strnequal(handle,pfsdat, strlen(pfsdat))) return TRUE;
    if(*root && strnequal(handle,root, strlen(root))) return TRUE;
    if(*aftpdir && strnequal(handle,aftpdir, strlen(aftpdir))) return TRUE;
    if(*aftpdir && strnequal(handle, "AFTP", 4)) return TRUE;
    if(*afsdir && strnequal(handle, afsdir, strlen(afsdir))) return TRUE;
    return FALSE;
}

/* 
 * dirsrv has an internal error handler.   This
 * is used by the internal_error macro and by the p__finternal_error()
 * function.
 */
static void
dirsrv_internal_error_handler(file, line, mesg)
    char file[];
    int line;
    char mesg[];
{
    /* if we get into a loop (if qsscanf or plog or restart_server have
       internal errors), we'll bail out eventually.  */ 
    /* Assume that incrementing this is an atomic operation; is on all
       architectures.  This is not a good choice, but since an internal error
       has already occurred, we don't want to assume that the multithreading
       kernel is working perfectly. */ 
    static int been_here_before = 0; 
    char estring[400];

    if (been_here_before++ < 5) {
        /* Problem: if internal errors or assertion problems occur inside
           PLOG (including running out of memory!) then we are in trouble.
           */
        plog(L_FAILURE, NOREQ,
             "Internal error in file %s, line %d: %s", file, line, mesg);
        if (mflag) 
            plog(L_FAILURE, NOREQ, "In manual mode; not restarting server.");
        else
            plog(L_FAILURE, NOREQ, "Trying to restart server.");
        qsprintf(estring, sizeof(estring), 
                 "Internal error in file %s, line %d: %s", file, line, mesg, 0);
    }
    if (been_here_before < 10) {
        if(!mflag) 
            restart_server(++fault_count, estring);
        else 
            abort();
    }
    write(2, "In dirsrv.c, dirsrv_internal_error_handler(): \
This should never happen.\n", 
          sizeof "In dirsrv.c, dirsrv_internal_error_handler(): \
This should never happen.\n");
    if (mflag) abort();                    /* and if THAT fails... */
    _exit(1);                   /* final desperation play. */
    write(2, "In dirsrv.c, dirsrv_internal_error_handler(): \
_exit(1) didn't abort execution!  This should REALLY never happen.\n", 
          sizeof "In dirsrv.c, dirsrv_internal_error_handler(): \
_exit(1) didn't abort execution!  This should REALLY never happen.\n");
}


/* This function will execute both a vplog() and an vsendmqf() in order to
   report on a failure.
   It returns PFAILURE, since it should only be used if an operation
   cannot be performed.
   It also automatically prefixes the words 
   "FAILURE AUTHENTICATION-DATA" to the reply packets.
   It appends the appropriate newlines, so you don't have to.
*/

static int 
auth_fail_reply(RREQ req, char *format, ...)
{
    va_list ap;
    char *bufp;
    
    va_start(ap, format);
    
    
    bufp = vplog(L_AUTH_ERR, req, format, ap); /* return formatted string */

    reply(req, "FAILURE AUTHENTICATION-DATA ");
    reply(req, bufp);
    creply(req, "\n");
    va_end(ap);
    RETURNPFAILURE;
}


#ifdef PSRV_ACCOUNT
/* Need to add overflow checking and normalization in these routines. */
/* Converts string of form "xxx.yy" into two integers a and b such */
/* that xxx.yy = a * 10^y */
static int 
str_to_fp(char *amount, struct decimal *amt)
{
    char *cp;
    char *lim;

    if (!amount)
	RETURNPFAILURE;

    if (!amt)
	amt = (struct decimal *) stalloc(sizeof(struct decimal));

    for(cp = amount; *cp && *cp != '.'; cp++);

    if (!*cp) {
	sscanf(amount, "%d", &amt->sig);
	amt->exp = 0;
    }
    else {
	lim = stalloc(strlen(amount));
	lim = strncpy(lim, amount, cp-amount);
	lim = strcat(lim, cp+1);
	sscanf(lim, "%d", &amt->sig);
	amt->exp = -strlen(cp+1);
	stfree(lim);
    }
    return PSUCCESS;
}

static int 
fp_to_str(struct decimal amt, char *amount)
{
    char tmp[255];
    int i;

    qsprintf(tmp, sizeof tmp, "%d", amt.sig);
    
    if (!amount)
	amount = (char *) stalloc(strlen(tmp)+2);

    if (amt.exp < 0) {
	amount = strncpy(amount, tmp, strlen(tmp)+amt.exp);
	amount = strcat(amount, ".");
	amount = strcat(amount, tmp+strlen(tmp)+amt.exp);
    }
    else if (amt.exp > 0) {
	amount = strcpy(amount, tmp);
	for (i = 0; i < amt.exp; i++)
	    amount = strcat(amount, "0");
    }
    else
	amount = strcpy(amount, tmp);
    return PSUCCESS;
}
    
/* Subtracts amt2 from amt1 and returns amount left in result */
static int subtract_fp(struct decimal amt1, struct decimal amt2,
		       struct decimal *result)
{
    while (amt1.exp > amt2.exp) {
	amt1.exp--;
	amt1.sig *= 10;
    }
    while (amt1.exp < amt2.exp) {
	amt2.exp--;
	amt2.sig *= 10;
    }

    if (amt1.sig < amt2.sig)
	RETURNPFAILURE;

    if (!result)
	result = (struct decimal *) stalloc(sizeof(struct decimal));
    result->sig = amt1.sig - amt2.sig;
    result->exp = amt1.exp;
    return PSUCCESS;
}

/* Adds amt1 to amt2 and returns amount in result */
static int add_fp(struct decimal amt1, struct decimal amt2, 
		  struct decimal *result)
{
    while (amt1.exp > amt2.exp) {
	amt1.exp--;
	amt1.sig *= 10;
    }
    while (amt1.exp < amt2.exp) {
	amt2.exp--;
	amt2.sig *= 10;
    }

    if (!result)
	result = (struct decimal *) stalloc(sizeof(struct decimal));
    result->sig = amt1.sig + amt2.sig;
    result->exp = amt1.exp;
    return PSUCCESS;    
}

static int charge(RREQ req, char *currency, struct decimal amount,
		  char *vendor, struct decimal vendor_amount, 
		  char *vendor_currency, char *description, 
		  char *signature)
{
    PAUTH auth_info;
    struct amt_info *amtp;
    struct decimal amt_left;
    static char charge_amt[255], vendor_amt[255];

    if (!req || !currency || !description)
	RETURNPFAILURE;
    if (!vendor)
	vendor = stcopy("");
    if (!vendor_currency)
	vendor_currency = stcopy("");
    if (!signature)
	signature = stcopy("");

    fp_to_str(amount, charge_amt);
    fp_to_str(vendor_amount, vendor_amt);

    auth_info = req->auth_info;
    for (amtp = auth_info->allocated_amts; 
	 amtp && strcmp(currency, amtp->currency);
	 amtp = amtp->next);

    if (!amtp)
	RETURNPFAILURE;

    /* Find amount left */
    if (subtract_fp(amtp->limit, amtp->amt_spent, &amt_left))
	RETURNPFAILURE; /* Should never happen! */

    /* Check if amount left sufficient */
    if (subtract_fp(amt_left, amount, 0)) 
	RETURNPFAILURE; /* Insufficient funds */
    
    /* Add amount charged to total amount spent */
    add_fp(amtp->amt_spent, amount, &amtp->amt_spent);

    plog(L_ACCOUNT, NOREQ, "Charged %'s %'s %'s %'s %'s %'s %'s, \
Vendor: %'s %'s %'s, Transaction: %'s\n",
	 charge_amt, currency, acc_method_name(auth_info->ainfo_type), 
	 auth_info->acc_server, auth_info->acc_name, signature,
	 auth_info->int_bill_ref, vendor, 
	 vendor_amt, vendor_currency, description);

    creplyf(req, "RECEIPT %'s %'s %'s %'s %'s %'s %'s %'s\n",
	    acc_method_name(auth_info->ainfo_type),
	    auth_info->acc_server, auth_info->acc_name, signature,
	    auth_info->int_bill_ref, description,
	    currency, charge_amt);
	    
    return PSUCCESS;
}

static char *acc_method_name(int acc_method)
{
    int i;

    /* Search for acc_method in array */
    for (i=0; acc_methods[i].acc_method_num &&
	      acc_methods[i].acc_method_num != acc_method;
	 i++);
    if (!acc_methods[i].acc_method_num)
	return NULL;   /* Unknown accounting method */
    
    return acc_methods[i].acc_method_name;
}

#endif
