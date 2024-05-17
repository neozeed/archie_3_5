/* vqsprintf.c
   Author: Steven Augart <swa@isi.edu>
   Written: 7/18/92 -- 7/24/92
   Long support added, 10/2/92
   vqsprintf() added, 10/6/92
    Support for field widths and zero-padded field widths added, 4/20/94  --swa


   I am really interested in comments on this code, suggestions for making it
   faster, and criticism of my style.  Please send polite suggestions for
   improvement to swa@isi.edu.

*/

/* Copyright (c) 1992 by the University of Southern California. */
/* For copying and distribution information, see the file <usc-copyr.h> */
#include <usc-copyr.h>

/* qsprintf() is not like the regular sprintf().  "qsprintf()" is faster than
   real sprintf.  It only handles a few commands:
   %d, %s, %%, and %c. 
   %b -- for bstring argument (like %s)
   %x -- lower-case HEX digits
   %X -- upper-case HEX digits
   
   It also handles the ' modifier, which may be applied to %d or %s or %c.
   thusly:  "%'d", "%'s", and "%c".  The ' modifier means "print this message
   using Prospero quoting".  The ' modifier will also treat empty strings as
   separate objects, so that the format sequence "%'s", when given the empty
   string as an argument, prints "''".  This is used for quoting tokens in
   text-based protocols such as Prospero.  

   It handles minimum field widths.  If no zero precedes a field width, the
   output  will be space-padded.  Otherwise, zero-padded.

   Discussing quoting requires some discussion of how we handle string
   concatenation.  Consider the format string "%'s%'s" and the two argument
   strings "This+" and "More Stuff".  The qsprintf invocation is:

      qsprintf(buf, sizeof buf, "%'s%'s", "This+", "More Stuff")

   qsprintf() will concatenate quoted output into one long quoted string.  This
   allows one to construct quoted strings from several components.  After the
   above call to qsprintf(), buf should contain:

               "This'+More Stuff'"

   This example also demonstrates that qsprintf() currently turns on quoting in
   the middle of strings, rather than always turning it on at the start of a
   string.  This allows the qsprintf() implementation to be faster; alternative
   implementations are equally valid.  So, the quoted strings "'a b'c",  
   "a' 'bc", and "'a bc'" are equivalent representations of the same 
   underlying character sequence.  

   Note that concatenating quoted and unquoted strings could lead to bizzare
   results.   If you print an unquoted object immediately following a quoted
   string, you get what you deserve.  For example, if you qsprintf using the
   format: "%'s'", you will get a dangling single quote at the end, which the
   qsscanf() at the other end will be unable to handle.  


   Regular sprintf returns the # of characters written, not including the
   trailing '\0' (NUL).  qsprintf() returns the # of characters it wrote to the
   string, OR the # of characters it would have written if there had been
   enough room.  Its return value INCLUDES the NUL ('\0').  Unlike regular
   sprintf(), qsprintf() takes the SIZE of the buffer it writes to as an
   argument.  This allows us to guard against buffer overflow.  qsprintf() will
   write partial strings to buffers which are not long enough.  This seems to
   be reasonable behavior to me.

   Moreover, the partial strings written by qsprintf() are guaranteed to be NUL
   ('\0') terminated.  This means that you can qsprintf() without checking the
   return code, and still safely hand the output buffer to other commands.

   Like in regular sprintf(), %c expects an int argument (not a char).  The int
   argument is then converted to an unsigned char when it is printed.  An
   unsigned char would have fit my intuition better, but I opted for historical
   compatibility.

   qsprintf() will call internal_error() if it encounters a malformed format
   string.  It could return 0 instead, but that would require lots of testing,
   and it's not clear to me why it is useful to return an error flag upon
   encountering a programmer error.  
*/

/*
   ** Implementation notes & future directions **

   The implementation of qsprintf() uses the inline function (or macro or
   regular function, if you are among the unfortunates who don't have GNU C)
   "needs_quoting(char c)" to define which characters need quoting in your
   particular protocol.  needs_quoting() is currently defined for the Prospero
   protocol.    Note that, if you define needs_quoting() to always say that a
   character needs quoting, the behavior of qsprintf will still be correct; it
   is never erroneous to quote conservatively.  It merely wastes a bit of
   buffer space. 

   At some point, we will want qsprintf() to be able to write a partial output
   string to a short buffer, and then write more of the output string to
   another buffer.  The need is not yet here, though.

   vqsprintf() is the varargs version, analagous to vsprintf() in ANSI C. 

   qsprintf_stalloc() will be the name of the version that calls the memory
   allocator to allocate enough room to print.   It returns NULL upon failure.
   And, of course, to complete the group, there will be vqsprintf_stalloc().

   This function is written without function calls where I might have
   otherwise used them.  That's because it has to be fast.

*/

