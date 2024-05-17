#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <varargs.h>

#include "protos.h"
#include "typedef.h"
#include "archie_inet.h"
#include "header.h"
#include "files.h"
#include "host_db.h"
#include "error.h"
#include "menu.h"
#include "master.h"
/*#include "retrieve_web.h" */
#include "db_files.h"
#include "lang_retrieve.h"
#include "archie_strings.h"
/*#include "web.h" */
#include "url.h"
#include "http.h"
#include "robot.h"
#include "urldb.h"
#include "sub_header.h"
#include "webindexdb_ops.h"

extern int errno;
extern int compress_flag;
pathname_t compress_pgm;
extern int force;

int Verbose = 0;

static char **doc_type_restricted PROTO((char *file));

#define OLD_STATE "Old"
#define NEW_STATE "New"
#define NA_STATE "N/A"
#define DEL_STATE "Delete"
#define NOT_TYPE_STATE "NoType"


#define DEFAULT_DOC_TYPES "doc.types"

#if 0 
extern int fread();
extern int fwrite();
extern int fclose();
extern int fprintf();
extern int sscanf();
extern int strncasecmp();
extern long fseek();
extern time_t time();
extern int rename();
extern int system();
extern int strcasecmp();
#endif

#define  MAX_SIZE_FILE 1000000



int max_number_errors = 5;


int send_message( server )
  char *server;
{

  char string[512];

  sprintf(string,"%s/%s/%s %s@%s %s/%s/%s",
          get_archie_home(),DEFAULT_BIN_DIR,"mail_inform",
          "webmaster",server,
          get_archie_home(),DEFAULT_ETC_DIR,"inform_web");
  system(string);
  
}


int is_it_ferretd = 0;

int find(s)
char *s;
{
while ( *s != '\0') {
  if ( strncasecmp(s,"hotlist",7) == 0 )
     return 1;
  s++;
}

return 0;
}


time_t httptime(str)
char *str;
{

  char day[20],m[20];
  static struct tm stm;
  char datestr[15];

  if ( sscanf(str," %[a-zA-Z], %2d %[a-zA-Z] %4d %2d:%2d:%2d GMT",
              day,&stm.tm_mday,m,&stm.tm_year,&stm.tm_hour,&stm.tm_min,
              &stm.tm_sec) == 7 ) {
    strptime(str,"%A, %d %h %Y %T",&stm);
  }
  else {
    if ( sscanf(str," %[a-zA-Z], %2d-%[^-]-%2d %2d:%2d:%2d GMT",
                day,&stm.tm_mday,m,&stm.tm_year,&stm.tm_hour,&stm.tm_min,
                &stm.tm_sec) == 7 ) {
          strptime(str,"%A, %d-%h-%y %T",&stm);
    }
    else { 
      if ( sscanf(str," %[a-zA-Z] %s %2d %2d:%2d:%2d %4d",
                  day,m,&stm.tm_mday,&stm.tm_hour,&stm.tm_min,&stm.tm_sec,
                  &stm.tm_year) == 7 ) {
        strptime(str,"%A %h %d %T %Y",&stm);
        
      }
      else {
        return 0;
      }
       
    }
  }

  sprintf(datestr,"%4u%02u%02u%02u%02u%02u",stm.tm_year+1900,stm.tm_mon+1,
          stm.tm_mday,          stm.tm_hour, stm.tm_min,stm.tm_sec);

/*  return timegm(&stm); */
  return cvt_to_inttime(datestr,0);
  
}

static int  free_char_table(table)
char **table;
{

   char **t;

   if ( table != NULL ) {

      t = table;
      while ( *t != NULL ) {
         free(*t);
         t++;
      }
      free(table);
   }

   return 1;

}

static void output_sub_header(fp,sub_head,state,size,fsize,type)
FILE *fp;
sub_header_t *sub_head;
char *state;
int size,fsize;
char *type;
{

  if ( state != NULL ) {
    HDR_SET_STATE_ENTRY(sub_head->sub_header_flags);
    strcpy(sub_head->state,state);
  }
  
  HDR_SET_SIZE_ENTRY(sub_head->sub_header_flags);
  sub_head->size = size;

  HDR_SET_FSIZE_ENTRY(sub_head->sub_header_flags);
  sub_head->fsize = fsize;

  if ( type != NULL ) {
    HDR_SET_TYPE_ENTRY(sub_head->sub_header_flags);
    strcpy(sub_head->type,type);
  }

  write_sub_header( fp, sub_head,0,0,0);          

}


