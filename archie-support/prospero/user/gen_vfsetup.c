/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 * Copyright (c) 1991, 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <stdio.h>
#include <string.h>

#include <psite.h>
#include <pmachine.h>

void
main(argc,argv)
    int		argc;
    char	*argv[];
{
    if (argc != 2) goto usage;
    if(strcmp(argv[1], "csh") == 0) {
        printf("set path=(%s $path);\n",P_PATH); 
        printf("alias vfsetup 'p__vfsetup -s csh \\!* >! /tmp/vfs$$;source /tmp/vfs$$;\
/bin/rm /tmp/vfs$$'\n");
    } else if (strcmp(argv[1], "sh") == 0) {
        /* Multiple directories separated by spaces may be specified for
           P_PATH */
        char p_path[] = P_PATH;
        char *cp;

        for (cp = p_path; *cp; cp++)
            if (*cp == ' ') 
                *cp = ':';
           
        printf("PATH=%s:$PATH\n",p_path); 
        /* The final newline before the closing } is needed for SunOS's SH */
        fputs("vfsetup () { p__vfsetup -s sh $* > /tmp/vfs$$;. /tmp/vfs$$; \
/bin/rm /tmp/vfs$$ \n}\n", stdout);
    } else {
        goto usage;
    }
    exit(0);

 usage:
    fprintf(stderr, "usage: %s { csh | sh }\n", argv[0]);
    exit(1);
       
}

