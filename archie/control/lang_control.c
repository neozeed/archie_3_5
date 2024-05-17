char*	 ARCONTROL_001	      = "Error while trying to set master database directory";
char*	 ARCONTROL_002	      = "Error while trying to set host database directory";
char*	 ARCONTROL_003	      = "Unknown control function";
char*	 ARCONTROL_004	      = "Can't get list of files to process";
char*	 ARCONTROL_005	      = "Can't allocate space for internal list";
char*	 ARCONTROL_006	      = "Can't properly preprocess input files";
char*	 ARCONTROL_007	      = "Error while trying to perform control actions";
char*	 ARCONTROL_008	      = "Error while trying to free internal list";
char*	 ARCONTROL_009	      = "No files found for processing";

/* preprocess_file */

char*	 PREPROCESS_FILE_001  = "Can't open input file %s";
char*	 PREPROCESS_FILE_002  = "Can't read header of %s";
char*	 PREPROCESS_FILE_003  = "Error while trying to open host database";
char*	 PREPROCESS_FILE_004  = "Error with %s: %s";
char*	 PREPROCESS_FILE_005  = "Can't write failure record to host databases";
char*	 PREPROCESS_FILE_006  = "Can't unlink() failure input %s";
char*	 PREPROCESS_FILE_007  = "Can't get temporary name for file %s. Ignoring";
char*	 PREPROCESS_FILE_008  = "Can't open temp file %s";
char*	 PREPROCESS_FILE_009  = "Unknown format for %s. Ignoring";
char*	 PREPROCESS_FILE_010  = "Unknown control function %d. Aborting.";
char*	 PREPROCESS_FILE_011  = "Failure writing header. Ignoring %s";
char*	 PREPROCESS_FILE_012  = "Can't execl() preprocess program %s";
char*	 PREPROCESS_FILE_013  = "Can't dup2() input file";
char*	 PREPROCESS_FILE_014  = "Can't dup2() output file";
char*	 PREPROCESS_FILE_015  = "Can't vfork() preprocess program %s";
char*	 PREPROCESS_FILE_016  = "Error while in wait() for preprocess program %s";
char*	 PREPROCESS_FILE_017  = "Preprocess program %s exited abnormally with signal %u";
char*	 PREPROCESS_FILE_018  = "Preprocess program %s terminated abnormally with signal %u";
char*	 PREPROCESS_FILE_019  = "Can't unlink() original input data file %s";
char*	 PREPROCESS_FILE_020  = "Can't close %s";
char*	 PREPROCESS_FILE_021  = "Can't rename temporary file %s to %s";
char*	 PREPROCESS_FILE_022  = "Can't rename delete header file from %s to %s";
char*	 PREPROCESS_FILE_023  = "Can't unlink input file %s";
char*	 PREPROCESS_FILE_024  = "Can't unlink() temporary data file %s";
char*	 PREPROCESS_FILE_025  = "Can't rename invalid data file %s to %s";
char*	 PREPROCESS_FILE_026  = "Can't fstat() file %s" ;
char*	 PREPROCESS_FILE_027  = "File %s is empty. Ignoring";

char*	 CNTL_CHECK_HOSTDB_001=	"Host %s: %s";
char*	 CNTL_CHECK_HOSTDB_002= "Error trying to insert new host %s into primary database";
char*	 CNTL_CHECK_HOSTDB_003= "Error trying to insert new host %s into primary database";
char*	 CNTL_CHECK_HOSTDB_004= "Host %s cannot be resolved. Host unknown.";


char*	 CNTL_FUNCTION_001    = "Unknown function type: %d";
char*	 CNTL_FUNCTION_002    = "Can't execl() program %s for %s database";
char*	 CNTL_FUNCTION_003    = "Can't vfork() program %s";
char*	 CNTL_FUNCTION_004    = "Error while in wait() for program %s";
char*	 CNTL_FUNCTION_005    = "Program %s exited with value %u";
char*	 CNTL_FUNCTION_006    = "Program %s terminated abnormally with signal %u";
char*	 CNTL_FUNCTION_007    = "Can't unlink() orginal input data file %s";
char*	 CNTL_FUNCTION_008    = "Unknown control function";
char*	 CNTL_FUNCTION_009    = "Can't kill child %d";
char*	 CNTL_FUNCTION_010    = "Timeout or terminate signal: %s %s. Terminated";
char*	 CNTL_FUNCTION_011    = "Can't open input file %s for error header";
char*	 CNTL_FUNCTION_012    = "Can't open output file %s for error header";
char*	 CNTL_FUNCTION_013    = "Error while trying to read header of input file %s";
char*	 CNTL_FUNCTION_014    = "Can't unlink input file %s";
char*	 CNTL_FUNCTION_015    = "wait() exited abnormally. Continuing";
char*	 CNTL_FUNCTION_016    = "Can't find child with pid %d in retlist table!";
char*	 CNTL_FUNCTION_017    = "Program %s (%d) exited with value %d";
char*	 CNTL_FUNCTION_018    = "Program %s (%d) terminated abnormally with signal %d";
char*	 CNTL_FUNCTION_019    = "Running %s on %s";
char*	 CNTL_FUNCTION_020    = "Can't malloc space for argument list";

char*	 WRITE_HOSTDB_FAILURE_001   = "%s does not exist in database. Shouldn't happen";
char*	 WRITE_HOSTDB_FAILURE_002   = "Can't write failure to primary host database for %s";
char*	 WRITE_HOSTDB_FAILURE_003   = "Can't update hostaux rec for %s";
