/* Checks Kerberos authentication, and returns authenticated client name.
   Returns non-zero on error. */

/* Copyright (c) 1992 by the University of Southern California.
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */
/* Author: Prasad Upasani (prasad@isi.edu)
 * Modifications by swa@isi.edu.
 */


#include <psite.h>              /* must precede pfs.h for PSRV_KERBEROS 
                                 compilation option. */
#include <pfs.h>
#include <psrv.h>               /* prototypes this function */
#include <pserver.h>
#ifdef PSRV_KERBEROS            /* Only compile this file if we are using
                                   Kerberos, since it refers to files in the
                                   Kerberos libraries. */
/* This is currently actually used only by the server.  It is in libpsrv. */
#include <stdio.h>
#include <krb5/krb5.h>
#include <netinet/in.h>


/* Returns 0 (PSUCCESS) on success; any other value on failure (should make
   this be PFAILURE, just for consistency. */
int 
check_krb_auth(char *auth, struct sockaddr_in client, 
               char **ret_client_namep   /* Stores the name to be returned.
                                            Either NULL or memory previously
                                            allocated with stalloc() or
                                            stcopy(). */) 
{
    krb5_data inbuf;
    krb5_principal server;
    krb5_address *sender_addr;
    char service_name[255], hostname[255];
    int retval;
    krb5_address client_addr;
    krb5_tkt_authent *authdat;
    char *client_name;
    char *srvtab = NULL;   /* NULL means to use the default Kerberos srvtab */
#ifdef KERBEROS_SRVTAB
    int srvtabsiz;
#endif

    /* Create a server principal structure. */
    if (retval = gethostname(hostname, sizeof(hostname)))
        RETURNPFAILURE;

    strcpy(service_name, KERBEROS_SERVICE);
    strcat(service_name, "/");
    strcat(service_name, hostname);
    
    if (krb5_parse_name(service_name, &server))
        RETURNPFAILURE;

    /* Put auth data into a krb5_data structure. */
    inbuf.data = stalloc(AUTHENTICATOR_SZ); /* starting value */
 again:
    if (inbuf.data == NULL) out_of_memory();
    retval = bindecode(auth, inbuf.data, p__bstsize(inbuf.data));
    if (retval > p__bstsize(inbuf.data)) {
        stfree(inbuf.data);
        inbuf.data = stalloc(retval);
        goto again;
    }
    /* Successfully decoded. */
    inbuf.length = retval;

    /* Put client address in a krb5_address structure. */
    client_addr.addrtype = client.sin_family;
    client_addr.length = sizeof(client.sin_addr);
    client_addr.contents = (krb5_octet *) &client.sin_addr;

#ifdef KERBEROS_SRVTAB
    srvtab = 
        stalloc(srvtabsiz = sizeof("FILE:") + sizeof(KERBEROS_SRVTAB) + 1);
    assert(qsprintf(srvtab, srvtabsiz, "FILE:%s", KERBEROS_SRVTAB) <= srvtabsiz);
#endif

    /* Check authentication. */
    retval = krb5_rd_req(&inbuf,
			 server,
			 &client_addr,
			 srvtab,
			 0, 0, 0,
                         &authdat);

    stfree(inbuf.data);
#ifdef KERBEROS_SRVTAB
    stfree(srvtab);
#endif

    if (retval) RETURNPFAILURE;          /* Authentication failed. */

    /* Get client name. */
    /* client_name points to memory allocated with malloc() by
       krb5_unparse_name().   The caller is supposed to free it. */
    if (retval = krb5_unparse_name(authdat->authenticator->client, 
				   &client_name))
      RETURNPFAILURE;
  
    *ret_client_namep = stcopyr(client_name, *ret_client_namep);
    free(client_name);
    return PSUCCESS;
}

    
#endif /* PSRV_KERBEROS */
