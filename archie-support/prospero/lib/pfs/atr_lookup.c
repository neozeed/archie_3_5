/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

/* This file will probably go away after we define REALLY_NEW_FIELD
   */
#include <usc-copyr.h>

/* Written by swa@ISI.EDU, Sept. 21--24, 1992 */

#include <pfs.h>
#include <pprot.h>
#include <pparse.h>

static struct atr_type_by_field_name {
    char *name;
    char type;
}   atr_type_by_field_name[] = {  
    /* Not all fields are defined here yet. */
    "ACCESS-METHOD",    ATR_SEQUENCE,
    "BASE-TYPE",        ATR_SEQUENCE,
    "CLOSURE",          ATR_LINK,
    "DEST-EXP",         ATR_SEQUENCE,
    "FILTER",           ATR_FILTER,
    "HOST",             ATR_SEQUENCE,
    "HOST-TYPE",        ATR_SEQUENCE,
    "HSONAME",          ATR_SEQUENCE,
    "HSONAME-TYPE",     ATR_SEQUENCE,
    "ID",               ATR_SEQUENCE,
    "LINK-TYPE",        ATR_SEQUENCE,
    "NAME-COMPONENT",   ATR_SEQUENCE,
    "OWNER",            ATR_SEQUENCE,
    "TARGET",           ATR_SEQUENCE,
    "TTL",              ATR_SEQUENCE,
    "VERSION",          ATR_SEQUENCE,
    NULL,               ATR_UNKNOWN
};


static struct atr_type_by_type_name {
    char *name;
    char type; 
} atr_type_by_type_name[] = {
    "FILTER",       ATR_FILTER,
    "LINK",         ATR_LINK,
    "SEQUENCE",     ATR_SEQUENCE,
    "ASCII",        ATR_SEQUENCE, /* XXX Backwards compatability; will go away
                                     soon.  */
    NULL,           ATR_UNKNOWN
};

/* used by atr_in. */
int
lookup_avtype_by_field_name(const char aname[])
{
    struct atr_type_by_field_name *p;
    for (p = atr_type_by_field_name; p->name; ++p) 
        if (strequal(p->name, aname))
            return p->type;
    return ATR_UNKNOWN;
}

/* If you add elements to this table, you will also need to change pfs.h */


int
lookup_avtype_by_avtypename(const char t_avtype[])
{
    struct atr_type_by_type_name *p;
    for (p = atr_type_by_type_name; p->name; ++p) 
        if (strequal(p->name, t_avtype))
            return p->type;
    return ATR_UNKNOWN;
}


char *
lookup_avtypename_by_avtype(int avtype)
{
    struct atr_type_by_type_name *p;
    for (p = atr_type_by_type_name; p->name; ++p) 
        if (p->type == avtype)
            return p->name;
    return "UNKNOWN-TYPE-NAME-PLEASE-COMPLAIN-TO-THE-MAINTAINER";
}




/* If you add elements to this table, you will also need to change pfs.h */

static struct aban {
    char *name;
    char precedence; 
} precedence_by_precedencename[] = {
    "OBJECT",       ATR_PREC_OBJECT,
    "LINK",         ATR_PREC_LINK,
    "CACHED",       ATR_PREC_CACHED,
    "REPLACEMENT",  ATR_PREC_REPLACE,
    "ADDITIONAL",   ATR_PREC_ADD,
    NULL,           ATR_PREC_UNKNOWN
};

int
lookup_precedence_by_precedencename(const char t_precedence[])
{
    struct aban *p;
    for (p = precedence_by_precedencename; p->name; ++p) 
        if (strequal(p->name, t_precedence))
            return p->precedence;
    return ATR_UNKNOWN;
}


char *
lookup_precedencename_by_precedence(int precedence)
{
    struct aban *p;
    for (p = precedence_by_precedencename; p->name; ++p) 
        if (p->precedence == precedence)
            return p->name;
    return "UNKNOWN-APPLIES-TO-VALUE-PLEASE-COMPLAIN-TO-THE-MAINTAINER";
}
