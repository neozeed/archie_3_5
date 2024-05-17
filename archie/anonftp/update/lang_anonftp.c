
char*	 INSERT_ANONFTP_001 =	"Can't open input file %s";
char*	 INSERT_ANONFTP_002 =	"Can't read header record of %s";
char*	 INSERT_ANONFTP_003 =	"Can't mmap() input file %s";
char*	 INSERT_ANONFTP_004 =	"Can't open anonftp database";
char*	 INSERT_ANONFTP_005 =	"Can't malloc() space for outlist";
char*	 INSERT_ANONFTP_006 =	"Doing hostname lookup";
char*	 INSERT_ANONFTP_007 =	"Error updating host database: %s";
char*	 INSERT_ANONFTP_008 =	"File %s already exists in the database";
char*	 INSERT_ANONFTP_009 =	"Reading input";
char*	 INSERT_ANONFTP_010 =	"Error while processing input. No data has been lost";
char*	 INSERT_ANONFTP_011 =	"Making internal links";
char*	 INSERT_ANONFTP_012 =	"Error while making internal links. No data has been lost";
char*	 INSERT_ANONFTP_013 =	"Can't rename %s to %s. anonftp database not changed.";
char*	 INSERT_ANONFTP_014 =	"Resolving new strings for site %s";
char*	 INSERT_ANONFTP_015 =	"Can't resolve new strings for %s.";
char*	 INSERT_ANONFTP_016 =	"Can't activate host database record for %s";
char*	 INSERT_ANONFTP_017 =	"Can't set up output file %s";
char*	 INSERT_ANONFTP_019 =   "Can't open log file %s";
char*	 INSERT_ANONFTP_020 =	"Can't open default log file";
char*	 INSERT_ANONFTP_021 =	"Error while trying to set master database directory";
char*	 INSERT_ANONFTP_022 =   "Error while trying to set anonftp database directory";
char*	 INSERT_ANONFTP_023 =   "Error while trying to set host database directory";
char*	 INSERT_ANONFTP_024 =   "No site name or address given";
char*	 INSERT_ANONFTP_025 =   "Can't unlink failed output file %s";
char*	 INSERT_ANONFTP_026 =	"Input file %s contains no data after header";
char*	 INSERT_ANONFTP_027 =	"Can't open dest file %s, errno = %d";
char*	 INSERT_ANONFTP_028 =	"Can't open src file %s, errno = %d";
char*	 INSERT_ANONFTP_029 =	"While copying file %s to %s, errno = %d";
char*	 INSERT_ANONFTP_030 =	"File %s does exist in the database. No need to compare";

char*	 SETUP_OUTPUT_FILE_001 = "Number of records %d. Ignoring";
char*	 SETUP_OUTPUT_FILE_002 = "Can't get new file info structure";
char*	 SETUP_OUTPUT_FILE_003 = "Can't get temporary name for file %s.";
char*	 SETUP_OUTPUT_FILE_004 = "Can't open output file %s";
char*	 SETUP_OUTPUT_FILE_005 = "Can't ftruncate output file %s";
char*	 SETUP_OUTPUT_FILE_006 = "Can't mmap() output file %s";

/* setup_insert.c */


char*	 ADD_TMP_STRING_001    = "Can't malloc space for new tmp_list element";

char*	 SETUP_HASH_TABLE_001  = "Can't allocate space for has table";

char*	 SETUP_INSERT_001      = "Can't setup hash table";
char*	 SETUP_INSERT_002      = "Possible zero length strings index file\nIgnore error message if this is a new database";
char*	 SETUP_INSERT_003      = "Can't malloc space for input string";
char*	 SETUP_INSERT_004      = "Can't add string to hash table";
char*	 SETUP_INSERT_005      = "Can't unmap strings index file";
char*	 SETUP_INSERT_006      = "Error while reading '%s' from hashed strings database";


char*	 DO_INTERNAL_001       = "Possible zero length strings index file\nIgnore error message if this is a new database";
char*	 DO_INTERNAL_002       = "Cannot process new or inactive record number %u, string %s";
char*	 DO_INTERNAL_003       = "Can't unmap strings index file" ;

