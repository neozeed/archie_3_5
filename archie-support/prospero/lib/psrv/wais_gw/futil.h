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

/*-------------------------------------------------------------------------- */

#ifndef futil_h
#define futil_h

#include "cdialect.h"
#include "cutil.h"

/*-------------------------------------------------------------------------- */

/* the maximum length of any filename in the system */
#define MAX_FILENAME_LEN (255L)

/* arguments for fseek, which aren't defined on all systems */
#ifndef SEEK_SET 
#define SEEK_SET (0L)  
#define SEEK_CUR (1L)  
#define SEEK_END (2L)  
#endif /* ndef SEEK_SET */

/* allow NeXT's to check if the file's mode indicates it's a directory */
#if (defined(NeXT) && !(defined(S_ISDIR)))
#define S_ISDIR(f_mode) ((f_mode) & S_IFDIR)
#endif

/*-------------------------------------------------------------------------- */

/* the following functions are "safe" versions of the standard fileio
   utilities.  They should be called through the macros rather than
   actually calling the functions themselves 
 */
/* Replaced by locked_{fopen,fclose} */
#if 0
#define safeFOpen(name,mode)	       doSafeFOpen((name),(mode))
#define safeFClose(file)	       { doSafeFClose((FILE*)(file)); \
					 (file) = NULL;}
#endif
#define safeFSeek(file,offset,whereFrom)                              \
                                       doSafeFSeek((file),(offset),(whereFrom))
#define safeFTell(file)	               doSafeFTell((file))
#define safeRename(path1,path2)	       doSafeRename((path1),(path2))
#if 0
FILE*	doSafeFOpen(char* fileName,char* mode);
long	doSafeFClose(FILE* file);
#endif
long 	doSafeFSeek(FILE* file,long offset,long wherefrom);
long 	doSafeFTell(FILE* file);


/* these functions read and write arbitrary numbers of bytes in 
   an architecture independent fashion
 */
long writeBytes(long value,long numBytes,FILE* stream);
long readBytes(long numBytes,FILE *stream);
long readBytesFromMemory(long numBytes,unsigned char* block);


/* these routines help manage unix style path names */
char *truename(char *filename,char *full_path);
char *pathnameName(char *pathname);
char *pathnameDirectory(char *pathname,char *destination);
char *mergePathnames(char *pathname,char *directory);


/* set and get characteristics of a file or directory*/
boolean directoryP(char* f);
boolean fileP(char* f);
boolean probeFile(char *filename);
boolean probeFilePossiblyCompressed(char *filename);
boolean touchFile(char *filename);
long fileLength(FILE* stream);
void getFileDates(char* filename,time_t* createDate,time_t* modDate);


/* miscellanious routines */
char *currentUserName(void);
boolean readStringFromFile(FILE* stream,char* array,long array_length);
#if 0
FILE* openTempFile(char* dir,char* root,char* fullName,char* mode);
#endif
void growFile(FILE* file,long len);

/*-------------------------------------------------------------------------- */

#endif /* ndef futil_h */
