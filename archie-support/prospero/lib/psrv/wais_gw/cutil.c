/*****************************************************************************
* 	        (c) Copyright 1992 Wide Area Information Servers, Inc        *
*	   	  of California.   All rights reserved.   	             *
*									     *
*  This notice is intended as a precaution against inadvertent publication   *
*  and does not constitute an admission or acknowledgement that publication  *
*  has occurred or constitute a waiver of confidentiality.		     *
*									     *
*  Wide Area Information Server software is the proprietary and              *
*  confidential property of Wide Area Information Servers, Inc.              *
*****************************************************************************/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#define cutil_c

#include <ctype.h>
#include <string.h>
#include <time.h>
#include <pfs.h>
#include <perrno.h>

#include "cutil.h"
#include "futil.h"

#ifdef NOTUSEDANDNOTTHREADSAFE
long myPID = -1;
#endif /*NOTUSEDANDNOTTHREADSAFE*/

typedef long (longfunc)(long c);

/*---------------------------------------------------------------------------*/

char*
s_strdup(char* s)
{
  unsigned long len;
  char* copy = NULL;
  
  if (s == NULL)		
    return(NULL);
    
  len = strlen(s);		
  copy = (char*)s_malloc((size_t)(sizeof(char)*(len + 1)));
  strncpy(copy,s,len + 1);
  return(copy);
}

/*---------------------------------------------------------------------------*/

#if !defined(IN_RMG) && !defined(PFS_THREADS)
char*
strtokf(char* s1,longfunc *isDelimiter)
{
  static char* searchStr = NULL;
  static longfunc *delimiterFunc;
  long i;
  char* startTok = NULL;

  if (s1 != NULL)		
    searchStr = s1;
    
  if (isDelimiter != NULL)
    delimiterFunc = isDelimiter;
   
  if (searchStr == NULL || searchStr[0] == '\0')
    return(NULL);		
    
  if (delimiterFunc == NULL)
    return(NULL);		
  
  for (i = 0; searchStr[i] != '\0'; i++)
   { if ((*delimiterFunc)((long)searchStr[i]) == NOT_DELIMITER)
       break;
   }
  
  if (searchStr[i] == '\0') 
    return(NULL);		
  else
    startTok = searchStr + i;	

  for (; searchStr[i] != '\0'; i++)
   { if ((*delimiterFunc)((long)searchStr[i]) == IS_DELIMITER)
       break;
   }
  
  if (searchStr[i] != '\0')
   { searchStr[i] = '\0';
     searchStr = searchStr + i + 1;
   }
  else
    searchStr = searchStr + i;
   
  return(startTok);
}

/*---------------------------------------------------------------------------*/

char*
strtokf_isalnum(char* s1)
{
  static char* searchStr = NULL;
  long i;
  char* startTok = NULL;

  if (s1 != NULL)		
    searchStr = s1;

  if (searchStr == NULL || searchStr[0] == '\0')
    return(NULL);		
  
  for (i = 0; searchStr[i] != '\0'; i++)
   { if (safeIsAlNum(searchStr[i]))
       break;
   }
  
  if (searchStr[i] == '\0') 
    return(NULL);		
  else
    startTok = searchStr + i;	
  
  for (; searchStr[i] != '\0'; i++)
   { if (!safeIsAlNum(searchStr[i]))
       break;
   }
   
  if (searchStr[i] != '\0')
   { searchStr[i] = '\0';
     searchStr = searchStr + i + 1;
   }
  else
    searchStr = searchStr + i;
   
  return(startTok);
}
#endif /*NOTUSEDANDNOTTHREADSAFE*/
/*---------------------------------------------------------------------------*/

boolean 
substrcmp(char* string1,char* string2)
/* return true if they match up to the length of the smaller (but not if the
   smaller is empty)
 */
{
  
  register char *a, *b;
  
  a = string1;
  b = string2;
  
  /* see if they are both empty - if so them match */
  if (*a == '\0' && *b == '\0')
    return(true);
  
  /* see if either is empty (but not both) - no match */
  if (*a == '\0' || *b == '\0')
    return(false);

  /* otherwise we need to look through each byte */
  while (*a && *b)  
   { if (*a++ != *b++) 
       return(false);
   }
  return(true);
}

/*---------------------------------------------------------------------------*/

char 
char_downcase(unsigned long long_ch)
{
  unsigned char ch = long_ch & 0xFF; 
  
  return(((ch >= 'A') && (ch <= 'Z')) ? (ch + 'a' -'A') : ch);
}

