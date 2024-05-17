#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include "error.h"
#include "typedef.h"
#include "prarch.h"
#include "ar_search.h"
#include "db_ops.h"
#ifdef OLD_FILES
#  include "old-host_db.h"
#else
#  include "host_db.h"
#endif
#include "files.h"
#include "archie_strings.h"
#include "master.h"
#include "times.h"

char	*hostname = "THISHOST";
char	*hostwport = "THISHOST";
char *prog ;


void main(argc,argv)
  int argc;
  char *argv[];
{
  VLINK		clink;
  VLINK		conflink;
  VDIR_ST	dir_st;
  VDIR		dir = &dir_st;
  PATTRIB	atlink;
  search_req_t	search_req;
  file_info_t	*strings_idx = create_finfo();
  file_info_t	*strings = create_finfo();
  file_info_t	*strings_hash = create_finfo();
  file_info_t	*domaindb = create_finfo();
  file_info_t   *hostdb = create_finfo();
  file_info_t   *hostbyaddr = create_finfo();
  file_info_t   *hostaux_db = create_finfo();  

  extern int optind;            /*NOT USED*/ 
  extern char *optarg;

  char **cmdline_ptr;
  int cmdline_args;
  char option;

  prog = tail(argv[0]) ;


  search_req.orig_type = S_EXACT;
  search_req.domains = (char *) NULL;
  search_req.comp_restrict = (char *) NULL;
  search_req.attrib_list = (attrib_list_t) 0;

  cmdline_ptr = argv + 1;
  cmdline_args = argc - 1;



  while((option = (int) getopt(argc, argv, "scerad:t:kzxKZX")) != EOF) {
    switch(option) {

    case 's':
      search_req.orig_type = S_SUB_NCASE_STR;
      cmdline_ptr ++;
      cmdline_args --;
      break;

    case 'c':
      search_req.orig_type = S_SUB_CASE_STR;
      cmdline_ptr ++;
      cmdline_args --;
      break;

#if 0
    case 'a':
      search_req.attrib_flag = FALSE;
      cmdline_ptr ++;
      cmdline_args --;
      break;
#endif

    case 'Z':
      search_req.orig_type = S_ZUB_NCASE;
      cmdline_ptr ++;
      cmdline_args --;
      break;

    case 'z':
      search_req.orig_type = S_E_ZUB_NCASE;
      cmdline_ptr ++;
      cmdline_args --;
      break;


    case 'X':
      search_req.orig_type = S_X_REGEX;
      cmdline_ptr ++;
      cmdline_args --;
      break;

    case 'x':
      search_req.orig_type = S_E_X_REGEX;
      cmdline_ptr ++;
      cmdline_args --;
      break;

    case 'K':
      search_req.orig_type = S_SUB_KASE;
      cmdline_ptr ++;
      cmdline_args --;
      break;

    case 'k':
      search_req.orig_type = S_E_SUB_KASE;
      cmdline_ptr ++;
      cmdline_args --;
      break;

    case 'e':
      search_req.orig_type = S_EXACT;
      cmdline_ptr ++;
      cmdline_args --;
      break;

    case 'r':
      search_req.orig_type = S_FULL_REGEX;
      cmdline_ptr ++;
      cmdline_args --;
      break;

    case 'd':
      search_req.domains = (char *) strdup(optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;
	    
    case 't':
      search_req.comp_restrict = (char *) strdup(optarg);
      cmdline_ptr += 2;
      cmdline_args -= 2;
      break;
    }
  }
	

  vdir_init(dir);

  set_master_db_dir("");

      
  /* Do only once */
  set_files_db_dir("");
  if(open_files_db(strings_idx, strings, strings_hash, O_RDONLY) != A_OK) {
    fprintf(stderr,"prarch: Can't open database\n");
    exit(1);
  }

  set_host_db_dir("");

  if(open_host_dbs(hostbyaddr, hostdb, domaindb, hostaux_db, O_RDONLY) != A_OK) {
    fprintf(stderr,"prarch: Can't open database\n");
    exit(1);
  }

  strcpy(search_req.search_str, *cmdline_ptr);
  search_req.orig_offset = 0;
  search_req.max_uniq_hits = 5;
  search_req.max_filename_hits = 100;  

   SET_LINK_SIZE(search_req.attrib_list);
   SET_LK_LAST_MOD(search_req.attrib_list);
   SET_LK_UNIX_MODES(search_req.attrib_list);


#if 0
  if(argc == 2) retval = search_files_db(&strings, &strings_idx, &strings_hash, &search_req,dir,0); 
  else retval = prarch_host(argv[1],argv[2],dir,1); 
#endif




#if 0
  while(1){

     search_files_db(strings, strings_idx, strings_hash, domaindb, hostdb, hostaux_db, hostbyaddr, &search_req,dir);
#else

     search_files_db(strings, strings_idx, strings_hash, domaindb, hostdb, hostaux_db, hostbyaddr, &search_req,dir);

#endif

  clink = dir->links;

  while(clink) {
    conflink = clink;
    while(conflink) {
      printf("\n      String: %s\n",conflink->name);
      printf("   ObjType: %s\n",conflink->type);
      printf("  LinkType: %s\n",((conflink->linktype == 'U') ? "Union" : "Standard"));
      printf("  HostType: %s\n",conflink->hosttype);
      printf("      Host: %s\n",conflink->host);
      printf("  NameType: %s\n",conflink->nametype);
      printf("  Pathname: %s\n",conflink->filename);
      if(conflink->version) printf("   Version: %d\n",conflink->version);
      printf("     Magic: %d\n",conflink->f_magic_no);
      atlink = conflink -> lattrib;
      while(atlink){
        printf("	Name: %s\n	Value: %s\n", atlink -> aname, atlink -> value.ascii);
        atlink = atlink -> next;
      }
      conflink = conflink->replicas;
    }
    clink = clink->next;
  }

#if 0

      printf("###DONE");
      vdir_init(dir);
  }
#endif  
	
  exit(0);

}

