#ifndef CLIENT_DEFS_H
#define CLIENT_DEFS_H

#define COMMENT_CHAR '#'

/*
   Array lengths.
*/

#define COMMAND_LEN 128		/* maximum length of a user's command */
#define DATE_STR_LEN 30		/* room to hold a string containing the date */
#define ERR_STR_LEN 128		/* enough room to hold an error message */
#define INPUT_LINE_LEN 256	/* length of line from generic input file */
#define REG_EX_LEN 128		/* space for regular expression string */
#define TC_ENT_LEN 1024		/* space to hold a termcap entry */

#endif
