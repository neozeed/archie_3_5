/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * "usc-copyr.h".
 *
 * Written  by bcn 2/15/93    definitions for archie API
 *
 * Added extra search types & `dbs' field to aquery structure.
 *   - wheelan Fri May 28 22:52:29 EDT 1993
 */

#include "usc-copyr.h"

/*
 * Known archie servers:   Server                Prospero Version and comments
 *                         archie.rutgers.edu    v5
 *                         archie.sura.net       v1   
 *                         archie.sura.net(1528) v5 - for burn in period only
 *                         archie.ans.net
 *                         archie.unl.edu
 *                         archie.au
 *                         archie.funet.fi
 *                         archie.sogang.ac.kr   v5
 *                         archie.doc.ic.ac.uk 
 *                         archie.ncu.edu.tw 
 *                         archie.kyoto-u.ac.jp  v5
 */
#define ARCHIE_HOST "archie.rutgers.edu"

/*
 * Default value for max hits.  Note that this is normally different
 * for different client implementations.  Doing so makes it easier to
 * collect statistics on the use of the various clients.
 */
#define	AQ_MAX_HITS	100
#define AQ_MAX_MATCH	100
#define AQ_MAX_HITSPM	100

/*
 * CLIENT_VERSION may be used to identify the version of the client if 
 * distributed separately from the Prospero distribution.  The version
 * command should then identify both the client version and the Prospero
 * version identifiers.  Note that this identifier is not sent across
 * the network.  You MUST also obtain a software identifier from 
 * info-prospero@isi.edu, and set PFS_SW_ID in pfs.h.
 *
 * #define CLIENT_VERSION	"xyz"
 */

struct aquery                   /* ** means opaque                */
{
  char	   *host;               /* Archie server w/ opt port      */
  char	   *string;             /* String to be matched           */
  char     *dbs;                /* Databases to search            */
  struct filter  *filters;      /* Path or domain restrictions    */    
  VLINK	   expand;              /* Links to be expanded           */
  int		   maxhits;             /* Max entries to return          */
  int		   maxmatch;            /* Max strings to match           */
  int		   maxhitpm;            /* Max hits per match             */
  int		   offset;              /**Where to start search (opaque) */
  char	   query_type;          /* Search type - see above        */
  int		   (*cmp_proc)();       /* Comp procedure for sorting     */
  int		   flags;               /* Options - see above            */
  VLINK	   results;             /* The results of the query       */
  char	   *motd;               /* Motd from server               */
  int		   qpos;                /* Service queue pos if available */
  int		   sys_time;            /* Expected time til svc complete */
  struct timeval retry_at;      /* Time for retry                 */
  int		   wait_fd;             /* FD on which waiting            */
  struct  vdir   dirst;         /**For directory query            */
  VLINK	   dirquery;            /**Args to directory query        */
  int		   status;              /* Status of query                */
};

#define AQFASTCOMP  /* Make assumptions about server to quicken DEFCMP */

/* Query types - search = return hits, match = return strings          */
#define AQ_EXACT        '='     /* Exact string search                 */
#define AQ_CI_SUBSTR    'S'     /* Case insensitive substring search   */
#define AQ_CS_SUBSTR    'C'     /* Case sensitive substring search     */
#define AQ_REGEX        'R'     /* Regular expression search           */
#define AQ_ECI_SUBSTR   's'     /* Exact or case insensitive sub srch  */
#define AQ_ECS_SUBSTR   'c'     /* Exact or case sensitive sub search  */
#define AQ_EREGEX       'r'     /* Exact or regular expression search  */
#define AQ_M_CI_SUBSTR  'Z'     /* Case insensitive substring match    */
#define AQ_M_CS_SUBSTR  'K'     /* Case sensitive substring match      */
#define AQ_M_REGEX      'X'     /* Regular expression match            */
#define AQ_M_ECI_SUBSTR 'z'     /* Exact or case insensitive sub match */
#define AQ_M_ECS_SUBSTR 'k'     /* Exact or case sensitive sub match   */
#define AQ_M_EREGEX     'x'     /* Exact or regular expression match   */
#define AQ_EXP_LINK     '\001'  /* Expand single directory link        */
#define AQ_HOSTINFO     '\002'  /* Obtain info on matching hosts       */
#define AQ_MOTD_ONLY    '\003'  /* Only check for motd                 */

/* more search tyes - wheelan */

#define AQ_SITELIST   '\004'
#define AQ_WHATIS     '\005'
#define AQ_DOMAIN     '\006'
#define AQ_RE_DOMAIN  '\007'
#define AQ_GENERIC    '\010'
#define	AQ_STATUS     '\011'  /* bajan */
#define AQ_DOMAINS    '\012'  /* bajan */


/* Flags                                                       */
#define AQ_NOSORT 0x01  /* Don't sort                          */
#define AQ_ASYNC  0x02  /* Run asynchronously                  */
#define AQ_MOTD   0x80  /* Request server's motd               */

/* Status - one of the following or Prospero error code  */
#define AQ_INACTIVE  0  /* Request is inactive           */
#define AQ_COMPLETE -1  /* Request is complete           */
#define AQ_ACTIVE   -2  /* Request is executing          */

/* Definitions for the comparison procedures                            */
#define AQ_DEFCMP     aq_defcmplink     /* Default for query type       */
#define AQ_INVDATECMP aq_invdatecmplink /* Inverted by date             */

#define aq_init(AQ)              \
  (AQ)->host = ARCHIE_HOST;      \
  (AQ)->dbs = NULL;              \
  (AQ)->string = NULL;           \
  (AQ)->filters = NULL;          \
  (AQ)->expand = (VLINK) 0;      \
  (AQ)->maxhits = AQ_MAX_HITS;   \
  (AQ)->maxmatch = AQ_MAX_MATCH; \
  (AQ)->maxhitpm = AQ_MAX_HITSPM;\
  (AQ)->offset = 0;              \
  (AQ)->query_type = AQ_EXACT;   \
  (AQ)->cmp_proc = AQ_DEFCMP;    \
  (AQ)->flags = 0;               \
  (AQ)->results = (VLINK) 0;     \
  (AQ)->motd = NULL;             \
  (AQ)->qpos = 0;                \
  (AQ)->sys_time = 0;            \
  (AQ)->wait_fd = -1;            \
  (AQ)->retry_at.tv_sec = 0;     \
  (AQ)->retry_at.tv_usec = 0;    \
  (AQ)->dirquery = (VLINK) 0;    \
  (AQ)->status = AQ_INACTIVE;

/* Free fields that may have been filled in in call to archie_query */
/* This must be called before a used aquery structure is freed, for */
/* example by returning from a procedure in which it was allocated  */
/* on the stack.  It must also be called before an aquery structure */
/* is reused.  If you hang onto the results, expand, or motd fields */
/* the field should be zeroed as soon as another reference is made  */
/* to it, and it then becomes your resonsibility to explicitly free */
/* the fields.                                                      */
#define aq_reset(AQ)                        \
  if((AQ)->filters) fllfree((AQ)->filters); \
  if((AQ)->expand) vllfree((AQ)->expand);   \
  if((AQ)->results) vllfree((AQ)->results); \
  if((AQ)->motd) stfree((AQ)->motd);        \
  if((AQ)->motd) stfree((AQ)->motd);        \
  aq_init(AQ);

/* Function prototypes                                                  */
int  archie_query(struct aquery*,int);/* Front-end to aq_query    */
int  aq_query(struct aquery*,int);    /* Execute an archie query        */
char *aq_lhost(VLINK,char*,int);      /* Extract host name from link    */
char *aq_lhsoname(VLINK);             /* Extract filename from link     */
int  aq_defcmplink(VLINK,VLINK);      /* Compare by host and filename   */
int  aq_invdatecmplink(VLINK,VLINK);  /* Compare links inverted by date */
int  aq_restrict(struct aquery*,char*,char*,char); /* Add filters       */
