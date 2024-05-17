/*
 * This file is copyright Bunyip Information Systems Inc., 1992. This file
 * may not be reproduced, copied or transmitted by any means mechanical or
 * electronic without the express written consent of Bunyip Information
 * Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <curses.h>
#include <varargs.h>
#include <ctype.h>
#ifndef SOLARIS
#include <string.h>
#endif
#include "typedef.h"
#include "host_db.h"
#include "host_manage.h"
#include "error.h"
#include "screen.h"
#include "archie_strings.h"
#include "lang_tools.h"
#include "times.h"
#include "protos.h"

/*
 * screen.c: this file contains screen handling routines for the program
 */

int modified = 0;

/* The windows of the screen */

WINDOW *base_win;
WINDOW *hostdb_win, *hostaux_win;
WINDOW *response_win;

/* supplied by the curses libraries */

extern int LINES, COLS;

/* column number of the middle of the screen */

int middle;


char input_buf[BUFSIZ];

/* termcap values */

char bell[16];
char rv_on[16];
char rv_off[16];

#define RV_ON  "mr"
#define	RV_OFF "me"
#define	BELL   "bl"

/* Holders for the values on the screen */

struct{
   hostname_t	     primary_hostname;
   hostname_t	     preferred_hostname;
   access_methods_t   access_methods;
   pathname_t	     os_type;
   pathname_t	     timezone;
} map_screen;

dbspec_t	     *glob_dbspec;

int in_hostlist = 0;
int hostcols[10];
WINDOW *hostlist_win = (WINDOW *) NULL;
hostname_t *tophost;
int curr_width;
int maxsites;
int nocols;
hostname_t *orighost;
hostname_t *local_tophost;
hostname_t *local_hostptr;
pathname_t tmp_str;
extern hostname_t *hostlist;
extern hostname_t *hostptr;
extern int hostcount;

#ifndef SOLARIS
    extern int mvprintw(), mvwprintw();
    extern int wprintw();
    extern int waddch PROTO((WINDOW *, int));
    extern int box PROTO((WINDOW *, int, int));
    extern int winsch PROTO((WINDOW *, int));
#else
   extern char *strcat(char *, const char *);
#endif

/*
 * setup_screen: initialize the curses system and configure the screen
 * template. Initialize global variables.
 */


status_t setup_screen()
{

#if 0
   extern int wmove PROTO((WINDOW *, int, int));
   extern int waddstr PROTO((WINDOW *, char *));
   extern int  stty PROTO((int, struct sgttyb *));
#endif
   char *tmp_ptr;   

   /* Initialize curses */

   initscr();

#if 0
   if(LINES < MIN_NUM_LINES){

      /* "Window has fewer number of lines than minimum required (%d)" */

      error(A_ERR, "setup_screen", SETUP_SCREEN_001, MIN_NUM_LINES);
      return(ERROR);
   }
#endif

   /* setup windows */

   base_win = newwin(1, 0, 0, 0);
   hostdb_win = newwin(5, 0, 1, 0);
   hostaux_win = newwin(18, 0, 6, 0);
   response_win = newwin(1, 0, 23, 0);

   /* Calculate the "middle" of the screen */

   middle = (COLS >> 1) + 5;

   /* Get terminal capabilities */

#if !defined(SOLARIS) && !defined(AIX)
   if((tmp_ptr =  getcap(BELL)) != (char *) NULL)
      strcpy(bell, tmp_ptr);

   if((tmp_ptr =  getcap(RV_ON)) != (char *) NULL)
      strcpy(rv_on, tmp_ptr);

   if((tmp_ptr =  getcap(RV_OFF)) != (char *) NULL)
      strcpy(rv_off, tmp_ptr);
#endif

   /* make the screen template */

   mvwaddstr(base_win, LCUR_PRIMARY_HOSTNAME, 0, CUR_PRIMARY_HOSTNAME);

   mvwaddstr(hostaux_win, LCUR_HOSTAUX_NAME, 0, CUR_HOSTAUX_NAME);
   mvwaddstr(hostaux_win, LCUR_RECORD_NUM_NAME, middle, CUR_RECORD_NUM_NAME);
   
   mvwaddstr(hostdb_win, LCUR_PREFERRED_HOSTNAME, 0, CUR_PREFERRED_HOSTNAME);
   mvwaddstr(hostdb_win, LCUR_ACCESS_METHODS, 0, CUR_ACCESS_METHODS);
   mvwaddstr(hostdb_win,  LCUR_PRIMARY_IPADDR, 0 ,CUR_PRIMARY_IPADDR);

   mvwaddstr(hostdb_win, LCUR_OS_TYPE, 0, CUR_OS_TYPE);   
   mvwaddstr(hostdb_win, LCUR_PROSPERO_HOST, middle, CUR_PROSPERO_HOST);
   mvwaddstr(hostdb_win, LCUR_TIMEZONE, middle, CUR_TIMEZONE);      

   mvwaddstr(hostaux_win, LCUR_GENERATED_BY, middle, CUR_GENERATED_BY);
   mvwaddstr(hostaux_win, LCUR_SOURCE_ARCHIE_HOSTNAME, 0, CUR_SOURCE_ARCHIE_HOSTNAME);
   mvwaddstr(hostaux_win, LCUR_NO_RECS, middle, CUR_NO_RECS);

   mvwaddstr(hostaux_win, LCUR_RETRIEVE_TIME, 0, CUR_RETRIEVE_TIME);
   mvwaddstr(hostaux_win, LCUR_PARSE_TIME, 0, CUR_PARSE_TIME);

   mvwaddstr(hostaux_win, LCUR_UPDATE_TIME, 0, CUR_UPDATE_TIME);      

   mvwaddstr(hostaux_win, LCUR_COMMENT, 0, CUR_COMMENT);

   mvwaddstr(hostaux_win, LCUR_CURRENT_STATUS, 0, CUR_CURRENT_STATUS);
   mvwaddstr(hostaux_win, LCUR_FAIL_COUNT, middle, CUR_FAIL_COUNT);
   mvwaddstr(hostaux_win, LCUR_FORCE_UPDATE, 0, CUR_FORCE_UPDATE);

   mvwaddstr(hostaux_win, LCUR_ACTION, 0, CUR_ACTION);


   /* put us into cbreak (semi-raw) mode */

   noecho();
   cbreak();

#if 0
   wrefresh(hostdb_win);
   wrefresh(hostaux_win);
   wrefresh(response_win);    
   refresh();
#endif

   return(A_OK);

}


/* teardown_screen: terminate the curses routines */

void teardown_screen()

{
   extern int endwin();

   endwin();
}

/*
 * display_records: given a primary host database and auxiliary records
 * display the values on the screen. dpspec_list contains instructions from
 * the configuration file about how to display the access commands
 */


status_t display_records(hostdb_rec, hostaux_rec, dbspec_list, action)
   hostdb_t *hostdb_rec;      /* primary host database record */
   hostdb_aux_t *hostaux_rec; /* aux. host database record */
   dbspec_t *dbspec_list;     /* access_commands configuration */
   action_status_t action;    /* status of currently displayed site */

