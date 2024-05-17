#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#if !defined(AIX) && !defined(SOLARIS)
#include <vfork.h>
#endif
#include <malloc.h>
#include <search.h>
#include <memory.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "typedef.h"
#include "db_files.h"
#include "host_db.h"
#include "header.h"
#include "error.h"
#include "archie_strings.h"
#include "files.h"
#include "master.h"
#include "protos.h"
#include "db_ops.h"

/* This performs a simple ACL based on ip addresses */

typedef struct {
  int ip[4];  /* Composes the ip address a.b.c.d */
} ip_t;


#define DIRSRV_ACL_FILE "dirsrv.acl"
#define DIRSRV_ACL_MESSAGE_FILE "dirsrv.acl.mesg"

#define DIRSRV_ACL_MESSAGE_DEFAULT "Access to this server is restricted"

static ip_t *acl_list = NULL;
static int acl_list_num = 0;
static int acl_list_max = 0;
static int acl_not_present = 0;

static char acl_err_mesg[100];


#define IP_STAR 257

extern int errno;



static int qcompare_ip(a,b)
  ip_t *a, *b;
{
  if ( a->ip[0] != b->ip[0] )
    return a->ip[0]- b->ip[0];
  
  if ( a->ip[1] != b->ip[1] )
    return a->ip[1]- b->ip[1];
  
  if ( a->ip[2] != b->ip[2] )
    return a->ip[2]- b->ip[2];
  
  return a->ip[3]- b->ip[3];

}


static int compare_ip(a,b) /* Assuming that can contain * */
  ip_t *a, *b;
{
  if ( b->ip[0] == IP_STAR )
   return 0;
  
  if ( a->ip[0] != b->ip[0] )
    return a->ip[0]- b->ip[0];

  if ( b->ip[1] == IP_STAR )
   return 0;
  
  if ( a->ip[1] != b->ip[1] )
    return a->ip[1]- b->ip[1];
  
  if ( b->ip[2] == IP_STAR )
   return 0;
  
  if ( a->ip[2] != b->ip[2] )
    return a->ip[2]- b->ip[2];

  if ( b->ip[3] == IP_STAR )
   return 0;
  
  return  a->ip[3]- b->ip[3];

}

static int file_proc(ft,name,present)
  file_info_t *ft;
  char *name;
  int *present;
{
  struct stat st;
  *present = 1;
  if ( ft == NULL ) {
    error(A_ERR,"file_proc", "Unable to create ft file pointer");
    return 0;
  }
  
  sprintf(ft->filename,"%s/%s/%s",get_archie_home(),DEFAULT_ETC_DIR,name);
  if ( stat(ft->filename,&st) == -1 ) {
    if ( errno == ENOENT ) {
     *present = 0;
      return 0;
    }
    error(A_ERR,"file_proc","Error finding ft: %s, errno = %d",ft->filename,errno);
    return 0;
  }
  
  if ( open_file(ft, O_RDONLY) == ERROR ) {
    error(A_ERR,"file_proc", "Error opening acl file: %s",ft->filename);
    return 0;
  }
  
  return 1;
}


    
static void  read_acl_message()
{
  char buff[100];
  int i;
  
  file_info_t *acl_mesg = create_finfo();

  strcpy(acl_err_mesg, DIRSRV_ACL_MESSAGE_DEFAULT);
  
  if ( file_proc(acl_mesg,DIRSRV_ACL_MESSAGE_FILE,&i) ) {
    if ( fgets(buff,99,acl_mesg->fp_or_dbm.fp) != NULL ) {
      *(strrchr(buff,'\n')) = '\0';
      strcpy(acl_err_mesg,buff);
    }
    close_file(acl_mesg);
    destroy_finfo(acl_mesg);
  }
}




