/* host_manage.c */

/* main routine */

char*	 HOST_MANAGE_001      = "Can't set master database directory";
char*	 HOST_MANAGE_002      = "Can't set host database directory";
char*	 HOST_MANAGE_003      = "Can't open host database directory";
char*	 HOST_MANAGE_004      = "Can't allocated space for auxiliary host database list";
char*	 HOST_MANAGE_005      = "Error reading host_manage configuration file";
char*	 HOST_MANAGE_054      = "Usage: host_manage [-M <dir>]\n\t[-h <dir>]\n\t[-C <config file>]\n\t[-D <domain list>]\n\t[-H <source hostname>]\n\t[ <sitename> ]";
char*	 HOST_MANAGE_055      = "Can't get hostname of this host";



/* the routine "host_manage"... dumb naming */

char*	 HOST_MANAGE_006      = "Reading host database...";
char*	 HOST_MANAGE_007      = "Can't get list of hostnames from database";
char*	 HOST_MANAGE_008      = "Can't locate host '%s' in local database or DNS";
char*	 HOST_MANAGE_009      = "Internal error. Can't resolve hostname in database";
char*	 HOST_MANAGE_010      = "Can't find %s in primary host database";
char*	 HOST_MANAGE_011      = "Can't find %s in %s";
char*	 HOST_MANAGE_012      = "%d sites in database, hostname set to %s";
char*	 HOST_MANAGE_013      = "1 site in database, hostname set to %s";
char*	 HOST_MANAGE_014      = "New database";
char*	 HOST_MANAGE_015      = "Can't display current record!";
char*	 HOST_MANAGE_016      = "Lose current changes to %s ? ";
char*	 HOST_MANAGE_017      = "Add site";
char*	 HOST_MANAGE_018      = "Hostname required";
char*	 HOST_MANAGE_019      = "No changes to original record";
char*	 HOST_MANAGE_020      = "Do you want to update %s ?";
char*	 HOST_MANAGE_021      = "Can't update host";
char*	 HOST_MANAGE_022      = "Host %s, database %s updated";
char*	 HOST_MANAGE_023      = "Can't get new list of hosts";
char*	 HOST_MANAGE_024      = "Can't find host just updated in database";
char*	 HOST_MANAGE_025      = "Last site in database";
char*	 HOST_MANAGE_026      = "First site in database";
char*	 HOST_MANAGE_027      = "First access method in site";
char*	 HOST_MANAGE_028      = "Last access method in site";
char*	 HOST_MANAGE_029      = "Site %s: %s";
char*	 HOST_MANAGE_030      = "Host %s not found in database";
char*	 HOST_MANAGE_031      = "Can't get auxiliary database entry for host %s with non-NULL access methods";
char*	 HOST_MANAGE_032      = "Error in program";
char*	 HOST_MANAGE_033      = "Unknown command code returned from command parser";
char*	 HOST_MANAGE_034      = "No database '%s' for site %s";
char*	 HOST_MANAGE_035      = "Add database";
char*	 HOST_MANAGE_036      = "Can't realloc space for new auxiliary host list";
char*	 HOST_MANAGE_037      = "Can't allocate space for database list";
char*	 HOST_MANAGE_038      = "Can't update primary host database entry for new database";
char*	 HOST_MANAGE_039      = "Can't update new or modified auxilary host database entry";
char*	 HOST_MANAGE_040      = "Do you want to update %s database ?";
char*	 HOST_MANAGE_041      = "Can't locate host returned from error";
char*	 HOST_MANAGE_042      = "No databases for site %s";
char*	 HOST_MANAGE_043      = "Command currently not implemented";
char*	 HOST_MANAGE_044      = "Do you really want to delete the current database ?";
char*	 HOST_MANAGE_045      = "Can't update auxiliary host database for deletion";
char*	 HOST_MANAGE_046      = "Host %s, database %s scheduled for deletion";
char*	 HOST_MANAGE_047      = "Database %s already scheduled for deletion";
char*	 HOST_MANAGE_048      = "Database %s already deleted";
char*	 HOST_MANAGE_049      = "Schedule early update ?";
char*	 HOST_MANAGE_050      = "Early update already scheduled";
char*	 HOST_MANAGE_051      = "Early update scheduled for %s database %s";
char*	 HOST_MANAGE_052      = "Database already exists for given host";
char*	 HOST_MANAGE_053      = "Can't compile domain list %s";
char*	 HOST_MANAGE_056      = "Can't find auxiliary database %s for %s";
char*	 HOST_MANAGE_057      = "Database %s not deleted. Cannot remove host %s";
char*	 HOST_MANAGE_058      = "Delete: %s. Are you sure ?";
char*	 HOST_MANAGE_059      = "Error while trying to delete database %s. Continue ?";
char*	 HOST_MANAGE_060      = "Can't delete  %s (%s) address cache. Continue ?";
char*	 HOST_MANAGE_061      = "Can't delete  %s from host database";
char*	 HOST_MANAGE_062      = "Database %s is already active"; 