{

   extern int wmove PROTO((WINDOW *, int, int));
   extern int waddstr PROTO((WINDOW *, char *));
   extern int stty PROTO((int, struct sgttyb *));
   extern int wrefresh PROTO((WINDOW *));

   char *tmp_ptr;
   pathname_t tmp_buf;


   /* check pointers */

   ptr_check(hostdb_rec, hostdb_t, "display_records", ERROR);
   ptr_check(hostaux_rec, hostdb_aux_t, "display_records", ERROR);
   ptr_check(dbspec_list, dbspec_t, "display_records", ERROR);

   /*
    * Print out values. Note that some values are mirrored in map_screen
    * for convenience
    */

   if(hostlist_win != (WINDOW *) NULL)
      return(A_OK);


   mvwprintw(base_win, LCUR_PRIMARY_HOSTNAME, CCUR_PRIMARY_HOSTNAME, "%-50s", hostdb_rec -> primary_hostname);
   strcpy(map_screen.primary_hostname, hostdb_rec -> primary_hostname);


   mvwprintw(hostdb_win, LCUR_PREFERRED_HOSTNAME, CCUR_PREFERRED_HOSTNAME, "%-50s", hostaux_rec -> preferred_hostname);
   strcpy(map_screen.preferred_hostname, hostaux_rec -> preferred_hostname);

   mvwprintw(hostdb_win, LCUR_ACCESS_METHODS, CCUR_ACCESS_METHODS, "%-50s", hostdb_rec -> access_methods);
   strcpy(map_screen.access_methods, hostaux_rec -> origin->access_methods);

   mvwprintw(hostdb_win, LCUR_PRIMARY_IPADDR, CCUR_PRIMARY_IPADDR, "%-16s", inet_ntoa(ipaddr_to_inet(hostdb_rec -> primary_ipaddr)));

   switch(hostdb_rec -> os_type){

      case UNIX_BSD:

	 /* "Unix BSD" */

	 tmp_ptr = DISPLAY_RECORDS_001;
	 break;

      case VMS_STD:

	 /* "VMS Standard" */

	 tmp_ptr = DISPLAY_RECORDS_002;
	 break;

      case NOVELL:

	 /* "Novell" */

	 tmp_ptr = DISPLAY_RECORDS_018;
	 break;

      default:

	 /* "Unknown" */

	 tmp_ptr = DISPLAY_RECORDS_003;
	 break;
   }


   mvwprintw(hostdb_win, LCUR_OS_TYPE, CCUR_OS_TYPE, "%-20s", tmp_ptr);
   strcpy(map_screen.os_type, tmp_ptr);


   /* convert timezone seconds into (signed) hours:minutes */

   sprintf(tmp_buf, "%+02d:%02d", (hostdb_rec -> timezone) / 3600, ((abs(hostdb_rec -> timezone) % 3600) / 60 ));
   mvwprintw(hostdb_win, LCUR_TIMEZONE, CCUR_TIMEZONE, tmp_buf);
   strcpy(map_screen.timezone, tmp_buf);

   /* display access commands */


   if(dbspec_setup_screen(dbspec_list, hostaux_rec) == ERROR){
/*      send_response("No template (or too many fields) for this access method"); */
   }

   mvwprintw(hostaux_win, LCUR_HOSTAUX_NAME, CCUR_HOSTAUX_NAME, "%-30s", hostaux_rec -> origin->access_methods);
   mvwprintw(hostaux_win, LCUR_RECORD_NUM_NAME, CCUR_RECORD_NUM_NAME, "%-10d", hostaux_rec -> origin->hostaux_index);
   
   switch(hostaux_rec -> generated_by){

      case INSERT:
	 tmp_ptr = GEN_PROG_INSERT;
	 break;
	 

      case SERVER:
         tmp_ptr = GEN_PROG_SERVER;
	 break;

      case RETRIEVE:
         tmp_ptr = GEN_PROG_RETRIEVE;
	 break;
      
      case PARSER:
         tmp_ptr = GEN_PROG_PARSER;
	 break;

      case ADMIN:
	 tmp_ptr = GEN_PROG_ADMIN;
	 break;
	 
      case CONTROL:
	 tmp_ptr = GEN_PROG_CONTROL;
	 break;

      default:
        tmp_ptr = "";
	break;

   }

   mvwprintw(hostaux_win, LCUR_GENERATED_BY, CCUR_GENERATED_BY, "%-12s", tmp_ptr);

   mvwprintw(hostaux_win, LCUR_SOURCE_ARCHIE_HOSTNAME, CCUR_SOURCE_ARCHIE_HOSTNAME, "%-28s", hostaux_rec -> source_archie_hostname);
   mvwaddstr(hostaux_win, LCUR_RETRIEVE_TIME, CCUR_RETRIEVE_TIME, cvt_to_usertime(hostaux_rec -> retrieve_time,1));
   mvwaddstr(hostaux_win, LCUR_PARSE_TIME, CCUR_PARSE_TIME, cvt_to_usertime(hostaux_rec -> parse_time,1));
   mvwaddstr(hostaux_win, LCUR_UPDATE_TIME, CCUR_UPDATE_TIME, cvt_to_usertime(hostaux_rec -> update_time,1));
   

   mvwprintw(hostaux_win, LCUR_NO_RECS, CCUR_NO_RECS, "%-6u", hostaux_rec -> no_recs);

   switch(hostaux_rec -> current_status){
      case ACTIVE:

	 /* "Active" */

	 tmp_ptr = DISPLAY_RECORDS_004;
	 break;

      case INACTIVE:

	 /* "Inactive" */

	 tmp_ptr = DISPLAY_RECORDS_005;
	 break;

      case NOT_SUPPORTED:

	 /* "Not supported" */

	 tmp_ptr = DISPLAY_RECORDS_006;
	 break;

      case DEL_BY_ADMIN:

	 /* "Scheduled for deletion by site administrator" */

	 tmp_ptr = DISPLAY_RECORDS_007;
	 break;

      case DEL_BY_ARCHIE:

	 /* "Scheduled for deletion by local administrator" */

	 tmp_ptr = DISPLAY_RECORDS_008;
	 break;

      case DELETED:

	 /* "Deleted" */

	 tmp_ptr = DISPLAY_RECORDS_012;
	 break;

      case DISABLED:

	 /* "Disabled" */
	 tmp_ptr = DISPLAY_RECORDS_014;
	 break;

      default:
        tmp_ptr = DISPLAY_RECORDS_015;
	break;
   }

   mvwprintw(hostaux_win, LCUR_CURRENT_STATUS, CCUR_CURRENT_STATUS, "%-45s", tmp_ptr);

   mvwprintw(hostaux_win, LCUR_FAIL_COUNT, CCUR_FAIL_COUNT, "%u", hostaux_rec -> fail_count);

   mvwprintw(hostaux_win, LCUR_COMMENT, CCUR_COMMENT, "%-140s", hostaux_rec -> comment);

   HDB_IS_PROSPERO_SITE(hostdb_rec -> flags) ? (tmp_ptr = DISPLAY_RECORDS_016) : (tmp_ptr = DISPLAY_RECORDS_017);
   mvwprintw(hostdb_win, LCUR_PROSPERO_HOST, CCUR_PROSPERO_HOST, "%-3s", tmp_ptr);


   HADB_IS_FORCE_UPDATE(hostaux_rec -> flags) ? (tmp_ptr = DISPLAY_RECORDS_016) : (tmp_ptr = DISPLAY_RECORDS_017);
   mvwprintw(hostaux_win, LCUR_FORCE_UPDATE, CCUR_FORCE_UPDATE, "%-3s", tmp_ptr);

   switch(action){

      case NEW:

	 /*  "New" */

	 tmp_ptr = DISPLAY_RECORDS_009;
	 break;

      case UPDATE:

	 /* "Update" */

	 tmp_ptr = DISPLAY_RECORDS_010;
	 break;

      case DELETE:

	 /* "Delete" */

	 tmp_ptr = DISPLAY_RECORDS_011;
	 break;
   }

   mvwprintw(hostaux_win, LCUR_ACTION, CCUR_ACTION, "%-10s",tmp_ptr);

#ifndef AIX
   wrefresh(hostaux_win);   
   wrefresh(hostdb_win);
   wmove(base_win, LCUR_PRIMARY_HOSTNAME, CCUR_PRIMARY_HOSTNAME + strlen(hostdb_rec -> primary_hostname));
   wrefresh(base_win);
#else
   wnoutrefresh(hostaux_win);   
   wnoutrefresh(hostdb_win);
   wmove(base_win, LCUR_PRIMARY_HOSTNAME, CCUR_PRIMARY_HOSTNAME + strlen(hostdb_rec -> primary_hostname));
   wnoutrefresh(base_win);
   doupdate();

#endif

   return(A_OK);


}

/*
 * process_input: read a character from the terminal determine if it is a
 * command or not and perform the appropriate action
 */

int process_input(new_hostdb, new_hostaux_db, local_dbspec, action)
   hostdb_t *new_hostdb;	 /* primary host database record */
   hostdb_aux_t *new_hostaux_db; /* aux host database record */
   dbspec_t *local_dbspec;	 /* access command config list */
   action_status_t *action;	 /* current action on record */

