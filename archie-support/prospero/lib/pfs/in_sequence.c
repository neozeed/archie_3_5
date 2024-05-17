/*
 * Copyright (c) 1992, 1993, 1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h> 
 */

#include <usc-license.h>

#include <pfs.h>
#include <pparse.h>

/* Command should be the head of a BSTRING and NEXT_WORD should be a pointer
   into the BSTRING.  */ 
/* Convert portion of command starting at *next_word to list of TOKENs */
int
in_sequence(INPUT in, char *command, char *next_word, TOKEN *valuep)
{
    CHECK_PTRinBUFF(command,next_word);
    *valuep =  p__qbstokenize(command, next_word);
    return PSUCCESS;
}

