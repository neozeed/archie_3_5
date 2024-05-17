#ifndef _GETINFO_H_
#define _GETINFO_H_
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include "search.h"
#include "excerpt.h"
#include "archstridx.h"
#include "patrie.h"

#ifndef NO_STDLIB_H
#include <stdlib.h>
#else
char *getenv();
#endif

#define SEARCH_STRINGS_ONLY 2
#define SEARCH_MORE_STRINGS 3

#define LF 10
#define CR 13

#define MAX_STR  64

#define EXACT 0
#define SUB 1
#define REGEX 2
#define MAX_HITS 200
#define BOOL_MAX_HITS 1000
#define BOOL_MAX_MATCH 1000
#define MAX_MATCH 100
#define MAX_HPM 100
#define MINIMUM_NM_SIZE 6
#define MAXIMUM_NM_SIZE 64


#define MAX_ENTRIES 30
#define FORM_PLAIN_OUTPUT_FLAG "oflag"

#define FORM_CASE "case"
#define FORM_CASE_SENS "Sensitive"
#define FORM_CASE_INS "Insensitive"

#define FORM_QUERY "query"
#define FORM_ORIG_QUERY "origquery"
#define FORM_OLD_QUERY "oldquery"

#define FORM_STRINGS_ONLY "strings"
#define FORM_MORE_SEARCH "more"
#define FORM_SERV_URL "url"
#define FORM_GIF_URL "gifurl"
#define FORM_STR_HANDLE "strhan"
#define FORM_STRINGS_NO "NO"
#define FORM_STRINGS_YES "YES"

#define FORM_DB "database"
#define FORM_ANONFTP_DB "Anonymous FTP"
#define FORM_WEB_DB "Web Index"
#define I_ANONFTP_DB 0
#define I_WEBINDEX_DB 1

#define FORM_TYPE "type"
#define FORM_EXACT "Exact"
#define FORM_SUB "Sub String"
#define FORM_REGEX "Regular Expression"

#define FORM_MAX_HITS "maxhits"
#define FORM_MAX_HPM "maxhpm"
#define FORM_MAX_MATCH "maxmatch"

#define FORM_PATH_REL "pathrel"
#define FORM_PATH "path"
#define FORM_EXCLUDE_PATH "expath"
#define FORM_AND "AND"
#define FORM_OR "OR"

#define FORM_START_STRING "start_string"
#define FORM_START_STOP "start_stop"
#define FORM_START_SITE_STOP "start_site_stop"
#define FORM_START_SITE_PRNT "start_site_prnt"
#define FORM_START_SITE_FILE "start_site_file"

#define FORM_BOOLEAN_MORE_ENT "bool_more_ent"

#define FORM_FORMAT "format"
#define FORM_FORMAT_KEYS "Keywords Only"
#define FORM_FORMAT_EXC "Excerpts Only"
#define FORM_FORMAT_LINKS "Links Only"
#define FORM_FORMAT_STRINGS_ONLY "Strings Only"
#define I_FORMAT_KEYS 2
#define I_FORMAT_LINKS 1
#define I_FORMAT_EXC 0


#define PATH_AND 0
#define PATH_OR 1

#define FORM_DOMAINS "domains"
#define END_CHAIN -1


#define PLAIN_HITS "HITS"
#define PLAIN_START "START_RESULT"
#define PLAIN_START_STRINGS "START_STRINGS_ONLY"
#define PLAIN_END "END_RESULT"
#define PLAIN_END_STRINGS "END_STRINGS_ONLY"
#define PLAIN_URL "URL"
#define PLAIN_STRING "STRING"
#define PLAIN_TITLE "TITLE"
#define PLAIN_NO_TITLE "NO_TITLE"
#define PLAIN_SITE "SITE"
#define PLAIN_PATH "PATH"
#define PLAIN_TYPE "TYPE"
#define PLAIN_WEIGHT "WEIGHT"
#define PLAIN_TEXT "TEXT"
#define PLAIN_PERMS "PERMS"
#define PLAIN_SIZE "SIZE"
#define PLAIN_DATE "DATE"
#define PLAIN_FILE "FILE"
#define PLAIN_KEY "KEY"
#define PLAIN_FTYPE "FTYPE"  /* file type : indexable, not indexed,
                                unidexable */
#define PLAIN_FTYPE_INDX "FTYPE_INDX"
#define PLAIN_FTYPE_NOT_INDX "FTYPE_NOT_INDX"
#define PLAIN_FTYPE_UNINDX "FTYPE_UNINDX"


/* types of comparisons.  Case and Accent sensitivity. */

#define CMP_CIAI 0   /* 00 */
#define CMP_CIAS 1   /* 01 */
#define CMP_CSAI 2   /* 10 */
#define CMP_CSAS 3   /* 11 */
#define CMP_NULL -1  /* No comparison specified */

#define CMP_CASE_SENS 2
#define CMP_ACCENT_SENS 1



extern void getword PROTO((char *word, char *line, char stop));
extern char x2c PROTO((char *what));
extern void unescape_url PROTO((char *url));
extern int chackvalue PROTO((char *val));
extern void plustospace PROTO((char *str));
extern int readFile PROTO((char *flname));
extern char *makeword PROTO((char *line, char stop));
extern char *fmakeword PROTO((FILE *f, char stop, char no_stop, int *len));
extern char *quoteString PROTO(( const char *));
extern char *dequoteString PROTO(( const char *));

extern int parse_entries PROTO((entry *,bool_query_t *,int *,int *,int *,
                                int *,int *,int *,char ***, char **,int *,
                                int *,char *,char **,char **,char **, int *,
                                int *, start_return_t *, boolean_return_t *));
extern void send_hidden_values_in_plain PROTO((char *,entry *,start_return_t,
                                              boolean_return_t));
extern int send_result_in_plain PROTO((query_result_t *,index_t **,int,
                                       start_return_t, boolean_return_t, int,
                                       struct arch_stridx_handle *,entry *,
                                       int));
extern char * get_type PROTO((int));
extern char * get_case PROTO((int));
extern int send_error PROTO((char *));
extern int read_form_entries PROTO((entry **));
extern int read_form_entries_post PROTO((entry **, char **));
extern void free_list PROTO(( bool_list_t **, int ));
extern void free_entries PROTO(( entry ** ));
extern void free_result PROTO(( query_result_t **));
extern void free_strings PROTO(( index_t ***, int ));
extern char *strlwr PROTO(( char * ));
extern char *strupr PROTO(( char * ));
extern char *trim PROTO(( char ** ));
#endif
