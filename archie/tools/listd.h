#ifndef _LISTD_H_
#define _LISTD_H_


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
   

extern status_t		read_arupdate_config PROTO((file_info_t *, udb_config_t *));



#endif
