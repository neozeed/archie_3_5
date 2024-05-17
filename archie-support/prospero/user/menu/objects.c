/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * The menu API and client were written by Kwynn Buess (buess@isi.edu)
 */

#include <usc-copyr.h>

/*
 * Future directions:   this code needs to change so that it's internally
 * consistent.  It should always use popen() or always fork, etc.

   Noted.  Partially done.  See my message.  -buess
 */


#include <stdio.h>             
#include <string.h>
#include <sys/types.h>          /* for lseek() */
#include <unistd.h>             /* for lseek() */
#include <pfs.h>
#include <stdlib.h>           /* For malloc and free */
#include "config.h"
#include "p_menu.h"
#include "menu.h"
char *getenv();

/* We include <pmachine.h> which, on SOLARIS, will make up for deficiencies in
   stdio.h (fdopen() undeclared, popen() undeclared. */
#include <pmachine.h>

static void save_file(int fd);
static void mail_file(int fd);

static void pipe_output_to_subprogram(int fd, char *subprogram, char *error_msg); 

static void run_subsystem_with_fd_as_stdin(int fd, char *subprogram, char *error_msg); 
static int retrieve_link(VLINK vl);

void
open_and_display(VLINK vl)
{
    int fd;
    int fd_dup;
    int get_option();
    int option;
    static char *pager;		/* pager to use */

    if (!pager) {
	pager = getenv("PAGER");
	if (!pager)
	    pager = "more";
    }

    fd = retrieve_link(vl);   /* retrieve link, print error messages if
                                   failure.  */
    if (fd < 0) return;
    run_subsystem_with_fd_as_stdin(fd,pager,"unable to display file");

    /* Finish work after display. */
    if(lseek(fd, (off_t) 0, SEEK_SET)) {
        fprintf(stderr, "lseek failed: this should never happen.\n");
        return;
    }

    for (;;) {
        printf("\n\nPress <RETURN> to continue, <m> to mail, <s> to save, or <p> to print:");
        option = get_option();
        if (option == -1)
            break;
        if (option == 's') {
            save_file(fd);
            break;
        } else if (option == 'm') {
            mail_file(fd);
            break;
        } else if (option == 'p') {
            run_subsystem_with_fd_as_stdin(fd,PRINT_PROGRAM,
					   "Unable to print file");
            break;
        } else 
            continue;
    }
    close(fd);
}



/* Pass it an open file descriptor.
   It might close it internally (due to the way fdopen() works), but caller
   should also run close() just to be sure. */

/* Pass it an open file descriptor.
   Caller takes responsibility for closing it.
*/
static
void 
save_file(int fd)
{
    char *name;
    FILE *org_fp;
    FILE *save_fp;
    int temp = 25;

    char *get_line();

    printf("\n\nEnter save file name: ");
    name = get_line();

    if (name == NULL)
	return;

    save_fp = fopen(name, "w");
    while (save_fp == NULL) {
	printf("Error opening :  Enter new name or <Enter> to cancel: ");
	stfree(name);
	name = get_line();
	save_fp = fopen(name, "w");
    }

    org_fp = fdopen(fd, "r");
    if (org_fp == NULL) {
	fprintf(stderr, "\nError retrieving file\n");
	return;
    }
    while (fgetc(org_fp) != EOF) {
        if (fputc(temp, save_fp) == EOF) {
            fprintf(stderr, "\nError while writing to output file\n");
            break;
        }
    }

    /* done; cleanup. */
    fclose(save_fp);
    fclose(org_fp);
}

/* Pass it an open file descriptor.
   It might close it internally (due to the way fdopen() works), but caller
   should also run close() just to be sure. */
static
void 
mail_file(int fd)
{
    char *address;

    char *get_line();
    static char *command = NULL;

    printf("\n\nMail document to:");
    fflush(stdout);	
    address = get_line();
    if (address == NULL)
	return;

    command = qsprintf_stcopyr(command, "%s %s", MAIL_SENDING_PROGRAM, address);

    run_subsystem_with_fd_as_stdin(fd,command,"Unable to mail");
}