char*	 MAKE_LINKS_001	       = "Error while seeking end of strings_list file";
char*	 MAKE_LINKS_002	       = "Error while extending strings_list file";
char*	 MAKE_LINKS_003	       = "Error while seeking end of strings_idx file";
char*	 MAKE_LINKS_004	       = "Error while seeking end of strings_idx file";
char*	 MAKE_LINKS_005	       = "Error while reading last record in strings index file";
char*	 MAKE_LINKS_006	       = "Error while seeking end of strings_idx file";
char*	 MAKE_LINKS_007	       = "Error while seeking last string in strings file";
char*	 MAKE_LINKS_008	       = "Error while reading last string in strings file";
char*	 MAKE_LINKS_009	       = "Error while seeking end of last string in strings_list file";
char*	 MAKE_LINKS_010	       = "Error while seeking beginning of strings_list file";
char*	 MAKE_LINKS_011	       = "Can't insert new string into hash database";
char*	 MAKE_LINKS_012	       = "Error writing strings_idx file";
char*	 MAKE_LINKS_013	       = "Error writing new string in strings_list file";
char*	 MAKE_LINKS_014	       = "Can't mmap() strings index file";
char*	 MAKE_LINKS_015	       = "Can't unmap previous site file";
char*	 MAKE_LINKS_016	       =  "Can't close previous site file";
char*	 MAKE_LINKS_017	       = "Can't open site file for link insertion";
char*	 MAKE_LINKS_018	       = "Can't mmap() site file for link insertion";
char*	 MAKE_LINKS_019	       = "Can't unmap strings index file";



/* delete.c */

char*	 DELETE_ANONFTP_001    = "No site name or address given";
char*	 DELETE_ANONFTP_002    = "Error while trying to set master database directory";
char*	 DELETE_ANONFTP_003    = "Error while trying to set anonftp database directory";
char*	 DELETE_ANONFTP_004    = "Error while trying to set host database directory";
char*	 DELETE_ANONFTP_005    = "Error while trying to open anonftp database";
char*	 DELETE_ANONFTP_006    = "Error while trying to open host database";
char*	 DELETE_ANONFTP_007    = "Can't determine input file for site %s";
char*	 DELETE_ANONFTP_008    = "Can't find input database file for %s";
char*	 DELETE_ANONFTP_009    = "Error while trying to inactivate %s in host database";
char*	 DELETE_ANONFTP_010    = "Error while trying to set up deletion of %s";
char*	 DELETE_ANONFTP_011    = "Can't remove the site file %s";
char*	 DELETE_ANONFTP_012    = "Can't close input file %s";
char*	 DELETE_ANONFTP_013    = "Can't open default log file";
char*	 DELETE_ANONFTP_014    = "Can't open log file %s";

/* setup_delete.c */

char*	 SETUP_DELETE_001     = "Can't malloc space for internal list";
char*	 SETUP_DELETE_002     = "No input file %s";
char*	 SETUP_DELETE_003     = "Can't mmap site file to be deleted %s";
char*	 SETUP_DELETE_004     = "Can't mmap strings index file";
char*	 SETUP_DELETE_005     =	"Can't open site file %s";
char*	 SETUP_DELETE_006     = "Can't mmap site file %s";
char*	 SETUP_DELETE_007     = "Can't unmap site file %s";
char*	 SETUP_DELETE_008     = "Can't close site file %s";
char*	 SETUP_DELETE_009     = "Can't unmap strings index file";
char*	 SETUP_DELETE_010     = "Can't free internal list";
char*	 SETUP_DELETE_011     = "Can't fstat() input site file %s";
char*	 SETUP_DELETE_012     = "Number of records in host auxiliary database (%d) different from implied by file size (%d)";
char*	 SETUP_DELETE_013     = "Site %s record %d points to itself (previous)";
char*	 SETUP_DELETE_014     = "Site %s record %d points to itself (next)";
char*	 SETUP_DELETE_015     = "Site %s record %d points to already checked record";

/* parse_anonftp.c */

