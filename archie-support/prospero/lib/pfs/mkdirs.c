/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>
#include <sys/param.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <pmachine.h>
#include <pfs.h>
#include <perrno.h>

/*
 * mkdirs - Make a directory and all superior directories
 *
 *          MKDIRS takes a pathame for a directory, checks to see
 *          whether it exists, and if not creates it.  Any parent
 *          directories which do not exist will be created as
 *          well.
 *
 *    ARGS: path    - path of the directoriy to be created
 *
 * RETURNS: 0 on success
 *          the contents of errno on error
 */
/*
 *  I've mutilated this file because most calls I found where having to
 *  go through the effort of making a string for the directory name
 *  from a file
 *
 *  The old version also had a bug (though I found it through looking at
 *  the code, not from experience, and a number of non-standards etc
 *  Specifically:
 *  if errno was non-zero on entry, and the directory exists, then
 * 	the code would find stat==0, it was a directory, tmp is zero,
 *	and it would return the errno (or even fail to recreate the directory)
 *	it should return immediately if the directory exists
 *
 *  rindex is not POSIX
 *
 *  it uses MAXPATHLEN, also non-POSIX
 *
 *  the headers in here, conflicted with those in pfs.h
 */
#define RETURN(r1)	{ retval=r1 ; goto cleanup;  }
/* Argument should be TRUE if one should not strip off the trailing suffix,
 *  false if one does need to strip off the trailing suffix. 
 */
int
p__mkdirs(const char path[], int wantdir)
{
    char *prefix = NULL;
    char *suffix;			/* Pointer into prefix */
    struct stat st;
    int mode = 0777;
    int retval = 0;

    if (stat(path, &st) == 0) {	/* exists, either dir. or file */
	if (!wantdir)
	    return 0;	/* File exists, so directory must */
	if (wantdir && !S_ISDIR(st.st_mode))
	    return (EEXIST);	/* already exists and is not dir. */
	return 0;	/* Directory exists */
    }

    if (!wantdir) {		/* Want file, build parent directory */
	prefix = stcopy(path);
	if (!(suffix = strrchr(prefix, '/'))) {
	    RETURN(PSUCCESS);	/* No directory to build - thats ok*/
	} else {
	    *suffix = '\0';
	    RETURN(p__mkdirs(prefix, 1)) ;	/* Recurse on directory*/
	}
    } else {	/*wantdir*/
	/* If want directory, but specified trailing slash*/
	if (path[strlen(path)-1] == '/') {
		prefix = stcopy(path);
		prefix[strlen(prefix)-1] = '\0';
		RETURN(p__mkdirs(prefix, 1)); /*recurse without /*/
    }
    if (!mkdir(path, mode)) {
	chmod(path, mode);
	RETURN(0);
    }
    switch (errno) {
    case ENOENT:		/* Failed because no parent, recurse for it */
    prefix = stcopy(path);
    if (suffix = strrchr(prefix, '/'))
	*suffix = '\0';
    else
	*prefix = '\0';

      	assert(*prefix != '\0'); 		/* Is no parent, strange */

      	if (retval = p__mkdirs(prefix, TRUE))
		RETURN(retval);			/* Recurse on parent */
	RETURN(p__mkdirs(path,TRUE));		/* Recurse on self */
    case EEXIST:		/* Someone else created it for us */
	RETURN(PSUCCESS);
    default:			/* mkdir failed for some other reason */
	p_err_string = qsprintf_stcopyr(p_err_string,
		"Couldnt create %s: %s", path, unixerrstr());
	RETURN(errno);	
    }
  } /*wantdir*/

cleanup:
	stfree(prefix);
	return(retval);
}

/* back compatibility*/
int
mkdirs(const char *path)
{
    return p__mkdirs(path, 1);
}
