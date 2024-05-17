/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <varargs.h>
#include <arpa/telnet.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#ifdef AIX
#include <sys/select.h>
#endif
#include <unistd.h>
#include "protos.h"
#include "typedef.h"
#include "error.h"
#include "ftp.h"
#include "lang_retrieve.h"

/* ftp.c: low level ftp routines */

   
static pathname_t reply_string;
static pathname_t out_string;



/*
 * get_reply_string: return the last reply string from the ftp session
 */

extern char *prog;


char *get_reply_string()

{
   static char rep_str[BUFSIZ];

   strcpy(rep_str, reply_string);
   rep_str[strlen(rep_str) - 1] = '\0';

   return(rep_str);
}

/*
 * get_request_string: return the last command to the remote ftp server
 */

char *get_request_string()

{
   static char req_str[BUFSIZ];

   strcpy(req_str, out_string);

   return(req_str);
}

void put_request_string(s)
   char *s;
{

   strcpy(out_string, s);
}

void put_reply_string(s)
   char *s;
{

   strcpy(reply_string, s);
}


/*
 * ftp_connect: connect to the given hostname on the port and return two
 * file pointers to the input and output streams
 */


con_status_t ftp_connect(hostname, port, command_in, command_out, timeout)
   hostname_t hostname;	   /* host name to connect to */
   int port;		   /* port number to connect to */
   FILE **command_in;	   /* returned input file pointer */
   FILE **command_out;	   /* returned output file pointer */
   int timeout;

{
#ifdef __STDC__

   extern int fclose(FILE *);

#else

   extern int fclose();
   extern setsockopt();

#endif


   extern int verbose;
   int command_fd;
   int on = 1;
   int retcode;

   if(verbose){

     /* "Trying to connect to %s on port %u"  */

      error(A_INFO,"ftp_connect", FTP_CONNECT_001, hostname, port);
   }

   if((retcode = cliconnect(hostname, port, &command_fd)) != A_OK)
      return(retcode);

   if(verbose){

      /* "Connected to %s" */

      error(A_INFO,"ftp_connect", FTP_CONNECT_002, hostname);
   }

   if((*command_in = fdopen(command_fd, "r")) == (FILE *) NULL){

      /* "Can't open file I/O for incoming connection" */

      error(A_ERR,"ftp_connect", FTP_CONNECT_003);
      return(ERROR);
  }
  
   if((*command_out = fdopen(command_fd, "w")) == (FILE *) NULL){

      /* "Can't open file I/O for outgoing connection" */

      error(A_ERR,"ftp_connect", FTP_CONNECT_004);

      if(command_in)
	 fclose(*command_in);
	
      return(ERROR);
  }

#ifdef SO_OOBINLINE

   if(setsockopt(command_fd, SOL_SOCKET, SO_OOBINLINE, (char *) &on, sizeof(on)) == -1){


      /* "Can't set socket options" */

      error(A_SYSERR, "ftp_connect", FTP_CONNECT_005);
      return(CON_SOCKETFAILED);
   }
      
#endif

   /* get the login message */

   if(get_reply(*command_in, *command_out, &retcode,0, timeout) == ERROR)
      return(retcode);

   return(A_OK);

}


/*
 * ftp_login: perform the login sequence to the remote ftp server
 */


int ftp_login(comm_in, comm_out, user, pass, acct, timeout)
   FILE *comm_in;
   FILE *comm_out;
   char *user;
   char *pass;
   char *acct;
   int timeout;

{
   int retcode;

   retcode = send_command(comm_in, comm_out,0,timeout,"USER %s", user);

   if(retcode == FTP_USER_OK){ /* User name ok, need password */

      if((retcode = send_command(comm_in, comm_out,0, timeout, "PASS %s", pass)) == FTP_LOGIN_OK){

	 /* User logged in OK */

	 return(A_OK);
      }
      else{

	 if(retcode == FTP_ACCT_WANTED){ /* Need "ACCT" name */

            if((retcode = send_command(comm_in, comm_out,0, timeout, "ACCT %s", acct)) == FTP_LOGIN_OK){
	 
	       /* User logged in OK */

	       return(A_OK);
	    }
	    else{

	       /* "ACCT command '%s' not accepted" */

	       /* error(A_ERR,"ftp_login", FTP_LOGIN_001, acct); */
	       return(retcode);  /* ACCT not accepted */
	    }
	 }
	 else{

	    /* "FTP password '%s' not accepted" */

	    /* error(A_ERR,"ftp_login", FTP_LOGIN_002, pass); */
	    return(retcode);	/* Login incorrect */
	 }
      }
   }
   else
      if(retcode == FTP_LOGIN_OK)  /* User logged in without password */
	 return(A_OK);
      else if(retcode == FTP_NOT_LOGGED_IN){

	 /* "Login not accepted. Service not currently available" */

	 /* error(A_ERR,"ftp_login", FTP_LOGIN_003, retcode); */
	 return(retcode);
      }else{

	 /* "Login not accepted. FTP code %u" */

	 /* error(A_ERR,"ftp_login", FTP_LOGIN_004, retcode); */
      	 return(retcode);
      }

}

/*
 * send_command: send an ftp command. varargs
 */


int send_command(va_alist)
   va_dcl
