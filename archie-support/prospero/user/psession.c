/*
 * Copyright (c) 1993             by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * psession was written by Prasad Upasani
 */

/*!! This looks like it leaks fp's on failure */

#include <usc-license.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <posix_signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pfs.h>
#include <pprot.h>
#include <termios.h>
#include <perrno.h>

static int 	ppw();
static int	account();
static void  	abort_pwread();
static void 	abort_prog();
static int  	read_password();
static int 	write_ppw_to_file();
static int	delete_ppw_entry();
static VLINK 	p_get_vsdesc();
static TOKEN 	get_vsdesc_principals();
static int 	usage();
static char 	*canonicalize();
static char 	*uppercase();
static int 	read_and_set_password();
static int 	delete_passwd_frm_entries();
static int 	set_princ_with_no_passwd();
static char     *addr_to_name();
static char     *get_host_from_user();
static int      write_acc_to_file();

char 		*get_psession_filename();

char *prog;
int  pfs_debug = 0;


void
main(int argc, char *argv[])
{    
    prog = argv[0];

    p_initialize("ufP", 0, (struct p_initialize_st *) NULL);
    if (argc < 2)
	usage();
    if (strequal(argv[0], "ppw")) {
	argc --, argv++;
	ppw(argc, argv, 0);
    }
    else if (strequal(argv[1], "-p_password")) {
	argc -=2, argv+=2;
	ppw(argc, argv, 0);
    }
    else if (argc > 2 && strequal(argv[1], "-authenticate")) {
	argc -=2, argv+=2;	
	if (strequal(argv[0], "P_PASSWORD"))
	    ppw(--argc, ++argv, 0);
	else if (strequal(argv[0], "KERBEROS"))
	    add_krb_entry(--argc, ++argv);
    }
    else if (strequal(argv[1], "-account")) {
	argc-=2, argv+=2;
	account(argc, argv);
    }
    else if (strequal(argv[1], "-delete")) {
	argc-=2, argv+=2;
	if (strequal(argv[0], "-authenticate")) {
	    argc--, argv++;
	    if (strequal(argv[0], "P_PASSWORD"))
		ppw(--argc, ++argv, 1);
	    else if (strequal(argv[0], "KERBEROS"))
		del_krb_entry(--argc, ++argv);
	    else if (strequal(argv[0], "UNAUTHENTICATED"))
		del_unauth_entry(--argc, ++argv);
	}
	else if (strequal(argv[0], "-account"))
	    del_acc_entry(--argc, ++argv);
    }
    else if (strequal(argv[1], "-zero")) {
	if (delete_passwd_frm_entries(NULL, NULL))
	    fprintf(stderr, 
		    "Could not delete password from entry!\n");
    }
    else
	usage();

    exit(0);			/* exit successfully */
}


