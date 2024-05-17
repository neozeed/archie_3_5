/* Copyright (c) 1993, by Pandora Systems */
/* Author:	Mitra <mitra@path.net> */
/* Allocation code copied and adapted from:
	lib/pfs/flalloc.c in the Prospero Alpha.5.2a release. */

#include <pfs.h>
#include <plog.h>
#include "flocks.h"

#include "mitra_macros.h"
#ifdef PFS_THREADS
extern p_th_mutex p_th_mutexFILES;
#endif

/* FileLocks are currently unused by dirsrv.c, and not linked into the ardp
   library. So dirsrv will not report on them in replies to a STATUS message.
   */ 

static FILELOCK	lfree = NULL;		/* Free filelocks */
/* These are global variables which will be read by dirsrv.c
   Too bad C doesn't have better methods for structuring such global data.  */

int		filelock_count = 0;
int		filelock_max = 0;
int             filelock_open = 0;
int             filelock_open_max = 0;
int             filelock_sepwaits = 0;
int             filelock_secwaits = 0;
FILELOCK        filelock_locked = NULL;

/************* Standard routines to alloc, free and copy *************/
/*
 * filelock_alloc - allocate and initialize FILELOCK structure
 *
 *    returns a pointer to an initialized structure of type
 *    FILELOCK.  If it is unable to allocate such a structure, it
 *    signals out_of_memory();
 */

FILELOCK
filelock_alloc()
{
    FILELOCK	fl;
    
    TH_STRUC_ALLOC(filelock,FILELOCK,fl);
    fl->name = NULL;
    fl->readers = 0;
    return(fl);
}

/*
 * filelock_free - free a FILELOCK structure
 *
 *    filelock_free takes a pointer to a FILELOCK structure and adds it to
 *    the free list for later reuse.
 */
void
filelock_free(FILELOCK fl)
{
    stfree(fl->name); fl->name = NULL;
    fl->readers = 0;
    TH_STRUC_FREE(filelock, FILELOCK, fl);
}

/*
 * filelock_lfree - free a linked list of FILELOCK structures.
 *
 *    filelock_lfree takes a pointer to a filelock structure frees it and 
 *    any linked
 *    FILELOCK structures.  It is used to free an entire list of FILELOCK
 *    structures.
 */
void
filelock_lfree(fl)
    FILELOCK	fl;
{
	TH_STRUC_LFREE(FILELOCK,fl,filelock_free);
}

static void
filelock_unreaders(FILELOCK flock)
{
  /* Assumes p_th_mutexFILES is locked already */
  assert(flock->readers > 0);
  if (!(--flock->readers)) {
    EXTRACT_ITEM(flock,filelock_locked);
    filelock_free(flock);
  }
}	

static void
filelock_unwriters(FILELOCK flock)
{
  /* Assumes p_th_mutexFILES is locked already */
 assert(flock->readers  == -1);
 EXTRACT_ITEM(flock, filelock_locked);
 filelock_free(flock);
}

void
filelock_freespares()
/* This is used for filelocks to free up space in the child */
{
	TH_FREESPARES(filelock,FILELOCK);
}

void
filelock_release(const char *filename, int readonly)
{
  FILELOCK flock;
  p_th_mutex_lock(p_th_mutexFILES);
  flock = filelock_locked;
  FIND_FNCTN_LIST(flock, name, filename, stequal);
  assert(flock);   /* it better be locked */
  if (readonly) {
    filelock_unreaders(flock);      /* May destroy flock */
  } else {
    filelock_unwriters(flock);	/* May destroy flock */
  }
  filelock_open--;
  p_th_mutex_unlock(p_th_mutexFILES);
}

int
locked_fclose_A(FILE *afile, const char *filename, int readonly)
{
  int retval;
  /* Assumes already obtained filelock for filename */
  retval = fclose(afile);
   /* At least on solaris, this can return an error for no apparent reason */
#if 0
  assert(!retval);
#endif
  filelock_release(filename, readonly);
  return(retval);
}

void
filelock_obtain(const char *filename, int readonly)
{
  FILELOCK(flock);
  int haswaited = 0;
  for (;;) {
    p_th_mutex_lock(p_th_mutexFILES);
    flock = filelock_locked;
    FIND_FNCTN_LIST(flock, name, filename,stequal);
    if (!flock) { /* didnt find a matching lock */
      flock = filelock_alloc();
      flock->name = stcopy(filename);
      flock->readers = 0;
      APPEND_ITEM(flock,filelock_locked);
    }
    /* Found, or created a matching lock */
    if (readonly) {
      if (flock->readers >= 0) {
	flock->readers++;
	break;
      }
       /* Drops thru here, if we want it readonly, but someone else writing*/
    } else {
      if (flock->readers == 0) {
	flock->readers = -1;      /* Special code for writer */
	break;
      }
      /* Drops thru here, if we want to write, but someone else 
	 is reading/writing */
    }
    /* At this point we cant lock it, so unlock mutex, wait, and try again*/
    p_th_mutex_unlock(p_th_mutexFILES);
    plog(L_QUEUE_INFO,NOREQ, "Waiting for filelock for %s", filename, 0);
    if (!haswaited) filelock_sepwaits++;
    filelock_secwaits++;
    sleep(1);     /* Maybe too long */
  } /*for*/
  if (++filelock_open > filelock_open_max) {
    filelock_open_max = filelock_open;
  }
  /* break is always done with mutex locked */
  p_th_mutex_unlock(p_th_mutexFILES);
}


FILE *
locked_fopen(const char *filename, const char *mode)
{
  FILELOCK flock;
  FILE *retval;
  int readonly = stequal(mode,"r");
  filelock_obtain(filename,readonly); /* Will wait till available */
  if (!(retval = fopen(filename,mode))) {
    filelock_release(filename, readonly);
  }
  return retval;
}

/* Suuitable sequence for creating a file via a temporary 
   filelock_obtain(filename,FALSE);
   if (!(afile = locked_fopen(tmpfilename, "w")) )
           filelock_release(filename,FALSE);
   written stuff to the file - retval set if fails
   return(locked_fclose_and_rename(afile, tmpfilename, filenam,retvale));
*/
   
int
locked_fclose_and_rename(
			FILE *afile, /* Open "w" temporary file */
			const char *tmpfilename, /* Name of temp file */
			const char *filename, /* Name desired */
			int retval) /* FALSE if ok to rename, TRUE cleanup*/
{

  if (fflush(afile) || ferror(afile)) { retval = PFAILURE; }
  if (locked_fclose_A(afile, tmpfilename, FALSE)) { retval = PFAILURE; }
  if (!retval) {		/* Dont attempt to rename if failed */
    if (rename(tmpfilename, filename)) { retval = PFAILURE; }
  }
  unlink(tmpfilename);
  filelock_release(filename,FALSE);
  return(retval);
}

