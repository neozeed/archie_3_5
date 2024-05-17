/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1991, 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>
#include <pcompat.h>
#include <pfs.h>
#include <psite.h>
#include <pmachine.h>
#include <perrno.h>
#ifdef SOLARIS
/* #include "p_solaris_stdlib.h"   \* Special tricks to prototype setenv() */
#else
#include <stdlib.h>           /* For malloc and free, getenv etc */
#endif

static char	vsroot_host[MAX_VPATH] = "";
static char	vsroot_file[MAX_VPATH] = "";

static char	vswork_host[MAX_VPATH] = "";
static char	vswork_file[MAX_VPATH] = "";
static char	vswork[MAX_VPATH] = "";

static char	vshome_host[MAX_VPATH] = "";
static char	vshome_file[MAX_VPATH] = "";
static char	vshome[MAX_VPATH] = "";

static char	vsdesc_host[MAX_VPATH] = "";
static char	vsdesc_file[MAX_VPATH] = "";
static char	vsname[MAX_VPATH] = "";


/* 
 * Note that there is one more environment variable used, PFS_DEFAULT,
 * but that environment variable is per process and is handled in the
 * pcompat library.
 */

#ifdef PUTENV                   /* If we don't have the lib. function */
static setenv();
#endif

void
pset_wd(wdhost,wdfile,wdname)
    char	*wdhost;
    char	*wdfile;
    char	*wdname;
{
  assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
    if(wdhost) {
        strcpy(vswork_host,wdhost);
        setenv("VSWORK_HOST",wdhost,1);
    }
    if(wdfile) {
        strcpy(vswork_file,wdfile);
        setenv("VSWORK_FILE",wdfile,1);
    }
    if(wdname) {
        strcpy(vswork,wdname);
        setenv("VSWORK",wdname,1);
    }
}

char * pget_wdhost()
{
    char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
    if(!*vswork_host && (env_st = getenv("VSWORK_HOST"))) 
        strcpy(vswork_host,env_st);
    if(*vswork_host) return(vswork_host);
    else return(NULL);
}

char * pget_wdfile()
    {
	char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(!*vswork_file && (env_st = getenv("VSWORK_FILE"))) 
	    strcpy(vswork_file,env_st);
	if(*vswork_file) return(vswork_file);
	else return(NULL);
    }

char * pget_wd()
    {
	char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(!*vswork && (env_st = getenv("VSWORK"))) 
	    strcpy(vswork,env_st);
	if(*vswork) return(vswork);
	else return(NULL);
    }

void
pset_hd(hdhost,hdfile,hdname)
    char	*hdhost;
    char	*hdfile;
    char	*hdname;
     {
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(hdhost) {
	    strcpy(vshome_host,hdhost);
	    setenv("VSHOME_HOST",hdhost,1);
	}
	if(hdfile) {
	    strcpy(vshome_file,hdfile);
	    setenv("VSHOME_FILE",hdfile,1);
	}
	if(hdname) {
	    strcpy(vshome,hdname);
	    setenv("VSHOME",hdname,1);
	}
    }

char * pget_hdhost()
    {
	char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(!*vshome_host && (env_st = getenv("VSHOME_HOST"))) 
	    strcpy(vshome_host,env_st);
	if(*vshome_host) return(vshome_host);
	else return(NULL);
    }

char * pget_hdfile()
    {
	char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(!*vshome_file && (env_st = getenv("VSHOME_FILE"))) 
	    strcpy(vshome_file,env_st);
	if(*vshome_file) return(vshome_file);
	else return(NULL);
    }

char * pget_hd()
    {
	char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(!*vshome && (env_st = getenv("VSHOME"))) 
	    strcpy(vshome,env_st);
	if(*vshome) return(vshome);
	else return(NULL);
    }

void
pset_rd(rdhost,rdfile)
    char	*rdhost;
    char	*rdfile;
     {
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(rdhost) {
	    strcpy(vsroot_host,rdhost);
	    setenv("VSROOT_HOST",rdhost,1);
	}
	if(rdfile) {
	    strcpy(vsroot_file,rdfile);
	    setenv("VSROOT_FILE",rdfile,1);
	}
    }

