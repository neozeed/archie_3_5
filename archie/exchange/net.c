/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <memory.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "protos.h"
#include "db_files.h"
#include "protonet.h"
#include "listd.h"
#include "error.h"
#include "lang_exchange.h"
#include "archie_dns.h"
#include "archie_inet.h"
#include "times.h"
#include "master.h"

/*
 * net.c: handle the reading or writing of commands to the network
 * connection
 */


/*
 * Read the input stream, decode the command and set up parameter vector
 */


command_v_t *read_net_command(ifp)
   FILE *ifp;	  /* command connection file pointer */

{
#if 0
#ifdef __STDC__

   extern int strncasecmp(char *, char *, int);

#else

   extern int strncasecmp();

#endif
#endif

   extern int verbose;

   static command_v_t ret_val;

   int input_index;
   net_scommand_t in_scommand;
   char *lf;

   extern command_t command_set[];

   ptr_check(ifp, FILE, "read_net_command", (command_v_t *) NULL);

   input_index = 0;
   in_scommand[0] = '\0';

   if(fgets(in_scommand, sizeof(in_scommand), ifp) == (char *) NULL){

      /* "Read EOF from network connection" */

      error(A_ERR, "read_net_command", READ_NET_COMMAND_001);
      return((command_v_t *) NULL);
   }
	    
   if((lf = strrchr(in_scommand, CR)) == (char *) NULL){

      /* "Network command not terminated with CR: %s" */

      error(A_ERR, "read_net_command", READ_NET_COMMAND_002, in_scommand);
      return((command_v_t *) NULL);
   }

   /* NULL terminate the command string, chopping off <cr><lf> */

   *lf = '\0';

   if(verbose){

      /* "Read: %s" */

      error(A_INFO, "read_net_command", READ_NET_COMMAND_003, in_scommand);
   }
      
   /* Compare it to known command strings */

   for(input_index = 0;
       (command_set[input_index].scommand != '\0') && 
       (strncasecmp(command_set[input_index].scommand,in_scommand,strlen(command_set[input_index].scommand)) != 0);
       input_index++);

   if(command_set[input_index].scommand[0] == '\0')
      input_index = C_UNKNOWN;

   switch(ret_val.command = command_set[input_index].icommand){

	/* No parameters */

	case C_QUIT:
	case C_UNKNOWN:
	case C_HEADER:
        case C_DUMPCONFIG:
	case C_VERSION:
	case C_AUTH_ERR:

	   break;

	/* One parameter */

        case C_SENDHEADER:	
        case C_TUPLELIST:
	case C_SITELIST:

	   sscanf(in_scommand,"%*s %s",ret_val.params[0]);
	   break;

        /* Three parameters */

	case C_LISTSITES:

	   sscanf(in_scommand,"%*s %s %s %s %s",ret_val.params[0], ret_val.params[1], ret_val.params[2], ret_val.params[3]);

	   break;

        /* variable number of params */
   case C_SENDEXCERPT:
   case C_SENDSITE:

	  sscanf(in_scommand,"%*s %s %s",ret_val.params[0], ret_val.params[1]);
	  break;

	default:

	   break;
	   
   }

   return(&ret_val);

}


/*
 * write_net_command: write a command with parameters (if any) to the
 * network connection
 */

status_t write_net_command(command_no, params, ofp)
   int command_no;	/* number of outgoing command */
   void *params;	/* parameters */
   FILE *ofp;		/* output command connection file pointer */

