#ifndef _FTP_H_
#define _FTP_H_

#include "archie_inet.h"

#define	FTP_DATACONN_OPEN       125
#define	FTP_OPEN_DATACONN	150
#define	FTP_COMMAND_OK		200    
#define FTP_FILE_STATUS		213
#define	FTP_QUIT		221	/* Quit OK */
#define	FTP_TRANSFER_COMPLETE	226	/* File transfer completed */
#define	FTP_LOGIN_OK		230	/* Login OK */
#define	FTP_FILE_ACTION_OK      250     
#define	FTP_PATHNAME_NONRFC     251
#define	FTP_PATHNAME_CREATED	257	/* Returned on successful PWD */
#define FTP_USER_OK		331     /* Give USER */
#define FTP_ACCT_WANTED		332     /* Give ACCT */
#define	FTP_LOST_CONN		421	/* Lost connection to site */
#define	FTP_CANT_DATACONN	425	/* Can't build data connection */
#define FTP_ABORT_DATACONN	426	/* Data conection aborted */
#define	FTP_FILE_UNAVAILABLE	450	/* File unavailable */
#define	FTP_LOCAL_ERROR		451	/* Action aborted, local processing error */
#define FTP_COMMAND_NOT_IMPL	500     /* Command not implemented */
#define FTP_COMM_PARAM_NOT_IMPL	504     /* Given parameters to command not implemented */
#define	FTP_NOT_LOGGED_IN	530     /* Currently not logged in */
#define	FTP_ACTION_NOT_TAKEN	550	/* Action not taken, file unavailable */

#ifdef __STDC__

extern	int		send_command( );
extern	status_t	get_reply(FILE *, FILE *, int *, int, int);
extern	con_status_t	ftp_connect(hostname_t, int, FILE **, FILE **, int);
extern	int		ftp_login(FILE *, FILE *, char *, char *, char *, int);
extern	status_t	setup_dataconn(FILE **,int *);
extern	char *		get_reply_string(void);
extern	char *		get_request_string(void);

#else

extern	status_t	get_reply();
extern	int		send_command();
extern	con_status_t	ftp_connect();
extern	int		ftp_login();
extern	status_t	setup_dataconn();
extern	char *		get_reply_string();
extern	char *		get_request_string();

#endif


extern	void		put_request_string PROTO((char*));
extern	void		put_reply_string PROTO((char*));


#endif