char * pget_rdhost()
    {
	char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(!*vsroot_host && (env_st = getenv("VSROOT_HOST"))) 
	    strcpy(vsroot_host,env_st);
	if(*vsroot_host) return(vsroot_host);
	else return(NULL);
    }

char * pget_rdfile()
    {
	char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(!*vsroot_file && (env_st = getenv("VSROOT_FILE"))) 
	    strcpy(vsroot_file,env_st);
	if(*vsroot_file) return(vsroot_file);
	else return(NULL);
    }

void
pset_desc(dhost,dfile,vsnm)
    char	*dhost;
    char	*dfile;
    char	*vsnm;
     {
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(dhost) {
	    strcpy(vsdesc_host,dhost);
	    setenv("VSDESC_HOST",dhost,1);
	}
	if(dfile) {
	    strcpy(vsdesc_file,dfile);
	    setenv("VSDESC_FILE",dfile,1);
	}
	if(vsnm) {
	    strcpy(vsname,vsnm);
	    setenv("VSNAME",vsnm,1);
	}
    }

char * pget_dhost()
    {
	char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(!*vsdesc_host && (env_st = getenv("VSDESC_HOST"))) 
	    strcpy(vsdesc_host,env_st);
	if(*vsdesc_host) return(vsdesc_host);
	else return(NULL);
    }

char * pget_dfile()
    {
	char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(!*vsdesc_file && (env_st = getenv("VSDESC_FILE"))) 
	    strcpy(vsdesc_file,env_st);
	if(*vsdesc_file) return(vsdesc_file);
	else return(NULL);
    }

char * pget_vsname()
    {
	char	*env_st;
    assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if(!*vsname && (env_st = getenv("VSNAME"))) 
	    strcpy(vsname,env_st);
	if(*vsname) return(vsname);
	else return(NULL);
    }

/*
 * Pprint shell string prints command on the standard output which when
 * interpreted by the shell will set environment variable and define
 * aliases needed to pass the prospero environment from program to
 * program.  This routine is called by vfsetup and vcd.
 *
 * The argument is a bit vector indicating which variable should be set.
 * The low order bit is (0x0):
 * 
 *    0x001   Print aliases
 *    0x002   Print virtual system description
 *    0x004   Print working directory
 *    0x008   Print home directory
 *    0x010   Print root directory
 *    0x1e0   Mask indicating which shell to print strings for.   We leave room
 *            for 16 possibilities for when more shells with their own weird
 *            syntaxes show up.
 */
#define SHELL_MASK	0x1e0
#define SHELL_ERROR     0x0     /* 0 is undefined */
#define SHELL_SH	0x020
#define SHELL_CSH	0x040

