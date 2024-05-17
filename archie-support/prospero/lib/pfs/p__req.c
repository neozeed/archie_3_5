/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>
 *
 * Written  by swa    1992     to provide common code for starting request
 * Modified by prasad 1992     to add Kerberos support
 * Modified by bcn    1/93     to use new ardp library and rreq structure
 * Modified by swa    6/93     to construct multiple-packet messages.
 */

#include <usc-copyr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
extern int h_errno;

/* For compatability between GCC-2.3.1 running under SunOS 4.1 and the sunOS
 * standard include files (they define size_t and others differently), we MUST 
 * include stddef and stdarg after anything that might include the sun include
 * file <sys/stdtypes.h> (in this case, <pwd.h> includes <sys/stdtypes.h>).
 */ 

#include <stddef.h>
#include <stdarg.h>

#include <ardp.h>
#include <psite.h> /* must precede pfs.h for defs. of P_KERBEROS, */
		   /* P_P_PASSWORD  */
#include <pfs.h>
#include <pprot.h>
#include <perrno.h>
#include <pcompat.h>

#ifdef P_KERBEROS
#include <krb5/krb5.h>
#endif /* P_KERBEROS */

/* gives us a name for this buffer, which we need so we
   can keep the reference around.  */

/* Ok that this is static, since it's read-only. */
static char     pfs_sw_id_buf[] = PFS_SW_ID; 
static char	*pfs_sw_id = pfs_sw_id_buf;

/* Oops - I dont think a "char p__username" is very usefull! - Mitra*/
char     *p__username = NULL;

extern int	pfs_debug;

static PAUTH p__get_pauth(int type, const char *hostname);
#ifdef P_P_PASSWORD
char *ppw_encode();
#endif /* P_P_PASSWORD */
char *get_psession_filename();
static char *uid_to_name();
#ifdef P_ACCOUNT
static void p__add_acc_req();
#endif

/* This code handles possible multiple resettings of the software ID
   without any memory leaks.
   It is internal; should only be called through p_initialize().   Nothing will
   break if you ignore this admonition, but it keeps the library interface
   smaller. */
void
p__set_sw_id(char *app_sw_id)
{
    assert(P_IS_THIS_THREAD_MASTER());
    if (app_sw_id && *app_sw_id) {
        if (pfs_sw_id == pfs_sw_id_buf)
            pfs_sw_id = qsprintf_stcopyr(NULL,"%s(%s)",app_sw_id,PFS_SW_ID);
        else 
            pfs_sw_id = qsprintf_stcopyr(pfs_sw_id,"%s(%s)",app_sw_id,
                                         PFS_SW_ID);
    } else  {
        /* Resetting software ID to normal values. */
        if (pfs_sw_id == pfs_sw_id_buf)
            ;                   /* Same as before; leave set to default. */
        else {
            stfree(pfs_sw_id);  /* must be set to some allocated memory. */
            pfs_sw_id = pfs_sw_id_buf;
        }
    }
}


/* 
 * p__start_req - start a prospero request
 *
 *     p__start_req takes the name of the server (used by Kerberos to 
 *     determine what credentials are needed), it allocates a request
 *     structure, and it fills in the start, including the Prospero
 *     version and software ID, and authentication information.  The
 *     function returns a request of type RREQ, which can be used 
 *     in subsequent calls to p__addreq, and passed to ardp_send.
 */