{
#ifdef __STDC__

   extern int fflush(FILE *);

#else

   extern int fflush();

#endif

   extern int verbose;

   extern command_t command_set[];

   net_scommand_t outbuf;
   outbuf[0] = '\0';

   ptr_check(ofp, FILE, "write_net_command", ERROR);

   /* params can be NULL so don't check for that here */

   switch( command_no ){

   case C_HEADER:
   case C_QUIT:
   case C_ENDDUMP:
   case C_VERSION:
   case C_AUTH_ERR:
      sprintf(outbuf,"%s",command_set[command_no].scommand);
      break;
   

   case C_ERROR:

      /* params is a pointer to char. Gives error message */

      ptr_check(params, void, "write_net_command", ERROR);

      sprintf(outbuf,"%s %s",command_set[command_no].scommand, (char *) params);
      break;

   case C_TUPLELIST:

      /* params is a pointer to int. Gives number of tuples being sent */

      ptr_check(params, void, "write_net_command", ERROR);

      sprintf(outbuf,"%s %d",command_set[command_no].scommand, *((int *) params));
      break;

   case C_SITEFILE:

      ptr_check(params, void, "write_net_command", ERROR);

      sprintf(outbuf,"%s %d",command_set[command_no].scommand, *((int *) params));
      break;

      /*
       * params is a pointer to int. It gives the port number on which
       * client can read the information
       */

    case C_SITELIST:

      ptr_check(params, void, "write_net_command", ERROR);

      sprintf(outbuf,"%s %d",command_set[command_no].scommand, *((int *) params));
      break;


    case C_LISTSITES:

      ptr_check(params, void, "write_net_command", ERROR);

      sprintf(outbuf,"%s %s %s %s %s",command_set[command_no].scommand, ((net_scommand_t *) params)[0], ((net_scommand_t *) params)[1], ((net_scommand_t *) params)[2], ((net_scommand_t *) params)[3]);
      break;

    case C_SENDEXCERPT:
    case C_SENDSITE:

      ptr_check(params, void, "write_net_command", ERROR);

      sprintf(outbuf,"%s %s %s",command_set[command_no].scommand, ((net_scommand_t *) params)[0], ((net_scommand_t *) params)[1]);

#if 0
      sprintf(outbuf,"%s %s",command_set[command_no].scommand, (char *) params);
#endif
      break;

    case C_SENDHEADER:
      ptr_check(params, void, "write_net_command", ERROR);

      sprintf(outbuf,"%s %s",command_set[command_no].scommand, (char *) params);
      break;

    default:

      /* "Unknown command %d" */

      error(A_INTERR, "write_net_command", WRITE_NET_COMMAND_002);
      return(ERROR);

   }




   /* Append <CR><LF> and send */

   /* This will have to be changed to do correct error handling */

   fprintf(ofp,"%s%c%c",outbuf,CR,LF);
   fflush(ofp);

   if(verbose){

      /* "Wrote: %s\r\n" */

      error(A_INFO, "write_net_command", WRITE_NET_COMMAND_001, outbuf);
   }


   return(A_OK);

}

/*
 * send_error: send and error return value on the command connection
 */


status_t send_error(level, ofp)
   char *level;
   FILE *ofp;

{
   ptr_check(ofp, FILE, "send_error", ERROR);

   if(write_net_command(C_ERROR, level, ofp) == ERROR){

       /* "Can't write error command on command connection" */

       error(A_ERR, "send_error", SEND_ERROR_001);
       return(ERROR);
   }

   return(A_OK);
}


/*
 * check_authorization: check to make sure that the client on the other end
 * of the connection is authorized to connect
 */


status_t check_authorization(config_info)
   udb_config_t	*config_info;