static int ppw(int argc, char *argv[], int dflag)
{
#ifndef P_P_PASSWORD
    fputs("Prospero Passwords are not in use at this site.\n", stderr);
    exit(1);
#else
    RREQ	req;
    char	*dirhst = NULL;
    int		sflag = 0, mflag = 0, zflag = 0;
    TOKEN 	princp, princ_list = NULL;
    char 	old_password[255], new_password[255];
    PAUTH 	authinfo;    /* Structure containing authentication info */
    PAUTH 	authp;       /* To iterate through authinfo list */
    char 	prompt[255];

    while (argc > 0 && **argv == '-') {

        switch (*(argv[0]+1)) {
        case 'D':
            pfs_debug = 1; /* Default debug level */
            sscanf(argv[0],"-D%d",&pfs_debug);
            break;

        case 'N':  /* Priority (nice) */
            ardp_priority = ARDP_MAX_PRI; /* Use this if no # */
            sscanf(argv[0],"-N%d",&ardp_priority);
            if(ardp_priority > ARDP_MAX_SPRI) 
                ardp_priority = ARDP_MAX_PRI;
            if(ardp_priority < ARDP_MIN_PRI) 
                ardp_priority = ARDP_MIN_PRI;
            break;

	case 's':	
	    if (dflag || zflag) {
		fprintf(stderr, "Only one of -s, -z, and -d allowed!\n");
		usage();
	    }
	    sflag = 1;	/* Set principal's password in remote server's */
	    break;      /* password file */

	case 'm':
	    if (dflag || zflag) {
		fprintf(stderr, "-m not allowed with -d or -z!\n");
		usage();
	    }
	    mflag = 1;	/* Maintainer, old passwd not required */
	    break;

	case 'd':
	    if (sflag || zflag) {
		fprintf(stderr, "Only one of -s, -z, and -d allowed!\n");
		usage();
	    }
	    dflag = 1;  /* Delete principal's password entry */
	    break;

	case 'z':
	    if (sflag || dflag) {
		fprintf(stderr, "Only one of -s, -z and -d allowed!\n");
		usage();
	    }
	    zflag = 1;  /* Delete just password from entry */
	    break;

	case 'p':	/* principal specified */
	    argc--; argv++;
	    princ_list = tkappend(argv[0], princ_list);
	    break;

        default:
	    usage();
        }
        argc--; argv++;
    }

    if(argc > 1)
	usage();

    if(argc > 0)
        dirhst = stcopyr(argv[0],dirhst);

    if (sflag) {
	if (!princ_list) {
	    /* Get default principals from instances of PRINCIPAL */
	    /* attribute of VS-DESCRIPTION */
	    if (!(princ_list = get_vsdesc_principals())) {
		fprintf(stderr, "No principals found!\n");
		usage();
	    }
	}
	if (!dirhst) {
	    if (!(dirhst = get_host_from_user(princ_list))) {
		dirhst = stalloc(255);
		gethostname(dirhst, p__bstsize(dirhst));
	    }
	}
	for (princp = princ_list; princp; princp = princp->next) {
	    if (mflag)
		req = p__start_req(dirhst);
	    else {
		if (!(req = ardp_rqalloc()))
		    out_of_memory();
		p__add_req(req, "VERSION %d %s\n", 
			   VFPROT_VNO, PFS_SW_ID);
		qsprintf(prompt, sizeof(prompt), 
			 "Old password for %s on host %s",
			 princp->token, dirhst);
		read_password(prompt, old_password, sizeof(old_password));
		p__add_req(req, "AUTHENTICATE OPTIONAL P_PASSWORD %'s %'s\n",
			   old_password, princp->token);
	    }
	    qsprintf(prompt, sizeof(prompt), 
		     "New password for %s on host %s", 
		     princp->token, dirhst);
	    read_password(prompt, new_password, sizeof(new_password));
	    if (new_password[0] == '\0') {
		printf("Password not set!\n");
		ardp_rqfree(req);
		continue;
	    }
	    p__add_req(req, "PARAMETER SET PASSWORD %'s %'s\n",
		       princp->token, new_password);

	    if (ardp_send(req,dirhst,0,ARDP_WAIT_TILL_TO)) {
		perrmesg("psession failed: ", 0, NULL);
		break;
	    }
	    if(pwarn) pwarnmesg("WARNING: ",0,NULL);
	
	    /* Print response */
	    while(req->inpkt) {
		printf("%s",req->inpkt->text);
		req->inpkt = req->inpkt->next;
	    }
	    
	    ardp_rqfree(req);
	}
    }
    else if (zflag) {
	if (delete_passwd_frm_entries(dirhst, princ_list))
	    fprintf(stderr, 
		    "Could not delete password from entry!\n");
    }
    else if (dflag) {
	if (!princ_list) {
	    fprintf(stderr, "ERROR: No principals specified!\n");
	    usage();
	}
	for (princp = princ_list; princp; princp = princp->next)
	    if (delete_ppw_entry(dirhst, princp->token)) {
		fprintf(stderr, "Could not delete entry!\n");
		break;
	    }
    }
    else {
	if (!dirhst)
	    dirhst = stcopy("*");
	if (!princ_list) {
	    /* Prompt for passwords for all instances of PRINCIPAL */
	    /* attribute of VS-DESCRIPTION and for all entries without */
	    /* passwords in the password file */
	    princ_list = get_vsdesc_principals();
	    if (read_and_set_password(dirhst, princ_list))
		fprintf(stderr, "Could not set password!\n");
	    else
		set_princ_with_no_passwd(princ_list);
	}
	else 
	    if (read_and_set_password(dirhst, princ_list))
		fprintf(stderr, "Could not set password!\n");
    }

    tklfree(princ_list);
    if (dirhst)
	stfree(dirhst);
    return 0;
#endif
}


/* write entry into user's password file using information supplied 
 * if entry already exists for specified host and principal, the old 
 * password is overwritten woth the new one. 
 */
static int write_ppw_to_file(char *hostname, char *principal, char *password)
{
    char in_line[255], out_line[255];
    static char *server = NULL, *princ_name = NULL, *passwd = NULL;
    char *hname = stcopy(hostname);
    long pos;
    int  num;
    char *psession_filename = get_psession_filename();
    FILE *psession_fp;
    char *ppw_encode();

    /* canonicalize host if not wildcarded */
    if (!(hname = canonicalize(hname)))
	RETURNPFAILURE;

    if (!(psession_fp = fopen(psession_filename, "r+"))) {
	/* cannot open password file, so create */
	if (!(psession_fp = fopen(psession_filename, "w+"))) {
	    if (pfs_debug)
		fprintf(stderr, "cannot create file %s\n",
			psession_filename);
	    RETURNPFAILURE;
	}
	if (chmod(psession_filename, 0600)) {
	    if (pfs_debug) 
		perror("could not change permissions of passwd file");
	    RETURNPFAILURE;
	}
	fprintf(psession_fp, "# Prospero client session file.\n\n");
	fprintf(psession_fp, "AUTHENTICATE * UNAUTHENTICATED ''\n");
	fseek(psession_fp, 0L, 1); /* to allow input after output */
    }

    /* check if entry exists for principal */
    while (pos = ftell(psession_fp),      /* remember position of line */
           fgets(in_line, sizeof(in_line), psession_fp)) { 
	num = qsscanf(in_line, "AUTHENTICATE %'&s P_PASSWORD %'&s %'&s\n", 
		      &server, &princ_name, &passwd);
	if (num < 3)
	    continue;
        if (strequal(hname, server) && 
	    strequal(principal, princ_name)) { 
            /* entry found; set file pointer to overwrite */
            fseek(psession_fp, pos, 0);
            break;
        }
    }

    qsprintf(out_line, sizeof(out_line), 
	     "AUTHENTICATE %'s P_PASSWORD %'s %'s\n", 
	     hname, principal, ppw_encode(password));
    
    if (feof(psession_fp) || strlen(in_line) == strlen(out_line))
	fputs(out_line, psession_fp);
    else {
	/* cannot insert space; have to create new file	*/
	FILE *tmp_fp;
	int newpos, uid = getuid();
	char tmp_filename[255];
	char line[255];

	qsprintf(tmp_filename, sizeof(tmp_filename), "%s_tmp", 
		 psession_filename);
	if (!(tmp_fp = fopen(tmp_filename, "w"))) {
	    if (pfs_debug)
		fprintf(stderr, "cannot create temporary file %s\n",
			tmp_filename);
	    RETURNPFAILURE;
	}

	rewind(psession_fp);

	while (newpos = ftell(psession_fp), 
	       fgets(line, sizeof(line), psession_fp) 
	       && (newpos < pos))
	    fputs(line, tmp_fp);
		
	fputs(out_line, tmp_fp);
	
	while(fgets(line, sizeof(line), psession_fp))
	    fputs(line, tmp_fp);
	
	(void) fclose(tmp_fp);
	(void) fclose(psession_fp);
		
	if (rename(tmp_filename, psession_filename)) {
	    unlink(tmp_filename);
	    if (pfs_debug)
		perror("could not overwrite password file");
	    RETURNPFAILURE;
	}

	if (chmod(psession_filename, 0600)) {
	    if (pfs_debug) 
		perror("could not change permissions of passwd file");
	    RETURNPFAILURE;
	}

	stfree(hname);
	return PSUCCESS;
    }

    stfree(hname);

    (void) fclose(psession_fp);
    return PSUCCESS;
}


