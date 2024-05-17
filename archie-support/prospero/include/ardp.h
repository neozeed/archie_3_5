
/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <stdio.h>              /* for FILE */
#ifdef ARCHIE_TIMING
#include <time.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

/*
 * Written  by bcn 1989-92  as pfs.h in the Prospero distribution
 * Modified by bcn 1/93     separate include file and extended preq structure
 * Modified by swa 11/93    error reporting; less dependent on Prospero
 */

#ifndef ARDP_H_INCLUDED

#define ARDP_H_INCLUDED
#include <pfs_threads.h>        /* PFS threads package. */

#include <sys/time.h>
#include <sys/types.h>

/* Unfortunately, some buggy compilers (in this case, gcc 2.1 under
   VAX BSD 4.3) get upset if <netinet/in.h> is included twice. */
/* Note that the IEEE C standard specifies that multiple inclusions of an
   include file should not matter. */
#if !defined(IP_OPTIONS)
#include <netinet/in.h> 
#endif

#include <list_macros.h>

/* Note: In doubly linked lists of the structures used for prospero, the  */
/* ->previous element of the head of the list is the tail of the list and */
/* the ->next element of the tail of the list is NULL.  This allows fast  */
/* addition at the tail of the list.                                      */

/* #defining this turns on extra consistency checking code in the various
   memory  allocators that checks for double freeing.  Turn on this compilation
   option if you are experimenting with the PFS library and want to make sure
   you are not making this common mistake.   We have left it turned on for this
   ALPHA release. */
/* This consistency checking mechanism is used by the ARDP library and general
   Prospero data structures.  The only exception is the RREQ structure, which
   does consistency checking differently. */
#define P_ALLOCATOR_CONSISTENCY_CHECK
#define ALLOCATOR_CONSISTENCY_CHECK /* deprecated but still necessary */
#ifdef P_ALLOCATOR_CONSISTENCY_CHECK /* patterns to set the 'consistency' member
                                      to.  */
#define FREE_PATTERN P__FREE_PATTERN /* old interface deprecated */
#define INUSE_PATTERN P__INUSE_PATTERN /* old interface deprecated */
#define P__FREE_PATTERN     0Xca01babe /* pattern to use if memory is now free. */
#define P__INUSE_PATTERN    0X53ad00d  /* Pattern to set consistency member to
                                       if memory is in use. */
#endif

#define PROSPERO

#ifdef PROSPERO
#define	     ARDP_DEFAULT_PEER	 "dirsrv"/* Default destination port name   */
#define	     ARDP_DEFAULT_PORT    1525   /* Default destination port number */
#endif /* PROSPERO */

#define      ARDP_BACKOFF(x)   (2 * x)   /* Backoff algorithm               */
#define      ARDP_DEFAULT_TIMEOUT    4	 /* Default time before retry (sec) */
#define      ARDP_DEFAULT_RETRY	     3	 /* Default number of times to try  */
#define	     ARDP_DEFAULT_WINDOW_SZ 16   /* Default maximum packets to send at
                                            once, unless special request
                                            received from client. */
#define      ARDP_MAX_WINDOW_SZ    256   /* Maximum # of packets we'll send
                                            out, no matter what a client
                                            requests.   Might be redefined
                                            locally if you're at the far end of
                                            a slow link. */
#define	     ARDP_FIRST_PRIVP	   901	 /* First prived local port to try  */
#define	     ARDP_NUM_PRIVP	    20	 /* Number of prived ports to try   */
#define	     ARDP_PTXT_HDR	    64   /* Max offset for start            */
#define	     ARDP_PTXT_LEN	  1250   /* Max length for data to send     */
#define	     ARDP_PTXT_LEN_R	  1405	 /* Max length for received data    */

/* Rationale for MAX_PTXT_LEN_R: According to IEEE std. 802.5, 1492 is the  */
/* max data length for an ethernet packet.  IP implementers tend to use     */
/* this information in deciding how large a packet could be sent without IP */
/* fragmentation occurring.  Subtract 16 octets for the IP header.          */
/* Subtract 8 octets for UDP header. The maximum ARDP header size is 6 bits */
/* or 64, so subtract 64 more for the ARDP header.  This leaves 1404 bytes. */
/* Note we only generate 1250 bytes because there are preV5 implementations */
/* out there with smaller limits. We also add one to MAX_PTXT_LEN_R to make */
/* sure there is always room to insert a null if needed.                    */