char*	 CHECK_UPDATE_001     = "Error: %s";
char*	 CHECK_UPDATE_002     = "Can't display current record";

char*	 DO_UPDATE_001	      = "Failed: %s";

char*	 ADDTO_AUXDBS_001     = "Can't realloc space for new auxiliary host list";
char*	 ADDTO_AUXDBS_002     = "Can't allocate space for new auxiliary host list";

/* screen.c */
char*	 SETUP_SCREEN_001     = "Window has fewer number of lines than minimum required (%d)";

char*	 DISPLAY_RECORDS_001  = "Unix BSD";
char*	 DISPLAY_RECORDS_002  = "VMS Standard";
char*	 DISPLAY_RECORDS_003  = "Unknown";
char*	 DISPLAY_RECORDS_004  = "Active";
char*	 DISPLAY_RECORDS_005  = "Inactive";
char*	 DISPLAY_RECORDS_006  = "Not supported";
char*	 DISPLAY_RECORDS_007  = "Scheduled for deletion by site administrator";
char*	 DISPLAY_RECORDS_008  = "Scheduled for deletion by local administrator";
char*	 DISPLAY_RECORDS_009  = "New";
char*	 DISPLAY_RECORDS_010  = "Update";
char*	 DISPLAY_RECORDS_011  = "Delete";
char*	 DISPLAY_RECORDS_012  = "Deleted";
char*	 DISPLAY_RECORDS_013  = "Do you want to reactivate the record for %s";
char*	 DISPLAY_RECORDS_014  = "Disabled";
char*	 DISPLAY_RECORDS_015  = "UNKNOWN";
char*	 DISPLAY_RECORDS_016  = "Yes";
char*	 DISPLAY_RECORDS_017  = "No";
char*	 DISPLAY_RECORDS_018  = "Novell";

char*	 NEXT_VALUE_001	      = "This value cannot be changed in this manner";

char*	 SIG_HANDLE_001	      = "Stopping...";
char*	 SIG_HANDLE_002	      = "Quit? ";
char*	 SIG_HANDLE_003	      = "Unexpected signal to signal handler %u";

char*	 PROCESS_INPUT_001    = "Unknown command %s";

char*	 PRINT_INPUT_001      = "Type <space> or <ESC> to change value";

/* dbspecs.c */

char*	 READ_DBSPECS_001     =	"Can't open configuration file %s";

/* enter_update */

/* enter_update.c */

char*	 ENTER_SUPDATE_001	      = "No input file supplied";
char*	 ENTER_SUPDATE_002	      = "Error while trying to set master database directory";
char*	 ENTER_SUPDATE_003	      = "Error while trying to set host database directory";
char*	 ENTER_SUPDATE_004	      = "Error while trying to open host database";
char*	 ENTER_SUPDATE_005	      = "Error in domains specification %s";
char*	 ENTER_SUPDATE_006	      = "Can't get local host name";
char*	 ENTER_SUPDATE_007	      = "Error while trying to enter input data";


char*	 MAKE_ENTRY_001		      = "Error opening input data file";
char*	 MAKE_ENTRY_002		      = "Error trying to mmap input data file";
char*	 MAKE_ENTRY_003		      = "Incorrectly formatted input line # %d";
char*	 MAKE_ENTRY_004		      = "Processing: %s\n";
char*	 MAKE_ENTRY_005		      = "Assuming VMS\n";
char*	 MAKE_ENTRY_006		      = "System type unknown. Ignoring this site\n";
char*	 MAKE_ENTRY_007		      = "Host %s unknown. Ignoring";
char*	 MAKE_ENTRY_008		      = "Given hostname %s doesn't match primary hostname %s\n";
char*	 MAKE_ENTRY_009		      = "Not in given domain(s). Ignoring\n";
char*	 MAKE_ENTRY_010		      = "Site name %s not in fully qualified Domain name format. Ignoring";
char*	 MAKE_ENTRY_011		      = "Picked up %s for preferred name\n";
char*	 MAKE_ENTRY_012		      = "Can't insert %s into primary host database";
char*	 MAKE_ENTRY_013		      = "Can't insert %s into auxiliary host database" ;
char*	 MAKE_ENTRY_014		      = "Can't insert %s into host address cache database";
char*	 MAKE_ENTRY_015		      = "Added %s (as %s)\n";
char*	 MAKE_ENTRY_016		      = "Can't unmap input data file";
char*	 MAKE_ENTRY_017		      = "Can't close input data file";

/* ardomain.c */

char*	 ARDOMAIN_001		      = "Error while trying to set master database directory";
char*	 ARDOMAIN_002		      = "Error while trying to set host database directory";
char*	 ARDOMAIN_003		      = "No domain file. Creating.";
char*	 ARDOMAIN_004		      = "Can't rename %s to %s ";
char*	 ARDOMAIN_005		      = "Can't open domain database";
char*	 ARDOMAIN_006		      = "Can't open input file %s";
char*	 ARDOMAIN_007		      = "\nInsufficient number of arguments (%u) line %u:\n%s\n";
char*	 ARDOMAIN_008		      = "Can't enter domain '%s' (line %u). Possible duplicate";
char*	 ARDOMAIN_009		      = "Loop detected on domain '%s'. Aborting.";
char*	 ARDOMAIN_010		      = "Can't unlink %s";
char*	 ARDOMAIN_011		      = "Can't link %s to %s";
char*	 ARDOMAIN_012		      = "Relinking previous domain database";
char*	 ARDOMAIN_013		      = "Can't get list of domains";