/*---------------------------------------------------------------------------*/

char*
string_downcase(char *word)
{
  long i = 0;

  if (word == NULL)
    return(NULL);

  while (word[i] != '\0')
   { word[i] = char_downcase((unsigned long)word[i]);
     i++;
   }

  return(word);
}

/*---------------------------------------------------------------------------*/

long
cprintf(boolean print, char* format, ...)
{
  va_list ap;			
  if (print == 1)
    { long res;
      va_start(ap,format);	
      res = vprintf(format,ap);	
      va_end(ap);		
      return(res);
    }
  else
    return(0);
}

/*---------------------------------------------------------------------------*/

void
printHexByte(unsigned char h) 
{
  if (h < 0x10)
    printf("0%x",h);
  else
    printf("%x",h);
}

/*---------------------------------------------------------------------------*/
#ifdef NOTUSEDANDNOTTHREADSAFE
char*
commafy(long val,char* buf)
/* 
   prints the value using format, then adds commas separating every
   three orders of magnitude on the left side of the decimal point.

   limitations
   - there needs to be a buffer to write the result into.  We provide
     one for convenience, but of course that's not reentrant, so if you
     are calling this is multiple times before using the result you'll
     need to pass in your own buffer
   - only works on longs
   - only works on positive nums
   - no other formatting allowed
*/
{
  static char myBuf[100];

  if (buf == NULL)
    buf = myBuf;

  if (val < 1000) /* 1 thousand */
    sprintf(buf,"%ld",val);
  else if (val < 1000000) /* 1 millon */
    { long lessThanAGrand = val%1000;
      long moreThanAGrand = val - lessThanAGrand;
      sprintf(buf,"%ld,%03ld",moreThanAGrand/1000,lessThanAGrand);
    }
  else if (val < 1000000000) /* 1 trillion, 1 gig */
    { long lessThanAGrand = val%1000;
      long lessThanAMil = (val%1000000) - lessThanAGrand;
      long moreThanAMil = val - lessThanAMil - lessThanAGrand;
      sprintf(buf,"%ld,%03ld,%03ld",moreThanAMil/1000000,lessThanAMil/1000,
	      val%1000);
    }

  return(buf);
}
#endif NOTUSEDANDNOTTHREADSAFE
/*---------------------------------------------------------------------------*/
#ifdef NOTUSEDANDNOTTHREADSAFE
char*
printable_time(void)
{ 
  static char *string;
  time_t tptr;
  time(&tptr);
  string = ctime(&tptr);
  if (string)
   { if (string[strlen(string)-1] == '\n')
       string[strlen(string)-1] = '\0';   
     return(string+4);
   }
  else
    return("Time Unknown");
}
#endif /*NOTUSEDANDNOTTHREADSAFE*/
/*---------------------------------------------------------------------------*/

#ifdef BSD
#define NEED_VPRINTF
#endif

#ifdef NEED_VPRINTF
#ifdef OSK		
#define LONGINT
#define INTSPRINTF
#endif

#ifdef NOVOID
typedef char *pointer;
#else
typedef void *pointer;
#endif

#ifdef	INTSPRINTF
#define Sprintf(string,format,arg)	(sprintf((string),(format),(arg)))
#else
#define Sprintf(string,format,arg)	(\
	sprintf((string),(format),(arg)),\
	strlen(string)\
)
#endif

typedef int *intp;

