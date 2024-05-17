/*
 *  Copyright(c) 1993 by the University of Southern California
 *  For copying information, see the file <usc-license.h>
 */

#include <usc-license.h>
#include <ardp.h>
#include <pfs.h>
#include <pparse.h>

extern int pfs_debug;

int (*stdio_fseek)();

/* Returns the distinguished value EOF if end of input detected.
*/
int
in_readc(INPUT in)
{
    int c;

    switch(in->sourcetype) {
    case IO_FILE:
        if((*stdio_fseek)(in->file, in->offset, 0) == -1) {
            /* improper seek */
            if (pfs_debug) 
                fprintf(stderr, "in in_readc() an improper fseek was detected.");
            return '\0';
        }
        if((c = getc(in->file)) == EOF) {
            clearerr(in->file);        /* don't want this to stick around. */
        }
        return c;
    case IO_STRING:
        return *(in->s) 
            ? (unsigned char) *(in->s) : EOF;
    case IO_RREQ:
        return  (in->inpkt) 
            ? (unsigned char) *in->ptext_ioptr : EOF;
    case IO_BSTRING:
        return (in->offset < in->bstring_length) 
            ? (unsigned char) *(in->s) : EOF;
    default:
        internal_error("invalid in->iotype");
    }
    return(-1); /* Unreached - keeps gcc happy */
}


/* this is not too efficient.  Fix it one day (yeah, right.). */
int
in_readcahead(INPUT in, int howfar)
{
    INPUT_ST incpy_st;
    INPUT incpy = &incpy_st;

    assert(howfar >= 0);
    input_copy(in, incpy);
    while (howfar-- > 0)
        in_incc(incpy);
    return in_readc(incpy);
}


/* This function may legally be called on a stream which has already reached
   EOF.  In that case, it's a no-op. */
void 
in_incc(INPUT in)
{
    /* this takes advantage of the null termination trick. */
    
    int c;

    switch(in->sourcetype) {
    case IO_FILE:
        ++in->offset;
        break;
    case IO_STRING:
        if (in->s) ++(in->s);   /* don't increment past end of the string. */
        break;
    case IO_RREQ:
        if (in->inpkt) {
            ++in->ptext_ioptr;
            ++in->offset;
            /* Do a loop because there might be a crazy client that
               sends some packets in a sequence with empty length fields. 
               Skip any of them we encounter; go to the next packet with some
               content. */
            while (in->ptext_ioptr >= in->inpkt->start + in->inpkt->length) {
                in->inpkt = in->inpkt->next;
                if (in->inpkt == NULL)
                    break;
                in->ptext_ioptr = in->inpkt->text;
            }
        }
        break;
    case IO_BSTRING:
        if (in->offset < in->bstring_length) {
            ++in->offset;
            ++in->s;
        }
        break;
    default:
        internal_error("invalid in->iotype");
    }
}


/*
 * It is not at all clear what role this plays, given that in_readc() does
 * this test for us.  But it remains.
 */
int
in_eof(INPUT in)
{
    switch(in->sourcetype) {
    case IO_FILE:
        if((*stdio_fseek)(in->file, in->offset, 0) == -1) {
            /* improper seek */
            if (pfs_debug) 
                fprintf(stderr, "in in_readc() an improper fseek was detected.");
            return EOF;
        }
        if(getc(in->file) == EOF) {
            clearerr(in->file);        /* don't want this to stick around. */
            return EOF;
        }
        return 0;
    case IO_STRING:
        if (*(in->s)) return 0; 
        else return EOF;
    case IO_RREQ:
        if (in->inpkt) return 0;
        else return EOF;
    case IO_BSTRING:
        if (in->offset < in->bstring_length) return 0;
        else return EOF;
    }
    assert(FALSE); /*Never gets here */
    return -1;
}