void
p__print_shellstring(int which)
{
    int	shell = which & SHELL_MASK;

    if((which & 0x1)&&(shell == SHELL_CSH)) {
        printf("alias vwd 'echo $VSWORK';\n");
        printf("alias vwp 'echo $VSWORK_HOST $VSWORK_FILE';\n");
        printf("alias vwho 'echo $VSNAME';\n");
        printf("alias vcd 'p__vcd -s csh \\!* >! /tmp/vfs$$;source /tmp/vfs$$;\
/bin/rm /tmp/vfs$$'\n");

#ifndef P_NO_PCOMPAT
        printf("alias venable 'setenv PFS_DEFAULT %d;if($?Poldcd == 0) set Poldcd = `alias cd`;alias pwd vwd;alias cd vcd';\n",PMAP_ATSIGN_NF);
        printf("set Presetcd = 'unalias cd;alias cd $Poldcd;unset Poldcd';\n");
        printf("alias vdisable 'unsetenv PFS_DEFAULT;unalias pwd;if($?Poldcd) eval $Presetcd';\n");
#endif P_NO_PCOMPAT
    } else if((which & 0x1)&&(shell == SHELL_SH)) {
        printf("vwd () { echo $VSWORK \n}\n");
        printf("vwp () { echo $VSWORK_HOST $VSWORK_FILE \n}\n");
        printf("vwho () { echo $VSNAME \n}\n");
        printf("vcd () { p__vcd -s sh $* > /tmp/vfs$$ ;. /tmp/vfs$$; /bin/rm /tmp/vfs$$ \n}\n");

#ifndef P_NO_PCOMPAT
        /* Note that this will bash any existing cd alias */
        printf("venable () { PFS_DEFAULT=%d;export PFS_DEFAULT;alias cd=vcd;alias pwd=vwd \n}\n",PMAP_ATSIGN_NF);
        printf("vdisable () { unset PFS_DEFAULT;unalias cd; unalias pwd\n}\n");
#endif  P_NO_PCOMPAT
    }
    if((which & 0x2)&&(shell == SHELL_CSH)) {
        printf("setenv VSDESC_HOST \"%s\";\n",pget_dhost());
        printf("setenv VSDESC_FILE \"%s\";\n",pget_dfile());
        printf("setenv VSNAME \"%s\";\n",pget_vsname());
    }
    else if((which & 0x2)&&(shell == SHELL_SH)) {
        printf("VSDESC_HOST=\"%s\";export VSDESC_HOST\n",pget_dhost());
        printf("VSDESC_FILE=\"%s\";export VSDESC_FILE\n",pget_dfile());
        printf("VSNAME=\"%s\";export VSNAME\n",pget_vsname());
    }

    if((which & 0x4)&&(shell == SHELL_CSH)) {
        printf("setenv VSWORK_HOST \"%s\";\n",pget_wdhost());
        printf("setenv VSWORK_FILE \"%s\";\n",pget_wdfile());
        printf("setenv VSWORK \"%s\";\n",pget_wd());
    }
    else if((which & 0x4)&&(shell == SHELL_SH)) {
        printf("VSWORK_HOST=\"%s\";export VSWORK_HOST\n",pget_wdhost());
        printf("VSWORK_FILE=\"%s\";export VSWORK_FILE\n",pget_wdfile());
        printf("VSWORK=\"%s\";export VSWORK\n",pget_wd());
    }

    if((which & 0x8)&&(shell == SHELL_CSH)) {
        printf("setenv VSHOME_HOST \"%s\";\n",pget_hdhost());
        printf("setenv VSHOME_FILE \"%s\";\n",pget_hdfile());
        printf("setenv VSHOME \"%s\";\n",pget_hd());
    }
    else if((which & 0x8)&&(shell == SHELL_SH)) {
        printf("VSHOME_HOST=\"%s\";export VSHOME_HOST\n",pget_hdhost());
        printf("VSHOME_FILE=\"%s\";export VSHOME_FILE\n",pget_hdfile());
        printf("VSHOME=\"%s\";export VSHOME\n",pget_hd());
    }

    if((which & 0x10)&&(shell == SHELL_CSH)) {
        printf("setenv VSROOT_HOST \"%s\";\n",pget_rdhost());
        printf("setenv VSROOT_FILE \"%s\";\n",pget_rdfile());
    }
    else if((which & 0x10)&&(shell == SHELL_SH)) {
        printf("VSROOT_HOST=\"%s\";export VSROOT_HOST\n",pget_rdhost());
        printf("VSROOT_FILE=\"%s\";export VSROOT_FILE\n",pget_rdfile());
    }
}


/* return a flag value for the shell that can be given to 
   p__print_shellstring().     If no shell defined, return 0 and set
   p_err_string.   This functions is only called by the VCD and VFSETUP
   commands at this time. */  