/* deletes entry for hostname and principal specified from user's */
/* password file. */
/* if hostname is null, all entries for the principal are deleted. */
static int delete_ppw_entry(char *hostname, char *principal)
{
    char line[255];
    static char *server = NULL, *princ_name = NULL, *passwd = NULL;
    char *hname = stcopy(hostname);
    char tmp_filename[255];
    int num;
    char *psession_filename = get_psession_filename();
    FILE *psession_fp, *tmp_fp;

    if (!principal)
	RETURNPFAILURE;

    /* canonicalize host if not wildcarded */
    if (hname)
	hname = canonicalize(hname);
    
    if (!(psession_fp = fopen(psession_filename, "r"))) {
	fprintf(stderr, "could not open password file: %s\n",
		psession_filename);
    	RETURNPFAILURE;
    }

    qsprintf(tmp_filename, sizeof(tmp_filename), "%s_tmp", psession_filename);
    if (!(tmp_fp = fopen(tmp_filename, "w"))) {
        if (pfs_debug)
            perror("could not create temporary file");
	RETURNPFAILURE;
    }

    while (fgets(line, sizeof(line), psession_fp)) { 
        num = qsscanf(line, "AUTHENTICATE %'&s P_PASSWORD %'&s %'&s\n", 
		      &server, &princ_name, &passwd);
	if (num < 3) {
	    fputs(line, tmp_fp);
	    continue;
	}
	if (hname && strcmp(hname, server) || 
	    strcmp(princ_name, principal))
	    fputs(line, tmp_fp);
    }

    (void) fclose(tmp_fp);
    (void) fclose(psession_fp);

    if (rename(tmp_filename, psession_filename)) {
	unlink(tmp_filename);
	if (pfs_debug)
	    perror("could not overwrite password file");
    	RETURNPFAILURE;
    }
    
    if (chmod(psession_filename, 0600)) {
	if (pfs_debug) 
	    perror("could not change permissions of passwd file");
	RETURNPFAILURE;
    }

    stfree(hname);
    return PSUCCESS;
}


/* read password and writes entry to file for principals in princ_list */
static int read_and_set_password(char *dirhst, TOKEN princ_list)
{
    TOKEN princp;
    char  prompt[255], passwd[255];

    for (princp = princ_list; princp; princp = princp->next) {	
	if(strequal(dirhst,"*")) 
	    qsprintf(prompt, sizeof(prompt), "Password for %s", princp->token);
	else qsprintf(prompt, sizeof(prompt), "Password for %s on host %s",
		 princp->token, dirhst);
	read_password(prompt, passwd, sizeof(passwd));
	if (passwd[0] == '\0') {
	    printf("Password not set!\n");
	    continue;
	}
	if (write_ppw_to_file(dirhst, princp->token, passwd))
	    RETURNPFAILURE;
    }

    return PSUCCESS;
}	

/* adds to princ_list, principals from the password file that do not */
/* have any passwords, and that are not already in princ_list. */
static int set_princ_with_no_passwd()
{
    char line[255], buf[255], prompt[255];
    static char *server = NULL, *princ_name = NULL, *passwd = NULL;
    char *psession_filename = get_psession_filename();
    char tmp_filename[255];
    FILE *psession_fp, *tmp_fp;
    int  num;
    unsigned long hostaddr;
    char *hostname = NULL;
    struct hostent *host;
    char *ppw_encode();

    if (!(psession_fp = fopen(psession_filename, "r"))) {
	if (pfs_debug)
	    fprintf(stderr, "could not open password file: %s\n",
		    psession_filename);
    	RETURNPFAILURE;
    }   

    qsprintf(tmp_filename, sizeof(tmp_filename), "%s_tmp", psession_filename);
    if (!(tmp_fp = fopen(tmp_filename, "w"))) {
        if (pfs_debug)
            perror("could not create temporary file");
	RETURNPFAILURE;
    }

    while (fgets(line, sizeof(line), psession_fp)) { 
        num = qsscanf(line, "AUTHENTICATE %'&s P_PASSWORD %'&s %'&s\n", 
		      &server, &princ_name, &passwd);
	if (num < 3) {
	    fputs(line, tmp_fp);
	    continue;
	}

	if (num == 2) { 	/* no password */
	    if (isdigit(server[0])) {
		hostname = addr_to_name(server);
	    }
	    else
		hostname = stcopyr(server,hostname);
	    if(strequal(hostname,"*")) 
		qsprintf(prompt, sizeof(prompt), 
			 "Password for %s", princ_name);
	    else 
		qsprintf(prompt, sizeof(prompt), 
			  "Password for %s on host %s",
			  princ_name, hostname);

	    stfree(hostname);
	    read_password(prompt, buf, sizeof(buf));
	    qfprintf(tmp_fp, "AUTHENTICATE %'s P_PASSWORD %'s %'s\n", 
		     server, princ_name, ppw_encode(buf));
	}
	else
	    fputs(line, tmp_fp);
    }

    (void) fclose(psession_fp);
    (void) fclose(tmp_fp);

    if (rename(tmp_filename, psession_filename)) {
	unlink(tmp_filename);
	if (pfs_debug)
	    perror("could not overwrite password file");
    	RETURNPFAILURE;
    }
    
    if (chmod(psession_filename, 0600)) {
	if (pfs_debug) 
	    perror("could not change permissions of passwd file");
	RETURNPFAILURE;
    }

    return PSUCCESS;
}