void 
open_telnet(VLINK vl)
{
    TOKEN args;			/* Used to get access method args. */
    char *paren;		/* used during parsing. */
    char *endparen;		/* Used during parsing. */
    char *hostpart;		/* HOST breaks into host part & port part.  */
    char *portpart;		/* Either NULL or the start of a string whose
                                     numeric interpretation is the appropriate
                                     port.  */
    int ch_par;			/* Used for fork(). */
    int tmp;			/* Return from subfunctions */

    if (pget_am(vl, &args, P_AM_TELNET) != P_AM_TELNET) {
      cant_find_am:
	fprintf(stderr, "Unable to find an appropriate access method to use \
this portal -- sorry.\n");
	return;
    }
    /* ARGS are guaranteed to be a safe copy for us to work with. */
    /* Length guaranteed by pget_am to be at least 5 elements. */
    tmp = qsscanf(elt(args, 2), "%r%*[^(]%r(%r%*d%r)", &hostpart, &paren,
		  &portpart, &endparen);
    if (tmp < 1)
	goto cant_find_am;
    if (tmp >= 2)
	*paren = '\0';
    if (tmp < 4)
	portpart = NULL;
    else
	*endparen = '\0';

    /* HOSTPART and PORTPART are now both correctly set. */

    /* display prompt if present and not just the empty string. */
    if (elt(args, 5) && *elt(args, 5))
	puts(elt(args, 5));
    /* Now fork.  */
    ch_par = fork();

    if (ch_par == -1) {
	perror("menu: fork() failed");
	return;
    } else if (ch_par == 0) {
	/* CHILD */
	if (portpart)
	    execlp(TELNET_PROGRAM, TELNET_PROGRAM, hostpart, portpart, (char *) 0);
	else
	    execlp(TELNET_PROGRAM, TELNET_PROGRAM, hostpart, (char *) NULL);
	/* Only get here if the execlp() failed. */
	fprintf(stderr,
		"Couldn't run the telnet program \"%s\" -- sorry!\n",
		TELNET_PROGRAM);
	_exit(1);
    } else {
	/* Parent */
	wait((int *) NULL);
    }
}

void 
open_data(VLINK vl)
{
/*
    int query_save_data();
    int fd = retrieve_link(vl);
    int fd_dup;
    char *n = vl->name;
    char *command;
    char *tn;
    char *to_free;
    static int hassuffix(char *,char *);
    pid_t pid;
    char *dummy_argv[3];

    if (hassuffix(n,"Z")) {
        tn = tmpnam(NULL);
	
	to_free = (char *)malloc(sizeof(char) * (9+strlen(tn)));
	strcpy(to_free,"zcat > ");
	strcat(to_free,tn);
	pipe_output_to_subprogram(fd,to_free,"Unable to zcat");
        stfree(to_free);
	close(fd);
    }
    
    pid = fork();
    if (pid == -1) {
	fprintf(stderr, "%s: fork() failed.\n", "Unable to run gs");
	return;
    }
    if (pid == 0) { 
	dummy_argv[0] = "gs";
	dummy_argv[1] = tn;
	dummy_argv[2] = NULL;

	if (execvp(dummy_argv[0], dummy_argv) == -1) {
	    fprintf(stderr, "couldn't execute the program %s.\n",
                    "gs");
            _exit(1);
	}
	_exit(0);
    } else {
	wait((int *) NULL);  
    }

*/
/* Original function... */
    if (query_save_data()) {
        int fd = retrieve_link(vl);
        if (fd >= 0) save_file(fd);
    }
}

static int
hassuffix(char *name, char *suf)
{
    char *lastdot = strrchr(name, '.');
    if (lastdot) return strequal(lastdot + 1, suf); /* 1 if true */
    else return 0;              /* false */
}

#if 0
/* Returns 1 if a file descriptor refers to a file that has more than 95%
   printing characters.  Note that this code is broken, since it gobbles up the
   file you give it and since it is never used and has never been tested.  
   So we comment it out for now. --swa */
