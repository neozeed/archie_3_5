/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 *
 * The menu API and client were written by Kwynn Buess (buess@isi.edu)
 * Hacked on a bit for robustness by Steven Augart (swa@isi.edu)
 */

#include <usc-license.h>

#include "menu.h"
#include <string.h>
#include <pfs.h>                /* for p_initialize() and VLINK */

void
main(int argc,char *argv[]) 
{ 
    int choice;
    VLINK temp;
    MCS mcstruct_ptr;
    extern int pfs_debug;
    MCS *mc = &mcstruct_ptr;
    char *cp;                   /* scratch char ptr.  */
    int tmp;                    /* return value from call to subfunc.  */


    /* Set the software ID for this client. */
    /* If you are developing a Prospero client, please send email to
       info-prospero@isi.edu to get your own software ID. */
    p_initialize("mPtb", 0, (struct p_initialize_st *) NULL);          
    printf("\n\nProspero Menu Browser\n\n");

    argc--; argv++;

    while (argc > 0)  {
#if 0
        if (!strncmp(argv[0], "-D", 2)) {
            pfs_debug = 9;
            argc--;argv++;
        }
#else
        tmp = qsscanf(argv[0], "-D%r%d", &cp, &pfs_debug);
        if (tmp == 1 || tmp == 2) {
            argc--,argv++;
            if (tmp == 1) pfs_debug = 1;
            continue;
        }
        tmp = qsscanf(argv[0], "-N%r%d", &cp, &ardp_priority);
        if (tmp == 1 || tmp == 2) {
            argc--,argv++;
            if (tmp == 1) pfs_debug = ARDP_MAX_PRI;
            if(ardp_priority > ARDP_MAX_SPRI) 
                ardp_priority = ARDP_MAX_PRI;
            if(ardp_priority < ARDP_MIN_PRI) 
                ardp_priority = ARDP_MIN_PRI;
            continue;
        }
#endif
    }
    init_menu(mc,set_environ(argc,argv));
    while (1) { 
        print_current_menu(mc);
        choice = query_choice(mc);
        
        if (choice == -1) continue;

        /*    if (choice == -1) {
              really_quit();
              }
              else if (choice == -2) printf("\nINVALID CHOICE.  TRY AGAIN.\n");
              */
        /*else*/    if (choice == 0) { 
            if (!top_menu(mc)) up_menu(mc);
            else printf("\nThere are no higher menus.\n");
        } 
        else {
      temp = return_choice(choice,mc);
      if (temp == NULL) printf("\nINVALID CHOICE.  TRY AGAIN.\n");
      else if (m_class(temp) == M_CLASS_MENU
               || m_class(temp) == M_CLASS_SEARCH) { 
          if (m_class(temp) == M_CLASS_MENU) get_menu(mc,temp);
          else get_search(mc,temp);
          if (is_empty(mc)) { 
              up_menu(mc);
              if (m_class(temp) == M_CLASS_MENU)
                  printf("\nNO FILES IN %s!  REPEATING PREVIOUS MENU...\n",
                         m_item_description(temp));
              else 
                  printf("No files were found as a result of the search %s. \
Repeating previous menu.\n", m_item_description(temp));
          } /* if empty */
      } /* if menu */
      else if (m_class(temp) == M_CLASS_DOCUMENT) open_and_display(temp);
      else if (m_class(temp) == M_CLASS_PORTAL) open_telnet(temp);
      else if (m_class(temp) == M_CLASS_DATA) open_data(temp);
      else printf("\nThis type is not implemented yet.\n");
  } /* if choice > * */
    } /* infinite loop */
} /* main() */