/*
   ** Maintainer **

   Yes, we are actually maintaining qsprintf() and qsscanf() as part of the
   Prospero project.  We genuinely want your bug reports, bug fixes, and
   complaints.  We figure that improving qsprintf()'s portability will help us
   make all of the Prospero software more portable.  Send complaints and
   comments and questions to bug-prospero@isi.edu. 

   In the Prospero project, we use GNU C because it is the best C compiler we
   know of for UNIX systems.  However, if you port this code to a non-GNU C
   compiler, we will incorporate your changes into the source.
*/

   

#include <ardp.h>               /* this is bad.  This defines size_t already,
                                   indirectly.  This is an incompatibly with
                                   GCC's stddef.h. */ 

#ifndef __sys_stdtypes_h
#include <stddef.h>             /* size_t */
#endif
#include <stdarg.h>             /* for varargs facility */
#include <limits.h>
#include <pfs.h>
#include "charset.h"

/* Set MAX_INT_DECIMAL_DIGITS to the # of decimal digits the largest int on
   your machine might possibly need to represent it in printed form.  This is
   used to allocate an internal buffer.  You do not need space for the sign,
   nor for a trailing '\0' (NUL). */ 
#define MAX_INT_DECIMAL_DIGITS 10 /* enough for 32-bit ints. */
#define MAX_LONG_DECIMAL_DIGITS 20 /* enough for 64-bit longs. */


/* This page contains macro definitions that replace the inline auto functions.
   If you're using GNU C, you get to skip this page.   I detest macros and
   argument passing in regular C. */

#define GNUC_SIGSEGV_BUG  1      /* GNU C seems to be giving me these error
                                    messages with inline nested functions, for
                                    reasons which I don't fully understand.  I
                                    believe it was not correctly installed on
                                    our system; */
#if __GNUC__ &&  !GNUC_SIGSEGV_BUG
#define NESTED_FUNCTIONS 1 /* 1 if we have inline nested functions (A
                                     GNU C feature). */
#else
#define NESTED_FUNCTIONS 0 /* no inline nested functions */
#endif

#if !NESTED_FUNCTIONS
/* If we're not using GNUC, don't use inline nested functions. */

#define needs_quoting(c) in_charset(nq_cs, (c))
        
#define rawput(c) do { \
        register char c2 = (c); /* in case (c) has side effects. */ \
        if (buflen)                 \
            *buf++ = c2, --buflen; \
        ++n;                        \
} while (0)

#define enable_quoting() do { \
        if (!am_quoting) {  \
            rawput('\''); \
            ++am_quoting; \
        }\
} while(0)

#define disable_quoting() do { \
    if (am_quoting) {\
        rawput('\'');\
        am_quoting = 0;\
    }\
} while(0)

#define qput(c) do {\
    char c1 = (c); /* in case (c) has side effects. */ \
    if (needs_quoting(c1)) {\
        enable_quoting();\
        if (c1 == '\'')      /* quote the quote */\
            rawput('\'');\
    }\
    rawput(c1);\
} while(0)

#define uqput(c) do {\
    disable_quoting();\
    rawput(c);\
} while(0)

#define put(c) { \
    if (do_quoting)\
        qput(c);\
    else\
        uqput(c);\
}
#endif /*!NESTED_FUNCTIONS */