static int file_append(ofp,tmpname,size)
FILE *ofp;
char *tmpname;
int size;
{

  struct stat stat_buf;
  FILE *ifp;
  int nbytes,count;
  char buff[1024*8];


  if ( size == -1 ) {
    if ( stat(tmpname,&stat_buf) == -1 ) {
      return ERROR;
    }
  }

  ifp = fopen(tmpname,"r");

  if ( ifp == NULL ) {
    /* "Unable to open file %s for reading, errno %d" */
    error(A_ERR,"file_append",DO_RETRIEVE_009, tmpname,errno);
    return ERROR;
  }

  for ( count= 0; count < size ; count += nbytes) {
    if ( (nbytes = fread(buff,sizeof(char),1024*8,ifp)) <= 0 ) {
      fclose(ifp);
      /* "Unable to read file %s, errno %d" */
      error(A_ERR,"file_append",DO_RETRIEVE_010, tmpname,errno);
      return ERROR;
    }
    if ( fwrite(buff,sizeof(char),nbytes,ofp) != nbytes ) {
      fclose(ifp);
      /* "Unable to write to stream, errno %d" */
      error(A_ERR,"file_append",DO_RETRIEVE_011, errno);
      return ERROR;
    }
  }
  fprintf(ofp,"\n");
  fflush(ofp);
  fclose(ifp);
  return(A_OK);
}




/* ***************************************************************** */
/* ***************************************************************** */

/*                Other functions                                    */


static char **doc_type_restricted(file)
char *file;
{
   FILE *fp;
   char **doc_list = NULL;
   int doc_num=0,doc_max=0;
   char **tmp;
   char buff[1024];
   pathname_t docfile;

   if ( file == NULL ) {
     sprintf(docfile,"%s/%s/%s", get_archie_home(),DEFAULT_ETC_DIR,DEFAULT_DOC_TYPES);
     file = docfile;
   }

   fp = fopen(file,"r");
   if ( fp == NULL ) {
     if ( errno != ENOENT ) {

       /* "Cannot open file %s, errno = %d\n" */

       error(A_ERR,"doc_type_restricted", DO_RETRIEVE_002, file,errno);
       return NULL;
     }
     else {
       doc_list = (char**)malloc(sizeof(char*));
       if ( doc_list != NULL ) {
         *doc_list = NULL;
       }
       return doc_list;
     }
   }

   while ( fgets(buff,1024,fp) != NULL ) {

      if ( buff[0] != '\n' ) {
         buff[strlen(buff)-1] = '\0';
         if (doc_num >= doc_max-1 ) {
            doc_max+=10;
            tmp = (char**)malloc(sizeof(char*)*doc_max);

            if ( tmp == NULL ) {
              /* "Not enough memory to expand table" */

              error(A_ERR,"doc_type_restricted",DO_RETRIEVE_003);
              return NULL;
            }

            memcpy(tmp,doc_list,sizeof(char*)*doc_num);
            if ( doc_list != NULL ) 
               free(doc_list);
            doc_list = tmp;
         }

         doc_list[doc_num] = (char*)malloc((strlen(buff)+1)*sizeof(char));
         strcpy(doc_list[doc_num],buff);
         doc_num++;
      }
   }

   if ( doc_list != NULL ) {
      doc_list[doc_num] = NULL;
   }

   return doc_list;
}


static int is_ferretd(fp)
FILE *fp;
{
  char buffer[80];
  
  fseek(fp,0L,0);
  while ( fgets(buffer,80,fp) != NULL ) {
    buffer[strlen(buffer)-1] = '\0';
    if ( strncasecmp(buffer,"Server: ",8) == 0 ) {
      if ( strncasecmp(buffer+8,"Ferretd",7) == 0 ) {
        return 1;
      }
      else
        return 0;
    }
  }
  return 0;
}

static int size_allowed(fp,size)
  FILE *fp;
  int *size;
{
  char buffer[80];

  *size = 0;
  fseek(fp,0L,0);
  while ( fgets(buffer,80,fp) != NULL ) {
    buffer[strlen(buffer)-1] = '\0';
    if ( strncasecmp(buffer,"Content-length: ",16) == 0 ) {
      *size = atoi(buffer+16);
      if ( *size > MAX_SIZE_FILE )
       return 0;
    }
  }
  return 1;
}


static int type_allowed(fp,type,url)
FILE *fp;
char *type;
URL *url;
{
  char buffer[80];

  fseek(fp,0L,0);  
  type[0] = '\0';
  while ( fgets(buffer,80,fp) != NULL ) {
    buffer[strlen(buffer)-1] = '\0';
    if ( strncasecmp(buffer,"Content-Type: ",14) == 0 ) {
      strcpy(type,buffer+14);
      if ( strncasecmp(buffer+14,"www/",4) == 0 ) {
        return 1;
      }
      if ( strncasecmp(buffer+14,"text/html",9) == 0 ) {
        return 1;
      }
      if ( strncasecmp(buffer+14,"text/plain",10) == 0 ) {
        return 1;
      }
      if ( is_it_ferretd &&
          strncasecmp(buffer+14,"application/octet-stream",24) == 0 )
        return 1;
    }
  }

  return 0;
}

static void modified_time(fp,mod_time)
  FILE *fp;
  time_t *mod_time;
{

  char buffer[80];
  time_t tt;
  
  *mod_time = 0;
   
  while ( fgets(buffer,80,fp) != NULL ) {
    buffer[strlen(buffer)-1] = '\0';
    if ( strncasecmp(buffer,"Last-Modified: ",15) == 0 ) {
      tt = httptime(buffer+15);
      *mod_time = tt;
      return;
    }
  }

}


