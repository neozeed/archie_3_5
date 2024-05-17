/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */

#include <usc-copyr.h>

#include <pfs_threads.h>

struct glink {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
	int	consistency;
#endif
    char type;
    char *name;
    char *selector;
    char *host;
    int port;
    char *protocol_mesg;        /* raw item description -- the Gopher
                                        protocol message. */
    struct glink *previous;
    struct glink *next;
};
typedef struct glink *GLINK;
typedef struct glink GLINK_ST;

extern GLINK glalloc(void);
extern void glfree(GLINK);
extern void gllfree(GLINK);
#ifdef PFS_THREADS
extern p_th_mutex p_th_mutexGLINK; /* declared in goph_gw_mutex.c */
#endif