RREQ
p__start_req(const char server_hostname[])
{
    RREQ req = ardp_rqalloc(); /* The request to fill in         */
    PAUTH authinfo;      /* Structure containing authentication info */
    PAUTH authp;         /* To iterate through authinfo list */
    TOKEN prin;          /* To Iterate through principal names       */

#ifdef CLIENTS_REQUEST_VERSION_FROM_SERVER
    p__add_req(req, "VERSION\n");
#endif
    p__add_req(req, "VERSION %d %s", VFPROT_VNO, pfs_sw_id);

    authinfo = p__get_pauth(PFSA_UNAUTHENTICATED, server_hostname);
    if (authinfo) {
	p__add_req(req, "\nAUTHENTICATE '' UNAUTHENTICATED %'s",
		   authinfo->authenticator);
	/* For the UNAUTHENTICATED and KERBEROS authentication types, */
	/* the principals are optional additional information for */
	/* debugging use.  We don't worry about them here. */
	for(prin = authinfo->principals; prin; prin = prin->next)
	    p__add_req(req, " %'s", prin->token); 
	pafree(authinfo);
    }

#ifdef P_KERBEROS
    authinfo = p__get_pauth(PFSA_KERBEROS, server_hostname);
    for (authp = authinfo; authp; authp = authp->next) {
        p__add_req(req, "\nAUTHENTICATE '' KERBEROS %'s", 
                   authp->authenticator);
        /* For the UNAUTHENTICATED and KERBEROS authentication types,  the
           principals are optional additional information for debugging use.
           We don't worry about them here. */
        for (prin = authp->principals; prin; prin = prin->next)
            p__add_req(req, " %'s", prin->token); 
    }
    if (authinfo)
	palfree(authinfo);
#endif
#ifdef P_P_PASSWORD
    authinfo = p__get_pauth(PFSA_P_PASSWORD, server_hostname);
    for (authp = authinfo; authp; authp = authp->next) {
        p__add_req(req, "\nAUTHENTICATE OPTIONAL P_PASSWORD %'s", 
                   authp->authenticator);
	/* Principal NOT optional here! */
        for (prin = authp->principals; prin; prin = prin->next)
	    p__add_req(req, " %'s", prin->token); 
    }
    if (authinfo)
	palfree(authinfo);
#endif
    p__add_req(req, "\n");
#ifdef P_ACCOUNT
    p__add_acc_req(req, server_hostname);
#endif
    return(req);
}

/* Adds stuff to a Prospero request. */
int
p__add_req(RREQ req, const char fmt[], ...)
{
    int retval;
    va_list ap;
    va_start(ap,fmt);
    retval = vp__add_req(req, fmt, ap);
    va_end(ap);
    return(retval);
}

int
vp__add_req(RREQ req, const char fmt[], va_list ap)
{
    PTEXT	ptmp;           /* packet being written to. */
    int         mesglen;        /* # of characters in this message, NOT 
                                   including trailing NUL which doesn't need to
                                   be sent. */ 
    AUTOSTAT_CHARPP(bufp);          /* buffer for long packets */
    int numsent;                /* How many chars. out of buf have been sent so
                                   far? */

    if(!req->outpkt) {
	ptmp = ardp_ptalloc();
	if(!ptmp) RETURNPFAILURE;
	APPEND_ITEM(ptmp,req->outpkt);
    } else {
        ptmp = req->outpkt->previous; /* last member of list. */
    }

    /* Subtract 1: Trailing NUL character isn't sent,
       since the length of the message is encoded into the message.  Recipient
       will automatically append the NUL byte for convenience of the users. */
    /* Add one to buffer space available, since it includes an MBZ byte to
       catch runaway strings.  vqsprintf() will always null-terminate the
       strings it writes. */
    mesglen = 
        vqsprintf(ptmp->ioptr, ARDP_PTXT_LEN + 1 - ptmp->length, fmt, ap) - 1;
    if (ptmp->length + mesglen <= ARDP_PTXT_LEN) {
        ptmp->length += mesglen;
        ptmp->ioptr = ptmp->start + ptmp->length;
        return(PSUCCESS);
    }
    /* Ran out of room.  Have to write it to a temporary buffer, then copy. */
    /* Four-line Optimization: we already know we're going to need mesglen + 1
       of buffer space.  By doing this, we save a possible double calling of
       vqsprintf() in p__vqbstprintf_stcopyr(). */
    if (p__bstsize(*bufp) < mesglen + 1) {
        stfree(*bufp);
        *bufp = stalloc(mesglen + 1);
    }
    /* There used to be a lot of code here that did this in a slightly more
       efficient way, but I am willing to suffer a performance penalty
       in order to make this work more transparently. */
    *bufp = p__vqbstprintf_stcopyr(*bufp, fmt, ap);
    assert(mesglen == p_bstlen(*bufp));

    /* Record what we already sent. */
    numsent = ARDP_PTXT_LEN - ptmp->length;
    ptmp->length = ARDP_PTXT_LEN;
    /* Start this loop with the current ptmp full of as much data as it can
       hold. */ 
    while (numsent < mesglen) {
	ptmp = ardp_ptalloc();
	if(!ptmp) RETURNPFAILURE;
	APPEND_ITEM(ptmp,req->outpkt);
        ptmp->length = ( mesglen - numsent > ARDP_PTXT_LEN
                        ? ARDP_PTXT_LEN : mesglen - numsent);
        strncpy(ptmp->start, *bufp + numsent, ptmp->length);
        ptmp->ioptr = ptmp->start + ptmp->length;
        numsent += ptmp->length;
    }
    return PSUCCESS;
}