/* dump_hostdb.c */

char*	 DUMP_HOSTDB_001	      = "Error while trying to set master database directory";
char*	 DUMP_HOSTDB_002	      = "Error while trying to set host database directory";
char*	 DUMP_HOSTDB_003	      = "Error while trying to open host databases";
char*	 DUMP_HOSTDB_004	      = "Can't compile given domain list %s";
char*	 DUMP_HOSTDB_005	      = "Can't open output file %s";
char*	 DUMP_HOSTDB_006	      = "Can't get list of hostnames from database";
char*	 DUMP_HOSTDB_007	      = "Can't find database entry for host %s";
char*	 DUMP_HOSTDB_008	      = "Can't get database %s for site %s";

/* restore_hostdb.c */

char*	 RESTORE_HOSTDB_001	      = "Error while trying to set master database directory";
char*	 RESTORE_HOSTDB_002	      = "Error while trying to set host database directory";
char*	 RESTORE_HOSTDB_003	      = "Error while trying to open host databases";
char*	 RESTORE_HOSTDB_004	      = "Can't compile given domain list %s";
char*	 RESTORE_HOSTDB_005	      = "Can't open input file %s";
char*	 RESTORE_HOSTDB_006	      = "Line %d: line too long";
char*	 RESTORE_HOSTDB_007	      = "Line %d: no newline found";
char*	 RESTORE_HOSTDB_008	      = "Line %d: incorrectly formed line" ;

/* clean_db */

char*	 CLEAN_DB_001		      = "Invalid value for timeout: %s";
char*	 CLEAN_DB_002		      = "Can't open default log file";
char*	 CLEAN_DB_003		      = "Can't open log file %s";
char*	 CLEAN_DB_004		      = "Error while trying to set master database directory";
char*	 CLEAN_DB_005		      = "Error while trying to set host database directory";
char*	 CLEAN_DB_006		      = "Error while trying to open host databases";
char*	 CLEAN_DB_007		      = "Can't compile domain list %s";
char*	 CLEAN_DB_008		      = "Can't compile database list %s";
char*	 CLEAN_DB_009		      = "Can't get list of hostnames from host databases";
char*	 CLEAN_DB_010		      = "Can't find site %s in host databases";
char*	 CLEAN_DB_011		      = "Can't decompose auxiliary database list for site %s, %s";
char*	 CLEAN_DB_012		      = "Can't get database %s for site %s";
char*	 CLEAN_DB_013		      = "%s.%s %s";
char*	 CLEAN_DB_014		      = "Can't mark entry %s for deletion" ;
char*	 CLEAN_DB_015		      = "Can't malloc space for argument list";
char*	 CLEAN_DB_016		      = "Running %s on %s, %s";
char*	 CLEAN_DB_017		      = "Can't get temporary file name";
char*	 CLEAN_DB_018		      = "Can't open temporary file %s";
char*	 CLEAN_DB_019		      = "Can't write header entry to clean up %s, %s";
char*	 CLEAN_DB_020		      = "Can't execvp() program %s for %s, %s";
char*	 CLEAN_DB_021		      = "Can't vfork() program %s";
char*	 CLEAN_DB_022		      = "Error while in wait() for program %s";
char*	 CLEAN_DB_023		      = "Program %s exited with value %d";
char*	 CLEAN_DB_024		      = "Program %s aborted with signal %d for %s %s";
char*	 CLEAN_DB_025		      = "Site %s already marked for deletion";


/* dump_supdate */

char*	 DUMP_SUPDATE_001	      = "No output file supplied";
char*	 DUMP_SUPDATE_002	      = "Error while trying to set master database directory";
char*	 DUMP_SUPDATE_003	      = "Error while trying to set host database directory";
char*	 DUMP_SUPDATE_004	      = "Error while trying to open host database";
char*	 DUMP_SUPDATE_005	      = "Error in domains specification %s" ;
char*	 DUMP_SUPDATE_006	      = "Error while trying to dump host database";

char*	 DUMP_ENTRY_001		      = "Error opening output data file %s";
char*	 DUMP_ENTRY_002		      = "Can't get list of hosts to dump";
char*	 DUMP_ENTRY_003		      = "Can't find host %s in host database. Ignoring.";
char*	 DUMP_ENTRY_004		      = "Site %s is VMS. Ignoring";
char*	 DUMP_ENTRY_005		      = "Site %s does not have an anonftp database entry";
char*	 DUMP_ENTRY_006		      = "Site %s has source archie host %s. Ignored";
char*	 DUMP_ENTRY_007		      = "%s.anonftp not active";
char*	 DUMP_ENTRY_008		      = "Error writing output file %s";
