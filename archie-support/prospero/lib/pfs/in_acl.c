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

static int 
    in_acl_data(INPUT in, char *command, char *next_word, ACL value); 

int in_ge1_acl(INPUT in, char *command, char *next_word, ACL *aclp)
{
    ACL nacl = acalloc();
    int retval;

    if (!nacl) out_of_memory();
    if(retval = in_acl_data(in, command, next_word, nacl)) {
        acfree(nacl);
        return perrno = retval;
    }
    if(retval = in_acl(in, &nacl->next)) {
        acfree(nacl);
        return perrno = retval;
    }
    /* now need to correct the doubly-linked list. */
    if (nacl->next) {
        nacl->previous = nacl->next->previous;
        nacl->next->previous = nacl;
    }
    *aclp = nacl;
    return PSUCCESS;
}

/* This function is called with in_nextline() referring to the first line that
   might contain ACL data.  It sets perrno and p_err_string if bad data is
   found, and ALSO returns an error code. */
/* Reads in a string of ACL lines. */

int
in_acl(INPUT in, ACL *aclp)
{
    ACL list = NULL;            /* head of a linked list of new acls. */
    char *command, *next_word;
    
    while(in_nextline(in) && strnequal(in_nextline(in), "ACL", 3)) {
        ACL nacl = acalloc();
        int retval;             /* return code from function calls. */

        if (!nacl) {
            aclfree(list);
            out_of_memory();
        }
        APPEND_ITEM(nacl, list);
        if(retval = in_line(in, &command, &next_word)) {
            aclfree(list);
            return perrno = retval;
        }
        if(retval = in_acl_data(in, command, next_word, nacl)) {
            aclfree(list);
            return perrno = retval;
        }
        /* We just safely read in the rest of the line.  */
        /* Read in additional lines to skip over any RESTRICTION lines. */
        while (in_nextline(in)
               && strnequal(in_nextline(in), "RESTRICTION ", 12)) {
            extern int p__server;
            if(retval = in_line(in, &command, &next_word)) {
                aclfree(list);
                return perrno = retval;
            }
            if (p__server) {
                aclfree(list);
                p_err_string = qsprintf_stcopyr(p_err_string,
                       "UNIMPLEMENTED This server cannot handle RESTRICTIONS \
yet: %s", command);
                return perrno = retval;
            } else {
                pwarn = PWARNING;
                p_warn_string = qsprintf_stcopyr(p_warn_string, 
                         "The server sent an ACL with a RESTRICTION, but we \
don't know what it means: %s", command);
                /* just go on; it's only a warning. */
            }                
        }
    }
    *aclp = list;             /* safe & done. */
    return PSUCCESS;
}


static int 
in_acl_data(INPUT in, char *command, char *next_word, ACL value)
{
    AUTOSTAT_CHARPP(t_acetypep);
    AUTOSTAT_CHARPP(t_atypep);
    AUTOSTAT_CHARPP(t_rightsp);
    char	*p_principals;
    extern char *acltypes[];

    p_principals = NULL;
    if(qsscanf(next_word, "%'&s %'&s %'&s %r",
               &*t_acetypep, &*t_atypep, &*t_rightsp, &p_principals) < 3) {
        p_err_string = qsprintf_stcopyr(p_err_string, 
                 "Malformed ACL line: %'s", command);
        return PARSE_ERROR;
    }
    for(value->acetype = 0;acltypes[value->acetype];
        (value->acetype)++) {
        if(strequal(acltypes[value->acetype],*t_acetypep))
            break;
    }
    if(acltypes[value->acetype] == NULL) {
        p_err_string = qsprintf_stcopyr(p_err_string, 
                 "Unknown ACL type: %'s", command);
        return PARSE_ERROR;
    }
    /* We stcopy() twice; this is not necessary. */
    value->atype = stcopyr((**t_atypep ? *t_atypep : NULL), value->atype);
    value->rights = stcopyr((**t_rightsp ? *t_rightsp : NULL), value->rights);
    value->principals = (p_principals ? qtokenize(p_principals) : NULL);
    return PSUCCESS;
}