{
#if 0
#ifdef __STDC__

   extern time_t time(time_t *);
   extern getsockname(int, struct sockaddr_in *, int *);

#else

   extern time_t time();
   extern getsockname();

#endif
#endif
   
   AR_DNS *dns;
   int i, found;
   struct sockaddr_in cliaddr ;
   int addrsize;
   hostname_t my_hostname;
   ip_addr_t local_addr;
   ip_addr_t myaddr;
   struct hostent *host;

   ptr_check(config_info, udb_config_t, "check_authorization", ERROR);

   addrsize = sizeof(struct sockaddr_in);
   
   memset((char *)&cliaddr, 0, sizeof( struct sockaddr_in )) ;

   local_addr = inet_addr("127.0.0.1");

   if( getpeername( fileno(stdin), &cliaddr, &addrsize) == -1){

      /* "Can't get address of client" */

      error(A_SYSERR,"check_authorization", CHECK_AUTHORIZATION_001);
      return(ERROR);
   }

   /* If we're on the same host then OK */

   if(get_archie_hostname(my_hostname, sizeof(hostname_t)) == (char *) NULL){

      /* "Can't determine local hostname" */

      error(A_SYSERR, "check_authorization", CHECK_AUTHORIZATION_005);
      return(ERROR);
   }

   myaddr = inet_to_ipaddr(&cliaddr.sin_addr);


   if(memcmp(&myaddr, &local_addr, sizeof(myaddr)) == 0){

      /* "Connection from (local host) %s accepted at %s" */

      error(A_INFO,"check_authorization",CHECK_AUTHORIZATION_007, inet_ntoa(ipaddr_to_inet(cliaddr.sin_addr.s_addr)),cvt_to_usertime(time((time_t *) NULL),0));

      return(A_OK);
   }

   if((dns = ar_open_dns_addr(myaddr, DNS_EXTERN_ONLY, (file_info_t *) NULL)) == (AR_DNS *) NULL){

     for(i = 0, found = 0; (config_info[i].source_archie_hostname[0] != '\0') && !found; i++){

       if(config_info[i].source_archie_hostname[0] == '*'){
         /* "Connection from %s (%s) accepted at %s" */
         error(A_INFO,"check_authorization",CHECK_AUTHORIZATION_004, "unknown primary name", inet_ntoa(ipaddr_to_inet(cliaddr.sin_addr.s_addr)),cvt_to_usertime(time((time_t *) NULL),0));
         return (A_OK);
       }
     }

     if ( config_info[i].source_archie_hostname[0] == '\0' ) {
       
      /* "Can't open dns record for client at %s" */

     
      error(A_ERR, "check_authorization", CHECK_AUTHORIZATION_006, inet_ntoa(cliaddr.sin_addr));
      return(ERROR);
    }
   }

   /* Always allow connections from yourself */

   if((strncasecmp(my_hostname, get_dns_primary_name(dns), strlen(my_hostname)) == A_OK) 
      || (strncasecmp("localhost", get_dns_primary_name(dns), strlen("localhost")) == A_OK) 
      || (cmp_dns_addr(&local_addr, dns) == A_OK)){

       /* "Connection from (local host) %s accepted at %s" */

       error(A_INFO,"check_authorization",CHECK_AUTHORIZATION_007, inet_ntoa(ipaddr_to_inet(cliaddr.sin_addr.s_addr)),cvt_to_usertime(time((time_t *) NULL),0));

       ar_dns_close(dns);

       return(A_OK);

     }

   /* Check if the connection is coming from a Bunyip machine 
      in which case let through */

   if ( (host = (struct hostent*) ar_gethostbyaddr(myaddr,DNS_EXTERN_ONLY,(file_info_t*)NULL)) != NULL ) {
     char *t;
     if ( host->h_name != NULL ) {
       t = strchr(host->h_name , '.' );
       if ( t != NULL ) {
         if ( strcasecmp(t+1,"bunyip.com") == 0 )  {

           /* "Connection from %s (%s) accepted at %s" */
           error(A_INFO,"check_authorization",CHECK_AUTHORIZATION_004, get_dns_primary_name(dns), inet_ntoa(ipaddr_to_inet(cliaddr.sin_addr.s_addr)),cvt_to_usertime(time((time_t *) NULL),0));
           return(A_OK);
         }
       }
     }
   }
   
   for(i = 0, found = 0; (config_info[i].source_archie_hostname[0] != '\0') && !found; i++){

     if(config_info[i].source_archie_hostname[0] == '*'){
       found = 1;
       continue;
     }

     /* This may have to change for multi-homed hosts */

     if((dns = ar_open_dns_name( config_info[i].source_archie_hostname, DNS_EXTERN_ONLY, (file_info_t *) NULL)) == (AR_DNS *) NULL){

       /* "Unable to resolve address for %s" */
	 
       error(A_INTERR,"check_authorization", CHECK_AUTHORIZATION_002, config_info[i].source_archie_hostname);
       continue;
     }

     /* authenticate the database */

     if(cmp_dns_addr( &cliaddr.sin_addr.s_addr, dns) != ERROR){
       udb_attrib_t *attrib_ptr;
      
       for(attrib_ptr = config_info[i].db_attrib; attrib_ptr -> db_name[0] != '\0'; attrib_ptr++){

         if((attrib_ptr -> perms[0] == 'r') || (attrib_ptr -> perms[1] == 'r')){
           found = 1;
           break;
         }
       }
     }

     ar_dns_close(dns);
   }

   dns = ar_open_dns_addr(cliaddr.sin_addr.s_addr, DNS_EXTERN_ONLY, (file_info_t *) NULL);


   if(!found){

      /* "Client at %s (%s) is not authorized to connect to this server" */

      error(A_ERR,"check_authorization", CHECK_AUTHORIZATION_003,  get_dns_primary_name(dns), inet_ntoa(ipaddr_to_inet(cliaddr.sin_addr.s_addr)));

      return(ERROR);
   }

   /* "Connection from %s (%s) accepted at %s" */

   error(A_INFO,"check_authorization",CHECK_AUTHORIZATION_004, get_dns_primary_name(dns), inet_ntoa(ipaddr_to_inet(cliaddr.sin_addr.s_addr)),cvt_to_usertime(time((time_t *) NULL),0));

   ar_dns_close(dns);

   return(A_OK);

}

