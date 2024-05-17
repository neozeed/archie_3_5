/*
**	A simple fast ls(1) replacement for Prospero. Only supports
**	a small subset of the multitude of ls options.
**
**	A quick whip-up for the Prospero FTPD.
**
**	Steve Cliffe
**	Department of Computer Science
**	University of Wollongong
**	Australia
**
**	steve@cs.uow.edu.au
**
**	January 1992
** 
**      Minor changes made to update it for prospero version 5; swa@isi.edu
**      Feburary, 1993
*/



#include <stdio.h>
#include <sys/param.h>
/* Needed for SCOUNIX which doesnt define this in MAXPATHLEN */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <search.h>

#include <pfs.h>
#include <pcompat.h>
#include <perrno.h>

#define TABSIZE		500		/* Max number of subdirs */

#define	DAYSPERNYEAR	(365)
#define	SECSPERDAY	(60*60*24)

int		pfs_debug = 0;
	
extern int	optind;	

int		opt_long = 0;		/* Listing options */
int		opt_revsort = 0;
int		opt_timesort = 0;
int		opt_recursive = 0;
int		opt_prospero = 0;
int		opt_union = 0;

char		*progname;

struct my_stat {			/* Info about a file */
	unsigned	st_size;
	unsigned	st_blocks;
	long		st_links;
	mode_t		st_mode;
/*Oops st_mtimes defined in sys/stat.h on SOLARIS */
	long		st_mtimes;
	char		st_owner[12];
	char		st_group[12];
	char		st_modes[12];
};

struct tabent {
	char		name[128];	/* Link name */
	char		host[128];	/* Link's host */
	char		hsoname[128];	/* Link's system level hsoname */
	long		magic_no;	/* Link's magic number */
};

struct tabent	table[TABSIZE];		/* Cycle detection table */
unsigned int	table_entries = 0;
unsigned int	table_width = sizeof(struct tabent);

/* Forward declerations */
void list_directory(char *path);
void display_link(VLINK l);
void display_long(VLINK l);
void display_prospero_long(VLINK l);
static void display_fil(FILTER fil);
void strmode(short mode, char *modestr);
void printtime(time_t ftime);


char *prog;


