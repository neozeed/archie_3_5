/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <pfs.h>
#include <pparse.h>
#include <pprot.h>
#include <perrno.h>


/* reads in a single FILTER line. */

int 
in_filter(INPUT in, char *command, char *next_word, int nesting, 
          FILTER *valuep)
{

    char    t_filtype[MAX_DIR_LINESIZE];
    char    t_execloc[MAX_DIR_LINESIZE];
    char    t_pre_or_post[MAX_DIR_LINESIZE];
    char    t_predef_or_link[MAX_DIR_LINESIZE];
    char    t_name[MAX_DIR_LINESIZE];
    char    *p_args;
    int tmp;                    /* return val. from qsscanf(). */
    FILTER fil;                 /* The result. */

    tmp = qsscanf(next_word, "%!!s %!!s %!!s %!!s %r", 
                  t_filtype, sizeof t_filtype,
                  t_execloc, sizeof t_execloc, 
                  t_pre_or_post, sizeof t_pre_or_post,
                  t_predef_or_link, sizeof t_predef_or_link, &next_word);
    /* Set them. */
    if (tmp < 5) {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Filter description format error: %'s", command); 
        return perrno = PARSE_ERROR;
    }
    fil = flalloc();
    if (!fil) 
        out_of_memory();
    if(strequal(t_filtype, "DIRECTORY"))
        fil->type = FIL_DIRECTORY;
    else if (strequal(t_filtype, "HIERARCHY"))
        fil->type = FIL_HIERARCHY;
    else if (strequal(t_filtype, "OBJECT"))
        fil->type = FIL_OBJECT;
    else if (strequal(t_filtype, "UPDATE"))
        fil->type = FIL_UPDATE;
    else {
        flfree(fil);
        p_err_string = qsprintf_stcopyr(p_err_string, 
                 "Unknown filter type: %'s", command);
        return perrno = PARSE_ERROR;
    }        
    if (strequal(t_execloc, "CLIENT"))
        fil->execution_location = FIL_CLIENT;
    else if (strequal(t_execloc, "SERVER"))
        fil->execution_location = FIL_SERVER;
    else {
        flfree(fil);
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Unknown filter execution location: %'s", command);
        return perrno = PARSE_ERROR;
    }
    if (strequal(t_pre_or_post, "PRE"))
        fil->pre_or_post = FIL_PRE;
    else if (strequal(t_pre_or_post, "POST"))
        fil->pre_or_post = FIL_POST;
    else {
        flfree(fil);
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Unknown filter pre-or-post specification: %'s",
                 command);
        return perrno = PARSE_ERROR;
    }
    if(strequal(t_predef_or_link, "PREDEFINED")) {
        tmp = qsscanf(next_word, "%'s ARGS %r", t_name, &p_args);
        if (tmp < 1) {
            flfree(fil);
            p_err_string = qsprintf_stcopyr(p_err_string,
                     "Malformed filter spec: %'s", command);
            return perrno = PARSE_ERROR;
        }
        fil->name = stcopyr(t_name, fil->name);
        if (tmp == 2)
            fil->args = qtokenize(p_args);
    } else if (strequal(t_predef_or_link, "LINK")) {
        int retval;             /* return from subfunction. */
        if(retval = in_link(in, command, next_word, nesting, &(fil->link), 
                            &(fil->args))) { 
            flfree(fil);
            return retval;
        }
    } else {
        flfree(fil);
        p_err_string = qsprintf_stcopyr(p_err_string, 
              "Malformed filter spec, neither LINK nor PREDEFINED : %'s",
                 command);
        return perrno = PARSE_ERROR;
    }
    *valuep = fil;
    return PSUCCESS;

}