char*	 PARSE_ANONFTP_001    = "Can't open default log file";
char*	 PARSE_ANONFTP_002    = "Can't open log file %s";
char*	 PARSE_ANONFTP_003    = "No input file given";
char*	 PARSE_ANONFTP_004    = "No output file given";
char*	 PARSE_ANONFTP_005    = "Error while trying to set master database directory";
char*	 PARSE_ANONFTP_006    = "Can't open input file %s";
char*	 PARSE_ANONFTP_007    = "Error reading header of input file %s";
char*	 PARSE_ANONFTP_008    = "Input file %s not in raw format. Cannot process";
char*	 PARSE_ANONFTP_009    = "Can't get temporary name for file %s";
char*	 PARSE_ANONFTP_010    = "Can't open temporary file %s";
char*	 PARSE_ANONFTP_011    = "Error while writing header of output file";
char*	 PARSE_ANONFTP_012    = "Can't execlp() filter program %s";
char*	 PARSE_ANONFTP_013    = "Can't vfork() filter program %s";
char*	 PARSE_ANONFTP_014    = "Error while in wait() for filter program %s";
char*	 PARSE_ANONFTP_015    = "Filter program %s exited abnormally with exit code %d";
char*	 PARSE_ANONFTP_016    = "Filter program %s terminated abnormally with signal %d";
char*	 PARSE_ANONFTP_017    = "Can't close input file %s";
char*	 PARSE_ANONFTP_018    = "Can't close temporary file %s";
char*	 PARSE_ANONFTP_019    = "Can't execvp() parse program %s";
char*	 PARSE_ANONFTP_020    = "Can't vfork() parse program %s";
char*	 PARSE_ANONFTP_021    = "Error while in wait() for parse program %s";
char*	 PARSE_ANONFTP_022    = "Parse program %s exited abnormally with code %d";
char*	 PARSE_ANONFTP_023    = "Parse program %s terminated abnormally with signal %d";
char*	 PARSE_ANONFTP_024    = "Can't rename output file %s";
char*	 PARSE_ANONFTP_025    = "Can't unlink input file %s";
char*	 PARSE_ANONFTP_026    = "Can't unlink temporary file %s";
char*	 PARSE_ANONFTP_027    = "Can't malloc space for argument list";
char*	 PARSE_ANONFTP_028    = "Can't rename failed temporary file %s to %s";
char*	 PARSE_ANONFTP_029    = "Unable to parse access command in header";

/* net_anonftp.c */

char*	 NET_ANONFTP_001      = "Can't open default log file";
char*	 NET_ANONFTP_002      = "Can't open log file %s";
char*	 NET_ANONFTP_003      = "Error while trying to set master database directory";
char*	 NET_ANONFTP_004      = "Error while trying to set anonftp database directory";
char*	 NET_ANONFTP_005      = "Error while trying to set host database directory";
char*	 NET_ANONFTP_006      = "Error while trying to open anonftp database";
char*	 NET_ANONFTP_007      = "Error while trying to open host database";
char*	 NET_ANONFTP_008      = "No host specified for output mode";
char*	 NET_ANONFTP_009      = "Can't find requested host %s in local primary host database";
char*	 NET_ANONFTP_010      = "Can't find requested host %s database %s in local auxiliary host database";
char*	 NET_ANONFTP_012      = "Error sending anonftp site file";
char*	 NET_ANONFTP_013      = "Error receiving anonftp site file";
char*	 NET_ANONFTP_014      = "Broken pipe: remote data transfer process existed prematurely";


char*	 GET_ANONFTP_SITE_001 =	"Error reading header of remote site file";
char*	 GET_ANONFTP_SITE_002 = "Error opening local site parser file %s";
char*	 GET_ANONFTP_SITE_003 = "Error writing header of local site parser file %s";
char*	 GET_ANONFTP_SITE_004 = "Can't open XDR stream for stdin";
char*	 GET_ANONFTP_SITE_005 =	"Error in translating XDR input. Deleting output file.";
char*	 GET_ANONFTP_SITE_006 = "Can't rename temporary file %s to %s";
char*	 GET_ANONFTP_SITE_007 = "Can't unlink failed transfer file";
char*	 GET_ANONFTP_SITE_008 = "Can't open pipe to uncompression program";
char*	 GET_ANONFTP_SITE_009 = "Can't dup2 pipe to stdout";
char*	 GET_ANONFTP_SITE_010 = "Can't execute uncompression program %s";
char*	 GET_ANONFTP_SITE_011 = "Can't fork() for uncompression process";
char*	 GET_ANONFTP_SITE_012 = "Can't open incoming pipe";
char*	 GET_ANONFTP_SITE_013 = "Can't open XDR stream for fp";
char*	 GET_ANONFTP_SITE_014 = "Can't fstat() output file %s";
char*	 GET_ANONFTP_SITE_015 = "Transferred %d bytes in %d seconds (%6.2f bytes/sec)";
char*	 GET_ANONFTP_SITE_016 = "Input for %s in compressed format";
char*	 GET_ANONFTP_SITE_017 = "Input for %s not in compressed format";
char*	 GET_ANONFTP_SITE_018 = "Can't open pipe to cat program";
char*	 GET_ANONFTP_SITE_019 = "Can't execute cat program %s";
char*	 GET_ANONFTP_SITE_020 = "Can't fork() for cat process %s";

