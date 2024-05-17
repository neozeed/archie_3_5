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

#ifndef cutil_h
#define cutil_h

#include <stdarg.h>
#include <stdio.h>   
#ifndef EXIT_SUCCESS 
/* #include <stdlib.h> */	/* Commented out so compiles on SCO */
#endif /* ndef EXIT_SUCCESS */
#include "cdialect.h"

#if defined(THINK_C)  || defined(SCOUNIX)
#include <time.h>
#else
#include <sys/time.h>
#endif

#define byte char

#ifndef cutil_c
extern long myPID;
#endif /* ndef cutil_c */

/*---------------------------------------------------------------------------*/
/* Math utilities */
/*---------------------------------------------------------------------------*/

#ifndef MAXLONG
#define MAXLONG ((long)0x7FFFFFFF)
#endif 

#undef  MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#undef  MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#undef  ABS
#define ABS(x) (((x) < 0) ? (-(x)) : (x))

/*---------------------------------------------------------------------------*/
/* Define boolean data type */
/*---------------------------------------------------------------------------*/

#ifndef boolean
#define boolean	unsigned long
#endif /* ndef boolean */

#ifndef true
#define true 	((boolean)1)
#endif /* ndef true */

#ifndef false
#define false 	((boolean)0)   
#endif /* ndef false */

#ifndef TRUE
#define TRUE	true
#endif /* ndef TRUE */

#ifndef FALSE
#define FALSE	false
#endif /* ndef FALSE */

#ifndef NULL
#define NULL	(0L)
#endif /* ndef NULL */

/*---------------------------------------------------------------------------*/
/* String utlities */
/*---------------------------------------------------------------------------*/

#define IS_DELIMITER	(1L)
#define	NOT_DELIMITER	!IS_DELIMITER

#define STREQ(s1,s2) ((*(s1)==*(s2)) && !strcmp(s1,s2))
#define STRNCMP(s1,s2,n) \
  ((*(s1)==*(s2)) ? strncmp(s1,s2,n) : (*(s1) - *(s2)))

#define s_strncat(dst,src,maxToAdd,maxTotal)	\
  fs_strncat((dst),(src),(maxToAdd),(maxTotal))
#define s_strncpy(s1,s2,n) fs_strncpy((s1), (s2), (n))

#define safeIsAlNum(c) ((c > '`' && c < '{') || \
			(c > '/' && c < ':') || \
			(c > '@' && c < '['))

char* 	s_strdup(char* s);
char*	strtokf(char* s1,long (*isDelimiter)(long c)); 
char*   strtokf_isalnum(char* s1);
boolean substrcmp(char *string1, char *string2);
char    char_downcase(unsigned long ch);
char*   string_downcase(char* word);

/*---------------------------------------------------------------------------*/
/* Printing utilities */
/*---------------------------------------------------------------------------*/

#define NL() printf("\n")

long cprintf(boolean print,char* format,...);
void printHexByte(unsigned char h);
char* commafy(long val,char* buf);
char* printable_time(void);

/*---------------------------------------------------------------------------*/
/* Heap utilities */
/*---------------------------------------------------------------------------*/

#define s_checkPtr(ptr) 	fs_checkPtr(ptr)

#define s_malloc(size)	      	fs_malloc(size)

#define s_realloc(ptr,size)	fs_realloc((ptr),(size))
#define s_free(ptr)		{ fs_free((char*)ptr); ptr = NULL; }

void	fs_checkPtr(void* ptr);
void*	fs_malloc(size_t size);
void*	fs_realloc(void* ptr,size_t size);
void	fs_free(void* ptr);
char* 	fs_strncat(char* dst,char* src,size_t maxToAdd,size_t maxTotal);
char* 	fs_strncpy(char* s1,char* s2, long n);

/*---------------------------------------------------------------------------*/
/* Miscelaneous Utilities */
/*---------------------------------------------------------------------------*/

void initMyPID(void);
void warn(char* message);
char* next_arg(long* argc, char*** argv);
char* peek_arg(long* argc, char*** argv);
char* readNextArg(long* argc,char*** argv,boolean* useFile,FILE* file);
void beFriendly(void);

/*---------------------------------------------------------------------------*/

#endif /* ndef cutil_h */