/* Must leave room to insert header when sending and to strip it on receipt */
#define		ARDP_PTXT_DSZ	ARDP_PTXT_LEN_R+2*ARDP_PTXT_HDR

/* Definition of text structure used to pass around each packet */
struct ptext {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int                 consistency;
#endif
    unsigned int       seq;		  /* Packet sequence number	    */
    int		       length;		  /* Length of text (from start)    */
    char	       *start;		  /* Start of packet		    */
    char	       *text;		  /* Start of text                  */
    char	       *ioptr;		  /* Current position for i/o       */
    char	       dat[ARDP_PTXT_DSZ];/* The data itself incl headers   */
    unsigned long      mbz;		  /* ZERO to catch runaway strings  */
    struct ptext       *previous;         /* Previous element in list       */
    struct ptext       *next;		  /* Next element in linked list    */
};

typedef struct ptext *PTEXT;
typedef struct ptext PTEXT_ST;
#define NOPKT   ((PTEXT) 0)               /* NULL pointer to ptext          */


/* Request structure: maintains information about pending requests          */
struct rreq {
    int         	status;		  /* Status of request          *rr1 */
    int			flags;		  /* Options for ARDP library   *rr2 */
    unsigned short	cid;		  /* Connection ID - net byte order */
    short		priority;	  /* Priority - host byte order     */
    int			pf_priority;	  /* Priority assigned by pri_func  */
    short		peer_ardp_version;/* Peer ARD protocol version *rr3 */
    struct ptext	*inpkt;		  /* Packet in process by applic    */
    unsigned short	rcvd_tot;	  /* Total # of packets to receive  */
    struct ptext	*rcvd;		  /* Received packets               */
    unsigned short	rcvd_thru;	  /* Received all packets through # */
    struct ptext	*comp_thru;	  /* Pointer to RCVD_THRUth packet  */
    struct ptext	*outpkt;	  /* Packets not yet sent           */
    unsigned short	trns_tot;	  /* Total # of packets for trns    */
    struct ptext	*trns;		  /* Transmitted packets            */
    unsigned short	prcvd_thru;	  /* Peer's rcvd_thru (host byte ord)*/
    unsigned short      window_sz;        /* My window size (default: 0 (sender
                                             does whatever they choose)
                                             (hostorder) */
    unsigned short      pwindow_sz;       /* Peer window (dflt ARDP_WINDOW_SZ).
                                             0 means no limit on # to send. 
                                             (hostorder)*/ 
    struct sockaddr_in	peer;   	  /* Sender/Destination		    */
#define peer_addr       peer.sin_addr	  /* Address in network byte order  */
#define peer_port       peer.sin_port	  /* Port in network byte order     */
    struct timeval	rcvd_time;	  /* Time request was received      */
    struct timeval	svc_start_time;	  /* Time service began on request  */
    struct timeval	svc_comp_time;	  /* Time service was completed     */
    struct timeval	timeout;	  /* Initial time to wait for resp  */
    struct timeval	timeout_adj;      /* Adjusted time to wait for resp */
    struct timeval	wait_till;	  /* Time at which timeout expires  */
    unsigned short	retries;	  /* Number of times to retry       */
    unsigned short	retries_rem;	  /* Number of retries remaining    */
    unsigned short	svc_rwait;	  /* Svc suggested tm2wait b4 retry */
    unsigned short	svc_rwait_seq;	  /* Seq # when scv_rwait changed   */
    unsigned short	inf_queue_pos;    /* Service queue pos if available */
    int			inf_sys_time;     /* Expected time til svc complete */
#ifdef PROSPERO
    struct pfs_auth_info *auth_info;      /* Client authentication info     */
#endif /* PROSPERO */
    char		*client_name;     /* Client name string for logging */
    char		*peer_sw_id;	  /* Peer's software specific ident */
    int		        (*cfunction)();   /* Function to call when done     */
    char		*cfunction_args;  /* Additional args to cfunction   */
    struct rreq		*previous;        /* Previous element in list       */
    struct rreq		*next;		  /* Next element in linked list    */
#ifdef ARCHIE_TIMING
    int                 no_matches;       /* Number of matches returned */
    char                query_state;      /* State of query S,F */
    char                match_type;       /* Type of match E,S,R */
    char                cached;
    char                case_type;        /* Case  S,X */
    char                domain_match;     /* Domain match D,X */
    char                path_match;	  /* Path match P,X */
    char                search_str[MAXPATHLEN]; /* search string */
    char                launch[30];       /* Time started the processing */
    char                start[30];        /* Time got request */
    char                end[30];          /* Time returned request */
    struct rusage       qtime_start;	  /* To figure out the timings */
    struct rusage       qtime_end;
    struct rusage       stime_start;
    struct rusage       stime_end;
    struct rusage       htime_start;
    struct rusage       htime_end;
    int                 hosts;		  /* Hosts that contain */
    int                 hosts_searched;   /* Hosts searched */
#endif
};
/* *rr1: this is also used for consistency checking. */
/* Values for this field: */
#define ARDP_STATUS_NOSTART   0 /* not started or inactive */
#define ARDP_STATUS_INACTIVE  0
#define ARDP_STATUS_COMPLETE -1 /* done */
#define ARDP_STATUS_ACTIVE   -2 /* running */
#define ARDP_STATUS_ACKPEND  -3
#define ARDP_STATUS_GAPS     -4
#define ARDP_STATUS_ABORTED  -5
#define ARDP_STATUS_FREE     -6 /* used for consistency checking; free */
#define ARDP_STATUS_FAILED   255

