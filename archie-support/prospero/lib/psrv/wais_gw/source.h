/* WIDE AREA INFORMATION SERVER SOFTWARE:
   Developed by Thinking Machines Corporation and put into the public
   domain with no guarantees or restrictions.

   This is part of the user-interface for the WAIS software.
*/

/*---------------------------------------------------------------------------*/

#ifndef _H_SOURCE
#define _H_SOURCE

#define STRINGSIZE	256

#ifndef boolean
#define boolean int
#endif
/* #include <cdialect.h> */

#ifdef IN_RMG
#include <pfs.h>			/* for ALLOCATOR_CONSISTENCY_CHECK*/
#endif

typedef struct SourceID {
  char *filename;
} _SourceID, *SourceID;

struct waissource{
#ifdef ALLOCATOR_CONSISTENCY_CHECK
	int	consistency;
#endif
  char *name;
  char *directory;
  char server[STRINGSIZE];
  char service[STRINGSIZE];
  char database[STRINGSIZE];
  char cost[STRINGSIZE];
  char units[STRINGSIZE];
  char *description;
  FILE *connection;
  long buffer_length;
  boolean initp;
  char *maintainer;
  char *subjects;
#ifdef IN_RMG
  struct	waissource *next;
  struct	waissource *previous;
#endif
};

typedef struct waissource *WAISSOURCE;
typedef struct waissource WAISSOURCE_ST;

/* functions */

extern WAISSOURCE waissource_alloc();
extern void waissource_free(WAISSOURCE source);
extern void waissource_lfree(WAISSOURCE source);
extern void waissource_freespares();
#ifdef PFS_THREADS
extern p_th_mutex p_th_mutexWAISSOURCE;
#endif
#ifdef NEVERDEFINED
/* Uses SourceList which is obsoleted by waissource 
   rebuild if need these functions in Prospero */
#if !defined(IN_RMG) && !defined(PFS_THREADS)
void freeSourceID (SourceID sid);
SourceID copysourceID (SourceID sid);
char** buildSourceItemList (SourceList sourcelist);
char** buildSItemList (SList sourcelist);
static short ReadSourceID (FILE* file, SourceID sid);
SourceList ReadListOfSources (FILE* fp);
#endif
#endif
static boolean ReadSource (WAISSOURCE source, FILE* file);
static boolean ReadSourceFile (WAISSOURCE asource, char* filename, char* directory);
static WAISSOURCE loadSource (char* name, char* sourcepath);
static void set_connection (WAISSOURCE source);
#ifdef NEVERDEFINED
#if !defined(IN_RMG) && !defined(PFS_THREADS)
boolean newSourcep (char* name);
boolean is_source (char* name, boolean test);
void SortSourceNames (int n);
void GetSourceNames (char* directory);
static void ReadSourceDirectory (char* directory, boolean test);
void WriteSource (char* directory, Source source, boolean overwrite);
SourceList makeSourceList (SourceID source, SourceList rest);
SList makeSList (Source source, SList rest);
void FreeSource (Source source);
void FreeSources (SList sources);
#endif
#endif
WAISSOURCE findsource (char* name, char* sourcepath);
#ifdef NEVERDEFINED
#if !defined(IN_RMG) && !defined(PFS_THREADS)
Source findSource (int n);
void format_source_cost (char* str, Source source);
void freeSource (SourceID sourceID);
void freeSourceList (SourceList slist);
boolean init_for_source (Source source, char* request,
			     long length, char* response);
#endif /*!IN_RMG && !PFS_THREADS */
#endif
#endif
