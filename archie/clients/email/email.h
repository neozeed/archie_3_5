#ifndef _EMAIL_H_
#define _EMAIL_H_

#define	 EMAIL_LOG_FILE	   "email.log"

#define	 EMAIL_TMP_DIR	   "tmp"

#define	 EMAIL_PREFIX	   "archie.email"

#define	 TELNET_PGM_NAME   "telnet-client"

#define	 PATH_COMMAND	   "path"
#define	 QUIT_COMMAND	   "quit"


extern status_t		   saveinput PROTO((file_info_t *, char *));
extern status_t		   process_input PROTO((file_info_t *, char *, int, int));
extern status_t		   readline PROTO((file_info_t *, char *));

#endif
