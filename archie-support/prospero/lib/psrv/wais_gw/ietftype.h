#ifndef _rmg_ietftype_h
#define _rmg_ietftype_h

#include <pfs.h>		/* For ALLOCATOR_CONSISTENCY_CHECK*/
#include <pfs_threads.h>

struct ietftype {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
    int		consistency;
#endif
    char	*standardtype;
    char	*waistype;
    PATTRIB	prosperotype;
    struct ietftype	*next;
    struct ietftype	*previous;
};

typedef struct ietftype *IETFTYPE;
typedef struct ietftype IETFTYPE_ST;



extern PATTRIB ietftype_atput(IETFTYPE it, char *name);
extern	IETFTYPE ietftype_alloc();
extern	void ietftype_free(IETFTYPE it);
extern  void ietftype_lfree(IETFTYPE it);
extern	IETFTYPE ietftype_copy(IETFTYPE f,int r);
#ifdef PFS_THREADS
extern p_th_mutex p_th_mutexIETFTYPE;
#endif

#endif /*_rmg_ietftype_h*/
