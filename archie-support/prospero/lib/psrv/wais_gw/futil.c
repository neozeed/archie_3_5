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
/*-------------------------------------------------------------------------- */

#ifdef THINK_C

#include <pascal.h>		
#if THINK_C == 1
#include <FileMgr.h>
#endif
#if THINK_C == 5
#include <Files.h>
#endif

#else

#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/param.h> 
#include <sys/file.h>

#if defined(__svr4__) || defined(SCOUNIX)
#include <unistd.h>
#endif

#endif 

#include <string.h>
#include "futil.h"
#include "panic.h"
#include <pfs_threads.h>       /* For locked_fopen etc */
/*---------------------------------------------------------------------------*/

/* Shared by all threads */
static long numFilesCurrentlyOpen = 0;
static long maxNumFilesOpenAtOneTime = 0;
#if 0
FILE*
doSafeFOpen(char* filename,char* mode)
{
  FILE* file = NULL;
  char realMode[100];
  
  file = locked_fopen(filename,mode);

  if (file != NULL)
    { numFilesCurrentlyOpen++;
      if (numFilesCurrentlyOpen > maxNumFilesOpenAtOneTime)
	maxNumFilesOpenAtOneTime = numFilesCurrentlyOpen;
/*cprintf(dbg,"fopen %ld %s\n",fileno(file),filename);*/
    }

  return(file);
}

/*---------------------------------------------------------------------------*/

long
doSafeFClose(FILE* file)
{
  if (file != NULL)
    { numFilesCurrentlyOpen--;
      return(locked_fclose(file));
    }
  else
    return(0L);
}
#endif
/*---------------------------------------------------------------------------*/

long 
doSafeFSeek(FILE* file,long offset,long fromWhere)
{
  long result;
  
  if (file == NULL)
    return -1;
  
/*cprintf(dbg,"seek %ld %ld %ld\n",fileno(file),offset,fromWhere);*/

  if (offset == 0 && fromWhere == SEEK_CUR) /* this is a nop */
    return(0);

  result = fseek(file,offset,fromWhere);

/*  if (result != 0) 
    cprintf(dbg,"seek failed:  file %ld offset %ld, fromWhere %d\n",
	    fileno(file),offset,fromWhere);
*/

  return(result);
}

/*---------------------------------------------------------------------------*/

long 
doSafeFTell(FILE* file)
{
  long result;
  
  if (file == NULL)
    return(0);
    
  result = ftell(file);
  
  if (result == EOF)
    panic("A seek on an index file failed.\n");

  return(result);
}

/*---------------------------------------------------------------------------*/

long 
doSafeRename(char* path1,char* path2)
{

#ifdef THINK_C
  remove(path2);
#endif  

  return(rename(path1,path2));
}

/*---------------------------------------------------------------------------*/

long 
writeBytes(long value,long size,FILE* stream)
{
  long i;
  long answer;

  if ((size < sizeof(long)) && ((value >> (size * 8)) != 0))
    panic("In a call to writeBytes, the value %ld can not be represented in %ld bytes",value,size);

  for (i = size - 1; i >= 0; i--)
    answer = putc((value >> (i * 8)) & 0xFF,stream);

  if (ferror(stream) != 0) 
    panic("Write failed");

  return(answer);
}

/*---------------------------------------------------------------------------*/
#ifdef NEVERDEFINEDUNUSED
long 
readBytes(long n,FILE* stream)
{
  long answer = 0;
  unsigned long ch;
  long i;

  for (i = 0; i < n; i++)
   { ch = fgetc(stream);	/* Note - no timeout */
     if (ch == EOF)
      return(EOF);
     answer = (answer << 8) + (unsigned char)ch;
  }
  return(answer);
}
#endif
/*---------------------------------------------------------------------------*/

long 
readBytesFromMemory(long n,unsigned char* block)
{
  long answer = 0;
  unsigned char ch;
  long i;

  for (i = 0; i < n; i++)
   { ch = *(block++);
     answer = (answer << 8) + ch;
   }
  return(answer);
}

/*---------------------------------------------------------------------------*/

