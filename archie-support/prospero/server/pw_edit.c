/*
 * Copyright (c) 1993             by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */

#include <usc-copyr.h>

#include <stdio.h>
#include <string.h>
#include <pfs.h>
#include <pserver.h>   /* For #define PSRV_P_PASSWORD */
#include <termios.h>
#include <posix_signal.h>
#include <errno.h>

#ifdef PSRV_P_PASSWORD
#include <ppasswd.h>

/* Possible actions */
#define SET_PASSWD	1
#define DELETE		2
#define LIST		3

char *progname = "pw_edit";
int  pfs_debug = 0;

static void abort_pwread(void);
static void abort_prog(void);
void perform_action(int action, char *principal);
int usage();
int read_password(char *prompt, char *pwstr, int pwlen);
int get_principal_name(char *principal);
int display_commands();


/* Program to locally edit server password file.
 * 	-s: 	set principal's password.
 *	-d:	delete principal's entry.
 * 	-l:	list principals in file.
 */
void
main(int argc, char *argv[])
{
    char principal[255];
    char response[255];
    char command[255];
    int  num, action = 0;
    extern int opterr, optind;
    extern char *optarg;	
    char ch;
    
    while ((ch = getopt(argc, argv, "D:s:d:l")) != EOF)
	switch(ch) {
        case 'D':
            pfs_debug = 1; /* Default debug level */
            sscanf(optarg,"%d",&pfs_debug);
            break;
	case 's':	/* Set password */
	    if (action)
		usage();
	    action = SET_PASSWD;
	    if (optarg[0] == '-') {
		printf("pw_edit: option requires an argument -- s\n");
		usage();
	    }
	    strncpy(principal, optarg, sizeof(principal));
	    break;
	case 'd':	/* Delete entry */
	    if (action)
		usage();
	    action = DELETE;
	    if (optarg[0] == '-') {
		printf("pw_edit: option requires an argument -- d\n");
		usage();
	    }
	    strncpy(principal, optarg, sizeof(principal));
	    break;
	case 'l':	/* List principals */
	    if (action)
		usage();
	    action = LIST;
	    break;
	case '?':
        default:
	    usage();
        } 

    if (optind != argc)
	usage();

    if (action) {
	perform_action(action, principal);
	exit(0);
    }

    while (1) {
	printf("%s: ", progname);
	fgets(response, sizeof(response), stdin);

	num = sscanf(response, "%s %s %s\n", command, principal, response);
	
	if (num < 1)
	    continue;

	if (num > 2) {
	    printf("Too many arguments!\n");
	    display_commands();
	    continue;
	}

	if (strequal(command, "s") || strequal(command, "set")) {
	    if (num < 2)
		if (get_principal_name(principal) == PFAILURE)
		    continue;
	    perform_action(SET_PASSWD, principal);
	    continue;
	}
	if (strequal(command, "d") || strequal(command, "del")) {
	    if (num < 2)
		if (get_principal_name(principal) == PFAILURE)
		    continue;
	    perform_action(DELETE, principal);
	    continue;
	}
	if (strequal(command, "l") || strequal(command, "list")) {
	    if (num > 1) {
		printf("Too many arguments!\n");
		continue;
	    }
	    perform_action(LIST, NULL);
	    continue;
	}
	if (strequal(command, "q") || strequal(command, "quit")) {
	    printf("Bye!\n");
	    break;
	}
	
	printf("Illegal command!\n");
	display_commands();
    }

    exit(0);
}


/* Display usage and exit. */
int
usage()
{
    fprintf(stderr,
	    "Usage: %s [-D[#]] {[-s(et) <princ>] | [-d(el) <princ>] | [-l(ist)]}\n",
	    progname);
    exit(1);
}


/* Display possible commands */
int
display_commands()
{
    printf("Please enter one of:\n");
    printf("\ts(et) [principal] - set principal's password.\n");
    printf("\td(el) [principal] - delete principal's entry.\n");
    printf("\tl(ist)            - list principals.\n");
    printf("\tq(uit)            - quit.\n");
    return 0;
}


/* Perform requested action */ 
void
perform_action(int action, char *principal)
{   
    char passwd[255], buf[255];
    
    switch (action) {
    case SET_PASSWD:
	qsprintf(buf, sizeof(buf), "Password for %s",
		 principal);
	read_password(buf, passwd, sizeof(passwd));
	if (set_passwd(principal, passwd) == PSUCCESS)
	    printf("Password set.\n");
	else
	    perror("Could not access password file");
	break;
    case DELETE:
	if (!get_ppw_entry(principal)) {
	    printf("Principal %s not found!\n", principal);
	    break;
	}
	
	printf("Really delete entry for principal '%s'? (yes/no): ",
	       principal);
	gets(buf);

	if (strcmp(buf, "yes")) {
	    printf("Password entry NOT deleted!\n");
	    break;
	}

	if (delete_ppw_entry(principal) == PSUCCESS)
	    printf("Password entry for principal '%s' deleted.\n",
		   principal);
	else {
	    qsprintf(buf, sizeof(buf),
		     "Could not delete password entry for principal '%s'",
		     principal);
	    perror(buf);
	}
	break;
    case LIST:
	if (list_ppw_file() == PFAILURE)
	    perror("Could not access Prospero password file");
	break;
    default: /* Should never happen */
	usage();
    }
}


/* Gets principal name */
int
get_principal_name(char *principal)
{
    int count = 0;

    do {
	printf("Principal name: ");
	gets(principal);
	count++;
    } while (principal[0] == '\0' && count < 3);
    
    if (principal[0] == '\0') {
	printf("Aborting!\n");
	return PFAILURE;
    }
    else
	return PSUCCESS;
}


static struct termios permmodes;


/* Read password for principal princ, and return in pwstr */
int
read_password(char *prompt, char *pwstr, int pwlen)
{
    struct termios	tempmodes;
    int			tmp;

    if (prompt)
        printf("%s: ", prompt);
    else
        printf("Password: ");
    fflush(stdout);

    if(tcgetattr(fileno(stdin), &permmodes)) return(errno);
    bcopy(&permmodes,&tempmodes,sizeof(struct termios));

    tempmodes.c_lflag &= ~(ECHO|ECHONL);
    
    signal(SIGQUIT,abort_pwread);
    signal(SIGINT,abort_pwread);

    if(tcsetattr(fileno(stdin), TCSANOW, &tempmodes)) return(errno);

    *pwstr = '\0';
    fgets(pwstr, pwlen, stdin);
    pwstr[strlen(pwstr)-1] = '\0';	/* Take off '\n' */
    printf("\n");
    fflush(stdout);

    if(tcsetattr(fileno(stdin), TCSANOW, &permmodes)) return(errno);
    signal(SIGQUIT,abort_prog);
    signal(SIGINT,abort_prog);
    return PSUCCESS;
}

static void abort_pwread(void)
{
    tcsetattr(fileno(stdin), TCSANOW, &permmodes);
    abort_prog();
}

static void abort_prog(void)
{
    printf("Aborted\n");
    exit(0);
}

#endif /* PSRV_P_PASSWORD */
