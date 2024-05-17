/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.
*/

#ifndef lint
static char *RCSid = "$Header: /buzza/cvsroot/technologies/archie-support/prospero/lib/psrv/wais_gw/ustubs.c,v 1.1.1.1 1999/03/18 16:50:52 pedro Exp $";
#endif

/* Change log:
 * $Log: ustubs.c,v $
 * Revision 1.1.1.1  1999/03/18 16:50:52  pedro
 * This modules includes the prosper server and the berkeley db code used
 * in Archie
 *
 * Revision 1.1.1.1  1994/08/31  16:33:09  bajan
 * New Prospero release
 *
 * Revision 1.2  94/04/19  00:29:17  swa
 * (mitra) assert(P_IS_THIS_THREAD_MASTER()) added in places.
 * Added tests for SOLARIS
 * 
 * Revision 1.1  93/11/10  13:35:08  swa
 * Initial revision
 * 
 * Revision 1.4  92/05/06  17:35:13  jonathan
 * Modified some #if's for NeXT and Mach.
 * 
 * Revision 1.3  92/03/06  11:08:49  jonathan
 * fixed incompatible return type error in HP version of getwd.  Thanks to
 * cshotton@oac.hsc.uth.tmc.edu.
 * 
 * Revision 1.2  92/02/12  13:54:29  jonathan
 * Added "$Log" so RCS will put the log message in the header
 * 
 * 
*/

/*----------------------------------------------------------------------*/
/* stubs for silly non-ansi c compilers                                 */
/*----------------------------------------------------------------------*/

#include "ustubs.h"
#include "futil.h" /* for MAX_FILENAME_LEN */
#include "cutil.h" /* for substrcmp and NULL */
#include <pmachine.h>		/* For Solaris */
#include <stdlib.h>		/* SOLARIS: for rand and srand */
#include <pfs_threads.h> /* For P_IS_THIS_THREAD_MASTER */


/*----------------------------------------------------------------------*/

#if (defined(__svr4__) || defined(hpux) || defined(SCOUNIX)) && !defined(SOLARIS)
char*
getwd(char* pathname)
{
  return((char*)getcwd(pathname, MAX_FILENAME_LEN));
}

long 
random(void)
{ 
  assert(P_IS_THIS_THREAD_MASTER()); /*SOLARIS: rand, srand & random MT-Unsafe */
  return(rand());
}


long 
srandom(unsigned long seed)
{ 
  assert(P_IS_THIS_THREAD_MASTER()); /*SOLARIS: rand, srand & random MT-Unsafe */
  srand(seed);
  return(0);
}

long 
sigmask(long sig)
{
}

long 
sigblock(long mask)
{
}

long 
sigsetmask(long mask)
{
}

#endif
#if (defined(__svr4__) || defined(hpux))
#include <sys/systeminfo.h>

long
gethostname(char* hostname,long len)
{ 
  return(sysinfo(SI_HOSTNAME,hostname,len));
}

#endif /* def SYSV */

/*----------------------------------------------------------------------*/

#ifndef ANSI_LIKE   /* memmove is an ANSI function not defined by K&R */
#ifndef hpux /* but HP defines it */
void*
memmove(void* str1,void* str2,size_t n)
{
#ifdef M_XENIX
  memcpy((char*)str2,(char*)str1,(long)n); /* hope it works! */
#else /* ndef M_XENIX */
  bcopy((char*)str2,(char*)str1,(long)n);
#endif /* ndef M_XENIX */
  return(str1);
}
#endif /* ndef hpux */

#else /* ansi is defined */

#ifdef __GNUC__ /* we are ansi like, are we gcc */

#if !(defined(NeXT) || defined(Mach) || defined(__svr4__)) 
       /* and we are not on a next or solaris ! */

void*
memmove(void* str1,void* str2,size_t n)
{
  bcopy((char*)str2,(char*)str1,(long)n);
  return(str1);
}

#endif /* not NeXT or Mach */

#endif /* __GNUC__ */

#endif /* else ndef ANSI_LIKE */

/*----------------------------------------------------------------------*/

#ifndef ANSI_LIKE  

/* atoi is not defined k&r. copied from the book */
long 
atol(char* s)
{
  long i, n, sign;
  for(i=0; s[i]==' ' || s[i]== 'n' || s[i]=='t'; i++)
    ;				/* skip white space */
  sign = 1;
  if (s[i] == '+' || s[i] == '-')
    sign = (s[i++]=='+') ? 1 : -1;
  for (n=0; s[i] >= '0' && s[i] <= '9'; i++)
    n= 10 * n + s[i] - '0';
  return(sign * n);
}

/*----------------------------------------------------------------------*/

char*
strstr(char* src,char* sub)
{
  /* this is a poor implementation until the ANSI version catches on */
  char *ptr;
  for(ptr = src; (long)ptr <= (long)src + strlen(src) - strlen(sub); ptr++){
    if(substrcmp(ptr, sub))
      return(ptr);
  }
  return(NULL);
}

/*----------------------------------------------------------------------*/

int 
remove(char* filename)
{
  return(unlink(filename));
}

/*----------------------------------------------------------------------*/

#endif /* ndef ANSI_LIKE */