int
p__get_shellflag(const char *shellname)
{
    char  *shellenvar;
    int     shellenvarlen;
    if (shellname) {
        if (strequal(shellname, "sh")) return SHELL_SH;
        if (strequal(shellname, "csh")) return SHELL_CSH;
        p_err_string = qsprintf_stcopyr(p_err_string,
                 "The system doesn't know about shells of type \"%s\".  \
Please see the person who maintains Prospero at your site.", shellname);
        perrno = PFAILURE;
        return SHELL_ERROR;
    }
    /* this part is going to go away eventually.  The -s flag to p__vfsetup and
       p__vcd has effectively obsoleted it.  It's still around for a while, (a)
       in case people are using old versions of vfsetup.source or
       vfsetup.profil.  and (b) because I'm too sentimental to flush it yet :).
       */
    shellenvar = getenv("SHELL");
    if (!shellenvar) {
	p_err_string = qsprintf_stcopyr(p_err_string, 
			"Couldn't find SHELL environment variable; \
I don't know what kind of SHELL you're using.  Please see your maintainer.");
        perrno = PFAILURE;
        return SHELL_ERROR;               /* undefined */
    }
    shellenvarlen = strlen(shellenvar);
    if (shellenvarlen >=  3 && strequal(shellenvar + shellenvarlen - 3, "csh"))
        return SHELL_CSH;
    else
        return SHELL_SH;
}



#ifdef PUTENV
static int
setenv(char *n,char *v, int o)
{
    int	tmp;

    /* Allocate space for environment variable */
    char	*template = (char *) stalloc(strlen(n)+strlen(v)+2);

    sprintf(template,"%s=%s",n,v);
    tmp = putenv(template);
    return(tmp);

    /* Potential memory leak - it is not clear whether putenv     */
    /* deallocates the string with the old value of the           */
    /* envorinment variable. If not, then we should do so here.   */
    /* This concern is not valid (swa, 5/27/93):
       From the SUN OS 4.1.3 manual page for 'putenv()':

       int putenv(string)
       char *string;
       "the  string  pointed  to by string becomes part of the environment, so
       altering the string will change  the  environment. "

       The HP-UX 8.05 manual page is similarly worded.
       */


}
#endif PUTENV

#ifdef BADSETENV
/*
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)setenv.c	1.2 (Berkeley) 3/13/87";
#endif LIBC_SCCS and not lint

#include <sys/types.h>
#include <stdio.h>

/*
 * setenv(name,value,rewrite)
 *	Set the value of the environmental variable "name" to be
 *	"value".  If rewrite is set, replace any current value.
 */
setenv(name,value,rewrite)
	register char	*name,
			*value;
	int	rewrite;
{
	extern char	**environ;
	static int	alloced;		/* if allocated space before */
	register char	*C;
	int	l_value,
		offset;
	char *_findenv();

	assert(P_IS_THIS_THREAD_MASTER()); /* Not thread safe yet */
	if (*value == '=')			/* no `=' in value */
		++value;
	l_value = strlen(value);
	if ((C = _findenv(name,&offset))) {	/* find if already exists */
		if (!rewrite)
			return(0);
#if BUG
		if (strlen(C) <= l_value) {	/* smaller; copy over */
			while (*C++ = *value++);
			return(0);
		}
#else
		if (l_value <= strlen(C)) {	/* smaller; copy over */
			while (*C++ = *value++);
			return(0);
		}
#endif BUG
	}
	else {					/* create new slot */
		register int	cnt;
		register char	**P;

		for (P = environ,cnt = 0;*P;++P,++cnt);
		if (alloced) {			/* just increase size */
			environ = (char **)realloc((char *)environ,
			    (u_int)(sizeof(char *) * (cnt + 2)));
			if (!environ)
				return(-1);
		}
		else {				/* get new space */
			alloced = 1;		/* copy old entries into it */
			P = (char **)malloc((u_int)(sizeof(char *) *
			    (cnt + 2)));
			if (!P)
				return(-1);
			bcopy(environ,P,cnt * sizeof(char *));
			environ = P;
		}
		environ[cnt + 1] = NULL;
		offset = cnt;
	}
	for (C = name;*C && *C != '=';++C);	/* no `=' in name */
	if (!(environ[offset] =			/* name + `=' + value */
	    malloc((u_int)((int)(C - name) + l_value + 2))))
		return(-1);
	for (C = environ[offset];(*C = *name++) && *C != '=';++C);
	for (*C++ = '=';*C++ = *value++;);
	return(0);
}

#endif BADSETENV