{
   extern int wgetch();

   extern int sleep PROTO((int));
   extern int wrefresh PROTO((WINDOW *));
   extern int wmove PROTO((WINDOW *, int, int));

   int val;
   int field_no = 0;
   int finished = 0;
   int inchar;
   WINDOW *currwin = base_win;
   int currcursor = CCUR_PRIMARY_HOSTNAME + strlen(new_hostdb -> primary_hostname);
   int x,y;
   int retval;
   
   ptr_check(new_hostdb, hostdb_t, "process_input", ERROR);
   ptr_check(new_hostaux_db, hostdb_aux_t, "process_input", ERROR);
   ptr_check(local_dbspec, dbspec_t, "process_input", ERROR);

   while(!finished){

      switch((inchar = wgetch(currwin))){

	 case '\0':
	    continue;

	 /* Goto next site/database */

	 case ' ': /* blank */

	    if(in_hostlist){

	       hostptr += maxsites - 1;
	       return(NEXT_SITE);
	    }

	    if((val = next_value(new_hostdb, new_hostaux_db, &currwin, &field_no, &currcursor, NEXT_VAL, local_dbspec, action)) != 0)
	       return(val);
	    break;

	 /* go to previous site/database */


	 case '':  /* Escape char */
	    if(in_hostlist){

	       hostptr -= maxsites - 1;
	       return(PREVIOUS_SITE);
	    }

	    if((val = next_value(new_hostdb, new_hostaux_db, &currwin, &field_no, &currcursor, PREVIOUS_VAL, local_dbspec, action)) != 0)
	       return(val);
	    break;

	 /* beginning of line */

	 case '':
	    if(in_hostlist)
	       continue;

	    move_cursor(&currwin, &field_no, &currcursor, FIRST_CHAR, local_dbspec, *action);
	    break;


	 /* First site/database */

	 case '^':
	    if((val = next_value(new_hostdb, new_hostaux_db, &currwin, &field_no, &currcursor, FIRST_SITE, local_dbspec, action)) != 0)
	       return(val);
	    break;
	    
	 /* Last site/database */

	 case '$':
	    if((val = next_value(new_hostdb, new_hostaux_db, &currwin, &field_no, &currcursor, LAST_SITE, local_dbspec, action)) != 0)
	       return(val);
	    break;

	 /* perform hostlist */

	 case '':

	    if(!in_hostlist){
	       orighost =  hostptr;
	       currwin = hostlist_win;
	       return(HOSTLIST);
	    }
	    else{
	       strcpy(new_hostdb -> primary_hostname, (char *) orighost);
#if !defined(AIX) && !defined(SOLARIS)
	       touchoverlap(hostlist_win, hostaux_win);
	       touchoverlap(hostlist_win, hostdb_win);
#else
	       touchwin(hostaux_win);
	       touchwin(hostdb_win);
#endif	       
	       delwin(hostlist_win);
	       hostlist_win = (WINDOW *) NULL;
	       in_hostlist = 0;
	       return(GOTO_SITE);
	    }
			
	    break;
	    
	 /* back one character */

	 case '':

	    if(in_hostlist){

	       if((hostptr - maxsites/nocols - 1) > tophost){

	          hostptr -= maxsites/nocols + 1;
	          return(NEXT_SITE);
	       }

	       break;
	    }

	    move_cursor(&currwin, &field_no, &currcursor, PREVIOUS_CHAR, local_dbspec, *action);
	    break;

	 /* Delete previous character */
	 case '':
	 /* Delete current character */
	 case '':
	 case '':
	 /* kill to end of line */
	 case '':
	 /* yank from kill buffer */
	 case '':

	    if(in_hostlist)
	       continue;

	    print_input(&currwin, &field_no, &currcursor,inchar, local_dbspec, *action);
	    break;

	 /* end of line */

	 case '':

	    if(in_hostlist)
	       continue;

	    move_cursor(&currwin, &field_no, &currcursor, LAST_CHAR, local_dbspec, *action);
	    break;

	 /* forward one character */

	 case '':
	    if(in_hostlist){

	       if((hostptr + maxsites/nocols + 1) <= tophost + maxsites){

		  hostptr += maxsites/nocols -1;
	          return(NEXT_SITE);
	       }

	       break;
	    }

	    move_cursor(&currwin, &field_no, &currcursor, NEXT_CHAR, local_dbspec, *action);
	    break;

	 /* redraw screen */

	 case '':
	    wrefresh(curscr);
	    break;

	 /* go to entered site/database */

	 case '\n':
	    if(in_hostlist)
	       currwin = hostlist_win;
	    return(go_to(&currwin, &field_no));
	    break;
	    
	 /* forward one field */

	 case '':

	    if(in_hostlist)
	       return(NEXT_SITE);
      if ( modified ) {
        dbspec_setup_screen(local_dbspec,new_hostaux_db);
        modified = 0;
        local_dbspec = glob_dbspec;
      }
	    move_field(&currwin, &field_no, &currcursor, local_dbspec, NEXT_FIELD);
	    break;

	 /* backwards one field */

	 case '':
	    if(in_hostlist)
	       return(PREVIOUS_SITE);
      if ( modified ) {
        dbspec_setup_screen(local_dbspec,new_hostaux_db);
        modified = 0;
        local_dbspec = glob_dbspec;
      }
	    move_field(&currwin, &field_no, &currcursor, local_dbspec, PREVIOUS_FIELD);
	    break;

	 /* Update current site */

	 case '\t':

	    if(in_hostlist)
	       continue;

	    if((retval = modify_record(currwin)) != 0)
	       return(retval);
	    break;

	 case '':
	    if(in_hostlist)
	       continue;

	    return(UPDATE_SITE);
	    break;

	 /* non-command character */

	 default:

	    if(in_hostlist)
	       continue;

	    if(iscntrl(inchar)){
	       
	       getyx(currwin, y, x);

	       /* "Unknown command %s" */

	       send_response(PROCESS_INPUT_001, unctrl(inchar));
	       sleep(1);
	       clear_response();
	       wmove(currwin, y, x);
	       wrefresh(currwin);
	    }
	    else{
	       print_input(&currwin, &field_no, &currcursor,inchar, local_dbspec, *action);
	    }
	 break;
      }

      /* update host database temporary records with what's on the screen */

      strcpy(new_hostdb -> primary_hostname, map_screen.primary_hostname);
      strcpy(new_hostaux_db -> preferred_hostname, map_screen.preferred_hostname);
      if ( strcasecmp(new_hostaux_db -> origin -> access_methods, map_screen.access_methods) ) {
        modified = 1;
      }
      strcpy(new_hostaux_db -> origin -> access_methods, map_screen.access_methods);
      get_new_acommand(new_hostaux_db->origin->access_methods, new_hostaux_db->access_command, local_dbspec);

    }

   

   return(A_OK);
}

/*
 * go_to: figure out from field position what 'goto' (site/database) action
 * to perform
 */


int go_to(currwin, field_no)
   WINDOW **currwin;
   int *field_no;
{
   extern int touchoverlap PROTO((WINDOW *, WINDOW *));
   extern int delwin PROTO((WINDOW *));
   

   WINDOW *local_win = *currwin;
   int local_field = *field_no;

   ptr_check(currwin, WINDOW*, "go_to", ERROR);
   ptr_check(field_no, int, "go_to", ERROR);   

   if(local_win == base_win)
      return(GOTO_SITE);
   else if(local_win == hostaux_win)
      return(GOTO_DATABASE);
   else if(local_win == hostlist_win){
#if !defined(AIX) && !defined(SOLARIS)
      touchoverlap(hostlist_win, hostdb_win);
      touchoverlap(hostlist_win, hostaux_win);
#else
      touchwin(hostdb_win);
      touchwin(hostaux_win);
#endif
      delwin(hostlist_win);
      hostlist_win = (WINDOW *) NULL;
      return(GOTO_SITE);
   }
   

   return(A_OK);
}

/*
 * print_input: Perform character operations on the screen. Printing
 * regular characters as well as yanking and placing lines
 */


status_t print_input(currwin, field_no, currcursor, inchar, local_dbspec, action)
   WINDOW **currwin;	   /* Current window */
   int *field_no;	   /* field that cursor is in */
   int *currcursor;	   /* column of current cursor */
   int inchar;		   /* input character */
   dbspec_t *local_dbspec;  /* access commands config list */
   action_status_t action; /* current action */

