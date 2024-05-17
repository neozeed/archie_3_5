/*
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */

#include <usc-copyr.h>
#include <pfs.h>
#include <pparse.h>
#include <perrno.h>
#include <pprot.h>

int p__server = 0;                 /* set to 1 by dirsrv.c.  */

static int
in_attribute_value(INPUT in, char *command, char *next_word, int nesting, 
                   char avtype, union avalue *value);

int in_ge1_atrs(INPUT in, char *command, char *next_word, PATTRIB *valuep)
{
    PATTRIB at = atalloc();
    int retval;

    CHECK_MEM();
    *valuep = NULL;
    
    CHECK_PTRinBUFF(command,next_word);
    if (!at) out_of_memory();
    if (retval = in_atr_data(in, command, next_word, 0, at)) {
        atfree(at);
        return retval;
    }
    APPEND_ITEM(at, *valuep);
    if (retval = in_atrs(in, 0, &at->next)) {
        atfree(at);
        return retval;
    }
    /* now need to correct the doubly-linked list. */
    if (at->next) {
        at->previous = at->next->previous;
        at->next->previous = at;
    }
    return PSUCCESS;
}

/* Read in a series of protocol-style ATTRIBUTE lines. */
/* starts with in_nextline() set to the first possible ATTRIBUTE line, and
   returns with in_nextline() set to the next line of available text.  */
/* the caller is responsible for calling palfree() on whatever in_atrs() 
   delivers up. */
int 
in_atrs(INPUT in, int nesting, PATTRIB *valuep)
{
    char t_nesting[100];
    char *command, *next_word;
    
    PATTRIB list = NULL;     /* Head of a linked list of new attributes. */
    int retval;

    while(in_nextline(in)
          && qsscanf(in_nextline(in), "ATTRIBUTE%!!(>)", t_nesting, sizeof
                     t_nesting) == 1) {
        PATTRIB at;
        
        if(strlen(t_nesting) < nesting) 
            break;    /* we're done with reading the subats */
        at  = atalloc();
        if (!at) {
            atlfree(list);
            out_of_memory();
        }
        APPEND_ITEM(at, list);
        if (strlen(t_nesting) > nesting) {
            /* OOPS!  Unexpectedly deep nesting. */
            atlfree(list);
            p_err_string = qsprintf_stcopyr(p_err_string,
                     "ATTRIBUTE line sent with Nesting too deep; got %d, \
expected %d: %s", strlen(t_nesting), nesting, command);
            return perrno = PARSE_ERROR;
        }
        assert(strlen(t_nesting) == nesting);
        if(retval = in_line(in, &command, &next_word)) {
            atlfree(list);
            return perrno = retval;
        }
        CHECK_PTRinBUFF(command,next_word);
        if(retval = in_atr_data(in, command, next_word, nesting, at)) {
            atlfree(list);
            return perrno = retval;
        }
    }
    /* done! */
    *valuep = list;
    return PSUCCESS;
}


/*  We may need to recursively read the subattributes of a link, which means we
    may need to read multiple lines here. */
