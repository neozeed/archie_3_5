#ifndef VCACHE_MACROS_H
#define VCACHE_MACROS_H
#include <errno.h>
extern char *old_err_string;

#include <ardp.h>               /* for pfs_debug declaration. */
#include <pfs.h>                /* for unixerrstr() and qsprintf_stcopyr()
                                   prototyped declarations.  */

/* An error occured, the result is likely to be in errno */
#define ERRSYS(fmt, args...) { 						\
		old_err_string = p_err_string;				\
		p_err_string = qsprintf_stcopyr(NULL,fmt,		\
			## args,					\
			old_err_string,					\
			(errno ? unixerrstr() : ""));			\
	}

#define ERR(args...) { 							\
		old_err_string = p_err_string;				\
		p_err_string = qsprintf_stcopyr(NULL,			\
			## args,					\
			old_err_string);				\
	}

#define TRACE(level, args...) { 					\
	if (pfs_debug >= level ) fprintf(stderr,## args);		\
	fflush(stderr);							\
	}

#define RETURN(val) { retval = val ; goto cleanup ; } 

#endif /*VCACHE_MACROS_H*/
