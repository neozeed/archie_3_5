/* These definitions are destined for pmachine.h, they are herer to 
    prevent LONG recompile's and can be moved when stable */
/*
 * This file determines how we handle various approaches to multithreading
 * or timing out on TCP opens.
 * The appropriate approaches for the single-threaded and multi-threaded
 * cases are different. 
 */

/* Several different approaches to handling timeouts

1. Do nothing - (old prospero) - problems occur when prospero is used
   mostly for gatewaying since connect could take 2 mins to time out,
   leaving hte gateway program to wait - or even worse, timeout itself.
2. Use SIGALRM - this works on SCOUNIX, and probably on most others
   it does NOT work on SOLARIS2.3 mutli-threaded, probably due to 
   problems in the call to signal (at least that is where it crashes).
3. Non-blocking, works fast, since it doesnt need to wait for connects,
   but this is probably not relevant on gateways. It still has the problems
   of (1). SWA reports problems with this in opentcp.c
4. Non-blocking and waiting, run non-blocking, but wait (using select) at
   crucial points. - Currently used by Mitra for AOL
*/

/* For Approach 1 - undef all of these */
#undef TIMEOUT_APPROACH            /*Approach 2*/
#undef NONBLOCKING_APPROACH       /*Approach 3*/
#define SELECT_APPROACH            /*Approach 4*/


/* SETSOCKOPTS are generally usefull, but they will fail under SOLARIS 2.3
setting Errno to the ambiguous "0" */
#ifndef SOLARIS
#define SETSOCKOPTS	/* Set various socket options */
#endif

/* This should go in pfs.h */
extern wait_till_readable(int fd, int timeout);

/* Open a stream (or start an opening of a stream).  Returns a filedescriptor
 * or -1.
 */
extern int p_open_tcp_stream(const char host[], int port);
