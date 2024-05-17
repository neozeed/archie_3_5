#ifndef CLASS_H
#define CLASS_H

#include "ansi_compat.h"
#include "p_menu.h"
#include "prosp.h"


/*  
 *  Based on the M_CLASS_* defines.
 */  
enum
{
  B_CLASS_UNKNOWN  = 0,
  B_CLASS_MENU     = 1,
  B_CLASS_DOCUMENT = 2,
  B_CLASS_PORTAL   = 4,
  B_CLASS_SEARCH   = 8,
  B_CLASS_DATA     = 16,
  B_CLASS_IMAGE    = 32,
  B_CLASS_VIDEO    = 64
};


extern int link_class proto_((VLINK v));
extern int fake_link_class proto_((VLINK v));
extern int get_class proto_((VLINK v));
extern int real_link_class proto_((VLINK v));

#endif
