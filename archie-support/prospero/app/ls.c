/*
 * Derived from Berkeley source code.  Those parts are
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Michael Fischbein.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ls.c	5.42 (Berkeley) 5/17/90";
#endif /* not lint */

#define  DAYSPERNYEAR (365)
#define  SECSPERDAY (24*60*60)
#define  UT_NAMESIZE 8

#include <sys/types.h>
#include <sys/stat.h>
#ifndef S_ISLNK                 /* On the sun, this is defined for us in
                                   /usr/include/sys/stat.h, but it must not be
                                   defined there on all machines, or Berkeley
                                   wouldn't have put this into ls.c, right?
                                   --swa@isi.edu  */
#define  S_ISLNK(m)  ((S_IFLNK & m) == S_IFLNK)
#endif
#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <utmp.h>
#include <stdlib.h>

#include <pcompat.h>
#include <pmachine.h>

#ifdef USE_SYS_DIR_H
#include <sys/dir.h>
#else
#include <dirent.h>
#endif

/* At the moment this program doesn't work under HP-UX, because the
   compatability library's  version of opendir() fails under  HP-UX.
*/
#ifdef HPUX
#error "The program ls.c should not be compiled under HP-UX, since it will \
not work.  It relies upon the compatability library's version of opendir(), \
which has not yet been ported to HP-UX.  Please proceed with compiling the \
rest of  the Prospero clients."
#endif


char			*user_from_uid();
char			*group_from_gid();

typedef struct _lsstruct {
	char *name;			/* file name */
	int len;			/* file name length */
	struct stat lstat;		/* lstat(2) for file */
} LS;

int (*sortfcn)(), (*printfcn)();
int lstat();
char *emalloc();

int termwidth = 80;		/* default terminal width */

/* flags */
int f_accesstime;		/* use time of last access */
int f_column;			/* columnated format */
int f_group;			/* show group ownership of a file */
int f_ignorelink;		/* indirect through symbolic link operands */
int f_inode;			/* print inode */
int f_kblocks;			/* print size in kilobytes */
int f_listalldot;		/* list . and .. as well */
int f_listdir;			/* list actual directory, not contents */
int f_listdot;			/* list files beginning with . */
int f_longform;			/* long listing format */
int f_needstat;			/* if need to stat files */
int f_newline;			/* if precede with newline */
int f_nonprint;			/* show unprintables as ? */
int f_nosort;			/* don't sort output */
int f_recursive;		/* ls subdirectories also */
int f_reversesort;		/* reverse whatever sort is used */
int f_singlecol;		/* use single column output */
int f_size;			/* list size in short listing */
int f_statustime;		/* use time of last mode change */
int f_dirname;			/* if precede with directory name */
int f_timesort;			/* sort by time vice name */
int f_total;			/* if precede with "total" line */
int f_type;			/* add type character for non-regular files */

char *dummyargv[2] = {"", NULL};

#ifndef TIOCGWINSZ
#include <sys/termio.h>
#endif

