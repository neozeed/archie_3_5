/*
 * Copyright (c) 1992,1994 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <usc-license.h>

#include <pfs.h>                /* includes internal error definitions. */
#include <stdio.h>
#include <perrno.h>

/* This is set by p__fout_of_memory().  It is looked at by the server restart
   code in server/dirsrv.c and server/restart_srv.c. */

int p__is_out_of_memory = 0;

/* A function version of internal_error().  Used by macros. */
void
p__finternal_error(const char file[], int line, const char msg[])
{
#if 0
    write(2, "Internal error in file ", 
          sizeof "Internal error in file " -1); 
    write(2, file, strlen(file));
    write(2, ": ", 2); 
    write(2, msg, strlen(msg)); 
    write(2, "\n", 1);        
#endif
    fprintf(stderr, "Internal error at %s:%d: %s\n",
            file, line, msg);
    if (internal_error_handler)   
        (*internal_error_handler)(file, line, msg);   
    else  
        abort();
}

void
p__fout_of_memory(const char file[], int line) 
{
    p__is_out_of_memory++;
    p__finternal_error(file, line, "Out of Memory");
}

