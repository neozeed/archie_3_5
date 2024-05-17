

#ifndef URLDB
#define URLDB

typedef struct {
  URL *url;
  int recno;
  int content;  /* if 1 then the database has the content of the url */
  time_t date;
} urlEntry;

extern int init_url_db PROTO((char *, char *server,ip_addr_t ip,char *port, char *path));
extern int close_url_db PROTO((void));
extern int print_url_db PROTO((void));
extern int output_url_db PROTO((FILE *fp));
extern int check_url_db PROTO((char *url));
extern int store_url_db PROTO((URL *url, int ext, int redirection));
extern urlEntry *get_unvisited_url PROTO((int *known));


#endif
