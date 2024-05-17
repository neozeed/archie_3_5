#ifndef  _OLD_HOSTDB_
#define  _OLD_HOSTDB_


#define DEFAULT_TUPLE_LIST_SIZE		1000   /* Default size of tuple array */
#define MAX_TUPLE_SIZE			257   /* Maximum size of the tuple */

typedef char	tuple_t[MAX_TUPLE_SIZE];


typedef struct{
   hostname_t		primary_hostname;	/* Primary DNS hostname of site	*/
   hostname_t		preferred_hostname;	/* Preferred hostname of site	*/
   ip_addr_t		primary_ipaddr;		/* Primary IP address of site	*/
   os_type_t		os_type;		/* OS operating at site		*/
   timezone_t		timezone;		/* Timezone in which site resides	*/
   access_methods_t	access_methods;		/* Access methods used at site	*/
   flags_t		flags;			/* Miscellaneous info about site	*/
} old_hostdb_t;


/* Flags by bit number for hostdb

   1) Site runs Prospero
*/  

#define HDB_SET_PROSPERO_SITE(x)	((x) |= 0x01)
#define	HDB_UNSET_PROSPERO_SITE(x)	((x) &= ~0x01)
#define	HDB_IS_PROSPERO_SITE(x)		((x) & 0x01)
					

typedef struct{
   gen_prog_t		generated_by;	/* System generating header	*/
   hostname_t		source_archie_hostname; /* archie host which generated data	*/
   access_comm_t	access_command;	/* Codename for access script	*/
   date_time_t		retrieve_time;	/* Time of retrieval (GMT) of data	*/
   date_time_t		parse_time;	/* Time of processing (GMT) of data	*/
   date_time_t		update_time;	/* Time of update (GMT) of data	*/
   index_t		no_recs;	/* Number of records in data if applicable	*/
   current_status_t	current_status;	/* Current disposition of site	*/
   index_t		fail_count;	/* Number of successive failures to update site */
   flags_t		flags;		/* Miscellaneous info about site */
   access_methods_t	access_methods;
   comment_t		comment;
} old_hostdb_aux_t;



extern status_t rename_host_dbs PROTO((file_info_t *, file_info_t *, file_info_t *));
extern status_t open_old_host_dbs PROTO((file_info_t *, file_info_t *, file_info_t *, int));

#endif

