/* new_Info_cell.c */

char* NEW_INFO_CELL_001	      = "Error malloc'ing %ld bytes";

/* input.c */

char* INPUT_001		      = "Error from fgets()";

/* output.c */

char* PRINT_CORE_INFO_001	 = "Error fwrite'ing parser_entry_t structure" ;
char* PRINT_CORE_INFO_002	 = "Error fwrite'ing file name";
char* PRINT_CORE_INFO_003	 = "Error fwrite'ing padding";

/* parse.c */

char* PARSE_ANONFTP_001	       = "Can't open file %s for reading";
char* PARSE_ANONFTP_002	       = "Can't open file %s for writing";
char* PARSE_ANONFTP_003	       = "Error from read_header() file %s";
char* PARSE_ANONFTP_004	       = "Can't initialize line parser";
char* PARSE_ANONFTP_005	       = "Error from set_elt_type()";
char* PARSE_ANONFTP_006	       = "Can't initialize directory stack";
char* PARSE_ANONFTP_007	       = "Error trying to initialize root directory";
char* PARSE_ANONFTP_008	       = "Unexpected %s line #%d in listing";
char* PARSE_ANONFTP_009	       = "Error initializing directory stack with root directory";
char* PARSE_ANONFTP_010	       = "Unknown state = '%d' on line #%d";
char* PARSE_ANONFTP_011	       = "Error from S_handle_file() on line #%d: aborting";
char* PARSE_ANONFTP_012	       = "Error from handle_dir_start() on line #%d: aborting";
char* PARSE_ANONFTP_013	       = "Unknown state = '%d' on line #%d";
char* PARSE_ANONFTP_014	       = "Error at line #%d: '%s'";
char* PARSE_ANONFTP_015	       = "Error from output_header(): aborting";
char* PARSE_ANONFTP_016	       = "Error from check_stack() on line #%d: aborting";
char* PARSE_ANONFTP_017	       = "Can't open default log file";
char* PARSE_ANONFTP_018	       = "Can't open log file %s";
char* PARSE_ANONFTP_019	       = "Trying to figure out the root_dir %s";
char* PARSE_ANONFTP_020	       = "Cannot figure out root_dir due to usage of standard streams";
char* PARSE_ANONFTP_021	       = "Cannot determine the root_dir";

char* CHECK_STACK_001	       = "Expected listing for the directory '%s' at the end";
char* CHECK_STACK_002	       = "Error removing queue element from top of stack";

char* HANDLE_DIR_START_001     = "Error from S_split_dir()";
char* HANDLE_DIR_START_002     = "Error from pop() while looking for non-empty queue";
char* HANDLE_DIR_START_003     = "directory '";
char* HANDLE_DIR_START_004     = "' was not previously declared.\n";
char* HANDLE_DIR_START_005     = "Error removing queue element from top of stack";
char* HANDLE_DIR_START_006     = "Error from push()";
char* HANDLE_DIR_START_007     = "Error expected listing for directory %s on line %d";


char* HANDLE_FILE_001	       = "Error allocating space for parser record";
char* HANDLE_FILE_002	       = "Error from S_file_parse(), line %d";
char* HANDLE_FILE_003	       = "Error strdup'ing directory name";
char* HANDLE_FILE_004	       = "Error adding directory to queue";
char* HANDLE_FILE_005	       = "Error strdup'ing file name";
char* HANDLE_FILE_006	       = "Error from write_header()";

char* PARSER_OUTPUT_001	       = "Error from first_elt()";
char* PARSER_OUTPUT_002	       = "Error from print_core_info()";

char* USAGE_001		       = "Usage: %s [-h] [-i <input-file>] [-o <output-file>] [-p <prep-dir>] [-r <root-dir>]";

/* queue.c */

char* NEW_QUEUE_ELT_001	       = "Error allocating %d bytes for new queue element";
char* NEW_QUEUE_ELT_002	       = "Error from init_Info_cell()";

/* stack.c */

char* INIT_STACK_001	       = "Root directory '%s' doesn't match pattern";
char* INIT_STACK_002	       = "Error from S_split_dir()";
char* INIT_STACK_003	       = "Error strdup'ing '%s'";
char* INIT_STACK_004	       = "Error from new_Info_cell()";
char* INIT_STACK_005	       = "Error from push()";

char* NEW_STACK_001	       = "Error allocating %d bytes for new stack element";

char* POP_001		       = "Stack is empty";
char* POP_002		       = "Queue is not empty";
char* POP_003		       = "Error from dispose_Info_cell()";

char* PRINT_STACK_001	       = "Stack is empty";

char* PUSH_001		       = "Error from new_Stack()";

/* unix.c */

char* U_S_FILE_PARSE_001	       = "Error looking for white space after %s";
char* U_S_FILE_PARSE_002	       = "Missing file name";
char* U_S_FILE_PARSE_003	       = "Error from ";
char* U_S_FILE_PARSE_004	       = "Error extracting field";
char* U_S_FILE_PARSE_005	       = "Error from S_file_type()";
char* U_S_FILE_PARSE_006         = "can't find ` -> ' in name field of `%s' (line #%d)";

char* S_SPLIT_DIR_001	       = "Expected ':' at end of path, but found '%c'";

/* unix2.c */

char* U2DB_TIME_001		 = "Error in date '%s'";
char* U2DB_TIME_002	         = "Error in time '%s'";
char* U2DB_TIME_003		 = "Error in year '%s'";
char* U2DB_TIME_004		 = "Day-of-month or year value error in '%s %s %s'";
char* U2DB_TIME_005		 = "Hour or minute value error in '%s'";
char* U2DB_TIME_006		 = "Error from timelocal()";
char* U2DB_TIME_007		 = "Error from timegm()";

char* U2DB_PERM_001		 = "Unknown permission character %c, line %s";

/* vms.c */

char* S_DUP_DIR_NAME_001	 = "'%S' doesn't contain a '.'";

char* V_S_FILE_PARSE_001		 = "Error looking for white space after %s";
char* V_S_FILE_PARSE_002		 = "Error extracting field";
char* V_S_FILE_PARSE_003		 = "Error from S_file_type()";

char* S_FILE_TYPE_001		 = "Can't find '.' in file name '%s'";

/* vms2.c */

char* V2DB_DATE_001	      =	"Error in date '%s'";
char* V2DB_DATE_002	      = "Error in time '%s'";
char* V2DB_DATE_003	      = "Day-of-month or year value error in '%s'";
char* V2DB_DATE_004	      = "Hour or minute value error in '%s'";
char* V2DB_DATE_005	      = "Error from timegm()";

char* V2DB_ONE_SET_PERM_001   = "Unexpected character ('%c') in permissions '%s'";
