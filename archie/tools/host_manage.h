#ifndef _HOST_MANAGE_H_
#define _HOST_MANAGE_H_

#include "host_db.h"
#include <signal.h>

#define	 MAX_NO_FIELDS	10

#define DEF_TEMPLATE_NAME     "NO_NAME"
#define	DEF_TEMPLATE_FNAME    "Access commands"

#define	DEF_DBSPEC_FILE	      "hm_db.cf"

typedef struct{
   pathname_t  field_name;
   int	       max_field_width;
   pathname_t  def_str;
   pathname_t  curr_val;
   int	       writable;
   int	       lineno;
   int	       colno;
} dbfield_t;

typedef struct{
   pathname_t  dbspec_name;
   dbfield_t   subfield[MAX_NO_FIELDS];
} dbspec_t;
   
#define	 LCUR_RESPONSE		    0
#define	 LCUR_DBSPEC		   10



#define	 PREVIOUS_FIELD	   -1
#define	 NEXT_FIELD	    1
#define	 PREVIOUS_CHAR	   -1
#define	 NEXT_CHAR	    1
#define	 FIRST_CHAR	    2
#define	 LAST_CHAR	    3

#define	 PREVIOUS_SITE	    2
#define	 NEXT_SITE	    3
#define	 PREVIOUS_VAL	    PREVIOUS_SITE
#define	 NEXT_VAL	    NEXT_SITE
#define	 PREVIOUS_HOSTAUX   4
#define	 NEXT_HOSTAUX	    5
#define	 ADD_SITE	    6
#define	 UPDATE_SITE	    7
#define	 GOTO_SITE	    8
#define	 GOTO_DATABASE	    9
#define	 EXIT_SITE	    0
#define	 ADD_HOSTAUX	   10
#define	 HELP		   11
#define	 FIRST_SITE	   12
#define	 LAST_SITE	   13
#define	 FIRST_HOSTAUX	   14
#define	 LAST_HOSTAUX	   15
#define	 DELETE_SITE	   16
#define	 DELETE_HOSTAUX	   17
#define	 REACTIVATE_DATABASE   18
#define	 FORCE_UPDATE	   19
#define	 HOSTLIST	   20
#define	 FORCE_DB_DELETE   21

#define	 DBSPECS_BEGIN	    '{'
#define	 DBSPECS_END	    '}'

extern	 status_t	host_manage PROTO((file_info_t *, file_info_t *, file_info_t *, file_info_t *, char *, hostname_t, dbspec_t *, hostname_t, hostname_t));
extern	 void		sig_handle PROTO((int));
extern	 status_t	check_update PROTO((file_info_t *, file_info_t *,  hostdb_t *, file_info_t *, hostdb_aux_t *,dbspec_t *, int,int,action_status_t));
extern	 status_t	do_update PROTO((file_info_t *,file_info_t *,file_info_t *,hostdb_t *,hostdb_aux_t *,action_status_t));
extern	 status_t	read_dbspecs PROTO((file_info_t *, dbspec_t *));
extern	 status_t	dbspec_setup_screen PROTO((dbspec_t *, hostdb_aux_t *));
extern	 status_t	move_dbspec_field PROTO((dbspec_t *,int,int *,int *));
extern	 status_t	move_dbspec_cursor PROTO((int, int *,int *,dbspec_t *));
extern	 status_t	get_new_acommand PROTO((char *, char *,dbspec_t *));
extern	 status_t	addto_auxdbs PROTO((char ***, char *, char ***));


#endif
