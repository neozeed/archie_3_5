/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>

#include <pfs.h>
#include <pparse.h>


/* This sends out the data for a filter, but does not prefix it with the words
   ATTRIBUTE FILTER, or anything like that. */
int
out_filter(OUTPUT out, FILTER fil, int nesting)
{
    assert(fil);                /* sanity checks */
    switch(fil->type) {
    case FIL_DIRECTORY:
        qoprintf(out, " DIRECTORY");
        break;
    case FIL_HIERARCHY:
        qoprintf(out, " HIERARCHY");
        break;
    case FIL_OBJECT:
        qoprintf(out, " OBJECT");
        break;
    case FIL_UPDATE:
        qoprintf(out, " UPDATE");
        break;
    default:
        internal_error("unknown fil->type value.");
    }
    switch(fil->execution_location) {
    case FIL_SERVER:
        qoprintf(out, " SERVER");
        break;
    case FIL_CLIENT:
        qoprintf(out, " CLIENT");
        break;
    default:
        internal_error("unknown fil->execution_location");
    }
    switch(fil->pre_or_post) {
    case FIL_PRE:
        qoprintf(out, " PRE");
        break;
    case FIL_POST:
        qoprintf(out, " POST");
        break;
    default:
        internal_error("unknown fil->pre_or_post");
    }
    if (fil->name)  {
        qoprintf(out, " PREDEFINED %'s", fil->name);
        qoprintf(out, " ARGS");
        return out_sequence(out, fil->args); /* sequence_reply() terminates
                                                  with a \n for us. */
    } else {
        assert(fil->link);
        qoprintf(out, " LINK ");
        out_link(out, fil->link, nesting, fil->args);
    }
    return PSUCCESS;
}