/* *rr2: There is only one flag currently defined.  */
/* Send the client's preferred window size along with the next packet you
   transmit.  At the moment, this flag is never cleared once it is set, so all
   packets are tagged with the preferred window size.  There are other
   strategies that will be tried later.  */
/* At the moment, this flag is only set in ardp_rqalloc(). */
#define ARDP_FLAG_SEND_MY_WINDOW_SIZE 0x1
#ifdef PROSPERO
/* This is used only on the clients.  It means they're when speaking to a
   Version 0 Prospero server, but one which used the Prospero 5.0, 5.1, or 5.2
   releases. */
#define ARDP_FLAG_PEER_HAS_EXCESSIVE_ACK_BUG 0x2
#endif

/* *rr3: There are several odd numbers you might find. */
/* 0 is the current version of the ARDP library. */
/* -1 is an older version.   This is used only on the servers when speaking to
   V1 archie clients. */
/* -2 is a newer version than -1, but still old enough that bugs appeared.  
   This is used only on the servers when speaking to V1 archie clients. */
/* Version 1 of the ARDP library has been designed.  Not yet implemented. */

#define S_AD_SZ           sizeof(struct sockaddr_in)

#define PEER_PORT(req)  (ntohs((req)->peer_port)) /* host byte order        */

typedef struct rreq *RREQ;
typedef struct rreq RREQ_ST;
#define NOREQ   ((RREQ) 0)                /* NULL pointer to rreq           */

/* ARDP library error status codes. */
/* These must remain in the range 0-20 for compatibility with the Prospero
   File System. */
/* Note that ARDP_SUCCESS must remain 0, due to code implementation. */
#define ARDP_SUCCESS		0	/* Successful completion of call    */
#define ARDP_PORT_UNKN		1	/* DIRSRV UDP port unknown          */
#define ARDP_UDP_CANT		2	/* Can't open local UDP port        */
#define ARDP_BAD_HOSTNAME	3	/* Can't resolve hostname           */
#define ARDP_NOT_SENT		4	/* Attempt to send message failed   */
#define ARDP_SELECT_FAILED	5	/* Select failed	            */
#define ARDP_BAD_RECV		6	/* Recvfrom failed 	            */
#define ARDP_BAD_VERSION	7       /* bad version # in rdgram protocol */
#define ARDP_BAD_REQ		8	/* Inconsistent request structure   */
#define ARDP_TIMEOUT            9       /* Timed out - retry count exceeded */
#define	ARDP_REFUSED 	       10	/* Connection refused by server     */
#define	ARDP_FAILURE 	       11	/* Unspecified ARDP failure         */
#define ARDP_TOOLONG	       12       /* Buffer too long for packet       */
/* These are the interface used to the ARDP library's error reporting.
   These are the same as definitions used in the Prospero PFS library. */
