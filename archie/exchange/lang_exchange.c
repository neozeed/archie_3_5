/* main.c */

char*	 ARSERVER_001		 = "Can't open default log file";
char*	 ARSERVER_002		 = "Can't open log file %s";
char*	 ARSERVER_003		 = "Error while trying to set master database directory";
char*	 ARSERVER_004		 = "Error while trying to set host database directory";
char*	 ARSERVER_005		 = "Error while trying to open host database";
char*	 ARSERVER_006		 = "Error reading configuration file %s";
char*	 ARSERVER_007		 = "Authorization error";
char*	 ARSERVER_008		 = "Error while performing server operations";
char*	 ARSERVER_009		 = "Error while performing client operations";
char*	 ARSERVER_010		 = "Error while rewriting configuration file %s";

/* client.c */

char*	 DO_CLIENT_001		 = "Can't connect to %s. Failure no %u";
char*	 DO_CLIENT_002		 = "Can't open file I/O for command connection";
char*	 DO_CLIENT_003		 = "Can't get current time!";
char*	 DO_CLIENT_004		 = "Number of retries (%u) exceeded for %s";
char*	 DO_CLIENT_005		 = "Error writing command C_LISTSITES to remote server %s";
char*	 DO_CLIENT_006		 = "Error reading C_TUPLELIST response from remote server %s";
char*	 DO_CLIENT_007		 = "Unexpected response from remote server %s. Expecting C_TUPLELIST. Got %d";
char*	 DO_CLIENT_008		 = "Error while trying to allocate space for tuplelist";
char*	 DO_CLIENT_009		 = "Error while trying to allocate space for tuplelist";
char*	 DO_CLIENT_010		 = "Processing %d tuples";
char*	 DO_CLIENT_011		 = "Error while trying to allocate space for new tuplelist";
char*	 DO_CLIENT_012		 = "Error while trying to process incoming tuples from %s";
char*	 DO_CLIENT_013		 = "Error while trying to obtain sites from remote server %s";
char*	 DO_CLIENT_014		 = "Error while trying to retrieve headers from remote server %s";
char*	 DO_CLIENT_015		 = "Error sending QUIT command to remote server %s";
char*	 DO_CLIENT_016		 = "Closing connection at %s";
char*	 DO_CLIENT_017		 = "Error closing connection with remote server %s";
char*	 DO_CLIENT_018		 = "This client not authorized to connect to server %s";
char*	 DO_CLIENT_019		 = "Configuration host %s not in command line host list %s";
char*	 DO_CLIENT_020		 = "Attempting to connect to server %s";
char*	 DO_CLIENT_021		 = "Connected to %s";
char*	 DO_CLIENT_022		 = "Server %s database %s not scheduled for update";
char*	 DO_CLIENT_023		 = "Can't expand domains '%s' in output";
char*	 DO_CLIENT_024		 = "Ignoring %s, %s";
char*	 DO_CLIENT_025		 = "Can't determine set of hosts from '%s'";
char*	 DO_CLIENT_026		 = "Can't determine set of databases from '%s'";
char*	 DO_CLIENT_027		 = "Can't extract list of databases '%s' from configuration file";

char*	 SEND_TUPLES_001	 = "Error while trying to send C_TUPLELIST command to remote site";

char*	 GET_TUPLE_LIST_001	 = "Error while reading tuplelist from remote site";
char*	 GET_TUPLE_LIST_002	 = "Input received from remote site not terminated with CR";

