#ifndef OPTIONS_H
#define OPTIONS_H

#define SUFFIX_OPTIONS ".cf"

typedef struct {
  char name[80];
  char type[80];
  pathname_t path;
} option_t;


extern status_t open_option_file PROTO((file_info_t *, char *));
extern status_t find_option PROTO(( file_info_t *,char *, char *, option_t **));
extern status_t close_option_file PROTO((file_info_t *));

#endif
