#ifndef rmg_filelock_h
#define rmg_filelock_h

#include <pfs.h> 

struct filelock {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
	int	consistency;
#endif
	char    *name;
	int     readers;        /* -1 for writing */
/* Note we dont keep the fileno here since each locker opens seperately*/
	struct	filelock	*next;
	struct	filelock	*previous;
};
typedef struct filelock *FILELOCK;
typedef struct filelock FILELOCK_ST;

extern FILELOCK filelock_alloc();
extern void filelock_free(FILELOCK chan);
extern void filelock_lfree(FILELOCK chan);
extern void filelock_freespares();
extern FILELOCK filelock_copy(FILELOCK f, int r);
#ifdef PFS_THREADS
extern p_th_mutex p_th_mutexFILELOCK;
#endif

#endif /*rmg_filelock_h*/