char*	 GET_SITES_001		 = "NULL pointer or contents passed for server host";
char*	 GET_SITES_002		 = "Error while trying to send C_SENDSITE command to remote server on %s";
char*	 GET_SITES_003		 = "Error while trying to read command from connection";
char*	 GET_SITES_004		 = "Expected command C_SITELIST not sent";
char*	 GET_SITES_005		 = "Can't connect to server host port %u";
char*	 GET_SITES_006		 = "Unable to open remote connection to remote server";
char*	 GET_SITES_007		 = "Unable to decompose given tuple %s";
char*	 GET_SITES_008		 = "Can't dup2() remote server connection to local transmission";
char*	 GET_SITES_009		 = "Can't execl() receive server %s";
char*	 GET_SITES_010		 = "Can't fork() database server %s";
char*	 GET_SITES_011		 = "Error while in select() for database server %s";
char*	 GET_SITES_012		 = "Database server %s exited abnormally with value %u";
char*	 GET_SITES_013		 = "Database server %s terminated abnormally with signal %d";
char*	 GET_SITES_014		 = "Error while in wait() for database server %s";
char*	 GET_SITES_015		 = "Timeout of %d minutes on site %s exchange";
char*	 GET_SITES_016		 = "Timeout of %d minutes on exchange from server %s for site %s database %s";
char*	 GET_SITES_017		 = "Can't malloc space for argument list";

char*	 GET_HEADERS_001	 = "Unable to decompose tuple %s";
char*	 GET_HEADERS_002	 = "Can't write C_SENDHEADER command to control connection";
char*	 GET_HEADERS_003	 = "Error while trying to read incoming command";
char*	 GET_HEADERS_004	 = "Got unexpected command. Expected C_HEADER";
char*	 GET_HEADERS_005	 = "Error while trying to read incoming header from control connection";
char*	 GET_HEADERS_006	 = "Error while trying to open header file %s";
char*	 GET_HEADERS_007	 = "Error while trying to write header into %s";
char*	 GET_HEADERS_008	 = "Error trying to close header file %s";
char*	 GET_HEADERS_009	 = "Can't rename temporary header file %s to %s";
char*	 GET_HEADERS_010	 = "Site/database already deleted %s. Ignoring";


char*	 SIG_HANDLE_001		 = "program terminated with signal %d";


/* db_functs.c */

char*	 COMPOSE_TUPLES_001	 = "Can't compile domain list %s";
char*	 COMPOSE_TUPLES_002	 = "Can't compile database list %s";
char*	 COMPOSE_TUPLES_003	 = "Can't malloc space for tuple hold list";
char*	 COMPOSE_TUPLES_004	 = "Can't find first entry in hostbyaddr database";
char*	 COMPOSE_TUPLES_005	 = "Located %s in hostbyaddr database. Can't find in primary host database";
char*	 COMPOSE_TUPLES_006	 = "Can't find first entry in hostbyaddr database";
char*	 COMPOSE_TUPLES_007	 = "Can't find next entry in hostbyaddr database";
char*	 COMPOSE_TUPLES_008	 = "Can't realloc space for tuple hold list";
char*	 COMPOSE_TUPLES_009	 = "Can't malloc space for tuple list";
char*	 COMPOSE_TUPLES_010	 = "Can't malloc space for tuple";

char*	 PROCESS_TUPLES_001	 = "Can't decompose incoming tuple %s";
char*	 PROCESS_TUPLES_002	 = "%d sites match given criteria";

char*	 SEND_HEADER_001	 = "Error trying to read primary database entry for tuple %s";
char*	 SEND_HEADER_002	 = "Error trying to obtain auxiliary host database entry for %s";

char*	 SENDSITE_001		 = "Can't get data channel port number";
char*	 SENDSITE_002		 = "Can't issue SITELIST command on connection";
char*	 SENDSITE_003		 = "Can't accept() connection for data channel";
char*	 SENDSITE_004		 = "Error while trying to send header to remote client";
char*	 SENDSITE_005		 = "Can't open file I/O for data channel socket";
char*	 SENDSITE_006		 = "Can't dup2() remote connection to stdin of exchange process";
char*	 SENDSITE_007		 = "Can't execl() send server %s";
char*	 SENDSITE_008		 = "Error in wait() for exchange process %s";
char*	 SENDSITE_009		 = "exchange program %s exited abnormally with value %u";
char*	 SENDSITE_010		 = "Database server terminated abnormally with signal %u";
char*	 SENDSITE_011		 = "Can't close socket";
char*	 SENDSITE_012		 = "Can't fork() database server %s";
char*	 SENDSITE_013		 = "Can't shutdown original socket descriptor";
char*	 SENDSITE_014		 = "Can't shutdown accepting socket";
char*	 SENDSITE_015		 = "Timeout of %d seconds waiting for remote client to connect";
char*	 SENDSITE_016		 = "Error returned from select()";
char*	 SENDSITE_017		 = "Can't malloc space for argument list";

