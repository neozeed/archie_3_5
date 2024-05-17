/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-license.h>
 */

#include <usc-license.h>

#include <pfs.h>
#include <pparse.h>
#include <pprot.h>

int
out_sequence(OUTPUT out, TOKEN tk)
{
    for(; tk; tk = tk->next)
        qoprintf(out, " %'b", tk->token);
    return qoprintf(out, "\n");
}