/* deletes just the password from the entries specified. */
/* if princ_list is null, then passwords are removed for all principal */
/* entries with the specified host. if dirhst is null, then entries */
/* are removed for all principals. */
static int delete_passwd_frm_entries(char *dirhst, TOKEN princ_list)
{
    char line[255], buf[255], prompt[255];
    static char *server = NULL, *princ_name = NULL, *passwd = NULL;
    char *psession_filename = get_psession_filename();
    char tmp_filename[255];
    int num;
    FILE *psession_fp, *tmp_fp;
    char *hname = NULL;

    if (dirhst) 
	if (!(hname = canonicalize(dirhst)))
	    RETURNPFAILURE;

    if (!(psession_fp = fopen(psession_filename, "r"))) {
	if (pfs_debug)
	    fprintf(stderr, "could not open password file: %s\n",
		    psession_filename);
    	RETURNPFAILURE;
    }   

    qsprintf(tmp_filename, sizeof(tmp_filename), "%s_tmp", psession_filename);
    if (!(tmp_fp = fopen(tmp_filename, "w"))) {
        if (pfs_debug)
            perror("could not create temporary file");
	RETURNPFAILURE;
    }

    while (fgets(line, sizeof(line), psession_fp)) { 
        num = qsscanf(line, "AUTHENTICATE %'&s P_PASSWORD %'&s %'&s\n", 
		      &server, &princ_name, &passwd);
	if (num < 3) {
	    fputs(line, tmp_fp);
	    continue;
	}

	if ((!hname || strequal(hname, server)) &&
	    (!princ_list || member(princ_name, princ_list))) {
	    passwd[0] = '\0';
	    qfprintf(tmp_fp, "AUTHENTICATE %'s P_PASSWORD %'s %'s\n", 
		     server, princ_name, passwd);
	}
	else
	    fputs(line, tmp_fp);
    }

    (void) fclose(psession_fp);
    (void) fclose(tmp_fp);

    if (rename(tmp_filename, psession_filename)) {
	unlink(tmp_filename);
	if (pfs_debug)
	    perror("could not overwrite password file");
    	RETURNPFAILURE;
    }
    
    if (chmod(psession_filename, 0600)) {
	if (pfs_debug) 
	    perror("could not change permissions of passwd file");
	RETURNPFAILURE;
    }

    return PSUCCESS;
}


/* this may be put into lib/pfs. 
 * gets a vlink structure for the vs_description file.
 */
static VLINK p_get_vsdesc()
{
    VLINK vl = vlalloc();

    if (!pget_dfile()) {
	if (pfs_debug)
	    fprintf(stderr, 
		    "could not get value of env. var. vsdesc_file\n");
	return NULL;
    }

    if (!pget_dhost()) {
	if (pfs_debug)
	    fprintf(stderr, 
		    "could not get value of env. var. vsdesc_host\n");
	return NULL;
    }
	
    vl->hsoname = stcopyr(pget_dfile(), vl->hsoname);
    vl->host    = stcopyr(pget_dhost(), vl->host);

    return vl;
}


/* gets the value of the principal attribute of the vs-description */
static TOKEN get_vsdesc_principals()
{
    VLINK   vsdesc_vl;
    PATTRIB attrs;
    TOKEN   princ_list = NULL;

    if (!(vsdesc_vl = p_get_vsdesc()))
	return NULL;
    
    if (!(attrs = pget_at(vsdesc_vl, "PRINCIPAL")))
	return NULL;
    
    for (; attrs; attrs = attrs->next)
	princ_list = tkappend(attrs->value.sequence->next->token,
			      princ_list);

    return princ_list;
}


/* goes sequentially through file, and when principal is a member of */
/* princ_list *and* the hostname is specific, asks user if passwd to */
/* be set for this host. if yes, returns hostname, else repeats with */
/* next matching entry. */ 
static char *get_host_from_user(TOKEN princ_list)
{
    char  *dirhst;
    FILE  *psession_fp;
    char  *psession_filename = get_psession_filename();
    char  line[255], ans[255];
    static char *server = NULL, *princ_name = NULL, *passwd = NULL;
    char  *hostname = NULL;
    int   num;

    if (!(psession_fp = fopen(psession_filename, "r+"))) {
	if (pfs_debug)
	    fprintf(stderr, "could not open password file\n");
	return NULL;
    }

    while (fgets(line, sizeof(line), psession_fp)) {
	num = qsscanf(line, "AUTHENTICATE %'&s P_PASSWORD %'&s %'&s\n", 
		      &server, &princ_name, &passwd);
	if (num < 3)
	    continue;
	if (member(princ_name, princ_list) &&
	    isdigit(server[0])) {
	    hostname = addr_to_name(server);
	    printf("set password on host %s? (y/n): ",
		   hostname);
	    gets(ans);
	    if (strequal(ans, "y"))
		return (hostname);
	}
    }

    return NULL;
}