static int old_doc(last_time,mod_time)
time_t last_time;
time_t mod_time;
{
  if ( mod_time <= last_time )
  return 1;

  return 0;
}

static URL *find_new_url(fp)
FILE *fp;
{
  char buffer[80];

  while ( fgets(buffer,80,fp) != NULL ) {
    int i = strlen(buffer);
    while ( i ) {
      if ( buffer[i-1] != '\n' && buffer[i-1] != '\r' )
        break;
      buffer[i-1] = '\0';
      i--;
    }
    if ( strncasecmp(buffer,"Location: ",10) == 0 ) {
      return urlParse(buffer+10);
    }
  }
  
  return NULL;
}




void error_header( va_alist )
va_dcl
{
   pathname_t tmp_name;
   file_info_t *c_file;  /* the output file */
   file_info_t *output_file;  /* the output file */
   int count;		      /* */
   header_t *header_rec;      /* the header record to be written */
   char *suffix;
   char holdstr[1024];
   va_list al;

   va_start( al );

   c_file = va_arg(al, file_info_t *);
   output_file = va_arg(al, file_info_t *);
   count = va_arg(al, int);
   header_rec = va_arg(al, header_t *);
   suffix = va_arg(al, char *);

   sprintf(tmp_name, "%s ", va_arg(al, char *));

   vsprintf(holdstr, tmp_name, al);

   do_error_header(c_file, output_file, count, header_rec, suffix, holdstr);

   va_end( al );

}


void problem_header( va_alist )
va_dcl
{
   pathname_t tmp_name;
   file_info_t *c_file;  /* the problem header output file */
   file_info_t *output_file;  /* the output file */
   int count;		      /* */
   header_t *header_rec;      /* the header record to be written */
   char *suffix;
   char holdstr[1024];
   va_list al;

   va_start( al );

   c_file = va_arg(al, file_info_t *);
   output_file = va_arg(al, file_info_t *);
   count = va_arg(al, int);
   header_rec = va_arg(al, header_t *);
   suffix = va_arg(al, char *);

   sprintf(tmp_name, "%s ", va_arg(al, char *));

   vsprintf(holdstr, tmp_name, al);

   do_problem_header(c_file, output_file, count, header_rec, suffix, holdstr);

   va_end( al );

}


void problem_handler( va_alist )
  va_dcl
{

  file_info_t *problem_file = create_finfo();
  pathname_t tmp_name;
  file_info_t *c_file;          /* the problem header output file */
  file_info_t *output_file;     /* the output file */
  int count;                    /* */
  header_t *header_rec;         /* the header record to be written */
  char *suffix;
  char holdstr[1024];
  urlEntry *ue;
  int known;
  va_list al;


  va_start( al );
  c_file = va_arg(al, file_info_t *);
  output_file = va_arg(al, file_info_t *);
  count = va_arg(al, int);
  header_rec = va_arg(al, header_t *);
  suffix = va_arg(al, char *);

  sprintf(tmp_name, "%s ", va_arg(al, char *));

  vsprintf(holdstr, tmp_name, al);

  sprintf(problem_file->filename, "%s_%d%s", output_file -> filename, count, suffix);

  problem_header(problem_file,output_file,count, header_rec, suffix, holdstr);
  
  va_end( al );
  
  /* problem_file is closed and contains the header with the
     problem specification */

  /* Now we need to spit out the good stuff .. which is in c_file */

  fseek(c_file->fp_or_dbm.fp,0L,0);
  open_file(problem_file,O_WRONLY);

  if ( archie_fpcopy(c_file->fp_or_dbm.fp, problem_file->fp_or_dbm.fp, -1 ) == ERROR){
    error_header(c_file,output_file,count,header_rec,suffix,
                 "Unable to store retrieved information");
    close_file(problem_file);
    return;
  }

  /* We then need to spit out the known URLs */

  while ( (ue = get_unvisited_url(&known)) != NULL ) {
    URL *url;
    int recno;
    sub_header_t sub_header_rec;
    
    url = ue->url;
    recno = ue->recno;
    free(ue);
    sub_header_rec.sub_header_flags = 0;

    strcpy(sub_header_rec.local_url,url->local_url);
    HDR_SET_LOCAL_URL_ENTRY(sub_header_rec.sub_header_flags);
    sub_header_rec.port = atoi(url->port);
    HDR_SET_PORT_ENTRY(sub_header_rec.sub_header_flags);
    strcpy(sub_header_rec.server,url->server);
    HDR_SET_SERVER_ENTRY(sub_header_rec.sub_header_flags);

    if (  known ) {
      HDR_SET_RECNO_ENTRY(sub_header_rec.sub_header_flags);
      sub_header_rec.recno = recno;

      output_sub_header(problem_file->fp_or_dbm.fp, &sub_header_rec,
                        NA_STATE, 0,0,NULL);
      
    }
    else {
      output_sub_header(problem_file->fp_or_dbm.fp, &sub_header_rec,
                        NA_STATE, 0,0,NULL);
    }

    urlFree(url);

  }

  
  close_file(problem_file);
  close_file(c_file);

  unlink(c_file->filename);

  return;
}




