/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>


#include <pmachine.h>           /* for SOLARIS */
#ifdef CRYPT_FUNCTION_PROTOTYPE_IN_CRYPT_H /* defined currently only
                                              when SOLARIS is active. */
#include <crypt.h>		/* Solaris: for the crypt() function. */
    /* Note that the crypt() function is not part of the 1989 ANSI C standard,
       nor in the 1990 POSIX 1 standard.
       Therefore, different vendors will treat it differently. */
#endif

#include <stdio.h>
#include <pfs.h>
#include <pserver.h>
#include <errno.h>

#ifdef PSRV_P_PASSWORD

#include <ppasswd.h>
#include <perrno.h>

static int write_ppw_to_file();
static char *get_salt(void);

extern int pfs_debug;


/* Get password file entry for principal. Returns pointer to p_passwd */
/* structure if found, else returns NULL. */
p_passwd *
get_ppw_entry(char *principal)
{
    FILE *passwd_fp;
    char line[255];
    char princ_name[255], encrypted_passwd[255];
    p_passwd *p_pwd = NULL;

    if (!(passwd_fp = locked_fopen(PSRV_PW_FILE, "r")))
	return NULL;

    while (fgets(line, sizeof(line), passwd_fp)) {
	sscanf(line, "%s %s\n", 
	       princ_name, encrypted_passwd);
	if (!strcmp(princ_name, principal)) {
	    p_pwd = (p_passwd *) stalloc(sizeof(p_passwd));
	    p_pwd->principal = stcopy(princ_name);
	    p_pwd->encrypted_passwd = stcopy(encrypted_passwd);
	}
    }

    (void) locked_fclose_A(passwd_fp,PSRV_PW_FILE, TRUE);
    return p_pwd;
}


/* Write password entry in structure p_pwd to password file, */
/* overwriting old entry if one exists for the principal */
static int write_ppw_to_file(p_passwd *p_pwd)
{
    FILE *passwd_fp;
    char in_line[255], out_line[255];
    char princ_name[255], encrypted_passwd[255];
    long pos;

    if (!(passwd_fp = locked_fopen(PSRV_PW_FILE, "r+"))) {
	/* Cannot open password file, so create */
	if (!(passwd_fp = locked_fopen(PSRV_PW_FILE, "w+"))) {
	    if (pfs_debug)
		fprintf(stderr, "Cannot create file %s\n",
			PSRV_PW_FILE);
	    RETURNPFAILURE;
	}
	if (chmod(PSRV_PW_FILE, 0600)) {
	    if (pfs_debug) 
		perror("Could not change permissions of passwd file");
	    locked_fclose_A(passwd_fp,PSRV_PW_FILE,FALSE);
	    RETURNPFAILURE;
	}
	fprintf(passwd_fp, "# Prospero Server Password File.\n");
	fprintf(passwd_fp, "%-32s %-32s\n\n",
		"# <principal>", "<encrypted password>");
	fseek(passwd_fp, 0L, 1); /* To allow input after output */
    }

    /* Check if entry exists for principal */
    while (pos = ftell(passwd_fp), 	/* Remember position of line */
	   fgets(in_line, sizeof(in_line), passwd_fp)) { 
	if (in_line[0] == '#')
	    continue;
	sscanf(in_line, "%s %s\n", 
	       princ_name, encrypted_passwd);
	if (!strcmp(princ_name, p_pwd->principal)) {
	    /* Entry found; set file pointer to overwrite */
	    fseek(passwd_fp, pos, 0);
	    break;
	}
    }
	    
    /* If entry not found, write new entry to end of file, else overwrite */
    fprintf(passwd_fp, "%-32s %-32s\n", 
	    p_pwd->principal, p_pwd->encrypted_passwd);

    (void) locked_fclose_A(passwd_fp,PSRV_PW_FILE,FALSE);
    return PSUCCESS;
}


/* Set password for principal in password file, overwriting old */
/* entry if one exists. */
int
set_passwd(char *principal, char *passwd)
{
    p_passwd p_pwd;
    char *salt = get_salt();

    p_pwd.principal = stcopy(principal);
    p_pwd.encrypted_passwd = (char *) crypt(passwd, salt);
    if (write_ppw_to_file(&p_pwd))
	RETURNPFAILURE;
    stfree(p_pwd.principal);
    stfree(salt);
    return PSUCCESS;
}


