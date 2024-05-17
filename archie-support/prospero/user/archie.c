/*
 * Copyright (c) 1991 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 *
 * Written by Clifford Neuman (bcn@isi.edu) with changes by
 *            Brendan Kehoe (brendan@cs.widener.edu) and
 *            George Ferguson (ferguson@cs.rochester.edu).
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <stdio.h>
#include <sys/time.h>
#include <time.h>	/*SCOUNIX for localtime*/

#include <ardp.h>
#include <pfs.h>
#include <perrno.h>
#include <archie.h>
#ifdef AIX
#include <sys/select.h>
#endif

char		*month_sname();
char		*index();

void		display_link();
int		expand_links();

int		pfs_debug = 0;

char	  	*progname;
char            *prog;

static char	lastpath[MAX_VPATH] = "\001";
static char	lasthost[MAX_VPATH] = "\001";
	

#define AQ_LF_ONEPERLINE 0x1
#define AQ_LF_HOSTINFO   0x2
#define AQ_LF_STRINGONLY 0x4

/*
 * Archie client using the Prospero protocol
 *
 *  Usage: archie [-[cehkrsxz][ltMT][NM #][H host][f? <farg>]] string
 *
 *        Send a query to Prospero server on host running 
 *        Archie, collect, process, and display results.
 *        A search returns all hits, a match only returns
 *        the matching strings, but not file references.
 *        The user will then be prompted to select the
 *        matching strings to expand.  If the match option
 *        is upper case, prompting is disabled and the 
 *        program displays the matches and exits.
 *        
 *
 *        OPTIONS:
 * 	     -c : case sensitive substring search
 *        -K -k : case sensitive substring match
 *           -e : exact string match (default)
 *           -r : regular expression search
 *        -X -x : regular expression match
 *           -s : case insensitive substring search
 *        -Z -z : case insensitive substring match
 *           -h : ask for information about matching hosts
 *           -E : expand directory and query responses from server
 *-fd -fc <arg> : specify domain or component filter and argument
 *           -l : list one match per line
 *           -t : sort inverted by date
 *       -M -MM : display the message of the day (MM immediate)
 *           -M : display the message of the day 
 *           -T : display status information for server
 *  -N# or -N # : specifies query niceness level (# optional 0-35765)
 *  -m# or -m # : specifies maximum number of hits to return 
 *      -H host : specifies server host
 *  -D# or -D # : Debug level (# optional)
 *
 *                If -e is specified in addition to either -r, -x, -s, -z,
 *                -c, or -k, then server will try an exact match before 
 *                falling back to a more expensive method.
 */
