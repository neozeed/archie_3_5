

/* hostdb.c */


char*	 OPEN_HOST_DBS_001    = "Can't open host address database %s";
char*	 OPEN_HOST_DBS_002    = "Can't open primary host database %s";
char*	 OPEN_HOST_DBS_003    = "Can't open auxiliary host database %s";
char*	 OPEN_HOST_DBS_004    = "Can't open domain database %s";

char*	 SET_HOST_DB_DIR_001  = "Error trying to malloc for host database directory name";
char*	 SET_HOST_DB_DIR_002  = "Can't access host database directory %s";
char*	 SET_HOST_DB_DIR_003  =	"%s is not a directory";

char*	 GET_HOST_ERROR_001   = "Host OK";
char*	 GET_HOST_ERROR_002   = "Preferred/primary names are not the same host";
char*	 GET_HOST_ERROR_003   =	"Preferred hostname (CNAME) cannot be resolved";
char*	 GET_HOST_ERROR_004   = "Host unknown";
char*	 GET_HOST_ERROR_005   =	"Host already stored in database";
char*	 GET_HOST_ERROR_006   = "Primary IP address already exists in database";
char*	 GET_HOST_ERROR_007   =	"Update of primary host database failed";
char*	 GET_HOST_ERROR_008   = "Host stored in database under different name";
char*	 GET_HOST_ERROR_009   =	"Given name is not primary for host";
char*	 GET_HOST_ERROR_010   = "Address in database doesn't match external reference";
char*	 GET_HOST_ERROR_011   = "Update attempted on active host entry";
char*	 GET_HOST_ERROR_012   = "Unknown error ";
char*	 GET_HOST_ERROR_013   = "Host which should exist in database, doesn't";


char*	 DO_HOSTDB_UPDATE_001  = "Host %s: %s";
char*	 DO_HOSTDB_UPDATE_002  = "Entry for %s not in host database";
char*	 DO_HOSTDB_UPDATE_003  = "Can't malloc space for new auxiliary list";
char*	 DO_HOSTDB_UPDATE_004  = "Can't insert %s into primary host database";
char*	 DO_HOSTDB_UPDATE_005  = "Can't insert database %s for %s in aux host database";


char*	 ACTIVATE_SITE_001     = "Can't find database %s for %s in auxiliary database";
char*	 ACTIVATE_SITE_002     = "Can't update database %s record for %s";

char*	 HANDLE_UNKNOWN_HOST_001 = "Site %s unknown to DNS. Site has been marked 'deleted'";

char*	 HANDLE_PADDR_MISMATCH_001 = "Can't find hostname %s in primary host database";
char*	 HANDLE_PADDR_MISMATCH_002 = "Can't find ipaddr record for hostname %s ipaddr %s";
char*	 HANDLE_PADDR_MISMATCH_003 = "Can't insert new IP address %s into host address cache";
char*	 HANDLE_PADDR_MISMATCH_004 = "Can't delete old IP address %s from host address cache";
char*	 HANDLE_PADDR_MISMATCH_005 = "Primary IP address for %s has changed from %s to %s";


/* ops.c */

char*	 UPDATE_HOSTDB_001    =	"Can't modify primary host database";
char*	 UPDATE_HOSTDB_002    = "Can't modify host address cache";

char*	 UPDATE_HOSTAUX_001   = "Invalid hostname parameter";
char*	 UPDATE_HOSTAUX_002   = "Can't update auxiliary host database";
char*  UPDATE_HOSTAUX_003   = "Cannot get access method";

char*	 DELETE_FROM_HOSTDB_001= "Invalid hostname parameter";
char*	 DELETE_FROM_HOSTDB_002= "Can't find host %s with database %s";
char*	 DELETE_FROM_HOSTDB_003= "Can't update entry in primary host database";

char*	 CHECK_NHENTRY_001="preferred name %s doesn't map back to primary name %s but instead to %s";


/* host_info.c */

char*	 GET_PREFERRED_NAME_001	  = "Can't find address in host address cache";
char*	 GET_PREFERRED_NAME_002	  = "Can't find name from host address cache in primary host database";

char*	 SEARCH_IN_PREFERRED_001 = "Invalid hostname parameter";
char*	 SEARCH_IN_PREFERRED_002 = "Error in reading first key of primary host database";

char*	 GET_HOSTNAMES_001	  = "Can't malloc space for hostlist";
char*	 GET_HOSTNAMES_002	  = "Can't realloc space for hostlist";

char*	 GET_HOSTAUX_ENT_001	  = "Can't find database %s for site %s";

/* domain.c */

char*	 COMPILE_DOMAINS_001	  = "Error trying to construct domain list";

char*	 GET_DOMAIN_LIST_001	  = "Error reading list of domains from domain database";
char*	 GET_DOMAIN_LIST_002	  = "Can't malloc() space for intermediate domain list";
char*	 GET_DOMAIN_LIST_003	  = "Empty domain database";