/* Returns TRUE if password specified for the principal matches */
/* password in entry for principal in password file, and FALSE */
/* otherwise */
int passwd_correct(char * principal, char *passwd)
{
    p_passwd *p_pwd;
    char *salt, *p;
    int correct;
    
    /* Get password file entry */
    if (!(p_pwd = get_ppw_entry(principal)))
	return FALSE;
    
    /* Salt stored as first two characters of encrypted password */
    salt = p_pwd->encrypted_passwd;

    /* Encrypt supplied password and compare to encrypted password */
    /* stored in password file */
    p = (char *) crypt(passwd, salt);
    correct = strequal(p, p_pwd->encrypted_passwd);

    stfree(p_pwd);
    return correct;
}


/* Constructs and returns a random two-character salt. */
/* Salt needed by crypt() is two characters from the set [a-zA-Z0-9./] */
static char *get_salt(void)
{
    time_t tim = time(NULL);      /* Use current time to achieve */
				  /* randomness between sessions */
    char *salt;

    salt = stalloc(2);
    
    salt[0] = 'a' + (tim % 26);	  /* Letter between 'a' and 'z' */
    salt[1] = '.' + (tim % 12);	  /* One of '.', '/', and '0'-'9' */

    return salt;
}

#define RETURN(rv)  do { retval = rv; goto cleanup ; }

/* Deletes principal's password entry from file */
int delete_ppw_entry(char *principal)
{
    FILE *passwd_fp = NULL;
      FILE *tmp_fp = NULL;
    int retval;
    char line[255];
    char princ_name[255], encrypted_passwd[255];
    char tmp_filename[255];
    int found = 0;

    assert(P_IS_THIS_THREAD_MASTER());
    /* XXX !! This is MT-UNSAFE, two writers will totally screw up the 
       pw file, needs mutexing*/

    if (!get_ppw_entry(principal))
	RETURNPFAILURE;

    if (!(passwd_fp = locked_fopen(PSRV_PW_FILE, "r")))
	RETURNPFAILURE;
    qsprintf(tmp_filename, sizeof(tmp_filename), "%s_tmp", PSRV_PW_FILE);
    if (!(tmp_fp = locked_fopen(tmp_filename, "w"))) {
	if (pfs_debug) { /* Old code was hosed, only closed if pfs_debug set*/
	    fprintf(stderr, "Could not create temporary file %s\n",
		    tmp_filename);
    }
	if (passwd_fp) locked_fclose_A(passwd_fp,PSRV_PW_FILE,TRUE);
	RETURNPFAILURE;
    }

    while (fgets(line, sizeof(line), passwd_fp)) {
	sscanf(line, "%s %s\n", 
	       princ_name, encrypted_passwd);
	if (strcmp(princ_name, principal)) 
	    fputs(line, tmp_fp);
    }

    (void) locked_fclose_A(tmp_fp,tmp_filename,FALSE);
    (void) locked_fclose_A(passwd_fp,PSRV_PW_FILE,TRUE);
    
    if (rename(tmp_filename, PSRV_PW_FILE)) {
	unlink(tmp_filename);
	if (pfs_debug)
	    perror("Could not overwrite password file");
	RETURNPFAILURE;
    }
	
    if (chmod(PSRV_PW_FILE, 0600)) {
	if (pfs_debug) 
	    perror("Could not change permissions of passwd file");
	RETURNPFAILURE;
    }

    return PSUCCESS;
}


/* Lists the principals in the Prospero password file */
int list_ppw_file(void)
{
    FILE *passwd_fp;
    char line[255];
    char princ_name[255], encrypted_passwd[255]; 
    int  num;
  
    if (!(passwd_fp = locked_fopen(PSRV_PW_FILE, "r")))
	RETURNPFAILURE;

    while (fgets(line, sizeof(line), passwd_fp)) {
	if (line[0] == '#')
	    continue;

	num = sscanf(line, "%s %s\n", 
		     princ_name, encrypted_passwd);
	if (num == 2)
	    printf("%s\n", princ_name);
    }

    (void) locked_fclose_A(passwd_fp,PSRV_PW_FILE,TRUE);

    return PSUCCESS;
}

#endif /* PSRV_P_PASSWORD */
