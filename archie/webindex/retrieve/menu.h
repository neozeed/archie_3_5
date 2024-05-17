#ifndef _MENU_H_
#define _MENU_H_


#define	 MAX_LINE    2048

#include "typedef.h"
#include "ansi_compat.h"

typedef struct menu_ menu_t;
struct menu_{
   char filetype;
   char *user_str;
   char *sel_str;
   char hostname[MAX_HOSTNAME_LEN];
   int  hostport;
   menu_t *next;
};

extern menu_t  *menu_alloc PROTO(());
extern menu_t  *append_menu PROTO((menu_t *, menu_t *));
extern menu_t  *read_menu PROTO((FILE *, int));



#endif