/* server.c */
char*	 DO_SERVER_001		 = "Error while trying to read input command from command connection";
char*	 DO_SERVER_002		 = "Connection closed at %s";
char*	 DO_SERVER_003		 = "Can't compose tuples";
char*	 DO_SERVER_004		 = "Can't send tuples";
char*	 DO_SERVER_005		 = "Unable to send header to client";
char*	 DO_SERVER_006		 = "Unable to write C_HEADER command to client";
char*	 DO_SERVER_007		 = "Unable to write header to client";
char*	 DO_SERVER_008		 = "Unable to send site to remote client";
char*	 DO_SERVER_009		 = "Unable to write requested dump to client";
char*	 DO_SERVER_010		 = "Remote server has forced uncompressed transmission";



char*	 DIE_001		 = "Signal %d received. Exiting";


/* configfile.c */

char*	 READ_ARUPDATE_CONFIG_001= "Can't open configuration file %s";
char*	 READ_ARUPDATE_CONFIG_002= "\nLine %u: Duplicate entry from archie host '%s'. Ignoring.";
char*	 READ_ARUPDATE_CONFIG_003= "Line %u: Can't read database domains";
char*	 READ_ARUPDATE_CONFIG_004= "Line %u: Can't read maximum number of retrieval entries";
char*	 READ_ARUPDATE_CONFIG_005= "Line %u: Can't read database permissions";
char*	 READ_ARUPDATE_CONFIG_006= "Line %u: Can't read database frequency";
char*	 READ_ARUPDATE_CONFIG_007= "Line %u: Can't read database update time";
char*	 READ_ARUPDATE_CONFIG_008= "Line %u: Can't read database failure count";
char*	 READ_ARUPDATE_CONFIG_009= "Duplicate entry for database '%s' in archie host '%s'";
char*	 READ_ARUPDATE_CONFIG_010= "Empty configuration file %s";

char*	 WRITE_ARUPDATE_CONFIG_001= "Can't open configuration file %s";
char*	 WRITE_ARUPDATE_CONFIG_002= "Can't unlink old configuration file %s";
char*	 WRITE_ARUPDATE_CONFIG_003= "Can't link in new configuration file %s";
char*	 WRITE_ARUPDATE_CONFIG_004= "Can't rename temporary file %s to new configuration file %s";

/* net.c */
char*	 READ_NET_COMMAND_001	 = "Read EOF from network connection";
char*	 READ_NET_COMMAND_002	 = "Network command not terminated with CR: %s";
char*	 READ_NET_COMMAND_003	 = "Read: %s";

char*	 WRITE_NET_COMMAND_001	 = "Wrote: %s\r\n";
char*	 WRITE_NET_COMMAND_002	 = "Unknown command %d";

char*	 SEND_ERROR_001		 = "Can't write error command on command connection";

char*	 CHECK_AUTHORIZATION_001	 = "Can't get address of client";
char*	 CHECK_AUTHORIZATION_002	 = "Unable to resolve address for %s";
char*	 CHECK_AUTHORIZATION_003	 = "Client at %s (%s) is not authorized to connect to this server";
char*	 CHECK_AUTHORIZATION_004	 = "Connection from %s (%s) accepted at %s";
char*	 CHECK_AUTHORIZATION_005	 = "Can't determine local hostname";
char*	 CHECK_AUTHORIZATION_006	 = "Can't open dns record for client at %s";
char*	 CHECK_AUTHORIZATION_007	 = "Connection from (local host) %s accepted at %s";