main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind, stat();
	struct winsize win;
	int ch;
	char *p, *getenv();
	int acccmp(), modcmp(), namecmp(), prcopy(), printcol();
	int printlong(), printscol(), revacccmp(), revmodcmp(), revnamecmp();
	int revstatcmp(), statcmp();

	/* terminal defaults to -Cq, non-terminal defaults to -1 */
	if (isatty(1)) {
		f_nonprint = 1;
		if (ioctl(1, TIOCGWINSZ, &win) == -1 || !win.ws_col) {
			if (p = getenv("COLUMNS"))
				termwidth = atoi(p);
		}
		else
			termwidth = win.ws_col;
		f_column = 1;
	} else
		f_singlecol = 1;

	/* root is -A automatically */
	if (!getuid())
		f_listdot = 1;

	/* Print sizes in kilobytes by default */
	f_kblocks = 1;

	while ((ch = getopt(argc, argv, "1ACFLRacdfgiklqrstu")) != EOF) {
		switch (ch) {
		/*
		 * -1, -C and -l all override each other
		 * so shell aliasing works right
		 */
		case '1':
			f_singlecol = 1;
			f_column = f_longform = 0;
			break;
		case 'C':
			f_column = 1;
			f_longform = f_singlecol = 0;
			break;
		case 'l':
			f_longform = 1;
			f_column = f_singlecol = 0;
			break;
		/* -c and -u override each other */
		case 'c':
			f_statustime = 1;
			f_accesstime = 0;
			break;
		case 'u':
			f_accesstime = 1;
			f_statustime = 0;
			break;
		case 'F':
			f_type = 1;
			break;
		case 'L':
			f_ignorelink = 1;
			break;
		case 'R':
			f_recursive = 1;
			break;
		case 'a':
			f_listalldot = 1;
			/* FALLTHROUGH */
		case 'A':
			f_listdot = 1;
			break;
		case 'd':
			f_listdir = 1;
			break;
		case 'f':
			f_nosort = 1;
			break;
		case 'g':
			f_group = 1;
			break;
		case 'i':
			f_inode = 1;
			break;
		case 'k':
			f_kblocks = 1;
			break;
		case 'q':
			f_nonprint = 1;
			break;
		case 'r':
			f_reversesort = 1;
			break;
		case 's':
			f_size = 1;
			break;
		case 't':
			f_timesort = 1;
			break;
		default:
		case '?':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/* -d turns off -R */
	if (f_listdir)
		f_recursive = 0;

	/* if need to stat files */
	f_needstat = f_longform || f_recursive || f_timesort ||
	    f_size || f_type;

	/* select a sort function */
	if (f_reversesort) {
		if (!f_timesort)
			sortfcn = revnamecmp;
		else if (f_accesstime)
			sortfcn = revacccmp;
		else if (f_statustime)
			sortfcn = revstatcmp;
		else /* use modification time */
			sortfcn = revmodcmp;
	} else {
		if (!f_timesort)
			sortfcn = namecmp;
		else if (f_accesstime)
			sortfcn = acccmp;
		else if (f_statustime)
			sortfcn = statcmp;
		else /* use modification time */
			sortfcn = modcmp;
	}

	/* select a print function */
	if (f_singlecol)
		printfcn = printscol;
	else if (f_longform)
		printfcn = printlong;
	else
		printfcn = printcol;

	if (argc) doargs(argc, argv);
	else doargs(1,dummyargv);
	exit(0);
}

static char path[MAXPATHLEN + 1];
static char *endofpath = path;

doargs(argc, argv)
	int argc;
	char **argv;
{
	register LS *dstatp, *rstatp;
	register int cnt, dircnt, maxlen, regcnt;
	LS *dstats, *rstats;
	struct stat sb;
	int (*statfcn)(), stat(), lstat();
	char top[MAXPATHLEN + 1];
	u_long blocks;
	long	r_st_btotal;
	long	r_st_maxlen;

	/*
	 * walk through the operands, building separate arrays of LS
	 * structures for directory and non-directory files.
	 */
	dstats = rstats = NULL;
	statfcn = (f_longform || f_listdir) && !f_ignorelink ? lstat : stat;
	for (dircnt = regcnt = 0; *argv; ++argv) {
		if (statfcn(*argv, &sb)) {
			if (statfcn != stat || lstat(*argv, &sb)) {
			    (void)fflush(stdout);
			    (void)fprintf(stderr, "ls: %s: %s\n", *argv,
					  unixerrstr());
			    if (errno == ENOENT)
				continue;
			    exit(1);
			}
		}
		if (S_ISDIR(sb.st_mode) && !f_listdir) {
			if (!dstats)
				dstatp = dstats = (LS *)emalloc((u_int)argc *
				    (sizeof(LS)));
			dstatp->name = *argv;
			dstatp->lstat = sb;
			++dstatp;
			++dircnt;
		}
		else {
			if (!rstats) {
				rstatp = rstats = (LS *)emalloc((u_int)argc *
				    (sizeof(LS)));
				blocks = 0;
				maxlen = -1;
			}
			rstatp->name = *argv;
			rstatp->lstat = sb;

			/* save name length for -C format */
			rstatp->len = strlen(*argv);

			if (f_nonprint)
				prcopy(*argv, *argv, rstatp->len);

			/* calculate number of blocks if -l/-s formats */
			if (f_longform || f_size)
				blocks += sb.st_blocks;

			/* save max length if -C format */
			if (f_column && maxlen < rstatp->len)
				maxlen = rstatp->len;

			++rstatp;
			++regcnt;
		}
	}
	/* display regular files */
	if (regcnt) {
		displaydir("", rstats, regcnt, blocks, maxlen);
		f_newline = f_dirname = 1;
	}
	/* display directories */
	if (dircnt) {
		register char *p;

		f_total = 1;
		if (dircnt > 1) {
			(void)getwd(top);
			qsort((char *)dstats, dircnt, sizeof(LS), sortfcn);
			f_dirname = 1;
		}
		for (cnt = 0; cnt < dircnt; ++dstats) {
			for (endofpath = path, p = dstats->name;
			    *endofpath = *p++; ++endofpath);
			subdir("",dstats);
			f_newline = 1;
			if (++cnt < dircnt && chdir(top)) {
				(void)fprintf(stderr, "ls: %s: %s\n",
				    top, unixerrstr());
				exit(1);
			}
		}
	}
}

displaydir(subdirname, stats, num, blocks, maxlen)
        char *subdirname;
	LS *stats;
	register int num;
        long blocks;
        long maxlen;
{
	register char *p, *savedpath;
	LS *lp;

	if (num > 1 && !f_nosort) {
		u_long save1, save2;
		qsort((char *)stats, num, sizeof(LS), sortfcn);
	}

	printfcn(subdirname, stats, num, blocks, maxlen);

	if (f_recursive) {
		savedpath = endofpath;
		for (lp = stats; num--; ++lp) {
			if (!S_ISDIR(lp->lstat.st_mode))
				continue;
			p = lp->name;
			if (p[0] == '.' && (!p[1] || p[1] == '.' && !p[2]))
				continue;
			if (endofpath != path && endofpath[-1] != '/')
				*endofpath++ = '/';
			for (; *endofpath = *p++; ++endofpath);
			f_newline = f_dirname = f_total = 1;
			subdir(subdirname,lp);
			*(endofpath = savedpath) = '\0';
		}
	}
}

subdir(wdir,lp)
        char	*wdir;
	LS *lp;
{
        char	sbdirname[MAXPATHLEN];
	LS *stats;
	int num;
	char *names;
	long	blocks;
	int	maxlen;

	if(*wdir) sprintf(sbdirname,"%s/%s",wdir,lp->name);
	else strcpy(sbdirname,lp->name);

	if (f_newline)
		(void)putchar('\n');
	if (f_dirname)
		(void)printf("%s:\n", path);

	if (num = tabdir(sbdirname, lp, &stats, &names, &blocks, &maxlen)) {
		displaydir(sbdirname, stats, num, blocks, maxlen);
		(void)free((char *)stats);
		(void)free((char *)names);
	}
}

tabdir(dirname, lp, s_stats, s_names, blocksp, maxlenp)
        char    *dirname;
	LS *lp, **s_stats;
	char **s_names;
        u_long	*blocksp;
        int	*maxlenp;
{
	register DIR 	*dirp;
	register int 	cnt, maxentry, maxlen;
	register char 	*p, *names;
#define NAMES_BLSIZ 2048
	int	      	names_len = 4*NAMES_BLSIZ;
	int		names_off;
	struct dirent 	*dp;
	u_long 		blocks;
	LS 		*stats;
	char		newfname[MAXPATHLEN];


	if (!(dirp = opendir(dirname))) {
		(void)fprintf(stderr, "ls: %s: %s\n", lp->name,
		    unixerrstr());
		return(0);
	}
	blocks = maxentry = maxlen = 0;
	stats = NULL;
	assert(P_IS_THIS_THREAD_MASTER()); /* readdir is MT-Unsafe */
	for (cnt = 0; dp = readdir(dirp);) {
		/* this does -A and -a */
		p = dp->d_name;
		if (p[0] == '.') {
			if (!f_listdot)
				continue;
			if (!f_listalldot && (!p[1] || p[1] == '.' && !p[2]))
				continue;
		}
		if (cnt == maxentry) {
			if (!maxentry)
			    *s_names = names = 	emalloc(names_len);

			if((names_len - (names_off = names - *s_names))
			   < (2*NAMES_BLSIZ)) {
			    names_len += 2*NAMES_BLSIZ;
			    *s_names = (char *) realloc(*s_names,names_len);
			    names = *s_names + names_off;
			}
#define	DEFNUM	256
			maxentry += DEFNUM;

 			if (stats==NULL) 
 			    *s_stats = stats = 
				(LS *) emalloc((u_int)maxentry * sizeof (LS));
 		        else if (!(*s_stats = stats = 
				   (LS *) realloc((char *)stats,
					  (u_int)maxentry * sizeof(LS))))
				nomem();
		}

		if(*dirname) sprintf(newfname,"%s/%s",dirname,dp->d_name);
		else strcpy(newfname,dp->d_name);

		if (f_needstat && lstat(newfname, &stats[cnt].lstat)) {
		    /*
		     * don't exit -- this could be an NFS mount that has
		     * gone away.  Flush stdout so the messages line up.
		     */
		    (void)fflush(stdout);
		    (void)fprintf(stderr, "ls: %s: %s\n",
				  dp->d_name, unixerrstr());
		    continue;
		}
		stats[cnt].name = names;

		if (f_nonprint)
			prcopy(dp->d_name, names, (int)dp->d_namlen);
		else
			bcopy(dp->d_name, names, (int)dp->d_namlen);
		names += dp->d_namlen;
		*names++ = '\0';

		/*
		 * get the inode from the directory, so the -f flag
		 * works right.
		 */
		stats[cnt].lstat.st_ino = dp->d_ino;

		/* save name length for -C format */
		stats[cnt].len = dp->d_namlen;

		/* calculate number of blocks if -l/-s formats */
		if (f_longform || f_size)
			blocks += stats[cnt].lstat.st_blocks;

		/* save max length if -C format */
		if (f_column && maxlen < (int)dp->d_namlen)
			maxlen = dp->d_namlen;
		++cnt;
	}
	(void)closedir(dirp);

	if (cnt) {
		*blocksp = blocks;
		*maxlenp = maxlen;
	} else if (stats) {
		(void)free((char *)stats);
		(void)free((char *)names);
	}
	return(cnt);
}

printscol(dirname, stats, num, blocks, maxlen)
        char  *dirname;
	register LS *stats;
	register int num;
	long blocks;
	long maxlen;
{
	for (; num--; ++stats) {
		(void)printaname(stats);
		(void)putchar('\n');
	}
}

printlong(dirname, stats, num , blocks, maxlen)
        char *dirname;
	LS *stats;
	register int num;
	u_long blocks;
	int    maxlen ;
{
	char modep[15];

	if (f_total)
		(void)printf("total %lu\n", f_kblocks ?
		    howmany(blocks, 2) :
		    blocks);
	for (; num--; ++stats) {
		if (f_inode)
			(void)printf("%6lu ", stats->lstat.st_ino);
		if (f_size)
			(void)printf("%4ld ", f_kblocks ?
			    howmany(stats->lstat.st_blocks, 2) :
			    stats->lstat.st_blocks);
		(void)strmode(stats->lstat.st_mode, modep);
		(void)printf("%s %3u %-*s ", modep, stats->lstat.st_nlink,
		    UT_NAMESIZE, user_from_uid((uid_t) stats->lstat.st_uid));
		if (f_group)
			(void)printf("%-*s ", UT_NAMESIZE,
			    group_from_gid((gid_t) stats->lstat.st_gid));
		if (S_ISCHR(stats->lstat.st_mode) ||
		    S_ISBLK(stats->lstat.st_mode))
			(void)printf("%3d, %3d ", major(stats->lstat.st_rdev),
			    minor(stats->lstat.st_rdev));
		else
			(void)printf("%8ld ", stats->lstat.st_size);
		if (f_accesstime)
			printtime(stats->lstat.st_atime);
		else if (f_statustime)
			printtime(stats->lstat.st_ctime);
		else
			printtime(stats->lstat.st_mtime);
		(void)printf("%s", stats->name);
		if (f_type)
			(void)printtype(stats->lstat.st_mode);
		if (S_ISLNK(stats->lstat.st_mode))
			printlink(dirname,stats->name);
		(void)putchar('\n');
	}
}

printcol(dirname, stats, num, blocks, maxlen)
        char	*dirname;
	LS *stats;
	int num;
	u_long blocks;
	int maxlen;
{
	extern int termwidth;
	register int base, chcnt, cnt, col, colwidth;
	int endcol, numcols, numrows, row;

	colwidth = maxlen + 2;
	if (f_inode)
		colwidth += 6;
	if (f_size)
		colwidth += 5;
	if (f_type)
		colwidth += 1;

	if (termwidth < 2 * colwidth) {
		printscol(dirname, stats, num, blocks, maxlen);
		return;
	}

	numcols = termwidth / colwidth;
	numrows = num / numcols;
	if (num % numcols)
		++numrows;

	if (f_size && f_total)
		(void)printf("total %lu\n", f_kblocks ?
		    howmany(blocks, 2) : blocks);
	for (row = 0; row < numrows; ++row) {
		endcol = colwidth;
		for (base = row, chcnt = col = 0; col < numcols; ++col) {
			chcnt += printaname(stats + base);
			if ((base += numrows) >= num)
				break;
			while ((cnt = chcnt + 1) <= endcol) {
				(void)putchar(' ');
				chcnt = cnt;
			}
			endcol += colwidth;
		}
		putchar('\n');
	}
}

/*
 * print [inode] [size] name
 * return # of characters printed, no trailing characters
 */
printaname(lp)
	LS *lp;
{
	int chcnt;

	chcnt = 0;
	if (f_inode) {
	    printf("%5lu ", lp->lstat.st_ino);
	    chcnt += 6;
	}
	if (f_size) {
	    printf("%4ld ", f_kblocks ?
		   howmany(lp->lstat.st_blocks, 2) : lp->lstat.st_blocks);
	    chcnt += 5;
	}
	printf("%s", lp->name);
	chcnt += strlen(lp->name);

	if (f_type) chcnt += printtype(lp->lstat.st_mode);

	return(chcnt);
}

printtime(ftime)
	time_t ftime;
{
	int i;
	char *longstring, *ctime();
	time_t time();

	if(ftime == 0) fputs("-            ",stdout);
	else {

	    DISABLE_PFS(longstring = ctime(&ftime));
	    for (i = 4; i < 11; ++i)
		(void)putchar(longstring[i]);

#define	SIXMONTHS	((DAYSPERNYEAR / 2) * SECSPERDAY)
	    if (ftime + SIXMONTHS > time((time_t *)NULL))
		for (i = 11; i < 16; ++i)
		    (void)putchar(longstring[i]);
	    else {
		(void)putchar(' ');
		for (i = 20; i < 24; ++i)
		    (void)putchar(longstring[i]);
	    }
	    (void)putchar(' ');
	}
}

printtype(mode)
	mode_t mode;
{
	switch(mode & S_IFMT) {
	case S_IFDIR:
		(void)putchar('/');
		return(1);
	case S_IFLNK:
		(void)putchar('@');
		return(1);
	case S_IFSOCK:
		(void)putchar('=');
		return(1);
	}
	if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
		(void)putchar('*');
		return(1);
	}
	return(0);
}

