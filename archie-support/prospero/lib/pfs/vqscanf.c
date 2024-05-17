/* vqscanf.c
   Author: Steven Augart (swa@isi.edu)
   Designed, Documented, and Written: 7/18/92 -- 7/27/92
   Ported from Gnu C to full ANSI C & traditional C, 10/5/92
   & modifier added, detraditionalized: 2/16/93.
   3/2/93 converted from qsscanf() to vqscanf(); not all comments updated.
   Sorry. :)
*/

#define ASSERT2                 /* expensive debugging assertions */
/* Copyright (c) 1992, 1993 by the University of Southern California. */
/* For copying and distribution information, see the file <usc-copyr.h> */

#include <usc-copyr.h>

#include <stdarg.h>             /* ANSI variable arguments facility. */

/*
  This is NOT the ordinary sscanf()!  It works somewhat like sscanf(), but it
  does *not* recognize all of sscanf()'s options.

  In order to understand the rest of this documentation, you should be familiar
  with the sscanf() function.

  Like sscanf(), qsscanf() returns a count of the number of successful
  assignment matches.  The return value is not as important as it is in
  sscanf(), though, since you can check whether you matched everything by
  making your final conversion specifier be %r, setting the corresponding
  pointer to NULL, then checking whether the corresponding pointer was reset.

  sscanf() ignores blanks and tabs in the input string.  qsscanf() considers
  those to match a stretch of at least one horizontal whitespace (space or tab)
  character.  This means that horizontal whitespace is NOT ignored in the input
  string, nor in the format string.  In other words, occurrence of one or more
  space or a tab in the format string is equivalent to %*[ \t].  This isn't
  really a terribly   creative thing to map whitespace onto, but it fits my
  intuition a little bit better than sscanf()'s approach of ignoring it
  altogether.  Other whitespace characters (such as newline ('\n')) which
  appear in the format string must literally match the corresponding character
  in the input string.

  The assignment suppression (the '*' modifier) feature of sscanf() is
  supported.   

  The "long instead of int" (the 'l' modifier) feature of sscanf() is
  supported, but we have not yet had a need for the "short instead of int" (the
  'h' modifier) feature.

  The "'" modifier will unquote the input string.  It turns off normal
  processing of all field terminators inside a quoted string, until the quoting
  ends, at which point normal processing continues.  See psprintf() for a
  discussion of quoting.  If we read only part of a quoted string, either
  because the input string terminated early or because the field width ended
  before the quoted part of the string ended or because the buffer ran out of
  room (even with a '_' modifier), then we consider the match to have failed.
  (I have thought about this quite a bit.  If you have an 
  application for partial quoted string matches, send e-mail to swa@isi.edu;
  I'd like to hear about it.)

  Regular sscanf() has a notion of maximum field width.  For example, to read
  no more than 5 spaces or slashes into a string, one would give sscanf() the
  conversion specifier "%5[ /]".  "qsscanf()" also supports field width.
  Note that, according to the ANSI spec, all fields except for %n must contain
  at least one   character that matches.  (In our extended sscanf(), all
  conversion specifiers except for %r, %~ and %( must match at least 1 single
  character of output field.  This means that negative or zero length field
  widths are meaningless, and should never be specified.  Similarly, buffer
  lengths (see below) must contain at least enough room for one character (and
  the trailing NUL ('\0') in all cases of NUL-terminated strings). 

  The institution of quoted strings means that there is now a separation
  between input field width and number of characters read into the buffer.  To
  give an example, should the quoted string "''''" have a field width of 4 (the
  input characters) or 1 (the output characters)?  I have decided "field width"
  represents the number of charcters we are willing to read into the output
  buffer (exclusive of terminating '\0' -- perhaps not including the '\0' is a
  bad decision, but it's backward compatible with sscanf().

  Output buffer size may be specified by using the '_' modifier, the '$'
  modifier, or the '!' modifier.  

  _: If the '_' modifier is specified, this means that the output buffer size
     follows (including space for a terminating '\0', in the case of every
     string conversion).  After the output buffer is full, the match will
     succeed and we go on to the next format character.  It is quite similar to
     specifying maximum field width, except that maximum field width doesn't
     include space for the terminating '\0'.  If two '_' modifiers are
     specified, then the output buffer size is read from the next integer
     argument.

  $: If the '$' modifier is specified, the output buffer size follows.  The
     match will FAIL if the output buffer overflows.  A count of the # of
     successful matches is returned.  If two '$' modifiers are specified, the
     output buffer size is read from the next integer argument.  Note that '_'
     works just like '$' when we are scanning quoted strings, because partial
     matches for quoted strings are not useful.

  !: If the '!' modifier is specified, the output buffer size follows.  If the
     output buffer overflows, qsscanf() will return with the value of the
     integer constant BUFFER_OVERFLOW (guaranteed to be negative).  Note that
     buffer overflow and numeric overflow are the only situations in which
     qsscanf() returns negative values.  the output buffer overflows.  If two
     '!' modifiers are specified, the output buffer size is read from the next
     integer argument.  Note that '_' works just like '!'  when we are scanning
     quoted strings.


  A Prospero-specific modifier is '&'.  The '&' modifier is only implemented
  for the %s and %[ conversions.  The argument to %s or %[, instead of being a
  pointer to a buffer, is a char **.  The argument will have the Prospero
  stcopyr() function applied to it.  Therefore, the argument must be NULL or
  contain data previously allocated by Prospero's stcopy() or stcopyr()
  function.  This allows us to read strings of unlimited size without overflow.
  This works with the '\'' modifier.

  qsscanf() checks for integer overflow.  By default, or if the '!' modifier is
  specified, qsscanf() aborts processing and returns the value of the integer
  constant "NUMERIC_OVERFLOW" (guaranteed to be negative).  If the '_' modifier
  has been specified, then we will terminate the integer conversion upon
  potential overflow (just like specifying an input field width for an
  integer).  If the '$' modifier has been specified, then the match will fail
  upon integer overflow.  

  It is meaningful to combine the '_', '$', and '!' modifiers with the '*'
  (assignment suppression) modifier. 

  I expect that nobody will actually use the '_' modifier with numeric input,
  but the functionality is there.

  Conversion    Argument    Function
  Specifier     Type
  ------        -----       -----------

  %d            int *       Looks for an optionally signed decimal integer.
                            Note that we do NOT skip over leading whitespace
                            like sscanf() does, nor do we in any other
                            conversions.  Terminates on the first non-numeric
                            character, exclusive of an optional leading '-'.

                            Unlike in regular sscanf(), I have not added
                            functionality for longs and shorts to qsscanf(),
                            although they could be added easily.

  %s            char *      Whitespace-terminated non-zero-length sequence of
                            characters.  In the context of "%s", "whitespace"
                            means any one or more of "\n\r\t\v\f ".  This
                            conversion adds a terminating '\0' to the output
                            string. .  Note one exception to the
                            non-zero-length rule: 

                            "%'s" will return the zero-length string when fed
                            the input "''".  (This exception applies to all
                            cases where "non-zero-length" is mentioned in this
                            documentation.) 

  %S            char *      Matches all of the remaining characters in the
                            input buffer.  Adds a terminating '\0' to the
                            output string.  Will return a zero-length string if
                            that's all that's left, (and won't fail).  In other
                            words, %S never fails if you get to it.  "%'S" will
                            strip off a layer of quoting while it gobbles, if
                            you need it to.

  %r            char **     "Rest of the string".  Sets the pointer to point 
                            to the portion of the input string beyond this
                            point.  This is useful, for instance, to check
                            whether there was any leftover input in the string
                            beyond a certain point.  Applying maximum
                            field-width to this construct is meaningless.
                            Applying assignment suppression to this construct
                            is not useful.  %r never fails, if you get to it.

  %R            char **     "Rest of the string, skipping to the next line".
                            Equivalent to specifying 
                            "%*( \t)%*[\r\n]%*( \t)%r" or "%~%*[\r\n]%~%r".
                            We can use this to go onto the next Prospero
                            command in a Prospero protocol packet.
                            
  %~            none        This conversion is automatically suppressed.
                            Equivalent to "%*( \t)".  Used to skip over
                            optional leading or trailing horizontal whitespace.
                            Ignores field width and buffer size specifiers.

  %c            char *      field_width characters are placed in the position
                            pointed to by the char *.  Default is 1, if no
                            field-width is specified.  (All of the other
                            constructs have an infinite default field-width.) 

  %[ ... ]      char *      Matches the longest non-empty string of input
                            characters from the set between brackets.  A '\0'
                            is added.  [] ... ] includes ']' in the set.  

  %[^ ... ]     char *      Matches the longest non-empty string of input
                            characters not from the set within brackets.  A
                            '\0' is added.  [^] ... ] behaves as expected.
                            It will *include* whitespace in the set of
                            acceptable input characters, unless you explicitly
                            exclude whitespace.

                            Note that %s is equivalent to %[^ \t\n\r\v\f]

                            Also, note that %'[^/ \t] will match a string of
                            characters up to the first *unquoted* slash.
                            In Prospero, will use this construct to disassemble
                            multi-component user-level filenames, which may
                            have components with (quoted) slashes.

  %( ... )      char *      Works just like %[, except that it accepts
  %(^ ... )                 zero-length matches.  
                            The construct "%*( \t)" is useful for skipping over
                            zero or more whitespace characters at the start of
                            an input line.
                            
  %%            none        Literally matches a '%'.  Does not increment the
                            counter of matches.

  Note that there is no way to match a '\0' (terminating NUL) in the input
  string, except with "%r". (And a good thing, too.)


*/

