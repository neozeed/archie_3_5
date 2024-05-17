#ifndef DB2V_H
#define DB2V_H

#include "parser_file.h"

#ifdef __STDC__

extern char *db2v_date(parser_entry_t *pe, char *s, int slen) ;
extern char *db2v_links(parser_entry_t *pe, char *s) ;
extern char *db2v_name(parser_entry_t *pe, char *s) ;
extern char *db2v_owner(parser_entry_t *pe, char *s) ;
extern char *db2v_perms(parser_entry_t *pe, char *s) ;
extern char *db2v_size(parser_entry_t *pe, char *s) ;
extern char *db2vype(parser_entry_t *pe, char *s) ;

#else

extern char *db2v_date(/* parser_entry_t *pe, char *s, int slen */) ;
extern char *db2v_links(/* parser_entry_t *pe, char *s */) ;
extern char *db2v_name(/* parser_entry_t *pe, char *s */) ;
extern char *db2v_owner(/* parser_entry_t *pe, char *s */) ;
extern char *db2v_perms(/* parser_entry_t *pe, char *s */) ;
extern char *db2v_size(/* parser_entry_t *pe, char *s */) ;
extern char *db2vype(/* parser_entry_t *pe, char *s */) ;

#endif
#endif