printlink(dirname, name)
    char *dirname;
    char *name;
{
        char	linkname[MAXPATHLEN];
	int lnklen;
	char path[MAXPATHLEN + 1];

	if(*dirname) sprintf(linkname,"%s/%s",dirname, name);
	else strcpy(linkname,name);

	if ((lnklen = readlink(linkname, path, MAXPATHLEN)) == -1) {
	    fflush(stdout);
	    (void)fprintf(stderr, "\nls: %s: %s", name, unixerrstr());
	    fflush(stderr);
	    return;
	}
	path[lnklen] = '\0';
	(void)printf(" -> %s", path);
}


prcopy(src, dest, len)
	register char *src, *dest;
	register int len;
{
	register int ch;

	while(len--) {
		ch = *src++;
		*dest++ = isprint(ch) ? ch : '?';
	}
}

char
*emalloc(size)
	u_int size;
{
	char *retval, *malloc();

	if (!(retval = malloc(size)))
		nomem();
	return(retval);
}

nomem()
{
	(void)fprintf(stderr, "ls: out of memory.\n");
	exit(1);
}

int
usage()
{
	(void)fprintf(stderr, "usage: ls [-1ACFLRacdfgiklqrstu] [file ...]\n");
	exit(1);
}



namecmp(a, b)
	LS *a, *b;
{
	return(strcmp(a->name, b->name));
}