char*	 SEND_ANONFTP_SITE_001=	"Can't open anonftp database sitefile %s";
char*	 SEND_ANONFTP_SITE_002=	"Can't mmap anonftp database sitefile %s";
char*	 SEND_ANONFTP_SITE_003=	"Can't open XDR stream for stdout";
char*	 SEND_ANONFTP_SITE_004=	"Error translating site file to XDR stream";
char*	 SEND_ANONFTP_SITE_005= "Can't open pipe to compression program";
char*	 SEND_ANONFTP_SITE_006= "Can't dup2 pipe to stdin";
char*	 SEND_ANONFTP_SITE_007= "Can't execute compression program %s";
char*	 SEND_ANONFTP_SITE_008= "Can't open outgoing pipe";
char*	 SEND_ANONFTP_SITE_009= "Can't open XDR stream for compression ftp";
char*	 SEND_ANONFTP_SITE_010= "Can't fork() for compression process";
char*	 SEND_ANONFTP_SITE_011= "Error while writing header of output file";
char*	 SEND_ANONFTP_SITE_012= "Output for %s in compressed format";


char*	 COPY_PARSER_TO_XDR_001= "Can't mmap strings file";
char*	 COPY_PARSER_TO_XDR_002= "Conversion to XDR stream from raw format failed";
char*	 COPY_PARSER_TO_XDR_003= "Conversion of string to XDR opaque type from raw format failed";
char*	 COPY_PARSER_TO_XDR_004= "Can't umap strings file";

char*	 COPY_XDR_TO_PARSER_001= "Conversion to raw format from XDR format failed";
char*	 COPY_XDR_TO_PARSER_002= "Can't write out parser record %d";
char*	 COPY_XDR_TO_PARSER_003= "Conversion of string to parser from XDR format failed for record %d";
char*	 COPY_XDR_TO_PARSER_004= "Can't write out parser string %s record %d";

/* update_anonftp.c */

char*	 UPDATE_ANONFTP_001    = "Can't open log file %s";
char*	 UPDATE_ANONFTP_002    = "Can't open default log file";
char*	 UPDATE_ANONFTP_003    = "No input file given";
char*	 UPDATE_ANONFTP_004    = "Error while trying to set master database directory";
char*	 UPDATE_ANONFTP_005    = "Error while trying to set anonftp database directory";
char*	 UPDATE_ANONFTP_006    = "Can't open given input file %s";
char*	 UPDATE_ANONFTP_007    = "Can't read header on %s";
char*	 UPDATE_ANONFTP_008    = "Error while checking for lockfile %s";
char*	 UPDATE_ANONFTP_009    = "Can't open lock file %s";
char*	 UPDATE_ANONFTP_010    = "Update for %s (%s) at %s";
char*	 UPDATE_ANONFTP_011    = "Giving up after %d tries to update %s";
char*	 UPDATE_ANONFTP_012    = "Deleting %s";
char*	 UPDATE_ANONFTP_013    = "Can't execvp() delete program %s for %s database";
char*	 UPDATE_ANONFTP_014    = "Can't vfork() delete program %s";
char*	 UPDATE_ANONFTP_015    = "Error while in wait() for delete program %s";
char*	 UPDATE_ANONFTP_016    = "Delete program %s exited with value %u";
char*	 UPDATE_ANONFTP_017    = "Delete program %s terminated abnormally with signal %u";
char*	 UPDATE_ANONFTP_018    = "Inserting %s into %s with %s";
char*	 UPDATE_ANONFTP_019    = "Can't execvp() insert program %s for %s database";
char*	 UPDATE_ANONFTP_020    = "Can't vfork() insert program %s";
char*	 UPDATE_ANONFTP_021    = "Error while in wait() for insert program %s";
char*	 UPDATE_ANONFTP_022    = "Insert program %s exited with value %u";
char*	 UPDATE_ANONFTP_023    = "Insert program %s terminated abnormally with signal %d";
char*	 UPDATE_ANONFTP_025    = "Error while trying to open host database";
char*	 UPDATE_ANONFTP_026    = "Can't get auxiliary database entry for site %s database %s for deletion";
char*	 UPDATE_ANONFTP_027    = "Can't write deletion record for %s to auxiliary host database";
char*	 UPDATE_ANONFTP_028    = "Waiting for lock file %s";
char*	 UPDATE_ANONFTP_029    = "Can't unlink original input file %s";
char*	 UPDATE_ANONFTP_030    = "Can't malloc space for argument list";
char*	 UPDATE_ANONFTP_031    = "Input file %s has retrieve time older than current entry. Ignoring.";

