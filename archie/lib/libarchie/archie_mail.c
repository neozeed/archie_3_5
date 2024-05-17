/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <stdio.h>
#include <unistd.h>
#include <varargs.h>
#include <sys/file.h>
#include "typedef.h"
#include "db_files.h"
#include "archie_mail.h"
#include "files.h"
#include "error.h"
#include "lang_libarchie.h"
#include "master.h"
#include "times.h"

/*
 * mail.c: routines for handling the archie sysadmin mail system
 */

/* write_mail: write the mail to the mail file */


void write_mail( va_alist )
   va_dcl
   /* int level;
      char *format;
      args */

{
#ifdef AIX
#ifdef __STDC__

   extern time_t time(time_t *);

#else

   extern time_t time();
#endif
#endif   

   int level;
   file_info_t *mail_file = create_finfo();
   char *format;
   char *fname;
   pathname_t enablef;
   va_list al;

   va_start( al );

   level = va_arg(al, int);

   ptr_check((format = va_arg(al, char *)), char, "write_mail",);

   sprintf(enablef, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, MAIL_RESULTS_FILE);

   /* see if the file for enabling mail exists */

   if(access(enablef, W_OK | R_OK | F_OK) == -1)
      return;

   switch(level){

      case MAIL_HOST_ADD:

	 fname = MAIL_HOST_ADD_FILE;
	 break;

      case MAIL_HOST_DELETE:
      
	 fname = MAIL_HOST_DELETE_FILE;
	 break;

      case MAIL_HOST_FAIL:
	 
	 fname = MAIL_HOST_FAIL_FILE;
	 break;

      case MAIL_HOST_SUCCESS:
	 
	 fname = MAIL_HOST_SUCCESS_FILE;
	 break;

      case MAIL_PARSE_FAIL:
	 fname = MAIL_PARSE_FAIL_FILE;
	 break;


      case MAIL_RETR_FAIL:
	 fname = MAIL_RETR_FAIL_FILE;
	 break;

      default:

	 /* "Unknown mail level %d" */

	 error(A_INTERR, "write_mail", WRITE_MAIL_005, level);
	 return;
	 break;

   }

   sprintf(mail_file -> filename, "%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, fname);

   if(open_file(mail_file, O_APPEND) == ERROR){

      /* "Can't open mail file %s" */

      error(A_ERR, "write_mail", WRITE_MAIL_001, mail_file -> filename);
      return;
   }

#if 0

   if(lockf(fileno(mail_file -> fp_or_dbm.fp), F_LOCK, 0) == -1){

      /* "Error trying to lock mail file %s" */

      error(A_SYSERR, "write_mail", WRITE_MAIL_002, mail_file -> filename);
      return;
   }

#endif
   fprintf(mail_file -> fp_or_dbm.fp, "%s ", cvt_to_usertime(time((time_t *) NULL), 1));
   vfprintf(mail_file -> fp_or_dbm.fp, format, al);
   fprintf(mail_file -> fp_or_dbm.fp, "\n");

#if 0
   if(lockf(fileno(mail_file -> fp_or_dbm.fp), F_ULOCK, 0) == -1){

      /* "Error trying to unlock mail file %s" */

      error(A_SYSERR, "write_mail", WRITE_MAIL_003, mail_file -> filename);
      return;
   }
#endif

   if(close_file(mail_file) == ERROR){

      /* "Can't close mail file %s" */

      error(A_ERR, "write_mail", WRITE_MAIL_004, mail_file -> filename);
      return;
   }

   va_end( al );

   return;
}
