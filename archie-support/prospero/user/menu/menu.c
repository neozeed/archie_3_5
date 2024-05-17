/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * The menu API and client were written by Kwynn Buess (buess@isi.edu)
 */

#include <usc-copyr.h>



#include "menu.h"
#include "config.h"
#include <stdio.h>
#include <psite.h>
#include <perrno.h>


extern char *m_error; 
static char error_buffer[300];  /* XXX Will have to replace this with the
                                   flexible-length general Prospero string
                                   handling routines. */

#define ck_fatal_err if ((m_error) != NULL) {  fprintf(stderr,"%s",m_error);  exit(1);}


VLINK set_environ(int argc,char *argv[]) {
  int err;
  VLINK vl,vl2,vl3,vl4;
  VDIR_ST dir_st;
  VDIR dir = &dir_st;
  char *err_msg_1 = "\nNo initial directory is available\n\n"; 

  if (argc == 0) {
    vl = rd_vlink("/MENU");
    if (vl != NULL) return vl;

#ifdef MENU_DEFAULT_HOST
#ifdef MENU_DEFAULT_DIRECTORY
    vl          = vlalloc();
    vl->hsoname = stcopy(MENU_DEFAULT_DIRECTORY);
    vl->host    = stcopy(MENU_DEFAULT_HOST);
    return vl;
#endif 
#endif
#ifndef P_SITE_DIRECTORY
    fprintf(stderr,"%s",err_msg_1);
    return NULL;
#endif
#ifndef P_SITE_HOST
    fprintf(stderr,"%s",err_msg_1);
    return NULL;
#endif    
    
    vl = vlalloc();
    vl->hsoname = stcopy(P_SITE_DIRECTORY);
    vl->host    = stcopy(P_SITE_HOST);

    vdir_init(dir);
    if (p_get_dir(vl,P_SITE_MASTER_VSLIST,dir,GVD_FIND,NULL) 
	!= PSUCCESS)
      {
	fprintf(stderr,"%s",err_msg_1);
	return NULL;
      } 
    vllfree(vl); 
    vl2 = dir->links;
    if (vl2 == NULL) return NULL;

    if (p_get_dir(vl2,P_PROTOTYPE_VS,dir,GVD_FIND,NULL) 
	!= PSUCCESS) 
      { 
	fprintf(stderr,"%s",err_msg_1);
	return NULL;
      }

/*    vllfree(vl2); */

    vl3 = dir->links;
    if (vl3 == NULL) return NULL;

    if (p_get_dir(vl3,"ROOT",dir,GVD_FIND,NULL) != PSUCCESS) { 
      fprintf(stderr,"%s",err_msg_1);
      return NULL;
    }
/*    vllfree(vl3); */

    vl4 = dir->links;
    if (vl4 == NULL) return NULL;

    if (p_get_dir(vl4,"MENU",dir,GVD_FIND,NULL) != PSUCCESS) { 
      fprintf(stderr,"%s",err_msg_1);
      return NULL;
    }
/*    vllfree(vl4); */
    if (dir->links != NULL) return (dir->links);

    return NULL;
  }
  else if (argc == 1) { 
    vl = rd_vlink(argv[0]);
    if (vl == NULL) { 
      m_error = error_buffer;
      sperrmesg(error_buffer,"ERROR: ", 0 ,NULL);
      return NULL;
    }
    return vl;
  }
  else if (argc == 3) { 
    argc--;argv++;
    
    vl = vlalloc();
    vl -> host = stcopy(argv[0]);
    vl -> hsoname = stcopy(argv[1]);
    
    return vl;

  }
      
  return NULL;
}



void init_menu(MCS *mc,VLINK first_link) { 
  VLINK temp;
  MCS mcs = (MCS) stalloc(sizeof(struct menu_control_struct));;

  if (mcs == NULL) { 
    fprintf(stderr,"\nOUT OF MEMORY!!!\n\n");
    exit(1);
  }
  temp = first_link;
  if (temp == NULL) {
    fprintf(stderr,"\nNo initial menu could be found\n\n");
    exit(1);
  }

  mcs->warning = NULL;
  *p_warn_string = '\0';
  mcs->current = m_get_menu(temp);
  if (*p_warn_string) 
      mcs->warning = tkappend(p_warn_string, mcs->warning);
  p_warn_string[0] = '\0';
  ck_fatal_err;
  mcs->earlier = mcs->later = NULL;
  mcs->name_of_parent = stcopy(m_item_description(temp));
  ck_fatal_err;
  vlfree(temp);
  (*mc) = mcs;
}

