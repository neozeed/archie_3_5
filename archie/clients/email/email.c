/*
 * This file is copyright Bunyip Information Systems Inc., 1994. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "protos.h"
#include "typedef.h"
#include "db_files.h"
#include "files.h"
#include "master.h"
#include "email.h"
#include "error.h"
#include "lang_email.h"

/* aremail: archie email clien


   argv, argc are used.


   Parameters:	  -
		  
*/

char *prog;
static int verbose = 0;


int main(argc, argv)
   int argc;
   char *argv[];

{
   extern int opterr;
   extern char *optarg;
#if 0
#ifdef __STDC__

   extern int getopt(int, char **, char *);

#else

   extern int getopt();

#endif
#endif
   char **cmdline_ptr;
   int cmdline_args;

   int option;

   pathname_t tmp_dir;

   pathname_t telnet_pgm;
   pathname_t master_database_dir;
   pathname_t host_database_dir;

   pathname_t logfile;

   int logging = 1;

   int log_address = 0;

   file_info_t *input_file = create_finfo();

   prog = argv[0];
   opterr = 0;


   cmdline_ptr = argv + 1;
   cmdline_args = argc - 1;

   telnet_pgm[0] = tmp_dir[0] = '\0';
   logfile[0] = '\0';

   host_database_dir[0] = master_database_dir[0] = '\0';

   while((option = (int) getopt(argc, argv, "i:t:T:L:luvM:h:")) != EOF){

      switch(option){

	 /* Log filename */

	 case 'L':
            strcpy(logfile, optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;


	 /* master database directory name */

	 case 'M':
	    strcpy(master_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* telnet program name */

	 case 'T':
	    strcpy(telnet_pgm,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* host database directory name */

	 case 'h':
	    strcpy(host_database_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* input file */

	 case 'i':
	    strcpy(input_file -> filename, optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 /* log output, default file */

	 case 'l':
	    logging = 0;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

	 /* temporary directory name */

	 case 't':
	    strcpy(tmp_dir,optarg);
	    cmdline_ptr += 2;
	    cmdline_args -= 2;
	    break;

	 case 'u':
	    log_address = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

	 case 'v':
	    verbose = 1;
	    cmdline_ptr++;
	    cmdline_args--;
	    break;

	 default:
	    error(A_ERR, "email-client", "Usage: email-client [ -T <telnet client> ] [ -u ] [ - v ] [ -t <tmp dir> ] [ -i <input file> ] [ - l ] [ -L <logfile> ]");
	    exit(ERROR);
	    break;
      }

   }


   /* set up logs */

   if(logging){

      if(logfile[0] == '\0'){

	 sprintf(logfile, "%s/%s/%s", get_archie_home(), DEFAULT_LOG_DIR, EMAIL_LOG_FILE);
      }

      if(open_alog(logfile, A_INFO, tail(argv[0])) == ERROR)
	 exit(ERROR);
   }

   if(verbose)
      error(A_INFO, "email-client", "Setting working directory to %s", get_archie_home());

   /* change working directory */

   if(chdir(get_archie_home()) == -1){

      /* "Can't change working directory to %s" */

      error(A_SYSERR, "email-client", EMAIL_005, get_archie_home());
      exit(ERROR);
   }

   if(set_master_db_dir(master_database_dir) == (char *) NULL){

      error(A_ERR,"email-client", "Error while trying to set master database directory");
      exit(ERROR);
   }


   /* setup default tmp directory if not given */

   if(tmp_dir[0] == '\0')
      sprintf(tmp_dir,"%s/%s", get_master_db_dir(), DEFAULT_TMP_DIR);

   if(access(tmp_dir, R_OK | W_OK | X_OK | F_OK) == -1){

      /* "Can't access temporary directory %s" */

      error(A_SYSERR, "email-client", EMAIL_001, tmp_dir);
      exit(ERROR);
   }

   if(verbose)
      error(A_INFO, "email-client", "Set temp directory to %s", tmp_dir);

   /* open the input file */

   if(input_file -> filename[0] != '\0'){

      if(open_file(input_file, O_RDONLY) == ERROR){

         /* "Can't open given input file %s" */

	 error(A_ERR, "email-client", EMAIL_002, input_file -> filename);
	 exit(ERROR);
      }

      if(verbose)
	 error(A_INFO, "email-client", "Reading input from %s", input_file -> filename);

   }
   else{

      /* The input comes from stdin */

      if(verbose)
	 error(A_INFO, "email-client", "Reading input from stdin");

      input_file -> fp_or_dbm.fp = stdin;
      saveinput(input_file, tmp_dir);
   }


   if(process_input(input_file,telnet_pgm, log_address, logging) == ERROR){

      /* "Error processing input" */

      error(A_ERR, "email-client", EMAIL_004);

      exit(ERROR);
   }
   else{

#if 0
      if(verbose)
	 error(A_INFO, "email-client", "Unlinking input file %s", input_file -> filename);

      if(unlink(input_file -> filename) == 1){

	 /* "Can't unlink input file %s" */

	 error(A_ERR, "email-client", EMAIL_003, input_file -> filename);
	 exit(ERROR);
      }
#endif

   }

   exit(A_OK);
   return(A_OK);
   
}

/* saveinput: save the incoming input into temporary file */

status_t saveinput(input_file,tmp_dir)
   file_info_t *input_file;
   char *tmp_dir;

{

   pathname_t line;
   file_info_t *tmp_file = create_finfo();

   sprintf(tmp_file -> filename, get_tmp_filename(tmp_dir));


   /* Make sure the file doesn't already exist by unlinking it */

   unlink(tmp_file -> filename);

   /* Open the file */

   if(open_file(tmp_file, O_WRONLY) == -1){

      /* "Can't open temporary file %s" */

      error(A_ERR, "saveinput", SAVEINPUT_001, tmp_file -> filename);
      return(ERROR);
   }

   /* Unlink it, the file descriptor remains valid until closed */

   unlink(tmp_file -> filename);

   if(verbose)
      error(A_INFO, "saveinput", "Copying input from stdin to %s", tmp_file -> filename);

   /* For the process for copying */

   while (1){
      if(fgets(line,sizeof(pathname_t), stdin) != line)
	    break;
      fputs(line, tmp_file -> fp_or_dbm.fp);
   }

   fflush(tmp_file -> fp_or_dbm.fp);

   rewind(tmp_file -> fp_or_dbm.fp);

   strcpy(input_file -> filename, tmp_file -> filename);

   input_file -> fp_or_dbm.fp = tmp_file -> fp_or_dbm.fp;

   return(A_OK);
}


#define ISLINE(hdr,readfrom) (!strncasecmp((readfrom),(hdr),sizeof(hdr)-1))

#define DOLINE(hdr,var,readfrom) do {if (ISLINE(hdr,readfrom)) strcpy((var),&(readfrom)[sizeof(hdr)-1]); } while (0) 
   

status_t process_input(input_file, telnet_pgm, log_address, logging)
   file_info_t *input_file;
   char *telnet_pgm;
   int log_address;
   int logging;
{
   pathname_t line;
   pathname_t subject_line;
   pathname_t envfrom, from, retpath, reply;
   pathname_t addr_copy;
   char *address;
   char *c, *d;
   int p[2];
   int not_empty = 0;
   pathname_t inp_line;
   int retcode;
   int telnet_quit;
   int status;
   char **arglist;
   int i,counter = 0;

   subject_line[0] = '\0';

   envfrom[0] = from[0] = retpath[0] = reply[0] = '\0';

   while (readline(input_file,line) == A_OK){

      if(line[0] == '\n')
	 break;

      DOLINE("from ", envfrom, line);
      DOLINE("from: ", from, line);
      DOLINE("return-path: ", retpath, line);
      DOLINE("reply-to: ", reply, line);

      if (ISLINE("subject: ",line)){

	 for(c= &line[sizeof("subject: ") - 1];isspace(*c);c++);
	 strcpy(subject_line, c);
	 not_empty = 1;
      }
   }

   /* Choose the correct return address */

   if(reply[0])
      address = &reply[0];
   else if(retpath[0])
      address = retpath;
   else if(from[0])
      address = &from[0];
   else if(envfrom[0])
      address = &envfrom[0];
   else
      return(ERROR);

   addr_copy[0] = '\0';

   for(c = address, d = addr_copy; *c != (char) '\0'; c++){

      if(*c == '"'){
	 *d++ = '\\';
	 *d++ = '"';
      }
      else
	 *d++ = *c;
   }

   *d = '\0';

   strcpy(address, addr_copy);

   if((c = strchr(address, '\n')))
      *c = '\0';

   if(log_address)
      error(A_INFO, "process_input", "From: %s", address);

   if(telnet_pgm[0] == '\0')
      sprintf(telnet_pgm, "%s/%s/%s", get_archie_home(), DEFAULT_BIN_DIR, TELNET_PGM_NAME);

   if((arglist = (char **) malloc(MAX_NO_PARAMS * sizeof(char *))) == (char **) NULL){

      /* "Can't malloc space for argument list" */

      error(A_SYSERR, "process_input", PROCESS_INPUT_017);
      return(ERROR);
   }

   /* set up argument list */

   i = 0;
   arglist[i++] = telnet_pgm;

   arglist[i++] = "-e";

   if(logging){

      arglist[i++] = "-L";
      arglist[i++] = get_archie_logname();
   }

   arglist[i] = (char *) NULL;

   if(pipe(p) == -1){

      /* "Can't open pipe to telnet server process" */

      error(A_SYSERR, "process_input", PROCESS_INPUT_001);
      goto on_error;
   }

   if(verbose)
      error(A_INFO, "process_input", "Starting telnet client %s", telnet_pgm);

   if((retcode = fork()) == 0){

      if(dup2(p[0], 0) == -1){

	 /* "Can't dup2 pipe to stdin" */

	 error(A_SYSERR, "process_input", PROCESS_INPUT_002);
	 exit(ERROR);
      }

      if(verbose)
	 error(A_INFO, "process_input", "Started telnet client %s", telnet_pgm);

      execvp(telnet_pgm, arglist);

      /* "Can't execvp the telnet server %s" */

      error(A_SYSERR, "process_input", PROCESS_INPUT_003, telnet_pgm);
      exit(ERROR);
   }

   if(verbose)
      error(A_INFO, "process_input", "Writing path command (%s) to telnet-client", address);

   sprintf(inp_line, "%s %s\n", PATH_COMMAND, address);

   if(write(p[1], inp_line, strlen(inp_line)) == -1){

      /* "Can't write initial path to telnet client" */

      error(A_SYSERR, "process_input", PROCESS_INPUT_004);
      goto on_error;
   }

   /* Make sure that the telnet client hasn't died on us here */

   status = 0;

   if((telnet_quit = waitpid(-1, &status, WNOHANG)) == -1){

      /* "Erorr in wait3() for telnet client" */

      error(A_SYSERR, "process_input", PROCESS_INPUT_006);
      goto on_error;
   }

   if((telnet_quit != 0) && (telnet_quit != retcode)){

      /* "Child returned from wait3() (pid: %d) not same as forked (pid: %d)" */

      error(A_INTERR, "process_input", PROCESS_INPUT_018, telnet_quit, retcode);
   }

   if(telnet_quit){

      if(WIFEXITED(status) && WEXITSTATUS(status)){

	 /* "telnet client %s exited abnormally with value %d" */

	 error(A_ERR, "process_input", PROCESS_INPUT_007, telnet_pgm, WEXITSTATUS(status));
	 goto on_error;
      }

      if(WIFSIGNALED(status)){

	 /* "telnet program %s terminated abnormally with signal %d" */

	 error(A_ERR,"process_input", PROCESS_INPUT_008, telnet_pgm, WTERMSIG(status));
	 goto on_error;
      }
   }
   
   if(verbose)
      error(A_INFO, "process_input", "Writing subject line '%s'", subject_line);


   if(write(p[1], subject_line, strlen(subject_line)) == -1){

      /* "Can't write subject line command to telnet client" */

      error(A_SYSERR, "process_input", PROCESS_INPUT_005);
      goto on_error;
   }

   /* process the mail, line by line */

   status = 0;

   do{
     counter++;
      if((telnet_quit = waitpid(-1, &status, WNOHANG)) == -1){

	 /* "Erorr in wait3() for telnet client" */

	 error(A_SYSERR, "process_input", PROCESS_INPUT_006);
      }

      if(telnet_quit){

	 if(WIFEXITED(status) && WEXITSTATUS(status)){

	    /* "telnet client %s exited abnormally with value %d" */

	    error(A_ERR, "process_input", PROCESS_INPUT_007, telnet_pgm, WEXITSTATUS(status));

	    goto on_error;
	 }

	 if(WIFSIGNALED(status)){

	    /* "telnet program %s terminated abnormally with signal %d" */

	    error(A_ERR,"process_input", PROCESS_INPUT_008, telnet_pgm, WTERMSIG(status));
	    goto on_error;
	 }

	 goto on_error;
      }

      if((telnet_quit != 0) && (telnet_quit != retcode)){

	 /* "Child returned from wait3() (pid: %d) not same as forked (pid: %d)" */

	 error(A_INTERR, "process_input", PROCESS_INPUT_018, telnet_quit, retcode);
      }

      if(verbose)
	 error(A_INFO, "process_input", "Writing line '%s'", line);

      if(telnet_quit)
	 break;
	 
      /* empty line */

      if(sscanf(line, " %[a-zA-Z]*", inp_line) != 1)
	 continue;

      not_empty = 1;

      if(write(p[1], line, strlen(line)) == -1){

	 /* "Can't write line %s to telnet client" */

	 error(A_SYSERR, "process_input", PROCESS_INPUT_014, line);
	 goto on_error;
      }

   }while(readline(input_file, line) == A_OK && counter < 50 );

   if(!telnet_quit){

      if(verbose)
	 error(A_INFO, "process_input", "Mail not terminated with 'quit'. Writing 'quit'");

      sprintf(inp_line, "%s\n", QUIT_COMMAND);

      /* Mail has ended without a "quit" */

      if(write(p[1], inp_line, strlen(inp_line)) == -1){

	 /* "Can't write quit command to telnet client" */

	 error(A_SYSERR, "process_input", PROCESS_INPUT_013);
	 goto on_error;

      }
   
      if(close(p[1]) == -1)
         error(A_SYSERR, "process_input", "Can't close pipe to telnet-client");

      if(close(p[0]) == -1)
         error(A_SYSERR, "process_input", "Can't close pipe to telnet-client");

      if((telnet_quit = wait(&status)) == -1){

	 /* "Erorr in wait for telnet client" */

	 error(A_SYSERR, "process_input", PROCESS_INPUT_009);
	 goto on_error;
      }

      if(telnet_quit){

	 if(WIFEXITED(status) && WEXITSTATUS(status)){

	    /* "telnet client %s exited abnormally with value %d" */

	    error(A_ERR, "process_input", PROCESS_INPUT_007, telnet_pgm, WEXITSTATUS(status));

	    goto on_error;
	 }

	 if(WIFSIGNALED(status)){

	    /* "telnet program %s terminated abnormally with signal %d" */

	    error(A_ERR,"process_input", PROCESS_INPUT_008, telnet_pgm, WTERMSIG(status));
	    goto on_error;
	 }

      }

      if(telnet_quit != retcode){

	 /* "Child returned from wait3() (pid: %d) not same as forked (pid: %d)" */

	 error(A_INTERR, "process_input", PROCESS_INPUT_018, telnet_quit, retcode);
      }
      
   }
   else{

      if(verbose)
	 error(A_INFO, "process_input", "telnet-client quit");

      if(close(p[1]) == -1)
	 error(A_SYSERR, "process_input", "Can't close pipe to telnet-client");

      if(close(p[0]) == -1)
	 error(A_SYSERR, "process_input", "Can't close pipe to telnet-client");
   }


   close_file(input_file);

   if(log_address)
      error(A_INFO, "process_input", "Request from: %s completed OK", address);

   return(A_OK);

on_error:

#if 0
   do_error_reply(input_file, address);
#endif

   if(log_address)
      error(A_INFO, "process_input", "Request from: %s completed with errors", address);

   return(ERROR);
   
}


status_t readline(input_file, line)
   file_info_t *input_file;
   char *line;
{
   int i;

   if(fgets(line,sizeof(pathname_t), input_file -> fp_or_dbm.fp) != line)
      return(ERROR);

   i = strlen(line) - 1;

   if (i < 0)
      return(ERROR);

#if 0
   while ((i>=0) && isascii(line[i]) && isspace(line[i])) i --;

   line[i+1] = '\0';
#endif

   return(A_OK);
}
   

   
#if 0
status_t open_mailserv(mail_host, mail_port, mail_conn)
   char *mail_host;	/* host to connect to */
   char *mail_port;	/* port (service) to connect to */
   fd   *mail_conn;	/* returned file pointer */

{
   int portno;

   struct servent *server_ent;

   ptr_check(mail_host, char, "open_mailserv", ERROR);
   ptr_check(mail_port, char, "open_mailserv", ERROR);
   ptr_check(mail_conn, int, "open_mailserv", ERROR);

   if(mail_host[0] == '\0'){
      char *tmp_ptr;

      if((tmp_ptr = get_var(V_MAIL_HOST)) == '\0'){

	 /* "No mail host given" */

	 error(A_ERR, "open_mailserv", OPEN_MAILSERV_001);
	 return(ERROR);
      }
   
     strcpy(mail_host, tmp_ptr);

   }

   if(mail_port[0] == '\0'){
      char *tmp_port;

      if((tmp_port = get_var(V_MAIL_SERVICE)) == '\0'){

	 /* "No mail port (service) given" */

	 erorr(A_ERR, "open_mailserv", OPEN_MAILSERV_002);
	 return(ERROR);
      }

      strcpy(mail_port, tmp_port);
   }

   
   if(sscanf(mail_port, "%d", &portno) != 1){

      if((server_ent = getservbyname(mail_port, "tcp")) == (struct servent *) NULL){

	 /* "Unknown service %s" */

	 error(A_ERR, "open_mailserv", mail_port);
	 return(ERROR);
      }

      portno = server_ent -> s_port;
   }

   if(cliconnect(mail_host, portno, mail_conn) != A_OK){

      /* "Can't open connection to mailserver %s port %d" */

      error(A_ERR, "open_mailserv", OPEN_MAILSERV_004, mail_host, portno);
      return(ERROR);
   }

   return(A_OK);
}

#endif
