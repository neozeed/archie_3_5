/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>

#include <pfs.h>
#include <pparse.h>

/* This just sends out the data for a link, but does not prefix it with the
   word LINK.  It will suffix it with arguments, if needed. */
#define chknl(s) ((s) ? (s) : "")
int
out_link(OUTPUT out, VLINK vl, int nesting, TOKEN args)
{
    int retval;
    assert(vl);
    qoprintf(out, " %c %s %'s %s %'s %s %'s %ld", vl->linktype,
             chknl(vl->target), chknl(vl->name), chknl(vl->hosttype), 
             chknl(vl->host), chknl(vl->hsonametype), chknl(vl->hsoname),
             vl->version, 0);
    if (vl->dest_exp) {
        char * cp = NULL;
        qoprintf(out, " DEST-EXP %s", 
                 cp = p_timetoasn_stcopyr(vl->dest_exp, cp));
        stfree(cp);
    }
    if (args) {
        qoprintf(out, " ARGS");
        retval = out_sequence(out, args); /* out_sequence() terminates
                                             with a \n for us. */
    } else {
        retval = qoprintf(out, "\n");
    }
#if 0
    if (vl->f_magic_no)
        qoprintf(out, "ID REMOTE %ld\n", vl->f_magic_no, 0);
#else /* There has been a minor format change; ID is now preferably sent as an
         ID line, not as an attribute line. */
    if(vl->f_magic_no) {
        int i;

        qoprintf(out, "ATTRIBUTE", 0);
        for (i = nesting; i; --i) {
            qoprintf(out, ">", 0);
        }
        qoprintf(out, " LINK FIELD ID SEQUENCE REMOTE %ld\n", vl->f_magic_no);
        retval = qoprintf(out, "\n");
    }
#endif
    /* Send recursive sub-attributes if we're already nested.  But, if we're at
       the top-level (nesting == 0), we could only have gotten here by being
       called by list_name(), which makes its own decisions about which
       attributes to send.  Well, we might have also been called by dswdir(),
       but that's OK. */
    if (nesting) {
        PATTRIB at;
        for (at = vl->lattrib; at; at = at->next)
            out_atr(out, at, nesting);
    }
    return(retval);
}
