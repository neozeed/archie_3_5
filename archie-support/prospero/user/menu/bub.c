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
#include "menu.h"

static  VLINK swap(VLINK,VLINK);

/* This prototype of the function performs a bubble sort; this will soon be
   changed.  */
VLINK 
vlink_list_sort(VLINK list) 
{ 
  VLINK temp;
  int compare_collation_ord(VLINK,VLINK);
  VLINK head = list;            /* use the variable 'list' to do final
                                   rearranging.  */
  VLINK next;                   /* used to do final rearranging. --swa */
  int all_in_order = 0;

  if (!head || !head->next) all_in_order = 1; /* singleton or empty list is
                                                 always in order. */
  while (!all_in_order) {

    all_in_order = 1;
    
    if (compare_collation_ord(head,head->next) == -1) 
      head = swap(head,head->next);

      temp = head;
      all_in_order = 1;

      while (temp->next->next != NULL) { 
	if (compare_collation_ord(temp -> next,temp -> next -> next) == -1) {
	  all_in_order = 0;
	  temp -> next = swap(temp->next,temp->next->next);
	}
	temp = temp -> next;
      }
  }
  /* We now have a singly-linked list of VLINKs rooted at 'head'.  */
     
  /* Now reorder the list of links, so that it meets the standard
     doubly-linked-list conventions. --swa */
  list = NULL;
  for (temp = head ; temp; temp = next) {
      next = temp->next;
      APPEND_ITEM(temp, list);
  }
  return list;
}



static VLINK 
swap(VLINK vl1,VLINK vl2) { 

  vl1 -> next = vl2 -> next;
  vl2 -> next = vl1;
  vl1 -> previous = vl2;
  vl2 -> previous = vl1 -> previous;

  
  return vl2;
}
