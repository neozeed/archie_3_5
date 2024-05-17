#ifndef wais_buffalloc_h
#define wais_buffalloc_h

#include "inface.h"	/* For MAX_MESSAGE_LENGTH */
#include <pfs.h> 
#include <pfs_threads.h>

struct waismsgbuff {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
	int	consistency;
#endif
	char	buff[(size_t)MAX_MESSAGE_LENGTH * sizeof(char)];
	struct	waismsgbuff	*next;
	struct	waismsgbuff	*previous;
};
typedef struct waismsgbuff *WAISMSGBUFF;
typedef struct waismsgbuff WAISMSGBUFF_ST;

extern WAISMSGBUFF waismsgbuff_alloc();
extern void waismsgbuff_free(WAISMSGBUFF msgbuff);
extern void waismsgbuff_lfree(WAISMSGBUFF msgbuff);
extern void waismsgbuff_freespares();
/*extern WAISMSGBUFF waismsgbuff_copy(WAISMSGBUFF f, int r);*/

EXTERN_ALLOC_DECL(WAISMSGBUFF);

#endif /*wais_buffalloc_h*/
