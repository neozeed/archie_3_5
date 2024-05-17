/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * The menu API and client were written by Kwynn Buess (buess@isi.edu)
 */

#include <usc-copyr.h>



#include <pfs.h>

#define M_CLASS_UNKNOWN 0
#define M_CLASS_MENU 1
#define M_CLASS_DOCUMENT 2
#define M_CLASS_PORTAL 4
#define M_CLASS_SEARCH 8
#define M_CLASS_DATA 16
#define M_CLASS_IMAGE 32

VLINK m_top_menu(void);
VLINK m_get_menu(VLINK);
int m_open_file(VLINK);
FILE *m_fopen_file(VLINK);
char *m_item_description(VLINK);
TOKEN m_interpretation(VLINK);
int m_class(VLINK);

extern char *m_error;