void
p__set_username(char *un)
{
  p__username = stcopyr(un, p__username);
}

static 
PAUTH p__get_pauth(int type, const char server_hostname[])
{
#ifdef P_KERBEROS
    krb5_data buf;
    krb5_checksum send_cksum;
    krb5_ccache ccdef;
    krb5_principal server;
    static char *tkt_file = NULL;
    int retval = 0;
#endif
#ifdef P_P_PASSWORD
    PAUTH curr_auth = NULL;
    static char *password = NULL;
    typedef struct _p_passwd {
	char *hostname;
	char *princ_name;
	char *password;
	struct _p_passwd *next;
    } p_passwd;
    p_passwd *p_list = NULL, *p_ptr;
#endif    
    PAUTH auth = paalloc();
    int ruid, euid;
    FILE *psession_fp;
    char *psession_filename = get_psession_filename();
    char line[255];
    AUTOSTAT_CHARPP(hostnamep);
    AUTOSTAT_CHARPP(princ_namep);
    int num, entry_exists = 0;
    char full_hname[255];
    struct hostent *host;
    char *cp;
    char *hostaddr = NULL;
    struct in_addr haddr;

#if defined(P_KERBEROS) || defined(P_P_PASSWORD)
    assert(P_IS_THIS_THREAD_MASTER());   /* Not thread safe yet */
#endif
    CHECK_MEM();
    auth->principals = NULL;    /* not really needed.  Let's be paranoid. */

    /* Convert hostname into canonical form */
    qsprintf(full_hname, sizeof full_hname, "%s", server_hostname);
    
    /* Strip off the trailing (portnum), if any */
    for (cp = full_hname; *cp; ++cp)
	;
    if (cp > full_hname && *--cp == ')') {
	while (*--cp && cp >= full_hname) {
	    if (*cp == '(') {
		*cp = '\0';
		break;
	    }
	    if (!isdigit(*cp)) 
		break;
	}
    }
    
    /* Look up server host */
    /* Not multi-threaded yet because clients not done yet. */
    CHECK_MEM();
    assert(P_IS_THIS_THREAD_MASTER());
    if ((host = gethostbyname(full_hname)) == (struct hostent *) 0) {
	if (pfs_debug) 
	    fprintf(stderr, "p__get_pauth(): unknown host %s\n",
		    full_hname);
	pafree(auth);
	return NULL;
    }
    qsprintf(full_hname, sizeof full_hname, "%s", host->h_name);
    bcopy(host->h_addr,&haddr,sizeof(haddr));    
    hostaddr = stcopy(inet_ntoa(haddr));
    
    switch(auth->ainfo_type = type) {
      case PFSA_UNAUTHENTICATED:
        if (p__username) {
	  auth->authenticator = stcopy(p__username);
	  return auth;
	}
	/* Upper-case to use in search of password file */
	for (cp = full_hname; *cp; cp++)
	    if (islower(*cp))
		*cp = toupper(*cp);

	/* Get the real and effective user ids */
	ruid = getuid();
	euid = geteuid();
	
	/* check if the psession file exists */
	if (!(psession_fp = fopen(psession_filename, "r")))
	    /* No file; send real uid across */
	    auth->authenticator = uid_to_name(ruid);
	else {
	    while (fgets(line, sizeof(line), psession_fp)) {
		num = qsscanf(line, 
			      "AUTHENTICATE %'&s UNAUTHENTICATED %'&s\n",
			      &(*hostnamep), &(*princ_namep)); 
		if (num < 2)
		    continue;
		if (wcmatch(full_hname, (*hostnamep)) || 
		    strequal(hostaddr, (*hostnamep))) {
		    entry_exists = 1;
		    break;
		}
	    }
	    fclose(psession_fp);
	    /* No entry in file; */
	    /* no UNAUTHENTICATED authentication sent */
	    if (!entry_exists) {
		pafree(auth);
		return NULL;
	    }
	    if ((*princ_namep)[0] && (!ruid || euid)) 
#ifdef OVERRIDE_DEFAULT_USERNAME		
		auth->authenticator = stcopy((*princ_namep));
#else
	        auth->authenticator = uid_to_name(ruid);
#endif
	    else
		auth->authenticator = uid_to_name(ruid);
	}

        return(auth);
	break;
	
#ifdef P_KERBEROS       /* 4/5/93  Made error handling pass errors
                                   to higher layers. */
      case PFSA_KERBEROS:
	if (!(psession_fp = fopen(psession_filename, "r"))) {
	    /* No file; do not send KERBEROS authentication */
	    pafree(auth);
	    return NULL;
	}

	while (fgets(line, sizeof(line), psession_fp)) {
	    num = qsscanf(line, "AUTHENTICATE %'&s KERBEROS %'&s %'&s\n",
			  &(*hostnamep), &(*princ_namep), &tkt_file); 
	    if (num < 3)
		continue;
	    if (wcmatch(full_hname, (*hostnamep)) || 
		strequal(hostaddr, (*hostnamep))) {
		entry_exists = 1;
		break;
	    }
	}

	fclose(psession_fp);

	if (!entry_exists) {
	    /* No entry; do not send KERBEROS authentication */
	    pafree(auth);
	    return NULL;
	}
	
	/* If ticket-file name not empty, tell KRB5 routines to look */
	/* at it. */
	if (tkt_file[0])
	    setenv("KRB5CCNAME", tkt_file, 1);

	/* PREPARE KRB_AP_REQ MESSAGE */
	
	/* compute checksum, using CRC-32 */
	if (!(send_cksum.contents = (krb5_octet *)
	      stalloc(krb5_checksum_size(CKSUMTYPE_CRC32)))) {
            if (pfs_debug)
                fprintf(stderr, "p__get_pauth(): Cannot allocate Kerberos checksum\n");
            pafree(auth);
            return NULL;
	}
	
	/* choose some random stuff to compute checksum from */
	if (retval = krb5_calculate_checksum(CKSUMTYPE_CRC32,
					     (char *) server_hostname,
					     strlen(server_hostname),
					     0,
					     0, /* if length is 0, */
						/* crc-32 doesn't use */
						/* the seed */ 
					     &send_cksum)) {
            if (pfs_debug)
                fprintf(stderr, "p__get_pauth(): Error while computing Kerberos checksum\n");
            stfree(send_cksum.contents);
            pafree(auth);
            return NULL;
	}
	
        /* Get credentials for server, create krb_mk_req message */
	
	if (retval = krb5_cc_default(&ccdef)) {
            
	    if (pfs_debug) 
                fprintf(stderr, "p__get_pauth(): Error while getting default \
Kerberos  cache\n");
            stfree(send_cksum.contents);
            pafree(auth);
            return NULL;
	}
	
	/* Format service name into a principal structure and */
	/* canonicalizes host name. */
	if (retval = krb5_sname_to_principal(full_hname,
					     KERBEROS_SERVICE, 
					     1,        /* Canonicalize */
					     &server)) {
	    if (pfs_debug)
                fprintf(stderr, "p__get_pauth(): Error while setting up \
Kerberos server principal\n");
            stfree(send_cksum.contents);
            pafree(auth);
            return NULL;
	}
	
	if (retval = krb5_mk_req(server, 0 /* default options */,  &send_cksum,
				 ccdef, &buf)) {
	    if (pfs_debug) fprintf(stderr, "p__get_pauth(): Error while \
preparing Kerberos AP_REQ\n");
            pafree(auth);
            stfree(send_cksum.contents);
            return NULL;
	}
	stfree(send_cksum.contents);
	auth->authenticator = stalloc(AUTHENTICATOR_SZ);
	retval = binencode(buf.data, buf.length, 
			   auth->authenticator, 
			   AUTHENTICATOR_SZ);
	if (retval >= AUTHENTICATOR_SZ) {
	    internal_error("binencode: Buffer too small");
	}

	return(auth);
	break;
#endif /* P_KERBEROS */
#ifdef P_P_PASSWORD
      case PFSA_P_PASSWORD:
	/* Upper-case to use in search of password file */
	for (cp = full_hname; *cp; cp++)
	    if (islower(*cp))
		*cp = toupper(*cp);

	if (!(psession_fp = fopen(psession_filename, "r"))) {
#if 0                           /* This is annoying.   It's NOT AN ERROR for 
                                 somebody not to have set up a session file, at
                                 least not right now.  --swa, 9 May 1994 */
	    if (pfs_debug) 
                fprintf(stderr, "p__get_pauth(): Error in P_PASSWORD \
authentication: Cannot open psession file %s\n", psession_filename);
#endif
            pafree(auth);
            return NULL;
	}

	while (fgets(line, sizeof(line), psession_fp)) {
	    if (line[0] == '#')
		continue;
	    num = qsscanf(line, "AUTHENTICATE %'&s P_PASSWORD %'&s %'&s\n", 
			  &(*hostnamep), &(*princ_namep), &password);
	    
	    if (num < 3) /* Ignore if no password */
		continue;

	    if (wcmatch(full_hname, (*hostnamep)) || 
		strequal(hostaddr, (*hostnamep))) {
		p_passwd *last = NULL;

		for (p_ptr = p_list; p_ptr; last = p_ptr, p_ptr = p_ptr->next)
		    if (!strcmp((*princ_namep), p_ptr->princ_name)) {
			if (isdigit(p_ptr->hostname[0]))
			    break; /* Already have most specific */
				   /* address */
			
			if (isdigit((*hostnamep)[0]) ||
			    strlen((*hostnamep)) > strlen(p_ptr->hostname)) {
			    p_ptr->hostname = stcopyr((*hostnamep), p_ptr->hostname);
			    p_ptr->password = stcopyr(password, p_ptr->password);
			}
			break;
		    }
		
		if (!p_ptr) {
		    if (!last) {
			p_list = (p_passwd *) stalloc(sizeof(p_passwd));
			last = p_list;
		    }
		    else {
			last->next = (p_passwd *) stalloc(sizeof(p_passwd));
			last = last->next;
		    }
		    last->hostname = stcopy((*hostnamep));
		    last->princ_name = stcopy((*princ_namep));
		    last->password = stcopy(password);
		    last->next = NULL;
		}
	    }
	}
	
	(void) fclose(psession_fp);

	pafree(auth);
	auth = NULL;

	for (p_ptr = p_list; p_ptr; p_ptr = p_ptr->next) {
	    if (!auth)
		auth = curr_auth = paalloc();
	    else {
		curr_auth->next = paalloc();
		curr_auth = curr_auth->next;
	    }
	    curr_auth->ainfo_type = type;
	    curr_auth->authenticator = stcopy(ppw_encode(p_ptr->password));
	    curr_auth->principals = tkalloc(p_ptr->princ_name);
	    curr_auth->next = NULL;  
	}

        return(auth);
	break;
#endif /* P_P_PASSWORD */
      default:
	fprintf(stderr, "p__get_pauth(): Unknown authenticator type: %d\n", type);
        internal_error("Unknown authenticator type!");
        /* NOTREACHED */
    }
    assert(FALSE); /*Unreached*/
    return(NULL); /* To keep GCC happy*/
}



