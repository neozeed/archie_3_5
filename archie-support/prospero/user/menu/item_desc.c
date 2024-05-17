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
char *
get_item_desc(VLINK vl) 
{
  PATTRIB highest = NULL;
  PATTRIB temp;
  int compare_precedence(char,char);
  struct token *temp_tok;
  /* int invisible_interp = 0;		OOPS - not used - Mitra*/


  if (vl == NULL) return NULL;
  if ((temp = vl->lattrib) == NULL) return vl->name;

  while (temp != NULL) { 
    if (!strcmp("MENU-ITEM-DESCRIPTION",temp->aname)) 
      if (highest == NULL) highest = temp;
      else if (
	       compare_precedence(highest->precedence,temp->precedence) == -1
	       )
	highest = temp;
    temp = temp->next;

  } /* while temp != NULL */

  if (highest == NULL) return vl->name;
  

  if ((temp_tok = highest->value.sequence) != NULL) {
    if (temp_tok->token != NULL) 
      return temp_tok->token;
    else return NULL;
  }
  else return NULL;
  
  /* Oops - no way to get to this next line - Mitra*/
  return vl->name;


} /* end get_item_desc() */