static int process_extern_urls()
{

  DBM *dbm;
  pathname_t dbfile;
  urlEntry *ue;
  hostname_t host;
  datum key, val;
  
  /*
         sprintf(docfile,"%s/%s/%s", get_archie_home(),DEFAULT_ETC_DIR,DEFAULT_DOC_TYPES);

*/
  
  sprintf(dbfile,"%s/%s", get_wfiles_db_dir(), DEFAULT_EXTERN_URLS_DB);
  
  dbm = dbm_open(dbfile, O_RDWR | O_CREAT, DEFAULT_FILE_PERMS);
  if ( dbm == (DBM*)NULL ) {
    error(A_ERR,"process_extern_urls", "Cannot open extern urls db");
    return ERROR;
  }

  while ( (ue = get_extern_url()) != NULL ) {

    if (SERVER_URL(ue->url) == NULL ||  *(SERVER_URL(ue->url)) == '\0' ||
        PORT_URL(ue->url) == NULL ||  *(PORT_URL(ue->url)) == '\0' )
    continue;
        
    sprintf(host,"%s:%s",SERVER_URL(ue->url),PORT_URL(ue->url));

    key.dptr = host;
    key.dsize = strlen(host);
    val.dptr = "0";
    val.dsize = 1;
    
    if ( dbm_store(dbm,key, val , DBM_REPLACE ) == -1 ) {
      error(A_INFO,"process_extern_urls","Cannot store server %s in extern urls db",host);
    }
    urlFree(ue->url);
  }
  
  dbm_close(dbm);

  return A_OK;
}




static void get_host_name(server, name)
  char *server, *name;
{
   struct hostent *h;

   h = gethostbyname(name);
   if ( h == NULL ) {
      server[0] = '\0';
   }
   else 
     strcpy(server,h->h_name);

}


