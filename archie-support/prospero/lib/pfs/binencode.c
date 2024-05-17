/* binencode.c
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
  binencode() takes a char* (INBUF) and a size count (INBUFSIZE) as its first
  two arguments.  INBUF is assumed to be an array of INBUFSIZE chars containing
  binary data.  binencode() converts the contents of INBUF to a
  printing-character representation, and saves it in OUTBUF.  OUTBUFSIZE is the
  size of OUTBUF.

  binencode() returns the # of characters it wrote to the string, or the # of
  characters it would have written if there had been enough room.  Its return
  value INCLUDES the NUL ('\0').  Binencode() takes the SIZE of the buffer it
  writes to as an argument.  This allows us to guard against buffer overflow.
  binencode() will write partial strings to buffers which are not long enough.
  This seems to be reasonable behavior to me.
  To check for error returns, all is OK if binencode's return value is less than
  OUTBUFSIZE.  Otherwise, you need to either allocate a bigger outbuf, or give
  up.  (This is consistent with the return values for qsprintf() and
  bindecode().) 

  This string can be converted back with bindecode().

  This string does NOT need to be quoted using qsprintf and qsscanf;  it
  escapes the NUL, backslash, newline, and horizontal whitespace.
*/

#ifdef __STDC__
#include <stddef.h>

int
binencode(char *inbuf, size_t inbufsize, char *outbuf, size_t outbufsize)
#else
int
binencode(inbuf, inbufsize, outbuf, outbufsize)
char *inbuf, *outbuf;
int inbufsize, outbufsize;
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

    for (; inbufsize > 0; ++inbuf, --inbufsize) {
        if (*inbuf == '\0') {
            rawput('\\');
            rawput('0');
        } else if (*inbuf == '\\') {
            rawput('\\');
            rawput('\\');
        } else if (*inbuf == ' ') {
            rawput('\\');
            rawput('s');
        } else if (*inbuf == '\t') {
            rawput('\\');
            rawput('t');
        } else if (*inbuf == '\v') {
            rawput('\\');
            rawput('v');
        } else if (*inbuf == '\r') {
            rawput('\\');
            rawput('r');
        } else if (*inbuf == '\f') {
            rawput('\\');
            rawput('f');
        } else if (*inbuf == '\n') {
            rawput('\\');
            rawput('n');
        } else {
            rawput(*inbuf);
        }
    }
    rawput('\0');

    return outcount;
}
