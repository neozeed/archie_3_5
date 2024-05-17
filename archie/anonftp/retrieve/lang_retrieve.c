char*	 RETRIEVE_ANONFTP_001	 = "Unknown option %c";
char*	 RETRIEVE_ANONFTP_002	 = "Can't open default log file";
char*	 RETRIEVE_ANONFTP_003	 = "Can't open log file %s";
char*	 RETRIEVE_ANONFTP_004	 = "No input (header) file given";
char*	 RETRIEVE_ANONFTP_005	 = "No output file template specified";
char*	 RETRIEVE_ANONFTP_006	 = "Error while trying to set master database directory";
char*	 RETRIEVE_ANONFTP_007	 = "Can't open input file %s";
char*	 RETRIEVE_ANONFTP_008	 = "Can't read header of input file %s";
char*	 RETRIEVE_ANONFTP_009	 = "Can't close input file %s" ;
char*	 RETRIEVE_ANONFTP_010	 = "Can't malloc() for retrieve defaults";
char*	 RETRIEVE_ANONFTP_011	 = "Error reading configuration file %s";
char*	 RETRIEVE_ANONFTP_012	 = "Error in retrieve from file %s";
char*	 RETRIEVE_ANONFTP_013	 = "Can't unlink input file %s";

char*	 DO_RETRIEVE_001	 = "Unable to parse access command in header";
char*	 DO_RETRIEVE_002	 = "get_archie_hostname() failed. Can't get local host name!";
char*	 DO_RETRIEVE_003	 = "Can't get hostname DNS record for %s";
char*	 DO_RETRIEVE_004	 = "Can't get host address for %s";
char*	 DO_RETRIEVE_005	 = "No default entry for %s in configuration file from %s";
char*	 DO_RETRIEVE_006	 = "Neither access command nor default action provided for %s";
char*	 DO_RETRIEVE_007	 = "Can't find service 'ftp/tcp' in list of services";
char*	 DO_RETRIEVE_008	 = "Error %d while trying to connect to %s";
char*	 DO_RETRIEVE_009	 = "Can't login as %s";
char*	 DO_RETRIEVE_010	 = "Unable to connect: %s"; 
char*	 DO_RETRIEVE_011	 = "Can't get home directory name for site";
char*	 DO_RETRIEVE_012	 = "Unexpected return code";
char*	 DO_RETRIEVE_013	 = "Lost connection";
char*	 DO_RETRIEVE_014	 = "Can't change directory to %s";
char*	 DO_RETRIEVE_015	 = "Unable to get local port for data transfer";
char*	 DO_RETRIEVE_016	 = "Command not implemented";
char*	 DO_RETRIEVE_017	 = "Error in making data connection";
char*	 DO_RETRIEVE_018	 = "Binary access method '%s' is not supported";
char*	 DO_RETRIEVE_019	 = "Can't get file %s. Currently unavailable";
char*	 DO_RETRIEVE_020	 = "Can't get file %s. File does not exist";
char*	 DO_RETRIEVE_021	 = "Unable to perform transfer";
char*	 DO_RETRIEVE_022	 = "Can't rename temporary file %s to %s";
char*	 DO_RETRIEVE_023	 = "Can't get file pointer for data connection";
char*	 DO_RETRIEVE_024	 = "Can't realloc() space for file list";
char*	 DO_RETRIEVE_025	 = "Can't malloc() space for file list";
char*	 DO_RETRIEVE_026	 = "Can't determine action (no access_commands)";

char*	 READ_RETDEFS_001	 = "Can't open configuration file %s";
char*	 READ_RETDEFS_002	 = "Error while parsing line %u in configuration file %s";
char*	 READ_RETDEFS_003	 = "Invalid empty access methods field in configuration file line %d";
char*	 READ_RETDEFS_004	 = "Invalid empty binary access field %d";
char*	 READ_RETDEFS_005	 = "Invalid empty default compress extension field line %d";
char*	 READ_RETDEFS_006	 = "Invalid OS field: %s";
char*	 READ_RETDEFS_007	 = "Invalid empty OS field in configuration file line %d";

char*	 GET_INPUT_001		 = "Can't open output file %s";
char*	 GET_INPUT_002		 = "Can't accept() data connection from %s";
char*	 GET_INPUT_003		 = "Can't shutdown() accepting socket";
char*	 GET_INPUT_004		 = "Can't write header for incoming data to output file %s";
char*	 GET_INPUT_005		 = "Can't spawn process %s";
char*	 GET_INPUT_006		 = "Can't fork() for %s";
char*	 GET_INPUT_007		 = "Error while in wait() for %s";
char*	 GET_INPUT_008		 = "%s exited abnormally with status %u";
char*	 GET_INPUT_009		 = "%s terminated abnormally with signal %u";
char*	 GET_INPUT_010		 = "Can't shutdown() data socket";
char*	 GET_INPUT_011		 = "Insufficient data to compress(1). Empty listings";
char*	 GET_INPUT_012		 = "Timeout of %d minutes on site %s retrieve";
char*	 GET_INPUT_013		 = "Can't fstat socket";
char*	 GET_INPUT_014		 = "Error while in select() for %s";
char*	 GET_INPUT_015		 = "Error while in select() for data transfer";


/* ftp.c */

char*	 FTP_CONNECT_001	 = "Trying to connect to %s on port %u";
char*	 FTP_CONNECT_002	 = "Connected to %s";
char*	 FTP_CONNECT_003	 = "Can't open file I/O for incoming connection";
char*	 FTP_CONNECT_004	 = "Can't open file I/O for outgoing connection";
char*	 FTP_CONNECT_005	 = "Can't set socket options";

char*	 FTP_LOGIN_001		 = "ACCT command '%s' not accepted";
char*	 FTP_LOGIN_002		 = "FTP password '%s' not accepted";
char*	 FTP_LOGIN_003		 = "Login not accepted. Service not currently available";
char*	 FTP_LOGIN_004		 = "Login not accepted. FTP code %u";

char*	 GET_REPLY_001	      = "Error in select() while waiting for reply from remote server";
char*	 GET_REPLY_002	      = "Timed out while waiting for reply from remote ftp server";

char*	 CHECK_FOR_LSLRZ_001  = "WARNING: This listing file '%s%s' may be a soft link";
char*	 CHECK_FOR_LSLRZ_002  = "WARNING: file '%s%s' has date of %s, last retrieve done %s. Not taken";

char*	 SIG_HANDLE_001	      =	"Retrieve program terminated with signal %d";


char*	 DO_ERROR_HEADER_001	 = "Invalid NULL pointer passed to routine";
char*	 DO_ERROR_HEADER_002	 = "Can't rename temporary file %s to %s";
