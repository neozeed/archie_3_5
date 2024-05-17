/*
 * Copyright (c) 1992, 1993  by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-license.h>

#include <pfs.h>
#include <pparse.h>
#include <pprot.h>

extern int p__server;           /* set if server is calilng this code. */

/* This function sends a protocol reply line in appropriate format describing
   the attribute AT.   It only outputs for a single attribute, not for a list
   of them. */

int
out_atr(OUTPUT out, PATTRIB at, int nesting)
{
    int i;

    assert(at);
    qoprintf(out, "ATTRIBUTE");
    for (i = nesting; i; --i) {
        qoprintf(out, ">");
    }
    if (at->nature == ATR_NATURE_FIELD) {
        qoprintf(out, " %'s FIELD %'s",
                 lookup_precedencename_by_precedence(at->precedence), 
                 at->aname);
#ifndef REALLY_NEW_FIELD
        /* Databases (files) get the new format right away.  */
        if (!p__server || out->f || 
            lookup_avtype_by_field_name(at->aname) == ATR_UNKNOWN) {
#endif
            qoprintf(out, " %'s", lookup_avtypename_by_avtype(at->avtype));
#ifndef REALLY_NEW_FIELD
        }
#endif
            
    } else if (at->nature == ATR_NATURE_INTRINSIC) {
        qoprintf(out, " %'s INTRINSIC %'s %'s",
                 lookup_precedencename_by_precedence(at->precedence),
                 at->aname, lookup_avtypename_by_avtype(at->avtype));
    } else {
        assert(at->nature == ATR_NATURE_APPLICATION);
        qoprintf(out, " %'s APPLICATION %'s %'s",
                lookup_precedencename_by_precedence(at->precedence),
                at->aname, lookup_avtypename_by_avtype(at->avtype));
    }
    switch(at->avtype) {
    case ATR_SEQUENCE:
        return out_sequence(out, at->value.sequence);
        break;
    case ATR_FILTER:
        return out_filter(out, at->value.filter, nesting + 1);
        break;
    case ATR_LINK:
        return out_link(out, at->value.link, nesting + 1, (TOKEN) NULL);
        break;
    default:
        internal_error("Unimplemented or Invalid attribute type!");
        /*NOTREACHED */
    }
    assert(0);    /* NOTREACHED */
    return(-1); /* To keep GCC happy */
}