/* added on Aug-14-95 to accomodate the set-to-zero change in update_naonftp*/
char*	 UPDATE_ANONFTP_032    = "Can't commit fail-count set to Zero for %s to auxiliary host database";
char*	 UPDATE_ANONFTP_033    = "Can't get auxiliary database entry for site %s database %s for updating information";

/* check_anonftp.c */

char*	 CHECK_ANONFTP_001     = "Can't open default log file";
char*	 CHECK_ANONFTP_002     = "Can't open log file %s";
char*	 CHECK_ANONFTP_003     = "Error while trying to set master database directory";
char*	 CHECK_ANONFTP_004     = "Error while trying to set anonftp database directory";
char*	 CHECK_ANONFTP_005     = "Error while trying to set host database directory";
char*	 CHECK_ANONFTP_006     = "Error while trying to open anonftp database";
char*	 CHECK_ANONFTP_007     = "Error while trying to open host database";
char*	 CHECK_ANONFTP_008     = "Can't find host %s in anonftp database";
char*	 CHECK_ANONFTP_009     = "Can't malloc space for file list";
char*	 CHECK_ANONFTP_010     = "Can't get list of files in directory %s" ;

char*	 CHECK_INDIV_001       = "Can't get host address cache entry for %s";
char*	 CHECK_INDIV_002       = "Checking site %s (%s)";
char*	 CHECK_INDIV_003       = "Ignoring %s";
char*	 CHECK_INDIV_004       = "Number of records in site file %s (%s) %d does not match auxiliary host database record %d";
char*	 CHECK_INDIV_005       = "Can't malloc space for checklist";
char*	 CHECK_INDIV_006       = "Site %s (%s) record %d points to itself!";
char*	 CHECK_INDIV_007       = "Site %s (%s) record %d has string %s but is on chain for record %d with string %s";
char*	 CHECK_INDIV_008       = "Site %s (%s) record %d has string index %d out of bounds";
char*	 CHECK_INDIV_009       = "Site %s (%s) record %d has string %s but is on chain for record %d with string %s";
char*	 CHECK_INDIV_010       = "Site %s (%s) rec %d -> rec %d -> site %s rec %d";
char*	 CHECK_INDIV_011       = "Site %s (%s) record %d points to next record, out of bounds";
char*	 CHECK_INDIV_012       = "Can't mmap strings file";
char*	 CHECK_INDIV_013       = "Site %s (%s) record %d points to site file %s record %d";
char*	 CHECK_INDIV_014       = "Site %s (%s) record %d points to site file %s record %d out of bounds (max: %d)";
char*	 CHECK_INDIV_015       = "Site %s (%s) record %d (string offset: %d \"%s\") points to site file %s record %d (strings offset: %d \"%s\")";
char*	 CHECK_INDIV_016       = "Site %s (%s) record %d points to site file %s record %d but not reverse (site %s record %d)";
char*	 CHECK_INDIV_017       = "Can't mmap strings index file" ;
char*	 CHECK_INDIV_018       = "Site %s (%s) rec: %d -> strings id rec: %d out of bounds (max: %d)";
char*	 CHECK_INDIV_019       = "Site %s (%s) rec: %d (off: %d \"%s\") -> strings idx rec: %d (off: %d, \"%s\") -> sitefile %s rec: %d";
char*	 CHECK_INDIV_020       = "Site %s (%s) rec: %d (offset: %d \"%s\") -> strings index rec: %d -> inactive string (off: %d) \"%s\"";
char*	 CHECK_INDIV_021       = "Site %s (%s) rec: %d (off: %d \"%s\") -> strings idx rec: %d -> string (off: %d \"%s\")";
char*	 CHECK_INDIV_022       = "Site %s (%s) rec %d -> site %s rec %d -> site %s rec %d";


      