static char*
cleanPath(char* filename)
{

#ifdef THINK_C
  return(filename);
#endif /* def THINK_C */

  char* beginningPtr = strstr(filename,"/../");
  if (beginningPtr != NULL)
   { char *ptr;
     for (ptr = beginningPtr - 1; ptr >= filename; ptr--)
      { if (*ptr == '/')
	 { strcpy(ptr,beginningPtr + strlen("/../") - 1);
	   cleanPath(filename);	
	   break;
	 }	
      }
   }
  
  beginningPtr = strstr(filename,"/./");
  if (beginningPtr != NULL)
   { strcpy(beginningPtr,beginningPtr + strlen("/./") - 1);
     cleanPath(filename);	
   }

  return(filename);
}

/*---------------------------------------------------------------------------*/
    
char*
truename(char* filename,char* fullPath)
{
  
#ifdef THINK_C

  strcpy(fullPath,filename);
  return(fullPath); 

#else

  if (filename[0] == '/')
   { strcpy(fullPath,filename);
     cleanPath(fullPath);
     return(fullPath);
   }
  else
   { getwd(fullPath);
     s_strncat(fullPath,"/",MAX_FILENAME_LEN,MAX_FILENAME_LEN);
     s_strncat(fullPath,filename,MAX_FILENAME_LEN,MAX_FILENAME_LEN);
     cleanPath(fullPath);
     return(fullPath);
   }
#endif 
}

/*---------------------------------------------------------------------------*/

char*
pathnameName(char* pathname)
{

#ifdef THINK_C
  char *answer = strrchr(pathname,':');
#else
  char *answer = strrchr(pathname,'/');
#endif	

  if (answer == NULL)
    return(pathname);
  return(answer + 1);
}

/*---------------------------------------------------------------------------*/

char*
pathnameDirectory(char* pathname,char* destination)
{
#ifdef THINK_C
n  char *dirptr = strrchr(pathname,':');
#else
  char *dirptr = strrchr(pathname,'/');
#endif 

  if (dirptr == NULL)
   {
#ifdef THINK_C
     strncpy(destination,pathname,MAX_FILENAME_LEN);
#else
     strncpy(destination,"./",MAX_FILENAME_LEN);
#endif 
   }
  else
    { strncpy(destination,pathname,MAX_FILENAME_LEN);
      destination[dirptr - pathname + 1] = '\0';
    }

  return(destination);
}
  
/*---------------------------------------------------------------------------*/
#ifdef NOTUSEDANDNOTTHREADSAFE
char*
mergePathnames(char* pathname,char* directory)
{
  static char answer[MAX_FILENAME_LEN + 1];

  if ((pathname[0] == '/') || (NULL == directory) || directory[0] == '\0')
    return(pathname);
  else
   { answer[0] = '\0';
     strncat(answer,directory,MAX_FILENAME_LEN);

#ifdef THINK_C
    if (directory[strlen(directory) - 1] != ':')
      strncat(answer,":",MAX_FILENAME_LEN);
#else
    if (directory[strlen(directory) - 1] != '/')
      strncat(answer,"/",MAX_FILENAME_LEN);
#endif
    strncat(answer,pathname,MAX_FILENAME_LEN);
  }
  
  return(answer);
}
#endif /*NOTUSEDANDNOTTHREADSAFE*/

/*---------------------------------------------------------------------------*/

boolean 
directoryP(char* f)
/* XXX strangely, this fails if f = "".  It says that f is a directory */
{
#ifdef THINK_C
  return(false);
#else
  struct stat stbuf;
  if (f == NULL)
    return(FALSE);
  else if (stat(f,&stbuf) == -1)
    return(FALSE);
  else if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
    return(true);
  else
    return(FALSE);
#endif
}

/*---------------------------------------------------------------------------*/

boolean 
fileP(char* f)
{
  if (f == NULL)
    return(false);

#ifdef THINK_C
 return(probeFile(f));
#else
  { struct stat stbuf;
    if (stat(f,&stbuf) == -1)
      return(FALSE);
    else if (!((stbuf.st_mode & S_IFMT) == S_IFDIR))
      return(true);
    else
      return(FALSE);
  }
#endif
}

/*---------------------------------------------------------------------------*/

boolean 
probeFile(char* filename)
{
  if (filename == NULL)
    return(false);

#ifdef THINK_C  

 { FILE* f;
   f = locked_fopen(filename,"r");
   if (f == NULL)
     return(false);
   else
    { locked_fclose_A(f,filename,TRUE);
      return(true);
    }
 }

#else

  if (access(filename,R_OK) == 0)
    return(true);
  else
    return(false);

#endif 
}