#ifdef P_ACCOUNT
static void 
p__add_acc_req(RREQ req, char *server_hostname)
{
    FILE *psession_fp;
    char *psession_filename = get_psession_filename();
    char line[255];
    static char *hostname = NULL, *acc_method = NULL;
    static char *acc_server = NULL, *acc_name = NULL;
    static char *acc_verifier = NULL, *int_bill_ref = NULL;
    static char *currency = NULL, *prompt_amt = NULL, *max_amt = NULL;
    struct hostent *host;
    char *cp;
    char *hostaddr = NULL;
    struct in_addr haddr;
    int num, entry_exists = 0;
    char full_hname[255];

    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
    /* Convert hostname into canonical form */
    qsprintf(full_hname, sizeof full_hname, "%s", server_hostname);
    
    /* Strip off the trailing (portnum), if any */
    for (cp = full_hname; *cp; ++cp)
	;
    if (cp > full_hname && *--cp == ')') {
	while (*--cp && cp >= full_hname) {
	    if (*cp == '(') {
		*cp = '\0';
		break;
	    }
	    if (!isdigit(*cp)) 
		break;
	}
    }
    
    /* Look up server host */
    assert(P_IS_THIS_THREAD_MASTER());
    if ((host = gethostbyname(full_hname)) == (struct hostent *) 0) {
	if (pfs_debug) 
	    fprintf(stderr, "p__add_acc_req(): unknown host %s\n",
		    full_hname);
	return;
    }
    qsprintf(full_hname, sizeof full_hname, "%s", host->h_name);
    bcopy(host->h_addr,&haddr,sizeof(haddr));    
    hostaddr = stcopy(inet_ntoa(haddr));
	
    /* Upper-case to use in search of password file */
    for (cp = full_hname; *cp; cp++)
	if (islower(*cp))
	    *cp = toupper(*cp);

    if (!(psession_fp = fopen(psession_filename, "r")))
	return;

    while (fgets(line, sizeof(line), psession_fp)) {
	num = qsscanf(line, "ACCOUNT %'&s %'&s %'&s %'&s %'&s %'&s\n",
		      &hostname, &acc_method, &acc_server, &acc_name,
		      &acc_verifier, &int_bill_ref); 

	if (num < 6)
	    continue;
	if (wcmatch(full_hname, hostname) || 
	    strequal(hostaddr, hostname)) {
	    p__add_req(req, "ACCOUNT MANDATORY %'s %'s %'s %'s %'s",
		       acc_method, acc_server, acc_name,
		       ppw_encode(acc_verifier), int_bill_ref);
	    while(fgets(line, sizeof(line), psession_fp) &&
		  strnequal(line, "CURRENCY", strlen("CURRENCY"))) {
		num = qsscanf(line, "CURRENCY %'&s %'&s %'&s\n",
			      &currency, &prompt_amt, &max_amt);
		if (num < 2)
		    continue;
		p__add_req(req, " %'s %'s", currency, prompt_amt);
	    }
	    p__add_req(req, "\n");
	    break;
	}
    }
    fclose(psession_fp);
}
#endif /* P_ACCOUNT */


