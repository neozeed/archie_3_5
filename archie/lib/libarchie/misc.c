#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <pwd.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>

#include "db_files.h"
#include "typedef.h"
#include "error.h"
#include "misc.h"



int strfind( txt, tlen )
   char *txt ;
   int tlen ;
{
   int pc ;
   int skip ;
   int tc ;
   unsigned char tchar ;


   if( tlen < plen )
      return 0 ;

   pc = tc = plen_1 ;

   do
   {
      tchar = *(txt + tc) ;
      if( nocase )
      {
	 if( isascii( (int)tchar ) && isupper( (int)tchar ))
	    tchar = (char)tolower( (int)tchar ) ;
      }

      if( tchar == *(pat + pc) )
      {
	 --pc ; --tc ;
      }
      else
      {
	 skip = skiptab[ tchar ] ;
	 tc += (skip < plen_1 - pc) ? plen : skip ;
	 pc = plen_1 ;
      }
   }
   while( pc >= 0 && tc < tlen ) ;

   return pc < 0 ;
}


   
/*
   The following two routines implement the case-sensitive and case-
   insensitive substring search.
*/

#define TABLESIZE 256

static char *pat = (char *)0 ;
static int nocase ;
static int plen ;
static int plen_1 ;
static int skiptab[ TABLESIZE ] ;


int initskip( pattern, len, ignore_case )
   char *pattern ;
   int len ;
   int ignore_case ;
{
   int i ;


   plen = len ;
   plen_1 = plen - 1 ;

   nocase = ignore_case ;

   if( pat == (char *)0 )
   {
      if(( pat = (char *)malloc((unsigned)(plen + 1))) == (char *)0 )
      {
	 error( A_ERR, "initskip", "can't malloc %d bytes for pattern.",
	        plen + 1) ;
	 return 0 ;
      }
   }
   else
   {
      if(( pat = (char *)realloc(pat, (unsigned)(plen + 1))) == (char *)0 )
      {
	 error( A_ERR, "initskip", "can't realloc %d bytes for pattern.",
	        plen + 1) ;
	 return 0 ;
      }
   }

   memcpy( pat, pattern, plen + 1 ) ;

   if( ignore_case )
      make_lcase( pat ) ;

   for( i = 0 ; i < TABLESIZE ; i++ )
      skiptab[ i ] = plen ;

#ifdef DEBUG
   fprintf( stderr, "Default skip length is %d\n", plen ) ;
#endif

   for( i = 0 ; i < plen ; i++ )
   {
      skiptab[ *(pat + i) ] = plen - 1 - i ;

#ifdef DEBUG
      fprintf( stderr, "Skip length for '%c' is %d\n", *(pat + i),
	       plen - 1 - i ) ;
#endif
   }

   return 1 ;
}




status_t delete_hostdb_entry( key, keysize, hostdb )
   void *key;
   int keysize;
   file_info_t *hostdb;

{
   datum h_search;

   h_search.dptr = key;
   h_search.dsize = keysize;

   if(dbm_delete(hostdb -> fp_or_dbm.dbm, h_search) != 0){
      return(ERROR);
   }

   return(A_OK);
}

