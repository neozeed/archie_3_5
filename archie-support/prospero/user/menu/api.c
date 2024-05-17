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
#include "menu.h"               /* auxiliary functions */
#include <stdlib.h>
#include <sys/file.h>

#ifdef AIX
#include <fcntl.h>		/* SCO UNIX needs O_RDONLY*/
#else
#include <sys/fcntl.h>		/* SCO UNIX needs O_RDONLY*/
#endif
#include <pfs.h>
#include <perrno.h>

char *m_error = NULL;
static char error_buffer[300];  /* This is really kludgy, but it'll be fixed
                                   soon. */


VLINK 
m_top_menu(void) {
  VLINK temp;

  temp = rd_vlink(".");
  if (temp == NULL) { 
    m_error = error_buffer;
    sperrmesg(error_buffer,"ERROR: ",perrno,NULL);
    return NULL;
  }
  m_error = NULL;
  return temp;

}

VLINK 
m_get_menu(VLINK vl) 
{ 
    VDIR_ST dir_st;
    VDIR dir = &dir_st;
    VLINK list;

    vdir_init(dir);

    /* As soon as p_get_dir returns specific attributes, we want 
       MENU-ITEM-DESCRIPTION+COLLATION-ORDER+ACCESS-METHOD to be returned.
       */
    if (p_get_dir(vl,"*", dir,GVD_REMEXP|GVD_ATTRIB|GVD_NOSORT,NULL) != 0) {
        m_error = error_buffer;
        sperrmesg(error_buffer,"ERROR: ",0,NULL);
        vdir_freelinks(dir);
        return NULL;
    }
    list = dir->links; dir->links = NULL;
    vdir_freelinks(dir);
    m_error = NULL;
    return vlink_list_sort(list);
}


/*
int m_is_menu(VLINK vl) { 
  static char * err_msg_1 = "A NULL link was sent to m_is_menu()";
  static char * err_msg_2 = "A link with a NULL target was sent to m_is_menu()";

  if (vl == NULL) { 
    m_error = err_msg_1;
    return 0;
  }
  if (vl->target == NULL) { 
    m_error = err_msg_2;
    return 0;
  }
  m_error = NULL;
  return (strequal(vl->target,"DIRECTORY"));
}

int m_is_text_file(VLINK vl) { 
  static char * err_msg_1 = "A NULL link was sent to m_is_text_file()";
  static char * err_msg_2 = 
    "A link with a NULL target was sent to m_is_text_file()";

  if (vl == NULL) { 
    m_error = err_msg_1;
    return 0;
  }
  if (vl->target == NULL) { 
    m_error = err_msg_2;
    return 0;
  }
  m_error = NULL;
  return 
    ((strequal(vl->target,"FILE")) || (strequal(vl->target,"EXTERNAL")));
}
*/

int 
m_open_file(VLINK vl) { 
  int temp;
  temp = pfs_open(vl,O_RDONLY); 
  if (temp == -1) { 
    m_error = error_buffer;
    sperrmesg(error_buffer,"ERROR: ",perrno,NULL);
    return -1;
  }
  m_error = NULL;
  return temp;
}

FILE *
m_fopen_file(VLINK vl) 
{ 
  FILE *temp;
  temp = pfs_fopen(vl,"r");
  if (temp == NULL) {
    m_error = error_buffer;
    sperrmesg(error_buffer,"ERROR: ",perrno,NULL);
    return NULL;
  }
  m_error = NULL;
  return temp;

}

char *
m_item_description(VLINK vl) 
{ 
  char *temp;
  char *get_item_desc(VLINK);
  static char *buf = NULL;
  static char * err_msg_1 = "NULL link sent to m_item_description()";
  static char * err_msg_2 = 
   "m_item_description could not find any description or name for the given link";

  if (vl == NULL) {
    m_error = err_msg_1;
    return NULL;
  }

  temp = get_item_desc(vl);
  if (temp != NULL) {
    buf = stcopy(temp);
    m_error = NULL;
    return buf;
  }

  m_error = err_msg_2;
  return NULL;
}

#include <string.h>             /* need strrchr() */

/* Does 'name' have the suffix 'suf' ?  Suffices all start with a '.'  which is
   not passed to this function. */