revnamecmp(a, b)
	LS *a, *b;
{
	return(strcmp(b->name, a->name));
}

modcmp(a, b)
	LS *a, *b;
{
	return(a->lstat.st_mtime < b->lstat.st_mtime);
}

revmodcmp(a, b)
	LS *a, *b;
{
	return(b->lstat.st_mtime < a->lstat.st_mtime);
}

acccmp(a, b)
	LS *a, *b;
{
	return(a->lstat.st_atime < b->lstat.st_atime);
}

revacccmp(a, b)
	LS *a, *b;
{
	return(b->lstat.st_atime < a->lstat.st_atime);
}

int
statcmp(a, b)
	LS *a, *b;
{
	return(a->lstat.st_ctime < b->lstat.st_ctime);
}

int
revstatcmp(a, b)
	LS *a, *b;
{
	return(b->lstat.st_ctime < a->lstat.st_ctime);
}

void
strmode(short mode,char *modestr)
{
	strcpy(modestr,"----------");

	if(mode & S_IFDIR) modestr[0] = 'd';
	if((mode & S_IFLNK) == S_IFLNK) modestr[0] = 'l';
	if(mode & S_IREAD) modestr[1] = 'r';
	if(mode & S_IWRITE) modestr[2] = 'w';
	if(mode & S_IEXEC) modestr[3] = 'x';
	if(mode & S_ISUID) modestr[3] = 's';
	if(mode & (S_IREAD>>3)) modestr[4] = 'r';
	if(mode & (S_IWRITE>>3)) modestr[5] = 'w';
	if(mode & (S_IEXEC>>3)) modestr[6] = 'x';
	if(mode & S_ISGID) modestr[6] = 's';
	if(mode & (S_IREAD>>6)) modestr[7] = 'r';
	if(mode & (S_IWRITE>>6)) modestr[8] = 'w';
	if(mode & (S_IEXEC>>6)) modestr[9] = 'x';
    }


char *
user_from_uid(uid)
    uid_t		uid;
    {
	static char	uidstring[10];
	struct passwd *pwent;

	if(uid == (uid_t) -1) return("-");

	assert(P_IS_THIS_THREAD_MASTER()); /*getpwuid unsafe */
	DISABLE_PFS(pwent =  getpwuid(uid));

	if (pwent == NULL) {
	    sprintf(uidstring,"%d",uid);
	    return(uidstring);
	}
	else return(pwent->pw_name);
    }


char *
group_from_gid(gid)
    gid_t		gid;
    {
	static char	gidstring[10];
	struct group	 *grent;

	if(gid == (gid_t) -1) return("-");

	assert(P_IS_THIS_THREAD_MASTER());       /*SOLARIS getgrgid MT-Unsafe */
	DISABLE_PFS(grent =  getgrgid(gid));

	if (grent == NULL) {
	    sprintf(gidstring,"%d",gid);
	    return(gidstring);
	}
	else return(grent->gr_name);
    }

