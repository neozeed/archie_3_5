/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * The menu API and client were written by Kwynn Buess (buess@isi.edu)
 */

#include <usc-copyr.h>


/* You can define these to be a root host and directory to retrieve by default
   if you are not running your own Prospero site.  We expect these options to
   be frequently used in small and medium sized CWIS systems based on this
   browser. */
/* These definitions are commented out since the browser by default retrieves
   the root menu by looking at the prototype virtual system at ISI. */
#if 0                           /* UNCOMMENT HERE TO USE THESE */
#ifndef MENU_DEFAULT_HOST
#define MENU_DEFAULT_HOST "PROSPERO.ISI.EDU"
#endif

#ifndef MENU_DEFAULT_HSONAME    
#define MENU_DEFAULT_HSONAME "/pfs/pfsdat/guest/local_vsystems/prototype/MENU"
#endif
#endif

#ifndef PRINT_PROGRAM
#define PRINT_PROGRAM "lpr"
#endif

#ifndef TELNET_PROGRAM
#define TELNET_PROGRAM  "telnet"
#endif

#ifndef MAIL_SENDING_PROGRAM
#define MAIL_SENDING_PROGRAM "mail"
#endif