void
main(int argc,char *argv[])
{
    char	  *cur_arg;             /* For argument parsing         */
    struct aquery aqst;                 /* The query to be sent         */
    int		  ptmp;			/* Temp for arg parsing         */
    int		  tmp;                  /* For return values            */

    int		  versionflag = 0;      /* Display release identifier   */
    int		  listflag = 0;         /* Flags for listing links      */
    int		  expandflag = 0;	/* Expand the response          */
    char	  etype = AQ_EXACT;     /* Type if only -e is specified */
    int		  eflag = 0;	        /* Exact flag specified         */
    int		  Mcount = 0;           /* # times -M flag specified    */
    int		  Tflag = 0;		/* Display status info for svr  */
    VLINK	  l;                    /* Temporary link pointer       */

    progname = argv[0];
    prog = progname;

    ardp_abort_on_int();
    p_initialize("aPtb", 0, (struct p_initialize_st *) NULL);
    aq_init(&aqst);

    argc--;argv++;

    while (argc > 0 && **argv == '-') {
	cur_arg = argv[0]+1;

	/* If a - by itself, or --, then no more arguments */
	if(!*cur_arg || ((*cur_arg == '-') && (!*(cur_arg+1)))) {
	    argc--, argv++;
	    goto scandone;
	}
	    
	while (*cur_arg) {
	    switch (*cur_arg++) {
		
	    case 'D':  /* debug level */
		pfs_debug = 1; /* Default debug level */
		if(*cur_arg && index("0123456789",*cur_arg)) {
		    sscanf(cur_arg,"%d",&pfs_debug);
		    cur_arg += strspn(cur_arg,"0123456789");
		}
		else if(argc > 2) {
		    tmp = sscanf(argv[1],"%d",&pfs_debug);
		    if (tmp == 1) {argc--;argv++;}
		}
		break;

	    case 'E':  /* Expand flag */
		if(listflag&AQ_LF_ONEPERLINE) {
		    fprintf(stderr, "%s: -E option incompatible with -l option\n",progname);
		    exit(1);
		}
		expandflag = 2;
		break;

	    case 'H':  /* archie server host */
		aqst.host = argv[1];
		argc--;argv++;
		break;

	    case 'K':  /* substring (case sensitive) */
		expandflag = 0;
		listflag |= AQ_LF_STRINGONLY;
		aqst.query_type = AQ_M_CS_SUBSTR;
		etype = AQ_M_ECS_SUBSTR;
		break;

	    case 'M':  /* Request motd */
		Mcount++;
		aqst.flags |= AQ_MOTD;
		break;

	    case 'N':  /* priority (nice) */
		ardp_priority = ARDP_MAX_PRI; /* Use this if no # */
		if(*cur_arg && index("-0123456789",*cur_arg)) {
		    sscanf(cur_arg,"%d",&ardp_priority);
		    cur_arg += strspn(cur_arg,"-0123456789");
		}
		else if(argc > 2) {
		    tmp = sscanf(argv[1],"%d",&ardp_priority);
		    if (tmp == 1) {argc--;argv++;}
		}
		if(ardp_priority > ARDP_MAX_SPRI) 
		    ardp_priority = ARDP_MAX_PRI;
		if(ardp_priority < ARDP_MIN_PRI) 
		    ardp_priority = ARDP_MIN_PRI;
		break;

	    case 'X':  /* regular expression match */
		listflag |= AQ_LF_STRINGONLY;
		expandflag = 0;
		aqst.query_type = AQ_M_REGEX;
		etype = AQ_M_EREGEX;
		break;

	    case 'Z':  /* substring (case insensitive) */
		listflag |= AQ_LF_STRINGONLY;
		expandflag = 0;
		aqst.query_type = AQ_M_CI_SUBSTR;
		etype = AQ_M_ECI_SUBSTR;
		break;

	    case 'T':  /* Status information for server */
		Tflag = 1;
		break;

	    case 'c':  /* substring (case sensitive) */
		aqst.query_type = AQ_CS_SUBSTR;
		etype = AQ_ECS_SUBSTR;
		break;

	    case 'e':  /* exact match */
		/* If -e specified by itself, then we use the  */
		/* default value of etype which is must be '=' */
		eflag++;
		break;

	    case 'f':  /* Add a filter to the query */
		if(argc < 3) {
		    fprintf(stderr, "%s: -f (filter) requires <filter-arg>\n",progname);
		    exit(1);
		}
		switch(*cur_arg++) {
		case 'd':
		    aq_restrict(&aqst,"AR_DOMAIN",argv[1],',');
		    argv++; argc--;
		    break;
		case 'p':
		    aq_restrict(&aqst,"AR_PATHCOMP",argv[1],' ');
		    argv++; argc--;
		    break;
		default:
		    fprintf(stderr, "%s: -f (filter) must be immediately followed by 'd' or 'p'\n",progname);
		    exit(1);
		}
		break;

	    case 'h':  /* Get host information */
		expandflag = 0;
		listflag &= (~AQ_LF_STRINGONLY);
		listflag |= AQ_LF_HOSTINFO;
		aqst.query_type = AQ_HOSTINFO;
		break;

	    case 'k':  /* substring (case sensitive) */
		listflag |= AQ_LF_STRINGONLY;
		if(expandflag == 0) expandflag = 1;
		aqst.query_type = AQ_M_CS_SUBSTR;
		etype = AQ_M_ECS_SUBSTR;
		break;

	    case 'l':  /* list one match per line */
		if(expandflag == 2) {
		    fprintf(stderr, "%s: -l option incompatible with -E option\n", progname);
		    exit(1);
		}
		listflag |= AQ_LF_ONEPERLINE;
		break;

	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		cur_arg--;
	    case 'm':  /* max hits */
		ptmp = -1;  
		if(*cur_arg && index("0123456789",*cur_arg)) {
		    sscanf(cur_arg,"%d",&ptmp);
		    cur_arg += strspn(cur_arg,"0123456789");
		}
		else if(argc > 1) {
		    tmp = sscanf(argv[1],"%d",&ptmp);
		    if(tmp == 1) {argc--;argv++;}
		}
		if(ptmp < 1) {
		    fprintf(stderr, "%s: -m option requires a value for max hits (>= 1)\n",
			    progname);
		    exit(1);
		}
		aqst.maxhits = ptmp;
		aqst.maxmatch = ptmp;
		aqst.maxhitpm = ptmp;
		break;

	    case 'o':  /* offset */
		if(*cur_arg && index("0123456789",*cur_arg)) {
		    sscanf(cur_arg,"%d",&ptmp);
		    cur_arg += strspn(cur_arg,"0123456789");
		}
		else if(argc > 1) {
		    tmp = sscanf(argv[1],"%d",&ptmp);
		    if(tmp == 1) {argc--;argv++;}
		}
		if(ptmp < 0) {
		    fprintf(stderr, "%s: -o option requires a value for offset (>= 0)\n",
			    progname);
		    exit(1);
		}
		aqst.offset = ptmp;
		break;

	    case 'r':  /* regular expression search */
		aqst.query_type = AQ_REGEX;
		etype = AQ_EREGEX;
		break;

	    case 's':  /* substring (case insensitive) */
		aqst.query_type = AQ_CI_SUBSTR;
		etype = AQ_ECI_SUBSTR;
		break;

	    case 't':  /* sort inverted by date */
		aqst.cmp_proc = AQ_INVDATECMP;
		break;

	    case 'v':  /* display version */
		fprintf(stderr,"Prospero client - Version %s\n",
			PFS_RELEASE);
		versionflag++;
		break;
		
	    case 'x':  /* regular expression match */
		listflag |= AQ_LF_STRINGONLY;
		if(expandflag == 0) expandflag = 1;
		aqst.query_type = AQ_M_REGEX;
		etype = AQ_M_EREGEX;
		break;

	    case 'z':  /* substring (case insensitive) */
		listflag |= AQ_LF_STRINGONLY;
		if(expandflag == 0) expandflag = 1;
		aqst.query_type = AQ_M_CI_SUBSTR;
		etype = AQ_M_ECI_SUBSTR;
		break;

	    default:
	fprintf(stderr,"Usage: %s [-[cehkrsxz][ltMT][NM #][H host][f? <farg>]] string\n", progname);
		exit(1);
	    }
	}
	argc--, argv++;
    }

 scandone:

    /* If -e specified, then choose the exact version of -crs option */
    if(eflag) aqst.query_type = etype;
    
    /* If one per line don't expand */
    if(listflag&AQ_LF_ONEPERLINE) expandflag = 0;

    /* If only -v specified, return, but not an error */
    if((argc != 1) && versionflag) exit(0);
    
    if(!((argc == 1) || ((argc == 0) && (Mcount > 0)))) {
	fprintf(stderr,"Usage: %s [-[cehkrsxz][ltMT][NM #][H host][f? <farg>]] string\n", progname);
	fprintf(stderr,"           -c : case sensitive substring search\n");
	fprintf(stderr,"           -e : exact string match (default)\n");
	fprintf(stderr,"           -r : regular expression search\n");
	fprintf(stderr,"           -s : case insensitive substring search\n");
	fprintf(stderr,"        -K -k : case sensitive substring match\n");
	fprintf(stderr,"        -X -x : regular expression match\n");
	fprintf(stderr,"        -Z -z : case insensitive substring match\n");
	fprintf(stderr,"           -h : ask for information about matching hosts\n");
	fprintf(stderr,"-fd -fc <arg> : specify domain or component filter\n");
	fprintf(stderr,"           -l : list one match per line\n");
	fprintf(stderr,"           -t : sort inverted by date\n");
	fprintf(stderr,"           -E : expand directory and query responses from server\n");
	fprintf(stderr,"       -M -MM : display the message of the day (MM immediate)\n");
	fprintf(stderr,"           -T : display status information for server\n");
	fprintf(stderr,"         -N # : specifies query niceness level (optional 0-35765)\n");
	fprintf(stderr,"         -m # : specifies maximum number of hits to return\n");
	fprintf(stderr,"      -H host : specifies server host\n");
	exit(1);
    }

    if(!(aqst.host)) {
	fprintf(stderr, "%s: You must specify an archie server (use the -H <host> option)\n",
		progname);
	exit(1);
    }

    if(!(listflag & AQ_LF_ONEPERLINE)) {
	printf("Prospero command line client to archie database\n\n");
	printf("  Prospero version: %s\n", PFS_RELEASE);
#ifdef CLIENT_VERSION
	printf("    Client version: %s\n", CLIENT_VERSION);
#endif
	printf("            Server: %s\n", aqst.host);
    }

    /* If immediate MOTD, or MOTD only */
    if((Mcount > 1) || ((Mcount == 1) && (argc == 0))) {
	struct aquery mqst;     /* For the motd request */
	aq_init(&mqst);
	mqst.query_type = AQ_MOTD_ONLY;
	mqst.host = aqst.host;
	mqst.flags |= AQ_MOTD;
	tmp = aq_query(&mqst,mqst.flags);
	if(mqst.motd) {
	    printf("Message of the day:%s\n",mqst.motd);
	    stfree(mqst.motd);
	    Mcount = -1;               /* Remember that we displayed it */
	    aqst.flags &= (~AQ_MOTD);  /* Don't ask again               */
	}
	/* If MOTD only */
	if(argc == 0) {
	    if(!mqst.motd) printf("Message of the day: not available\n");
	    printf("\n");
	    exit(0);
	}
    }

    p_clear_errors();
    
    aqst.string = argv[0];

    if(Tflag == 0) tmp = aq_query(&aqst,aqst.flags);
    else {
	tmp = aq_query(&aqst,aqst.flags|AQ_ASYNC);
    
	while(tmp == AQ_ACTIVE) {
	    fd_set		readfds;
	    struct timeval	selwait;
	    static int		qpos = 0;
	    static int		sys_time = 0;

	    FD_ZERO(&readfds);
	    FD_SET(aqst.wait_fd, &readfds);

	    selwait.tv_sec = aqst.retry_at.tv_sec - time(NULL);
	    if(selwait.tv_sec < 0) selwait.tv_sec = 0;
	    selwait.tv_usec = 0;

	    if(qpos != aqst.qpos) {
		qpos = aqst.qpos;
		printf("    Queue position: %d\n",qpos);
		fflush(stdout);
	    }

	    if(sys_time != aqst.sys_time) {
		sys_time = aqst.sys_time;
		printf("   Est system time: %d\n", sys_time);
		fflush(stdout);
	    }

	    select(aqst.wait_fd+1,&readfds,(fd_set *)0,(fd_set *)0,&selwait);

	    tmp = aq_query(&aqst,AQ_ASYNC);
	}
    }

    if(tmp > 0) {
	fprintf(stderr,"%s",progname);
	perrmesg(" failed: ",0,NULL);
	exit(1);
    }

    if(pwarn) pwarnmesg("WARNING: ",0,NULL);
    
    /* If received motd and haven't yet displayed it */
    if(aqst.motd && (Mcount >= 0)) {
	if(aqst.flags & AQ_MOTD) printf("Message of the day:%s\n",
					    aqst.motd);
	else fprintf(stderr,"Unanticipated message of the day:\n%s\n",
		     aqst.motd);
    }

    if((listflag & AQ_LF_STRINGONLY) && !(listflag & AQ_LF_ONEPERLINE)) {
	printf("\n\nStrings matching query (%s):\n\n",aqst.string);
    }

    /* Display the results */
    for(l = aqst.results; l; l = l->next) display_link(l,listflag);
    
    if(!(listflag&AQ_LF_ONEPERLINE)) printf("\n");
    
    if(expandflag > 0) expand_links(aqst.results,listflag,expandflag,"QUERY");
    else vllfree(aqst.results);
    exit(0);
}

