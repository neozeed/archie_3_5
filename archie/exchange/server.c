#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include <memory.h>
#include "protos.h"
#include "typedef.h"
#include "header.h"
#include "db_ops.h"
#include "tuple.h"
#include "listd.h"
#include "error.h"
#include "lang_exchange.h"
#include "times.h"
#include "archie_strings.h"

/*
 * do_server: invoked when the process is being run in server mode
 */


status_t do_server(hostbyaddr, hostdb, hostaux_db, domaindb, config_info, timeout, compress_trans)
   file_info_t *hostbyaddr;	 /* host address cache */
   file_info_t *hostdb;		 /* primary host database */
   file_info_t *hostaux_db;	 /* auxiliary host database */
   file_info_t *domaindb;	 /* domain database */
   udb_config_t *config_info;	 /* configuration information */
   int timeout;
   int compress_trans;		 /* compress transferred data */
{

   extern char *ar_version();

#ifdef __STDC__

   extern time_t time(time_t *);

#else

   extern time_t time();

#endif

   command_v_t *command_v;			/* command vector */

   tuple_t **tuple_list;			/* points to list of tuples */
   int tuple_count;

   header_t header_rec;
   udb_config_t *config_ptr;
   udb_attrib_t *attrib_ptr;
   

   int finished = FALSE;

   ptr_check(hostbyaddr, file_info_t, "do_server", ERROR);
   ptr_check(hostdb, file_info_t, "do_server", ERROR);
   ptr_check(hostaux_db, file_info_t, "do_server", ERROR);
   ptr_check(domaindb, file_info_t, "do_server", ERROR);
   

   setup_signals();

   while(!finished){

      if((command_v = read_net_command(stdin)) == (command_v_t *) NULL){
	 /* error or EOF on read */

	 /* "Error while trying to read input command from command connection" */

	 error(A_ERR, "do_server", DO_SERVER_001);
         return(ERROR);
      }

      switch(command_v -> command){

	case  C_UNKNOWN:
	   break;

	 case C_QUIT:

	    /* "Connection closed at %s UTC" */

	    error(A_INFO,"do_server", DO_SERVER_002, cvt_to_usertime(time((time_t *) NULL),0));
	    return(A_OK);
	    break;

         case C_LISTSITES:

	/*
	 * First parameter of LISTSITES command is <db> (database of query)
	 * Second parameter is <from date> convert it into a date_time_t.
	 * Also convert second parameter into colon separated list of
	 * domain names to search in and call compose_tuples() Note: at this
	 * time cvt_to_domainlist is a macro defined in host_db.h
	 */

	/* ******* Need to verify all parameters here ***********/

	if((tuple_list = compose_tuples(command_v -> params[0],
				    command_v -> params[1],
			            cvt_to_inttime(command_v -> params[2],0),
				    cvt_to_domainlist(command_v -> params[3]),
				    hostbyaddr,
				    hostdb,
				    domaindb,
				    hostaux_db,
				    &tuple_count)) == (tuple_t **) NULL){

	   send_error(DO_SERVER_003, stdout);

	   /* "Can't compose tuples" */

	   error(A_FATAL,"do_server", DO_SERVER_003);
	   continue;
	}


	/* Write response back to client */

	if(send_tuples(tuple_list, &tuple_count, stdout) != A_OK){

	   send_error(DO_SERVER_004, stdout); 

	   /* "Can't send tuples" */

	   error(A_FATAL,"do_server", DO_SERVER_004);
	   continue;
	}



	break;

      case C_SENDHEADER:

	 if(send_header(command_v -> params[0], &header_rec, hostdb, hostaux_db) == ERROR){
	    pathname_t err_msg;

	    /* "Unable to send header to client" */

	    error(A_ERR, "do_server", DO_SERVER_005);

	    sprintf(err_msg, "No such site/database combination %s",  command_v -> params[0]);

	    send_error(err_msg, stdout);
	 }
	 else{

	    if(write_net_command(C_HEADER, (void *) NULL, stdout) == ERROR){

	       /* "Unable to write C_HEADER command to client" */

	       error(A_ERR, "do_server", DO_SERVER_006);
	    }
	    else{

	       if(write_header(stdout, &header_rec, (u32 *) NULL, 1, 0) == ERROR){

		  /* "Unable to write header to client" */

		  error(A_ERR, "do_server", DO_SERVER_007);
	       }
	    }
	 }


	 break;
	 
      case C_SENDSITE:

	 if(command_v -> params[1]){

	    /* Check for forced compression (or uncompression) */

	    if(strcasecmp((char *) command_v -> params[1], "compress") == 0)
	       compress_trans = 1;
	    else{
	       if(strcasecmp((char *) command_v -> params[1], "uncompress") == 0){

		  if(compress_trans){

		     /* "Remote server has forced uncompressed transmission" */
		     error(A_INFO, "do_server", DO_SERVER_010);
		  }
		  compress_trans = 0;
	       }
	    }
	 }

	 if(sendsite((void *) command_v -> params[0], hostdb, hostaux_db, timeout, compress_trans) == ERROR){

	    /* "Unable to send site to remote client" */

	    error(A_ERR, "do_server", DO_SERVER_008);
	    send_error(DO_SERVER_008, stdout);
	 }
	 
	 break;

   case C_SENDEXCERPT:

	 if(command_v -> params[1]){

	    /* Check for forced compression (or uncompression) */

	    if(strcasecmp((char *) command_v -> params[1], "compress") == 0)
	       compress_trans = 1;
	    else{
	       if(strcasecmp((char *) command_v -> params[1], "uncompress") == 0){

		  if(compress_trans){

		     /* "Remote server has forced uncompressed transmission" */
		     error(A_INFO, "do_server", DO_SERVER_010);
		  }
		  compress_trans = 0;
	       }
	    }
	 }

	 if(sendexcerpt((void *) command_v -> params[0], hostdb, hostaux_db, timeout, compress_trans) == ERROR){

	    /* "Unable to send site to remote client" */

	    error(A_ERR, "do_server", DO_SERVER_008);
	    send_error(DO_SERVER_008, stdout);
	 }
	 
	 break;

      case C_DUMPCONFIG:

	 for(config_ptr = config_info; config_ptr -> source_archie_hostname[0] != '\0'; config_ptr++){

	    for(attrib_ptr = config_ptr -> db_attrib; attrib_ptr -> db_name[0] != '\0'; attrib_ptr++){

	       fprintf(stdout, "%s;%s;%s;%d;%s;%ld;%s;%d\r\n",
		      config_ptr -> source_archie_hostname,
		      attrib_ptr -> db_name,
		      attrib_ptr -> domains,
		      attrib_ptr -> maxno,
		      attrib_ptr -> perms,
		      attrib_ptr -> update_freq,
		      cvt_to_usertime(attrib_ptr -> update_time, 0),
		      attrib_ptr -> fails);
	    }
	 }

	 if(write_net_command(C_ENDDUMP, (void *) NULL, stdout)== ERROR){

	    /* "Unable to write requested dump to client" */

	    error(A_ERR, "do_server", DO_SERVER_009);
	    continue;
	 }

	 break;

      case C_VERSION:

	 fprintf(stdout, "%s\r\n", ar_version());
	 break;

      }


   }/* while */

   return(A_OK);
}