static int
hassuffix(char *name, char *suf)
{
    char *lastdot = strrchr(name, '.');
    if (lastdot) return strequal(lastdot + 1, suf); /* 1 if true */
    else return 0;              /* false */
}

int 
m_class(VLINK vl) 
{ 
    TOKEN type = m_interpretation(vl);
    char *name;

    if (vl == NULL) return M_CLASS_UNKNOWN;

    if (type == NULL) { 
        if (strequal("DIRECTORY",vl->target)) 
            return M_CLASS_MENU;
        else if (strequal("FILE",vl->target) || 
                 strequal("EXTERNAL",vl->target)) {
            char *n = vl->name; /* file name */

            /* If the file has no OBJECT-INTERPRETATION attribute, try 
               to pick an class for it based upon the last component of the
               file name or upon the contents.   This is a transitional
               measure, since at the moment many files lack 
               OBJECT-INTERPRETATION attributes.  */ 
            if (!n 
                || hassuffix(n, "a") /* UNIX library archives */
                || hassuffix(n, "com") /* MS-DOS executables */
                || hassuffix(n, "exe") /* executables from some systems */
                || hassuffix(n, "gif") /* you know :) */
                || hassuffix(n, "gz") /* gnuzip */
                || hassuffix(n, "hqx") /* MAC binary encoding format */
                || hassuffix(n, "jpeg") /* an image format */
                || hassuffix(n, "jpg") /* JPEG from MS-DOS :) */
                || hassuffix(n, "mpeg") /* video format :) */
                || hassuffix(n, "mpg") /* MPEG from ms-dos :) */
                || hassuffix(n, "o") /* Unix .o (object) file. */
                || hassuffix(n, "tar") /* Unix TAR format (tape archive) */
                || hassuffix(n, "tif") /* TIFF files from MS-DOS */
                || hassuffix(n, "tiff") /* TIFF */
                || hassuffix(n, "pbm") /* portable bitmaps */
                || hassuffix(n, "xbm") /* x bit maps */
                || hassuffix(n, "Z") /* compress */
                || hassuffix(n, "z") /* old gnuzip */
                /* A few common cases. */
                || strequal(n, "core") || strequal(n, "a.out"))
                return M_CLASS_DATA;
            else
                return M_CLASS_DOCUMENT;
        } else
            return M_CLASS_UNKNOWN;
    }                           /* type == NULL */
  
    if (strequal(elt(type,0),"PORTAL")) return M_CLASS_PORTAL;
    if (strequal(elt(type,0),"SEARCH")) return M_CLASS_SEARCH;
    if (length(type) >= 3 && strequal(elt(type,0), "DOCUMENT")
        && strequal(elt(type, 1), "TEXT") && strequal(elt(type, 2), "ASCII"))
        return M_CLASS_DOCUMENT;
    if (strequal(elt(type,0),"DIRECTORY")) return M_CLASS_MENU;
    if (strequal(elt(type,0),"DATA")) return M_CLASS_DATA;
    if (strequal(elt(type,0),"EXECUTABLE")) return M_CLASS_DATA;
    if (strequal(elt(type,0),"AGGREGATE")) return M_CLASS_DATA;
    if (strequal(elt(type,0),"IMAGE")) return M_CLASS_DATA;
    if (strequal(elt(type,0),"SOUND")) return M_CLASS_DATA;
    if (strequal(elt(type,0),"VIDEO")) return M_CLASS_DATA;
    if (strequal(elt(type,0),"VIRTUAL-SYSTEM")) return M_CLASS_MENU;
    if (strequal(elt(type,0),"EMBEDDED")) return M_CLASS_DATA;
    if (length(type) >= 2 && strequal(elt(type,0), "DOCUMENT")
        && strequal(elt(type, 1), "MIME"))
        return M_CLASS_DOCUMENT;
    if (strequal(elt(type,0),"SOURCE-CODE")) return M_CLASS_DOCUMENT;
    if (strequal(elt(type,0),"PROGRAM")) return M_CLASS_DOCUMENT;
    /* Any documents this implementation can't display using a standard 
       text display program are M_CLASS_DATA. */
    if (strequal(elt(type,0),"DOCUMENT")) return M_CLASS_DATA;
    /* Unrecognized */
    return M_CLASS_UNKNOWN;
}


TOKEN
m_interpretation(VLINK vl) 
{ 
  return get_token(vl, "OBJECT-INTERPRETATION");
}