/* returns name of user's psession file. */
char *get_psession_filename()
{
    int uid;
    char *home;
    static char *psession_filename = NULL;

    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
    if (!(psession_filename = stcopy(getenv("PSESSION")))) {
	uid = getuid();
	if (!(home = getenv("HOME"))) {
	    if (pfs_debug)
		fprintf(stderr, 
			"ERROR: Could not get value of HOME!\n");
	    return NULL;
	}
	psession_filename = qsprintf_stcopyr(psession_filename,
					     "%s/.psession_%d", 
					     home, uid);
    }

    return psession_filename;
}


/* Converts a uid into a username. If unknown uid, returns a string of */
/* the form "uid#<uid>" */
static char *uid_to_name(int uid)
{
    struct passwd *whoiampw;

    assert(P_IS_THIS_THREAD_MASTER()); /*getpwuid MT-Unsafe*/
    DISABLE_PFS(whoiampw = getpwuid(uid));
    if (whoiampw == 0) {
	char tmp_uid_str[100];
	qsprintf(tmp_uid_str, sizeof tmp_uid_str, "uid#%d", uid);
	return stcopy(tmp_uid_str);
    }
    else {
	return stcopy(whoiampw->pw_name);
    }
}


/* Two-way function to encode string.*/
char *ppw_encode(char *str)
{
    static char *retstr = NULL;
    int i;

  assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
    retstr = stcopyr(str, retstr);
    
    for (i = 0; retstr[i]; i++)
	if (retstr[i] > 31 && retstr[i] < 127)
	    retstr[i] = 126 - retstr[i] + 32;

    return retstr;
}