/* extern int perrno;               \* Place where error codes are set. */
extern void p_clear_errors(void); /* Clear perrno in ARDP library; clear pwarn
                                     and p_err_string and p_warn_string as well
                                     in PFS library. */

#define ARDP_PENDING           -1       /* The request is still pending     */
#define ARDP_WAIT_TILL_TO      -1       /* Wait until timeout occurs        */

#define ARDP_A2R_SPLIT       0x00       /* OK to split across packets       */
#define ARDP_A2R_NOSPLITB    0x01       /* Don't split across packets       */
#define ARDP_A2R_NOSPLITL    0x02       /* Don't split lines across packets */
#define ARDP_A2R_NOSPLITBL   0x03       /* NOSPLITB|NOSPLITL                */
#define ARDP_A2R_TAGLENGTH   0x04       /* Include length tag for buffer    */
#define ARDP_A2R_COMPLETE    0x08       /* This is the final packet to add  */

#define ARDP_R_INCOMPLETE    0x00       /* More messages will follow        */
#define ARDP_R_NOSEND        0x02       /* Add to req->trns but don't send  */
#define ARDP_R_COMPLETE      0x08       /* This is the final packet to send */

/* Queuing priorities for requests */
#define	       ARDP_MAX_PRI   32765  /* Maximum user proiority          */
#define	       ARDP_MAX_SPRI  32767  /* Maximum priority for system use */
#define	       ARDP_MIN_PRI  -32765  /* Maximum user proiority          */
#define	       ARDP_MIN_SPRI -32768  /* Maximum priority for system use */

/* LONG_TO_SHORT_NAME is needed for linkers that can't handle long names */
#ifdef LONG_TO_SHORT_NAME
#define ardp_abort                RDABOR
#define ardp_abort_on_int         RDABOI
#define ardp_accept               RDACPT
#define ardp_activeQ              RDACTV
#define ardp_add2req		  RDA2RQ
#define ardp_acknowledge          RDACKN
#define ardp_bind_port            RDBPRT
#define ardp_breply               RDBREP
#define ardp_completeQ            RDCMPQ
#define ardp_def_port_no          RDDPNO
#define ardp_default_retry        RDDFRT
#define ardp_default_timeout      RDDFTO
#define ardp_doneQ                RDDONQ
#define ardp_get_nxt              RDGNXT
#define ardp_get_nxt_nonblocking  RDGNNB
#define ardp_headers              RDHDRS
#define ardp_init	          RDINIT
#define ardp_next_cid	          RDNCID
#define ardp_partialQ             RDPRTQ
#define ardp_pendingQ             RDPNDQ
#define ardp_port	          RDPORT
#define ardp_pri_func             RDPRIF
#define ardp_pri_override         RDOVRD
#define ardp_priority             RDPRIO
#define ardp_process_active       RDPACT
#define ardp_prvport              RDPPRT
#define ardp_ptalloc              RDPTAL
#define ardp_ptfree               RDPTFR
#define ardp_ptlfree              RDPTLF
#define ardp_redirect             RDREDR
#define ardp_refuse               RDRFSE
#define ardp_reply                RDREPL
#define ardp_respond              RDRESP
#define ardp_retrieve             RDRETR
#define ardp_rqalloc              RDRQAL
#define ardp_rqfree               RDRQFR
#define ardp_rq_partialfree       RDRQPF
#define ardp_rqlfree              RDRQLF
#define ardp_runQ                 RDRUNQ
#define ardp_rwait                RDRWAI
#define ardp_send                 RDSEND
#define ardp_set_prvport          RDSPPT
#define ardp_set_queuing_policy   RDSQPL
#define ardp_set_retry            RDSETR
#define ardp_showbuf              RDSHBF
#define ardp_snd_pkt              RDSPKT
#define ardp_srvport              RDSPRT
#define ardp_trap_int             RDTINT
#define ardp_update_cfields       RDUPCF
#define ardp_xmit                 RDXMIT
#define ptext_count               PTXCNT
#define ptext_max                 PTXMAX
#define rreq_count                RRQCNT
#define rreq_max                  RRQMAX
#endif /* LONG_TO_SHORT_NAME */

