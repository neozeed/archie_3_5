#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <time.h>
#include "protos.h"
#include "stdlib.h"
#include "menu.h"
#include "error.h"

menu_t *menu_alloc()

{  
   menu_t *curr_menu;

   if((curr_menu = (menu_t *) malloc(sizeof(menu_t))) == (menu_t *) NULL){

#if 0
      error();
#endif
      return((menu_t *) NULL);
   }

   memset(curr_menu, '\0', sizeof(menu_t));

   return(curr_menu);
}

menu_t *append_menu(new_menu, curr_menu)
   menu_t *new_menu;
   menu_t *curr_menu;
{

   ptr_check(new_menu, menu_t, "append_menu", (menu_t *) NULL);
   ptr_check(curr_menu, menu_t, "append_menu", (menu_t *) NULL);

   if(curr_menu == new_menu)
      return(curr_menu);

   return (curr_menu -> next = new_menu);
}

int free_menu(head)
   menu_t *head;

{
   menu_t *a, *b;

   if(!head)
      return(0);

   for(a = head; a != (menu_t*) NULL;){

      if(a -> user_str)
	 free(a -> user_str);

      if(a -> sel_str)
	 free(a -> sel_str);

      b = a -> next;
      free(a);
      a = b;
   }

   return(0);
}
