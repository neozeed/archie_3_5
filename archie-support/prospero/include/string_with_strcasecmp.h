/*
 * <string_with_strcasecmp.h>
 * Includes standard POSIX/ANSI C <string.h>, plus
 * strcasecmp() prototype.
 */
#ifdef SOLARIS
/* This is for SOLARIS, but may be usefull elsewhere */
/* SOLARIS and GCC conspire to exclude strcasecmp */
#ifdef _XOPEN_SOURCE
#define OLD_XOPEN_SOURCE _XOPEN_SOURCE
#else
#undef OLD_XOPEN_SOURCE
#endif

#define _XOPEN_SOURCE 1
#include "/usr/include/string.h"

#ifdef OLD_XOPEN_SOURCE
#define _XOPEN_SOURCE OLD_XOPEN_SOURCE
#undef OLD_XOPEN_SOURCE
#else
#undef _XOPEN_SOURCE
#endif
#else /* SOLARIS */
#include <string.h>
#endif /* SOLARIS */
