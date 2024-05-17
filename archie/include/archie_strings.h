/*
 * This file is copyright Bunyip Information Systems Inc., 1993. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */


#ifndef _ARCHIE_STRINGS_H_
#define _ARCHIE_STRINGS_H_

#include "typedef.h"
#include "ansi_compat.h"

extern          int             splitwhite PROTO((const char *, int *, char ***));
extern          int             strsplit PROTO((const char *, const char *, int *, char ***));
extern		int		strrcmp PROTO((char *, char *));
extern		int		strrncmp PROTO((char *, char *, int));
extern		char*		make_lcase PROTO(( char * ));
extern		status_t	str_decompose();
extern          char *          strndup PROTO((const char *, int));
extern	        char **		str_sep PROTO((const char *, int));
extern	        char **		str_sep_single_free PROTO((const char *, int));
extern	        void		free_opts PROTO((char **));
extern	        void		insert_char PROTO((char *, int, int));
extern	        void		delete_char PROTO((char *, int, int));
extern	        int		cmp_strcase_ptr PROTO((char **, char **));


#endif