int vsprintf(dest, format, args)
char *dest;
register char *format;
va_list args;
{
    register char *dp = dest;
    register char c;
    register char *tp;
    char tempfmt[64];
#ifndef LONGINT
    int longflag;
#endif

    tempfmt[0] = '%';
    while( (c = *format++) != 0) {
	if(c=='%') {
	    tp = &tempfmt[1];
#ifndef LONGINT
	    longflag = 0;
#endif
continue_format:
	    switch(c = *format++) {
		case 's':
		    *tp++ = c;
		    *tp = '\0';
		    dp += Sprintf(dp, tempfmt, va_arg(args, char *));
		    break;
		case 'u':
		case 'x':
		case 'o':
		case 'X':
#ifdef UNSIGNEDSPECIAL
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			dp += Sprintf(dp, tempfmt, va_arg(args, unsigned long));
		    else
#endif
			dp += Sprintf(dp, tempfmt, va_arg(args, unsigned));
		    break;
#endif
		case 'd':
		case 'c':
		case 'i':
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			dp += Sprintf(dp, tempfmt, va_arg(args, long));
		    else
#endif
			dp += Sprintf(dp, tempfmt, va_arg(args, int));
		    break;
		case 'f':
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		    *tp++ = c;
		    *tp = '\0';
		    dp += Sprintf(dp, tempfmt, va_arg(args, double));
		    break;
		case 'p':
		    *tp++ = c;
		    *tp = '\0';
		    dp += Sprintf(dp, tempfmt, va_arg(args, pointer));
		    break;
		case '-':
		case '+':
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
		case '.':
		case ' ':
		case '#':
		case 'h':
		    *tp++ = c;
		    goto continue_format;
		case 'l':
#ifndef LONGINT
		    longflag = 1;
		    *tp++ = c;
#endif
		    goto continue_format;
		case '*':
		    tp += Sprintf(tp, "%d", va_arg(args, int));
		    goto continue_format;
		case 'n':
		    *va_arg(args, intp) = dp - dest;
		    break;
		case '%':
		default:
		    *dp++ = c;
		    break;
	    }
	} else *dp++ = c;
    }
    *dp = '\0';
    return dp - dest;
}


int vfprintf(dest, format, args)
FILE *dest;
register char *format;
va_list args;
{
    register char c;
    register char *tp;
    register int count = 0;
    char tempfmt[64];
#ifndef LONGINT
    int longflag;
#endif

    tempfmt[0] = '%';
    while(c = *format++) {
	if(c=='%') {
	    tp = &tempfmt[1];
#ifndef LONGINT
	    longflag = 0;
#endif
continue_format:
	    switch(c = *format++) {
		case 's':
		    *tp++ = c;
		    *tp = '\0';
		    count += fprintf(dest, tempfmt, va_arg(args, char *));
		    break;
		case 'u':
		case 'x':
		case 'o':
		case 'X':
#ifdef UNSIGNEDSPECIAL
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			count += fprintf(dest, tempfmt, va_arg(args, unsigned long));
		    else
#endif
			count += fprintf(dest, tempfmt, va_arg(args, unsigned));
		    break;
#endif
		case 'd':
		case 'c':
		case 'i':
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			count += fprintf(dest, tempfmt, va_arg(args, long));
		    else
#endif
			count += fprintf(dest, tempfmt, va_arg(args, int));
		    break;
		case 'f':
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		    *tp++ = c;
		    *tp = '\0';
		    count += fprintf(dest, tempfmt, va_arg(args, double));
		    break;
		case 'p':
		    *tp++ = c;
		    *tp = '\0';
		    count += fprintf(dest, tempfmt, va_arg(args, pointer));
		    break;
		case '-':
		case '+':
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
		case '.':
		case ' ':
		case '#':
		case 'h':
		    *tp++ = c;
		    goto continue_format;
		case 'l':
#ifndef LONGINT
		    longflag = 1;
		    *tp++ = c;
#endif
		    goto continue_format;
		case '*':
		    tp += Sprintf(tp, "%d", va_arg(args, int));
		    goto continue_format;
		case 'n':
		    *va_arg(args, intp) = count;
		    break;
		case '%':
		default:
		    putc(c, dest);
		    count++;
		    break;
	    }
	} else {
	    putc(c, dest);
	    count++;
	}
    }
    return count;
}

vprintf(format, args)
char *format;
va_list args;
{
    return vfprintf(stdout, format, args);
}

#endif


/*---------------------------------------------------------------------------*/

void
fs_checkPtr(void* ptr)
{ 
  if (ptr == NULL)
    p_err_string = qsprintf_stcopyr(p_err_string, 
    	"checkPtr found a NULL pointer");
}

/*---------------------------------------------------------------------------*/

void*
fs_malloc(size_t size)
{ 
  void* ptr = NULL;

  if (size <= 0)  {
	/* Define error message - even though caller may change it */
	p_err_string = qsprintf_stcopyr(p_err_string,
		"Allocating zero or -ve size %d", size);
    return(NULL);
  }

#ifdef THINK_C
  ptr = (void*)NewPtr((long)size);
  s_checkPtr(ptr);
  memset(ptr,0,(size_t)size);	
#else  
  ptr = (void*)calloc((size_t)size,(size_t)1);
  s_checkPtr(ptr);
  if (!ptr) out_of_memory();
#endif
  
  return(ptr);
}

/*---------------------------------------------------------------------------*/