/*
 * display_link: Prints the value of a virtual link. If listflag does
 *               not have AQ_LF_ONEPERLINE set then the link is displayed
 *               in a format suitable for reading by humans.  If listflag
 *               has AQ_LF_ONEPERLINE set all information is printed on a
 *               single line in a form suitable for parsing by programs.
 *
 *        Flags: AQ_LF_ONEPERLINE - Output entries on a single line
 *               AQ_LF_HOSTINFO   - Output attributes for host info only
 *               AQ_LF_ONLYSTRING - Output only the matching string
 *
 *         NOTE: If AQ_LF_ONPERLINE is not set, this procedure uses static
 *               variables to detect when fields have changed since the last 
 *               call.  The display of field unchanged since the last call is
 *               suppressed.
 */
void
display_link(VLINK	l,         /* Link to be displayed               */
	     int	listflag)  /* See above                          */
{
    static struct tm *presenttime = NULL;
    time_t	now;

    char	linkpath[MAX_VPATH];
    char	archie_date[20];
    char	archie_hupdate[100];
    char	hostbuf[100];
    int		dirflag = 0;
    int		size = 0;
    char	*hipaddr = "";
    char	*modes = "";
    char	*gt_date = "";
    int		gt_year = 0;
    int		gt_mon = 0;
    int		gt_day = 0;
    int		gt_hour = 0;
    int		gt_min = 0;
    PATTRIB 	ap;

    /* First time called, set localtime */
    if(!presenttime) {
	(void) time(&now);
	assert(P_IS_THIS_THREAD_MASTER());
	presenttime = localtime(&now);
    }

    /* Initialize local buffers */
    *archie_date = '\0';
    *archie_hupdate = '\0';

    /* Remember if we're looking at a directory */
    if (sindex(l->target,"DIRECTORY")) dirflag = 1;
    else dirflag = 0;
    
    /* Extract the linkpath from the hsoname */
    strcpy(linkpath,aq_lhsoname(l));
    *(linkpath + (strlen(linkpath) - strlen(l->name) - 1)) = '\0';
    
    aq_lhost(l,hostbuf,sizeof(hostbuf));

    /* Is this a new host? */
    if(!(listflag&(AQ_LF_ONEPERLINE|AQ_LF_STRINGONLY))) {
	if (strcmp(hostbuf,lasthost) != 0) {
	    printf("\nHost %s\n\n",hostbuf);
	    strcpy(lasthost,hostbuf);
	    *lastpath = '\001';
	}
    }
    
    /* Is this a new linkpath (location)? */
    if (!(listflag&(AQ_LF_ONEPERLINE|AQ_LF_HOSTINFO|AQ_LF_STRINGONLY))) {
	if(strcmp(linkpath,lastpath) != 0) {
	    printf("    Location: %s\n",(*linkpath ? linkpath : "/"));
	    strcpy(lastpath,linkpath);
	}
    }

    /* Parse the attibutes of this link */
    for (ap = l->lattrib; ap; ap = ap->next) {
	if (strcmp(ap->aname,"SIZE") == 0) {
	    sscanf(ap->value.sequence->token,"%d",&size);
	} else if(strcmp(ap->aname,"UNIX-MODES") == 0) {
	    modes = ap->value.sequence->token;
	} else if(strcmp(ap->aname,"LAST-MODIFIED") == 0) {
	    gt_date = ap->value.sequence->token;
	    sscanf(gt_date,"%4d%2d%2d%2d%2d",&gt_year,
		   &gt_mon, &gt_day, &gt_hour, &gt_min);
	    if ((12 * (presenttime->tm_year + 1900 - gt_year) + 
		 presenttime->tm_mon - gt_mon) > 6) 
		sprintf(archie_date,"%s %2d  %4d",month_sname(gt_mon),
			gt_day, gt_year);
	    else
		sprintf(archie_date,"%s %2d %02d:%02d",month_sname(gt_mon),
			gt_day, gt_hour, gt_min);
	} else if(strcmp(ap->aname,"AR_H_LAST_MOD") == 0) {
	    gt_date = ap->value.sequence->token;
	    sscanf(gt_date,"%4d%2d%2d%2d%2d",&gt_year,
		   &gt_mon, &gt_day, &gt_hour, &gt_min);
	    sprintf(archie_hupdate,"%d %s %d %02d:%02d UTC",
		    gt_day, month_sname(gt_mon), gt_year, gt_hour, gt_min);
	} else if(strcmp(ap->aname,"AR_H_IP_ADDR") == 0) {
	    hipaddr = ap->value.sequence->token;
	}
    }
    
    /* Print this link's information */
    if (listflag&AQ_LF_ONEPERLINE) {
	if(listflag&AQ_LF_HOSTINFO) printf("%s %15s %s\n",gt_date, hipaddr,
					   hostbuf);
	else if(listflag&AQ_LF_STRINGONLY) printf("%s\n",l->name);
	else printf("%s %6d %s %s%s\n", gt_date, size,
		    hostbuf, aq_lhsoname(l), (dirflag ? "/" : ""));
    }
    else {
	if(listflag&AQ_LF_HOSTINFO) 
	    printf("   Last Updated: %s\n     IP Address: %s\n",
		   archie_hupdate, hipaddr);
	else if(listflag&AQ_LF_STRINGONLY) printf("    %s\n",l->name);
	else printf("      %9s %s %10d  %s  %s\n",
		    (dirflag ? "DIRECTORY" : "FILE"),
		    modes,size,archie_date,l->name);
    }
}

