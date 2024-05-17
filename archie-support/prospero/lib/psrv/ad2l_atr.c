/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * Written by bcn, 17 January 1993
 */

#include <usc-copyr.h>

#include <stdarg.h>
#include <pfs.h>
#include <perrno.h>

/*
 * ad2l_seq1_atr - add single element sequence attribute to link
 *
 *    The routines ad2l_seq1_atr is intended to provide a high level 
 *    interface for adding a common form of attribute to a link,
 *    adding a sequence attribute.  If compiled under ANSI C, it
 *    allows the addition of sequence attributes.  If under non
 *    ANSI C, it allows the addition of a sequence attribute with
 *    a single element value.
 *
 *    This functions is intended for use by information service providers
 *    converting information from their database to link information to be
 *    returned by a Prospero server.
 */
int
ad2l_seq_atr(VLINK l,           /* Link to receive attribute      */
	     char precedence,   /* To what the attribute applies  */
	     char nature,       /* Nature of the attribute        */
	     char *aname,       /* The name of the attribute      */
	     ...)               /* Elements of the value          */
{
    va_list 	ap;
    char 	*seq_elem;           /* Element of sequence            */

    PATTRIB	at = atalloc(); 

    if(!at) RETURNPFAILURE;

    at->precedence = precedence;
    at->nature = nature;
    at->aname = stcopy(aname);
    at->avtype = ATR_SEQUENCE;
    va_start(ap, aname);
    while(seq_elem = va_arg(ap, char *)) {
	at->value.sequence = tkappend(seq_elem, at->value.sequence);
    }
    va_end(ap);
    APPEND_ITEM(at, l->lattrib);
    return(PSUCCESS);
}


/*
 * ad2l_am_atr - add access method attribute to link
 *
 *    The routine ad2l_am_atr is intended to provide a high level 
 *    interface for adding the common form of the access method attribute to  
 *    a link, adding an access method attribute with the first four generic
 *    arguments null (the value will be taken from the link itself). 
 *
 *    This function is intended for use by information service providers
 *    converting information from their database to link information to be
 *    returned by a Prospero server.
 */
void
ad2l_am_atr(VLINK l,            /* Link to receive attribute      */
	     char *am,          /* The access method              */
	     ...)               /* Specific elements of the value */
{
    PATTRIB	at = atalloc();  
    va_list 	ap;
    char 	*arg;  /* Arguments to access method */

    va_start(ap, am);
    arg = va_arg(ap, char *);
    va_end(ap);

    at->precedence = ATR_PREC_CACHED;
    at->nature = ATR_NATURE_FIELD;
    at->aname = stcopy("ACCESS-METHOD");
    at->avtype = ATR_SEQUENCE;
    at->value.sequence = tkappend(am, at->value.sequence);
    
    if(arg) {
	at->value.sequence = tkappend("", at->value.sequence);
	at->value.sequence = tkappend("", at->value.sequence);
	at->value.sequence = tkappend("", at->value.sequence);
	at->value.sequence = tkappend("", at->value.sequence);
	at->value.sequence = tkappend(arg, at->value.sequence);
    }	    
	
    while(arg = va_arg(ap, char *)) {
	at->value.sequence = tkappend(arg,at->value.sequence);
    }
    APPEND_ITEM(at, l->lattrib);
}