/*
  Implementation notes:

  This routine was written to be fast above all else; it is used to
  parse Prospero commands.  Therefore, there's a lot of in-line code and
  macros that I would have put into a separate function if I'd been working
  under different constraints.

  I also do a small amount of loop unrolling to avoid excessive tests. 

  I really must apologize to the reader; I find this stuff pretty thick to get
  through.  I'm sorry.  The charset implementation is not great, and could
  be made more efficient.  We could also pre-build the charset for '%s', and
  save about 300 instructions per use of %s.  That would be a good quick
  speedup. 

  The GNU C implementation of readc() and incc() is reasonably efficient, but
  the easiest translation to a non-GNU C system involves making the inline
  functions into static functions, and all variables they reference into file
  static variables.  Totally gross, and much less efficient to boot.  

  One may #define NDEBUG to remove some internal consistency checking code and
  to remove code that checks for malformed format strings.
*/

/*
   ** Maintainer **
   Yes, we are actually maintaining qsprintf() and qsscanf() as part of the
   Prospero project.  We genuinely want your bug reports, bug fixes, and
   complaints.  We figure that improving qsprintf()'s portability will help us
   make all of the Prospero software more portable.  Send complaints and
   comments and questions to bug-prospero@isi.edu.
*/

#if __GNUC__ && 0
#define NESTED_FUNCTIONS        /*  Use nested functions.  (Currently buggy, in
                                  my version of GCC, so I turned them off.) */ 