void get_menu(MCS *mcs,VLINK parent) { 

  MCS new = (MCS) stalloc(sizeof(struct menu_control_struct));
  
  if (new == NULL) {
    fprintf(stderr,"\nOUT OF MEMORY!!!\n\n");
    exit(1);
  }

  (*mcs)->later = new;
  new->earlier = (*mcs);
  new->name_of_parent = stcopy(m_item_description(parent));
  new->later = NULL;
  new->warning = NULL;
  *p_warn_string = '\0';
  new->current = m_get_menu(parent);
  if (*p_warn_string) 
      new->warning = tkappend(p_warn_string, new->warning);
  p_warn_string[0] = '\0';
  (*mcs) = new;
  if (m_error != NULL) { 
    fprintf(stderr,"%s\n",m_error);
    printf("\n\nRepeating previous menu\n");
    m_error = NULL;             /* clear error flag */
    up_menu(mcs);
  }    
}


void get_search(MCS *mcs,VLINK parent) 
{ 
  MCS new = (MCS) stalloc(sizeof(struct menu_control_struct));
  
  if (new == NULL) {
    fprintf(stderr,"\nOUT OF MEMORY!!!\n\n");
    exit(1);
  }

  (*mcs)->later = new;
  new->warning = NULL;
  new->earlier = (*mcs);
  new->name_of_parent = stcopy(m_item_description(parent));
  new->later = NULL;
  new->warning = NULL;
  *p_warn_string = '\0';
  new->current = open_search(parent);
  if (*p_warn_string) 
      new->warning = tkappend(p_warn_string, (new)->warning);
  (*mcs) = new;
  p_warn_string[0] = '\0';
  if (m_error != NULL) { 
    fprintf(stderr,"%s\n",m_error);
    printf("\n\nRepeating previous menu\n");
    m_error = NULL;             /* clear error flag */
    up_menu(mcs);
  }    
}


/* Read value from user and return */
int query_choice(MCS *mcs) { 
  
  

  printf("\nPress a number to select a menu item, q to Quit");
  if (!top_menu(mcs)) printf(", u to go up a menu");
  printf(": ");

  return get_top_level_answer();
/*
  if (answer[0] == 'q') return -1;
  if (answer[0] == 'u') return 0 ;
*/
/*
  for (cnt=0; 
       (isdigit(answer[cnt])) 
       && (answer[cnt] != '\0')
       && (answer[cnt]!= '\n')
       ; 
       cnt++);

  if (answer[cnt] == '\n') answer[cnt] = '\0';
  temp = atoi(answer);


  if (temp == 0) return -2;
*/
/* If the answer string got to the end that means that all components were 
   digits and it's a valid number (values that are too high are handled 
   in return_choice().
*/
/*  if (answer[cnt] == '\0') return atoi(answer); */
/* If it ended because there were no digits, then it's invalid. */
/*  else return -2; */
}

int top_menu(MCS *mcs) { 
  if ((*mcs)->earlier == NULL) return 1;
  else return 0;
}

void up_menu(MCS *mcs) { 
  if ((*mcs)->current != NULL) {
    vllfree((*mcs)->current);
  }
  stfree((*mcs)->name_of_parent);
  (*mcs) = (*mcs)->earlier;
  stfree((*mcs)->later);
  (*mcs)->later = NULL;
}

void print_current_menu(MCS *mcs) { 
  int cnt = 0;
  VLINK trans = (*mcs)->current;
  char *temp;
  int temp_cl;

  printf("\n\nMenu for %s\n",(*mcs)->name_of_parent);
  
  if ((*mcs)->warning) {
      TOKEN tk;
      puts("Got the following warning(s) from Prospero:");
      for (tk = (*mcs)->warning; tk; tk = tk->next)
          puts(tk->token);
      tkfree((*mcs)->warning);  (*mcs)->warning = NULL;
  }
  putchar('\n');
  while (trans != NULL) { 
    temp = m_item_description(trans);
    if (!(temp == NULL || trans->linktype == 'I')) { 
      printf("%4d",++cnt);
      if ((temp_cl = m_class(trans)) == M_CLASS_MENU) printf(">  ");
      else if (temp_cl == M_CLASS_PORTAL) printf("]  ");
      else if (temp_cl == M_CLASS_SEARCH) printf(":  ");
      else printf(".  ");
      printf("%s\n",temp);
    }
    trans = trans -> next;
  }
}

/* Walks linked list of VLINKS and returns link 'ch' (first is 1) */
VLINK return_choice(int ch,MCS *mcs) { 
  int cnt = 0;
  VLINK temp = (*mcs)->current;

  do {
    if (!(get_item_desc(temp) == NULL || temp->linktype == 'I')) cnt++;
    if (cnt != ch) temp = temp -> next;
  } while (cnt != ch && temp != NULL);
  return temp;
}

int is_empty(MCS *to_check) {
  if ((*to_check)->current == NULL) return 1;
  else return 0;
}  

/* These are not yet implemented; they will soon replace the current
   calls to stalloc(). */

#if 0
MCS
mcsalloc(void)
{
}

void
mcsfree(MCS mcs)
{

}
#endif
