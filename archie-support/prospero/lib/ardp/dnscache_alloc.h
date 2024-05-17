#ifndef dnscache_alloc_h
#define dnscache_alloc_h

#include <pfs.h> 
#include <pfs_threads.h>
#include <netinet/in.h>

struct dnscache {
#ifdef ALLOCATOR_CONSISTENCY_CHECK
	int	consistency;
#endif
	char	*name;
	struct	sockaddr_in sockad;
	int     usecount;       /* For determining size of cache */
	struct	dnscache	*next;
	struct	dnscache	*previous;
};
typedef struct dnscache *DNSCACHE;
typedef struct dnscache DNSCACHE_ST;

extern DNSCACHE dnscache_alloc();
extern void dnscache_free(DNSCACHE acache);
extern void dnscache_lfree(DNSCACHE acache);
extern void dnscache_freespares();
extern int dnscache_count, dnscache_max;
/*extern DNSCACHE dnscache_copy(DNSCACHE f, int r);*/

#ifdef PFS_THREADS
extern p_th_mutex p_th_mutexDNSCACHE;
extern p_th_mutex p_th_mutexALLDNSCACHE;
#endif

#endif /*dnscache_alloc_h*/
