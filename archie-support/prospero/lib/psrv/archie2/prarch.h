/* Error codes returned by prarch routines */
#define PRARCH_SUCCESS		0	/* Successful completion       */
#define PRARCH_BAD_ARG		1	/* Bad argument                */
#define PRARCH_OUT_OF_MEMORY	2	/* Can't allocate enough space */
#define PRARCH_BAD_REGEX	3	/* Bad regular expression      */
#define PRARCH_DONT_HAVE_SITE	4	/* Can't find site file        */
#define PRARCH_CANT_OPEN_FILE	5	/* Can't open DB file          */
#define PRARCH_DB_ERROR		6	/* Database Error              */
#define PRARCH_CLEANUP		7       /* Cleanup failed              */
#define PRARCH_TOO_MANY		8	/* Too many matches            */


/* For constructing link attributes */
#define A2PL_H_IP_ADDR        0x001
#define A2PL_HOSTIP           0x001
#define A2PL_H_OS_TYPE        0x002
#define A2PL_H_TIMEZ          0x004
#define A2PL_LK_LAST_MOD      0x020
#define A2PL_LINK_COUNT       0x040
#define A2PL_LINK_SZ          0x080
#define A2PL_NATIVE_MODES     0x100
#define A2PL_H_LAST_MOD       0x200
#define A2PL_SITEDATE         0x200
#define A2PL_UNIX_MODES       0x800

#define A2PL_ROOT	    0x10000
#define A2PL_ARDIR	    0x40000

/* Structure definitions */
struct site_out_t{
	struct in_addr site_ipaddr;
	db_date site_mod_time;
	char site_name[MAX_HOST_LEN];
        char site_update[SMALL_STR_LEN];
        char site_path[MAX_FILE_NAME];
	site_rec site_ent;
};

typedef struct site_out_t site_out;

char	*get_host_file_name();
struct vlink *atoplink();
