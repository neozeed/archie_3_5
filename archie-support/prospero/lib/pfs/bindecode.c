/* bindecode.c
   Author: Steven Augart <swa@isi.edu>
   Written: 8/17/92
   I am really interested in comments on this code, suggestions for making it
   faster, and criticism of my style.  Please send polite suggestions for
   improvement to swa@isi.edu.
*/

/* Copyright (c) 1992 by the University of Southern California. */
/* For copying and distribution information, see the file <usc-copyr.h> */
#include <usc-copyr.h>

/*
  bindecode() takes a char* (INBUF) that represents a null-terminated string
  containing a NUL-less representation of binary data, produced by
  binencode().   bindecode() converts the contents of INBUF to a pure binary
  representation, and  saves it in OUTBUF.  OUTBUFSIZE is the size of OUTBUF.

  bindecode() returns the # of characters it wrote to OUTBUF, or the # of
  characters it would have written if there had been enough room.  Note that no
  terminating NUL is added to the string.    

  bindecode() will write partial strings to buffers which are not long enough.
  This seems to be reasonable behavior to me.
  To check for error returns, all is OK if bindecode's return value is less
  than OUTBUFSIZE.  Otherwise, you need to either allocate a bigger outbuf, or
  give up.  (This is consistent with the return values for qsprintf() and
  binencode().) 
*/


#ifdef __STDC__
#include <stddef.h>

int
bindecode(char *inbuf, char *outbuf, size_t outbufsize)
#else
int
bindecode(inbuf, outbuf, outbufsize)
char *inbuf, *outbuf;
int outbufsize;
#endif
{
    int outcount = 0;               /* how many characters did we output? */

#define rawput(c) do {                  \
        ++outcount;                     \
        if (outbufsize > 0)  {          \
            --outbufsize;               \
            *outbuf++ = (c);            \
        }                               \
    } while (0)                       

    for (; *inbuf; ++inbuf) {
        if (*inbuf == '\\') {
            ++inbuf;
            if (*inbuf == '0')
                rawput('\0');
            else if (*inbuf == '\\')
                rawput('\\');
            else if (*inbuf == 's')
                rawput(' ');
            else if (*inbuf == 't')
                rawput('\t');
            else if (*inbuf == 'v')
                rawput('\v');
            else if (*inbuf == 'f')
                rawput('\f');
            else if (*inbuf == 'r')
                rawput('\r');
            else if (*inbuf == 'n')
                rawput('\n');
            else
                return -1;      /* format error */
        } else {
            rawput(*inbuf);
        }
    }

    return outcount;
}