#endif
#ifdef NESTED_FUNCTIONS

#endif



#define NUMERIC_OVERFLOW (-1)   /* Return value from qsscanf() */
#define BUFFER_OVERFLOW (-2)    /* Return value from qsscanf() */

#include <pfs_threads.h>        /* for definitions of thread stuff, below. */
#include <pfs.h>                /* For definition of ZERO, for charset.h. */
#include <pparse.h>             /* for definition of INPUT & prototype for
                                   vqscanf(). */ 
#include "charset.h"            /* character set stuff; shared with qsprintf()
                                   */  
#include <ctype.h>


/* The next macro only works on ASCII systems.  It also involves a subtraction
   operation, which is likely to be as efficient as a table lookup, so I won't
   rewrite it. */

#ifdef __GNUC__
static inline int chartoi(char c)
{
    return c - '0';
}
#else
#define chartoi(c)  ((c) - '0')
#endif

/* being in a quotation is an automatic match, since we can't break up
   quotations across fields, EXCEPT that EOF is always a failure to match
   (otherwise we might run off the end of the string.) */
/* This uses its argument twice, but that's OK. */
#define match(cs, c)    ((c) != EOF && (am_quoting || in_charset(cs,(c))))

static int inpsetspn(charset cs, INPUT in);
static int qinpsetspn(charset cs, INPUT in, int am_quoting);

