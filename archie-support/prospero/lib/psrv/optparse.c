/* optparse.c */
/*
 * Copyright (c) 1992 by the University of Southern California
 *
 * For copying and distribution information, please see the file <usc-copyr.h>
 */
/* Ground out by swa@isi.edu */
#include <usc-copyr.h>

#include <pfs.h>
#include <pparse.h>
#include <psrv.h>
#include <perrno.h>
#include <stdlib.h>           /* For malloc and free */

#ifndef NULL
#define NULL 0
#endif

#if 0
static OPT  free = NULL;
#endif
int         opt_count = 0;
int         opt_max = 0;

#if 0                           /* XXX Options parsing isn't great; but it
                                   turns out that we don't need to get fancy
                                   yet.  What we have will suffice.*/

/* Initialize option parsing. Feed it an unquoted string containing a

   + or , -separated list of options.  It assumes that it may freely overwrite
   the string, and make memory references to it.  */

OPT
#ifdef __STDC__
optparse(char *optionstr)
#else
optparse(optionstr)
    char *optionstr;
#endif
{
    extern int perrno;
    perrno = PSUCCESS;
    OPT retval = NULL;
    
    while(*optionstr) {
        OPT nopt = optalloc();
        char buf[MAX_DIR_LINESIZE];
        char *bufp;

    }
    
}


static struct opt *
optalloc(char * name)
{
    OPT retval;

    if (free) {
        retval = free;
        free = free->next;
    } else {
        retval = malloc(sizeof (OPT_ST));
        if (!retval) return NULL;
        ++opt_max;
    }
    ++opt_count;

    retval->name= name;
    retval->used = 0;
    retval->args = NULL;
    retval->argstring = NULL;
    retval->next = NULL;

    return retval;
}

static 
#ifdef __STDC__
optfree(OPT opt)
#else
optfree(opt)
    OPT opt;
#endif
{
    optlfree(opt->args);
    free(opt);
}

void 
#ifdef __STDC__
optlfree(OPT opts)
#else
optlfree(opts)
OPT opts;
#endif
{
    while (opts) {
        nextop = opts->next;
        optfree(opts);
        opts = nextop;
    }
}


int
optquery(opts, optname)
    OPT opts;
    char optname[];
{
}


/* # of arguments to the option.  0 if none specified. */
int 
optnargs(opts,  optname)
    OPT opts;
    char optname[];
{
}


/* Any options which haven't been queried yet.  Check for leftovers! 
   This needs to be freed.
 */ 
OPT
optremaining()
{
}
#endif