/* Please keep this list alpphabetically sorted. */

int		ardp_abort(RREQ);
int		ardp_abort_on_int(void);
int		ardp_accept(void);
int		ardp_add2req(RREQ,int,char*,int);
int             ardp_acknowledge(RREQ req);
int		ardp_bind_port(char*);
int		ardp_breply(RREQ,int,char*,int);
RREQ		ardp_get_nxt(void);
RREQ		ardp_get_nxt_nonblocking(void);
int		ardp_headers(RREQ);
/* Variable name "hostname" Commented out to shut up GCC -Wshadow */
int             ardp_hostname2addr(const char * /* hostname */, struct sockaddr_in *hostaddr);
void            ardp_hostname2addr_initcache(void);
extern	int	ardp_priority;
int		ardp_process_active(void);
PTEXT		ardp_ptalloc(void);
void		ardp_ptfree(PTEXT);
void		ardp_ptlfree(PTEXT);
int		ardp_reply(RREQ,int,char*);
int		ardp_respond(RREQ,int);
int		ardp_retrieve(RREQ,int);
RREQ		ardp_rqalloc(void);
void		ardp_rqfree(RREQ);
#ifndef NDEBUG
void		ardp_rq_partialfree(RREQ);
#endif
void		ardp_rqlfree(RREQ);
int		ardp_rwait(RREQ,int,short,int);
int		ardp_send(RREQ,char*,struct sockaddr_in*,int);
int		ardp_set_prvport(int);
int		ardp_set_queuing_policy(int(*pf)(),int);
int		ardp_set_retry(int,int);
void            ardp_showbuf(const char *st, int length, FILE *out);
int		ardp_snd_pkt(PTEXT,RREQ);
int		ardp_xmit(RREQ,int);
/* these are used to look for memory leaks.   Currently used by dirsrv.c to
   return STATUS information.  Internal to ARDP library.  */
extern int dnscache_count;     
extern int dnscache_max;
/* Used to see how many cached items are in */
extern int alldnscache_count;

/* Checked for memory leaks. */
extern int filelock_count;
extern int filelock_max;
extern int      pfs_debug;      /* used in ARDP library for error messages. */
                                /* Should be renamed. */
/* these are used to look for memory leaks.   Currently used by dirsrv.c to
   return STATUS information. */
extern int ptext_count;         
extern int ptext_max;
extern int rreq_count;
extern int rreq_max;
/* Check for case-blind string equality, where s1 or s2 may be null
   pointer. */
int stcaseequal(const char *s1, const char*s2);
extern const char*unixerrstr(void);


/* File locking prototypes. */
/* Could these be made ARDP internal?  hope so. */
#include <../lib/ardp/flocks.h>
FILELOCK filelock_alloc(void);
void         filelock_free(FILELOCK fl);
void filelock_lfree(FILELOCK fl);
void filelock_freespares(void);
void filelock_release(const char *filename, int readonly);
void filelock_obtain(const char *filename, int readonly);

/* File locking functions.   Also in pfs_threads.h */
extern int locked_fclose_A(FILE *afle, const char *filename, int readonly);
extern FILE *locked_fopen(const char *filename, const char *mode);
int locked_fclose_and_rename(FILE *afile, const char *tmpfilename, const char *filename, int retval);
extern void locked_clear(FILE *a); /* also in pfs_threads.h */


/* Mutex stuff for pfs_threads on server side only still.. */
extern void ardp_init_mutexes(void); /* need not be called. */
#ifndef NDEBUG
extern void ardp_diagnose_mutexes(void); /* need not be called. */
#endif /*NDEBUG*/
EXTERN_MUTEXED_DECL(RREQ, ardp_runQ);
EXTERN_MUTEXED_DECL(RREQ, ardp_doneQ);
EXTERN_MUTEXED_DECL(RREQ, ardp_pendingQ);
#ifdef PFS_THREADS
extern p_th_mutex p_th_mutexARDP_ACCEPT; /* declared in ardp_mutexes.c */
extern p_th_mutex p_th_mutexPTEXT; /* declared in ardp_mutexes.c */
extern p_th_mutex p_th_mutexARDP_RQALLOC; /* declared in ardp_mutexes.c */
extern p_th_mutex p_th_mutexGETHOSTBYNAME; /* declared in ardp_mutexes.c */
extern p_th_mutex p_th_mutexARDP_SELFNUM; /* declared in ardp_mutexes.c */
#endif