int
vqscanf(INPUT in, const char *fmt, va_list ap)
/* in: source
   fmt: format describing what to scan for.
   remaining args: pointers to places to store the data we read, or 
      integers (field widths).
*/
{
    INPUT_ST    newin_st;
    int nmatches = 0;           /* no assignment-producing directives matched
                                   so far! */

    if ((in->flags & CONSUME_INPUT) == 0) {
        input_copy(in, &newin_st);
        in = &newin_st;
    }
    for (;;) {              /* check current format character */
        /* Each case in this switch statement is responsible for leaving fmt
           and s pointing to the next format and input characters to process.
        */ 
        switch (*fmt) {
        case ' ':
        case '\t':
            if (in_readc(in) != ' ' && in_readc(in) != '\t')
                goto done;      /* must match at least 1 space; failure
                                   otherwise  */
            do { /* eat up any remaining spaces in the input */
                in_incc(in);
            } while (in_readc(in) == ' ' || in_readc(in) == '\t');
            /* Eat up any remaining spaces in the format string and leave it
               poised at the next formatting character */
            while (*++fmt == ' ' || *fmt == '\t')
                ;
            break;
        case '%':
            /* This is the big long part! Handle the conversion specifiers. */
        {
            int use_long = 0;   /* Use long instead of int?  */
            int quote = 0;      /* quoting modifier? */
            int suppress = 0;   /* suppression modifier? */
            int maxfieldwidth = 0; /* max field width specified?  0 would be
                                      meaningless; if the user does specify 0,
                                      that would be strange, and I don't know
                                      what it would mean.  It will be ignored.
                                      */
            /* outbuf_size could be set to this too: */
#define READ_FROM_NEXT_ARGUMENT (-1)            
            size_t outbuf_size = 0; /* output buffer size */
            int seen_underscore = 0; /* How many underscores have we seen? */
            int seen_bang = 0;  /* How many bangs ('!') have we seen? */
            int seen_dollar = 0;  /* How many dollar signs ('$') have we seen?
                                     */ 
            int seen_ampersand = 0; /* How many ampersands ('&') have we 
                                       seen? */
            int am_quoting = 0; /* Are we actively in the middle of a
                                     quotation?  */

#ifdef NESTED_FUNCTIONS
            /* read a character from the input stream, handling quoting if it's
               turned on.  Do not advance the input stream, except while
               parsing single quotation marks.  This has the effect that
               multiple calls to readc() without an intervening incc() will
               return the same value. */ 
            /* this code has actually been tested in its current configuration.
             */
            inline int readc(void) {
                if (!quote)
                    return in_readc(in);
            redo:
                if (in_readc(in) != '\'')
                    return in_readc(in);
                /* in_readc(in) == '\'' */
                if (!am_quoting) {
                    ++am_quoting;
                    in_incc(in);
                    goto redo;
                }
                /* We *are* quoting, & just saw a '\'' */
                if (in_readcahead(in, 1) == '\'') /* examine next char. */
                    return '\''; /* do NOT increment s, since we might call
                                    readc() again. */
                else {
                    am_quoting = 0; /* didn't get two successive 's */
                    in_incc(in);
                    return in_readc(in); /* quoting gone; return real char.  */
                }
            }

            /* We're finished with this non-NUL ('\0') character; increment */
            /* This function will work even if readc() was never called;
               thus, "incc(), incc();" will do the right thing. */
            inline void incc(void) {
                assert(in_readc(in) != '\0'); /* inappropriate call to incc()!
                                                 */ 
                if (!quote)
                    in_incc(in); /* the easy case :) */
                else {
                redo:
                    if (in_readc(in) == '\'') {
                        if (!am_quoting) {
                            ++am_quoting;
                            in_incc(in);
                            goto redo;
                        }
                        /* We're in a quotation (am_quoting == 1) && we're on
                           top of a quotation mark.  Still need to increment
                           past the 'real' character. */ 
                        assert(am_quoting);
                        if (in_readcahead(in,1) == '\'') {
                            in_incc(in);
                            in_incc(in);
                            /* When inside a quotation, the two-character
                               sequence '' is treated as one character */
                        } else { 
                            /* We're on top of a quotation mark that must close
                               the current quoted section.  Go past it. */
                            in_incc(in);
                            am_quoting = !am_quoting;
                            /* Now (just changed this; hope I'm right)
                               step on.  The character we're stepping over is
                               NOT a quotation mark. */
                            in_incc(in);
                        }
                    } else { /* Quoting enabled, but we're not on a single
                                quote.  Just increment. */
                        in_incc(in);
                    }
                } /* if (!quote) */
            }
#ifndef NDEBUG
#define check_null(fmtc) if ((fmtc) == '\0') \
internal_error("improperly specified character set given as qsscanf() format")
#else
/* Assume format strings are always properly formed.   This is reasonable
   behavior, since it's a programming error to submit a malformed format
   string. */
#define check_null(fmtc)
#endif
#define build_charset(cs, endc) _build_charset(&(cs), (endc))

            inline void _build_charset(charset *csp, char endc) {
                
                int negation;
                if (*++fmt == '^') {
                    negation = 1;
                    new_full_charset(*csp);
                    remove_char(*csp, *++fmt);
                } else {
                    negation = 0;
                    new_empty_charset(*csp);
                    add_char(*csp, *fmt);
                }
                check_null(*fmt);
                while (*++fmt != endc) {
                    check_null(*fmt);
                    if (negation)
                        remove_char(*csp, *fmt);
                    else
                        add_char(*csp, *fmt);
                }
                /* fmt now points to closing bracket or paren.  Done! */
            } 

#undef check_null
#else
#define readc() _readc(in, quote, &am_quoting)
#define incc() _incc(in, quote, &am_quoting)
#define build_charset(cs, endc) _build_charset(&(cs), (endc), &fmt)            

            /* Names changed from in to _in and from quote to _quote in order
               to avoid getting bogus GCC warnings with -Wshadow under
               gcc version 2.5.8:
lib/pfs/vqscanf.c:470: warning: declaration of `in' shadows a parameter
lib/pfs/vqscanf.c:470: warning: declaration of `quote' shadows previous local 
            */

            static int _readc(INPUT _in, int _quote, int *am_quotingp);
            static void _incc(INPUT _in, int _quote, int *am_quotingp);
/* This version generates the warning: */
/* lib/pfs/vqscanf.c:471: warning: declaration of `in' shadows a parameter */
/* lib/pfs/vqscanf.c:471: warning: declaration of `quote' shadows previous local */
/*            static void _incc(INPUT in, int quote, int *am_quotingp); */
            static void _build_charset(charset *csp, char endc, const char **fmtp);

#endif /* NESTED_FUNCTIONS */

        more:
            switch(*++fmt) {
                /* Process the modifiers (options) */
            case '\'':
                ++quote;
                goto more;
            case '_':
#ifndef NDEBUG
                if (seen_bang)
                    internal_error("qsscanf(): can't use ! and _ modifiers \
together");
                if (seen_dollar)
                    internal_error("qsscanf(): can't use $ and _ modifiers \
together");
                if (seen_underscore > 1)
                    internal_error("qsscanf(): can't use > 2 underscore\
  modifiers together.");
#endif
                if (seen_underscore++)
                    outbuf_size = READ_FROM_NEXT_ARGUMENT;
                goto more;

            case '!':
#ifndef NDEBUG
                if (seen_underscore)
                    internal_error("qsscanf(): can't use ! and _ modifiers \
together");
                if (seen_dollar)
                    internal_error("qsscanf(): can't use $ and ! modifiers \
together");
                if (seen_bang > 1)
                    internal_error("qsscanf(): can't use > 2 bang\
  modifiers together.");
#endif
                if (seen_bang++)
                    outbuf_size = READ_FROM_NEXT_ARGUMENT;
                goto more;
            case '$':
#ifndef NDEBUG
                if (seen_underscore)
                    internal_error("qsscanf(): can't use $ and _ modifiers \
together");
                if (seen_bang)
                    internal_error("qsscanf(): can't use $ and ! modifiers \
together");
                if (seen_dollar > 1)
                    internal_error("qsscanf(): can't use > 2 dollar sign\
  modifiers together.");
#endif
                if (seen_dollar++)
                    outbuf_size = READ_FROM_NEXT_ARGUMENT;
                goto more;
            case '&':
                seen_ampersand++;
                goto more;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (seen_underscore == 1 || seen_bang == 1 || seen_dollar == 1)
                    outbuf_size = outbuf_size * 10 + chartoi(*fmt);
                else
                    maxfieldwidth = maxfieldwidth * 10 + chartoi(*fmt);
                goto more;
            case 'l':
                ++use_long;
                goto more;
            case '*':
                ++suppress;
                goto more;


                /* commands */
            case '%':           /* literal match */
                if (seen_ampersand)
                    internal_error("& modifier cannot be combined \
with %% conversion.");
                if (in_readc(in) != '%')
                    goto done;
                in_incc(in);
                break;
            case '~':
            {
                int r;
                if (seen_ampersand)
                    internal_error("& modifier cannot be combined \
with %~ conversion.");
                /* Match zero or more whitespace characters. */
                while ((r = readc()) == ' ' || r == '\t')
                    incc();
            }
                break;
            case 'R':
            {
                register int r;
                if (seen_ampersand)
                    internal_error("& modifier cannot be combined \
with %R conversion.");
                /* strip trailing whitespace from previous line. */
                while ((r = readc()) == ' ' || r == '\t')
                    incc();
                /* skip the newline character.  Be generous and accept \r. 
                 Be even more generous and accept blank lines. */
                if (r != '\n' && r != '\r')
                    goto done;
                do {
                    incc();
                } while ((r = readc()) == '\n' && r == '\r');
                /* Skip leading whitespace from next line. */
                /* This could be rewritten to eliminate a call to readc(). */
                while ((r = readc()) == ' ' || r == '\t')
                    incc();
            }
                /* DELIBERATE FALLTHROUGH */
            case 'r':           /* ptr. to rest of string */
                if (seen_ampersand)
                    internal_error("& modifier cannot be combined \
with %r conversion.");
                if (!suppress) { /* suppression is really stupid in this case,
                                    but we'll support stupidity. */
                    if (in->flags & PERCENT_R_TARGET_IS_STRING) {
                        /* cast to char * to throw away CONST */
                        *va_arg(ap, char **) = (char *) in->s;
                    } else {
                        INPUT inparg = va_arg(ap, INPUT);
                        input_copy(in, inparg);
                    }
                    ++nmatches;
                }
                break;
            case 'c':           /* char */
            {
                char *out = 0; /* assignment quiets gcc -Wall */
                register int r;

                if (seen_ampersand)
                    internal_error("& modifier cannot be combined \
with %c conversion.");
                if (!suppress)
                    out = va_arg(ap, char *);
                if (outbuf_size == READ_FROM_NEXT_ARGUMENT)
                    outbuf_size = va_arg(ap,int);
                else if (outbuf_size == 0)
                    outbuf_size = -1; /* ignore outbuf size by default */
                if (maxfieldwidth < 1)
                    maxfieldwidth = 1; /* Read 1 character by default. */
                if ((r = readc()) == '\0')
                    goto done;  /* must match at least 1 character. */
                do {
                    if (outbuf_size-- == 0) {
                        if (seen_dollar)
                            goto done;
                        else if (seen_bang)
                            return BUFFER_OVERFLOW;
                        else
                            break;
                    }
                    if (!suppress)
                        *out++ = r;
                    incc();
                } while (--maxfieldwidth && (r = readc()) != '\0');
                if (!suppress)
                    ++nmatches;
            }
                break;
            case 'd':
                if (seen_ampersand)
                    internal_error("& modifier cannot be combined \
with %d conversion.");
            if (use_long) {
                long d = 0;      /* decimal # we're generating. */
                /* Use these 2 definitions to check for overflow. */
                const long div = LONG_MAX / 10;
                const long mod = LONG_MAX % 10;


                /* Save the last return from readc() for reuse.  We must use an
                   explicit temporary variable because the compiler won't know
                   that readc() always returns the same value without an
                   intervening incc(). */
                register int r;

                int negative;   /* non-zero if negative #. */

                if ((r = readc()) == '-')
                    negative = -1, incc(), r = readc();
                else
                    negative = 0;
                if (!isdigit(r))
                    goto done;
                do {
                    register i = chartoi(r);

                    if (d > div || (d == div && i > mod)) {
                        /* Integer overflow! */
                        if (seen_dollar)
                            goto done; /* failure to match */
                        else if (seen_underscore)
                            break; /* conversion done */
                        else
                            return -1; /* abort */
                    }
                    d = d * 10 + i;
                    incc();
                } while (isdigit(r = readc()));
                /* s points to the next non-digit now. */
                if (!suppress) {
                    *va_arg(ap, long *) = (negative ? -d : d);
                    ++nmatches;
                }
            } else {
                int d = 0;      /* decimal # we're generating. */
                /* Use these 2 definitions to check for overflow. */
                const int div = INT_MAX / 10;
                const int mod = INT_MAX % 10;


                /* Save the last return from readc() for reuse.  We must use an
                   explicit temporary variable because the compiler won't know
                   that readc() always return the same value without an
                   intervening incc(). */
                register int r;

                int negative;   /* non-zero if negative #. */

                if ((r = readc()) == '-')
                    negative = -1, incc(), r = readc();
                else
                    negative = 0;
                if (!isdigit(r))
                    goto done;
                do {
                    register i = chartoi(r);

                    if (d > div || (d == div && i > mod)) {
                        /* Integer overflow! */
                        if (seen_bang)
                            goto done; /* failure to match */
                        else if (seen_underscore)
                            break; /* conversion done */
                        else
                            return -1; /* abort */
                    }
                    d = d * 10 + i;
                    incc();
                } while (isdigit(r = readc()));
                /* s points to the next non-digit now. */
                if (!suppress) {
                    *va_arg(ap, int *) = (negative ? -d : d);
                    ++nmatches;
                }
            }

                break;
                
            case '(':
            {
                char *out = 0; /* assignment quiets gcc -Wall */      /* output buffer */
                register int r;
                charset cs;

                if (seen_ampersand)
                    internal_error("& modifier cannot currently be combined \
with %( conversion.");
                if (!maxfieldwidth)
                    maxfieldwidth = -1;
                if (!suppress)
                    out = va_arg(ap, char *);
                if (outbuf_size == READ_FROM_NEXT_ARGUMENT)
                    outbuf_size = va_arg(ap,int);
                build_charset(cs, ')');

                /* don't have to match any characters. */
                for (; maxfieldwidth-- && match(cs, r = readc()); incc()) {
                    if (--outbuf_size == 0) {
                        if (seen_dollar)
                            goto done;
                        else if (seen_bang)
                            return BUFFER_OVERFLOW;
                        else
                            break;
                    }
                    if (!suppress)
                        *out++ = r;
                }
                /* if we stopped while still quoting, there are problems! */
                if (am_quoting)
                    goto done;
                if (!suppress)
                    *out = '\0', ++nmatches;
            }
                break;
            case '[':
            {
                char *out = 0; /* assignment quiets gcc -Wall */      /* output buffer */
                char **outp = 0; /* assignment quiets gcc -Wall */     /* Pointer to place to stash output. */
                register int r;
                charset cs;

                if (suppress) {
                    seen_ampersand = 0; /* ignore the seen_ampersand flag */
                } else {
                    if (seen_ampersand) {
                        outp = va_arg(ap, char **);
                    } else {
                        out = va_arg(ap, char *);
                    }
                }
                if (outbuf_size == READ_FROM_NEXT_ARGUMENT)
                    outbuf_size = va_arg(ap,int);
                if ((suppress || outbuf_size) && seen_ampersand)
                    internal_error("qsscanf(): Specifying an output buffer \
size or suppression  and the ampersand conversion together is ridiculous. You \
don't know what you're doing.");
                build_charset(cs, ']');

                /* must match at least 1 character if not quoting.  If quoting,
                   we might see the (quoted) null string. */
                /* Treat the quoted null string (also, by the way, a 
                   common case) as a special case. */ 
                if (quote && in_readc(in) == '\'' 
                    && in_readcahead(in, 1) == '\'' 
                    && in_readcahead(in, 2) != '\''
                    && !match(cs, in_readcahead(in, 2))) {
                    /* quoted null string. */
                    assert(in_readcahead(in,2) == readc());
                    if (!suppress) {
                        ++nmatches;
                        if (seen_ampersand)
                            *outp = stcopyr("", *outp);
                        else
                            *out = '\0';
                    }
                    break;
                }
                if (!match(cs, r = readc()))
                    goto done;      /* did not match any characters */
                if (seen_ampersand) {
                    /* Set out to start of string; we can increment out, but
                       should leave *outp always at the start of the string. */
                    if ((out = *outp) == NULL) { 
                        /* passing null pointer to strcpy is legal. */
                        outbuf_size = 1 + (quote ? 
                                           qinpsetspn(cs, in, am_quoting) 
                                           : inpsetspn(cs, in));
                        if (outbuf_size <= 1) /* unbalanced quoting or failed
                                                 to match any characters. */
                            goto done;
                        out = *outp = stalloc(outbuf_size);
#ifndef NDEBUG
                        /* should have allocated exactly enough memory for this
                           conversion. */ 
                        seen_ampersand = -1; 
#endif
                    } else {
                        /* stalloc() guarantees not to honor requests for 0 or
                           fewer bytes of memory, so outbuf_size > 0. */
                        outbuf_size = p__bstsize(out);
                        assert(outbuf_size > 0);
                    }
                }
                do {
                    if (--outbuf_size == 0) {
                        if (seen_ampersand) {
                            int oldsize = p__bstsize(*outp);
                            char *oldstart = *outp;

#ifdef ASSERT2                  /* expensive assertion */
                            assert(oldsize == strlen(oldstart) + 1);
#endif
#ifndef NDEBUG
                            /* We should never run through this code twice for
                               the same conversion. */
                            assert(seen_ampersand > 0);
                            seen_ampersand = -1;
#endif
                            /* Don't need room for the trailing null, since
                               the current outbuf_size allocated space for it.
                               */ 
                            outbuf_size = quote ? 
                                qinpsetspn(cs, in, am_quoting) : 
                                    inpsetspn(cs, in);
                            if (outbuf_size < 1) /* unbalanced quoting */
                                goto done;
                            
                            out = *outp = stalloc(outbuf_size + oldsize);
                            strcpy(out, oldstart);
                            out += oldsize - 1;
                        } else if (seen_dollar)
                            goto done;
                        else if (seen_bang)
                            return BUFFER_OVERFLOW;
                        else {
                            break;
                        }
                    }
                    if (!suppress) {
                        *out++ = r;
                    }
                    incc();
                } while (--maxfieldwidth && match(cs, r = readc()));
                /* if we stopped while still quoting, there are problems! */
                if (am_quoting)
                    goto done;
#ifndef NDEBUG
                if (seen_ampersand == -1) /* if we allocated just enough memory
                                             to fit */
                    assert(*outp + p__bstsize(*outp) - 1 == out);
#endif
                if (!suppress) {
                    *out = '\0';
                    ++nmatches;
                }
            }
                break;

            case 'b':           /* just like %s, except '\0' is OK */
            case 's':           /* Just like %b, except that encountering '\0'
                                   in a string (quoted or not) is a failure to
                                   match.  */
                /* This distinction needs to be implemented and isn't. */
            {
                static charset nw_cs; /* not whitespace cs */
                static int nw_cs_initialized = 0; 
                register int r;
                char *out = 0; /* assignment quiets gcc -Wall */      /* output buffer */
                char **outp = 0; /* assignment quiets gcc -Wall */     /* Pointer to place to stash output. */
                int expected_inputlen;


                /* Have we already built a charset for %s?  If not, build it
                   now and flag it as having been initialized. */
                if (!nw_cs_initialized) {
                    p_th_mutex_lock(p_th_mutexPFS_VQSCANF_NW_CS);
                    new_full_charset(nw_cs);
                    remove_char(nw_cs, '\n');
                    remove_char(nw_cs, ' ');
                    remove_char(nw_cs, '\t');
                    remove_char(nw_cs, '\r');
                    remove_char(nw_cs, '\v');
                    remove_char(nw_cs, '\f');

                    ++nw_cs_initialized;
                    p_th_mutex_unlock(p_th_mutexPFS_VQSCANF_NW_CS);
                }
                if (suppress) {
                    seen_ampersand = 0; /* ignore the seen_ampersand flag */
                } else {
                    if (seen_ampersand) {
                        outp = va_arg(ap, char **);
                    } else {
                        out = va_arg(ap, char *);
                    }
                }
                if (outbuf_size == READ_FROM_NEXT_ARGUMENT)
                    outbuf_size = va_arg(ap,int);
                if ((suppress || outbuf_size) && seen_ampersand)
                    internal_error("qsscanf(): Specifying an output buffer \
size or suppression  and the ampersand conversion together is ridiculous. You \
don't know what you're doing.");
                /* must match at least 1 character if not quoting.  If quoting,
                   we might see the (quoted) null string. */
                /* Treat the quoted null string (also, by the way, a 
                   common case) as a special case. */ 
                if (quote && in_readc(in) == '\'' 
                    && in_readcahead(in,1) == '\'' 
                    && in_readcahead(in,2) != '\''
                    && !match(nw_cs, in_readcahead(in,2))) {
                    /* quoted null string. */
                    assert(in_readcahead(in,2) == readc());
                    if (!suppress) {
                        ++nmatches;
                        if (seen_ampersand)
                            *outp = stcopyr("", *outp);
                        else
                            *out = '\0';
                    }
                    break;
                }
                if (!match(nw_cs, r = readc()))
                    goto done;      /* did not match any characters */
                expected_inputlen = quote ? 
                    qinpsetspn(nw_cs, in, am_quoting) 
                        : inpsetspn(nw_cs, in);
                if (expected_inputlen <= 0) /* unbalanced quoting or failed
                                                 to match any characters. */
                    goto done;
                if (seen_ampersand) {
                    /* Set out to start of string; we can increment out, but
                       should leave *outp always at the start of the string. */
                    /* Stick on the +1 for trailing NUL for safety. */
                    if (expected_inputlen + 1 > p__bstsize(*outp)) {
                        stfree(*outp); /* might be NULL.  Ok if it is. */
                        /* passing null pointer to strcpy is legal. */
                        outbuf_size = 1 + expected_inputlen;
                        out = *outp = stalloc(outbuf_size);
#ifndef NDEBUG
                        /* should have allocated exactly enough memory for this
                           conversion. */ 
                        seen_ampersand = -1; 
#endif
                    } else {
                        outbuf_size = p__bstsize(out = *outp);
                        /* This assertion is ok, since we tested above. */
                        assert(outbuf_size > 0);
                    }
                }
                do {
                    if (--outbuf_size == 0) {
                        if (seen_ampersand) {
                            internal_error("somehow failed to allocate enough \
memory for the output in a %s or %b conversion.");
                        } else if (seen_dollar) {
                            goto done;
                        } else if (seen_bang) {
                            return BUFFER_OVERFLOW;
                        } else {
                            break;
                        }
                    }
                    if (!suppress)
                        *out++ = r;
                    incc();
                } while (--maxfieldwidth && match(nw_cs, r = readc()));
                /* if we stopped while still quoting, there are problems! */
                if (am_quoting)
                    goto done;
#ifndef NDEBUG
                if (seen_ampersand == -1) /* if we allocated just enough memory
                                             to fit */
                    assert(*outp + p__bstsize(*outp) - 1 == out);
#endif
                if (seen_ampersand)
                    p_bst_set_buffer_length_nullterm(*outp, out - *outp);
                if (!suppress) {
                    *out = '\0';
                    ++nmatches;
                }
            }
                break;
            case 'S':
            {
                register int r;
                char *out = 0; /* assignment quiets gcc -Wall */      /* output buffer */
                char **outp = 0; /* assignment quiets gcc -Wall */     /* Pointer to place to stash output. */

                if (suppress) {
                    seen_ampersand = 0; /* ignore the seen_ampersand flag */
                } else {
                    if (seen_ampersand) {
                        outp = va_arg(ap, char **);
                    } else {
                        out = va_arg(ap, char *);
                    }
                }
                if (seen_ampersand)
                    internal_error("& modifier cannot currently be combined \
with %S conversion.");
                if (outbuf_size == READ_FROM_NEXT_ARGUMENT)
                    outbuf_size = va_arg(ap,int);
                if ((suppress || outbuf_size) && seen_ampersand)
                    internal_error("qsscanf(): Specifying an output buffer \
size or suppression  and the ampersand conversion together is ridiculous. You \
don't know what you're doing.");

                while (--maxfieldwidth && (r = readc())) {
                    if (--outbuf_size == 0) {
                        if (seen_dollar)
                            goto done;
                        else if (seen_bang)
                            return BUFFER_OVERFLOW;
                        else
                            break;
                    }
                    if (!suppress) {
                        *out++ = r;
                    }
                    incc();
                }
                /* if we stopped while still quoting, there are problems! */
                if (am_quoting)
                    goto done;
                if (!suppress)
                    *out = '\0', ++nmatches;
            }
                break;

#ifndef NDEBUG
            default:
                internal_error("malformed format string passed to qsscanf()");
                /* NOTREACHED */
#endif
            }
            ++fmt;
        }                       /* end of case '%' */
            break;


        case '\0':              /* no more format specifiers to match. */
            goto done;
        default:                /* literal character match */
            if (*fmt++ != in_readc(in))
                goto done;
            in_incc(in);
        }
    }

 done:
    return nmatches;
}




