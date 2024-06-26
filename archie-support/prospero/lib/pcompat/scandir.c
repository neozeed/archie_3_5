/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)scandir.c	5.9 (Berkeley) 6/24/90";
#endif /* LIBC_SCCS and not lint */

/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct dirent (through namelist). Returns -1 if there were any errors.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>           /* For malloc and free */

#include <pmachine.h>

#ifdef USE_SYS_DIR_H
#include <sys/dir.h>
#else
#include <dirent.h>
#endif

#include <pfs_threads.h>	/* For PFS_THREADS */
/*
 * The DIRSIZ macro gives the minimum record length which will hold
 * the directory entry.  This requires the amount of space in struct dirent
 * without the d_name field, plus enough space for the name with a terminating
 * null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 */
#undef DIRSIZ
#define DIRSIZ(dp) \
    ((sizeof (struct dirent) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))

int
scandir(const char *dirname, struct dirent ***namelist, 
        int (*select)(const struct dirent *), 
        int (*dcomp)(const struct dirent **, const struct dirent **))
{
	register struct dirent *d, *p, **names;
	register int nitems;
	struct stat stb;
	long arraysz;
	DIR *dirp;
        /* The next variable shuts up GCC thinking that dcomp is of a different
           type from int  (*)() */
        int (*dcomp_qsort_losing_prototype)() = (int (*)()) dcomp;

#ifdef PFS_THREADS
    struct {
        struct dirent a;
        char b[_POSIX_PATH_MAX];
    } readdir_result_st;
    struct dirent *readdir_result = (struct dirent *) &readdir_result_st;
#endif
	if ((dirp = opendir(dirname)) == NULL) {
		return(-1);
	}
	if (fstat(dirp->dd_fd, &stb) < 0) {
	  closedir(dirp);
		return(-1);
	}

	/*
	 * estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	arraysz = (stb.st_size / 24);
	names = (struct dirent **)malloc(arraysz * sizeof(struct dirent *));
	if (names == NULL)
		return(-1);

	nitems = 0;
	assert(P_IS_THIS_THREAD_MASTER()); /*SOLARIS: readdir is MT-Unsafe */

#ifdef PFS_THREADS
	while ((d = readdir_r(dirp,readdir_result)) != NULL) {
#else
	while ((d = readdir(dirp)) != NULL) {
#endif
		if (select != NULL && !(*select)(d))
			continue;	/* just selected names */
		/*
		 * Make a minimum size copy of the data
		 */
		p = (struct dirent *)malloc(DIRSIZ(d));
		if (p == NULL)
			return(-1);
		p->d_ino = d->d_ino;
		p->d_reclen = d->d_reclen;
		p->d_namlen = d->d_namlen;
		bcopy(d->d_name, p->d_name, p->d_namlen + 1);
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems >= arraysz) {
			if (fstat(dirp->dd_fd, &stb) < 0)
				return(-1);	/* just might have grown */
			arraysz = stb.st_size / 12;
			names = (struct dirent **)realloc((char *)names,
				arraysz * sizeof(struct dirent *));
			if (names == NULL)
				return(-1);
		}
		names[nitems-1] = p;
	}
	closedir(dirp);
	if (nitems && dcomp != NULL)
		qsort(names, nitems, sizeof(struct dirent *), 
                      dcomp_qsort_losing_prototype);
	*namelist = names;
	return(nitems);
}

/*
 * Alphabetic order comparison routine for those who want it.
 */
int
alphasort(const struct dirent **d1, const struct dirent **d2)
{
	return strcmp((*d1)->d_name, (*d2)->d_name);
}