{

   extern int wdelch PROTO((WINDOW *));
   extern int wclrtoeol PROTO((WINDOW *));
   extern int waddstr PROTO((WINDOW *, char *));
   extern int wmove PROTO((WINDOW *, int, int));
   extern int wrefresh PROTO((WINDOW *));

   WINDOW *local_win = *currwin;
   int local_field = *field_no;
   int local_cursor = *currcursor;
   int y,x;
   int field_len = 0;
   pathname_t tmp_str;
   char *field_mirror = (char *)0;

   /* Yank buffer */

   static pathname_t yank_buf;

   ptr_check(currwin, WINDOW*, "print_input", ERROR);
   ptr_check(field_no, int, "print_input", ERROR);
   ptr_check(currcursor, int, "print_input", ERROR);
   ptr_check(local_dbspec, dbspec_t, "print_input", ERROR);
   
   /* Get current position */

   getyx(local_win, y, x);

   /* If primary window */

   if(local_win == base_win){

      field_len = CCUR_PRIMARY_HOSTNAME;
      field_mirror = map_screen.primary_hostname;

   } else if(local_win == hostdb_win){

      if(local_field == WFIELD_PREFERRED_HOSTNAME){

         field_len = CCUR_PREFERRED_HOSTNAME;
	 field_mirror = map_screen.preferred_hostname;
      }
      else{
         if((local_field == WFIELD_TIMEZONE) || (local_field == WFIELD_OS_TYPE) || (local_field == WFIELD_PROSPERO_HOST)){

	   /* "Type <space> or <ESC> to change value" */
	   
	   send_response(PRINT_INPUT_001);
	   return(A_OK);
	 }
      }
   } else if(local_win == hostaux_win){

	if(local_field == WFIELD_HOSTAUX_NAME){
           field_len = CCUR_HOSTAUX_NAME;
	   field_mirror = map_screen.access_methods;
	}
	else{
	   if(local_field - WFIELD_DBSPEC >= 0){
	      dbfield_t *dbfield;

	      dbfield = local_field - WFIELD_DBSPEC + &(local_dbspec -> subfield[0]);

	      field_len = strlen(dbfield -> field_name) + 2 + dbfield -> colno;
	      field_mirror = dbfield -> curr_val;
	   }
	   else
	      if(local_field == WFIELD_CURRENT_STATUS){
		 /* "Type <space> or <ESC> to change value" */
		 
		 send_response(PRINT_INPUT_001);
		 return(A_OK);
	      }
	      else{
	         if((local_field == WFIELD_FORCE_UPDATE) || (local_field == WFIELD_ACTION))
		    return(A_OK);
   
	      }
		    
	}
   }
   else 
      return(A_OK);

   switch(inchar){

      /* delete previous character */
      case '':
      case '':

	 if(local_cursor - 1 >= field_len){
	    local_cursor--;
	    delete_char(field_mirror, inchar, local_cursor - field_len);
	    sprintf(tmp_str,"%%-%ds", strlen(field_mirror) + 1);
	    mvwprintw(local_win, y, field_len, tmp_str, field_mirror);
	    wmove(local_win, y, x-1);
	 }

	 break;

      /* delete current character */

      case '':
	 delete_char(field_mirror, inchar, local_cursor - field_len);
	 sprintf(tmp_str,"%%-%ds", strlen(field_mirror) + 1);
	 mvwprintw(local_win, y, field_len, tmp_str, field_mirror);
	 wmove(local_win, y, x);

	 break;

      /* Kill to end of line. Put current value in kill buffer */

      case '':

	 /*
	  * BUG: this should just overwrite to end of field, not clear to
	  * end of line
	  */

	 sprintf(tmp_str,"%%-%d ", strlen(field_mirror) - field_len + local_cursor);
	 wprintw(local_win,tmp_str);
	 wmove(local_win,y, x);
	 strcpy(yank_buf, &field_mirror[local_cursor - field_len]);
	 field_mirror[local_cursor - field_len] = '\0';
	 break;

	 /* yank from kill buffer to current line */

      case '':

         strcat(field_mirror, yank_buf);
	 local_cursor += strlen(yank_buf);
	 sprintf(tmp_str,"%%-%ds", strlen(field_mirror));
	 mvwprintw(local_win, y, field_len, tmp_str, field_mirror);
	 break;


      /* insert character to current line */

      default:

	 insert_char(field_mirror, inchar, local_cursor - field_len);
	 sprintf(tmp_str,"%%-%ds", strlen(field_mirror) + 1);
	 mvwprintw(local_win, y, field_len, tmp_str, field_mirror);
	 wmove(local_win,y, x+1);
	 local_cursor++;

	 break;
   }

   *currcursor = local_cursor;

   /* refresh screen */


   wrefresh(local_win);

   return(A_OK);
}


/*
 * move_cursor: move the cursor to another character on the same line
 */

status_t move_cursor(currwin, field_no, currcursor, direction, local_dbspec, action)
   WINDOW **currwin;	/* current window */
   int *field_no;	/* current field */
   int *currcursor;	/* current cursor column */
   int direction;	/* Direction of move */
   dbspec_t *local_dbspec;  /* access commands configuration */
   action_status_t action; /* current action */

{
   extern int wmove PROTO((WINDOW *, int, int));
   extern int wrefresh PROTO((WINDOW *));

   WINDOW *local_win = *currwin;
   int local_field = *field_no;
   int local_cursor = *currcursor;
   int y, x;

   ptr_check(currwin, WINDOW*, "move_cursor", ERROR);
   ptr_check(field_no, int, "move_cursor", ERROR);
   ptr_check(currcursor, int, "move_cursor", ERROR);
   ptr_check(local_dbspec, dbspec_t, "move_cursor", ERROR);

   /* the routine is basically an automaton */

   if(local_win == base_win){

      switch(direction){

	 case NEXT_CHAR:
	 
	    if(local_cursor + 1 <= CCUR_PRIMARY_HOSTNAME + strlen(map_screen.primary_hostname))
	       local_cursor++;
	    break;
	       
	 case PREVIOUS_CHAR:
      
	    if(local_cursor - 1 >= CCUR_PRIMARY_HOSTNAME )
	       local_cursor--;
               break;

	    case FIRST_CHAR:
	       local_cursor = CCUR_PRIMARY_HOSTNAME;
	       break;

	    case LAST_CHAR:
	       local_cursor = CCUR_PRIMARY_HOSTNAME + strlen(map_screen.primary_hostname);
	       break;
      }
	       
   } else if(local_win == hostdb_win){

      switch(local_field){

	 case WFIELD_PREFERRED_HOSTNAME:

	    switch(direction){

	       case NEXT_CHAR:

	          if(local_cursor + 1 <= CCUR_PREFERRED_HOSTNAME + strlen(map_screen.preferred_hostname))
		     local_cursor++;

	          break;

	       case PREVIOUS_CHAR:
	       
		  if(local_cursor - 1 >= CCUR_PREFERRED_HOSTNAME )
		     local_cursor--;

		  break;

	       case FIRST_CHAR:
	          local_cursor = CCUR_PREFERRED_HOSTNAME;
		  break;

	       case LAST_CHAR:
	          local_cursor = CCUR_PREFERRED_HOSTNAME + strlen(map_screen.preferred_hostname);
		  break;

	       }

	    break;

         case WFIELD_TIMEZONE:
	 
	    switch(direction){

	       case NEXT_CHAR:
	          if(local_cursor + 1 < CCUR_TIMEZONE + strlen(map_screen.timezone))
		     local_cursor++;

		  break;
		  
	       case PREVIOUS_CHAR:   

	          if(local_cursor - 1 >= CCUR_TIMEZONE)
		     local_cursor--;
		  break;
	    }

	    if(local_cursor == CCUR_TIMEZONE + 2){
	       if(direction == NEXT_CHAR)
		  local_cursor++;
		 else
		  local_cursor--;
	    }

	    break;
      }
   } else if(local_win == hostaux_win){

        if(local_field == WFIELD_HOSTAUX_NAME){

	   switch(direction){

	      case NEXT_CHAR:

	         if(local_cursor + 1 <= CCUR_HOSTAUX_NAME + strlen(map_screen.access_methods))
		    local_cursor++;

		 break;

	      case PREVIOUS_CHAR:
	       
		 if(local_cursor - 1 >= CCUR_HOSTAUX_NAME)
		    local_cursor--;

		 break;

	      case FIRST_CHAR:
		 local_cursor = CCUR_HOSTAUX_NAME;
		 break;

	      case LAST_CHAR:
		 local_cursor = CCUR_HOSTAUX_NAME + strlen(map_screen.access_methods);
		 break;

	   }
	}
	else if(local_field - WFIELD_DBSPEC >= 0)
	   move_dbspec_cursor(direction, &local_field, &local_cursor, glob_dbspec);
   }


   getyx(local_win, y, x);
   *currcursor = local_cursor;
   wmove(local_win, y, local_cursor);
   wrefresh(local_win);
   return(A_OK);
}


/*
 * move_field: move cursor to the next/previous field
 */


status_t move_field(currwin, field_no, currcursor, local_dbspec, direction)
   WINDOW **currwin;	/* current window */
   int *field_no;	/* current field */
   int *currcursor;	/* current cursor column */
   dbspec_t *local_dbspec;  /* access commands configuration */
   int direction;	/* direction of move */