int
in_atr_data(INPUT in, char *command, char *next_word, int nesting, PATTRIB at)
{
    char t_nature[sizeof "APPLICATION"];
    AUTOSTAT_CHARPP(t_anamep);
    AUTOSTAT_CHARPP(t_avtypep);
    char t_precedence[40];
    int retval;
    int tmp;                    /* from qsscanf() */
    char *maybe_eol;       /* Need a pointer to end of line. */
                           /* Used as a temporary in two separate places in the
                              code. */

    CHECK_PTRinBUFF(command,next_word);
    tmp = qsscanf(next_word, "%!!s %!!s %'&s%r %r",
               t_precedence, sizeof t_precedence, 
               t_nature, sizeof t_nature, t_anamep, &maybe_eol, &next_word);
    if (tmp < 4) {
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Malformed attribute line: %s", command);
        return perrno = PARSE_ERROR;
    } else if (tmp == 4) {
        /* no tokens for data given.  This still might be legal -- e.g., a
           zero-length sequence. */ 
        next_word = maybe_eol;
#if 0
        /* This following item won't work because in_attribute_value expects
           NEXT_WORD to be a pointer into the same string indicated by COMMAND.
           This was causing a crash whenever we had a zero-item SEQUENCE. 
           --swa */
        next_word = "";     
#endif
    } /* else all is ok. */
    
    at->precedence = lookup_precedence_by_precedencename(t_precedence);
#ifndef REALLY_NEW_FIELD
    if(strequal(t_nature, "FIELD")) {
        char *cp;               /* temp. ptr. */

        at->nature = ATR_NATURE_FIELD;
        /* If the oldstyle field name field doesn't contain a valid old field
           name, try to parse it as a new style message. */
        if((at->avtype = lookup_avtype_by_field_name(*t_anamep)) 
           == ATR_UNKNOWN)
            goto new_field;
        /* If there's a t_avtype that matches the appropriate type for this
           field AND we have additional data following it then it's probably a
           new-style field. */
        tmp = qsscanf(next_word, "%&'s %r", t_avtypep, &cp);
        if (tmp == 2 && at->avtype == lookup_avtype_by_avtypename(*t_avtypep))
            goto new_field;
        /* Treat it as an old-style field. */
    } else                      /* Note the indentation below looks odd,
                                   but it's the most correct way I can think of
                                   to do it. --swa */
#endif
        if (strnequal(t_nature, "APPLICATION", 11)  || 
#ifdef REALLY_NEW_FIELD
               strnequal(t_nature, "FIELD", 5)  || 
#endif
               strnequal(t_nature, "INTRINSIC", 9)) {
        at->nature = (t_nature[0] == 'A' ? ATR_NATURE_APPLICATION
#ifdef REALLY_NEW_FIELD
                      : t_nature[0] == 'F' ? ATR_NATURE_FIELD
#endif
                      : ATR_NATURE_INTRINSIC);
    new_field:
        /* An independent use of maybe_eol as a temporary. */
        tmp = qsscanf(next_word, "%&'s%r %r", 
                      t_avtypep, &maybe_eol, &next_word);
        if (tmp < 2) {
            p_err_string = qsprintf_stcopyr(p_err_string, 
                     "Malformed ATTRIBUTE line: %s", command);
            return perrno = PARSE_ERROR;
        } else if (tmp == 2)
            next_word = maybe_eol;  /* no more data -- zero length sequence,
                                       etc. */ 
#if 0
        /* This following item won't work because in_attribute_value expects
           NEXT_WORD to be a pointer into the same string indicated by COMMAND.
           This was causing a crash whenever we had a zero-item SEQUENCE. 
           --swa */
            next_word = "";     
#endif
        if ((at->avtype = lookup_avtype_by_avtypename(*t_avtypep)) 
            == ATR_UNKNOWN) {
            p_err_string = qsprintf_stcopyr(p_err_string,
                    "Unknown Attribute Value type: %s", 
                    *t_avtypep);
            return perrno = PARSE_ERROR;
        }
	CHECK_MEM();
    } else {                /* Unknown t_nature */
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "Only APPLICATION, INTRINSIC, and FIELD attribute value \
types exist -- Malformed ATTRIBUTE line: %s", command);
        return perrno = PARSE_ERROR;
	CHECK_MEM();
    }
    at->aname = stcopy(*t_anamep);
    if (next_word) {
    	/* Lack of next_word is not an error, just dont fill out the value */
    if(retval = in_attribute_value(in, command, next_word, nesting, 
                                   at->avtype, &at->value)) { 
        /* Error message will have been sent by in_attribute_value */
        return perrno = retval;
    }
    }
    /* The entry AT now has all of its members filled in.  On to the next!
     */ 
    return PSUCCESS;
}

/* Based upon the value of the attribute, as passed in AVTYPE, parse the
   input line and store the results in VALUE.  This may cause recursive calls
   to in_attributes().    Return PSUCCESS or PFAILURE. 
   Command should be the head of a BSTRING and NEXT_WORD should be a pointer
   into the bstring.  */
static
int 
in_attribute_value(INPUT in, char *command, char *next_word, int nesting,
                   char avtype, union avalue *value)
{
    
    CHECK_PTRinBUFF(command,next_word);
    CHECK_MEM();
    switch(avtype) {
    case ATR_FILTER:
        return in_filter(in, command, next_word, nesting + 1,
                            &value->filter);
    case ATR_LINK:
        return in_link(in, command, next_word, nesting + 1,
                          &value->link, (TOKEN *) NULL); 
    case ATR_SEQUENCE:
        return in_sequence(in, command, next_word, &value->sequence);
    default:
        internal_error("Invalid attribute value type!");
        /*NOTREACHED */
    }
    /* NOTREACHED */
    return -1;      /* Keep gcc happy */
}

