/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1991, 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#ifndef P_PASSWD_H_INCLUDED
#define P_PASSWD_H_INCLUDED

/* Structure used to contain entry for the prospero password file */
typedef struct _p_passwd {
    char *principal;
    char *encrypted_passwd;
} p_passwd;


/* Get password file entry for principal. Returns pointer to p_passwd */
/* structure if found, else returns NULL. */
p_passwd *get_ppw_entry(char *principal);

/* Set password for principal in password file, overwriting old */
/* entry if one exists. */
int set_passwd(char *principal, char *passwd);

/* Returns TRUE if password specified for the principal matches */
/* password in entry for principal in password file, and FALSE */
/* otherwise */
int passwd_correct(char *principal, char *passwd);

/* Deletes principal's password entry from file */
int delete_ppw_entry(char *principal);

/* Lists the principals in the Prospero password file */
int list_ppw_file(void);

#endif /* P_PASSWD_H_INCLUDED */