static char *addr_to_name(char *addr)
{
    unsigned long hostaddr;
    struct hostent *host;
    char *hostname = NULL;

    hostaddr = inet_addr(addr);
    host = gethostbyaddr((char *)&hostaddr,
			 sizeof(hostaddr), AF_INET);
    if(host) 
	hostname = stcopy(host->h_name);
    else 
	hostname = stcopy(addr);
    return hostname;
}


struct amounts {
    char *currency;
    char *prompt_amt;
    char *max_amt;
    struct amounts *next;
};


static int account(int argc, char *argv[])
{
    char *acc_method = NULL, *server = NULL, *acc_server= NULL;
    char *account = NULL, *verifier = NULL, *int_bill_ref = NULL;
    struct amounts *amt_list = NULL, *curr_amt, *last_amt = NULL;
    char *prompt_and_get();
    
    if (argc > 0) {
	if (argv[0][1] != '-')
	    acc_method = stcopy(argv[0]);
	argc--, argv++;
	while (argc > 0 && **argv == '-') {
	    if (strequal(argv[0], "-server")) {
		server = stcopy(argv[1]);
		argc-=2, argv+=2;
	    }
	    else if (strequal(argv[0], "-acc_server")) {
		acc_server = stcopy(argv[1]);
		argc-=2, argv+=2;
	    }
	    else if (strequal(argv[0], "-account")) {
		account = stcopy(argv[1]);
		argc-=2, argv+=2;
	    }
	    else if (strequal(argv[0], "-verifier")) {
		verifier = stcopy(argv[1]);
		argc-=2, argv+=2;
	    }
	    else if (strequal(argv[0], "-int_bill_ref")) {
		int_bill_ref = stcopy(argv[1]);
		argc-=2, argv+=2;
	    }
	    else if (strequal(argv[0], "-currency")) {
		curr_amt = (struct amounts *) stalloc(sizeof(struct
							     amounts));
		curr_amt->currency = stcopy(argv[1]);
		curr_amt->prompt_amt = stcopy(argv[2]);
		curr_amt->max_amt = stcopy(argv[3]);
		curr_amt->next = NULL;

		if (amt_list == NULL)
		    amt_list = curr_amt;
		else
		    last_amt->next = curr_amt;
		last_amt = curr_amt;
		argc -=4, argv+=4;
	    }
	    else 
		usage();
	}
	if (argc > 0)
	    usage();
    }
    if (!acc_method)
	acc_method = prompt_and_get("Accounting method");
    if (!server)
	server = prompt_and_get("Server");
    if (!acc_server)
	acc_server = prompt_and_get("Accounting server");
    if (!account)
	account = prompt_and_get("Account name");
    if (!verifier) {
	verifier = (char *) stalloc(255);
	read_password("Verifier", verifier, p__bstsize(verifier));
    }
    if (!int_bill_ref)
	int_bill_ref = prompt_and_get("Internal billing reference");
    if (!amt_list) {
	while (1){
	    curr_amt = (struct amounts *) stalloc(sizeof(struct
							 amounts));
	    curr_amt->currency = prompt_and_get("Currency");
	    if (curr_amt->currency[0] == '\0') {
		stfree(curr_amt);
		break;
	    }
	    curr_amt->prompt_amt = prompt_and_get("Prompt amount");
	    curr_amt->max_amt    = prompt_and_get("Max amount");
	    curr_amt->next = NULL;
	    if (amt_list == NULL) 
		amt_list = curr_amt;
	    else 
		last_amt->next = curr_amt;
	    last_amt = curr_amt;
	};
    }

    if (amt_list == NULL) {
	fprintf(stderr, 
		"ERROR: Amounts must be specified in atleast one currency!\n");
	exit(1);
    }

    /* Write to file */
    write_acc_to_file(server, acc_method, acc_server, account, 
		      verifier, int_bill_ref, amt_list);

    /* Free allocated strings */
    stfree(server);
    stfree(acc_method);
    stfree(acc_server);
    stfree(account);
    stfree(verifier);
    stfree(int_bill_ref);
    while (amt_list) {
	stfree(amt_list->currency);
	stfree(amt_list->prompt_amt);
	stfree(amt_list->max_amt);
	amt_list = amt_list->next;
    }
    return PSUCCESS;
}


static int write_acc_to_file(char *server, char *acc_method, 
			     char *acc_server, char *account, 
			     char *verifier, char *int_bill_ref,
			     struct amounts *amt_list)
{
    char in_line[255], out_line[255];
    static char *hostname = NULL;
    struct amounts *amtp;
    char *hname = stcopy(server);
    long pos;
    int num;
    char *psession_filename = get_psession_filename();
    FILE *psession_fp;
    char *ppw_encode();

    /* canonicalize host if not wildcarded */
    if (!(hname = canonicalize(hname)))
	RETURNPFAILURE;

    if (!(psession_fp = fopen(psession_filename, "r+"))) {
	/* cannot open password file, so create */
	if (!(psession_fp = fopen(psession_filename, "w+"))) {
	    if (pfs_debug)
		fprintf(stderr, "cannot create file %s\n",
			psession_filename);
	    RETURNPFAILURE;
	}
	if (chmod(psession_filename, 0600)) {
	    if (pfs_debug) 
		perror("could not change permissions of passwd file");
	    RETURNPFAILURE;
	}
	fprintf(psession_fp, "# Prospero client session file.\n\n");
	fprintf(psession_fp, "AUTHENTICATE * UNAUTHENTICATED ''\n");
	fseek(psession_fp, 0L, 1); /* to allow input after output */
    }