{
   extern int wmove PROTO((WINDOW *, int, int));
   extern int wrefresh PROTO((WINDOW *));

   WINDOW *local_win = *currwin;
   int local_field = *field_no;
   int local_cursor = *currcursor;   

   ptr_check(currwin, WINDOW*, "move_field", ERROR);
   ptr_check(field_no, int, "move_field", ERROR);
   ptr_check(currcursor, int, "move_field", ERROR);
   ptr_check(local_dbspec, dbspec_t, "move_field", ERROR);

   if(local_win == base_win){

      if(direction == NEXT_FIELD){

	 local_win = hostdb_win;
	 wmove(local_win, LCUR_PREFERRED_HOSTNAME, (local_cursor = CCUR_PREFERRED_HOSTNAME + strlen(map_screen.preferred_hostname)));

      } else{ /*PREVIOUS FIELD */

#if 0
	 local_cursor = CCUR_ACTION;
	 local_win = hostaux_win;
	 local_field = WFIELD_ACTION;
	 wmove(local_win, LCUR_ACTION, CCUR_ACTION);
#endif

	 local_cursor = CCUR_CURRENT_STATUS;
	 local_win = hostaux_win;
	 local_field = WFIELD_CURRENT_STATUS;
	 wmove(local_win, LCUR_CURRENT_STATUS, CCUR_CURRENT_STATUS);


      }
   } else if(local_win == hostdb_win){
      if(direction == NEXT_FIELD){
	 
	 if(local_field == 0){

	    local_cursor = CCUR_OS_TYPE;
	    local_field = WFIELD_OS_TYPE;
	    wmove(local_win, LCUR_OS_TYPE, CCUR_OS_TYPE);

	 } else if(local_field == WFIELD_OS_TYPE){

	    local_field = WFIELD_PROSPERO_HOST;
	    wmove(local_win, LCUR_PROSPERO_HOST, CCUR_PROSPERO_HOST);
	    local_cursor = CCUR_PROSPERO_HOST;

	 } else if(local_field == WFIELD_PROSPERO_HOST){

	    local_field = WFIELD_TIMEZONE;
	    wmove(local_win, LCUR_TIMEZONE, CCUR_TIMEZONE);
	    local_cursor = CCUR_TIMEZONE;

	 } else if(local_field == WFIELD_TIMEZONE){
	    local_cursor = CCUR_HOSTAUX_NAME;
	    local_field = WFIELD_HOSTAUX_NAME;
	    local_win = hostaux_win;
	    wmove(local_win, LCUR_HOSTAUX_NAME, CCUR_HOSTAUX_NAME);

	 }
      } else{ /* PREVIOUS_FIELD */

	 if(local_field == WFIELD_PREFERRED_HOSTNAME){

	    local_field = WFIELD_PRIMARY_HOSTNAME;
	    local_win = base_win;
	    wmove(local_win, LCUR_PRIMARY_HOSTNAME, (local_cursor = CCUR_PRIMARY_HOSTNAME + strlen(map_screen.primary_hostname) ));

	 } else if(local_field == WFIELD_OS_TYPE){
	    
	    local_field = WFIELD_PREFERRED_HOSTNAME;
	    wmove(local_win, LCUR_PREFERRED_HOSTNAME, CCUR_PREFERRED_HOSTNAME + strlen(map_screen.preferred_hostname));
	    local_cursor = CCUR_PREFERRED_HOSTNAME;

	 } else if(local_field == WFIELD_PROSPERO_HOST){

	    wmove(local_win, LCUR_OS_TYPE, CCUR_OS_TYPE);
	    local_field = WFIELD_OS_TYPE;
	    local_cursor = CCUR_OS_TYPE;

	 } else if(local_field == WFIELD_TIMEZONE){

	    local_field = WFIELD_PROSPERO_HOST;
	    wmove(local_win, LCUR_PROSPERO_HOST, CCUR_PROSPERO_HOST);
	    local_cursor = CCUR_PROSPERO_HOST;
	 }
      } 
   } else { /* currwin == hostaux_db */

      if(direction == NEXT_FIELD){

         if(local_field == WFIELD_HOSTAUX_NAME){

	    move_dbspec_field(glob_dbspec, direction, &local_field, &local_cursor);


	 } else if(local_field == WFIELD_CURRENT_STATUS){

	    local_field = WFIELD_PRIMARY_HOSTNAME;
	    local_win = base_win;
	    wmove(local_win, LCUR_PRIMARY_HOSTNAME, (local_cursor = CCUR_PRIMARY_HOSTNAME + strlen(map_screen.primary_hostname)));
	 }
#if 0
	 } else if(local_field == WFIELD_CURRENT_STATUS){

	    local_field = WFIELD_FORCE_UPDATE;
	    wmove(local_win, LCUR_FORCE_UPDATE, CCUR_FORCE_UPDATE);
	    local_cursor = CCUR_FORCE_UPDATE;

	 } else if(local_field == WFIELD_FORCE_UPDATE){

	    local_field = WFIELD_ACTION;
	    wmove(local_win, LCUR_ACTION, CCUR_ACTION);
	    local_cursor = CCUR_ACTION;


	 } else if(local_field == WFIELD_ACTION){

	    local_field = WFIELD_PRIMARY_HOSTNAME;
	    local_win = base_win;
	    wmove(local_win, LCUR_PRIMARY_HOSTNAME, (local_cursor = CCUR_PRIMARY_HOSTNAME + strlen(map_screen.primary_hostname)));
	 }
#endif
	 else 
	    move_dbspec_field(glob_dbspec, direction, &local_field, &local_cursor);

      } else { /* PREVIOUS_FIELD */
         if(local_field == WFIELD_HOSTAUX_NAME){

	    local_field = WFIELD_TIMEZONE;
	    local_win = hostdb_win;
	    wmove(local_win, LCUR_TIMEZONE, CCUR_TIMEZONE);
	    local_cursor = CCUR_TIMEZONE;


	 } else if(local_field == WFIELD_CURRENT_STATUS){

	    move_dbspec_field(glob_dbspec, direction, &local_field, &local_cursor);

#if 0
	 } else if(local_field == WFIELD_ACTION){

	    local_field = WFIELD_FORCE_UPDATE;
	    wmove(local_win, LCUR_FORCE_UPDATE, CCUR_FORCE_UPDATE);
	    local_cursor = CCUR_FORCE_UPDATE;

	 } else if(local_field == WFIELD_FORCE_UPDATE){

	    local_field = WFIELD_CURRENT_STATUS;
	    wmove(local_win, LCUR_CURRENT_STATUS, CCUR_CURRENT_STATUS);
	    local_cursor = CCUR_CURRENT_STATUS;

#endif
	 }
	 else
	    move_dbspec_field(glob_dbspec, direction, &local_field, &local_cursor);
      }
   }

	    

   *currwin = local_win;
   *field_no = local_field;
   *currcursor = local_cursor;
   wrefresh(local_win);

   return(A_OK);
}

/*
 * send_response: varargs. Write the given message to the status line on
 * display. Arguments are printf format string for message followed by
 * values
 */


void send_response(va_alist)
   va_dcl
{
   extern int vsprintf();

   extern int wclrtoeol PROTO((WINDOW *));
   extern int wmove PROTO((WINDOW *, int, int));
   extern int wrefresh PROTO((WINDOW *));

   char *format;
   va_list al;
   char output_buf[BUFSIZ];

   va_start(al);

   /* get the format */

   format = va_arg(al, char *);
   
   /* clear format line */

   wmove(response_win, LCUR_RESPONSE, 0);
   wclrtoeol(response_win);

   vsprintf(output_buf, format, al);

   va_end(al);

   wstandout(response_win);

   wprintw(response_win, "%s", output_buf);

   wstandend(response_win);

   wrefresh(response_win);
}

/*
 * next_value: determine the next/previous value for the current field
 */

   