/* Note: This function frees l after expansion               */
/*       Do not disable this without thinking this through   */
/*       since it must unlink elements of the list before    */
/*       adding them to the aquery structure for expansion.  */
/*       Thus, if not freed, you may loose track of them     */
int
expand_links(VLINK	el,        /* Links to be expanded               */
	     int	listflag,  /* See above                          */
	     int	expflag,   /* Wether to continue expansion       */
	     char	*parent)   /* Name of link being expanded        */
{
    struct aquery	aqst;      /* To make new queries                */
    VLINK	  	clink, nlink, l;
    char		line[100];
    char		newparent[1000];
    char		hostbuf[100];
    int			npc;
    int			dircount = 0;
    int			tmp;

    aq_init(&aqst);

    /* Force display of host and path from display link (kludge) */
    lastpath[0] = '\001'; lasthost[0] = '\001';

    listflag &= (~(AQ_LF_STRINGONLY|AQ_LF_HOSTINFO));
    nlink = el;

    while(nlink) {
	clink = nlink;
	nlink = nlink->next;
	if(strcmp(clink->target,"DIRECTORY") == 0) {
	    if(dircount++ == 0) printf("\nExpanding %s\n\n", parent);
	    aq_lhost(clink,hostbuf,sizeof(hostbuf));
	    if(strncmp(clink->hsoname,"ARCHIE/MATCH",12)==0) hostbuf[0] = '\0';
	reenter:
	    if(*hostbuf) printf("  %s(%s): ", clink->name, hostbuf);
	    else printf("  %s: ", clink->name);
	    line[0] = '\0';
	    if((fgets(line,sizeof(line),stdin) == NULL) ||
	       (line[0] == 'q') || (line[0] == 'Q')) {
		printf("\n");
		exit(0);
	    }
	    if((line[0] == 'u') || (line[0] == 'y')) {
		vllfree(clink);
		return(dircount);
	    }
	    if((line[0] == '\n') || (line[0] == 'y') || (line[0] == 'Y') ||
	       (line[0] == 'e') || (line[0] == 'E')) {
		strncpy(newparent,parent,sizeof(newparent));
		npc = sizeof(newparent) - strlen(newparent) - 1;
		if(npc-- > 1) strcat(newparent," ");
		strncat(newparent,clink->name,npc);
		newparent[sizeof(newparent)-1] = '\0';

		aqst.query_type = AQ_EXP_LINK;
		aqst.expand = clink;
		clink->next = NULL;
		clink->previous = NULL;
		tmp = aq_query(&aqst,aqst.flags);
		if(tmp > 0) {
		    fprintf(stderr,"%s",progname);
		    perrmesg(" failed: ",0,NULL);
		}
		if(pwarn) pwarnmesg("WARNING: ",0,NULL);
    
		/* Display the results */
		for(l = aqst.results; l; l = l->next) display_link(l,listflag);

		if(expflag > 1) {
		    if(expand_links(aqst.results,listflag,expflag,newparent))
		      printf("\nReturned from expansion of %s %s\n\n", 
			     parent,clink->name);
		    else printf("\n");
		    aqst.results = NULL;
		}
		else printf("\n");
		aqst.expand = NULL;
		aq_reset(&aqst);
	    }
	    else if((line[0] != 'n') && (line[0] != 'N') &&
		    (line[0] != 's') && (line[0] != 'S')) {
		printf("   <CR> - expand, s - skip, u - up, q - quit\n");
		goto reenter;
	    }
	}
        clink->next = NULL;
        clink->previous = NULL;
	vlfree(clink);
    }
    return(dircount);
}