size_t
vqsprintf(char *buf, size_t buflen, const char *fmt, va_list ap)
{
    register int n = 0;         /* n is # of chars that we wrote, or that we
                                   would write if there were room enough.
                                   Includes '\0'. */
    int am_quoting = 0;         /* If we are quoting a string and
                                   enable_quoting has already been called. */
    char *buf_lastpos = buf + buflen - 1; /* Last character position of the
                                             buffer.  We will write a
                                             terminating '\0' here.  */

#if NESTED_FUNCTIONS
    /* These are all of our inline local functions.  If we have 
       a bug-free GNU C, then these are preferable to macros.  The
       corresponding macro definitions occur at the top of the file. */

    /* This would be faster to code as a table lookup.  However, we don't need
       the speed that badly yet, and there's other work to be done.  Perhaps
       later. --swa */
    inline int needs_quoting(char c) {
        return in_charset(nq_cs, c);
    }
    
    inline void rawput(char c) {
        if (buflen)
            *buf++ = c, --buflen;
        ++n;
    }
    
    inline void enable_quoting(void) {
        if (!am_quoting) {
            rawput('\'');
            ++am_quoting;
        }
    }

    inline void disable_quoting(void) {
        if (am_quoting) {
            rawput('\'');
            am_quoting = 0;
        }
    }

    inline void qput(char c) {
        if (needs_quoting(c)) {
            enable_quoting();
            if (c == '\'')      /* quote the quote */
                rawput('\'');
        }
        rawput(c);
    }

    inline void uqput(char c) {
        disable_quoting();
        rawput(c);
    }
            
#endif
    static int nq_cs_initialized = 0; /* set to 1 after nq_cs is initialized */
    static charset nq_cs;       /* Charset of characters that need quoting */

    if (buflen <= 0) buf_lastpos = NULL;
    if (!nq_cs_initialized) {
        p_th_mutex_lock(p_th_mutexPFS_VQSPRINTF_NQ_CS);
        /* Initialize nq_cs with the characters that must be quoted. */
        if (!nq_cs_initialized) { /* test again now that we're synchronized */
            new_empty_charset(nq_cs);
            /* traditional whitespace characters. */
            add_char(nq_cs, ' ');
            add_char(nq_cs, '\t');
            add_char(nq_cs, '\f');
            add_char(nq_cs, '\v');
            add_char(nq_cs, '\n');
            add_char(nq_cs, '\r');
            /* additional characters treated specially by the Prospero protocol. */
            add_char(nq_cs, '/');
            add_char(nq_cs, '\'');
            add_char(nq_cs, '+');
            add_char(nq_cs, ',');
            ++nq_cs_initialized;
        }
        p_th_mutex_unlock(p_th_mutexPFS_VQSPRINTF_NQ_CS);
    }
        
    do {
        if (*fmt == '%') {
            int do_quoting = 0; /* flag: will we do quoting? */
            int use_long = 0;   /* flag: is the numeric arg a long? */
            int pad_with_zeroes = 0; /* flag: If we pad, do we do so with 
                                        zeroes? */
            int min_field_width = 0; /* What's the minimum field width we need?
                                        */ 

#if NESTED_FUNCTIONS
            inline void put(char c) {
                if (do_quoting)
                    qput(c);
                else
                    uqput(c);
            }
#endif

        rescan:
            switch(*++fmt) {
            case '0':           /* might be a modifier */
                if (!min_field_width) {
                    ++pad_with_zeroes;
                    goto rescan;
                }
                /* Deliberate FALLTHROUGH */
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                min_field_width *= 10;
                min_field_width += (*fmt - '0'); /* works for ASCII character
                                                      set; not necessarily for
                                                      others. */ 
                goto rescan;
            case '\'':          /* modifier */
#ifndef NDEBUG
                if (do_quoting)
                    internal_error("qsprintf: two ' modifiers in a row");
#endif
                ++do_quoting;
                goto rescan;
            case 'l':          /* modifier */
#ifndef NDEBUG
                if (use_long)
                    internal_error("qsprintf: two l modifiers in a row");
#endif
                ++use_long;
                goto rescan;

            case '%':
#ifndef NDEBUG
                if (do_quoting)
                    internal_error("qsprintf: ' modifier does not apply to %");
#endif
                uqput('%');
                break;

            case 'd':
            if (use_long) {
                long i = va_arg(ap, long);
                char itoabuf[MAX_LONG_DECIMAL_DIGITS + 1] ;
                register char *bp;
                
                if (i < 0) {
                    put('-');
                    i = -i;
                }                
                bp = itoabuf;
                do {
                    *bp++ = "0123456789"[i % 10];
                } while ((i /= 10) > 0);
                /* Use the min_field_width as an index variable.  
                   bp - itoabuf is the length of the string about to be
                   printed.   As long as it's not long enough, print a space or
                   zero and go on. */
                while (min_field_width-- > bp - itoabuf) {
                    put(pad_with_zeroes ? '0' : ' ');
                }
                /* Reverse the string.  At least 1 char is in itoabuf. */
                do {
                    put(*--bp);
                } while (bp > itoabuf);
            } else {
                int i = va_arg(ap, int);
                char itoabuf[MAX_INT_DECIMAL_DIGITS];
                register char *bp;
                
                if (i < 0) {
                    put('-');
                    i = -i;
                }                
                bp = itoabuf;
                do {
                    *bp++ = "0123456789"[i % 10];
                } while ((i /= 10) > 0);
                /* Use the min_field_width as an index variable.  
                   bp - itoabuf is the length of the string about to be
                   printed.   As long as it's not long enough, print a space or
                   zero and go on. */
                while (min_field_width-- > bp - itoabuf) {
                    put(pad_with_zeroes ? '0' : ' ');
                }
                /* Reverse the string.  At least 1 char is in itoabuf. */
                do {
                    put(*--bp);
                } while (bp > itoabuf);
            }
                break;

                
            case 'x':           /* print in hex */
            if (use_long) {
                unsigned long i = va_arg(ap, unsigned long);
                char itoabuf[MAX_LONG_DECIMAL_DIGITS]; /* long enuf for hex */
                register char *bp;
                
                bp = itoabuf;
                do {
                    *bp++ = "0123456789abcdef"[i % 16];
                } while ((i /= 16) > 0);
                /* Use the min_field_width as an index variable.  
                   bp - itoabuf is the length of the string about to be
                   printed.   As long as it's not long enough, print a space or
                   zero and go on. */
                while (min_field_width-- > bp - itoabuf) {
                    put(pad_with_zeroes ? '0' : ' ');
                }
                /* Reverse the string.  At least 1 char is in itoabuf. */
                do {
                    put(*--bp);
                } while (bp > itoabuf);
            } else {
                unsigned int i = va_arg(ap, unsigned int);
                char itoabuf[MAX_INT_DECIMAL_DIGITS];
                register char *bp;
                
                bp = itoabuf;
                do {
                    *bp++ = "0123456789abcdef"[i % 16];
                } while ((i /= 16) > 0);
                /* Use the min_field_width as an index variable.  
                   bp - itoabuf is the length of the string about to be
                   printed.   As long as it's not long enough, print a space or
                   zero and go on. */
                while (min_field_width-- > bp - itoabuf) {
                    put(pad_with_zeroes ? '0' : ' ');
                }
                /* Reverse the string.  At least 1 char is in itoabuf. */
                do {
                    put(*--bp);
                } while (bp > itoabuf);
            }
                break;

            case 'X':           /* print in capitalized hex */
            if (use_long) {
                unsigned long i = va_arg(ap, unsigned long);
                char itoabuf[MAX_LONG_DECIMAL_DIGITS]; /* long enuf for hex */
                register char *bp;
                
                bp = itoabuf;
                do {
                    *bp++ = "0123456789ABCDEF"[i % 16];
                } while ((i /= 16) > 0);
                /* Use the min_field_width as an index variable.  
                   bp - itoabuf is the length of the string about to be
                   printed.   As long as it's not long enough, print a space or
                   zero and go on. */
                while (min_field_width-- > bp - itoabuf) {
                    put(pad_with_zeroes ? '0' : ' ');
                }
                /* Reverse the string.  At least 1 char is in itoabuf. */
                do {
                    put(*--bp);
                } while (bp > itoabuf);
            } else {
                unsigned int i = va_arg(ap, unsigned int);
                char itoabuf[MAX_INT_DECIMAL_DIGITS];
                register char *bp;
                
                bp = itoabuf;
                do {
                    *bp++ = "0123456789ABCDEF"[i % 16];
                } while ((i /= 16) > 0);
                /* Use the min_field_width as an index variable.  
                   bp - itoabuf is the length of the string about to be
                   printed.   As long as it's not long enough, print a space or
                   zero and go on. */
                while (min_field_width-- > bp - itoabuf) {
                    put(pad_with_zeroes ? '0' : ' ');
                }
                /* Reverse the string.  At least 1 char is in itoabuf. */
                do {
                    put(*--bp);
                } while (bp > itoabuf);
            }
                break;

            case 'c':
                /* Use the min_field_width as an index variable.  
                   bp - itoabuf is the length of the string about to be
                   printed.   As long as it's not long enough, print a space or
                   zero and go on. */
                while (min_field_width-- > 1) {
                    put(pad_with_zeroes ? '0' : ' ');
                }
                put(va_arg(ap, int));
                break;

            case 's':
            {
                register char *s = va_arg(ap, char *);

#ifndef NDEBUG
                /* special case for null pointer (why not?).  Catch a common
                   bug. */
                if (s == NULL)
                    s = "(NULL POINTER)";
#endif
                
                /* This test ensures that concatenation with a null string will
                   work.  It also ensures that a null string will be displayed
                   signly as '', if it is surrounded by whitespace or unquoted
                   characters. */ 
                if (do_quoting && *s == '\0')
                    enable_quoting();  

                /* Use the min_field_width as an index variable.  
                   printed.   As long as it's not long enough, print a space or
                   zero and go on. */
                while (min_field_width-- > strlen(s)) {
                    put(pad_with_zeroes ? '0' : ' ');
                }
                while (*s)
                    put(*s++);
            }
                break;

            case 'b':
            {
                register char *b = va_arg(ap, char *);
                int len = p_bstlen(b);

                /* No need to test for null pointer; p_bstlen() includes a
                   consistency check. */
                /* This test ensures that concatenation with a null string will
                   work.  It also ensures that a null string will be displayed
                   signly as '', if it is surrounded by whitespace or unquoted
                   characters. */ 
                if (do_quoting && len == 0)
                    enable_quoting();  

                /* Use the min_field_width as an index variable.  
                   len is the length of the string about to be
                   printed.   As long as it's not long enough, print a space or
                   zero and go on. */
                while (min_field_width-- > len ) {
                    put(pad_with_zeroes ? '0' : ' ');
                }
                while (len--)
                    put(*b++);
            }
                break;

#ifndef NDEBUG
            default:
                internal_error("qsprintf: unrecognized format directive");
                /*NOTREACHED*/
                break;
#endif
            }
        } else {                /* *fmt != '%' */
            uqput(*fmt);
        }
    } while (*fmt++);
    /* Note that structuring this as a do ... while loop means that '\0' will
       be stuck at the end of the output string (space permitting). */
    /* Stick on terminating NUL in case there was no space to write one.  This
       means we can avoid checking the return value from qsprintf() in routine
       cases, and still be guaranteed that the output buffer contains
       meaningful (i.e., NUL-terminated) data. */ 
    if (buf_lastpos) *buf_lastpos = '\0'; 
    return n;
}