/* FILE *comm_in;
   FILE *comm_out;
   int  expecteof;
   int  timeout;
   char *format;
   arglist */

{
   extern int vfprintf();
   extern int vsprintf();

#ifdef __STDC__

   extern int fflush(FILE *);

#else

   extern int fflush();

#endif

   extern int verbose;

   FILE *comm_in;
   FILE *comm_out;
   va_list al;
   char *format;
   int retcode;
   char outformat[BUFSIZ];
   int expecteof;
   int timeout;


   va_start(al);

   comm_in = va_arg(al, FILE *);
   comm_out = va_arg(al, FILE *);
   expecteof = va_arg(al, int);
   timeout = va_arg(al, int);
   format = va_arg(al, char *);

   sprintf(outformat,"%s\r\n", format);

   vfprintf(comm_out, outformat, al);

   vsprintf(out_string, format, al);

   if(verbose)
      error(A_INFO, "send_command","%s", out_string);

      
   fflush(comm_out);

   va_end(al);

   if(get_reply(comm_in, comm_out, &retcode,0, timeout) == ERROR)
      return(-1);
   else
      return(retcode);

}

/*
 * get_reply: read the reply from the remote ftp daemon
 */


status_t get_reply(comm_in, comm_out, retcode, expecteof, timeout)
    FILE *comm_in;	/* input file pointer */
    FILE *comm_out;	/* output file pointer */
    int *retcode;	/* returned pointer to ftp return code */
    int expecteof;	/* non-zero if EOF condition expected on next read from remote daemon */
    int timeout;	/* Time to wait for reply */

{
   extern void sig_handle();
   extern int signal_set;

   extern int verbose;
   int c, n;
   int code;
   char *cp;
   int dig;
   int continuation = 0;
   int originalcode = 0;

#if 0
   fd_set read_set;
   int select_code;
   struct timeval timev_struct;

   timev_struct.tv_sec = timeout;
   timev_struct.tv_usec = 0L;
   
   FD_ZERO(&read_set);
   FD_SET(fileno(comm_in),&read_set);

   if((select_code = select(FD_SETSIZE, &read_set, (fd_set *) NULL, (fd_set *) NULL, &timev_struct)) < 0){

      /* "Error in select() while waiting for reply from remote server" */

      error(A_SYSERR, "get_reply", GET_REPLY_001);
      *retcode = FTP_LOST_CONN;
      return(ERROR);
   }
   else{
      /* timeout */

      if(select_code == 0){

	 /* "Timed out while waiting for reply from remote ftp server" */

	 error(A_ERR, "get_reply", GET_REPLY_002);
	 put_reply_string(GET_REPLY_002);
	 *retcode = FTP_LOST_CONN;
	 return(ERROR);
      }
   }
#endif

#ifndef SOLARIS
   /* 
    * Don't restart the system call (a read) if we are interrupted by
    * sigalarm. This is the default behaviour under Solaris, so we don't
    * need this call
    */  
   siginterrupt(SIGALRM, 1);
#endif

   signal_set = 0;           
   signal(SIGALRM, sig_handle);
   alarm(timeout);

   c = getc(comm_in);

   if(signal_set){
   
      /* "Timed out while waiting for reply from remote ftp server" */

      error(A_ERR, "get_reply", GET_REPLY_002);
      put_reply_string(GET_REPLY_002);
      *retcode = FTP_LOST_CONN;
      return(ERROR);
   }

   alarm(0);
   signal(SIGALRM, SIG_DFL);


#ifndef SOLARIS
   siginterrupt(SIGALRM, 0);
#endif

   ungetc(c, comm_in);

   while(1){

      dig = n = code = 0;
      cp = reply_string;

      while((c = getc(comm_in)) != '\n'){

         if(c == IAC){     /* handle telnet comms */

            switch (c = getc(comm_in)) {

	       case WILL:
	       case WONT:

	          c = getc(comm_in);
		  fprintf(comm_out, "%c%c%c",IAC,DONT,c);
		  fflush(comm_out);
		  break;

	       case DO:
	       case DONT:

	          c = getc(comm_in);
		  fprintf(comm_out, "%c%c%c",IAC,WONT,c);
		  fflush(comm_out);
		  break;

	       default:
	          break;
	    }

	    continue;

	 }

         dig++;

         if(c == EOF){

             if(expecteof){
	        *retcode = FTP_QUIT;
                return (A_OK);
	     }

	     *retcode = FTP_LOST_CONN;
	     return(ERROR);
	 }

         if(dig < 4 && isdigit(c))
	   code = code * 10 + (c - '0');

         if(dig == 4 && c == '-'){
	   if(continuation)
	      code = 0;
	      continuation++;
	 }

	 if(n == 0)
	   n = c;

	 if(cp < &reply_string[sizeof(reply_string) - 1])
	    *cp++ = c;

      }

      if(continuation && (code != originalcode)){
         if(originalcode == 0)
	    originalcode = code;
     	    continue;
      }

      *cp = '\0';

      if(verbose)
         error(A_INFO,"get_reply","%s", reply_string);

      if(code == FTP_LOST_CONN || originalcode == FTP_LOST_CONN){
	 return(ERROR);
      }
      
      *retcode = code;
      return(A_OK);
   }
   
}
	       