int 
m_file_has_more_than_95_percent_printing_chars(int fd) 
{
    FILE *fp = fdopen(fd,"r");
    int num_printing_chars = 0;
    int tot_cnt = 0;
    int retval;

    do { 
        retval = fgetc(fp);
	if (isprint(retval)) num_printing_chars++;
	tot_cnt++;
    } while (retval != -1 && tot_cnt < 1000);

    if (((float) num_printing_chars / tot_cnt) > 0.95) 
        return 1;
    else
        return 0;
}
#endif


TOKEN
get_token(VLINK vl, char *which_token)
{
    PATTRIB temp;
    PATTRIB highest = NULL;
    int compare_precedence(char, char);	/* defined in comp.c */
    TOKEN temp_tok;

    if (vl == NULL)
	return NULL;
    if ((temp = vl->lattrib) == NULL)
	return NULL;

    while (temp != NULL) {
	if (!strcmp(which_token, temp->aname))
	    if (highest == NULL)
		highest = temp;
	    else if (compare_precedence(highest->precedence, temp->precedence)
                     == -1)
		highest = temp;
	temp = temp->next;
    }	/* while temp != NULL */

    if (highest == NULL)
	return NULL;
    return highest->value.sequence;
}

static
void
pipe_output_to_subprogram(int fd, char *subprogram, char *error_msg) 
{
    FILE *org_fp;
    FILE *output_fp;
    int temp = 25;

    org_fp = fdopen(fd, "r");
    if (org_fp == NULL) {
	fprintf(stderr,error_msg);
	return;
    }
    output_fp = popen(subprogram, "w");
    if (output_fp == NULL) {
	fprintf(stderr,error_msg);
	fclose(org_fp);
	return;
    }
    while(fgetc(org_fp) != EOF) {
        int err = fputc(temp, output_fp);
	if (err == EOF) {
	    fprintf(stderr,error_msg);
            break;
	}
    }
    pclose(output_fp); /* This was corrected from fclose. */
}


static
void
run_subsystem_with_fd_as_stdin(int fd, char *subprogram, char *error_msg)
{
/*    char *dummy_argv[2];*/
    char *execvp_argv[30]; /* Over 30 parameters would be nuts! */
    int argc = 0;
    pid_t pid;
    int retval;
    char *progcopy = stcopy(subprogram);
    char *org_pc = progcopy;

    memset(execvp_argv,0,sizeof(char *) * 30);
    while (qsscanf(progcopy,"%&s",&(execvp_argv[argc]))==1) {
        progcopy += strlen(execvp_argv[argc]);
	while (isspace(*progcopy)) progcopy++;
	argc++;
    }
    stfree(org_pc);
    execvp_argv[argc] = NULL;

    pid = fork();
    if (pid == -1) {
	fprintf(stderr, "%s: fork() failed.\n", error_msg);
	return;
    }
    if (pid == 0) { 
        /* Child process */
      assert(P_IS_THIS_THREAD_MASTER());	/* SOLARIS: dup2 MT-Unsafe */
	if (dup2(fd, 0) == -1) {
	    fprintf(stderr, 
                    "%s: dup2(fd,0) failed.  This should *never* happen.\n",
                    error_msg);
            _exit(1);
	}


/*	dummy_argv[0] = subprogram;
	dummy_argv[1] = NULL;
*/

	if (execvp(execvp_argv[0], execvp_argv) == -1) {
	    fprintf(stderr, "%s: couldn't execute the program %s.\n",
                    error_msg, subprogram); 
            _exit(1);
	}
	_exit(0);
    } else {
	wait((int *) NULL);  /* wait for child */
    }
}


static
int
retrieve_link(VLINK vl)
{
    int fd;                     /* return value */
    printf("\nRetrieving %s...", m_item_description(vl));
    fflush(stdout);
    fd = m_open_file(vl);
    if (fd < 0) {
        puts("retrieval failed.");
    } else {
        puts("done.");
    }
    return fd;
}