status_t do_retrieve(header_rec, output_file, timeout, sleep_period,num_retr)
  header_t *header_rec;
  file_info_t *output_file;
  int timeout;
  int sleep_period;
  int num_retr;
{
  extern int verbose;
  extern int curr_port;

  int portno;
  int finished = 0;
  pathname_t initpath;
  char **cmds;

  char **rurls = NULL,**lurls,**t;
  HTTPReqHead Rq;
  int known;

  URL *url, url_rec;
  int ret;
  FILE *fp;
  int count = 0;

  hostdb_t hostdb_ent;
  hostdb_aux_t hostaux_ent;
  hostname_t tmp_hostname;
   
  file_info_t *hostaux_db = create_finfo();
  file_info_t *hostdb = create_finfo();
  file_info_t *curr_file = create_finfo();

  time_t mod_time;
  int size;  
  char port[10];
  char **rdocs = NULL;
  char *error_msg;

  pathname_t tmpname;
  pathname_t system_cmd;
  pathname_t tmp_name;

  int num_retr_urls = 0;
  int max_retr_urls = num_retr;
  
  struct stat stat_buf;
  char *ext;
  
  urlEntry *ue;
  
  hostname_t type;

  sub_header_t sub_header_rec;

  index_t index;
  int number_errors = 0;
  
  ptr_check(header_rec, header_t, "do_retrieve", ERROR);
  ptr_check(output_file, file_info_t, "do_retrieve", ERROR);

  if ( compress_flag > 0 ) {  /* Check if we have the gzip */

    if ( get_option_path( "COMPRESS", "GZIP", compress_pgm) != ERROR )  {
      compress_flag = 2;
    }
    else {
      strcpy(compress_pgm, COMPRESS_PGM);
      compress_flag = 1;
    }
  }
     
  /* access_commands in the header and the port number */

  if ((cmds = str_sep(header_rec -> access_command, NET_DELIM_CHAR)) != (char **) NULL){
    int i;

    if((cmds[0] != (char *) NULL) && (*cmds[0] != '\0')) 
    sscanf(cmds[0], "%d", &portno);
    else
    portno = DEFAULT_WEB_PORT;

    if((cmds[1] != (char *) NULL) && (*cmds[1] != '\0')) 
    strcpy(initpath, cmds[1]);
    else
    strcpy(initpath, DEFAULT_WEB_STRING);

    for ( i = 0; cmds[i] != NULL; i++ ) {
      free(cmds[i]);
    }
    free(cmds);

  }

  curr_port = portno;
  sprintf(port,"%d",portno);


  /* Open host database */

      
  if (open_host_dbs((file_info_t *) NULL, hostdb, (file_info_t *) NULL, hostaux_db, O_RDONLY) != A_OK){

    /* "Error while trying to open host database" */

    error(A_ERR,"retrieve_webindex", "Error");
    exit(A_OK);
  }

  if(get_dbm_entry(header_rec -> primary_hostname,
                   strlen(header_rec-> primary_hostname) + 1,
                   &hostdb_ent, hostdb) == ERROR){

    return (ERROR);
  }

/*  
  if (get_hostaux_ent(header_rec->primary_hostname, header_rec->access_methods, &hostaux_ent, hostaux_db) == ERROR){
    return ERROR;
  }
*/
  if ( get_hostaux_ent(header_rec->primary_hostname,
                       header_rec->access_methods, &index,
                       header_rec->preferred_hostname,
                       header_rec->access_command,
                       &hostaux_ent, hostaux_db) == ERROR){
    return ERROR;
  }
  
  close_host_dbs((file_info_t *) NULL, hostdb, (file_info_t *) NULL, hostaux_db);



  /* got host and port, now do initial connect */

  if (verbose)
  error(A_INFO, "do_retrieve", "Trying to connect to %s(%d)", header_rec -> primary_hostname, portno);


  /* Open output file,  Generate a "random" file name */

  srand(time((time_t *) NULL));
   
  for(finished = 0; !finished;){
      
    sprintf(curr_file -> filename,"%s_%d%s%s", output_file -> filename, rand() % 100, SUFFIX_PARSE, TMP_SUFFIX);

    if(access(curr_file -> filename, R_OK | F_OK) == -1)
    finished = 1;
  }

  if(open_file(curr_file, O_WRONLY) == ERROR){
    error(A_ERR, "do_retrieve", "Can't open output file %s", curr_file);
    return (ERROR);
  }



  /* Get the list of restricted paths on that server */   

  rurls = check_robot_file(output_file->filename,header_rec->primary_hostname,port,WEB_AGENT_NAME );  

  if ( rurls == NULL ) {

    /* "Error accessing host %s while looking for robot file\n" */

    error(A_ERR,"do_retrieve", DO_RETRIEVE_001, header_rec->primary_hostname);
    error_header(curr_file, output_file, count, header_rec, SUFFIX_UPDATE,
                 DO_RETRIEVE_001, header_rec->primary_hostname);
    return(ERROR);
  }
  else {
    char **str;

    if ( rurls[0] != NULL && verbose  ) {
      error(A_INFO,"do_retrieve", "Robots file found");
      for (str = rurls; *str != NULL; str++ ) {
        error(A_INFO,"do_retrieve", "Restricted path: %s",*str);
      }
    }
  }

  memset(&url_rec, '0', sizeof(url_rec));
  url_rec.local_url = "/";
  if ( urlRestricted(&url_rec, rurls,"/") ) {
    error(A_ERR,"do_retrieve", "This site is completely restricted");
    error_header(curr_file, output_file, count, header_rec, SUFFIX_UPDATE, "This site is completely restricted by robots.txt file");
    return ERROR;
  }
  
  /* Get the list of restricted types of docs */
  rdocs = doc_type_restricted(NULL);

  if ( rdocs == NULL ) {

    /* "Unable to get list of permitted document types\n" */

    error(A_ERR,"do_retrieve", DO_RETRIEVE_005);
    return(ERROR);
  }


  HDR_SET_PRIMARY_IPADDR((header_rec->header_flags));
  header_rec->primary_ipaddr = hostdb_ent.primary_ipaddr;
  
  /* Initializes the list of the known URLs */
  if ( header_rec->preferred_hostname &&
      header_rec->preferred_hostname[0] != '\0' ) 
    init_url_db(output_file->filename,header_rec->preferred_hostname,
                hostdb_ent.primary_ipaddr, port, initpath);
  else 
    init_url_db(output_file->filename,header_rec->primary_hostname,
                hostdb_ent.primary_ipaddr, port, initpath);



  HDR_SET_RETRIEVE_TIME(header_rec->header_flags);
  header_rec->retrieve_time = time((time_t *) NULL);
  HDR_SET_GENERATED_BY(header_rec->header_flags);
  header_rec->generated_by = RETRIEVE;
  HDR_SET_SOURCE_ARCHIE_HOSTNAME(header_rec->header_flags);
  strcpy(header_rec->source_archie_hostname, get_archie_hostname(tmp_hostname,sizeof (tmp_hostname)));

  write_header(curr_file -> fp_or_dbm.fp, header_rec, (u32 *) 0, 0, 0);


  Rq.uagent = WEB_AGENT_NAME;
  Rq.uaver = WEB_AGENT_VERSION;
  Rq.from = NULL;
  Rq.accept = NULL;

  while ( num_retr_urls < max_retr_urls &&
         (ue = get_unvisited_url(&known)) != NULL ) {
    hostname_t server;
    char port[40];
    int recno;
    time_t ret_date;
    
    url = ue->url;
    recno = ue->recno;
    ret_date = ue->date;
    free(ue);
    get_host_name(server,SERVER_URL(url));
    strcpy(port,PORT_URL(url));

    if ( urlRestricted(url, rurls, initpath) ) {
      if ( verbose )
      error(A_INFO,"do_retrieve","URL %s is restricted.\n",url->curl);
      urlFree(url);
      continue;
    }

    if ( verbose ) 
    error(A_INFO,"do_retrieve","Processing url %s",CANONICAL_URL(url));

    sub_header_rec.sub_header_flags = 0;

    
    strcpy(sub_header_rec.local_url,url->local_url);
    HDR_SET_LOCAL_URL_ENTRY(sub_header_rec.sub_header_flags);
    sub_header_rec.port = atoi(url->port);
    HDR_SET_PORT_ENTRY(sub_header_rec.sub_header_flags);
    strcpy(sub_header_rec.server,url->server);
    HDR_SET_SERVER_ENTRY(sub_header_rec.sub_header_flags);

    switch (compress_flag) {
    case 0:
      sub_header_rec.format = FRAW;
      break;
    case 1:
      sub_header_rec.format = FCOMPRESS_LZ;
      ext = ".Z";
      break;
    case 2:
      sub_header_rec.format = FCOMPRESS_GZIP;
      ext = ".gz";
      break;
    }
    HDR_SET_FORMAT_ENTRY(sub_header_rec.sub_header_flags);
    
    /* Generate a "random" file name */  

    srand(time((time_t *) NULL));
   
    for(finished = 0; !finished;){
      sprintf(tmpname,"%s-%s_%d%s", output_file->filename, "tempWeb", rand() % 100, TMP_SUFFIX);      

      if(access(tmpname, R_OK | F_OK) == -1)
      finished = 1;
    }


    
    if ( (fp = fopen(tmpname,"w+")) == NULL ) {

      /* "Unable to open temporary file %s, errno = %d\n" */

      error(A_ERR,"do_retrieve", DO_RETRIEVE_004, tmpname, errno);
      urlFree(url);
      return(ERROR);
    }




    /* HEAD */

      
    if ( httpHeadfp(url,&Rq, timeout, &ret,NULL,fp) == 0 ) {
      if ( ret == ETIMEDOUT) {        /* Timeout */
        /* "Timeout of %d seconds expired" */
        error(A_ERR, "do_retrieve", DO_RETRIEVE_023, timeout);
        if ( number_errors > max_number_errors) {
          problem_handler(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_023,timeout);
          goto hell;

        }
      }
      /* "Error while accessing remote server, errno %d" */
      error(A_ERR, "do_retrieve", DO_RETRIEVE_024, ret);

      if ( number_errors++  > max_number_errors) {
        problem_handler(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_024,ret);
        goto hell;
      }
      fclose(fp);
      unlink(tmpname);
      urlFree(url);
      continue;
    }
    
    is_it_ferretd = is_ferretd(fp);
    
    if ( is_it_ferretd && ret == 500){
      ret = 404;
    }
    mod_time = 0;
    switch ( ret ) {
    
    case 200:                   /* SUCCESS */

      modified_time(fp,&mod_time);
      
      HDR_SET_DATE_ENTRY(sub_header_rec.sub_header_flags);
      if ( mod_time == 0 )
      sub_header_rec.date = header_rec->retrieve_time;
      else 
      sub_header_rec.date = mod_time;
      
      if ( size_allowed(fp,&size) == 0 ) {
        if ( verbose ) 
        error(A_INFO,"do_retrieve", "Document with size %d is to big to be indexed",size);

        fclose(fp);
        unlink(tmpname);
        urlFree(url);
        output_sub_header(curr_file->fp_or_dbm.fp, &sub_header_rec,
                          NA_STATE, 0, 0, type);
        sleep(sleep_period);        
        continue;
        
      }
      
      if ( type_allowed(fp,type,url) == 0 ){
	
        /* "Document of type %s not indexed" */

        if ( verbose ) 
        error(A_INFO,"do_retrieve", DO_RETRIEVE_007, type);

        fclose(fp);
        unlink(tmpname);
        urlFree(url);
        output_sub_header(curr_file->fp_or_dbm.fp, &sub_header_rec,
                          NOT_TYPE_STATE, size,0, type);
        sleep(sleep_period);        
        continue;
      }

      
      if ( old_doc(/*hostaux_ent.update_time*/ret_date, mod_time) && known && force == 0 ){
        fclose(fp);
        unlink(tmpname);
        urlFree(url);
        HDR_SET_RECNO_ENTRY(sub_header_rec.sub_header_flags);
        sub_header_rec.recno = recno;

        output_sub_header(curr_file->fp_or_dbm.fp, &sub_header_rec, 
                          OLD_STATE, 0,0,type);
        sleep(sleep_period);        
        continue;

      }

      break;

    case 201:
    case 202:
    case 204:
    case 300:                   /* MULTI CHOICES */
    case 304:                   /* NOT MODIFIED */
      /* "Unexpected response from the server: %d" */
      error_msg = DO_RETRIEVE_016;
      break;
    
    
    case 301:                   /* MOVED PERM */
    case 302:                   /* MOVED TEMP */
      {
        URL *nurl;
        
        fseek(fp,0L,0);
        nurl = find_new_url(fp);
        if ( nurl == NULL ) {
          fclose(fp);
          unlink(tmpname); 
          urlFree(url);
          /* "Invalid redirection message sent by server" */
          error(A_ERR,"do_retrieve", DO_RETRIEVE_020 , ret);
          if ( number_errors++ > max_number_errors ) {
            problem_handler(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_020);
            goto hell;
          }
          continue;
        }

/*        if ( urlLocal(SERVER_URL(url),PORT_URL(url), nurl) ) { */
        if ( urlLocal(server,port, nurl) ) { 
          /* "Redirection to an internal url %s" */
          error_msg = DO_RETRIEVE_021;
          store_url_db(nurl,0,1);
        }
        else {
          /* "Redirection to an external url %s" */
          error_msg = DO_RETRIEVE_022;
          store_url_db(nurl,1,1); 
        }
        
        if ( verbose ) 
        error(A_INFO,"do_retrieve",error_msg,nurl);

        fclose(fp);
        unlink(tmpname); 
        urlFree(url);
/*        urlFree(nurl); */
        continue;

      }
        
    case 400:                   /* BAD REQUEST */
      /* "A bad request was send to the server" */
      error_msg = DO_RETRIEVE_017;
      break;
    
    case 401:                   /* UNAUTHORIZED */
    case 403:                   /* FORBIDEN */
      /* "Access to URL %s is restricted" */
      error_msg = DO_RETRIEVE_018;
      break;
    
    case 404:                   /* NOT FOUND */
      /* "URL %s not found" */
      error_msg = DO_RETRIEVE_019;
      break;
    

    case 500:                   /* INTERNAL SERVER ERROR */
      /* "Internal server error (%d)" */
      error_msg = DO_RETRIEVE_012;
      break;
    
    case 501:                   /* NOT IMPLEMENTED */
      /* "Request not implemented (%d)" */
      error_msg = DO_RETRIEVE_013;
      break;
    
    case 502:                   /* BAD GATEWAY */
      /* "Server responded with ``bad gateway'' (%d)" */
      error_msg = DO_RETRIEVE_014;
      break;
    
    case 503:                   /* SERVICE UNAVAILABLE */
      /* "Service unavailable, (%d)" */
      error_msg = DO_RETRIEVE_015;
      break;

    }
  
    switch ( ret ) {
    case 201:
    case 202:
    case 204:
    case 300:
    case 304:      
    case 400:
    case 500:
    case 501:
    case 502:
    case 503:

      fclose(fp);
      unlink(tmpname);
      urlFree(url);
      error(A_ERR,"do_retrieve","Error sending head request");
      error(A_ERR,"do_retrieve",error_msg , ret);

      if ( number_errors++ > max_number_errors )  {
        problem_handler(curr_file, output_file, count, header_rec, SUFFIX_PARSE, error_msg);
        goto hell;
      }

      continue;

    case 401:
    case 403:
    case 404:
      fclose(fp);
      unlink(tmpname);
      error(A_ERR,"do_retrieve",error_msg , url->curl);
      urlFree(url);
      continue;

    }


    /* GET */
    truncate(tmpname,0);
    fseek(fp,0L,0);                      
    if ( httpGetfp(url,&Rq, 60, &ret,fp,NULL) == 0 ) {
      if ( ret == ETIMEDOUT ) {        /* Timeout */
        /* "Timeout of %d seconds expired" */
        error(A_ERR, "do_retrieve", DO_RETRIEVE_025, timeout);

        if ( number_errors > max_number_errors ) {
          problem_handler(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_025,timeout);
          goto hell;
        }
        
      }
      /* "Error while accessing remote server, errno %d" */
      error(A_ERR, "do_retrieve", DO_RETRIEVE_026, ret);
      if ( number_errors++ > max_number_errors ) {
        problem_handler(curr_file, output_file, count, header_rec, SUFFIX_PARSE, DO_RETRIEVE_026,ret);
        goto hell;
      }
      fclose(fp);
      unlink(tmpname);
      urlFree(url);
      continue;
    }

    
    if ( is_it_ferretd && ret == 500){
      ret = 404;
    }
    
    switch ( ret ) {

    case 200:                   /* Success */
        

      /* Need to parse the file now for new URLs... */

      /*      url_base = urlStrBuild(url); */

      fseek(fp,0L,0);
      lurls = find_urls(fp,url);
      fclose(fp);                           

      if ( size == 0 ) {        /* There was no Content-Length line .. */
        if ( stat(tmpname,&stat_buf) == -1 ) {
          size = 0;
        }
        else {
          size = stat_buf.st_size;
        }
      }

      for ( t = lurls; t != NULL && *t != NULL; t++ ) {
        
        URL *u;

        u = urlParse(*t);
        if ( u == NULL ) {
          continue;
        }
        if ( Verbose )
          error(A_INFO,"do_retrieve","Found url %s",*t);
        
        urlClean(u);
        if ( urlLocal(server,port,u) ) {
          store_url_db(u,0,0);
        }
        else {
          store_url_db(u,1,0);    /* This is an external URL */
        }
        free(*t); 
      }


      if ( compress_flag > 0 ) {
        /* Need to compress the file */
        sprintf(system_cmd,"%s -f %s",compress_pgm,tmpname);
        if ( system(system_cmd) == -1 ) {
          goto hell;
        }
        strcat(tmpname,ext);
      }

      /* Figure out the size of the compressed file */
      if ( stat(tmpname,&stat_buf) == -1 ) {
        goto hell;
      }

      output_sub_header(curr_file->fp_or_dbm.fp, &sub_header_rec, 
                        NEW_STATE, size, stat_buf.st_size,NULL);

      if ( file_append(curr_file->fp_or_dbm.fp,tmpname,stat_buf.st_size) == ERROR ) {
        goto hell;
      }
      
      break;
    
    case 301:                   /* REDIRECT */
    case 302:                   /* REDIRECT */ 

      break;


    case 201:
    case 202:
    case 204:
    case 304:                   /* NOT MODIFIED */
      /* "Unexpected response from the server: %d" */
      error_msg = DO_RETRIEVE_016;
      break;
      
    case 300:                   /* MULTI CHOICES */

      
    case 400:                   /* BAD REQUEST */
      /* "A bad request was send to the server" */
      error_msg = DO_RETRIEVE_017;
      break;
      
    case 401:                   /* UNAUTHORIZED */
    case 403:                   /* FORBIDEN */
      /* "Access to URL %s is restricted" */
      error_msg = DO_RETRIEVE_018;
      break;
      
    case 404:                   /* NOT FOUND */
      /* "URL %s not found" */
      error_msg = DO_RETRIEVE_019;
      break;
      

    case 500:                   /* INTERNAL SERVER ERROR */
      /* "Internal server error (%d)" */
      error_msg = DO_RETRIEVE_012;
      break;
      
    case 501:                   /* NOT IMPLEMENTED */
      /* "Request not implemented (%d)" */
      error_msg = DO_RETRIEVE_013;
      break;
      
    case 502:                   /* BAD GATEWAY */
      /* "Server responded with ``bad gateway'' (%d)" */
      error_msg = DO_RETRIEVE_014;
      break;
      
    case 503:                   /* SERVICE UNAVAILABLE */
      /* "Service unavailable, (%d)" */
      error_msg = DO_RETRIEVE_015;
      break;

    }
    
    switch ( ret ) {
    case 201:
    case 202:
    case 204:
    case 304:      
    case 400:
    case 500:
    case 501:
    case 502:
    case 503:

      fclose(fp);
      unlink(tmpname);
      urlFree(url);
      error(A_ERR,"do_retrieve","Error sending head request");
      error(A_ERR,"do_retrieve",error_msg , ret);
      
      if ( number_errors++ > max_number_errors ) {
        problem_handler(curr_file, output_file, count, header_rec, SUFFIX_PARSE, error_msg);
        goto hell;
      }
      continue;


    case 401:
    case 403:
    case 404:
      fclose(fp);
      error(A_ERR,"do_retrieve",error_msg , url->curl);
      unlink(tmpname);
      urlFree(url);
      continue;


    }

    fclose(fp);
      
    unlink(tmpname);

    urlFree(url);

    sleep(sleep_period);
    num_retr_urls++;

  }

  while ( (ue = get_unvisited_url(&known)) != NULL ) {
    URL *url;
    int len;
    int recno;
    sub_header_t sub_header_rec;
    
    url = ue->url;
    recno = ue->recno;
    free(ue);
    len = strlen(url->local_url);
    if ( url->local_url[len-1] == '/' ) {
      url->local_url[len-1] = '\0';
    }
    
    sub_header_rec.sub_header_flags = 0;

    strcpy(sub_header_rec.local_url,url->local_url);
    HDR_SET_LOCAL_URL_ENTRY(sub_header_rec.sub_header_flags);
    sub_header_rec.port = atoi(url->port);
    HDR_SET_PORT_ENTRY(sub_header_rec.sub_header_flags);
    strcpy(sub_header_rec.server,url->server);
    HDR_SET_SERVER_ENTRY(sub_header_rec.sub_header_flags);

    if (  known ) {
      HDR_SET_RECNO_ENTRY(sub_header_rec.sub_header_flags);
      sub_header_rec.recno = recno;

      output_sub_header(curr_file->fp_or_dbm.fp, &sub_header_rec,
                        OLD_STATE, 0,0,NULL);
      
    }
    else {
      output_sub_header(curr_file->fp_or_dbm.fp, &sub_header_rec,
                        NA_STATE, 0,0,NULL);
    }

    urlFree(url);

  }

  
  process_extern_urls();
  
  free_char_table(rurls);
  free_char_table(rdocs);

  destroy_finfo(hostaux_db);
  destroy_finfo(hostdb);

  close_url_db();
  
  sprintf(tmp_name, "%s_%d%s", output_file -> filename, count, SUFFIX_PARSE);

  if(rename(curr_file -> filename, tmp_name) == -1){

    /* "Can't rename temporary file %s to %s" */

    error(A_SYSERR,"do_retrieve", DO_RETRIEVE_006, curr_file -> filename, tmp_name);
    return(ERROR);
  }
      
  return(A_OK);

hell:
  fclose(fp);
  unlink(tmpname);
  urlFree(url);
  close_url_db();
  return ERROR;
}



