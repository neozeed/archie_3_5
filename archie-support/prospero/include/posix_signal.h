/* Make sure sys/types included here before included by signal.h */
#include <sys/types.h>
#ifdef _POSIX_C_SOURCE
#define OLD_POSIX_C_SOURCE _POSIX_C_SOURCE
#else
#undef OLD_POSIX_C_SOURCE
#endif

#define _POSIX_C_SOURCE 1
#include "/usr/include/signal.h"

#ifdef OLD_POSIX_C_SOURCE
#define _POSIX_C_SOURCE OLD_POSIX_C_SOURCE
#undef OLD_POSIX_C_SOURCE
#else
#undef _POSIX_C_SOURCE
#endif