    /* check if entry exists for principal */
    while (pos = ftell(psession_fp),      /* remember position of line */
           fgets(in_line, sizeof(in_line), psession_fp)) { 
        num = qsscanf(in_line, "ACCOUNT %'&s", 
		      &hostname);
	if (num < 1)
	    continue;
        if (strequal(hname, hostname)) {
            /* entry found; set file pointer to overwrite */
            fseek(psession_fp, pos, 0);
            break;
        }
    }

    qsprintf(out_line, sizeof(out_line), 
	     "ACCOUNT %'s %'s %'s %'s %'s %'s\n", 
	     hname, acc_method, acc_server, account,
	     ppw_encode(verifier), int_bill_ref);
    
    if (feof(psession_fp)) {
	fputs(out_line, psession_fp);
	for (amtp = amt_list; amtp; amtp = amtp->next)
	    qfprintf(psession_fp, "CURRENCY %'s %'s %'s\n",
		     amtp->currency, amtp->prompt_amt, amtp->max_amt);
    }
    else {
	/* cannot insert space; have to create new file	*/
	FILE *tmp_fp;
	int newpos, uid = getuid();
	char tmp_filename[255];
	char line[255];

	qsprintf(tmp_filename, sizeof(tmp_filename), "%s_tmp", 
		 psession_filename);
	if (!(tmp_fp = fopen(tmp_filename, "w"))) {
	    if (pfs_debug)
		fprintf(stderr, "cannot create temporary file %s\n",
			tmp_filename);
	    RETURNPFAILURE;
	}

	rewind(psession_fp);

	while (newpos = ftell(psession_fp), 
	       fgets(line, sizeof(line), psession_fp) 
	       && (newpos < pos))
	    fputs(line, tmp_fp);
		
	fputs(out_line, tmp_fp);

	for (amtp = amt_list; amtp; amtp = amtp->next) 
	    qfprintf(tmp_fp, "CURRENCY %'s %'s %'s\n",
		     amtp->currency, amtp->prompt_amt, amtp->max_amt);

	while(fgets(line, sizeof(line), psession_fp),
	      strnequal(line, "CURRENCY", strlen("CURRENCY")));
	fputs(line, tmp_fp);

	while(fgets(line, sizeof(line), psession_fp))
	    fputs(line, tmp_fp);
	
	(void) fclose(tmp_fp);
	(void) fclose(psession_fp);
		
	if (rename(tmp_filename, psession_filename)) {
	    unlink(tmp_filename);
	    if (pfs_debug)
		perror("could not overwrite password file");
	    RETURNPFAILURE;
	}

	if (chmod(psession_filename, 0600)) {
	    if (pfs_debug) 
		perror("could not change permissions of passwd file");
	    RETURNPFAILURE;
	}

	stfree(hname);
	return PSUCCESS;
    }

    stfree(hname);

    (void) fclose(psession_fp);
    return PSUCCESS;
}


char *prompt_and_get(char *prompt)
{
    char buf[255];
    
    printf("%s: ", prompt);
    gets(buf);
    return(stcopy(buf));
}


/* Display command usage and exit. Needs to be updated. */
static int usage()
{
    fprintf(stderr,  
	    "Usage: psession [-delete] -authenticate P_PASSWORD\n");
    fprintf(stderr,
	    "           [-s [-m]] [-d] [-D[#]] [=N[#]]\n");
    fprintf(stderr,
	    "           [-p <principal>] [-p <principal>] .. [hostname]\n");
    fprintf(stderr,
	    "OR:    psession -p_password\n");
    fprintf(stderr,
	    "           [-s [-m]] [-d] [-D[#]] [-N[#]]\n");
    fprintf(stderr,
	    "           [-p <principal>] [-p <principal>] .. [hostname]\n");
    fprintf(stderr,
	    "OR:    ppw [-s [-m]] [-d] [-D[#]] [-N[#]]\n");
    fprintf(stderr,
	    "           [-p <principal>] [-p <principal>] .. [hostname]\n");
    fprintf(stderr, 
	    "OR:    psession -account [<method>] [-server <server>]\n");
    fprintf(stderr,
	    "           [-acc_server <acc_server>] [-account <acc_name>]\n");
    fprintf(stderr,
	    "           [-verifier <verifier>] [-int_bill_ref <ibf>]\n");
    fprintf(stderr,
	    "           [-currency <currency> <prompt_amt> <max_amt>]\n");
    fprintf(stderr, 
	    "OR:    psession -delete -authenticate <method> [-server <server>]\n");
    fprintf(stderr, 
	    "OR:    psession -delete -account [-server <server>]\n");
    fprintf(stderr, 
	    "OR:    psession -authenticate KERBEROS [-s <server>]\n");
    fprintf(stderr, 
	    "                [-p <principal] [-f <ticket-file>]\n");
    fprintf(stderr, 
	    "OR:    psession -zero\n");
    exit(1);
}


static struct termios	permmodes;

/* read user's password */
static int read_password(char *prompt, char *pwstr, int pwlen)
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
    pwstr[strlen(pwstr)-1] = '\0';
    printf("\n");
    fflush(stdout);

    if(tcsetattr(fileno(stdin), TCSANOW, &permmodes)) return(errno);
    signal(SIGQUIT,abort_prog);
    signal(SIGINT,abort_prog);
    return(0);
}

