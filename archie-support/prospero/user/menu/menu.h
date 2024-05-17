/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * The menu API and client were written by Kwynn Buess (buess@isi.edu)
 */

#include <usc-copyr.h>



#include "p_menu.h"

struct menu_control_struct {
    VLINK current;
    char *name_of_parent;
    TOKEN warning;		/* warning messages received (if any) */
    
    struct menu_control_struct *earlier;
    struct menu_control_struct *later;
};
typedef struct menu_control_struct *MCS;

VLINK set_environ(int, char *[]);
void init_menu(MCS *, VLINK);
void print_current_menu(MCS *);
int query_choice(MCS *);
int top_menu(MCS *);
void up_menu(MCS *);
VLINK return_choice(int, MCS *);
void get_menu(MCS *, VLINK);
void get_search(MCS *, VLINK);
int is_empty(MCS *);

/* Files that display data or otherwise deal with it. */
void open_and_display(VLINK);
void open_telnet(VLINK);
VLINK open_search(VLINK);
extern TOKEN get_token(VLINK, char *);	/* get_token is defined in objects.c */
extern VLINK vlink_list_sort(VLINK);
extern int get_top_level_answer();
extern MCS mcsalloc(void);
extern void mcsfree(MCS);

#if 0
/* in objects.c.  Not currently used, but appears in the library.  */
int m_file_has_more_than_95_percent_printing_chars(int fd);
#endif
