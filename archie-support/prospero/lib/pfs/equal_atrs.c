/* Copyright (c) 1992, 1993 by the University of Southern California.
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */

#include <usc-copyr.h>
#include <pfs.h>

/* This function is used by edit_link_info() and edit_object_info() in the
   server to determine whether two attributes are the same.  It will eventually
   also be used for merging attributes in VLS and other clients.
*/
int
equal_attributes(PATTRIB a1, PATTRIB a2)
{
    if (a1->precedence != a2->precedence 
        || a1->nature != a2->nature
        || a1->avtype != a2->avtype
        || !strequal(a1->aname, a2->aname))
        return FALSE;
    switch(a1->avtype) {
    case ATR_SEQUENCE:
        return equal_sequences(a1->value.sequence,
                               a2->value.sequence); 
    case ATR_FILTER:
        return equal_filters(a1->value.filter, a2->value.filter);
    case ATR_LINK:
        return vl_equal(a1->value.link, a2->value.link);
    default:
        internal_error("Invalid attribute type!");
        /*NOTREACHED */
    }
    /* NOTREACHED */
    assert(0);
    return(-1); /*Keep gcc happy*/
}


int
equal_filters(FILTER f1, FILTER f2)
{
    if (f1->name && f2->name) {     /* Both PREDEFINED */
        if (!strequal(f1->name, f2->name))
            return FALSE;
    } else if (f1->link && f2->link) { /* both LOADABLE */
        if (!vl_equal(f1->link, f2->link))
            return FALSE;
    } else {                    /* One PREDEFINED, other LOADABLE */
        return FALSE;
    }
    return (f1->type == f2->type
            && f1->execution_location == f2->execution_location 
            && f1->pre_or_post == f2->pre_or_post
            && equal_sequences(f1->args, f2->args));
}
