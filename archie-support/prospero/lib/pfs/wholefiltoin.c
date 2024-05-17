/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */

#include <usc-copyr.h>

#include <pfs.h>
#include <pparse.h>
#include <perrno.h>

#include <sys/types.h>
#include <sys/stat.h>

/* Frees the buffer on subsequent calls. */
int
wholefiletoin(FILE *file, INPUT in)
{
    struct stat st_buf;
    AUTOSTAT_CHARPP(bufp);
    int tmp;                    /* temp. return value from subfunctions */
    
    if(fstat(fileno(file), &st_buf)) {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "wholefiletoin(): Couldn't fstat input file");
        return perrno = PFAILURE;
    }
    /* If buffer sufficiently large, don't bother calling malloc() twice. */
    if (p__bstsize(*bufp) < st_buf.st_size + 1) {
        if (*bufp) stfree(*bufp);
        if((*bufp = stalloc(st_buf.st_size + 1)) == NULL) {
        p_err_string = qsprintf_stcopyr(p_err_string,
            "wholefiletoin(): out of memory");
            return perrno = PFAILURE;
        }
    }
    tmp = fread(*bufp, sizeof (char), st_buf.st_size, file);
    if (tmp != st_buf.st_size) {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "wholefiletoin(): fread() returned %d, not the %d expected.",
                 tmp, st_buf.st_size);
        return perrno = PFAILURE;
    }
    (*bufp)[st_buf.st_size] = '\0'; /* treat it as a special null-terminated
                                    string.  */
    in->sourcetype = IO_STRING;
    in->rreq = NULL;
    in->s = *bufp;
    in->file = NULL;
    in->flags = SERVER_DATA_FILE;
    return PSUCCESS;
}
        
                 