static void abort_pwread()
{
    tcsetattr(fileno(stdin), TCSANOW, &permmodes);
    abort_prog();
}

static void abort_prog()
{
    printf("aborted\n");
    exit(0);
}

/* if the hostname is not wildcarded, returns the internet address as */
/* a dotted string. if wildcarded, the hostname is just converted to */
/* uppercase and returned.*/
static char *canonicalize(char *hostname)
{
    struct hostent *host;
    char           *cp;
    struct in_addr hostaddr;
    
    if (!hostname)
	return NULL;

    /* check if wildcarded */
    for (cp = hostname; *cp; ++cp)
	if (*cp == '*') 	/* cannot cannicalize */
	    return uppercase(hostname);

    /* look up server host */
    assert(P_IS_THIS_THREAD_MASTER());
    if ((host = gethostbyname(hostname)) == (struct hostent *) 0) {
	if (pfs_debug) 
	    fprintf(stderr, "canonicalize(): %s: unknown host\n", hostname);
	return NULL;
    }

    bcopy(host->h_addr,&hostaddr,sizeof(hostaddr));
    hostname = stcopyr(inet_ntoa(hostaddr), hostname);

    return hostname;
}

/* convert a string to uppercase */
static char *
uppercase(char *str)
{
    char *cp;

    if (!str)
	return NULL;

    for (cp = str; *cp; cp++)
	if (islower(*cp))
            *cp = toupper(*cp);

    return str;
}


int
add_krb_entry(int argc, char *argv[])
{
    char *server = NULL, *principal = NULL, *tkt_file = NULL;
    char in_line[255], out_line[255];
    static char *hostname = NULL, *method = NULL;
    long pos;
    int num;
    char *psession_filename = get_psession_filename();
    FILE *psession_fp;

    while (argc > 0 && **argv == '-') {
        switch (*(argv[0]+1)) {
        case 's':
	    if (server)
		usage();
	    server = stcopy(argv[1]);
	    argc-=2, argv+=2;
	    break;
        case 'p':
	    if (principal)
		usage();
	    principal = stcopy(argv[1]);
	    argc-=2, argv+=2;
	    break;
	case 'f':
	    if (tkt_file)
		usage();
	    tkt_file = stcopy(argv[1]);
	    argc-=2, argv+=2;
	    break;	    
	default:
	    usage();
	}
    }

    if (!server) 
	server = prompt_and_get("Server");
    if (!principal)
	principal = prompt_and_get("Principal");
    if (!tkt_file)
	tkt_file = prompt_and_get("Ticket file");

    /* canonicalize host if not wildcarded */
    if (!(server = canonicalize(server)))
	RETURNPFAILURE;

    if (!(psession_fp = fopen(psession_filename, "r+"))) {
	/* cannot open session file, so create */
	if (!(psession_fp = fopen(psession_filename, "w+"))) {
	    if (pfs_debug)
		fprintf(stderr, "cannot create file %s\n",
			psession_filename);
	    RETURNPFAILURE;
	}
	if (chmod(psession_filename, 0600)) {
	    if (pfs_debug) 
		perror("could not change permissions of passwd file");
	    RETURNPFAILURE;
	}
	fprintf(psession_fp, "# Prospero client session file.\n\n");
	fprintf(psession_fp, "AUTHENTICATE * UNAUTHENTICATED ''\n");
	fseek(psession_fp, 0L, 1); /* to allow input after output */
    }

    /* check if entry exists for principal */
    while (pos = ftell(psession_fp),      /* remember position of line */
           fgets(in_line, sizeof(in_line), psession_fp)) { 
        num = qsscanf(in_line, "AUTHENTICATE %'&s %'&s", 
		      &hostname, &method);
	if (num < 2 || strcmp(method, "KERBEROS"))
	    continue;
        if (strequal(server, hostname)) { 
            /* entry found; set file pointer to overwrite */
            fseek(psession_fp, pos, 0);
            break;
        }
    }

    qsprintf(out_line, sizeof(out_line), 
	     "AUTHENTICATE %'s KERBEROS %'s %'s\n", 
	     server, principal, tkt_file);
    
    if (feof(psession_fp) || strlen(in_line) == strlen(out_line))
	fputs(out_line, psession_fp);
    else {
	/* cannot insert space; have to create new file	*/
	FILE *tmp_fp;
	int newpos, uid = getuid();
	char tmp_filename[255];
	char line[255];

	qsprintf(tmp_filename, sizeof(tmp_filename), "%s_tmp", 
		 psession_filename);
	if (!(tmp_fp = fopen(tmp_filename, "w"))) {
	    if (pfs_debug)
		fprintf(stderr, "cannot create temporary file %s\n",
			tmp_filename);
	    RETURNPFAILURE;
	}

	rewind(psession_fp);

	while (newpos = ftell(psession_fp), 
	       fgets(line, sizeof(line), psession_fp) 
	       && (newpos < pos))
	    fputs(line, tmp_fp);
		
	fputs(out_line, tmp_fp);
	
	while(fgets(line, sizeof(line), psession_fp))
	    fputs(line, tmp_fp);
	
	(void) fclose(tmp_fp);
	(void) fclose(psession_fp);
		
	if (rename(tmp_filename, psession_filename)) {
	    unlink(tmp_filename);
	    if (pfs_debug)
		perror("could not overwrite session file");
	    RETURNPFAILURE;
	}

	if (chmod(psession_filename, 0600)) {
	    if (pfs_debug) 
		perror("could not change permissions of passwd file");
	    RETURNPFAILURE;
	}

	stfree(server);
	return PSUCCESS;
    }

    stfree(server);

    (void) fclose(psession_fp);
    return PSUCCESS;    
}