void
main(argc,argv)
   int		argc;
   char		*argv[];
{
	int		opt;

        prog = argv[0];

        p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
	progname = argv[0];
	
	while ((opt = getopt(argc, argv, "lgrtRPU")) != EOF) {
		switch (opt) {

		case 'l':
			opt_long = 1;
			break;
		case 'r':
			opt_revsort = 1;
			break;
		case 't':
			opt_timesort = 1;
			break;
		case 'R':
			opt_recursive++;
			break;

		case 'P':
			opt_prospero = 1;
			break;

		case 'U':
			opt_union = 1;
			break;
			
		default:
			fprintf(stderr,
			"Usage: %s [-lrtR] [file or directory name]\n", progname);
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	list_directory(argc ? *argv : "");
	
	exit(0);
}

static void display_fil(FILTER fil);

/*
** list_directory:
**
**	List out a directory. (i.e. do all the work)
*/

void
list_directory(char *path)
{
	VDIR_ST		dir_st;
	VDIR		dir= &dir_st;
	VLINK		l;
	int		flags = GVD_LREMEXP;
	int		error, been_here, old_table_size;
	char		newpath[MAXPATHLEN];
	struct tabent	newent;
	int		compare();

	vdir_init(dir);

	if (opt_union)
		flags = GVD_UNION;
	if (opt_long)
		flags |= GVD_ATTRIB;
		
	error = rd_vdir(path, 0, dir, flags);

	if(error && (error != DIRSRV_NOT_DIRECTORY)) {
		fprintf(stderr, "%s", progname);
		perrmesg(" failed: ", error, NULL);
		exit(1);
	}

	if(pwarn)
		pwarnmesg("WARNING: ", 0, NULL);


	/*
	**	Display this directory.
	*/

	l = dir->links;

	while(l) {
		display_link(l);
		l = l->next;
	}

	/*
	**	Handle union links.
	*/
	
	l = dir->ulinks;	

	if ((error != DIRSRV_NOT_DIRECTORY))
	    while(l) {
		if ((l->expanded == FALSE) || (l->expanded == FAILED)) 
			display_link(l);
		
		l = l->next;
	    }
	    
	/*
	**	Display subdirectories if requested (-R).
	*/
	
	if (opt_recursive) {
		l = dir->links;
		while (l) {

			/*
			**	Make sure we haven't been here before.
			*/


			strcpy(newent.name, l->name);
			strcpy(newent.host, l->host);
			strcpy(newent.hsoname, l->hsoname);
			newent.magic_no = l->f_magic_no;
	
			if (table_entries == TABSIZE) {
				fprintf(stderr, "%s: cycle detection table overflow\n", progname);
				exit(1);
				}

			old_table_size = table_entries;
			been_here = 0;

			/*
			**	Don't cross machine boundaries
			**	unless -R option is specified twice.
			*/
			
			if (strcmp(table[table_entries].host, l->host) &&
			    (opt_recursive != 2))
				goto next;

			/*
			**	Make sure we haven't been here before.
			*/
			
			lsearch((char *) &newent, (char *) &table[0], &table_entries,
				    table_width, compare);

			if (old_table_size == table_entries)	
				been_here = 1;

			if ((strcmp(l->target, "DIRECTORY") == 0) && (!been_here)) {
				if (path[0] == '\0')
					strcpy(newpath, l->name);
				else
					sprintf(newpath, "%s/%s", path, l->name);
				printf("\n%s:\n", newpath);
				list_directory(newpath);
			}
next:
			l = l->next;
		}
	}

	vllfree(dir->links);
	return;
}

/*
** display_link:
**
**	Display a link based on what options have been selected.
*/

void
display_link(VLINK l)
{   
	if (opt_long) {
		if (opt_prospero)
			display_prospero_long(l);
		else
			display_long(l);
		return;
	}
	
	if (opt_prospero)
		printf("%c%c %-20.20s %c%-15.15s %-38.38s\n",
	         ((l->linktype == 'L') ? ' ' : l->linktype),
		 (l->expanded ? 'F' : ' '),
		 l->name,
		 (l->filters ? '*' : ' '),
		 l->host,l->hsoname);
	else
		printf("%s\n", l->name);
}

/*
** display_long:
**
**	Print out a directory tabent in the UNIX ls -l format. Code to
**	fill in the stat(2) buffer based on lib/pcompat/stat.c
*/

void
display_long(VLINK l)
{
	PATTRIB		ap, apnext;
	struct my_stat	stat_buffer;
	struct my_stat	*sbp = &stat_buffer;
	char		mode_string[15];
	int		size;

	sbp->st_size = 0;
	sbp->st_mtimes = (time_t) 0;
	strcpy(sbp->st_owner, "-");
	strcpy(sbp->st_group, "-");
	sbp->st_mode = 0;
	sbp->st_modes[0] = '\0';

	if (strcmp(l->target, "DIRECTORY") == 0)
		sbp->st_mode |= (S_IFDIR | 0555);
	else
		sbp->st_mode |= 0444;
		
	sbp->st_links = 1;

	ap = NULL;

	if (l->lattrib == NULL)
		ap = pget_at(l, "#ALL"); /* typo fixed, swa, 5/15/94 */

	if ((ap == NULL) && (l->lattrib)) {
		ap = l->lattrib;
		l->lattrib = NULL;
	}

	while (ap) {
		if ((strcmp(ap->aname, "SIZE") == 0) &&
		    (ap->avtype == ATR_SEQUENCE) && ap->value.sequence) {
			sscanf(ap->value.sequence->token, "%d", &size);
			sbp->st_size = size;
			sbp->st_blocks = (size + 511) / 512;
		}
		if ((strcmp(ap->aname, "NATIVE-OWNER") == 0) &&
		    (ap->avtype == ATR_SEQUENCE) && ap->value.sequence)
			strcpy(sbp->st_owner, ap->value.sequence->token);
		if ((strcmp(ap->aname, "NATIVE-GROUP") == 0) &&
		    (ap->avtype == ATR_SEQUENCE) && ap->value.sequence)
			strcpy(sbp->st_group, ap->value.sequence->token);
		if ((strcmp(ap->aname, "UNIX-MODES") == 0) &&
		    (ap->avtype == ATR_SEQUENCE) && ap->value.sequence)
			strcpy(sbp->st_modes, ap->value.sequence->token);
		if ((strcmp(ap->aname, "LAST-MODIFIED") == 0) &&
		    (ap->avtype == ATR_SEQUENCE) && ap->value.sequence)
			sbp->st_mtimes = asntotime(ap->value.sequence->token);

		apnext = ap->next;
		atfree(ap);
		ap = apnext;
	}

	/*
	**	Now display the tabent.
	*/

	if (sbp->st_modes[0] == '\0')
		(void) strmode(sbp->st_mode, mode_string);
	else
		strcpy(mode_string, sbp->st_modes);

	printf("%s %3lu %-8s %-8s %8u ", mode_string, sbp->st_links,
	       sbp->st_owner, sbp->st_group, sbp->st_size);

	printtime(sbp->st_mtimes);

	printf("%s\n", l->name);
}

/* check if a string is a null pointer.  If it is, return an empty string.  */
#define chknl(s) ((s) ? (s) : "")

/*
** display_prospero_long:
**
**	Print out verbose information about a link. Taken from vls.c
*/

void
display_prospero_long(l)
   VLINK l;
{
        FILTER      fil;
	PATTRIB		ap;

        printf("\n      Name: %s\n",chknl(l->name));
        printf("   ObjType: %s\n",chknl(l->target));
        printf("  LinkType: %s\n",((l->linktype == 'U') ? "Union" : "Standard"));
        printf("  HostType: %s\n",chknl(l->hosttype));
        printf("      Host: %s\n",chknl(l->host));
        printf("  NameType: %s\n",chknl(l->hsonametype));
        printf("  Pathname: %s\n",chknl(l->hsoname));
	if(l->version)
		printf("   Version: %ld\n",l->version);
        if(l->f_magic_no)
		printf("     Magic: %ld\n",l->f_magic_no);

	if (opt_long) {		/* Show attributes and filters */

		ap = NULL;

		if (l->lattrib == NULL)
			ap = pget_at(l, "#ALL");

		if ((ap == NULL) && (l->lattrib)) {
			ap = l->lattrib;
			l->lattrib = NULL;
		}

		printf("Attributes:\n\n");
		for( ; ap; ap = ap->next) {
                        printf("%15s: ",ap->aname);
                        if(ap->avtype == ATR_SEQUENCE) {
                                TOKEN tk;
                                for (tk = ap->value.sequence; tk; tk = tk->next)
                                        if (*(tk->token))
                                                printf("%s ", tk->token);
                                        else
                                                fputs("'' ", stdout);
                                putchar('\n');
                        } else if(ap->avtype == ATR_LINK)
                                printf("%s %s %s %ld %ld\n",
                                       ap->value.link->name,ap->value.link->host,
                                       ap->value.link->hsoname,
                                       ap->value.link->version, 
                                       ap->value.link->f_magic_no);
                        else if (ap->avtype == ATR_FILTER)
                                display_fil(ap->value.filter);
                        else printf("<unknown-type %d>\n",ap->avtype);
                }
                printf("\n");
                atlfree(ap);

                /* Copied from vls.c */
                if(l->filters) {
                        printf("   Filters:\n");
                        for(fil = l->filters; fil; fil = fil->next)
                                display_fil(fil);
                }
                putchar('\n');
	}
}

/* Copied from vls.c */
static void
display_fil(FILTER fil)
{
        if (fil->name)
                printf("Predefined Filter: %s\n", fil->name);
        else
                printf("Link Filter: %s %s %s %ld %ld\n", chknl(fil->link->name),
                       chknl(fil->link->host), 
                       chknl(fil->link->hsoname), fil->link->version, fil->link->f_magic_no);
        switch(fil->type) {
        case FIL_DIRECTORY:
                printf(" DIRECTORY");
                break;
        case FIL_HIERARCHY:
                printf(" HIERARCHY");
                break;
        case FIL_OBJECT:
                printf(" OBJECT");
                break;
        case FIL_UPDATE:
                printf(" UPDATE");
                break;
        default:
                internal_error("unknown fil->type value.");
        }
        switch(fil->execution_location) {
        case FIL_SERVER:
                printf(" SERVER");
                break;
        case FIL_CLIENT:
                printf(" CLIENT");
                break;
        default:
                internal_error("unknown fil->execution_location");
        }
        switch(fil->pre_or_post) {
        case FIL_PRE:
                printf(" PRE");
                break;
        case FIL_POST:
                printf(" POST");
                break;
        default:
                internal_error("unknown fil->pre_or_post");
        }
        if (fil->args) {
                TOKEN tk;
                printf(" ARGS");
                for (tk = fil->args; tk; tk = tk->next)
                        printf(" %s", tk->token);
        }
        putchar('\n');
}


/*
**	compare - Compare 2 nodes to detect a cycle.
*/
int
compare(one, two)
   struct tabent	*one, *two;
{
	if (strcmp(one->name, two->name))
		return(1);
	if (strcmp(one->host, two->host))
		return(1);
	if (strcmp(one->name, two->name))
		return(1);
	if ((one->magic_no == 0) || (two->magic_no == 0) ||
	    (one->magic_no == two->magic_no))
		return(0);
	else
		return(1);
}


/*	----------------------------------------------------------	*/

/*
**	strmode - Pinched from BSD ls. 
*/

void
strmode(mode,modestr)
   short	mode;
   char		*modestr;
{

	strcpy(modestr,"----------");

	if(mode & S_IFDIR) modestr[0] = 'd';
	if((mode & S_IFLNK) == S_IFLNK) modestr[0] = 'l';
	if(mode & S_IREAD) modestr[1] = 'r';
	if(mode & S_IWRITE) modestr[2] = 'w';
	if(mode & S_IEXEC) modestr[3] = 'x';
	if(mode & S_ISUID) modestr[3] = 's';
	if(mode & (S_IREAD>>3)) modestr[4] = 'r';
	if(mode & (S_IWRITE>>3)) modestr[5] = 'w';
	if(mode & (S_IEXEC>>3)) modestr[6] = 'x';
	if(mode & S_ISGID) modestr[6] = 's';
	if(mode & (S_IREAD>>6)) modestr[7] = 'r';
	if(mode & (S_IWRITE>>6)) modestr[8] = 'w';
	if(mode & (S_IEXEC>>6)) modestr[9] = 'x';
}

/*
**	printtime - pinched from the BSD ls.
*/

void
printtime(time_t ftime)
{
	int i;
	char *longstring, *ctime();
	time_t time();

	if(ftime == 0) fputs("-            ",stdout);
	else {

	    DISABLE_PFS(longstring = ctime((long *)&ftime));
	    for (i = 4; i < 11; ++i)
		(void)putchar(longstring[i]);

#define	SIXMONTHS	((DAYSPERNYEAR / 2) * SECSPERDAY)
	    if (ftime + SIXMONTHS > time((time_t *)NULL))
		for (i = 11; i < 16; ++i)
		    (void)putchar(longstring[i]);
	    else {
		(void)putchar(' ');
		for (i = 20; i < 24; ++i)
		    (void)putchar(longstring[i]);
	    }
	    (void)putchar(' ');
	}
}

