#include "archstridx.h"
#include "patrie.h"
#ifndef _CHECK_ANONFTP_H_
#define	_CHECK_ANONFTP_H_

typedef struct{
   char p;
   char n;
} chklist_t;

#ifdef __STDC__

extern	 status_t check_indiv(char **, struct arch_stridx_handle *, file_info_t *, file_info_t *, int);

#else

extern	 status_t check_indiv();

#endif

#endif