/*
 * die: signal handler. If you receive any of the caught signals then exit
 * with a message
 */


void die(sig, code, scp, addr)
   int sig, code;
   struct sigcontext *scp;
   char *addr;

{
   /* "Signal %d received. Exiting" */

   error(A_FATAL,"die", DIE_001, sig);
   exit(ERROR);
}

/*
 * setup_signals: catch the following signals
 */


void setup_signals()

{
   signal(SIGBUS, die);
   signal(SIGSEGV, die);
}

/*
 * send_tuples: send the given tuples to the remote server
 */


status_t send_tuples(tuple_list, tuple_count, ofp)
   tuple_t **tuple_list;   /* list of tuples to be sent */
   int *tuple_count;	   /* number of tuples */
   FILE *ofp;		   /* file descriptor of remote connection */

{
   int count;

   ptr_check(tuple_list, tuple_t*, "send_tuples", ERROR);
   ptr_check(tuple_count, int, "send_tuples", ERROR);
   ptr_check(ofp, FILE, "send_tuples", ERROR);
   

   if(write_net_command(C_TUPLELIST, tuple_count, ofp) == ERROR){

      /* "Error while trying to send C_TUPLELIST command to remote site" */

      error(A_ERR, "send_tuples", SEND_TUPLES_001);
      return(ERROR);
   }

   for(count = 0; count < *tuple_count; count++){

       fprintf(stdout,"%s%c%c", *tuple_list[count], CR, LF);
       if(tuple_list)
          free(tuple_list[count]);
   }

   tuple_list[*tuple_count] = (tuple_t *) NULL;
   free_opts((char **) tuple_list);

   return(A_OK);
}