#if 0
/* This is just for debugging. */

#include <stdio.h>

int
main()
{
    char sbuf[1000]; 
    char ibuf[80];
    int d;
    int i;

    fputs("Gimme string -->>", stdout);
    fflush(stdout);
    gets(ibuf);
    fputs("Gimme number -->", stdout);
    fflush(stdout);
    if (scanf("%d", &d)!= 1) {
        fputs("That's not a number!\n", stdout);
        exit(1);
    }
    i = qsprintf(sbuf, sizeof sbuf, "String*2: |%s%s|\nQuoted String*2: |%'s%'s|\n\
Number: %d\n\
Number appearing in a %%02d field width: |%02d|\n\
Number appearing in a %%2d field width: |%2d|\n\
Quoted Number: %'d\nHex number: %x\nUpper hex number:%X\n\
Number as Character: %c\nNumber as quoted character: %'c\n", 
                 ibuf, ibuf, ibuf, ibuf, d, d, d, d, d, d, d, d);
    fputs("Made it past the qsprintf!\n", stderr);
    fflush(stdout);
    puts("Just flushed STDOUT.  Here's the string:");
    fwrite(sbuf, 1, i - 1, stdout);
    puts("EOS");
    printf("qsprintf says sbuf had %d chars; strlen(sbuf) (if appropriate) said
%d\n", i, i > sizeof sbuf ? -1 : strlen(sbuf));
    return 0;
}
/* Local Variables: */
/* compile-command: "gcc -o t -I../../include -ggdb3 -pipe qsprintf.c vqsprintf.c libpfs.a ../ardp/libardp.a" */
/* End: */

#endif