#ifndef NESTED_FUNCTIONS
static int
_readc(INPUT in, int quote, int *am_quotingp)
{
    if (!quote)
        return in_readc(in);
 redo:
    if (in_readc(in) != '\'')
        return in_readc(in);
    /* in_readc(in) == '\'' */
    if (!(*am_quotingp)) {
        ++(*am_quotingp);
        in_incc(in);
        goto redo;
    }
    /* We *are* quoting, & just saw a '\'' */
    if (in_readcahead(in,1) == '\'') /* examine next char. */
        return '\''; /* do NOT advance input stream, since we might call
                        readc() again. */
    else {
        (*am_quotingp) = 0; /* didn't get two successive 's */
        in_incc(in);
        return in_readc(in); /* quoting gone; return real char.  */
    }
}



static void
_incc(INPUT in, int quote, int *am_quotingp)
{
    assert(in_readc(in) != EOF); /* inappropriate call to incc()! */

    /* Skip single ', if it's there. */
    if (!quote)
        in_incc(in);            /* easy case :) */
    else {
    redo:
        if (in_readc(in) == '\'') {
            if (!(*am_quotingp)) {
                ++(*am_quotingp);
                in_incc(in);
                goto redo;
            }
            /* We're in a quotation (am_quoting == 1) && we're on
               top of a quotation mark.  Still need to increment
               past the 'real' character. */ 
            assert(*am_quotingp);
            if (in_readcahead(in,1) == '\'') {
                in_incc(in);
                in_incc(in);
                /* When inside a quotation, the two-character
                   sequence '' is treated as one character */
            } else { 
                /* We're on top of a quotation mark that must close
                   the current quoted section.  Go past it. */
                in_incc(in);
                (*am_quotingp) = !(*am_quotingp);
                /* Now (just changed this; hope I'm right) step on.  The
                   character we're stepping over is NOT a quotation mark. */
                in_incc(in);
            }
        } else { /* Quoting enabled, but we're not on a single
                    quote.  Just increment. */
            in_incc(in);
        }
    } /* if (!quote) */
}


