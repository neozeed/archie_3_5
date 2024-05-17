/*
 * Copyright (c) 1991-1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <pserver.h>

#ifdef SERVER_SUPPORT_V1
#include <pfs.h>
#include <perrno.h>

/* cur_link->target is already set to l_type.  Return PSUCCESS if this is OK.
   */ 
int
convert_v1_ltype(char l_type[], VLINK cur_link)
{
    char *param;
    PATTRIB amat;     /* Access-method attribute. */

    if (strequal(l_type, "SYM-LINK")) {
        cur_link->target = stcopyr("SYMBOLIC", cur_link->target);
        return PSUCCESS;
    }
    if (!strnequal(l_type, "EXTERNAL(", 9))
        return PSUCCESS;
    if (strequal(l_type, "EXTERNAL(AFTP,BINARY)"))
        param = "BINARY";
    else if (strequal(l_type, "EXTERNAL(AFTP,TEXT)"))
        param = "TEXT";
    else
        RETURNPFAILURE;
    cur_link->target = stcopyr("EXTERNAL", cur_link->target);
    /* Ok, now set it. */
    amat = atalloc();
    if (!amat) out_of_memory();

    amat->precedence = ATR_PREC_LINK;
    amat->value.sequence = (TOKEN) NULL;
    amat->nature = ATR_NATURE_FIELD;
    amat->avtype = ATR_SEQUENCE;
    amat->aname = stcopyr("ACCESS-METHOD", amat->aname);
    
    amat->value.sequence = tkappend("AFTP", amat->value.sequence);
    /* next 4 are null strings; they are all the same, by convention. */
    amat->value.sequence = tkappend("", amat->value.sequence);
    amat->value.sequence = tkappend("", amat->value.sequence);
    amat->value.sequence = tkappend("", amat->value.sequence);
    amat->value.sequence = tkappend("", amat->value.sequence);
    amat->value.sequence = tkappend(param, amat->value.sequence); 
    APPEND_ITEM(amat, cur_link->lattrib);
    return PSUCCESS;

}
#endif          /* SERVER_SUPPORT_V1 */