int read_acl_file()
{
  char buff[100];
  int i,j;
  char **av;
  file_info_t *acl_file = create_finfo();
  
  if (! file_proc(acl_file,DIRSRV_ACL_FILE,&j) ) {
    if ( j == 0 ) {
      acl_not_present = 1;
      return 1;
    }
    return 0;
  }
      
  /* Count the number of items */
  fseek(acl_file->fp_or_dbm.fp, 0L ,0);
  i = 0;
  while (fgets(buff,99,acl_file->fp_or_dbm.fp) != NULL ) {
    i++;
  }

  if ( i == 0  ) {
    error(A_INFO,"read_acl_file","ACL file is empty");
    acl_not_present = 1;
    return 1;
  }
  
  if ( acl_list_max == 0 ) {  /* New */
    acl_list_max = i+10;
    acl_list = (ip_t*)malloc(sizeof(ip_t)*acl_list_max);
    if ( acl_list == NULL ) {
      error(A_ERR,"read_acl_file","Unable to allocate memory");
      return 0;
    }
  }
  else {
    ip_t *tmp;
    if ( acl_list_max <= i ) {
      acl_list_max = i+10;
      tmp  = (ip_t*)realloc(acl_list, sizeof(ip_t)*acl_list_max);
      if ( tmp == NULL ) {
        error(A_ERR,"read_acl_file", "Unable to expand allocated memory");
      }
      acl_list = tmp;
    }
  }

  acl_list_num = 0;
  
  fseek(acl_file->fp_or_dbm.fp, 0L ,0);
  while (fgets(buff,99,acl_file->fp_or_dbm.fp) != NULL ) {
    av = str_sep(buff,'.');
    if ( av == NULL ) {
      error(A_ERR,"read_acl_file", "Unable to decompose string: %s",buff);
    }
    for ( i = 0; i < 4; i++ ) {
      if ( av[i] == NULL|| av[i][0] == '*' ) {
        break;
      }
      acl_list[acl_list_num].ip[i] = atoi(av[i]);
    }
    for ( ; i < 4; i++ ) {
      acl_list[acl_list_num].ip[i] = IP_STAR;
    }
    acl_list_num++;
  }

  close_file(acl_file);

  qsort(acl_list,acl_list_num, sizeof(ip_t),qcompare_ip);

  read_acl_message();
  
  destroy_finfo(acl_file);
  return 1;
}


int verify_acl_list(from)
struct sockaddr_in *from;
{
  ip_t ip;
  int i;
  char **av;
  char *ip_addr;

  
  if ( acl_not_present )
    return 1;
  
  if ( acl_list == NULL ) {
    if (!read_acl_file() ) 
       return 0;
    if ( acl_not_present )
      return 1;
  }
  

  ip_addr = inet_ntoa(from->sin_addr);
  printf("%s\n",ip_addr);
  /* decompose address */
  av = str_sep(ip_addr,'.');
  if ( av == NULL ) {
    error(A_ERR,"verify_acl_list","Unable to decompsoe ip address: %s. Accepting",ip_addr);
    return 1;
  }

  ip.ip[0] = atoi(av[0]);
  ip.ip[1] = atoi(av[1]);
  ip.ip[2] = atoi(av[2]);
  ip.ip[3] = atoi(av[3]);
  

  if ( ip.ip[0] == 127 && ip.ip[1] == 0 && ip.ip[2] == 0 && ip.ip[3] == 1 ) {
    free_opts(av);
    return 1;
  }

  
  for ( i = 0; i < acl_list_num; i++ ) {
    int ret;

    ret = compare_ip(&ip, &acl_list[i]);
    if ( ret == 0 ){
      free_opts(av);
      return 1;
    }

    if ( ret < 0 ) {
      free_opts(av);
       return 0;
    }
  }
  free_opts(av);
  return 0;
}

char *acl_message()
{
  return acl_err_mesg;
}


#if 0
char *prog;

int main(argc,argv)
  int argc;
  char **argv;
{
  char buff[100];
  struct sockaddr_in from;
  
  prog = argv[0];
  while ( fgets(buff,99,stdin) != NULL ) {
    *(strrchr(buff,'\n')) = '\0';
    printf("Checking %s, ",buff);

    from.sin_addr.s_addr =  inet_addr(buff);
    
    if ( verify_acl_list(&from) )
      printf("Ok\n");
    else
      printf("No:%s\n",acl_message());
  }
return 0;
}
#endif