void*
fs_realloc(void* ptr,size_t size)
{ 
  register void* nptr = NULL;
  
  if (size <= 0)
    return(NULL);

  if (ptr == NULL)		
    return(s_malloc(size));
    
#ifdef THINK_C
  nptr = NewPtr(size);		 
  s_checkPtr(nptr);
  BlockMove(ptr,nptr,size);	
  DisposPtr(ptr);		    
#else
  nptr = (void*)realloc(ptr,size);
  s_checkPtr(ptr);
  if (!ptr) out_of_memory();
#endif  
   
  return(nptr);
}

/*---------------------------------------------------------------------------*/

void
fs_free(void* ptr)
{
  if (ptr != NULL)		
    {				
#ifdef THINK_C
      DisposPtr(ptr);
#else
      free(ptr);
#endif
    }
}

/*---------------------------------------------------------------------------*/

char*
fs_strncat(char* dst,char* src,size_t maxToAdd,size_t maxTotal)
{
  size_t dstSize = strlen(dst);
  size_t srcSize = strlen(src);
  
  if (dstSize + srcSize < maxTotal) 
    return(strncat(dst,src,maxToAdd));
  else
   { size_t truncateTo = maxTotal - dstSize - 1;
     char   saveChar;
     char*  result = NULL;

     saveChar = src[truncateTo];
     src[truncateTo] = '\0';
     result = strncat(dst,src,maxToAdd);
     src[truncateTo] = saveChar;
     return(result);
   }
}

/*---------------------------------------------------------------------------*/

char*
fs_strncpy(char* s1,char* s2,long n)
{
  s1[n-1] = '\0';
  return(strncpy(s1, s2, n - 1));
}

/*---------------------------------------------------------------------------*/
#ifdef NOTUSEDANDNOTTHREADSAFE
void
initMyPID(void)
{
  if (myPID == -1)
    myPID = getpid();
}
#endif /*NOTUSEDANDNOTTHREADSAFE*/

/*---------------------------------------------------------------------------*/

void
warn(char* message)
{
#ifdef THINK_C
  Debugger();
#else
  printf("%s\n<press return to continue>\n",message);
  getchar();
#endif
}

/*---------------------------------------------------------------------------*/

char*
next_arg(long* argc,char*** argv)
{
  if ((*argc)-- > 0)
   { char* arg = *((*argv)++);
     return(arg);
   }
  else
   { return(NULL);
   }
}

/*---------------------------------------------------------------------------*/

char* 
peek_arg(long* argc,char*** argv)
{
  if ((*argc) > 0)
    return(**argv);
  else
    return(NULL);
}

/*---------------------------------------------------------------------------*/
#ifdef NOTUSEDANDNOTTHREADSAFE
char*
readNextArg(long* argc,char*** argv,boolean* useFile,FILE* file)
/* 
   Like next_arg(), but can read arguments from a file (one line per arg).
   Note you must copy the returned value if you need access to more
   than one argument at a time.
 */
{
  static char buf[300];
  if (file != NULL && useFile != NULL && *useFile)
   { if (fgets(buf,300,file) == NULL) /* nothing more on the file */
      { *useFile = false;
	return(next_arg(argc,argv)); /* start reading regular args again */
      }
     else /* return a line from the file */
      { buf[strlen(buf) - 1] = '\0'; /* get rid of newline */
	return(buf);
      }
   }
  else /* just read regular args from the command line */
   { return(next_arg(argc,argv));
   }
}
#endif /*NOTUSEDANDNOTTHREADSAFE*/
/*---------------------------------------------------------------------------*/

#ifdef THINK_C

#if THINK_C == 1
#include <EventMgr.h>
#endif 

#if THINK_C == 5
#include <Events.h>
#endif

#ifdef WAIStation
#include "CRetrievalApp.h"
#endif 
#endif 

#ifdef NOTUSEDANDNOTTHREADSAFE
void
beFriendly()
{ 
#ifdef never
#ifdef THINK_C
   EventRecord  	macEvent;	

   static RgnHandle	mouseRgn = NULL; 
   long		        sleepTime; 	
   
#ifdef WAIStation
   gApplication->FrobWaitCursor();
#endif 
   
   if (mouseRgn == NULL)
     mouseRgn = NewRgn(); 
     
   sleepTime = 5; 
   
   WaitNextEvent(everyEvent,&macEvent,sleepTime,mouseRgn); 
#endif 
#endif
}
#endif /*NOTUSEDANDNOTTHREADSAFE*/

/*---------------------------------------------------------------------------*/