int next_value(new_hostdb, new_hostaux, currwin, field_no, currcursor, direction, local_dbspec, action)
   hostdb_t *new_hostdb;
   hostdb_aux_t *new_hostaux;
   WINDOW **currwin;
   int *field_no;
   int *currcursor;
   int direction;
   dbspec_t *local_dbspec;
   action_status_t *action;
{
   extern int wclrtoeol PROTO((WINDOW *));
   extern int wrefresh PROTO((WINDOW *));
   extern int wmove PROTO((WINDOW *, int, int));

   WINDOW *local_win = *currwin;
   int local_field = *field_no;
   int local_cursor = *currcursor;
   int y, x;

   ptr_check(new_hostdb, hostdb_t, "next_value", EXIT_SITE);
   ptr_check(new_hostaux, hostdb_aux_t, "next_value", EXIT_SITE);
   ptr_check(currwin, WINDOW*, "next_value", EXIT_SITE);
   ptr_check(field_no, int, "next_value", EXIT_SITE);
   ptr_check(currcursor, int, "next_value", EXIT_SITE);
   ptr_check(local_dbspec, dbspec_t, "next_value", EXIT_SITE);
   ptr_check(action, action_status_t, "next_value", EXIT_SITE);

   if(local_win == base_win)
      return(direction);

   getyx(local_win, y, x);

   if(local_win == hostdb_win){

   if(local_field == WFIELD_PREFERRED_HOSTNAME){
      return(direction);
   } else if(local_field == WFIELD_OS_TYPE){

	 /* When this becomes more than OS types the direction of the request
	    will have to be taken into account */

	 switch(new_hostdb -> os_type){

	    case UNIX_BSD:
	       new_hostdb -> os_type = VMS_STD;
	       break;

	    case VMS_STD:
	       new_hostdb -> os_type = NOVELL;
	       break;

	    case NOVELL:
	       new_hostdb -> os_type = UNIX_BSD;
	       break;

	    default:
	       new_hostdb -> os_type = UNIX_BSD;
	       break;
	 }

      } else if(local_field == WFIELD_PROSPERO_HOST){

	 if(HDB_IS_PROSPERO_SITE(new_hostdb -> flags))
	    HDB_UNSET_PROSPERO_SITE(new_hostdb -> flags);
	 else
	    HDB_SET_PROSPERO_SITE(new_hostdb -> flags);

      } else if(local_field == WFIELD_TIMEZONE){

	 wclrtoeol(local_win);

	 if(direction == NEXT_VAL){

	    if(local_cursor < CCUR_TIMEZONE + 3){	   /* Hours */

	       if((new_hostdb -> timezone / 3600) + 1 < 12)
		  new_hostdb -> timezone += 3600;	   /* Seconds in an hour */
	       else
		  new_hostdb -> timezone = new_hostdb -> timezone % 3600;
	    }
	    else {  /* Minutes */

	       if(((abs(new_hostdb -> timezone) % 3600) / 60 + 15) < 60)
		  new_hostdb -> timezone += 900 * (new_hostdb -> timezone < 0 ? -1 : 1);
	       else
		 /* Note this is integer division: x/y * y != x necessarily */

		  new_hostdb -> timezone = (new_hostdb -> timezone / 3600) * 3600;
	    }
	 }
	 else { /* Previous */

	    if(local_cursor < CCUR_TIMEZONE + 3){	   /* Hours */

	       if((new_hostdb -> timezone / 3600) - 1 > -12)
		  new_hostdb -> timezone -= 3600;	   /* Seconds in an hour */
	       else
		  new_hostdb -> timezone = new_hostdb -> timezone % 3600;
	    }
	    else {  /* Minutes */

	       if(((abs(new_hostdb -> timezone) % 3600) / 60 - 15) > 0)
		  new_hostdb -> timezone -= 900;	/* 15 minutes */
	       else
		 /* Note this is integer division: x/y * y != x necessarily */

		  new_hostdb -> timezone = (new_hostdb -> timezone / 3600) * 3600;
	    }
	 }
      
	 sprintf(map_screen.timezone, "%+02d:%02d", (new_hostdb -> timezone) / 3600, ((abs(new_hostdb -> timezone) % 3600) / 60 ));

      }
   }

   if(local_win == hostaux_win){

      if(local_field == WFIELD_HOSTAUX_NAME){

	 switch(direction){
	    case NEXT_VAL:
	       return(NEXT_HOSTAUX);
	       break;
	    case PREVIOUS_VAL:
	       return(PREVIOUS_HOSTAUX);
	       break;
	    case FIRST_SITE:
	       return(FIRST_HOSTAUX);
	       break;
	    case LAST_SITE:
	       return(PREVIOUS_HOSTAUX);
	       break;
	 }
      }

      if(local_field == WFIELD_CURRENT_STATUS){

         wclrtoeol(local_win);

         switch(new_hostaux -> current_status){

	  case ACTIVE:
	     if(direction == NEXT_VAL)
	        new_hostaux -> current_status = DISABLED;
	     else
	        new_hostaux -> current_status = NOT_SUPPORTED;
	     break;
	    
	  case NOT_SUPPORTED:
	     if(direction == NEXT_VAL)
	        new_hostaux -> current_status = ACTIVE;
	     else
      	        new_hostaux -> current_status = DISABLED;
	     break;

	  case DISABLED:
	     if(direction == NEXT_VAL)
	        new_hostaux -> current_status = NOT_SUPPORTED;
	     else
      	        new_hostaux -> current_status = ACTIVE;
	     break;

	  case DEL_BY_ARCHIE:
	  case DEL_BY_ADMIN:
	  case DELETED:
	  case INACTIVE:

	    /* "This value cannot be changed in this manner" */

	    send_response(NEXT_VALUE_001);
	     
	    break;
	}
      } else if(local_field == WFIELD_FORCE_UPDATE){

	    /* "This value cannot be changed in this manner" */

	    send_response(NEXT_VALUE_001);

      } else if(local_field == WFIELD_ACTION) {

	 if(*action == UPDATE)
	  *action = DELETE;
	 else if(*action == DELETE)
	  *action = UPDATE;
      }
   }

   if(display_records(new_hostdb, new_hostaux, local_dbspec, *action) == ERROR)
       return(EXIT_SITE);

   wmove(local_win, y, x);
   wrefresh(local_win);

   return(0);

}

/* clear_response: clear the response window */

void clear_response()

{
   extern int wclear PROTO((WINDOW *));
   extern int wrefresh PROTO((WINDOW *));
#ifndef AIX

   /* can't use wclear, calls clearok() */
   wclear(response_win);

#else
   werase(response_win);
#endif
   wrefresh(response_win);

}

/*
 * sig_handle: handle keyboard signals
 */


void sig_handle(sig)
   int sig;
{

   extern int getpid();

   extern int  stty PROTO((int, struct sgttyb *));
   extern int werase PROTO((WINDOW *));
   extern int wrefresh PROTO((WINDOW *));

   echo();
   nocbreak();

   switch(sig){

      case SIGCONT:
         noecho();
	 cbreak();
	 wrefresh(curscr);
	 werase(response_win);
 	 wrefresh(response_win);
	 return;
	 break;

      case SIGTSTP:
#if 0
	 mvwaddstr(response_win,0,0,"Stopping...\n");
	 wrefresh(response_win);
#else
	 /* "Stopping..." */

	 send_response(SIG_HANDLE_001);
#endif
	 kill(getpid(), SIGSTOP);
	 break;


      case SIGTERM:
         exit(0);

      case SIGINT:

	 /* "Quit ? " */

	 if(get_yn_question(SIG_HANDLE_002) == A_OK){
	    teardown_screen();
      clean_shutdown();
	    exit(0);
	 }
	 else{
	    noecho();
	    cbreak();
	    wrefresh(hostaux_win);   
	    wrefresh(hostdb_win);
	    wrefresh(base_win);
	    return;
	 }
	 break;

      default:

	 /* "Unexpected signal to signal handler %u" */

	 error(A_WARN,"sig_handle", SIG_HANDLE_003, sig);
	 exit(1);
	 break;
   }

   return;

}

/*
 * get_yn_question: ask a question to which the user has to respond 'y' or
 * 'n'. Return A_OK on former, ERROR on latter. Argument is varargs
 * consisting of a printf format string and its values
 */

	 
status_t get_yn_question(va_alist)
   va_dcl

{
   extern int vsprintf();

   extern int wgetch PROTO((WINDOW *));
   extern int waddstr PROTO((WINDOW *, char *));
   extern int wclrtoeol PROTO((WINDOW *));
   extern int wmove PROTO((WINDOW *, int, int));
   extern int wrefresh PROTO((WINDOW *));
   extern int sleep PROTO((int));


   pathname_t output_buf;
   int answer;
   char *format;

   va_list al;
   va_start(al);

   format = va_arg(al, char *);

   ptr_check(format, char, "get_yn_question", ERROR);

   wmove(response_win, LCUR_RESPONSE, 0);
   wclrtoeol(response_win);

   vsprintf(output_buf, format ,al);

   va_end(al);


   /* add "(y/n)" to the end of the given string */

   strcat(output_buf," (y/n) ");

   mvwaddstr(response_win, LCUR_RESPONSE, 0, output_buf);

   wrefresh(response_win);

   for(;;){
      answer = wgetch(response_win);

      /* allow screen to be refreshed while waiting for answer */

      if(answer == '')
         wrefresh(curscr);

      if((answer == 'y') || (answer == 'Y')){
         clear_response();
         return(A_OK);
      }

      if((answer == 'n') || (answer == 'N')){
         clear_response();
         return(ERROR);
      }
     
      mvwprintw(response_win, LCUR_RESPONSE, 0, "Invalid answer %c", answer);
      wrefresh(response_win);
      sleep(1);
      clear_response();
      mvwaddstr(response_win, LCUR_RESPONSE, 0, output_buf);
      wrefresh(response_win);
   }

}