int
del_krb_entry(int argc, char *argv[])
{
    char *server = NULL;
    char line[255], *hostname = NULL, *method = NULL;
    char tmp_filename[255];
    int num;
    char *psession_filename = get_psession_filename();
    FILE *psession_fp, *tmp_fp;

    if (argc > 1 && strequal(argv[0], "-server")) {
	server = stcopy(argv[1]);
	/* canonicalize host if not wildcarded */
	if (!(server = canonicalize(server)))
	    RETURNPFAILURE;
    }

    if (!(psession_fp = fopen(psession_filename, "r"))) {
	fprintf(stderr, "could not open session file: %s\n",
		psession_filename);
    	RETURNPFAILURE;
    }

    qsprintf(tmp_filename, sizeof(tmp_filename), "%s_tmp", psession_filename);
    if (!(tmp_fp = fopen(tmp_filename, "w"))) {
        if (pfs_debug)
            perror("could not create temporary file");
	RETURNPFAILURE;
    }

    while (fgets(line, sizeof(line), psession_fp)) { 
        num = qsscanf(line, "AUTHENTICATE %'&s %'&s", 
		      &hostname, &method);
	if (num < 2 || strcmp(method, "KERBEROS") ||
	    (server && strcmp(server, hostname)))
	    fputs(line, tmp_fp);
    }

    (void) fclose(tmp_fp);
    (void) fclose(psession_fp);

    if (rename(tmp_filename, psession_filename)) {
	unlink(tmp_filename);
	if (pfs_debug)
	    perror("could not overwrite session file");
    	RETURNPFAILURE;
    }
    
    if (chmod(psession_filename, 0600)) {
	if (pfs_debug) 
	    perror("could not change permissions of passwd file");
	RETURNPFAILURE;
    }

    return PSUCCESS;
}


int
del_unauth_entry(int argc, char *argv[])
{
    char *server = NULL;
    char line[255], *hostname = NULL, *method = NULL;
    char tmp_filename[255];
    int num;
    char *psession_filename = get_psession_filename();
    FILE *psession_fp, *tmp_fp;

    if (argc > 1 && strequal(argv[0], "-server")) {
	server = stcopy(argv[1]);
	/* canonicalize host if not wildcarded */
	if (!(server = canonicalize(server)))
	    RETURNPFAILURE;
    }

    if (!(psession_fp = fopen(psession_filename, "r"))) {
	fprintf(stderr, "could not open session file: %s\n",
		psession_filename);
    	RETURNPFAILURE;
    }

    qsprintf(tmp_filename, sizeof(tmp_filename), "%s_tmp", psession_filename);
    if (!(tmp_fp = fopen(tmp_filename, "w"))) {
        if (pfs_debug)
            perror("could not create temporary file");
	RETURNPFAILURE;
    }

    while (fgets(line, sizeof(line), psession_fp)) { 
        num = qsscanf(line, "AUTHENTICATE %'&s %'&s", 
		      &hostname, &method);
	if (num < 2 || strcmp(method, "UNAUTHENTICATED") ||
	    (server && strcmp(server, hostname)))
	    fputs(line, tmp_fp);
    }

    (void) fclose(tmp_fp);
    (void) fclose(psession_fp);

    if (rename(tmp_filename, psession_filename)) {
	unlink(tmp_filename);
	if (pfs_debug)
	    perror("could not overwrite session file");
    	RETURNPFAILURE;
    }
    
    if (chmod(psession_filename, 0600)) {
	if (pfs_debug) 
	    perror("could not change permissions of passwd file");
	RETURNPFAILURE;
    }

    return PSUCCESS;
}


int
del_acc_entry(int argc, char *argv[])
{
    char *server = NULL;
    char line[255], *hostname = NULL;
    char tmp_filename[255];
    int num, deleting = 0;
    char *psession_filename = get_psession_filename();
    FILE *psession_fp, *tmp_fp;

    if (argc > 1 && strequal(argv[0], "-server")) {
	server = stcopy(argv[1]);
	/* canonicalize host if not wildcarded */
	if (!(server = canonicalize(server)))
	    RETURNPFAILURE;
    }

    if (!(psession_fp = fopen(psession_filename, "r"))) {
	fprintf(stderr, "could not open session file: %s\n",
		psession_filename);
    	RETURNPFAILURE;
    }

    qsprintf(tmp_filename, sizeof(tmp_filename), 
	     "%s_tmp", psession_filename);
    if (!(tmp_fp = fopen(tmp_filename, "w"))) {
        if (pfs_debug)
            perror("could not create temporary file");
	RETURNPFAILURE;
    }

    while (fgets(line, sizeof(line), psession_fp)) {
	if (deleting) {
	    if (strnequal(line, "CURRENCY", strlen("CURRENCY")))
		continue;
	    else
		deleting = 0;
	}
        num = qsscanf(line, "ACCOUNT %'&s", 
		      &hostname);
	if (num < 1 || (server && strcmp(server, hostname)))
	    fputs(line, tmp_fp);
	else /* Delete */
	    deleting = 1;
    }

    (void) fclose(tmp_fp);
    (void) fclose(psession_fp);

    if (rename(tmp_filename, psession_filename)) {
	unlink(tmp_filename);
	if (pfs_debug)
	    perror("could not overwrite session file");
    	RETURNPFAILURE;
    }
    
    if (chmod(psession_filename, 0600)) {
	if (pfs_debug) 
	    perror("could not change permissions of passwd file");
	RETURNPFAILURE;
    }

    return PSUCCESS;
}