/*---------------------------------------------------------------------------*/

boolean 
probeFilePossiblyCompressed(char* filename)
{
  if (filename == NULL)
    return(false);
  else if (!probeFile(filename))
    { char buffer[MAX_FILENAME_LEN * 2];
      strcpy(buffer,filename);
      strcat(buffer,".Z");
      return(probeFile(buffer));
    }
  else
    return(true);
}

/*---------------------------------------------------------------------------*/

boolean 
touchFile(char* filename)
{
  FILE *stream = NULL;
  if (filename == NULL)
    return(false);
  stream = locked_fopen(filename,"a");
  if (stream == NULL)
    return(false);
  else
    { locked_fclose_A(stream,filename,FALSE);
      return(true);
    }
}

/*---------------------------------------------------------------------------*/

long 
fileLength(FILE* stream)
{
  long position;
  long end = -1;
  if (stream == NULL)
    return(-1);
  position = ftell(stream);
  safeFSeek(stream,0L,SEEK_END);
  end = ftell(stream);	
  safeFSeek(stream,position,SEEK_SET);
  return(end);
}

/*---------------------------------------------------------------------------*/

void
getFileDates(char* filename,time_t* createDate,time_t* modDate)
{ 
#ifdef THINK_C
  *createDate = *modDate = 0;
#else
  struct stat buf;

  if (stat(filename,&buf) != 0)
    panic("could not stat %s",filename);

  *modDate =  buf.st_mtime;
  *createDate = buf.st_mtime; /* no separate creation date maintained in 
				 unix! */
#endif 
}

/*---------------------------------------------------------------------------*/

#ifdef NOTUSEDANDNOTTHREADSAFE
char*
currentUserName(void)
{
  static char answer[200];
  char hostname[120];
  
#ifdef THINK_C

  strcpy(answer,"MAC"); 

#else

#include <pwd.h>  

  assert(MASTER_IS_ONLY_UNTHREAD); /*getpwuid MT-Unsafe*/
  struct passwd *pwent = getpwuid(getuid());
  strncpy(answer,pwent->pw_name,200);
  strncat(answer,"@",200);
  gethostname(hostname,120);
  strncat(answer,hostname,200);

#endif 

  return(answer);
}
#endif /*NOTUSEDANDNOTTHREADSAFE*/
/*---------------------------------------------------------------------------*/
#ifdef NOTUSEDANDNOTIMEOUTONREAD
boolean 
readStringFromFile(FILE* stream,char* array,long arrayLength)
{
  long ch;
  long count = 0;

  array[0] = '\0';
  while (true)
   { ch = fgetc(stream);	/* Note no timeout */
     if(ch == EOF)
      { array[count] = '\0';
	return(false);
      }
    else if (count == arrayLength)
     { array[count] = '\0';
       return(false);
     }
    else if('\0' == ch)
     { array[count] = '\0';
       return(true);
     }
    else
      array[count++] = ch;
   }
}
#endif
/*---------------------------------------------------------------------------*/
/* Probably works, but unused */
#if 0 
FILE* 
openTempFile(char* dir,char* root,char* fullName,char* mode)
/* given dir and mode, return a file pointer to the file, fullName is the
   full name of the file including dir.  The leaf of the name begins with 
   root. Only makes sense if mode is a write or append mode
 */
{
    FILE* f;
    
    if (dir == NULL)
        dir = ".";

    if (root == NULL)
        root = "temp";

    if (!directoryP(dir))
        return(NULL);

    assert(P_IS_THIS_THREAD_MASTER()); /* random is MT-Unsafe */
    for(;;) {
        sprintf(fullName,"%s/%s%d.%d",dir,root,getpid(),random() % 10000);
        if ((f = locked_fopen(fullName,"r")) == NULL) { 
            /* no file there to read, we're ok */
            return locked_fopen(fullName, mode);
        }
        else
            locked_fclose(f,fullName, TRUE);
    }
}
#endif

/*---------------------------------------------------------------------------*/

void 
growFile(FILE* file,long length)
{
  long currentLength;
  safeFSeek(file,0L,SEEK_END);
  currentLength = safeFTell(file);
  safeFSeek(file,length - currentLength,SEEK_END);
}

/*---------------------------------------------------------------------------*/
