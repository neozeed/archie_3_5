/* pparse.h */
/* Copyright (c) 1992, 1993 by the University of Southern California
   For copying information, see the file <usc-copyr.h>
*/
#ifndef PPARSE_H_INCLUDED
#define PPARSE_H_INCLUDED
#include <usc-copyr.h>
#ifndef FILE
#include <stdio.h>
#endif
#ifndef PFS_SW_ID
#include <pfs.h>
#endif

#include <stddef.h>
#include <stdarg.h>
#include <ardp.h>

/* This structure is used by the parsing code.  It defines a standard IO type
   that lets the parsing code know where to read its input from.  This code is
   shared among the client protocol interface, server protocol interface, and
   server directory format reading mechanism.  All information about the next
   location to read input from is contained INSIDE this structure.
   That means that if we make a duplicate of the structure (using input_copy())
   and feed it to a function that resets the pointer (e.g., in_inc(), or
   in_line()), the input pointer will be reset for the copy but not for the
   original. 

   If there are any data structures which have been allocated for an INPUT,
   then input_freelinks() will free them.   At the moment, this is a null
   macro. 

   The user is responsible for freeing any memory that the user passed to
   rreqtoin() or filetoin().  The sending functions will normally do this, as
   will fclose().

   The only low-level functions that actually read from this all eventually
   call in_readc() and in_readcahead() and in_incc().
*/  

enum iotype {IO_UNSET = 0, IO_RREQ, IO_STRING, IO_FILE, IO_BSTRING};

struct input {
    enum iotype sourcetype;
    RREQ rreq;                 /* server or client reading */
    struct ptext *inpkt;        /* packet in process.  Null iff no more input.
                                   */
    char *ptext_ioptr;          /* position of next character from within ptxt.
                                   This will point to a null iff there is no
                                   more input. */
    const char *s;              /* Current position in string or bstring */
    FILE *file;                    /* server, set if reading via dsrfile() or
                                      dsrdir() */
    long bstring_length;        /* Length of the bstring.  */
    long offset;                /*  Offset from start of file or input stream. 
                                    This is repeatedly set and tested.
                                    Used to make sure that having two INPUT
                                    variables referring to the same FILE with
                                    different read pointers will not cause
                                    problems.  Also used in in_line() to
                                    allocate appropriate memory. */ 
                                /* Also used for BSTRINGs to see how much data
                                   is still readable. */
    int flags;
#define CONSUME_INPUT               0x1 /* qscanf() should consume input. 
                                         Not currently implemented. */
#define PERCENT_R_TARGET_IS_STRING  0x2 /* if set, the target for %r is a
                                          string.  Otherwise, it's an IN.
                                          This is temporarily set by the
                                          qsscanf() interface to qscanf()
                                          */ 
#define JUST_INITIALIZED            0x4 /* set upon initialization.  
                                         Not currently used. */
#define SERVER_DATA_FILE                 0x8 /* A server data file, 
                                                whether Cached by the
                                                wholefiletoin()  or treated in
                                                raw form by the (unused)
                                                filetoin() interface. */
};

typedef struct input INPUT_ST;
typedef struct input *INPUT;


/* Copy the whole schmeer. */
#define input_copy(src,dest) do {                               \
    *dest = *src;                                               \
} while (0)

#define input_freelinks() do ; while (0)

/* All of these input functions, except for input_line, report their errors to
   their callers. */
/*   in_line does not do this, though. */

/* Set up for input.  Fills in its 2nd argument.  Found in lib/pfs/in_line.c */
extern void rreqtoin(RREQ rreq, INPUT in);
extern void filetoin(FILE *file, INPUT in);
extern int wholefiletoin(FILE *file, INPUT in);

/* These are the low-level input routines. */
/* This replaces in_line(), eventually. */
extern int qscanf(INPUT in, const char fmt[], ...);
extern int vqscanf(INPUT in, const char fmt[], va_list ap);
extern int in_line(INPUT in, char **commandp, char **next_wordp);
extern char *in_nextline(INPUT in); /* returns the command that the next
                                        in_line would read.   Just the first
                                        word is enough.  NULL If no more 
                                        stuff. */
extern int in_readc(INPUT in);
extern int in_readcahead(INPUT in, int howfar);
extern void in_incc(INPUT in);
extern int in_eof(INPUT in);   /* test for EOF status.  1 if true; 0 if not.
                                  */ 
