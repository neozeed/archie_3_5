#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
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
#include "master.h"
#include "ar_attrib.h"


char	*hostname = "THISHOST";
char	*hostwport = "THISHOST";

void main(argc,argv)
     int	argc;
     char	*argv[];
{

  PATTRIB	atlink;
  VLINK		conflink;
  VDIR_ST	dir_st;
  VLINK		clink;
  file_info_t	*strings_idx = create_finfo();
  file_info_t	*strings = create_finfo();
  file_info_t	*strings_hash = create_finfo();
  file_info_t	*domaindb = create_finfo();
  file_info_t   *hostdb = create_finfo();
  file_info_t   *hostaux_db = create_finfo();
  file_info_t   *hostbyaddr = create_finfo();
  VDIR		dir = &dir_st;
  attrib_list_t attrib_list;

  SET_ALL_ATTRIB(attrib_list);

  set_master_db_dir("");

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

  vdir_init(dir);

  prarch_host_dir(argv[1], "anonftp", attrib_list, (argv[2] == NULL ? "" : argv[2]),dir, hostdb, hostaux_db, strings);
  clink = dir->links;

  while(clink) {
    conflink = clink;
    while(conflink) {
      printf("\n      Name: %s\n",conflink->name);
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


}
