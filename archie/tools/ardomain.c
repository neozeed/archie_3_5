/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <memory.h>
#include "typedef.h"
#include "db_files.h"
#include "host_db.h"
#include "domain.h"
#include "error.h"
#include "files.h"
#include "master.h"
#include "lang_tools.h"
#include "archie_dbm.h"

/*
 * ardomain: takes the archie pseudodomains description file and builds
 * it into the domains database


   argv, argc are used.


   Parameters:	  -M <master database pathname>
		  -f <domain file>
		  -h <host database pathname>
		  -d (dump domain file)


 */

char *prog;

int main(argc, argv)
   int argc;
   char *argv[];

{

  extern int errno;
#if 0
#ifdef __STDC__

  extern int getopt(int, char **, char *);

#else

  extern int getopt();

#endif
#endif

  extern int opterr;
  extern char *optarg;


  pathname_t host_database_dir;
  pathname_t master_database_dir;
   
  file_info_t *domain_file = create_finfo();
  char input_buf[1024];

  int option;
  int lineno;                   /* line number */

  domain_t domain_list[MAX_NO_DOMAINS];


  file_info_t *domain_db = create_finfo();

  /* for changing link names */

  pathname_t old_db;
  pathname_t new_db;
  pathname_t curr_db;

  char *iptr;

  char *tmp_ptr;
  int tmp_args;

  int dump_domains = 0;

  domain_struct domain_s;

  char **cmdline_ptr = argv + 1;
  int cmdline_args = argc - 1;

  host_database_dir[0] = '\0';
  master_database_dir[0] = '\0';
   
  domain_file -> filename[0] = '\0';

  opterr = 0;

  while((option = (int) getopt(argc, argv, "M:h:f:d")) != EOF)

  switch(option){

    /* master database directory name */

  case 'M':
    strcpy(master_database_dir,optarg);
    cmdline_ptr += 2;
    cmdline_args -= 2;
    break;

  case 'd':
    dump_domains = 1;
    cmdline_ptr++;
    cmdline_args--;
    break;

    /* domain file name */

  case 'f':
    strcpy(domain_file -> filename,optarg);
    cmdline_ptr += 2;
    cmdline_args -= 2;
    break;

    /* host database directory name */

  case 'h':
    strcpy(host_database_dir,optarg);
    cmdline_ptr += 2;
    cmdline_args -= 2;
    break;

  default:

    exit(ERROR);
    break;

  }


  if(set_master_db_dir(master_database_dir) == (char *) NULL){

    /* "Error while trying to set master database directory" */

    error(A_ERR,"ardomain", ARDOMAIN_001);
    exit(ERROR);
  }

   
      
  if(set_host_db_dir(host_database_dir) == (char *) NULL){

    /* "Error while trying to set host database directory" */

    error(A_ERR,"ardomain", ARDOMAIN_002);
    exit(ERROR);
  }

  if(dump_domains){
    domain_struct  domain_set[MAX_NO_DOMAINS];
    int i;


    if(open_host_dbs(NULL, NULL, domain_db, NULL, O_RDONLY) != A_OK){

      /* "Can't open domain database" */

      error(A_ERR,"ardomain", ARDOMAIN_005);
      exit(ERROR);

    }

    if(get_domain_list(domain_set, MAX_NO_DOMAINS, domain_db) == ERROR){

      /* "Can't get list of domains" */

      error(A_ERR, "ardomain", ARDOMAIN_013);
      exit(ERROR);
    }

    for(i = 0; domain_set[i].domain_def[0] != '\0'; i++){


      printf("%-20s%-40s %-s\n", domain_set[i].domain_name, domain_set[i].domain_def, domain_set[i].domain_desc);
    }

    exit(A_OK);
  }
      
  /* Play with links here */

  sprintf(new_db,"%s-new.db", host_db_filename( DEFAULT_DOMAIN_DB));
  sprintf(old_db,"%s-old.db", host_db_filename( DEFAULT_DOMAIN_DB));   


  if(rename(new_db, old_db) == -1){
    if(errno == ENOENT){

      /* "No domain file. Creating." */

      error(A_WARN,"ardomain", ARDOMAIN_003);
    }
    else{

      /* "Can't rename %s to %s " */

      error(A_SYSERR, "ardomain", ARDOMAIN_004, new_db, old_db);
      exit(ERROR);
    }
  }

  /* curr_dir is just used as a temporary variable here */

  sprintf(curr_db,"%s-new",host_db_filename( DEFAULT_DOMAIN_DB));

  strcpy(domain_db -> filename, curr_db);

  if(open_host_dbs(NULL, NULL, domain_db, NULL, O_RDWR) != A_OK){

    /* "Can't open domain database" */

    error(A_ERR,"ardomain", ARDOMAIN_005);
    exit(ERROR);

  }


  /* make input file default if not given */

  if(domain_file -> filename[0] == '\0')
  sprintf(domain_file -> filename,"%s/%s/%s", get_archie_home(), DEFAULT_ETC_DIR, DEFAULT_ARDOMAINS);
      
  sprintf(curr_db,"%s.db", host_db_filename( DEFAULT_DOMAIN_DB));


  if(open_file(domain_file, O_RDONLY) != A_OK){

    /* "Can't open input file %s" */

    error(A_ERR, "ardomain", ARDOMAIN_006, domain_file -> filename);
    exit(ERROR);
  }
  
  for(iptr = input_buf, lineno = 1; fgets(iptr, input_buf + sizeof(input_buf) - iptr, domain_file -> fp_or_dbm.fp) != (char *) NULL; lineno++){

    if((tmp_ptr = strstr(input_buf,CONTINUATION_LINE)) != NULL){
      *tmp_ptr = '\0';
      iptr = tmp_ptr;
      continue;
    }
         
    /* Recognise comments */

    if((tmp_ptr = strchr(input_buf,COMMENT_CHAR)) != NULL)
    *tmp_ptr = '\0';

    memset(&domain_s, '\0', sizeof(domain_struct));

    tmp_args = sscanf(input_buf,"%s %s %[^\n]",domain_s.domain_name, domain_s.domain_def, domain_s.domain_desc);

    if(tmp_args == EOF)
    continue;

    if((tmp_args != 0) && (tmp_args < 2)){

      /* "\nInsufficient number of arguments (%u) line %u:\n%s\n" */

      error(A_ERR, "ardomain", ARDOMAIN_007, tmp_args, lineno, input_buf);
      goto cleanup;
    }

    if(tmp_args == 0)
    continue;

    if(put_dbm_entry(domain_s.domain_name, strlen(domain_s.domain_name) + 1, &domain_s, sizeof(domain_struct), domain_db, 0) == ERROR){

      /* "Can't enter domain '%s' (line %u). Possible duplicate." */

      error(A_ERR, "ardomain",ARDOMAIN_009, domain_s.domain_name, lineno);
      goto cleanup;
    }

    /* compile_domains will detect loops in the domain database */

    if(compile_domains(domain_s.domain_def, domain_list, domain_db, &tmp_args) == ERROR){

      /* "Loop detected on domain '%s'. Aborting." */

      error(A_ERR, "ardomain", ARDOMAIN_009, domain_s.domain_name);
      goto cleanup;
    }

    iptr = input_buf;

  } /* while */

  if(unlink(curr_db) != 0){

    if(errno != ENOENT){

      /* "Can't unlink %s" */

      error(A_SYSERR, "ardomain", ARDOMAIN_010, curr_db);
      goto cleanup;
    }
  }

  if(link(new_db, curr_db) != 0){

    /* "Can't link %s to %s" */

    error(A_SYSERR, "ardomain",ARDOMAIN_011,new_db,curr_db);
    goto cleanup;
  }

  close_host_dbs(NULL, NULL, domain_db, NULL);

  close_file(domain_file);

  exit(A_OK);
  return(A_OK);


  /*
   * something failed, return to old database. Don't worry about error
   * checking
   */

 cleanup:

  error(A_INFO,"ardomain", ARDOMAIN_012);
  close_host_dbs(NULL, NULL, domain_db, NULL);
  unlink(curr_db);
  rename(old_db, new_db);
  link(new_db, curr_db);

  exit(ERROR);
  return(ERROR);
  
}
