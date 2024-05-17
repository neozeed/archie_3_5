#ifndef RESULTS_H
#define RESULTS_H

#include <stdio.h>
#include "ansi_compat.h"
#include "prosp.h"


#define OFUN_PROTO proto_((FILE *, VLINK, TOKEN))


/*  
 *  `commands' is a sequences of strings controlling what results are output
 *  and in what format.  It is interpreted as a sequence of commands
 *  (describing what function to perform) and arguments to that function.
 *  
 *  Currently recognized formats are:
 *  
 *    SEARCH, {anonftp|domains|gopher|list|wais}, {html|text}
 *    FILE, {data|document}, {html|text}
 *    MENU, {html|text}
 *    MESSAGE, {<body of message>} // print "as is"
 *  
 *  Currently, this is represented by a list of TOKENs.  The list starts with
 *  a command (an all uppercase string), followed by zero or more arguments,
 *  and terminated by a TOKEN with a NULL string pointer.
 */  

struct Result
{
  VLINK res;
  TOKEN commands;
};


extern void printContents proto_((void));
extern void displayResults proto_((FILE *fp, struct Result *res));
extern void cancelOutputFunctions proto_((void));
extern int registerOutputFunction proto_((const char *cmd, int (*fn) OFUN_PROTO));

#endif
