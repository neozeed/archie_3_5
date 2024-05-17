#ifndef _LISTD_H_
#define _LISTD_H_

#include "tuple.h"
#include "protonet.h"
#include "databases.h"
#include "header.h"
#include "domain.h"
#include "error.h"



#define	 MAX_NO_SOURCE_ARCHIES	32

#define	MAX_FAILS	3

typedef	struct{
   database_name_t	db_name;
   domain_t		domains;
   int			maxno;
   char			perms[3];
   date_time_t		update_freq;
   date_time_t		update_time;
   int			fails;
} udb_attrib_t;

typedef struct{
   hostname_t	source_archie_hostname;
   int		fails;
   udb_attrib_t	db_attrib[MAX_NO_DATABASES];
} udb_config_t;
   

typedef struct{
   hostname_t	     source_archie_hostname;
   date_time_t	     retrieve_time;
   hostname_t	     primary_hostname;
   hostname_t	     preferred_hostname;
   ip_addr_t	     primary_ipaddr;
   database_name_t   database_name;
   flags_t	     flags;
   int           port;
} t_hold_t;
   
#ifndef	 RET_MANAGER_NAME
#define	 RET_MANAGER_NAME     "arretrieve"
#endif

#ifndef	 CLIENT_NAME
#define	 CLIENT_NAME	      "arexchange"
#endif

#ifndef  DEFAULT_TIMEOUT
#define	 DEFAULT_TIMEOUT	15 * 60	     /* 15 minutes */
#endif


extern tuple_t		**compose_tuples PROTO((char *, char *, date_time_t, char *,file_info_t *, file_info_t *,file_info_t *,file_info_t *, int *));
extern command_v_t	*read_net_command PROTO((FILE *));
extern status_t		write_net_command PROTO((int, void *, FILE *));
extern status_t		do_server PROTO((file_info_t *, file_info_t *, file_info_t *, file_info_t *, udb_config_t *, int, int));
extern status_t		do_client PROTO((file_info_t *, file_info_t *, file_info_t *, file_info_t *, udb_config_t *, char *, char *, int, int, char *, int, int, int));
extern status_t		send_tuples PROTO((tuple_t **, int *, FILE *));
extern status_t		send_header PROTO((void *, header_t *, file_info_t *, file_info_t *));
extern status_t		sendsite PROTO((void *, file_info_t *, file_info_t *, int, int));
extern status_t		read_arupdate_config PROTO((file_info_t *, udb_config_t *));
extern status_t		get_tuple_list PROTO((FILE *,tuple_t *, index_t));
extern status_t		process_tuples PROTO((tuple_t *, index_t *, tuple_t *, char *, int *, file_info_t *, file_info_t *));
extern status_t		get_sites PROTO((file_info_t *, hostname_t, tuple_t *, index_t, FILE *, FILE *, int, int));
extern status_t		check_authorization PROTO(( udb_config_t *));
extern void		setup_signals PROTO(());
extern status_t		send_error PROTO((char *, FILE *));
extern status_t		get_headers PROTO((file_info_t *,tuple_t *, int, int *, FILE *, FILE *));
extern status_t		write_arupdate_config PROTO((file_info_t *, udb_config_t *, char *));
extern void		sig_handle PROTO(());


#endif