/* These handy little functions parse input for us. */
extern int in_acl(INPUT in, ACL *aclp);
extern int in_ge1_acl(INPUT in, char *command, char *next_word, ACL *aclp);
extern int in_atr_data(INPUT in, char *command, char *next_word, int nesting,
                       PATTRIB at); 
extern int in_atrs(INPUT in, int nesting, PATTRIB *valuep);
extern int in_ge1_atrs(INPUT in, char *command, char *next_word,
                       PATTRIB *valuep); 
extern int in_filter(INPUT in, char *command, char *next_word, int nesting,
                     FILTER *valuep); 
extern int in_forwarded_data(INPUT in, char *command, char *next_word, 
                             VLINK dlink);
extern int in_id(INPUT in, long *magic_nop);
extern int in_select(INPUT in, long *magic_nop);
extern int in_link(INPUT in, char *command, char *next_word, int nesting, 
                   VLINK *valuep, TOKEN *argsp);
extern int in_sequence(INPUT in, char *command, char *next_word, 
                       TOKEN *valuep);

/* Used to look at error messages */
extern int scan_error(char *line, RREQ req);
/* This structure is used by the formatting code.  It defines a standard
     output type that lets the formatting code know where to send the output.
     */ 
struct output {
    RREQ req;                   /* Server -- replying to client request */
    RREQ request;               /* Client -- querying the server */
    FILE *f;                    /* Server, writing to dsfile.c or dsdir.c */
    /* No need for string output. */
};
typedef struct output *OUTPUT;
typedef struct output OUTPUT_ST;

/* requesttoout() and reqtoout() are inherently different, because libardp has
   an asymmetry between a client sending a request to a server and a server
   replying to a client's request.

   Requests to the server are sent as a unit, whereas replies are returned a
   packet at a time as data becomes available.  Also, the functions they use to
   push the output & terminate the request are different.
*/
/* Automatic reply and formatting utilities (unparsing, sort of :)). */
/* Set up for output.  Fills in its 2nd argument.*/
/* lib/pfs/cl_qoprintf.c */
extern int requesttoout(RREQ req, OUTPUT out);
/* lib/psrv/srv_qoprintf.c */
extern void reqtoout(RREQ req, OUTPUT out);
extern void filetoout(FILE *file, OUTPUT out);

/* outputs raw lines and data. */
extern int (*qoprintf)(OUTPUT out, const char format[], ...);
/* set to one of the two below. */
extern int cl_qoprintf(OUTPUT out, const char format[], ...);
extern int srv_qoprintf(OUTPUT out, const char format[], ...);

/* outputs formatted stuff. */
extern int out_acl(OUTPUT out, ACL acl);
extern int out_atr(OUTPUT out, PATTRIB at, int nesting);
extern int out_atrs(OUTPUT out, PATTRIB at, int nesting);
extern int out_filter(OUTPUT out, FILTER fil, int nesting);
extern int out_link(OUTPUT out, VLINK vlink, int nesting, TOKEN args);
extern int out_sequence(OUTPUT out, TOKEN tk);

/* in LIBPSRV. */



/* Attribute manipulation stuff that is used by the parsing code too. */
/* These report errors by writing to p_err_string and returning error codes. */
/* Used by in_link() in LIBPFS and by edit_link_info() in the server. */
extern int vl_add_atrs(PATTRIB at, VLINK clink); 
extern int vl_add_atr(PATTRIB at, VLINK clink);
extern void vl_atput(VLINK vl, char *name, ...);

/* Used ONLY by in_atrs() and atr_out() */

/* used by atr_out() */
extern char *   lookup_precedencename_by_precedence(int precedence);
extern char *   lookup_avtypename_by_avtype(int avtype);

/* used by in_atrs() */
extern int      lookup_avtype_by_field_name(const char aname[]);
extern int      lookup_precedence_by_precedencename(const char t_precedence[]);
extern int      lookup_avtype_by_avtypename(const char []);


/* This variable is set to the real function in the stdio library if you
   want the IN package to deal with input streams as defined in <stdio.h>.
   They are set by dirsrv.c and by shadowcvt.c
   They should be set when you set qoprintf().
*/
extern int (*stdio_fseek)();

#endif                          /* PPARSE_H_INCLUDED */