/*
 * dbpsec_setup_screen: given the dbspec_list describing what the
 * access_command field of the auxiliary host database record looks like,
 * print it to the screen
 */


status_t dbspec_setup_screen(dbspec_list, hostaux_rec)
   dbspec_t *dbspec_list;	 /* access commands configuration */
   hostdb_aux_t  *hostaux_rec;	 /* aux host database record */

{

   extern int wclrtoeol PROTO((WINDOW *));
   extern int wmove PROTO((WINDOW *, int, int));

   int j;
   dbspec_t *dbspec;
   dbfield_t *dbfield;
   int finished;
   int curline, curcol;
   char **file_list;
   char *ptr = DEF_TEMPLATE_FNAME;
   pathname_t tmp_str;


   curline = LCUR_DBSPEC;
   curcol = 0;


   ptr_check(dbspec_list, dbspec_t, "dbspec_setup_screen", ERROR);
   ptr_check(hostaux_rec, hostdb_aux_t, "dbspec_setup_screen", ERROR);

   /* Clear the appropriate region of the screen */

   for(j = LCUR_DBSPEC; j != LCUR_CURRENT_STATUS;j++){
      wmove(hostaux_win,j,0);
      wclrtoeol(hostaux_win);
   }



   /* first find the given access method in the dbspec_list */

   for(dbspec = dbspec_list, finished = FALSE; !finished && (dbspec -> dbspec_name[0] != '\0');)
      if(strcasecmp(dbspec -> dbspec_name, hostaux_rec -> origin -> access_methods) == 0)
         finished = TRUE;
      else
         dbspec++;

   if(!finished){

      /* Can't find given access method */

      glob_dbspec = dbspec_list;
      dbfield = &(glob_dbspec -> subfield[0]);

      /* Clear the appropriate region of the screen */

      for(j = LCUR_DBSPEC; j != LCUR_CURRENT_STATUS;j++){
	 wmove(hostaux_win,j,0);
         wclrtoeol(hostaux_win);
      }

      /* print the whole unformatted access command to the screen */

      mvwprintw(hostaux_win, curline, 0, "%s: %s", dbfield -> field_name, hostaux_rec -> access_command);
      strcpy(dbfield -> curr_val, hostaux_rec -> access_command);
      if(hostaux_rec -> origin -> access_methods[0] == '\0')
         return(A_OK);
      else
	 return(ERROR);
   }

   glob_dbspec = dbspec;


   file_list = str_sep(hostaux_rec -> access_command, NET_DELIM_CHAR);

   for(dbfield = &(glob_dbspec -> subfield[0]); dbfield -> field_name[0] != '\0';){

      if(curline == LCUR_CURRENT_STATUS)
         goto noformat;
	 

      /* Start on the right */

      if(curcol == 0){
	 char *ptr;

	 sprintf(tmp_str,"%%s: %%-%ds", dbfield -> max_field_width);

	 dbfield -> colno = 0;
	 dbfield -> lineno = curline;

	 if(*file_list == (char *) NULL)
	    ptr = "";
	 else
	    ptr = *(file_list++);

	 wmove(hostaux_win, curline, strlen(dbfield -> field_name) + 2);
	 wclrtoeol(hostaux_win);
	 mvwprintw(hostaux_win, curline, 0, tmp_str, dbfield -> field_name, ptr);
	 strcpy(dbfield -> curr_val, ptr);
   
         if(strlen(dbfield -> field_name) + 2 + dbfield -> max_field_width + 1< middle)
	    curcol = middle;
	 else
	    curline++;

	 dbfield++;
      }
      else{

#if 0
	 if(((dbfield - 1) -> writable == 0) &&
	    strlen(dbfield -> field_name) + 2 + dbfield -> max_field_width + 1 <= COLS){
#else

	 if(strlen(dbfield -> field_name) + 2 + dbfield -> max_field_width + 1 <= COLS){
#endif

	    char *ptr;
	    
	    dbfield -> colno = curcol;
	    dbfield -> lineno = curline;


	    sprintf(tmp_str,"%%s: %%-%ds", dbfield -> max_field_width);

	    if(*file_list == (char *) NULL)
	       ptr = dbfield -> def_str;
	    else
	       ptr = *(file_list++);

	    wmove(hostaux_win, curline, middle + strlen(dbfield -> field_name) + 2);
	    wclrtoeol(hostaux_win);

	    mvwprintw(hostaux_win, curline, middle, tmp_str, dbfield -> field_name, ptr);
	    strcpy(dbfield -> curr_val, ptr);

	    dbfield++;
	 }

	 curline++;
	 curcol = 0;
      }
      
   }

   return(A_OK);

noformat:

   return(ERROR);

}
   
/*
 * move_dbspec_field: when the cursor is in the access command area, this
 * routine allows movement into, out of and inside the region
 */


status_t move_dbspec_field(dbspec_list, direction, local_field, local_cursor)
   dbspec_t *dbspec_list;     /* access command config */
   int direction;	      /* direction of cursor movement */
   int *local_field;	      /* local field number */
   int *local_cursor;	      /* local cursor column */
{
   extern int wmove PROTO((WINDOW *, int, int));

   dbspec_t *dbspec;
   dbfield_t *dbfield;
   int finished;

   ptr_check(dbspec_list, dbspec_t, "move_dbspec_field", ERROR);
   ptr_check(local_field, int, "move_dbspec_field", ERROR);
   ptr_check(local_cursor, int, "move_dbspec_field", ERROR);

   dbspec = glob_dbspec;

   if(direction == NEXT_FIELD){

      if(*local_field == WFIELD_HOSTAUX_NAME)
	 dbfield = &(dbspec -> subfield[0]);
      else
         dbfield = *local_field - WFIELD_DBSPEC + &(dbspec -> subfield[0]) + 1;
	 
      for( finished = FALSE; !finished && (dbfield -> field_name[0] != '\0');){

	 if(dbfield -> writable == 1)
	    finished = TRUE;
	 else
	    dbfield++;
      }

      if(!finished){
	 *local_field = WFIELD_CURRENT_STATUS;
	 wmove(hostaux_win, LCUR_CURRENT_STATUS, CCUR_CURRENT_STATUS);
	 *local_cursor = CCUR_CURRENT_STATUS;
      }
      else{
	 *local_field = WFIELD_DBSPEC + dbfield - &(dbspec -> subfield[0]);
	 wmove(hostaux_win, dbfield -> lineno, strlen(dbfield -> field_name) + dbfield -> colno + 2);
	 *local_cursor = dbfield -> colno + strlen(dbfield -> field_name) + 2;
      }
   }
   else { /* PREVIOUS FIELD */

      if(*local_field == WFIELD_CURRENT_STATUS){

	 /* Go to the end of the list */

	 for(dbfield = &(dbspec -> subfield[0]); dbfield -> field_name[0] != '\0'; dbfield++);

	 dbfield--;
      }
      else
         dbfield = *local_field - WFIELD_DBSPEC + &(dbspec -> subfield[0]) - 1;

      for(finished = FALSE;!finished && (dbfield >= &(dbspec -> subfield[0]));){

	 if(dbfield -> writable == 1)
	    finished = TRUE;
	 else
	    dbfield--;
      }

      if(!finished){
	 *local_field = WFIELD_HOSTAUX_NAME;
	 wmove(hostaux_win, LCUR_HOSTAUX_NAME, CCUR_HOSTAUX_NAME);
	 *local_cursor = CCUR_HOSTAUX_NAME;
      }
      else{
	 *local_field = WFIELD_DBSPEC + dbfield - &(dbspec -> subfield[0]);
	 wmove(hostaux_win, dbfield -> lineno, strlen(dbfield -> field_name) + dbfield -> colno + 2);
	 *local_cursor = dbfield -> colno + strlen(dbfield -> field_name) + 2;
      }
   }

   return(A_OK);
	    
}

/*
 * move_dbspec_cursor: move the cursor on the lines within the access
 * commands region
 */

status_t move_dbspec_cursor(direction, local_field, local_cursor, dbspec_list)
   int direction;	/* direction of move */
   int *local_field;	/* current field */
   int *local_cursor;	/* current cursor column */
   dbspec_t *dbspec_list; /* access commands config */

{
   dbspec_t *dbspec = glob_dbspec;
   dbfield_t *dbfield;

   ptr_check(dbspec_list, dbspec_t, "move_dbspec_cursor", ERROR);
   ptr_check(local_field, int, "move_dbspec_cursor", ERROR);
   ptr_check(local_cursor, int, "move_dbspec_cursor", ERROR);

   dbfield = *local_field - WFIELD_DBSPEC + &(dbspec -> subfield[0]);

   switch(direction){

      case NEXT_CHAR:

         if(dbfield -> curr_val[0] != '\0'){

	    if(*local_cursor + 1 <= dbfield -> colno + strlen(dbfield -> curr_val) + strlen(dbfield -> field_name) + 2)
	       (*local_cursor)++;
	    else
	       *local_cursor = strlen(dbfield -> field_name) + 2;
	 }
	 break;

      case PREVIOUS_CHAR:

         if(dbfield -> curr_val[0] != '\0'){

	    if(*local_cursor - 1 >= dbfield -> colno + strlen(dbfield -> field_name) + 2)
	       (*local_cursor)--;
	    else
	      *local_cursor = strlen(dbfield -> field_name) + 2;
	 }
         break;

      case FIRST_CHAR:

	 *local_cursor = dbfield -> colno + strlen(dbfield -> field_name) + 2;

	 break;

      case LAST_CHAR:

         if(dbfield -> curr_val[0] != '\0')
	    *local_cursor = dbfield -> colno + strlen(dbfield -> curr_val) + strlen(dbfield -> field_name) + 2;
	 else
	    *local_cursor = dbfield -> colno + strlen(dbfield -> field_name) + 2;
   }

   return(A_OK);
}

/*
 * modify_record: the user has asked to modify the current record. Put up
 * menu of options
 */

int modify_record(currwin)
   WINDOW *currwin;

{

   extern int wrefresh PROTO((WINDOW *));
   extern int wgetch PROTO((WINDOW *));
   extern int wmove PROTO((WINDOW *, int, int));
   extern int touchoverlap PROTO((WINDOW *, WINDOW *));
   extern int delwin PROTO((WINDOW *));

   WINDOW *mod_menu = newwin(20, 60, 4, 10);
   int origy, origx;
   int y, x;
   int result;

   getyx(currwin, origy, origx);

   box(mod_menu, '|', '-');

   mvwprintw(mod_menu, LCUR_MODMEN_TITLE, CCUR_MODMEN_TITLE, MODMEN_TITLE);

   mvwprintw(mod_menu, LCUR_MODMEN_ADDSITE, CCUR_MODMEN_ADDSITE, MODMEN_ADDSITE);
   mvwprintw(mod_menu, LCUR_MODMEN_DELSITE, CCUR_MODMEN_DELSITE, MODMEN_DELSITE);
   mvwprintw(mod_menu, LCUR_MODMEN_ADDHOSTAUX, CCUR_MODMEN_ADDHOSTAUX, MODMEN_ADDHOSTAUX);
   mvwprintw(mod_menu, LCUR_MODMEN_DELHOSTAUX, CCUR_MODMEN_DELHOSTAUX, MODMEN_DELHOSTAUX);
   mvwprintw(mod_menu, LCUR_MODMEN_REACTIVATE, CCUR_MODMEN_REACTIVATE, MODMEN_REACTIVATE);
   mvwprintw(mod_menu, LCUR_MODMEN_FORCE_UPDATE, CCUR_MODMEN_FORCE_UPDATE, MODMEN_FORCE_UPDATE);
   mvwprintw(mod_menu, LCUR_MODMEN_FORCE_DB_DELETE, CCUR_MODMEN_FORCE_DB_DELETE, MODMEN_FORCE_DB_DELETE);

   mvwprintw(mod_menu, LCUR_MODMEN_QUIT, CCUR_MODMEN_QUIT, MODMEN_QUIT);

   
   mvwprintw(mod_menu, LCUR_MODMEN_SEL, CCUR_MODMEN_SEL, MODMEN_SEL);

   getyx(mod_menu, y, x);

   wrefresh(mod_menu);

   for(;;){

      char tmp_str[16];

      tmp_str[0] = wgetch(mod_menu);
      tmp_str[1] = '\0';

      if(tmp_str[0] == ''){
         wrefresh(curscr);
	 continue;
      }

      if(!isdigit(tmp_str[0])){
	 send_response("Invalid value '%s'", tmp_str);
	 wmove(mod_menu, y, x);
	 wrefresh(mod_menu);
	 continue;
      }

      result = atoi(tmp_str);

      switch(result){

	 /* Quit. No change */

	 case 0:
	    result = 0;
	    break;

	 /* Add site */
	    
	 case 1:
	    result = ADD_SITE;
	    break;

	 /* Delete a site */

	 case 2:
	    result = DELETE_SITE;
	    break;

	 /* add a new database */

	 case 3:
	    result =  ADD_HOSTAUX;
	    break;
	    
	 /* delete a database */
	 
	 case 4:
	    result = DELETE_HOSTAUX;
	    break;

	 /* reactive current database*/

	 case 5:
	    result = REACTIVATE_DATABASE;
	    break;

	 /* force update on current database */

	 case 6:
	    result = FORCE_UPDATE;
	    break;

	 /* force deletion of current database */

	 case 7:
	    result = FORCE_DB_DELETE;
	    break;

	 default:
	    send_response("Invalid value '%s'", tmp_str);
	    wmove(mod_menu, y, x);
	    wrefresh(mod_menu);
	    continue;
      }

      break;
   }

#if !defined(AIX) && !defined(SOLARIS)
   touchoverlap(mod_menu, hostaux_win);
   touchoverlap(mod_menu, hostdb_win);
   delwin(mod_menu);
   wrefresh(hostaux_win);
   wrefresh(hostdb_win);
   wmove(currwin, origy, origx);
   wrefresh(currwin);
   wrefresh(curscr);
#else
   touchwin(hostaux_win);
   touchwin(hostdb_win);
   delwin(mod_menu);
   wnoutrefresh(hostaux_win);
   wnoutrefresh(hostdb_win);
   wmove(currwin, origy, origx);
   wnoutrefresh(currwin);
/*   wrefresh(curscr); */
   doupdate();
#endif

   return(result);
}

void display_hostlist()

{


   extern int wrefresh PROTO((WINDOW *));
   extern int werase PROTO((WINDOW *));

   hostname_t *myhptr;
   int maxlen;
   int currlen;
   int i,j,k;
   pathname_t tmp_str1;
   hostname_t *holdtop = (hostname_t *) NULL;

   

   ptr_check(hostptr, hostname_t , "display_hostlist",);
   ptr_check(hostlist, hostname_t, "display_hostlist",);

   /* calculate maximum hostname length */

   if(hostlist_win == (WINDOW *) NULL){

      for(myhptr = hostlist, maxlen = 0; myhptr < hostlist + hostcount; myhptr++)
	  if((currlen = strlen((char *) myhptr)) > maxlen)
	     maxlen = currlen;

      /* calculate where the columns should be */

      i = COLS / (maxlen + MIN_COL_DIST);

      curr_width = maxlen + MIN_COL_DIST;

      for(j = 0, k = 0; k < i; k++){

	 hostcols[k] = j;

	 j += curr_width;
      }


      hostcols[k] = -1;

      nocols = k;

      maxsites = nocols * 21;

      hostlist_win = newwin(0, 0, 1, 0);

      werase(hostlist_win);
      box(hostlist_win, ' ', '-');

      tophost = hostptr;

      sprintf(tmp_str, "%%-%ds",  curr_width);

   }

   holdtop = tophost;

   if((hostptr - tophost < 0) || (hostptr - tophost >= maxsites))
      tophost = hostptr;

   if(tophost < hostlist)
      tophost = hostlist;

   if(tophost > hostlist + hostcount)
      tophost = hostlist + hostcount - maxsites + 1;



   if(tophost != holdtop){
      werase(hostlist_win);
      box(hostlist_win, ' ', '-');
   }

   for(i = 1, k = 0, j = maxsites, myhptr = tophost;
       (myhptr < hostlist + hostcount)  && (hostcols[k] != -1) && j > 0; j--){


      if(myhptr == hostptr){
	 sprintf(tmp_str1, "%%-%ds", curr_width);
	 mvwprintw(hostlist_win, i, hostcols[k], tmp_str1, " ");
	 wstandout(hostlist_win);
	 mvwprintw(hostlist_win, i, hostcols[k], myhptr);
	 wstandend(hostlist_win);
      }
      else{
	 if(local_hostptr == myhptr){
	    mvwprintw(hostlist_win, i, hostcols[k], myhptr);
	 }
	 else 
         if(local_tophost != tophost){
            mvwprintw(hostlist_win, i, hostcols[k],  myhptr);
	 }
      }

      i++;

      if(i == 22){

	 i = 1;
	 k++;
      }

      myhptr++;
   }


   local_tophost = tophost;
   local_hostptr = hostptr;

   wrefresh(hostlist_win);
}