#include <implicit_fixes.h>      /* Fixes for implicit definitions */
/* Internal error handling code (formerly in pfs.h). */

/* Internal error handling routines used by the pfs code; formerly in *
 * internal_error.h.  These include a replacement for the assert()    *
 * macro, and an interface for internal error handling, better        *
 * documented in internal_error.c                                     */
/* XXX These are duplicated in include/pfs_utils.h; need to do code 
   cleanup here. */

#ifndef NDEBUG
#ifndef assert
#define assert(expr) \
    do { \
        if (!(expr)) \
            internal_error("assertion violated: " #expr); \
    } while(0)
#endif                          /* assert */
#else /* NDEBUG */
#ifndef assert
#define assert(expr) do {;} while(0)
#endif                          /* assert() */
#endif /* NDEBUG */



#if 0                           /* OLD version -- keep until new one works */
#define internal_error(msg) \
  do { \
      write(2, "Internal error in file " __FILE__ ": ", \
            sizeof "Internal error in file " __FILE__ ": " -1); \
      write(2, msg, strlen(msg)); \
      write(2, "\n", 1);        \
      if (internal_error_handler)   \
          (*internal_error_handler)(__FILE__, __LINE__, msg);   \
      else  { \
          fprintf(stderr, "line of error: %d\n", __LINE__); \
          abort(); \
      } \
  } while(0)
#else
#ifndef internal_error
#define internal_error(msg) \
    internal_error_helper(msg, __LINE__)
#define internal_error_helper(msg,line) \
  do { \
     write(2, "Internal error in file " __FILE__ " (line " #line "): ",\
     sizeof "Internal error in file " __FILE__ " (line " #line "): " -1);\
     write(2, msg, strlen(msg)); \
     write(2, "\n", 1);        \
     if (internal_error_handler)   \
         (*internal_error_handler)(__FILE__, line, msg);   \
     /* If the internal_error_handler() ever returns, we should not continue.
        */ \
     abort(); \
} while(0)
#endif                          /* internal_error() */
#endif

/* This function may be set to handle internal errors.  Dirsrv handles them in
   this way, by logging to plog.  We make it int instead of void, because
   older versions of the PCC (Portable C Compiler) cannot handle pointers to
   void functions. */
/* variable name "line" commented out to shut up GCC -Wshadow. */
extern int (*internal_error_handler)(const char file[], int /* line */, const char mesg[]);


/* Some short macros that really hike the efficiency of this code. */
/* Like bcopy(a, b, 1), but much faster. */
#define bcopy1(a, b) do {                       \
    ((char *) (b))[0] = ((char *) (a))[0];      \
} while(0)                              

#define bcopy2(a, b) do {                        \
    bcopy1(a, b);                                 \
    /* Next line depends on unary cast having higher precedence than \
       addition.   (Guaranteed to be true.) */ \
    bcopy1((char *) (a) + 1, (char *) (b) + 1);   \
} while(0)

#define bcopy4(a, b) do {   \
    bcopy2((a), (b));       \
    bcopy2((char *) (a) + 2, (char *) (b) + 2);\
} while(0)

#define bzero1(a) do {                  \
    ((char *) (a))[0] = '\0';           \
} while(0)

#define bzero2(a) do {                  \
    bzero1(a);                          \
    bzero1((char *) (a) + 1);           \
} while (0)

#define bzero3(a) do {              \
    bzero2(a);                      \
    bzero1((char *) (a) + 2);       \
} while(0)

#define bzero4(a) do {              \
    bzero2(a);                      \
    bzero2((char *) (a) + 2);       \
} while(0)


#endif /* not ARDP_H_INCLUDED */