/* inpsetspn() returns the length of the initial segment of s whose 
   characters are in set cs.  This relies on the trick that '\0' will never be
   in a character set. */
static int
inpsetspn(charset cs, INPUT oldin)
{
    int retval = 0;
    INPUT_ST in_st;
    INPUT in = &in_st;

    input_copy(oldin, in);
    while (in_charset(cs, in_readc(in))) {
        ++retval;
        in_incc(in);
    }
    return retval;
}

/* qinpsetspn() returns the length of the initial segment of s whose unquoted
   characters are in set cs.  -1 is returned if the quoting is ill-formed. */
static int
qinpsetspn(charset cs, INPUT oldin, int am_quoting)
{
    /* code swiped from qindex() */
    int count = 0;

    enum { OUTSIDE_QUOTATION, IN_QUOTATION, 
               SEEN_POSSIBLE_CLOSING_QUOTE } state; 
    INPUT_ST in_st;
    INPUT in = &in_st;
    input_copy(oldin, in);
    
    state = am_quoting? IN_QUOTATION : OUTSIDE_QUOTATION;
    
    for (; !in_eof(in); in_incc(in)) {
        switch (state) {
        case OUTSIDE_QUOTATION:
            if (in_readc(in) == '\'')
                state = IN_QUOTATION;
            else if (in_charset(cs, in_readc(in))) {
                ++count;
            } else  {
                return count;   /* failure to match. */
            }
            break;
        case IN_QUOTATION:
            if (in_readc(in) == '\'')
                state = SEEN_POSSIBLE_CLOSING_QUOTE;
            else
                ++count;
            break;
        case SEEN_POSSIBLE_CLOSING_QUOTE:
            if (in_readc(in) == '\'') {
                ++count;
                state = IN_QUOTATION;
            } else {
                state = OUTSIDE_QUOTATION;
                if (!in_charset(cs, in_readc(in)))
                    return count;
                ++count;
            }
            break;
        default:
            internal_error("inpsetspn(): impossible state!");
        }
    }
    if (state == IN_QUOTATION)  /* unbalanced quoting */
        return -1;
    return count;
}


#ifndef NDEBUG
#define check_null(fmtc) if ((fmtc) == '\0') \
internal_error("improperly specified character set given as qsscanf() format")
#else
/* Assume format strings are always properly formed.   This is reasonable
behavior, since it's a programming error to submit a malformed format
string. */
#define check_null(fmtc)
#endif
static void 
_build_charset(charset *csp, char endc, const char **fmtp)
{
    int negation;
    if (*++(*fmtp) == '^') {
        negation = 1;
        new_full_charset(*csp);
#if 0
        remove_char(*csp, '\0'); /* don't ever want to match \0 as being
                                    valid. */ 
#endif
        remove_char(*csp, *++(*fmtp));
    } else {
        negation = 0;
        new_empty_charset(*csp);
        add_char(*csp, *(*fmtp));
    }
    check_null(*(*fmtp));
    while (*++(*fmtp) != endc) {
        check_null(*(*fmtp));
        if (negation)
            remove_char(*csp, *(*fmtp));
        else
            add_char(*csp, *(*fmtp));
    }
    /* (*fmtp) now points to closing bracket or paren.  Done! */
} 
#undef check_null

#endif
 
