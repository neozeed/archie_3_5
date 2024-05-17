#ifndef LINE_TYPE_H
#define LINE_TYPE_H

#include "site_file.h"

#ifdef __STDC__

extern char *S_dup_dir_name(const char *name) ;
extern char *S_file_parse(char *in, parser_entry_t *pe, int *is_dir) ;
extern int S_file_type(const char *s, int *is_dir) ;
extern int S_handle_file(char *line, int state) ;
extern int S_init_parser(void) ;
extern int S_line_type(const char *line) ;
extern int S_split_dir(char *str, char *prep_dir, int *dev, int *n, char ***p) ;

#else

extern char *S_dup_dir_name(/* const char *name */) ;
extern char *S_file_parse(/* char *in, parser_entry_t *pe, int *is_dir */) ;
extern int S_file_type(/* const char *s, int *is_dir */) ;
extern int S_handle_file(/* char *line, int state */) ;
extern int S_init_parser(/* void */) ;
extern int S_line_type(/* const char *line */) ;
extern int S_split_dir(/* char *str, char *prep_dir, int *dev, int *n, char ***p */) ;

#endif

#define L_BLANK	    0
#define L_CONT	    1
#define L_DIR_START 2
#define L_ERROR	    3
#define L_FILE	    4
#define L_PARTIAL   5
#define L_TOTAL	    6
#define L_UNREAD    7
#define L_UNKNOWN   8 /* code depends on this being the last valid value */

#define L_NUM_ELTS (L_UNKNOWN + 1)
#define L_INTERN_ERR L_NUM_ELTS

#endif
